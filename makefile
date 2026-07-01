CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -I"head clones"

.PHONY: all clean

all:
	$(CC) $(CFLAGS) "tail clones/tail2.c" "tail clones/helpers.c" -o "tail2.exe"

clean:
	del "tail2.exe"