

all: c2t

clean:
	rm c2t.h c2t
	cd asm; make clean

c2t: c2t.h
	gcc -Wall -O3 -o c2t c2t.c

c2t.h: dos33.boot1.mon dos33.boot2.mon asm/autoload.s asm/diskload2.s asm/diskload3.s asm/diskload8000.s asm/diskload9600.s asm/fastload8000.s asm/fastload9600.s asm/fastloadcd.s asm/inflate.s
	./makeheader
