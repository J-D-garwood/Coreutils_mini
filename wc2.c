#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum statistics {byte_count, line_count, char_count, word_count, Longest_line};

void printStatistics(FILE * f, int flags) 
{

    int lines = 0;
    int bytes = 0;
    int characters = 0;
    int words = 0;
    int longLine = 0;

    /// Figure out efficient way to read 
    // a file, collect,and then print
    //  the above information

}
void printBinary(int num)
{
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
    for (int i = 0; i<len; i++) {
        *flags = ((*flags) | (1<<bits[i]));
    }
}



int findFlags(int * flags, const char * s)
{
    int temp = 0;

    if (*s != '-' || *(s + 1)=='\0') return 0;

    int i = 1;
    char ch = *(s + i);

    while (ch != '\0')
    {
        i++;
        if      (ch == 'c') temp | (1 << byte_count);
        else if (ch == 'l') temp | (1 << line_count);
        else if (ch == 'm') temp | (1 << char_count);
        else if (ch == 'w') temp | (1 << word_count);
        else if (ch == 'L') temp | (1 << Longest_line);
        else {
            return 0;
        }
        ch = *(s + i);
    }
    *flags = (*flags) | temp;
    return 1;
}


int main(int argc, char * argv[])
{
    int flags = 0;
    int default_bits[] = {line_count, word_count, char_count};


    const char ** pos = malloc(argc * sizeof(char*));
    int npos = 0;

    for (int i = 1; i < argc; i++)
    {
        if (!findFlags(&flags, argv[i])) pos[npos++] = argv[i];
    }

    FILE * f;

    if (flags == 0) set_bits(&flags, default_bits, 3);

    if (npos == 0) {

        f = stdin;
        // INSERT DISPLAY FUNCTION
    } else {
        for (int i = 0; i < npos; i++) 
        {
            if (strcmp(pos[i], "-") != 0) f = fopen(pos[i], "r");
            else f = stdin;

            if (f == NULL) {
                // Diagnostic goes to stderr so it doesn't pollute the
                // output stream when cat is used in a pipe.
                fprintf(stderr, "wc2: %s: %s\n", pos[i], strerror(errno));
            } else {
                printStatistics(f, flags);
            }

        }
    }

    free(pos);
}
