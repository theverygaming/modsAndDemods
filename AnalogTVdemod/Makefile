CFLAGS = -Wall -Wshadow -O3 -g
LDLIBS = -lsndfile -lm -lvolk
CC = gcc

main: TVdemod.o filter.o
	$(CC) TVdemod.o filter.o $(LDLIBS) -o build/TVdemod

clean:
	$(RM) *.o
	$(RM) build/TVdemod
