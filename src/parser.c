#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/parser.h"
#include "../include/debug.h"

/**
 * GLibC's strndup was being flaky so we're no longer using theirs.
 * Borrowed this from:
 *      http://opensource.apple.com/source/gcc/gcc-5575.11/libiberty/strndup.c
 */
char* our_strndup(const char* str, size_t n)
{
    char *result = NULL;
    char *result_str = NULL;
    size_t len = strlen(str);

    if(n < len)
        len = n;
    printf("%s %d: str = %s, n = %d\n", __func__, __LINE__, str, n);

    if(NULL == (result = (char *) malloc(len + 1)))
        return NULL;
    printf("%s %d: str = %s, n = %d\n", __func__, __LINE__, str, n);

    result[len] = '\0';
    result_str = (char *) memcpy(result, str, len);
    printf("%s %d: result_str = %s, len = %d\n", __func__, __LINE__, result_str, len);

    return result_str;
}


/**
 * Grabs commands and args and formats them to pass to child process
 * No escape characters - " " is the only delimiter.
 */
void parseUserInput(char *userInputStr, char **storeArgs)
{
    unsigned int i;
    unsigned int arg = 0;
    unsigned int string_start = 0;
    unsigned int cur_string_len = 0;
    unsigned int last_was_space = 1;

    for (i = 0; i <= strlen(userInputStr); i++) {
        if ((userInputStr[i] == ' ') || (userInputStr[i] == '\0')) {
            if (!last_was_space) {
                printf("%d userInputStr = %s currstringlen = %d str_start = %d\n",
                       __LINE__, userInputStr, cur_string_len, string_start);

                /* PROBLEM IS HERE!  STRING GETS TRUNCATED
                 * but for some reason, only happens with the first set of
                 * commands given to dash. Doesn't happen with subsequent sets
                 * ex: cat val1.log | grep dash | wc -l | wc -c
                 *      becomes: cat val1.log | grep dash!
                 * */
                storeArgs[arg] = our_strndup(&userInputStr[string_start], cur_string_len);
                printf("%d userInputStr = %s\n", __LINE__, userInputStr);
                if (storeArgs[arg] == NULL)
                    ERROR("Failed malloc!\n");
                //storeArgs[arg][cur_string_len] = '\0';
                printf("storeArgs[%d] = %s\n", arg, storeArgs[arg]);
                printf("%d userInputStr = %s\n", __LINE__, userInputStr);
                
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

int three_d_array_len(char ***three_d_array)
{
    int i = 0;

    for(; three_d_array[i] != NULL; i++)
        printf("3d_array[%d] = %s\n", i, three_d_array[i][0]);

    return i;
}

int num_pipes(char **userInputStr)
{
    int i = 0, total_num_pipes = 0;
    const char *tmp = NULL;

    for(; userInputStr[i] != NULL; i++) {
        tmp = userInputStr[i];
        printf("%s %d: tmp = %s, userinputstr = %s\n",
               __func__, __LINE__, tmp, userInputStr[i]);
        if (tmp && (tmp[0] == '|') && (tmp[1] == '\0'))
        {
            printf("%s %d: totes in here\n", __func__, __LINE__);
            total_num_pipes += 1;
        }
    }
    printf("%s %d: num pipes total = %d\n", __func__, __LINE__, total_num_pipes);

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
    printf("%s %d: number of pipes = %d\n", __func__, __LINE__, pipes);

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
        printf("%d: Showing command\n", __LINE__);
        show_cmd(userInputStr);
        // Grab each string making up the user input
        for(i = 0; userInputStr[i] != NULL; i++) {
            tmp = userInputStr[i];
            
            if (((userInputStr[i+1] == NULL)) ||
                ((tmp[0] == '|') && (tmp[1] == '\0')))
            {
                cmdlen = sizeof(char*) * (command_parts+1);
                command = malloc(cmdlen);
                // TODO: check for failure
                memset(command, '\0', cmdlen);
                
                for(j = 0; j < (command_parts + 1); j++)
                {
                    printf("%s %d: cmd_parts = %d\n", __func__,
                        __LINE__, command_parts);
                    
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

                // Time to add to main command list
                command[j] = NULL;
                cmds_to_be_run[cmdi] = command;
                cmdi += 1;

                // Increase this var to reflect starting pos of next cmd
                cmd_start = i + 1;

                // Reset so next command+args can start at 0
                command_parts = 0;

                // TODO: remove show_cmd(command);
                printf("%s %d\n",__func__, __LINE__);
                show_cmd(command);
                printf("%s %d\n",__func__, __LINE__);
                //show_cmd_list(cmds_to_be_run);
                //printf("%s %d\n",__func__, __LINE__);
            }
            else
            {
                // TODO: remove show_cmd(command);
                printf("%s %d: BEFORE incrementing cmd_parts = %d\n",
                       __func__, __LINE__, command_parts);
                printf("%s %d: userinputstr_i[%i] is %s\n",
                       __func__, __LINE__, i, userInputStr[i]);
                //printf("%s %d: userinputstr = %s\n",
                //       __func__, __LINE__, userInputStr);
                command_parts += 1;
                printf("%s %d: AFTER incrementing cmd_parts = %d\n",
                       __func__, __LINE__, command_parts);
            }
        }
        printf("cmdi = %d\n", cmdi);
        cmds_to_be_run[cmdi] = NULL;
        printStdErrMessage(__func__, __LINE__, "3d array", 0); 
        three_d_array_len(cmds_to_be_run);
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
    {
        printf("%s: oldInputStr[%d] = %c\n",
               __func__, i, oldInputStr[i]);
        newInputStr[i] = oldInputStr[i];
        printf("%s: newInputStr[%d] = %c\n",
               __func__, i, newInputStr[i]);
    }
    newInputStr[i] = '\0';  // Terminate string properly
}

