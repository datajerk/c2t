
WIN32GCC = /usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-gcc

all: c2t c2t-96h

windows: c2t.exe c2t-96h.exe

dist: all windows

clean:
	rm c2t.h c2t c2t-96h
	cd asm; make clean

c2t: c2t.c c2t.h
	gcc -Wall -Wno-unused-value -Wno-unused-function -I. -O3 -o c2t c2t.c

c2t-96h: c2t-96h.c c2t.h
	gcc -Wall -Wno-unused-value -Wno-unused-function -I. -O3 -o c2t-96h c2t-96h.c

c2t.exe: c2t.c c2t.h
	$(WIN32GCC) -Wall -Wno-unused-value -Wno-unused-function -I. -O3 -o c2t.exe c2t.c

c2t-96h.exe: c2t-96h.c c2t.h
	$(WIN32GCC) -Wall -Wno-unused-value -Wno-unused-function -I. -O3 -o c2t-96h.exe c2t-96h.c

c2t.h: mon/dos33.boot1.mon mon/dos33.boot2.mon asm/autoload.s asm/diskload2.s asm/diskload3.s asm/diskload8000.s asm/diskload9600.s asm/fastload8000.s asm/fastload9600.s asm/fastloadcd.s asm/inflate.s
	./makeheader

test: c2t-96h c2t-96h.exe
	./test.sh
