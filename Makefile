CC = gcc
CFLAGS = -Wall -Werror

my_shell: my_shell.o
	$(CC) $(CFLAGS) -o my_shell my_shell.o

my_shell.o: my_shell.c
	$(CC) $(CFLAGS) -c my_shell.c
