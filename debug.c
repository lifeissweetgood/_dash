#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "debug.h"

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


