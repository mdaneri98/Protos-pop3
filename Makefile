CC = gcc
CFLAGS = -std=c11 -pedantic -pedantic-errors -g -Wall -Werror -Wextra -Wno-unused-parameter -Wno-newline-eof -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200112L -fsanitize=address
LDFLAGS = -pthread

LIB = lib

# Source files
SRCS = $(wildcard $(LIB)/*.c) $(wildcard $(LIB)/logger/*.c) $(wildcard server/*.c) $(wildcard server/args/*.c) $(wildcard server/stm/*.c)

# Object files
OBJS = $(patsubst %.c,%.o,$(SRCS))

# Output file
OUTP = ./bin/pop3-server

server: $(OUTP)

# Rule to compile source files
%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(LIB) -c -o $@ $<

# Rule to link object files into the executable
$(OUTP): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Cleaning rule
clean:
	rm -rf $(OBJS) $(OUTP)
