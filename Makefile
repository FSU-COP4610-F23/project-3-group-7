CC = gcc
CFLAGS = -g -Wall -Wextra -std=c99
SRC_DIR = src
BIN_DIR = bin
EXECUTABLE = $(BIN_DIR)/filesys

all: $(BIN_DIR) $(EXECUTABLE)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(EXECUTABLE): $(SRC_DIR)/filesys.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BIN_DIR)

.PHONY: run all clean

