CC = gcc
CFLAGS = -ggdb3 -Wall -Wextra -pedantic -std=gnu99

DEPS := debug.h
DEPS += parser.h

OBJS := dash.o
OBJS += debug.o
OBJS += parser.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

dash: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

time-test:
	$(CC) -o dash-read dash_read_time.c
	$(CC) -o dash-fgets dash_fgets_time.c

clean:
	rm dash *.o
