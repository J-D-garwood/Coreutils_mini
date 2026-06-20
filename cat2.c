//Implement following cat flags indicated in the struct

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int b;
    int e; //implement
    int n;
    int s; //implement
    int t; //implement
    int u; //implement
    int v; //implement
} Flags;

int main(int argc, char **argv) {
    Flags flags = {0};
    const char *pos[10];
    int npos = 0;

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

    if (npos != 1) { printf("Invalid Input\nusage: cat2 [-b] [-e] [-n] [-s]  [-t]  [-u]  [-v] filepath\n"); return 1; }
    
    FILE *fptr;
    fptr = fopen(*(pos), "r");

    char buffer[100];

    int line = 0;
    if (fptr == NULL) {
        printf("Error opening file\nusage: cat2 [-b] [-e] [-n] [-s]  [-t]  [-u]  [-v] filepath\n");
    } else {
        while(fgets(buffer, 100, fptr) != NULL) 
        {
            if (flags.n) {
                printf("%d: ", line++);
            }
            if (flags.b && !flags.n && strcmp(buffer, "\n") != 0) {
                printf("%d: ", line++);
            }
            printf("%s", buffer);
        }

        fclose(fptr);
    }
    return 0;
}