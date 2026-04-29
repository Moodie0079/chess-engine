CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = chess_engine

SRCS = main.c init.c movegen.c eval.c search.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c defs.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)