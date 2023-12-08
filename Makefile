# Makefile for filesys

CC = gcc
CFLAGS = -g -Wall -Wextra -std=c99
SRC_DIR = src
BIN_DIR = bin
EXEC := $(BIN_DIR)/$(EXECUTABLE)

all: $(BIN_DIR)/filesys

$(BIN_DIR)/filesys: $(BIN_DIR) lexer.o filesys.o
	$(CC) $(CFLAGS) -o $@ $(BIN_DIR)/lexer.o $(BIN_DIR)/filesys.o

lexer.o: $(SRC_DIR)/lexer.c
	$(CC) $(CFLAGS) -c -o $(BIN_DIR)/lexer.o $(SRC_DIR)/lexer.c

filesys.o: $(SRC_DIR)/filesys.c
	$(CC) $(CFLAGS) -c -o $(BIN_DIR)/filesys.o $(SRC_DIR)/filesys.c

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
