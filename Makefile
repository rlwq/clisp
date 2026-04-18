CC := gcc
CFLAGS := -I./include -Wall -Wextra
SRC := ./src/main.c ./src/tokenizer.c ./src/string_view.c
HEADERS := ./include/string_view.h
TARGET := ./build/clisp

build: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: build
	$(TARGET)

clean:
	rm -rf ./build
