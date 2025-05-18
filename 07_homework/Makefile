CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
BUILD_DIR = build

SRC = logger.c main.c
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))

LIB = $(BUILD_DIR)/liblogger.a
MAIN = $(BUILD_DIR)/main

.PHONY: all clean

all: $(MAIN)

$(MAIN): $(BUILD_DIR)/main.o $(LIB)
	$(CC) $(CFLAGS) $^ -L$(BUILD_DIR) -llogger -o $@

$(LIB): $(BUILD_DIR)/logger.o
	ar rcs $@ $^

$(BUILD_DIR)/%.o: %.c logger.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
