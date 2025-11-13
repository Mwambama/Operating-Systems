CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c99
LDFLAGS = -pthread

# Target for the fine-grained server
TARGET = appserver

# Source files needed for compilation
SRCS = appserver.c Bank.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target: builds appserver
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Rule for compiling .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET) appserver-coarse