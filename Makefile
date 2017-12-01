
#This make file doesn't recompile the header files if that is the only thing that changed. (Should probably add [-ansi -pedantic] and/or [-Wall ]back if shits too fucked up)
CC := gcc

CFLAGS := -c -std=c99 

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