# ==============================================================================
# Makefile for Web-Based Hotel Management System (Pure C Backend)
# Works on both Linux and Windows (MinGW GCC)
# ==============================================================================

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

# Detect OS platform
ifeq ($(OS),Windows_NT)
    TARGET = backend/hotel_server.exe
    LDFLAGS = -lws2_32
    RM = del /f /q 2>NUL || true
else
    TARGET = backend/hotel_server
    LDFLAGS = 
    RM = rm -f
endif

all: $(TARGET)

$(TARGET): backend/hotel_server.c
	$(CC) $(CFLAGS) backend/hotel_server.c -o $(TARGET) $(LDFLAGS)

clean:
	$(RM) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
