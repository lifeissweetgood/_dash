#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int num_pipes(char **userInputStr);
void parseUserInput(const char *userInputStr, char **storeArgs);
char ***parse_commands(char **userInputStr);
void removeNewLine(char *oldInputStr, char *newInputStr);
