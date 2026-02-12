# makefile for Compiler Project
# CS F363 - BITS Pilani
# Compatible with GCC

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
LDFLAGS = 

# Target executable
TARGET = stage1exe

# Source files
SRCS = driver.c lexer.c parser.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files (for dependency tracking)
HEADERS = lexerDef.h lexer.h parserDef.h parser.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile driver.c
driver.o: driver.c lexer.h parser.h
	$(CC) $(CFLAGS) -c driver.c -o driver.o

# Compile lexer.c
lexer.o: lexer.c lexerDef.h lexer.h
	$(CC) $(CFLAGS) -c lexer.c -o lexer.o

# Compile parser.c
parser.o: parser.c parserDef.h parser.h lexer.h
	$(CC) $(CFLAGS) -c parser.c -o parser.o

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Clean and rebuild
rebuild: clean all

.PHONY: all clean rebuild
