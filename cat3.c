#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// One bit per supported flag. Using a struct (rather than a bitfield int)
// keeps the call sites readable: `flags.n` instead of `flags & FLAG_N`.
typedef struct {
    int b;  // -b: number non-blank lines (overrides -n)
    int e;  // -e: show $ at end of each line (implies -v)
    int n;  // -n: number every line, including blanks
    int s;  // -s: squeeze consecutive blank lines into one
    int t;  // -t: show tabs as ^I (implies -v)
    int u;  // -u: POSIX "unbuffered" — accepted but a no-op (matches GNU cat)
    int v;  // -v: show non-printing chars in ^X / M-X notation
} Flags;

// Reads f to stdout, applying `flags`. `line` is a pointer so the counter
// persists across files (real cat numbers continuously, not per-file).
void dispOut(FILE * f, Flags flags, int * line) {
    int ch;            // fgetc returns int so EOF (negative) is distinguishable
    int prev = '\n';   // Pretend a newline came before the file so the very
                       // first character is treated as the start of a line.

    while ((ch = fgetc(f)) != EOF)
    {
        // --- line numbering ---------------------------------------------
        // -n numbers every line (including blanks). -b numbers only
        // non-blank lines, and overrides -n when both are set — hence the
        // !flags.b guard on the -n branch.
        if (flags.n && prev=='\n' && !flags.b) printf("%6d\t", (*line)++);
        if (flags.b && prev=='\n' && ch!='\n') printf("%6d\t", (*line)++);

        // --- squeeze (-s) -----------------------------------------------
        // We're sitting on a '\n' that follows another '\n' — i.e. a blank
        // line. Emit exactly one blank, then drain any further consecutive
        // newlines from the stream.
        if (flags.s && ch=='\n' && prev=='\n')
        {
            // Drain extra newlines. After this loop ch holds the first
            // non-newline char (or EOF). prev is kept on '\n' for the
            // numbering logic that follows.
            while ((ch = fgetc(f)) == '\n') prev = ch;

            // Emit the kept blank line, with the $ marker if -e is on.
            if (flags.e) { putchar('$'); putchar('\n'); }
            else putchar('\n');

            if (ch == EOF) break;   // file ended in a run of newlines

            // The drained newlines collapsed into one printed blank line,
            // so the *next* line (whose first char is now in `ch`) needs
            // its number printed here — the top-of-loop number-printing
            // already ran for the original '\n' and won't run again for
            // this character.
            if (flags.n || flags.b) printf("%6d\t", (*line)++);
        }

        // --- per-character rendering ------------------------------------
        // Order matters. -t handles tabs before -v's general non-printing
        // pass, so '\t' becomes "^I" rather than passing through. -e
        // handles '\n' before -v so newlines stay as real newlines
        // (with a $ prefix) and don't get caught by the control-char branch.
        if (flags.t && ch=='\t') printf("^I");
        else if (flags.e && ch=='\n') printf("$\n");
        else if (flags.v)
        {
            // -v expands non-printing bytes. Layout:
            //   0x00         -> ^@
            //   0x01..0x1F   -> ^A..^_      (control char + 0x40)
            //   0x09 ('\t')  -> raw tab     (only reached if -t is off)
            //   0x20..0x7E   -> as-is       (printable ASCII)
            //   0x7F         -> ^?          (DEL)
            //   0x80..0xFF   -> M-<above>   (strip high bit, re-classify)
            switch (ch) {
                case '\t':
                    // Reached only when -t is off; -t branch above wins
                    // otherwise. Tabs are "printable" for -v purposes.
                    putchar(ch);
                    break;
                case '\0':
                    printf("^@");
                    break;
                case 0x7F:
                    printf("^?");
                    break;
                default:
                    if (ch < 0x20) {
                        // Control char: ^ + (ch + 0x40) gives '^A' for 0x01,
                        // '^B' for 0x02, ..., '^_' for 0x1F.
                        putchar('^');
                        putchar(ch + 0x40);
                    } else if (ch < 0x7F) {
                        putchar(ch);          // ordinary printable
                    } else {
                        // High-bit byte: prefix M- and recurse on the
                        // low 7 bits using the same control / printable /
                        // DEL classification.
                        putchar('M');
                        putchar('-');
                        int c = ch & 0x7F;
                        if (c < 0x20) {
                            putchar('^');
                            putchar(c + 0x40);
                        } else if (c == 0x7F) {
                            putchar('^');
                            putchar('?');
                        } else {
                            putchar(c);
                        }
                    }
                    break;
            }
        }
        else putchar(ch);   // no -v, no special handling — copy through

        prev = ch;          // drives next iteration's "start of line" logic
    }

    // Don't close stdin — the caller may want to keep reading more files
    // from it, and closing stdin has process-wide consequences.
    if (f != stdin) fclose(f);
    return;
}

// Parses a single argv element as a flag bundle (e.g. "-bne"). Returns 1
// if the whole thing parsed cleanly and `flags` was updated; 0 otherwise
// (in which case `flags` is left untouched so partial bundles don't
// half-apply).
//
// A lone "-" returns 0 on purpose: it's the conventional stand-in for
// stdin and should end up in the positional-args list, not be treated
// as a flag.
int findFlags(Flags * flags, const char * s)
{
    Flags temp = {0};

    // Reject anything that isn't "-X[Y...]". A lone "-" hits the second
    // condition and falls through to the caller as a positional arg.
    if (*s != '-' || *(s + 1)=='\0') return 0;

    int i = 1;             // skip the leading '-'
    char ch = *(s + i);

    while (ch != '\0')
    {
        i++;
        if      (ch == 'b') temp.b = 1;
        else if (ch == 'e') temp.e = 1;
        else if (ch == 'n') temp.n = 1;
        else if (ch == 's') temp.s = 1;
        else if (ch == 't') temp.t = 1;
        else if (ch == 'u') temp.u = 1;
        else if (ch == 'v') temp.v = 1;
        else {
            // Unknown letter — bail out without touching `flags`. The
            // caller will treat the whole arg as a positional, which is
            // usually wrong but at least keeps state consistent.
            return 0;
        }
        ch = *(s + i);
    }

    // OR temp into flags so repeated flags or multiple bundles compose
    // (e.g. `-b -n` ends up the same as `-bn`).
    flags->b = (flags->b || temp.b);
    flags->e = (flags->e || temp.e);
    flags->n = (flags->n || temp.n);
    flags->s = (flags->s || temp.s);
    flags->t = (flags->t || temp.t);
    flags->u = (flags->u || temp.u);
    flags->v = (flags->v || temp.v);
    return 1;
}

int main(int argc, char **argv) {
    Flags flags = {0};
    int line = 1;          // shared counter across all input files

    // Worst case every arg is a path, so size pos[] for argc. One slot is
    // wasted (argv[0]) but the simplicity is worth it.
    const char ** pos = malloc(argc * sizeof(char*));
    int npos = 0;

    // First pass: peel flags off, collect everything else as a path.
    for (int i = 1; i < argc; i++)
    {
        if (!findFlags(&flags, argv[i])) {
            pos[npos++] = argv[i];
        }
    }

    FILE * f;

    // -t and -e both imply -v in real cat. Forcing it on here means
    // dispOut only has to check flags.v in the rendering switch.
    if (flags.t || flags.e) flags.v = 1;

    if (npos == 0) {
        // No paths given — read from stdin, the standard cat behavior
        // when invoked as a filter in a pipeline.
        f = stdin;
        dispOut(f, flags, &line);
    } else {
        for (int i = 0; i < npos; i++)
        {
            // "-" as a path means stdin, conventional across Unix tools.
            if (strcmp(pos[i], "-") != 0) f = fopen(pos[i], "r");
            else f = stdin;

            if (f == NULL) {
                // Diagnostic goes to stderr so it doesn't pollute the
                // output stream when cat is used in a pipe.
                fprintf(stderr, "cat2: %s: %s\n", pos[i], strerror(errno));
            } else {
                dispOut(f, flags, &line);
            }
        }
    }

    free(pos);
    // main implicitly returns 0 in C99+, which is fine here.
}