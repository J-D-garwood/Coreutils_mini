// ADD ABILITY TO PARSE "-5" like command line args


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

enum configs {LB, SH};

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

void printBinary(int num)
{
    // Nice utility function, though unused.
    int len = sizeof(int) * 8;
    char binary[len + 1];

    for (int i = 0; i < len; i++)
        binary[i] = ((num >> (len - 1 - i)) & 1) + '0';

    binary[len] = '\0';
    printf("%s\n", binary);
}

int check_bit(int * flags, int bit) {
    return (*flags & (1 << bit)) != 0;
}

int findFlags(int * flags, const char * s, long int * num, int * units, int * titles)
{
    int temp = *flags;

    if (*s != '-' || *(s + 1) == '\0')
        return 0;

    int i = 1;
    char ch = *(s + i); //
    int num_check = is_integer((s+i), num);
    if (num_check) return 1;
    while (ch != '\0')
    {
        num_check = is_integer((s+i), num);
        i++;
        if (ch == 'n' && !(*units)) 
        {
            temp |= (1 << LB); // 000 | 001 = 001
            *units = 1;
        }
        else if (ch == 'c' && !(*units)) {
            temp &= ~(1 << LB); // 011 & 110 = 010
            *units = 1;
        }
        else if (ch == 'v' && !(*titles)) {
            temp |= (1 << SH); // 001 | 010
            *titles = 1;
        }
        else if (ch == 'q' && !(*titles)) {
            temp &= ~(1 << SH); // 001 | 010
            *titles = 1;
        }
        else if (num_check) break;
        else {

            fprintf(stderr, "head: invalid flag '%c'\n", ch);
            return -1;
        }
        ch = *(s + i);
    }
    *flags = temp;
    return 1;
}

void printContent(FILE * f, int * flags, char * filename, long int num)
{ 

    // add -q / -v functionality

    int newlines = 0;
    int bytes = 0;

    int ch;
    if (check_bit(flags, SH)) printf("==> %s <==\n", filename);
    if (num <= 0) return;

    while((ch = fgetc(f)) != EOF)
    {
        putchar(ch);
        if (check_bit(flags, LB)) {
            if (ch == '\n') newlines++; 
            if (newlines == num) return;
        } else {
            bytes++;
            if (bytes == num) return;
        }
    }

    return; 
}

int main(int argc, char * argv[])
{
    int flags = (1 << LB);
    char * pos[argc];
    int npos = 0;
    int toggle = 0;

    int units = 0;
    int titles = 0;
    long int count = 10;

    for (int i = 1; i < argc; i++)
    {   
        if (is_integer(argv[i], &count)) continue;
        toggle = findFlags(&flags, argv[i], &count, &units, &titles);
        if (toggle == 0) pos[npos++] = argv[i];
        else if (toggle == -1) return -1;
    }
    FILE * f;
    char * filename;

    if (npos == 0) {
        f = stdin;
        filename = "stdin";
        printContent(f, &flags, filename, count);
    } else {
        for (int i = 0; i < npos; i++)
        {
            if (strcmp(pos[i], "-") != 0) {
                f = fopen(pos[i], "r");
                filename = pos[i];
            } else {
                f = stdin;
                filename = "stdin";
            }

            if (f == NULL) {
                fprintf(stderr, "head2: %s: %s\n", pos[i], strerror(errno));
                continue;
            } else {
                printContent(f, &flags, filename, count);
                if (f != stdin) fclose(f);
            }
        }
    }
    return 0;
}