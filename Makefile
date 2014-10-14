CFLAGS=-O3 -Wall -Werror -g
CC=mpicc
BINARYNAME=lz78

OBJFILES=main.o wrapper.o lz78.o bitio.o mp.o distribute.o time.o

all: $(BINARYNAME)

$(BINARYNAME): $(OBJFILES)
	$(CC) -o $@ $^ 

main.o: wrapper.h bitio.h distribute.h mp.h
wrapper.o: wrapper.h lz78.h 
lz78.o: lz78.h bitio.h
bitio.o: bitio.h
mp.o: mp.h mp.c
distribute.o: distribute.h distribute.c
time.o: time.h time.c

clean:
	rm -rf $(OBJFILES) $(BINARYNAME)
