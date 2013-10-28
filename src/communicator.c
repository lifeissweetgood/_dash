#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "../include/debug.h"
#include "../include/parser.h"
#include "../include/communicator.h"

/**
 * Writer child
 *
 * @returns pid
 */
int write_child(int pipe_fd[], char **command)
{
    int cpid = fork();

    if( cpid == 0 ) {
        printf("Writer child launched with command %s!\n", command[0]);

        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        if( execvp(command[0], command) < 0)
            printErrorMessage(command, errno);
    }

    return cpid;
}

/**
 * Reader Child
 *
 * @returns pid
 */
int read_child(int pipe_fd[], char **command, int execute)
{
    int cpid = fork();

    if( cpid == 0 ) {
        printf("Reader child launched\n");

        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);

        if( execute && (command != NULL) ) {
            printf("Reader child executing command %s!\n", command[0]);
            if( execvp(command[0], command) < 0)
                printErrorMessage(command, errno);
        }
    }

    return cpid;
}

int run_pipe(int pipefd[], char ***cmd_list)
{
    int cpid1, cpid2, cpid3, cpid4;

    // Need at least 1 command.
    if( cmd_list[0] == NULL ) return -1;

    // Handle just 1 command passed
    if((cmd_list[1] == NULL) || (cmd_list[1][0] == NULL)) {
        cpid1 = fork();
        if(cpid1 == 0) {
            if(execvp(cmd_list[0][0], cmd_list[0]) < 0)
                printErrorMessage(cmd_list[0], errno);
            _exit(EXIT_SUCCESS);  
        }
    } else { // Handle piping
        //cpid4 = read_child(pipefd, NULL, 0);
        //cpid3 = read_child(pipefd, cmd_list[2], 1);
        //close(pipefd[0]);
        //close(pipefd[1]);
        //pipefd += 2;
        cpid2 = read_child(pipefd, cmd_list[1], 1);
        cpid1 = write_child(pipefd, cmd_list[0]);
        close(pipefd[0]);
        close(pipefd[1]);

        if((cpid1 == -1) || (cpid2 == -1)) {
           //|| (cpid3 == -1)) {  //|| (cpid4 == -1)) {
            // TODO: handle this
        }
    }

    return 0;
}
