#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/debug.h"

void show_cmd(char **cmd)
{
    int i;

    if (!cmd)
    {
        printf("Cmd is null\n");
        return;
    }
    if (!cmd[0])
    {
        printf("Cmd array is null\n");
        return;
    }
    for(i = 0; cmd[i] != NULL; i++)
        printf("cmd[%i] = %s\n", i, cmd[i]);
}

void show_cmd_list(char ***cmd)
{
    int i, j;

    if (!cmd)
    {
        printf("Cmd is null\n");
        return;
    }
    if (!cmd[0])
    {
        printf("Cmd_list array is null\n");
        return;
    }
    if (!cmd[0][0])
    {
        printf("Cmd_list's first command set is null\n");
        return;
    }
    for(i = 0; cmd[i] != NULL; i++)
        for(j = 0;cmd[i][j] != NULL; j++)
            printf("cmd_list[%i][%i] = %s\n", i, j, cmd[i][j]);
}

/*
 * Displays error message for failed exec call.
 *
 */
void printErrorMessage(char** args, int code)
{
    switch(code) {
        case EACCES:
            printf("Permission DENIED: %s\n", args[0]);
            break;
        case ENOENT:
            printf("Command not found: %s, errno = %d\n", args[0], errno);
            break;
        default:
            printf("Something bad happened. Error code %d\n", code);
            printf("You tried to run: %s\n", args[0]);
    }
}

/*
 * Displays error message to stderr (useful for piped calls).
 *
 */
void printStdErrMessage(const char *func, int line, const char* msg, int code)
{
    switch(code) {
        case EACCES:
            fprintf(stderr, "Permission DENIED: %s\n", msg);
            break;
        case ENOENT:
            fprintf(stderr, "Command not found: %s\n", msg);
            break;
        default:
            fprintf(stderr,
                    "Something bad happened. Error code %d\n",
                    code);
    }
    fprintf(stderr, "%s %d: %s errno: %d\n",
            func, line, msg, errno);
}

