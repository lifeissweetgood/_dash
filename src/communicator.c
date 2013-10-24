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

//int run_pipe(int pipefd[], char **cmd1, char **cmd2)
int run_pipe(int pipefd[], char ***cmd_list)
{
    int cpid1, cpid2;
    int rc = 0, counter = 1;

    /*printf("show 1\n");
    show_cmd(cmd1);
    printf("show 2\n");
    show_cmd(cmd2);*/

    // Need at least 1 command.
    if (cmd_list[0] == NULL) return -1;

    // Handle just 1 command passed
    if((cmd_list[1] == NULL) || (cmd_list[1][0] == NULL)) {
        cpid1 = fork();
        if(cpid1 == 0) {
            rc = execvp(cmd_list[0][0], cmd_list[0]);
            if(rc < 0)
                printErrorMessage(cmd_list[0], errno);
            _exit(EXIT_SUCCESS);  
        }
    } else { // Handle piping
        //while(cmd_list[counter] != NULL)
        while(counter < 2)
        {
            cpid2 = fork();
            if (cpid2 == 0) { // set up the reader child
                printf("kid2 launched, it'll be %s!\n", cmd_list[counter][0]);
                //show_cmd(cmd_list[counter]);
                //printf("%s %d\n",__func__, __LINE__);
                
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);

                rc = execvp(cmd_list[counter][0],
                            cmd_list[counter]);
                if(rc < 0)
                        printErrorMessage(cmd_list[counter], errno);
            }
            
            cpid1 = fork();
            if (cpid1 == 0) { // set up the writer child
                printf("kid1 launched, it'll be %s!\n",
                       cmd_list[counter -1][0]);
                //show_cmd(cmd_list[counter - 1]);
                //printf("%s %d\n",__func__, __LINE__);
                
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                
                /*printf("%s %d cmd_list[0][0] = %s\n",
                       __func__, __LINE__,
                       cmd_list[counter - 1][0]);*/
                rc = execvp(cmd_list[counter - 1][0],
                            cmd_list[counter - 1]);
                if(rc < 0)
                        printErrorMessage(cmd_list[counter - 1], errno);
            }
            
            if ((cpid1 == -1) || (cpid2 == -1)) {
                // TODO: handle this
            }
            counter++;
        }
    }

    //printf("Yay, kids done\n");
    return 0;
}

