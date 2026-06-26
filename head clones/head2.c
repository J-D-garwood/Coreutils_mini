
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

enum configs {line_byte, show_suppress};

int is_integer(const char * str, long int * num) {
    if (str == NULL || *str == '\0') return 0;

    char *endptr;
    errno = 0;

    long int temp = strtol(str, &endptr, 10);

    if (errno == ERANGE) return 0;

    // Check if endptr points to the null terminator
    // If it doesn't, there were invalid trailing characters (e.g., "50a")
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

void set_bits(int * flags, int * bits, int len) {
    for (int i = 0; i < len; i++) {
        *flags = (*flags | (1 << bits[i]));
    }
}

int findFlags(int * flags, const char * s)
{
    int temp = 0;

    if (*s != '-' || *(s + 1) == '\0')
        return 0;

    int i = 1;
    char ch = *(s + i);

    while (ch != '\0')
    {
        i++;
        if      (ch == 'n') temp |= (1 << line_byte);
        else if (ch == 'c') temp & ~(1 << line_byte);
        else if (ch == 'q') temp |= (1 << show_suppress);
        else if (ch == 'v') temp &= ~(1 << show_suppress);
        else {
            // Unknown letter: report and abandon this whole token.
            // temp is discarded so partial state doesn't leak into *flags.
            fprintf(stderr, "head: invalid flag '%c'\n", ch);
            return -1;
        }
        ch = *(s + i);
    }
    *flags &= temp;
    return 1;
}

void printContent(FILE * f, int * flags, char * filename, long int num)
{ 

    int newlines = 0;
    int bytes = 0;

    int lines = check_bit(flags, line_byte);

    int ch;

    while((ch = fgetc(f)) != EOF)
    {
        putchar(ch);
        if (lines) {
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
    int flags = 3;
    char * pos[argc];
    int npos = 0;
    int toggle = 0;

    long int count = 10;

    for (int i = 1; i < argc; i++)
    {   
        if (is_integer(argv[i], &count)) continue;
        toggle = findFlags(&flags, argv[i]);
        if (toggle == 0) pos[npos++] = argv[i];
        else if (toggle == -1) return 0;
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
                fprintf(stderr, "wc4: %s: %s\n", pos[i], strerror(errno));
                continue;
            } else {
                printContent(f, &flags, filename, count);
                if (f != stdin) fclose(f);
            }
        }
    }
    return 0;
}