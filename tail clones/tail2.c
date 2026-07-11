// Write static content function (get used to posix i/o calls) THEN live content 

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

enum Flags {LINE, BYTE, VERBOSE, QUIET, START_FROM, FOLLOW_by_descriptor, FOLLOW_by_name};

int parseFlags(char * arg, int * flags, int * count,  int * startfrom) {

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
        if (parseFlags(arg, flags, count, startfrom) != 0) return EXIT_FAILURE;
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

int liveContent(char * files[], int * flags, int file_count)
{ 
    return 0;
};

// WORKING on static content
int editAndPrint(int fd, int * flags, int count) {
    if (check_bit(flags, LINE)) {
    } else {
    }
}
int staticContent(char * files[], int * flags, int file_count)
{
    int fd;
    struct stat st;
    char buf[257];
    off_t place;
    int len = 256;
    int complete = 0;
    int n;
    for (int i = 0; i < file_count; i++) {
        fd = open(files[i], O_RDONLY);
        if (fd == -1) 
        {
            perror("open");
            return 1;
        }
        if (fstat(fd, &st) == -1) {perror("fstat"); close(fd); return 1; }
    
        //printf("name: %s size: %ld\n", files[i] ,st.st_size);

        place = st.st_size;
        while (!complete) {
            place = place - 256;
            if (place<0) {
                len = 256 + place; complete = 1;
                place = 0;
            }
            lseek(fd, place, SEEK_SET);
            n = read(fd, buf, len);
            if (n == -1) {perror("read");  close(fd); return 1;}
            buf[n] = '\0';
            if (check_bit(flags, ))
        }

        len = 256;
        complete = 0;
        close(fd);
    }
    return 0;
}

int main(int argc, char * argv[]) {

    int flags = 0;
    char * pos[argc];
    int npos = 0;
    int count = 10;
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
        //insert live content function here
    } else {
        if (staticContent(pos, &flags, npos)==1) { perror("Error printing static content"); return 1;}
    }
    // if live (-f -F)
    // parse pos through and continually print
    // else
    // standard printing
    return 0;
};
