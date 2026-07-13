CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu99

TARGET = smallsh
SRC = smallsh.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
