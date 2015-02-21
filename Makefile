CC = cc

files = \
	main.c \
	interpreter.c \
	numbers.c

devperc: $(files)
	$(CC) -std=c99 -Wall -pedantic -o devperc $(files)
