CC = cc

files = \
	main.c \
	interpreter.c \
	numbers.c \
	random.c

devperc: $(files)
	$(CC) -std=c99 -Wall -pedantic -o devperc $(files)

clean:
	rm devperc
