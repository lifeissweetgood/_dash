make:
	gcc -o dash dash.c -ggdb3 -Wall -Wextra -pedantic -std=gnu99

clean:
	rm dash
