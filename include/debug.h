#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ERROR(x) if(x) { printf("%s: %d: ERROR %s\n", \
                                __func__, __LINE__, x); exit(1); }
#define ERROR_CLEANUP(x) if(x) { printf("%s: %d: ERROR %s\n", \
                                        __func__, __LINE__, x); \
                                rc = -__LINE__; goto cleanup; }
#define ASSERT(x) if(x) { printf("ASSERT: %d: Assert failed!\n", __LINE__); \
                          rc = -__LINE__; goto cleanup; }

void show_cmd(char **cmd);
void printErrorMessage(char** args, int code);
