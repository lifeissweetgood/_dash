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

void show_cmd(char **cmd) {
    int i;

    if (!cmd) return;
    printf("showing command\n");
    for(i = 0; cmd[i] != NULL; i++)
        printf("cmd[%i] = %s\n", i, cmd[i]);
}

void show_error(char *extrainfo, int errnum) {
    char *err = strerror(errnum);

    if (extrainfo != NULL)
        printf("%s\n", extrainfo);
    printf("%s\n", err);
}

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

/* This function assumes userInputStr is immutable...
and shares the actual string data with it */
char ***parse_commands(char **userInputStr)
{
    int i = 0;
    int j, cmdlen, cmd_start;
    int pipes = 0;
    int cmdi = 0;
    int command_parts = 0;
    const char *tmp;
    char **command = NULL;
    char ***commands = NULL;

    for (i = 0; userInputStr[i] != NULL; i++) {
        tmp = userInputStr[i];
        if (tmp && (tmp[0] == '|') && (tmp[1] == '\0'))
            pipes += 1;
    }

    cmdlen = sizeof(char*) * (pipes + 2);
    commands = malloc(cmdlen);
    if (commands == NULL) {
        show_error("Mallocing commands failed", errno);
        return NULL;
    }

    memset(commands, '\0', cmdlen);

    cmd_start = 0;
    for(i = 0; userInputStr[i] != NULL; i++) {
        tmp = userInputStr[i];
        if ((userInputStr[i+1] == NULL) || ((tmp[0] == '|') && (tmp[1] == '\0'))) {
            cmdlen = sizeof(char*) * (command_parts + 1);
            command = malloc(cmdlen);
            if (command == NULL) { /* TODO: this leaks memory! */
                show_error("Mallocing command failed", errno);
                return NULL;
            }

            memset(command, '\0', cmdlen);
            for(j = 0; j < command_parts; j++) {
                command[j] = userInputStr[cmd_start + j];
            }
            command[j] = NULL;
            cmd_start = i + 1;
            commands[cmdi] = command;
            cmdi += 1;
        }
        else {
            command_parts += 1;
        }
    }
    printf("done\n");
    return commands;
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


int run_pipe(char **cmd1, char **cmd2) {
    int pipefd[2];
    int cpid1, cpid2;
    int piperet, rc;

    /*printf("show 1\n");
    show_cmd(cmd1);
    printf("show 2\n");
    show_cmd(cmd2);*/
    piperet = pipe(pipefd);
    if (piperet == -1) {
        show_error("Creating a pipe failed", errno);
        return -1;
    }
    
    cpid1 = fork();
    if (cpid1 == 0) { // set up the reader child
        //TODO: remove printf("kid1 launched, it'll be %s!\n", cmd2[0]);
        close(pipefd[1]);
        dup2(pipefd[0], 0);

        rc = execvp(cmd2[0], cmd2);
        if(rc < 0)
                printErrorMessage(cmd1, errno);
    }
    
    cpid2 = fork();
    if (cpid2 == 0) { // set up the writer child
        // TODO: remove printf("kid2 launched!\n");
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        
        rc = execvp(cmd1[0], cmd1);
        if(rc < 0)
                printErrorMessage(cmd2, errno);
    }
    
    if ((cpid1 == -1) || (cpid2 == -1)) {
        show_error("Forking off a child failed", errno);
        return -1;
    }

    wait(NULL);
    return 0;
}

/**
 * Main function
 *
 */
int main(int argc, char* argv[])
{
    int i, cmdargv_len;
    int rc = 0;

    char *formattedInput = NULL;
    char *userInput = NULL;
    char **cmdargv = NULL ;

    char *cdCmd = NULL;
    char ***commands = NULL;

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

        parseUserInput(formattedInput, cmdargv);
        commands = parse_commands(cmdargv);
        if (commands == NULL) {
            show_error("Parsing commands failed", errno);
            goto cleanup;
        }

        rc = run_pipe(commands[0], commands[1]);
        ASSERT( rc != 0 );
    }

/* Cleanup! */
cleanup:
    free(formattedInput);
    free(userInput);
    if (commands) {
        for (i = 0; commands[i] != NULL; i++)
            free(commands[i]);
    }
    free(commands);

    if (cmdargv) {
        for (i = 0; i < MAX_CMD_ARGS; i++) {
            free(cmdargv[i]); /* free(NULL) is ok with glibc */
        }
    }
    free(cmdargv);

    printf("All finished.\n");

    return 0;
}
