
WIN32GCC = /usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-gcc

all: nix

nix: bin/c2t bin/c2t-96h

windows: bin/c2t.exe bin/c2t-96h.exe

macos: bin/c2t_x86 bin/c2t_arm bin/c2t-96h_x86 bin/c2t-96h_arm
	SDKROOT=$(shell xcrun --sdk macosx --show-sdk-path)
	lipo -create -output bin/c2t bin/c2t_x86 bin/c2t_arm
	lipo -create -output bin/c2t-96h bin/c2t-96h_x86 bin/c2t-96h_arm

dist: macos windows

clean: testclean
	rm -rf c2t.h bin/c2t bin/c2t-96h bin/c2t.exe bin/c2t-96h.exe bin/c2t_x86 bin/c2t_arm bin/c2t-96h_x86 bin/c2t-96h_arm cc65-sources-2.13.3.tar.bz2 cc65-2.13.3
	cd asm; make clean

# nix
bin/c2t: c2t.c c2t.h
	$(CC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -o bin/c2t c2t.c -lm

bin/c2t-96h: c2t-96h.c c2t.h
	$(CC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -o bin/c2t-96h c2t-96h.c -lm

# macos universal
bin/c2t_x86: c2t.c c2t.h
	$(CC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -target x86_64-apple-macos10.12 -o $@ c2t.c -lm

bin/c2t-96h_x86: c2t-96h.c c2t.h
	$(CC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -target x86_64-apple-macos10.12 -o $@ c2t-96h.c -lm

bin/c2t_arm: c2t.c c2t.h
	$(CC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -target arm64-apple-macos11 -o $@ c2t.c -lm

bin/c2t-96h_arm: c2t-96h.c c2t.h
	$(CC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -target arm64-apple-macos11 -o $@ c2t-96h.c -lm

# windows
bin/c2t.exe: c2t.c c2t.h
	$(WIN32GCC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -o bin/c2t.exe c2t.c

bin/c2t-96h.exe: c2t-96h.c c2t.h
	$(WIN32GCC) -Wall -Wno-strict-aliasing -Wno-unused-value -Wno-unused-function -I. -O3 -o bin/c2t-96h.exe c2t-96h.c

cc65-sources-2.13.3.tar.bz2:
	curl -sLO https://github.com/mrdudz/cc65-old/raw/master/cc65-sources-2.13.3.tar.bz2

cc65-2.13.3/bin/cl65: cc65-sources-2.13.3.tar.bz2
	bunzip2 -c cc65-sources-2.13.3.tar.bz2 | tar xf -
	(cd cc65-2.13.3; /usr/bin/sed 's!/usr/local!'${PWD}'/cc65-2.13.3!' <make/gcc.mak >Makefile; make -j4 bins || make bins && make install || true)

c2t.h: c2t.h.0 makeheader mon/dos33.boot1.mon mon/dos33.boot2.mon asm/autoload.s asm/diskload2.s asm/diskload3.s asm/diskload8000.s asm/diskload9600.s asm/fastload8000.s asm/fastload9600.s asm/fastloadcd.s asm/inflate.s cc65-2.13.3/bin/cl65
	./makeheader

test: bin/c2t-96h bin/c2t-96h.exe tests/test.md
	cd tests; ./test.sh test.md

quicktest: testclean bin/c2t-96h tests/quicktest.md
	cd tests; ./test.sh quicktest.md

testclean:
	cd tests; rm -f passed test.log
