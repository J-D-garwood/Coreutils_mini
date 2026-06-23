#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct {
    int b; 
    int e; 
    int n; 
    int s; 
    int t; 
    int u; 
    int v; 
} Flags;

int findFlags(Flags * flags, const char * s) {
    if (*s != '-') return 0;
    // BUG: lone "-" should mean "read stdin", not "this is a valid flag bundle".
    // Returning 1 here causes main to discard it instead of adding it to pos[].
    if (*(s+1) == '\0') return 1;
    int i = 1;
    char ch = *(s+i);
    while (ch != '\0') {
        i++;
        // STYLE: flags->b reads better than (*flags).b.
        if      (ch == 'b') (*flags).b = 1;
        else if (ch == 'e') (*flags).e = 1;
        else if (ch == 'n') (*flags).n = 1;
        else if (ch == 's') (*flags).s = 1;
        else if (ch == 't') (*flags).t = 1;
        else if (ch == 'u') (*flags).u = 1;
        else if (ch == 'v') (*flags).v = 1;  
        else {
            // BUG: errors must go to stderr, not stdout. Also missing trailing '\n'.
            // BUG: returning 0 here makes main "return 0" — an unknown flag should
            //      exit non-zero so shell pipelines can detect the failure.
            printf("cat2: illegal option -- %c", ch);
            return 0;
        }  
        ch = *(s+i);  
    }
    return 1;
}

// QUALITY: pointless probe — opens then immediately closes the file just to ask
// "does it exist?". Racy (file may vanish before the real fopen) and doubles the
// syscalls per arg. Drop this and let the real fopen in main do the check.
int testFile(const char * s) {
    FILE * f = fopen(s, "r");
    if (f == NULL) return 0;
    else fclose(f);
    return 1;
}

int main(int argc, char **argv) {
    Flags flags = {0};
    // QUALITY: heap allocation is unnecessary and never freed. A VLA or stack
    // array (`const char *pos[argc];`) would do the same job without the leak.
    const char ** pos = malloc(sizeof(char*)*argc);
    int npos = 0;
    int piped = 0;

    for (int i = 1; i < argc; i++) 
    {
        if      (strcmp(argv[i], "-b") == 0) flags.b = 1;
        else if (strcmp(argv[i], "-e") == 0) flags.e = 1;
        else if (strcmp(argv[i], "-n") == 0) flags.n = 1;
        else if (strcmp(argv[i], "-s") == 0) flags.s = 1;
        else if (strcmp(argv[i], "-t") == 0) flags.t = 1;
        else if (strcmp(argv[i], "-u") == 0) flags.u = 1;
        else if (strcmp(argv[i], "-v") == 0) flags.v = 1;
        // BUG: dispatch order is wrong. testFile runs before findFlags, so a file
        // named "-bn" (or any "-xyz" that happens to exist on disk) is opened as
        // a path instead of being parsed as a flag bundle. Parse flags first,
        // then fall through to "treat as path".
        else if (testFile(argv[i])) pos[npos++] = argv[i];
        else if (findFlags(&flags, argv[i])) {
            continue;
        } else {
            // BUG: silent "return 0" on a bad arg. Should exit non-zero and have
            // already printed a diagnostic to stderr.
            return 0;
        };  // STYLE: stray ';' — a no-op null statement after the else block.
    }

    // QUALITY: awkward — the `piped` flag plus `npos++` trick leaves pos[0]
    // uninitialized but relied-upon-by-omission. Cleaner: if (npos == 0) run
    // the read loop once with f = stdin directly.
    if (npos < 1) { piped = 1; npos++;}

    int ch;
    int prev = '\n';
    // BUG: line numbering starts at 0; real cat starts at 1.
    int line = 0;
    FILE * f;

    for (int i = 0; i<npos; i++) 
    {

        // NOTE: "-" as a path arg never reaches here because findFlags ate it
        // (see the lone-dash bug above). This strcmp is dead code today.
        if (!piped && strcmp(pos[i], "-") != 0) {
            f = fopen(pos[i], "r");
        }
        else f = stdin;
        
        if (f == NULL) {
            // BUG: error to stdout instead of stderr; no filename; no errno/
            // strerror context. A leading '\n' before any output is also odd.
            printf("\nError opening file");
        } else {
            // QUALITY: this is idempotent but belongs once before the outer
            // file loop, not re-run per file.
            if (flags.t || flags.e) flags.v = 1;
            while ((ch = fgetc(f)) != EOF)
            {
                // NOTE: -b correctly overrides -n via !flags.b. That part's fine.
                // BUG: numbering format is "%d: " — real cat uses "%6d\t".
                if (flags.n && prev=='\n' && !flags.b) printf("%d: ", line++);
                if (flags.b && prev=='\n' && ch!='\n') printf("%d: ", line++);

                // BUG: squeeze interacts badly with -n. The block above already
                // printed a number for the current '\n'; then we consume more
                // newlines and print ANOTHER number below — the blank line ends
                // up with its own number separate from the content line that
                // follows it.
                if (flags.s && ch=='\n' && prev=='\n') 
                {
                    while ((ch = fgetc(f)) == '\n') prev = ch;
                    // BUG: bypasses the flags.e branch. With "-se" the squeezed
                    // blank line should print "$\n", not just "\n".
                    putchar('\n'); 
                    if (ch == EOF) break;
                    if (flags.n || flags.b) printf("%d: ", line++);
                }
                if (flags.t && ch=='\t') 
                {
                    printf("^I"); 
                } 
                else if (flags.e && ch=='\n') 
                {
                    printf("$\n");
                } 
                else if (flags.v) 
                {
                    // BUG: -v is incomplete. Real cat maps every control char
                    // < 0x20 (except \n and \t) to "^A".."^Z" etc., and bytes
                    // 0x80..0xFF to "M-" forms. Here only \0 and 0x7F are
                    // handled; everything else falls through to a raw putchar,
                    // which defeats the point of -v.
                    switch (ch)
                    {
                        case '\0':
                            printf("^@");
                            break;
                        case 0x7F:
                            printf("^?");
                            break;
                        default:
                            putchar(ch);
                            break;  
                    }
                } 
                else 
                {
                putchar(ch);
                }
                prev = ch;
            }
            // QUALITY: f != NULL is guaranteed inside this else branch already,
            // so the second half of the check is redundant.
            if (f != stdin && f != NULL) fclose(f);
        }
        prev = '\n';
        line = 0;
    }
    // QUALITY: pos malloc'd at the top, never freed. Harmless at process exit
    // but inconsistent — either own the lifetime or don't allocate.
    return 0;
}