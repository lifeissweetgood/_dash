#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "../include/parser.h"
#include "../include/debug.h"
#include "../include/communicator.h"

#define MAX_CMD_LEN 1000
#define MAX_CMD_ARGS 100
#define MAX_NUM_PIPES 5

static bool got_sigint = false;

/**
 * Termination handler for signal.
 *
 */
static void termination_handler(int signum)
{
    (void) signum;
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
 * Main function
 *
 */
int main(int argc, char* argv[])
{
    int i, cmdargv_len, status;
    int piperet, pid, pipes_num;
    int rc = 0;
    int pipefd[MAX_NUM_PIPES*2];

    char *formattedInput = NULL;
    char *userInput = NULL;
    char **cmdargv = NULL ;

    char *cdCmd = NULL;
    char ***cmds_to_be_run = NULL;

    if((userInput = malloc(sizeof(char)*MAX_CMD_LEN)) == NULL)
        ERROR("Failed malloc\n");

    registerSignalHandler();

    while(1)
    {
        cmdargv_len = sizeof(char*) * MAX_CMD_ARGS;
        if((cmdargv = (char**) malloc(cmdargv_len)) == NULL) {
            ERROR("Failed malloc\n");
        } else {
            memset(cmdargv, '\0', sizeof(char*) * MAX_CMD_ARGS);
        }

        printf("_dash > ");
        got_sigint = false;
        if (fgets(userInput, MAX_CMD_LEN, stdin) == NULL) {
            if (got_sigint) {
              printf("\tIt's handled.\n");
              continue;
            } else {
              // Ctrl+D
              return 0;
            }
        }

        // Check for empty input
        if(!userInput || (strcmp("", userInput) == 0)
           || (userInput[0] == '\n'))
            continue;

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
        cmds_to_be_run = parse_commands(cmdargv);
        // TODO: check for error

        if((cmds_to_be_run[1] == NULL) ||
           (cmds_to_be_run[1][0] == NULL)) {
            //printf("caller: The *only* command is:\n");
            //show_cmd(cmds_to_be_run[0]);
            
            /* ORIG: rc = run_pipe(pipefd, cmds_to_be_run[0], NULL);*/
            rc = run_pipe(pipefd, 0, cmds_to_be_run);
            if(rc == -1)
                printf("No commands passed to run_pipe\n");
        } else {
            // Need to make sure this never goes over MAX_NUM_PIPES
            pipes_num = num_pipes(cmdargv);
            for(i=0; i < pipes_num; i++) {
                piperet = pipe(pipefd + (i * 2));
                if (piperet == -1) {
                    printf("Piping problems\n");
                    // TODO: handle this
                }
            }

            /* ORIG: rc = run_pipe(pipefd, cmds_to_be_run[0],
             *                     cmds_to_be_run[1]); */
            
            rc = run_pipe(pipefd, pipes_num, cmds_to_be_run);
            if(rc == -1)
                printf("No commands passed to run_pipe\n");
            //ASSERT( rc == 0 );
            
            //close(pipefd[0]);
            //close(pipefd[1]);
            for(i=0; i < pipes_num; i++)
                close(pipefd[i]);
        }

        while((pid = wait(&status)) != -1) {
            //fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));
        }

        //printf("%s %d: About to show commands to be run\n",
        //       __func__, __LINE__);
        //if(cmds_to_be_run)
        //    show_cmd_list(cmds_to_be_run);

        // Free triple star list
        free(cmds_to_be_run);

        // Free list of command strings
        if (cmdargv) {
            for (i = 0; i < MAX_CMD_ARGS; i++) {
                free(cmdargv[i]); /* free(NULL) is ok with glibc */
            }
        }
        free(cmdargv);
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
