#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    //printf("You have entered %d arguments: \n", argc);
//
    //for (int i = 0; i<argc; i++) {
    //    printf("%s\n", *(argv+i));
    //}

    FILE *fptr;
    fptr = fopen(*(argv+1), "r");

    char buffer[100];

    if (fptr == NULL) {
        printf("Error opening file...");
    } else {
        while(fgets(buffer, 100, fptr) != NULL) 
        {
            printf("%s", buffer);
        }

        fclose(fptr);
    }
    return 0;
}