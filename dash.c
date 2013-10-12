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
#define ERROR_CLEANUP(x) if(x) { printf("ERROR: %s\n", x); goto cleanup; }

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

    int counter = 0;
    int i, cmdargv_len;
    int rc = 0;
    int status = 0;
    int pipeExists = 0;

    char *formattedInput = NULL;
    char *userInput = NULL;
    char **cmdargv = NULL;
    char *cdCmd = NULL;

    if((userInput = malloc(sizeof(char)*MAX_CMD_LEN)) == NULL)
        ERROR("Failed malloc\n");

    cmdargv_len = sizeof(char*) * MAX_CMD_ARGS;
    if((cmdargv = (char**) malloc(cmdargv_len)) == NULL) {
        ERROR("Failed malloc\n");
    } else {
        memset(cmdargv, '\0', sizeof(char*) * MAX_CMD_ARGS);
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

        formattedInput = malloc(sizeof(userInput));
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
            if(changeDir(cdCmd) != 0)
                ERROR("cd failed.");

            free(cdCmd);

            // No need to fork/exec, can just move on.
            continue;
        }

        if(strcmp("ls", formattedInput) == 0)
        {
            if(pipe(pipefd) < 0)
                ERROR("Can't create pipe!");
            pipeExists = 1;
        }

        parseUserInput(formattedInput, cmdargv);

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

        // Clear buffer, reset buffer counter
        memset(result_buf, 0, sizeof(result_buf));
        counter = 0;

        // Reset pipe bool
        pipeExists = 0;
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
