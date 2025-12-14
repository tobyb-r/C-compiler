CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g

BUILD_DIR = build

sources = main lexer parser symbols types

objects = $(patsubst %,$(BUILD_DIR)/%.o,$(sources))


all: $(BUILD_DIR) $(objects)
	$(CC) $(CFLAGS) $(objects) -o compiler

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f compiler $(objects)
	rmdir $(BUILD_DIR)

run: all
	./compiler test.c
