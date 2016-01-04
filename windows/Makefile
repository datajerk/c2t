

CC = /usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-gcc

all: c2t.exe c2t-96h.exe

clean:
	rm -f c2t.exe c2t-96h.exe c2t.c c2t-96h.c c2t.h fake6502.h

c2t.exe: c2t.c c2t.h fake6502.h miniz.h
	$(CC) -Wall -Wno-unused-value -Wno-unused-function -O3 -o c2t.exe c2t.c

c2t-96h.exe: c2t-96h.c c2t.h fake6502.h miniz.h
	$(CC) -Wall -Wno-unused-value -Wno-unused-function -O3 -o c2t-96h.exe c2t-96h.c

c2t.c: ../c2t.c
	cp ../c2t.c .

c2t-96h.c: ../c2t-96h.c
	cp ../c2t-96h.c .

c2t.h: ../c2t.h
	cp ../c2t.h .

fake6502.h: ../fake6502.h
	cp ../fake6502.h .

