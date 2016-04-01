all:
	gcc -g -Wall -Wpedantic -o example thpool.c example.c -lpthread

clean:
	rm -f example *.o core
