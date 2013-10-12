#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define MAX_CMD_LEN 1000
#define MAX_CMD_ARGS 100
#define ERROR(x) if(x) { printf("%s: %d: ERROR %s\n", \
                                __func__, __LINE__, x); exit(1); }
#define ERROR_CLEANUP(x) if(x) { printf("%s: %d: ERROR %s\n", \
                                        __func__, __LINE__, x); \
                                rc = -__LINE__; goto cleanup; }
#define ASSERT(x) if(x) { printf("ASSERT: %d: Assert failed!\n", __LINE__); \
                          rc = -__LINE__; goto cleanup; }


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
int printErrorMessage(char** args, int code)
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
    got_sigint = true;
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

    if( (cdPathToMoveTo = malloc(sizeof(cdCommand))) == NULL)
        ERROR("Failed malloc!\n");
   
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
        asprintf(&errMsg, "Can't move to '%s'!", cdPathToMoveTo);
        ERROR_CLEANUP(errMsg);
    }

cleanup:
    free(cdPathToMoveTo);
    free(errMsg);

    return rc;
}

/**
 * Executes the fork
 */
int execute_fork(char **cmdargv, int pipeExists)
{
    char buf;
    int pipefd[2];
    pid_t pid;

    int j;
    int i = 0, cursor = 0, rc = 0, status = 0;

    char **subCmdArgv = NULL;

    while(cmdargv[cursor] != NULL)
    {
        for(j=0; cmdargv[j] != NULL; j++)
            printf("CHECKINGCHECKING %d: cmdargv[%d]: %s\n", __LINE__, j, cmdargv[j]);
        if(pipeExists)
        {
            if(pipe(pipefd) < 0)
                ERROR("Can't create pipe!");
        }

        if((subCmdArgv = (char**) malloc(sizeof(char*) * j)) == NULL)
            ERROR("Failed malloc!\n");

        for(j=0; cmdargv[j] != NULL; j++)
            printf("CHECKINGCHECKING %d: cmdargv[%d]: %s\n", __LINE__, j, cmdargv[j]);

        // Parse out each command separated by a | (pipe)
        for(i = 0; cmdargv[i+cursor] != NULL; i++)
        {
            if(strcmp("|", cmdargv[i+cursor]) == 0)
                break;
            subCmdArgv[i] = strdup(cmdargv[i+cursor]);
        }

        cursor += (i+1); // holds the next position in cmdargv after the |

        pid = fork();
        if(pid == 0)  // Child Process
        {
            if(pipeExists)
            {
                dup2(STDOUT_FILENO, pipefd[1]);
                if(close(pipefd[0]) < 0)
                    ERROR("Child Proc: Can't close read end of pipe!");
            }

            rc = execvp(subCmdArgv[0], subCmdArgv);
            if(rc < 0)
                printErrorMessage(cmdargv, errno);

            if(pipeExists)
            {
                if(close(pipefd[1]) < 0)
                    ERROR("Child Proc: Can't close write end of pipe!");
            }

            // Child process needs to exit out or else program never exists.
            exit(1);
        }
        else if( pid < 0)  // Fork failed. Boo.
        {
            ERROR_CLEANUP("Fork failed\n");
        }
        else  // Parent Process
        {
            if(pipeExists)
            {
                if(close(pipefd[1]) < 0)
                    ERROR_CLEANUP("Parent Proc: Can't close write end of pipe!");
                
                while(read(pipefd[0], &buf, 1) == 1)
                    printf("%c", buf);
                
                if(close(pipefd[0]) < 0)
                    ERROR_CLEANUP("Parent Proc: Can't close read end of pipe!");
            }
        }
        
        // Wait on child process
        waitpid(pid, &status, 0);

        // Since we did a strdup above for each command arg, we have to free it
        // before getting the next set of commands.
        for(j=0; subCmdArgv[j] != NULL; j++)
        {
            printf("subcmdargv[%d]: %s\n", j, subCmdArgv[j]);
            free(subCmdArgv[j]);
            subCmdArgv[j] = NULL;
        }

    } // end while loop

cleanup:
    free(subCmdArgv);
    subCmdArgv = NULL;

    return rc;
}

/**
 * Main function
 *
 */
int main(int argc, char* argv[])
{
    int i, j, cmdargv_len;
    int rc = 0;
    int pipeExists = 0;

    char *formattedInput = NULL;
    char *userInput = NULL;
    char **cmdargv = NULL ;

    char *cdCmd = NULL;
    char *pipeCmd = NULL;

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

        if( (formattedInput = malloc(sizeof(userInput))) == NULL )
            ERROR("Failed malloc\n");
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

        // Check to see if user wants to pipe commands
        if( (pipeCmd = strstr(formattedInput, "|")) != NULL )
        {
            pipeExists = 1;

            // Don't need to free pipeCmd bc freeing formattedInput will take
            // care of that for us.
            //free(pipeCmd);
        }

        parseUserInput(formattedInput, cmdargv);

        for(j=0; cmdargv[j] != NULL; j++)
            printf("%d: cmdargv[%d]: %s\n", __LINE__, j, cmdargv[j]);

        rc = execute_fork(cmdargv, pipeExists);
        ASSERT( rc != 0 );

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
}
