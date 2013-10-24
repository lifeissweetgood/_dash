#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/parser.h"
#include "../include/debug.h"

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
                //printf("storeArgs[%d] = %s\n", arg, storeArgs[arg]);
                
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

int two_d_array_len(char **two_d_array)
{
    int i = 0;

    for(; two_d_array[i] != NULL; i++);

    return i;
}

int num_pipes(char **userInputStr)
{
    int i = 0, total_num_pipes = 0;
    const char *tmp = NULL;

    for(; userInputStr[i] != NULL; i++) {
        tmp = userInputStr[i];
        if (tmp && (tmp[0] == '|') && (tmp[1] == '\0'))
            total_num_pipes += 1;
    }

    return total_num_pipes;
}

/* This function assumes userInputStr is immutable...
and shares the actual string data with it */
char ***parse_commands(char **userInputStr)
{
    int i, j, pipes, array_len,
        cmdlen = 0, cmd_start = 0,
        cmdi = 0, command_parts = 0;

    const char *tmp = NULL;
    char **command = NULL;
    char ***cmds_to_be_run = NULL;

    // Use number of pipes to determine how many commands need to be executed
    pipes = num_pipes(userInputStr);

    // Allocate enough space to accomodate the number of commands that need to
    // be run
    cmdlen = sizeof(char*) * (pipes + 1);
    if( (cmds_to_be_run = malloc(cmdlen)) == NULL)
        ERROR("Malloc cmds_to_be_run failed!");
    memset(cmds_to_be_run, '\0', cmdlen);

    if( pipes == 0 )    // Single command with args, no pipes
    {
        // Allocating space for command to go into bigger cmds_to_be_run array
        array_len = two_d_array_len(userInputStr);
        cmdlen = sizeof(char *) * (array_len + 1);
        command = malloc(cmdlen);
        memset(command, '\0', cmdlen);

        // Copy command over
        for(i = 0; userInputStr[i] != NULL; i++)
        {
            tmp = userInputStr[i];
            command[i] = strndup(tmp, strlen(tmp));
            // DEBUG: Sanity check
            printf("%s %d: command[%i] is %s\n", __func__,
                __LINE__, i, command[i]);
        }
        command[i] = NULL;

        // Now add command to cmds_to_be_run
        cmds_to_be_run[0] = command;
    }
    else    // Multiple commands with pipes
    {
        for(i = 0; userInputStr[i] != NULL; i++) {
            tmp = userInputStr[i];
            printf("%s %d: userInputStr[%i] = tmp -> %s\n",
                   __func__, __LINE__, i, tmp);
            
            if (((userInputStr[i+1] == NULL)) ||
                ((tmp[0] == '|') && (tmp[1] == '\0')))
            {
                cmdlen = sizeof(char*) * (command_parts+1);
                command = malloc(cmdlen);
                // TODO: check for failure
                memset(command, '\0', cmdlen);
                
                // TODO: remove printf("found a command\n");
                printf("%s %d: cmd_parts = %d\n", __func__,
                    __LINE__, command_parts);

                for(j = 0; j < (command_parts + 1); j++)
                {
                    printf("%s %d: cmd_parts = %d\n", __func__,
                        __LINE__, command_parts);
                    printf("%s %d: userInputStr[%d] = %s cmd-start = %d j = %d\n",
                           __func__, __LINE__,cmd_start + j,
                           userInputStr[cmd_start + j], cmd_start, j);
                    
                    if( strcmp("|", userInputStr[cmd_start + j]) != 0)
                        command[j] = strdup(userInputStr[cmd_start + j]);
                    
                    // TODO: remove printf("command[%i] is %s\n", j, command[j]);
                    printf("%s %d: userinputstrJ[%i] is %s\n",
                           __func__, __LINE__, j, userInputStr[j]);
                    printf("%s %d: userinputstr_cmd-start+J[%i] is %s\n",
                           __func__, __LINE__,
                           j, userInputStr[cmd_start + j]);
                    printf("%s %d: command[%i] is %s\n", __func__,
                        __LINE__, j, command[j]);
                }
                command[j] = NULL;
                cmd_start = i + 1;
                cmds_to_be_run[cmdi] = command;
                cmdi += 1;
                command_parts = 0;
                printf("%s %d\n",__func__, __LINE__);
                show_cmd(command);
                printf("%s %d\n",__func__, __LINE__);
                show_cmd_list(cmds_to_be_run);
                printf("%s %d\n",__func__, __LINE__);
                // TODO: remove show_cmd(command);
            }
            else
            {
                printf("%s %d: BEFORE incrementing cmd_parts = %d\n",
                       __func__, __LINE__, command_parts);
                printf("%s %d: userinputstr_i[%i] is %s\n",
                       __func__, __LINE__, i, userInputStr[i]);
                //printf("%s %d: userinputstr = %s\n",
                //       __func__, __LINE__, userInputStr);

                // TODO: remove show_cmd(command);
                command_parts += 1;
                printf("%s %d: AFTER incrementing cmd_parts = %d\n",
                       __func__, __LINE__, command_parts);
            }
        }
        printf("%s %d\n",__func__, __LINE__);
        show_cmd_list(cmds_to_be_run);
        printf("%s %d\n",__func__, __LINE__);
    }
    printf("done\n");
    return cmds_to_be_run;
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

