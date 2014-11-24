

all: c2t

clean:
	rm c2t.h c2t
	cd asm; make clean


c2t: c2t.h
	gcc -Wall -O3 -o c2t c2t.c

c2t.h:
	./makeheader
