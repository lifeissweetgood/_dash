#define _GNU_SOURCE /* for asprintf */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


#define MAX_CMD_LEN 1000
#define MAX_CMD_ARGS 50
#define ERROR(x) if(x) { printf("ERROR: %s\n", x); exit(1); }
#define ERROR_CLEANUP(x) if(x) { printf("ERROR: %s\n", x); \
                                rc = -__LINE__; goto cleanup; }
#define ASSERT(x) if(x) { printf("ASSERT: Assert failed!\n"); \
                                rc = -__LINE__; goto cleanup; }

// Signal received bool
static bool got_sigint = false;

/**
 * Grabs commands and args and formats them to pass to child process
 * No escape characters - " " is the only delimiter.
 */
static void parseUserInput(const char *userInputStr, char **storeArgs)
{
    unsigned int i;
    unsigned int arg = 0;
    unsigned int string_start = 0;
    unsigned int cur_string_len = 0;
    unsigned int last_was_space = 1;

    for (i = 0; i <= strlen(userInputStr); i++) {
        if ((userInputStr[i] == ' ') || (userInputStr[i] == '\0')) {
            if (!last_was_space) {
                storeArgs[arg] = strndup(&userInputStr[string_start], cur_string_len + 1);
                if (storeArgs[arg] == NULL)
                    ERROR("Failed malloc!\n");
                storeArgs[arg][cur_string_len] = '\0';
                printf("storeArgs[%d] = %s\n", arg, storeArgs[arg]);
                
                arg += 1;
                cur_string_len = 0;
                string_start = i + 1;
            } else { /* multiple spaces in a row */
                string_start += 1;
            }
            last_was_space = 1;
        } else {
            last_was_space = 0;
            cur_string_len += 1;
        }
    }
}
// TODO: Refactor/Combine with parseUserInput
/**
 * Splits pipe commands into separate command chunks
 *
 */
static void parsePipeInput(const char *pipeInputStr, char **storeArgs)
{
    unsigned int i;
    unsigned int arg = 0;
    unsigned int string_start = 0;
    unsigned int cur_string_len = 0;
    unsigned int last_was_pipe = 1;

    for (i = 0; i <= strlen(pipeInputStr); i++) {
        if ((pipeInputStr[i] == '|') || (pipeInputStr[i] == '\0')) {
            if (!last_was_pipe) {
                storeArgs[arg] = strndup(&pipeInputStr[string_start], cur_string_len + 1);
                if (storeArgs[arg] == NULL)
                    ERROR("Failed malloc!\n");
                storeArgs[arg][cur_string_len] = '\0';
                printf("storeArgsPipe[%d] = %s\n", arg, storeArgs[arg]);
                
                arg += 1;
                cur_string_len = 0;
                string_start = i + 2;
            } else { /* multiple pipes in a row */
                string_start += 1;
            }
            last_was_pipe = 1;
            cur_string_len -= 1;
        } else {
            last_was_pipe = 0;
            cur_string_len += 1;
        }
    }
}

/**
 * Removes new line char from user input string.
 *
 */
void removeNewLine(char *oldInputStr, char *newInputStr)
{
    int i = 0;
    for(; oldInputStr[i] != '\n'; i++)
        newInputStr[i] = oldInputStr[i];
    newInputStr[i] = '\0';  // Terminate string properly
}

/*
 * Displays error message for failed exec call.
 *
 */
void printErrorMessage(char** args, int code)
{
    switch(code) {
        case EACCES:
            printf("Permission DENIED: %s\n", args[0]);
            break;
        case ENOENT:
            printf("Command not found: %s\n", args[0]);
            break;
        default:
            printf("Something bad happened. Error code %d\n", code);
            printf("You tried to run: %s\n", args[0]);
    }
}

/**
 * Termination handler for signal.
 *
 */
static void termination_handler(int signum)
{
    (void) signum;
    got_sigint = true;
    printf("\tIt's handled.");
}

/**
 * Registers signal handler.
 *
 */
static void registerSignalHandler() {
    struct sigaction new_action;
    new_action.sa_handler = termination_handler;
    // new_action.sa_handler = SIG_IGN;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction(SIGINT, &new_action, NULL);

}

/**
 * Wrapper for chdir function to change directories
 *
 * NOTE: Changes are not persistent after _dash exits
 */
int changeDir(char *cdCommand)
{
    int i = 0, rc = 0;
    char *cdPathToMoveTo = NULL;
    char *errMsg = NULL;

    cdPathToMoveTo = malloc(sizeof(cdCommand));
   
    // Essentially, we're copying the original command but removing the 'cd'
    // part to grab the path.  We're expecting input in the format of:
    //      cd /path/to/move/to/
    // 
    // We're trusting the user input an awwwwful lot here (see TODO
    // below)
    for(; cdCommand[i] != '\0'; i++)
        cdPathToMoveTo[i] = cdCommand[i+3];
    cdPathToMoveTo[i+1] = '\0';  // Terminate string properly
   
    // Call built-in function to move dirs
    if(chdir(cdPathToMoveTo) < 0)
    {
        rc = -1;
        asprintf(&errMsg, "Can't move to '%s'!", cdPathToMoveTo);
        ERROR_CLEANUP(errMsg);
    }

cleanup:
    free(cdPathToMoveTo);
    free(errMsg);

    return rc;
}

static int array_len(char **array)
{
    int i = 0;

    for(; array[i] != NULL; i++);

    return i;
}

static int execute_fork(char **cmdargv, int pipeExists)
{
    pid_t pid;
    int i = 1, rc = 0, status = 0, tmp_argv_len = 0;
    char **tmp_cmd_argv = NULL;
    char *first_cmd = NULL;
    char **first_cmd_args = NULL;

    //tmp_argv_len = sizeof(char*) * (array_len(cmdargv) - 1);
    tmp_argv_len = sizeof(char*) * (MAX_CMD_ARGS + 1);
    if((tmp_cmd_argv = (char**) malloc(tmp_argv_len)) == NULL)
    {
        ERROR("Failed malloc!\n");
    } else {
        memset(tmp_cmd_argv, '\0', tmp_argv_len);
    }
    if((first_cmd_args = (char**) malloc(tmp_argv_len)) == NULL)
    {
        ERROR("Failed malloc!\n");
    } else {
        memset(first_cmd_args, '\0', tmp_argv_len);
    }

    pid = fork();
    if(pid == 0)  // Child Process
    {
        printf("%d: In child! Pipe exists = %d\n", __LINE__, pipeExists);
            for(i=0; cmdargv[i] != NULL; i++)
                printf("%d cmdargv[%d]: %s\n", __LINE__, i, cmdargv[i]);

        // Single command to execute
        if(!pipeExists)
        {
            // Split args for command, if there are any, then execute
            if(strstr(cmdargv[0], " ") != NULL)
            {
                first_cmd = strndup(cmdargv[0], strlen(cmdargv[0]));
                memset(first_cmd_args, '\0', tmp_argv_len);
                parseUserInput(first_cmd, first_cmd_args);
                if(execvp(first_cmd_args[0], first_cmd_args) < 0)
                    printErrorMessage(first_cmd_args, errno);
            }
            else // ...or just execute.
            {
                if(execvp(cmdargv[0], cmdargv) < 0)
                    printErrorMessage(cmdargv, errno);
            }
        }

        // There's piping involved and multiple commands
        // to parse through still
        printf("%d: In child!\n", __LINE__);
        
        // Split cmdargv into smaller chunk
        for(i=1; cmdargv[i] != NULL; i++)
            tmp_cmd_argv[i-1] = strndup(cmdargv[i], strlen(cmdargv[i]));
        
        // DEBUG
        for(i=0; tmp_cmd_argv[i] != NULL; i++)
            printf("tmp_cmd_argv[%d]: %s\n", i, tmp_cmd_argv[i]);

        // Single out first cmd so it doesn't get forgotten
        first_cmd = cmdargv[0];
        parseUserInput(first_cmd, first_cmd_args);

        // Cmdargv now has the new list that is
        // without the first command
        cmdargv = tmp_cmd_argv;
        
        // DEBUG
        printf("%d: In child!\n", __LINE__);
        for(i=0; cmdargv[i] != NULL; i++)
            printf("cmdargv[%d]: %s\n", i, cmdargv[i]);
        
        // Set pipe up
        
        // If cmdargv only has 1 element, we have reached the base
        // case and thus we can treat it like a single command above
        if(array_len(cmdargv) == 1)
            pipeExists = 0;
        
        /* Recursive calls */
        // Call the single command
        if( execute_fork(first_cmd_args, 0) < 0 )
            ERROR_CLEANUP("Recursive exec_fork() call failed!");

        // Repeat the process for the rest of the commands
        if( execute_fork(cmdargv, pipeExists) < 0 )
            ERROR_CLEANUP("Recursive exec_fork() call failed!");

        // DEBUG
        printf("%d: In child!\n", __LINE__);

        // Child process needs to exit out or else program never exists.
        exit(1);
    }
    else if( pid < 0)  // Fork failed. Boo.
    {
        ERROR("Fork failed\n");
    }
    else  // Parent Process
    {
        printf("Hello.\n");
    }
    
    // Wait on child
    waitpid(pid, &status, 0);

/* Cleanup! */
cleanup:

    return rc;
}

/**
 * Main function
 *
 */
int main(int argc, char* argv[])
{
    int pipefd[2];
    pid_t pid;

    char buf;
    char result_buf[4096];

    int i, cmdargv_len;
    int rc = 0, counter = 0;
    int status = 0;
    int pipeExists = 0;

    char **cmdargv = NULL;
    char *formattedInput = NULL, *userInput = NULL,
         *cdCmd = NULL, *pipe_cmd = NULL;

    if((userInput = malloc(sizeof(char)*MAX_CMD_LEN)) == NULL)
        ERROR("Failed malloc\n");

    cmdargv_len = sizeof(char*) * (MAX_CMD_ARGS + 1);
    if((cmdargv = (char**) malloc(cmdargv_len)) == NULL) {
        ERROR("Failed malloc\n");
    } else {
        memset(cmdargv, '\0', cmdargv_len);
    }

    registerSignalHandler();

    while(1)
    {
        printf("_dash > ");
        got_sigint = false;
        if (fgets(userInput, MAX_CMD_LEN, stdin) == NULL) {
            if (got_sigint) {
              printf("\n");
              continue;
            } else {
              // Ctrl+D
              return 0;
            }
        }

        // TODO: Sanitize user input! We're currently hoping the user is a
        // benelovent, sweet human being. HA!

        formattedInput = malloc(strlen(userInput)+1);
        removeNewLine(userInput, formattedInput);

        // See if user wants out.
        if((strcmp("quit", formattedInput) == 0) ||
           (strcmp("exit", formattedInput) == 0) ||
           (strcmp("q", formattedInput) == 0))
        {
            printf("Quitting!\n");
            goto cleanup;
        }

        // Check to see if user wants to change working directories
        if( (cdCmd = strstr(formattedInput, "cd ")) != NULL )
        {
            // do a strdup here
            cdCmd = strdup(formattedInput);
            if(changeDir(cdCmd) != 0)
                ERROR("cd failed.");
            
            free(cdCmd);

            // No need to fork/exec, can just move on.
            continue;
        }

        if( (pipe_cmd = strstr(formattedInput, "|")) != NULL )
        {
            if(pipe(pipefd) < 0)
                ERROR("Can't create pipe!");
            pipeExists = 1;
            parsePipeInput(formattedInput, cmdargv);
        }
        else
            parseUserInput(formattedInput, cmdargv);

        if( execute_fork(cmdargv, pipeExists) < 0 )
            ERROR("Bad exec!");
        goto cleanup;

        pid = fork();
        if(pid == 0)  // Child Process
        {
            if(pipeExists)
            {
                /* Close read end of pipe */
                if(close(pipefd[0]) < 0)
                    ERROR("Child Proc: Can't close write end of pipe!");
                
                /* Redirect stdout to write end of pipe */
                if(dup2(pipefd[1], STDOUT_FILENO) < 0)
                    ERROR("Dup failed");

                /* Close write end of pipe, don't need it now */
                if(close(pipefd[1]) < 0)
                    ERROR("Child Proc: Can't close read end of pipe!");
            }

            /* Execute command */
            rc = execvp(cmdargv[0], cmdargv);
            if(rc < 0)
                printErrorMessage(cmdargv, errno);

            // Child process needs to exit out or else program never exists.
            exit(1);
        }
        else if( pid < 0)  // Fork failed. Boo.
        {
            free(formattedInput);
            free(userInput);
            free(cmdargv);
            ERROR("Fork failed\n");
        }
        else  // Parent Process
        {
            if(pipeExists)
            {
                /* Close write end of pipe */
                if(close(pipefd[1]) < 0)
                    ERROR("Parent Proc: Can't close write end of pipe!");

                /* Read & spit out output from child process */
                while(read(pipefd[0], &buf, sizeof(buf)) == 1)
                {
                    result_buf[counter] = buf;
                    counter++;
                }
                printf("%s", result_buf);

                /* Close read end of pipe, don't need it anymore */
                if(close(pipefd[0]) < 0)
                    ERROR("Child Proc: Can't close write end of pipe!");
            }
        }

        // Wait on child
        waitpid(pid, &status, 0);

        if(pipeExists)
        {
            // Clear buffer, reset buffer counter
            memset(result_buf, 0, sizeof(result_buf));
            counter = 0;
        }

        // Reset pipe bool
        pipeExists = 0;

        // Reset cmdargv
        memset(cmdargv, '\0', sizeof(char*) * MAX_CMD_ARGS);
    }

/* Cleanup! */
cleanup:
    free(formattedInput);
    free(userInput);
    if (cmdargv) {
        for (i = 0; i < MAX_CMD_ARGS; i++) {
            free(cmdargv[i]); /* free(NULL) is ok with glibc */
        }
    }
    free(cmdargv);

    printf("All finished.\n");

    return 0;

    if(0)
    {
        (void) argc;
        (void) argv;
    }
}
