#include "function.h"

char* addattr(char *src, char* str1, char* str2) {
    int s1,s2;
    s1 = strlen(str1);
    memmove(src, str1, s1+1);
    if (str2) {
        s2 = strlen(str2);
        memmove(src+s1, str2, s2+1);
        return src+s1+s2;
    }
    else return src+s1;
}

char* strrep(char* dest, char* src) {
    if (src) strcpy(dest,src);
    else *dest = 0;
    return dest;
}

int strxcpy(char *dest, char *src, int max) {
    if (strlen(src) <= max) {
        max = strlen(src);
    }
    strncpy(dest,src,max);
    dest[max] = '\0';
    return max;
}

char* tail(char* str) {
    char *p = str;
    while (*p!=0) p++;
    return p;
}