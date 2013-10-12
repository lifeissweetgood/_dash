make:
	gcc -o dash dash.c -ggdb3 -Wall -Wextra -pedantic -std=gnu99

time-test:
	gcc -o dash-read dash_read_time.c
	gcc -o dash-fgets dash_fgets_time.c

clean:
	rm dash
