CC ?= cc
CFLAGS ?= -Os
CPPFLAGS ?= -Wall -Wextra

all: stacker.c
        $(CC) $(CFLAGS) $(CPPFLAGS) $< -o stacker
