
#This make file doesn't recompile the header files if that is the only thing that changed.
CC := gcc

CFLAGS := -c -std=c99 -Wall

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)



.PHONY: default clean

default: output

output: $(OBJS)
	$(CC) $(LFLAGS)  $(OBJS) -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I.

clean:
	rm -f *.o output