#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int parseUserInput(char *userInputStr, char **storeArgs[])
{
    char *try[] = {"ls", "-l", "-a"};
    *storeArgs = try;
}

int main(int argc, char* argv[])
{
    pid_t pid;
    int rc = 0;
    int status = 0;
    char *userinput = NULL;
    //char *const try[] = {"ls", "-l", "-a"};
    char **try;

    while(!userinput || (strcmp("quit", userinput) != 0))
    {
        userinput = malloc(sizeof(char)*100);
        printf("_dash > ");
        scanf("%s", userinput);

        parseUserInput(userinput, &try);
        printf("try 0: %s\n", try[0]);
        printf("try 1: %s\n", try[1]);
        printf("try 2: %s\n", try[2]);
        pid = fork();

        if(pid == 0) // Child
        {
            printf("I'm the child!\n");
            rc = execvp("ls", try);
            if(rc < 0)
            {
                printf("Failed in child process!\n");
                exit(1);
            }
        }
        else if( pid < 0) // failed fork
        {
            printf("Fork failed\n");
            exit(1);
        }
        else
        {
            printf("I'm the parent!\n");
        }
        waitpid(pid, &status, 0);
    }
    printf("All finished.\n");
}
