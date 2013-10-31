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

void close_pipes(int pipefd[], int num_pipes)
{
    int i = 0;
    for(; i < num_pipes*2; i++)
        close(pipefd[i]);
}

int run_pipe(int pipefd[], int pipes_num, char ***cmd_list)
{
    int /*fd_counter, cmd_counter,*/ cmd_list_len, pipe_fd_total,
        final_cmd_pos;
    int cpid1, cpid2, cpid3;

    cmd_list_len = three_d_array_len(cmd_list);
    printf("%d: Cmd_list length = %d\n", __LINE__, cmd_list_len); 

    // Need at least 1 command.
    if( cmd_list[0] == NULL ) return -1;

    // Handle just 1 command passed
    if(pipes_num == 0)
    {
        cpid1 = fork();
        if(cpid1 == 0)
        {
            if(execvp(cmd_list[0][0], cmd_list[0]) < 0)
                printErrorMessage(cmd_list[0], errno);
            _exit(EXIT_SUCCESS);  
        }
    }
    else // Handle piping
    {
        // First child, always executes first command regardless of
        // number of pipes
        cpid1 = fork();
        if(cpid1 == 0)
        {
            printf("Child 0 launched with command %s\n", cmd_list[0][0]);

            if(dup2(pipefd[1], STDOUT_FILENO) < 0)
                printStdErrMessage(__func__, __LINE__,
                                   "CHILD 0: Dup failed!", errno);
            close_pipes(pipefd, pipes_num);

            if( execvp(cmd_list[0][0], cmd_list[0]) < 0)
                printErrorMessage(cmd_list[0], errno);
            _exit(EXIT_SUCCESS);  
        }

        // Middle child[ren]
        // If cmd_list > 2, then multiple pipes are in order.
        if(cmd_list_len > 2)
        {
            cpid2 = fork();
            if(cpid2 == 0)
            {
                printf("Child 2 launched with command %s\n", cmd_list[1][0]);
                if(dup2(pipefd[0], STDIN_FILENO) < 0)
                    printStdErrMessage(__func__, __LINE__,
                                       "CHILD 2: Dup STDIN failed!", errno);
                if(dup2(pipefd[3], STDOUT_FILENO) < 0)
                    printStdErrMessage(__func__, __LINE__,
                                       "CHILD 2: Dup STDOUT failed!", errno);
                close_pipes(pipefd, pipes_num);

                if( execvp(cmd_list[1][0], cmd_list[1]) < 0)
                    printErrorMessage(cmd_list[1], errno);
                _exit(EXIT_SUCCESS);  
            }
            printf("Making it here %d\n", __LINE__); 
        }
        printf("Making it here %d\n", __LINE__); 
        
        // Last child, executed regardless of num of pipes
        cpid3 = fork();
        if(cpid3 == 0)
        {
            final_cmd_pos = cmd_list_len - 1;
            printf("Child 1 launched with command %s\n",
                   cmd_list[final_cmd_pos][0]);
            
            pipe_fd_total = (pipes_num*2);

            if(dup2(pipefd[pipe_fd_total - 2], STDIN_FILENO) < 0)
                printStdErrMessage(__func__, __LINE__,
                                   "CHILD 1: Dup STDIN failed!", errno);
            close_pipes(pipefd, pipes_num);

            if( execvp(cmd_list[final_cmd_pos][0],
                       cmd_list[final_cmd_pos]) < 0)
                printErrorMessage(cmd_list[final_cmd_pos], errno);
            _exit(EXIT_SUCCESS);  
        }
    }

    close_pipes(pipefd, pipes_num);

    return 0;
}
