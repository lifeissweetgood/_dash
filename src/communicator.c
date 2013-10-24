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

int run_pipe(int pipefd[], char **cmd1, char **cmd2)
{
    int cpid1, cpid2;
    int rc;

    printf("show 1\n");
    show_cmd(cmd1);
    printf("show 2\n");
    show_cmd(cmd2);

    // Need at least 1 command.
    if (cmd1 == NULL) return -1;

    // Handle just 1 command passed
    if((cmd2 == NULL) || (cmd2[0] == NULL)) {
        printf("%d: Only 1 command present\n", __LINE__);
        cpid1 = fork();
        if(cpid1 == 0) {
            rc = execvp(cmd1[0], cmd1);
            if(rc < 0)
                printErrorMessage(cmd1, errno);
            _exit(EXIT_SUCCESS);  
        }
    } else { // Handle piping
        cpid1 = fork();
        if (cpid1 == 0) { // set up the reader child
            printf("kid1 launched, it'll be %s!\n", cmd2[0]);
            show_cmd(cmd2);
            printf("%d\n", __LINE__);
            close(pipefd[1]);
            //fflush(stdin);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            rc = execvp(cmd2[0], cmd2);
            if(rc < 0)
                    printErrorMessage(cmd1, errno);
        }
        
        cpid2 = fork();
        if (cpid2 == 0) { // set up the writer child
            printf("kid2 launched!\n");
            close(pipefd[0]);
            //fflush(stdout);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            
            rc = execvp(cmd1[0], cmd1);
            if(rc < 0)
                    printErrorMessage(cmd2, errno);
        }
        
        if ((cpid1 == -1) || (cpid2 == -1)) {
            // TODO: handle this
        }
    }

    //wait(NULL);
    //waitpid(-1, &status, 0);
    //fflush(stdout);
    //waitpid(cpid1, NULL, NULL);
    //waitpid(cpid2, NULL, NULL);
    printf("Yay, kids done\n");
    return 0;
}

