// Write static content function (get used to posix i/o calls) THEN live content 
// EVENTUALLY YOU NEED TO EXPAND THE FUNCTION SO IT'S NOT JUST TEXT ONLY

// NEXT UP, IMPLEMENT -n K (byte-wise) functionality!!!
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

int parseFlags(char * arg, int * flags,  int * startfrom) {

    long int tmp_s;

    if (*arg == '+') { 
        if (is_integer(arg+1, &tmp_s)) {
            *flags |= (1<<START_FROM);
            *startfrom = tmp_s;
            return 0;
        } else {
            fprintf(stderr, "tail: invalid flag '%s'\n", arg);
            return EXIT_FAILURE;
        }
    }

    int tmp_f = *flags;

    int i = 1;
    char ch = *(arg + i);

    while (ch != '\0') {
        
        i++;
             if (ch == 'n') tmp_f |= (1 << LINE);
        else if (ch == 'c') tmp_f |= (1 << BYTE);
        else if (ch == 'v') tmp_f |= (1 << VERBOSE);
        else if (ch == 'q') tmp_f |= (1 << QUIET);
        else if (ch == 'f') tmp_f |= (1 << FOLLOW_by_descriptor);
        else if (ch == 'F') tmp_f |= (1 << FOLLOW_by_name);   
        else {
            fprintf(stderr, "tail: invalid flag '%c'\n", ch);
            return EXIT_FAILURE;
        }
        ch = *(arg + i);
    }
    *flags = tmp_f;
    return 0;
}

int parseArg(char * arg, char * pos[], int * count, int * npos, int * flags, int * startfrom) {
    long int temp;
    if (*arg == '-' || *arg == '+') {
        if (parseFlags(arg, flags, startfrom) != 0) return EXIT_FAILURE;
        //printf("flag\n");
    } else if (is_integer(arg, &temp)) {
        //printf("number\n");
        *count = temp;
    } else {
        //printf("filename\n");
        pos[(*npos)++] = arg;
    }
    return 0;
}

int staticContent(char * files[], int * flags, int file_count, int count, int startfrom)
{
    int fd;
    struct stat st;
    for (int i = 0; i < file_count; i++) {
        if (check_bit(flags, VERBOSE)) {
            printf("==> %s <==\n", files[i]);
        }
        fd = open(files[i], O_RDONLY);
        if (fd == -1) 
        {
            perror("open");
        }
        if (fstat(fd, &st) == -1) {perror("fstat"); close(fd); return 1; }
    
        if (check_bit(flags, LINE)) {
            if (print_by_line(fd, flags, &st, count, startfrom) == 1) {perror("line printing"); return 1;}
        } else {
            if (print_by_byte(fd, flags, &st, count, startfrom) == 1) {perror("byte printing"); return 1;}
        }
        //byte variants to be added later
    }
    return 0;
}

int main(int argc, char * argv[]) {

    int flags = 0;
    char * pos[argc];
    int npos = 0;
    int count = 2;
    int startfrom = -1;

    for (int i = 1; i < argc; i++) {
        if (parseArg(argv[i], pos, &count, &npos, &flags, &startfrom) != 0) return EXIT_FAILURE;
    }

    if (check_bit(&flags, LINE)&&check_bit(&flags, BYTE)) { 
        fprintf(stderr, "tail: cannot use '-c' and '-n' flags concurrently\n"); 
        return EXIT_FAILURE;
    }
    else if (check_bit(&flags, QUIET)&&check_bit(&flags, VERBOSE)) {
        fprintf(stderr, "tail: cannot use '-q' and '-v' flags concurrently\n"); 
        return EXIT_FAILURE;
    }
    else if (check_bit(&flags, FOLLOW_by_descriptor)&&check_bit(&flags, FOLLOW_by_name)) { 
        fprintf(stderr, "tail: cannot use '-f' and '-F' flags concurrently\n"); 
        return EXIT_FAILURE;
    }
    
    int live = (check_bit(&flags, FOLLOW_by_descriptor)|check_bit(&flags, FOLLOW_by_name));
    if (flags == 0) flags = (1 << LINE);

    if (live) {
        //to be added later
    } else {
        if (staticContent(pos, &flags, npos, count, startfrom)==1) { perror("Error printing static content"); return 1;}
    }
    return 0;
};
