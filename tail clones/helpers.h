#include <sys/stat.h>

enum Flags {LINE, BYTE, VERBOSE, QUIET, START_FROM, FOLLOW_by_descriptor, FOLLOW_by_name};

void printBinary(int num);
int check_bit(int * flags, int bit);
int is_integer(const char * str, long int * num);
int print_by_line(int fd, int * flags, struct stat * st, int count, int startfrom); 
//int newlines_parser(char * buf, int * target, int * start, int * qty);
int print_by_byte(int fd, int * flags, struct stat * st, int count, int startfrom);