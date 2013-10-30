#include <stdio.h>
#include <stdlib.h>

//int run_pipe(int pipefd[], char **cmd1, char **cmd2);
//int run_pipe(int pipefd[], char ***cmd_list);
int run_pipe(int pipefd[], int pipes_num, char ***cmd_list);
