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

int main(int argc, char **argv) {
    Flags flags = {0};
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
        else pos[npos++] = argv[i];
    }

    if (npos < 1) { piped = 1; npos++;}

    int ch;
    int prev = '\n';
    int line = 0;
    FILE * f;

    for (int i = 0; i<npos; i++) 
    {

        if (!piped && strcmp(pos[i], "-") != 0) {
            f = fopen(pos[i], "r");
        }
        else f = stdin;
        
        if (f == NULL) {
            printf("\nError opening file\nusage: cat2 [-b] [-e] [-n] [-s]  [-t]  [-u]  [-v] filepath\n");
        } else {
            if (flags.t || flags.e) flags.v = 1;
            while ((ch = fgetc(f)) != EOF)
            {
                if (flags.n && prev=='\n' && !flags.b) printf("%d: ", line++);
                if (flags.b && prev=='\n' && ch!='\n') printf("%d: ", line++);
                if (flags.s && ch=='\n' && prev=='\n') 
                {
                    while ((ch = fgetc(f)) == '\n') prev = ch;
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
            if (f != stdin && f != NULL) fclose(f);
        }
        prev = '\n';
        line = 0;
    }
    return 0;
}   