#ifndef FUNCTION_H
#define FUNCTION_H

#include "esp_common.h"
#include <string.h>

char* addattr(char*, char*, char*);
char* strrep(char*, char*);
int strxcpy(char*, char*, int);
char* tail(char*);
#endif