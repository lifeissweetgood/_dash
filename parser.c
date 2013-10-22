#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.h"
#include "debug.h"

/**
 * Grabs commands and args and formats them to pass to child process
 * No escape characters - " " is the only delimiter.
 */
void parseUserInput(const char *userInputStr, char **storeArgs)
{
    unsigned int i;
    unsigned int arg = 0;
    unsigned int string_start = 0;
    unsigned int cur_string_len = 0;
    unsigned int last_was_space = 1;

    for (i = 0; i <= strlen(userInputStr); i++) {
        if ((userInputStr[i] == ' ') || (userInputStr[i] == '\0')) {
            if (!last_was_space) {
                storeArgs[arg] = strndup(&userInputStr[string_start], cur_string_len + 1);
                if (storeArgs[arg] == NULL)
                    ERROR("Failed malloc!\n");
                storeArgs[arg][cur_string_len] = '\0';
                printf("storeArgs[%d] = %s\n", arg, storeArgs[arg]);
                
                arg += 1;
                cur_string_len = 0;
                string_start = i + 1;
            } else { /* multiple spaces in a row */
                string_start += 1;
            }
            last_was_space = 1;
        } else {
            last_was_space = 0;
            cur_string_len += 1;
        }
    }
}

/* This function assumes userInputStr is immutable...
and shares the actual string data with it */
char ***parse_commands(char **userInputStr)
{
    int i = 0;
    int j, cmdlen, cmd_start;
    int pipes = 0;
    int cmdi = 0;
    int command_parts = 0;
    const char *tmp;
    char **command = NULL;
    char ***commands = NULL;

    for (i = 0; userInputStr[i] != NULL; i++) {
        tmp = userInputStr[i];
        if (tmp && (tmp[0] == '|') && (tmp[1] == '\0'))
            pipes += 1;
    }

    cmdlen = sizeof(char*) * (pipes + 2);
    commands = malloc(cmdlen);
    // TODO: check for failure
    memset(commands, '\0', cmdlen);

    printf("got here\n");
    cmd_start = 0;
    for(i = 0; userInputStr[i] != NULL; i++) {
        tmp = userInputStr[i];
        
        // Case of 1 command
        if((userInputStr[0] != NULL) && (userInputStr[1] == NULL))
            command_parts = 1;

        if ((userInputStr[i+1] == NULL) || ((tmp[0] == '|') && (tmp[1] == '\0'))) {
            cmdlen = sizeof(char*) * (command_parts + 1);
            command = malloc(cmdlen);
            // TODO: check for failure
            memset(command, '\0', cmdlen);
            // TODO: remove printf("found a command\n");
            printf("cmd_parts = %d\n", command_parts);
            for(j = 0; j < command_parts; j++) {
                command[j] = userInputStr[cmd_start + j];
                // TODO: remove printf("command[%i] is %s\n", j, command[j]);
                printf("command[%i] is %s\n", j, command[j]);
            }
            command[j] = NULL;
            cmd_start = i + 1;
            commands[cmdi] = command;
            cmdi += 1;
            printf("%d\n", __LINE__);
            show_cmd(command);
            // TODO: remove show_cmd(command);
        }
        else {
            command_parts += 1;
        }
    }
    printf("done\n");
    return commands;
}

/**
 * Removes new line char from user input string.
 *
 */
void removeNewLine(char *oldInputStr, char *newInputStr)
{
    int i = 0;
    for(; oldInputStr[i] != '\n'; i++)
        newInputStr[i] = oldInputStr[i];
    newInputStr[i] = '\0';  // Terminate string properly
}

