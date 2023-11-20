CC = gcc
CFLAGS= -std=c11 -pedantic -pthread -pedantic-errors -g -Wall -Werror -Wextra -Wno-unused-parameter -Wno-newline-eof -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200112L -fsanitize=address
LIB_DIR = lib
SERVER_DIR = server
CLIENT_DIR = client
PARSER_DIR = parser
STM_DIR = stm

LIB_OBJS = $(LIB_DIR)/buffer.o $(LIB_DIR)/netutils.o $(LIB_DIR)/parser.o $(LIB_DIR)/selector.o $(LIB_DIR)/stm.o $(LIB_DIR)/logger/logger.o
SERVER_OBJS = $(SERVER_DIR)/args/args.o $(SERVER_DIR)/parser/pop3_parser.o $(SERVER_DIR)/stm/pop3_stm.o $(SERVER_DIR)/pop3.o $(SERVER_DIR)/main.o 
CLIENT_OBJS = # Add your client .o files here if any
ALL_OBJS = $(LIB_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS)

TARGET = pop3

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Object files dependencies

$(LIB_DIR)/%.o: $(LIB_DIR)/%.c $(LIB_DIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SERVER_DIR)/args/%.o: $(SERVER_DIR)/args/%.c $(SERVER_DIR)/args/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SERVER_DIR)/parser/%.o: $(SERVER_DIR)/parser/%.c $(SERVER_DIR)/parser/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SERVER_DIR)/stm/%.o: $(SERVER_DIR)/stm/%.c $(SERVER_DIR)/stm/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Add rules for client objects here if any

clean:
	rm -f $(ALL_OBJS) $(TARGET)
