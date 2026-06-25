#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>   // Needed because you use errno in main()

// Good use of an enum to index statistics cleanly.
enum statistics {line_count, word_count, byte_count, Longest_line, STAT_COUNT};

// Macro is fine, but the trailing semicolon is unnecessary (harmless though).
#define MAX(a, b) (((a) > (b)) ? a : b)

int check_bit(int * flags, int bit) {
    // Nice clean bitmask check.
    return (*flags & (1 << bit)) != 0;
}

void printStatistics(FILE * f, int * flags, char * filename) 
{
    // BUG: lines should start at 0, not 1. wc counts '\n', not lines.
    int lines = 1; 

    int bytes = 0;
    int words = 0;

    // Good: tracking longest line.
    int longestLine = 0;
    int currentLine = 0;

    int ch;
    int prev = '\0';

    // Main loop is simple and readable.
    while((ch = fgetc(f)) != EOF)
    {
        bytes++;

        // BUG: word detection is incorrect.
        // isprint() is not a proxy for "is part of a word".
        // This breaks on tabs, punctuation, UTF‑8, etc.
        if (isprint(ch) && (!isprint(prev) || prev == ' ')) 
            words++;

        if (ch == '\n') {
            lines++;
            longestLine = MAX(currentLine, longestLine);
            currentLine = 0;
        } else {
            currentLine++;
        }

        prev = ch;
    }

    // BUG: if file doesn't end with newline, last line isn't compared.
    int statistics[] = {lines, words, bytes, longestLine};

    // Clean loop, readable.
    for (int i = line_count; i < STAT_COUNT; i++) 
    {
        if (check_bit(flags, i)) 
            printf(" %d", statistics[i]);
    }

    printf(" %s\n", filename);
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

void set_bits(int * flags, int * bits, int len) {
    // Clean and simple.
    for (int i = 0; i < len; i++) {
        *flags = (*flags | (1 << bits[i]));
    }
}

int findFlags(int * flags, const char * s)
{
    int temp = 0;

    // Good: reject non-flags early.
    if (*s != '-' || *(s + 1) == '\0') 
        return 0;

    int i = 1;
    char ch = *(s + i);

    // Simple flag parser, works for combined flags like -clw.
    while (ch != '\0')
    {
        i++;
        if      (ch == 'c') temp |= (1 << byte_count);
        else if (ch == 'l') temp |= (1 << line_count);
        else if (ch == 'w') temp |= (1 << word_count);
        else if (ch == 'L') temp |= (1 << Longest_line);
        else {
            // BUG: unknown flag returns 0, treating it as a filename.
            return 0;
        }
        ch = *(s + i);
    }

    *flags |= temp;
    return 1;
}

int main(int argc, char * argv[])
{
    int flags = 0;

    // Good: default wc behaviour.
    int default_bits[] = {line_count, word_count, byte_count};

    // Allocating argv list for non-flag arguments.
    // Could avoid malloc entirely, but this works.
    char ** pos = malloc(argc * sizeof(char*));
    int npos = 0;

    for (int i = 1; i < argc; i++)
    {
        // Good: flags OR filenames.
        if (!findFlags(&flags, argv[i])) 
            pos[npos++] = argv[i];
    }

    FILE * f;
    char * filename;

    // Good: default flags if none provided.
    if (flags == 0) 
        set_bits(&flags, default_bits, 3);

    if (npos == 0) {
        // Good: stdin support.
        f = stdin;
        filename = "stdin";
        printStatistics(f, &flags, filename);
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
                // Good: stderr for diagnostics.
                fprintf(stderr, "wc2: %s: %s\n", pos[i], strerror(errno));
            } else {
                printStatistics(f, &flags, filename);

                // BUG: missing fclose(f) for real files.
                if (f != stdin)
                    fclose(f);
            }
        }
    }

    free(pos);
}
