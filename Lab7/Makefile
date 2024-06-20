CC = gcc
CFLAGS = -Wall -W -Wextra -Wno-unused-parameter
LDFLAGS =

SRCS = main.c func.c
OBJS = $(SRCS:.c=.o)
DEPS = header.h
TARGET = program

.PHONY: all clean run

all: $(TARGET) run

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)