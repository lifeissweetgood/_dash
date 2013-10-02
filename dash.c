#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define MAX_CMD_LEN 1000
#define MAX_CMD_ARGS 50
#define ERROR(x) if(x) { printf("ERROR: %s\n", x); exit(1); }

/**
 * Grabs commands and args and formats them to pass to child process
 *
 */
void parseUserInput(char *userInputStr, char **storeArgs)
{
    storeArgs[0] = strtok(userInputStr, " ");
    int i = 1;
    for(; storeArgs[i] = strtok(NULL, " "), storeArgs[i] != NULL; i++)
    {}
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
void termination_handler(int signum)
{
    printf("I'M GOING TO KILL SOMEONE!!!!1\n");
    exit(1);
}

/**
 * Registers signal handler.
 *
 */
void registerSignalHandler() {
    struct sigaction new_action, old_action;
    new_action.sa_handler = termination_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction (SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGINT, &new_action, NULL);
}

/**
 * Main function
 *
 */
int main(int argc, char* argv[])
{
    int i;
    int pipefd[2];
    pid_t pid;
    char buf;

    int rc = 0;
    int status = 0;
    int pipeExists = 0;

    char *formattedInput = NULL;
    char *userInput = malloc(sizeof(char)*MAX_CMD_LEN);
    char **cmdargv = (char**) malloc(sizeof(char*) * MAX_CMD_ARGS);

    char *cdCmd = NULL;
    char *cdPathToMoveTo = NULL;

    char *errMsg = NULL;

    registerSignalHandler();

    while(1)
    {
        printf("_dash > ");
        fgets(userInput, MAX_CMD_LEN, stdin);

        // TODO: Sanitize user input! We're currently hoping the user is a
        // benelovent, sweet human being. HA!

        formattedInput = malloc(sizeof(userInput));
        removeNewLine(userInput, formattedInput);

        // See if user wants out.
        if(strcmp("quit", formattedInput) == 0)
        {
            printf("Quitting!\n");
            goto cleanup;
        }

        // Check to see if user wants to change working directories
        cdCmd = strstr(formattedInput, "cd ");
        if(cdCmd)
        {
            cdPathToMoveTo = malloc(sizeof(cdCmd));
           
            // We're trusting the user input an awwwwful lot here (see TODO
            // above)
            for(i=0; cdCmd[i] != '\0'; i++)
                cdPathToMoveTo[i] = cdCmd[i+3];
            cdPathToMoveTo[i+1] = '\0';
            
            //printf("Path = %s\n", cdPathToMoveTo);

            if(chdir(cdPathToMoveTo) < 0)
            {
                asprintf(&errMsg, "Can't move to '%s'!", cdPathToMoveTo);
                ERROR(errMsg);
            }

            // No need to fork/exec, can just move on.
            continue;
        }

        if(strcmp("pipe", formattedInput) == 0)
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
                dup2(STDOUT_FILENO, pipefd[1]);
                if(close(pipefd[0]) < 0)
                    ERROR("Child Proc: Can't close read end of pipe!");
            }

            rc = execvp(cmdargv[0], cmdargv);
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
            free(formattedInput);
            free(userInput);
            free(cmdargv);
            ERROR("Fork failed\n");
        }
        else  // Parent Process
        {
            if(pipeExists)
            {
                if(close(pipefd[1]) < 0)
                    ERROR("Parent Proc: Can't close write end of pipe!");
                
                while(read(pipefd[0], &buf, 1) == 1)
                    printf("%c", buf);
                
                if(close(pipefd[0]) < 0)
                    ERROR("Parent Proc: Can't close read end of pipe!");
            }
        }

        waitpid(pid, &status, 0);
        pipeExists = 0;
    }

/* Cleanup! */
cleanup:
    free(formattedInput);
    free(userInput);
    free(cmdargv);

    printf("All finished.\n");

    return 0;
}
