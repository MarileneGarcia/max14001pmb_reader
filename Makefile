# Makefile for MAX14001PMB Reader Program

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -pthread -O2

# Source and target
SRC = max14001pmb_reader.c
TARGET = max14001pmb_reader

# Default target
all: compile

# Compile target
compile: $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean target
clean:
	rm -f $(TARGET)

# Run target: compile if needed, then execute
run: compile
	./$(TARGET)