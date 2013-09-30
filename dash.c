#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_CMD_LEN 100

int parseUserInput(char *userInputStr, char **storeArgs)
{
    // char *try[] = {"ls", "-l", "-a"};
    storeArgs[0] = strtok(userInputStr, " \n");
    int i;
    for (i = 1;
        storeArgs[i] = strtok(NULL, " \n"), storeArgs[i] != NULL;
        i++) {}
}

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

int main(int argc, char* argv[])
{
    pid_t pid;
    int rc = 0;
    int status = 0;
    char *userinput = NULL;
    //char *const try[] = {"ls", "-l", "-a"};
    char **try = (char**) malloc(sizeof(char*) * 10);

    while(1)
    {
        userinput = malloc(sizeof(char)*MAX_CMD_LEN);
        printf("_dash > ");
        fgets(userinput, MAX_CMD_LEN, stdin);
        if (strcmp("quit\n", userinput) == 0) {
          break;
        }

        parseUserInput(userinput, try);
        pid = fork();

        if(pid == 0) // Child
        {
            // printf("I'm the child!\n");
            rc = execvp(try[0], try);
            if(rc < 0)
            {
                printErrorMessage(try, errno);
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
            // printf("I'm the parent!\n");
        }
        waitpid(pid, &status, 0);
    }
    printf("All finished.\n");
}
