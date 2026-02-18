CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = engine

SRCS = main.c init.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c defs.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	m -f $(OBJS) $(TARGET)