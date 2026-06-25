//Fix errors outlined by claude code...

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h> 

#define MAX(a, b) (((a) > (b)) ? a : b)

enum statistics {line_count, word_count, byte_count, Longest_line, STAT_COUNT};

int check_bit(int * flags, int bit) {
    return (*flags & (1 << bit)) != 0;
}

void set_bits(int * flags, int * bits, int len) {
    for (int i = 0; i < len; i++) {
        *flags = (*flags | (1 << bits[i]));
    }
}

int get_utf8_length(unsigned char lead_byte) {
    if ((lead_byte & 0x80) == 0x00) return 1; //ASCII
    if ((lead_byte & 0xE0) == 0xC0) return 2; // 2-byte sequence
    if ((lead_byte & 0xF0) == 0xE0) return 3; // 3-byte sequence
    if ((lead_byte & 0xF8) == 0xF0) return 4; // 4-byte sequence    
    return -1; //Invalid UTF-8 Lead byte
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
        if      (ch == 'c') temp |= (1 << byte_count);
        else if (ch == 'l') temp |= (1 << line_count);
        else if (ch == 'm') temp |= (1 << char_count);
        else if (ch == 'w') temp |= (1 << word_count);
        else if (ch == 'L') temp |= (1 << Longest_line);
        else {
            fprintf(stderr, "wc3: invalid flag '%c'\n", ch);
            return 0;
        }
        ch = *(s + i);
    }

    *flags |= temp;
    return 1;
}

int isUTFwhitespace(char *character, int len) {
    if (len == 1) return 0;

    unsigned char seq1[] = {0xC2, 0xA0};             // NBSP
    unsigned char seq2[] = {0xE2, 0x80, 0x82};       // EN SPACE
    unsigned char seq3[] = {0xE2, 0x80, 0x83};       // EM SPACE
    unsigned char seq4[] = {0xE2, 0x80, 0x89};       // THIN SPACE
    unsigned char seq5[] = {0xE2, 0x80, 0x80};       // IDEOGRAPHIC SPACE

    if (len == 2 && memcmp(character, seq1, 2) == 0) return 1;
    if (len == 3 && memcmp(character, seq2, 3) == 0) return 1;
    if (len == 3 && memcmp(character, seq3, 3) == 0) return 1;
    if (len == 3 && memcmp(character, seq4, 3) == 0) return 1;
    if (len == 3 && memcmp(character, seq5, 3) == 0) return 1;

    return 0;
}

void printStatistics(FILE * f, int * flags, char * filename)
{
    int lines = 0;
    int bytes = 0;
    int words = 0;
    int characters = 0;

    int longestLine = 0;
    int currentLine = 0;

    int ch;
    int ch2;
    int prev = ' '

    char multi_byte_buffer[4];
    int charLen = 0;
    int prev_is_space = 1;

    while((ch = fgetc(f)) != EOF) 
    {
        bytes++;
        charLen = get_utf8_length(ch);
        if (charlen > 1) {
            multi_byte_buffer[0] = ch;
            for (int i = 0; i<charLen-1; i++) {
                ch2 = fgetc(f);
                multi_byte_buffer[i+1] = ch2;
                bytes++;
            }
        }
        prev_is_space = (isUTFwhitespace(multi_byte_buffer, charLen)||isspace(prev));
        characters++; 
        currentLine++;

        if (prev_is_space && !isspace(ch)) {
            words++;
            prev_is_space = 0;
        }

        if (ch == '\n') {
            lines++;
            longestLine = MAX(currentLine, longestLine);
            currentLine = 0;
        }

        prev = ch;
    }

    longestLine = MAX(currentLine, longestLine);

    int statistics[] = {lines, words, characters, bytes, longestLine};

    for (int i = line_count; i < STAT_COUNT; i++) 
    {
        if (check_bit(flags, i)) 
            printf(" %d", statistics[i]);
    }

    printf(" %s\n", filename);
    return;
}

int main(int argc, char * argv[])
{
    int flags = 0;

    int default_bits[] = {line_count, word_count, char_count};

    char * pos[argc];
    int npos = 0;

    for (int i = 1; i < argc; i++)
    {
        if (!findFlags(&flags, argv[i])) 
            pos[npos++] = argv[i];
    }

    FILE * f;
    char * filename;

    if (flags == 0) set_bits(&flags, default_bits, 3);

    if (npos == 0) {
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
                fprintf(stderr, "wc3: %s: %s\n", pos[i], strerror(errno));
            } else {
                printStatistics(f, &flags, filename);
                if (f != stdin) fclose(f);
            }
        }
    }
}