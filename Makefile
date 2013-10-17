make:
	gcc -o dash dash.c

time-test:
	gcc -o dash-read dash_read_time.c
	gcc -o dash-fgets dash_fgets_time.c

clean:
	rm dash
