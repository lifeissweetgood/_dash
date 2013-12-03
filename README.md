_dash
======
###A C shell!

_dash is a command-line shell implemented in C.  Current functionality includes parsing of user input and piping any number of commands.  Future plans: add unit tests, implement I/O redirection

##To Compile
$ make

##To Run
$ ./dash

##To Compile Time Tests
### Tests for comparing speed of fgets() vs. speed of read()
$ make time-tests

###To Run Time Tests
$ ./test/dash-read

$ ./test/dash-fgets
