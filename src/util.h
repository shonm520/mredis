#pragma once


#include <stdarg.h>
#include <stdio.h>
#include <string.h>



char* make_message(const char *fmt, ...)
{
    int size = 100;     /* Guess we need no more than 100 bytes */
    char *p, *np;
    va_list ap;
    if ((p = malloc(size)) == NULL)
        return NULL;
    while (1) {
        /* Try to print in the allocated space */
        va_start(ap, fmt);
        int n = vsnprintf(p, size, fmt, ap);
        va_end(ap);

        if (n < 0)
            return NULL;

        /* If that worked, return the string */
        if (n < size)
            return p;

        /* Else try again with more space */
        size = n + 1;       /* Precisely what is needed */
        if ((np = realloc (p, size)) == NULL) {
            free(p);
            return NULL;
        } else {
            p = np;
        }
    }
}
