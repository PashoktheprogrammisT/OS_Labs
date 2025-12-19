CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L
TARGETS = parent child

all: $(TARGETS)

parent: parent.c
	$(CC) $(CFLAGS) parent.c -o parent

child: child.c
	$(CC) $(CFLAGS) child.c -o child

clean:
	rm -f $(TARGETS) *.txt

run: parent child
	./parent

test: parent child
	@printf "file1.txt\nfile2.txt\nHello\nWorld\nTest\n" | ./parent

.PHONY: all clean run test
