CC = gcc
CFLAGS = -Wall -pthread
TARGET = scheduler
OBJS = main.o parser.o validator.o executor.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c scheduler.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c scheduler.h
	$(CC) $(CFLAGS) -c parser.c

validator.o: validator.c scheduler.h
	$(CC) $(CFLAGS) -c validator.c

executor.o: executor.c scheduler.h
	$(CC) $(CFLAGS) -c executor.c

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET) test.ini
