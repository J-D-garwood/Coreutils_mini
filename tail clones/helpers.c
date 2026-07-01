#include "helpers.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int check_bit(int * flags, int bit) {
    return (*flags & (1 << bit)) != 0;
}

void printBinary(int num)
{
    // Prints an int as binary; handy for debugging, currently unused.
    int len = sizeof(int) * 8;
    char binary[len + 1];

    for (int i = 0; i < len; i++)
        binary[i] = ((num >> (len - 1 - i)) & 1) + '0';

    binary[len] = '\0';
    printf("%s\n", binary);
}

int is_integer(const char * str, long int * num) {
    if (str == NULL || *str == '\0') return 0;

    char *endptr;
    errno = 0;

    long int temp = strtol(str, &endptr, 10);

    if (errno == ERANGE) return 0;

    if (*endptr != '\0') {
        return 0; 
    }

    *num = temp;
    return 1;
}