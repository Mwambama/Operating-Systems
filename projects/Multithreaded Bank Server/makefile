
CC = gcc
# CFLAGS includes necessary warnings and the C99 standard
# -pthread is critical for compiling C code that uses pthreads
CFLAGS = -Wall -Wextra -pthread -std=c99 

# LDFLAGS is used by the linker
# -pthread links the final executable with the Pthreads library
LDFLAGS = -pthread 

# --- File Definitions ---
TARGET = appserver
SRCS = appserver.c Bank.c
OBJS = $(SRCS:.c=.o)
# Note: Project2Test.c must be compiled separately using a manual gcc command

# --- Build Rules ---

# Default target: builds appserver executable
all: $(TARGET)

# Rule for linking the executable (appserver: appserver.o Bank.o)
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Rule for compiling individual C files into object files (standard rule)
# This uses CFLAGS which includes -pthread for thread support
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up compiled files
clean:
	rm -f $(OBJS) $(TARGET) appserver-coarse