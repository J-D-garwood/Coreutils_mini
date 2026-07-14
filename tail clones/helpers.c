#include "helpers.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


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

int newlines_parser(char * buf, int * target, int * start, int * qty) 
{
    int i = 0;
    int newline_counter = 0;
    char newline = 0x0A;
    char nullchar = 0x00;
    while (memcmp(buf + i, &nullchar, 1) != 0) {
        if (memcmp(buf + i, &newline, 1) == 0) newline_counter++;
        if (newline_counter == *target) {*start = i+1; return 1;}
        i++;
    }
    *qty = newline_counter;
    return 0;
}


int print_by_line(int fd, int *flags, struct stat * st, int count, int startfrom) 
{
    char buf[255];
    int len = 254;
    int n;
    int target = startfrom - 1;
    int newline_counter = 0;
    int partition;
    int found = 0;
    if (check_bit(flags, START_FROM)) {
        if (target <= 0) {
        // start at line 1: just dump the whole file
        while ((n = read(fd, buf, sizeof(buf))) > 0)
            write(STDOUT_FILENO, buf, n);
        if (n < 0) { perror("read"); close(fd); return 1; }
        close(fd);
        return 0;
        }
        while (1)
        {
            n = read(fd, buf, len);
            if (n==0) break;
            if (n == -1) {perror("read");  close(fd); return 1;}
            buf[n] = '\0';
            if (newlines_parser(buf, &target, &partition, &newline_counter)) {found = 1; break;}
            target = target - newline_counter;
        }

        if (found)
        {
            write(STDOUT_FILENO, buf + partition, n-partition);
            while ((n = read(fd, buf, sizeof(buf)))>0) {
                write(STDOUT_FILENO, buf, n);
            }
            if (n<0) {perror("read"); return 1;}
        }
    } 
    close(fd);
    return 0;
}
