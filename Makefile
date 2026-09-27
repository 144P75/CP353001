CC = gcc
CFLAGS = -Wall -Wextra -D_DEFAULT_SOURCE -Iinclude
TARGET = editor

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

build/%.o: src/%.c include/editor.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)

.PHONY: clean
