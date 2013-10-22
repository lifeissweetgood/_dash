CC = gcc
CFLAGS = -ggdb3 -Wall -Wextra -pedantic -std=gnu99

PWD = ~/github/_dash
INCLUDE_DIR = $(PWD)/include
BIN_DIR = $(PWD)/bin
TEST_DIR = $(PWD)/test
SRC_DIR = $(PWD)/src

DEPS := $(INCLUDE_DIR)/debug.h
DEPS += $(INCLUDE_DIR)/parser.h
DEPS += $(INCLUDE_DIR)/communicator.h

OBJS := $(BIN_DIR)/dash.o
OBJS += $(BIN_DIR)/debug.o
OBJS += $(BIN_DIR)/parser.o
OBJS += $(BIN_DIR)/communicator.o

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

dash: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

time-test:
	$(CC) -o dash-read $(TEST_DIR)/dash_read_time.c
	$(CC) -o dash-fgets $(TEST_DIR)/dash_fgets_time.c

clean:
	rm dash $(BIN_DIR)/*
