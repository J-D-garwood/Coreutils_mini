#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    int c;
    int l;   
    int m;    
    int w;   
    int L;   
} Flags;

int main(int argc, char **argv)
{
    Flags flags = {0};
    const char ** pos = malloc(sizeof(char*) *argc);
    int npos = 0;
    int piped = 0;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-c") == 0) flags.c = 1;        
        else if (strcmp(argv[i], "-l") == 0) flags.l = 1;
        else if (strcmp(argv[i], "-m") == 0) flags.m = 1;
        else if (strcmp(argv[i], "-w") == 0) flags.w = 1;
        else if (strcmp(argv[i], "-L") == 0) flags.L = 1;
        else pos[npos++] = argv[i];        
    }

    if (npos < 1) { piped = 1; npos++;}

    int ch;
    int prev = '\n';
    int line = 0;
    int blk = 0;
    FILE * f;

    for (int i = 0; i<npos; i++)
    {

    }
}