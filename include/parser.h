#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void parseUserInput(const char *userInputStr, char **storeArgs);
char ***parse_commands(char **userInputStr);
void removeNewLine(char *oldInputStr, char *newInputStr);
