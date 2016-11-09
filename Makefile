CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=
EXEC=ptar

all:: $(EXEC)

ptar: main.o
	$(CC) -o ptar main.o $(LDFLAGS)

main.o: main.c header.h
	$(CC) $(CFLAGS) -o main.o -include header.h -c main.c $(CFLAGS)

.PHONY: clean mrproper

clean::
	rm -rf *.o

mrproper:: clean
	rm -rf $(EXEC)
			
