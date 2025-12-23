CC = gcc
CFLAGS = -fPIC -Wall -Wextra
LDFLAGS = -lm -ldl

all: lib1.so lib2.so program1 program2

lib1.so: implementation_1.c implementation_1.h
	$(CC) $(CFLAGS) -shared -o lib1.so implementation_1.c -lm

lib2.so: implementation_2.c implementation_2.h
	$(CC) $(CFLAGS) -shared -o lib2.so implementation_2.c -lm

program1: main_1.c implementation_1.c implementation_1.h
	$(CC) -o program1 main_1.c implementation_1.c -lm

program2: main_2.c lib1.so lib2.so
	$(CC) -o program2 main_2.c $(LDFLAGS)

run1: program1
	./program1

run2: program2
	LD_LIBRARY_PATH=. ./program2

clean:
	rm -f lib1.so lib2.so program1 program2 *.o

.PHONY: all clean run1 run2
