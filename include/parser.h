#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int num_pipes(char **userInputStr);
int two_d_array_len(char **two_d_array);
int three_d_array_len(char ***three_d_array);
void parseUserInput(const char *userInputStr, char **storeArgs);
char ***parse_commands(char **userInputStr);
void removeNewLine(char *oldInputStr, char *newInputStr);
