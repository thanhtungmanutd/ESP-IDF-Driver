# Compiler and compiler flag
CC = gcc

# Source files and object files
SRCS = server.c tcp_server.c

# Target executable
OBJ = server

all: $(SRCS)
		$(CC) -o $(OBJ) `xml2-config --cflags` $(SRCS) `xml2-config --libs`

clean:
		rm -rf $(OBJ)