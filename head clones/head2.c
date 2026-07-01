/*
 * head2.c - a minimal clone of the Unix `head` utility.
 *
 * Reads from one or more files (or stdin) and writes the leading portion of
 * each to stdout. Supported options:
 *   -n   limit output by line count (default)
 *   -c   limit output by byte count
 *   -v   print a "==> filename <==" header before each file
 *   -q   suppress the filename header
 * A bare numeric argument (e.g. `5` or `-5`) sets the count.
 *
 * Usage: head2 [-n|-c] [-v|-q] [count] [file ...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <head2.h>

/*
 * Bit positions within the `flags` bitfield.
 *   LB - line mode when set, byte mode when clear.
 *   SH - emit the filename header when set.
 */
enum configs {LB, SH};

/*
 * Parse `str` as a base-10 integer. On success, stores the value in *num and
 * returns 1. Returns 0 if the string is null/empty, out of range, or contains
 * any trailing non-numeric characters (i.e. it must be a whole-string match).
 */
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
    // Prints an int as binary; handy for debugging, currently unused.
    int len = sizeof(int) * 8;
    char binary[len + 1];

    for (int i = 0; i < len; i++)
        binary[i] = ((num >> (len - 1 - i)) & 1) + '0';

    binary[len] = '\0';
    printf("%s\n", binary);
}

/* Return 1 if the given bit position is set in *flags, 0 otherwise. */
int check_bit(int * flags, int bit) {
    return (*flags & (1 << bit)) != 0;
}

/*
 * Parse a single command-line token `s` and fold any option flags it contains
 * into *flags. Multiple short flags may be bundled (e.g. "-nv").
 *
 * `units` and `titles` are sticky guards: each option group (-n/-c and -v/-q)
 * may only be set once, so the first occurrence wins and later conflicting
 * flags in the same group are ignored.
 *
 * Returns:
 *    0  token is not an option (treat as a positional/filename argument)
 *    1  token was parsed as one or more flags
 *    2  token was a negative number (e.g. "-5"); *num holds the count
 *   -1  token contained an unrecognized flag (error already reported)
 */
int findFlags(int * flags, const char * s, long int * num, int * units, int * titles)
{
    int temp = *flags;

    /* Not an option: either no leading '-' or a lone "-" (meaning stdin). */
    if (*s != '-' || *(s + 1) == '\0')
        return 0;
    /* A fully numeric token like "-5" is a count, not a flag. */
    if(is_integer((s+1), num)) return 2;
    int i = 1;
    char ch = *(s + i);
    int num_check;
    while (ch != '\0')
    {
        /* Does the remainder of the token form a number? Supports forms like
         * "-n5", where a count is appended directly to the flags. */
        num_check = is_integer((s+i), num);
        i++;
        if (ch == 'n' && !(*units)) 
        {
            temp |= (1 << LB); // -n: count lines
            *units = 1;
        }
        else if (ch == 'c' && !(*units)) {
            temp &= ~(1 << LB); // -c: count bytes
            *units = 1;
        }
        else if (ch == 'v' && !(*titles)) {
            temp |= (1 << SH); // -v: show filename header
            *titles = 1;
        }
        else if (ch == 'q' && !(*titles)) {
            temp &= ~(1 << SH); // -q: hide filename header
            *titles = 1;
        }
        else if (num_check) break;  /* trailing count consumed; stop scanning */
        else {

            fprintf(stderr, "head: invalid flag '%c'\n", ch);
            return -1;
        }
        ch = *(s + i);
    }
    *flags = temp;
    return 1;
}

/*
 * Copy the leading portion of stream `f` to stdout, bounded by `num`.
 * In line mode (LB set) `num` caps the number of lines; otherwise it caps the
 * number of bytes. Prints the filename header first when the SH flag is set.
 */
void printContent(FILE * f, int * flags, char * filename, long int num)
{

    int newlines = 0;
    int bytes = 0;

    int ch;
    if (check_bit(flags, SH)) printf("==> %s <==\n", filename);
    if (num <= 0) return;  /* nothing requested */

    while((ch = fgetc(f)) != EOF)
    {
        putchar(ch);
        if (check_bit(flags, LB)) {
            if (ch == '\n') newlines++;
            if (newlines == num) return;  /* reached the line limit */
        } else {
            bytes++;
            if (bytes == num) return;  /* reached the byte limit */
        }
    }

    return;
}

int main(int argc, char * argv[])
{
    int flags = (1 << LB);   /* default: line mode, no filename header */
    char * pos[argc];        /* positional (filename) arguments */
    int npos = 0;
    int toggle = 0;

    int units = 0;           /* guard: -n/-c chosen */
    int titles = 0;          /* guard: -v/-q chosen */
    long int count = 10;     /* default head size */

    /* First pass: classify each argument as an option, a count, or a file. */
    for (int i = 1; i < argc; i++)
    {
        /* A leading non-'-' token that is purely numeric sets the count. */
        if (*argv[i] != '-') if (is_integer(argv[i], &count)) continue;
        toggle = findFlags(&flags, argv[i], &count, &units, &titles);
        if (toggle == 0) pos[npos++] = argv[i];   /* not an option: a filename */
        else if (toggle == -1) return -1;         /* unrecognized flag: bail out */
        else if (toggle == 2) continue;           /* negative-number count */
    }
    FILE * f;
    char * filename;

    /* No files given: read from stdin. */
    if (npos == 0) {
        f = stdin;
        filename = "stdin";
        printContent(f, &flags, filename, count);
    } else {
        /* Process each file in order; "-" is treated as stdin. */
        for (int i = 0; i < npos; i++)
        {
            if (strcmp(pos[i], "-") != 0) {
                f = fopen(pos[i], "r");
                filename = pos[i];
            } else {
                f = stdin;
                filename = "stdin";
            }

            /* Report the open failure but keep going with the next file. */
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