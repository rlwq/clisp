CC := gcc
CFLAGS := -I./include -Wall -Wextra -Wswitch-enum -Wswitch-default
SRC := $(wildcard ./src/*.c) 
HEADERS := $(wildcard ./include/*.h)
TARGET := ./build/clisp

debug: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) -g -fsanitize=address,undefined -fanalyzer -o $(TARGET)

build: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) -O3 -DNDEBUG -o $(TARGET)

run: build
	$(TARGET)

clean:
	rm -rf ./build *.plist

.PHONY: build debug run clean
