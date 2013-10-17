#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
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
    int pipefd[2];
    pid_t pid;
    //char buf;
    char buf[4096];
    FILE *fd_pipe = NULL;

    int count;
    int rc = 0;
    int status = 0;
    int pipeExists = 0;

    char *formattedInput = NULL;
    char *userInput = malloc(sizeof(char)*MAX_CMD_LEN);
    char **cmdargv = (char**) malloc(sizeof(char*) * MAX_CMD_ARGS);

    registerSignalHandler();

    while(1)
    {
        count = 1;
        printf("_dash > ");
        fgets(userInput, MAX_CMD_LEN, stdin);

        formattedInput = malloc(sizeof(userInput));
        removeNewLine(userInput, formattedInput);

        if(strcmp("quit", formattedInput) == 0)
        {
            printf("Quitting!\n");
            goto cleanup;
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

                if((fd_pipe = fdopen(pipefd[0], "r")) == NULL)
                    ERROR("Can't open file pointer for pipe!\n");

                /* Read & spit out output from child process */
                while(fgets(buf, sizeof(buf), fd_pipe) != NULL)
                {
                    //printf("Parent %d: %s\n",count, buf);
                    printf("%s",buf);
                    //count++;
                }

                /* Close read end of pipe, don't need it anymore */
                if(fclose(fd_pipe) != 0)
                {
                    perror("DAPH");
                    ERROR("Can't close file pointer to pipe!\n");
                }
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
