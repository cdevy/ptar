CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=
EXEC=ptar

all:: $(EXEC)

ptar: main.o utils.o
	cd zlib/;make distclean;./configure;make;cd ../;

	$(CC) -pthread -o ptar main.o utils.o $(LDFLAGS) -ldl

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -o utils.o -include utils.h -c utils.c $(CFLAGS)

main.o: main.c utils.h
	$(CC) $(CFLAGS) -o main.o -include utils.h -c main.c $(CFLAGS)

.PHONY: clean mrproper

clean::
	rm -rf *.o

mrproper:: clean
	rm -rf $(EXEC)
