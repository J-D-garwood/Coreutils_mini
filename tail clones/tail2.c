#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

enum Flags {LINE_BYTE, VERBOSE_QUIET, FOLLOW_by_descriptor, FOLLOW_by_name};

int parseArg(char * arg, char * pos[], int * count, int * npos) {
    long int temp;

    if (*arg == '-' || *arg == '+') {
        printf("flag\n");
    } else if (is_integer(arg, &temp)) {
        printf("number\n");
        *count = temp;
    } else {
        printf("filename");
        pos[*(npos++)] = arg;
    }
    return 0;
}

int main(int argc, char * argv[]) {

    int flags = (1 << LINE_BYTE) | (1 <<VERBOSE_QUIET);
    char * pos[argc];
    int npos = 0;
    int count = 10;

    for (int i = 1; i < argc; i++) {
        parseArg(argv[i], pos, &count, &npos);
    }
    return 0;
};
