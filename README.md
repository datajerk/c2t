## Introduction

`c2t` is a command line tool that can convert binary code/data and/or Apple-1/II Monitor text, as well as 140K disk images, into audio files suitable for use with the Apple-1 and II (II, II+, //e) cassette interface.

`c2t` offers three high-speed options for the 64K Apple II, II+, and //e: 8000 bps, 8820 bps, and 9600 bps.  The `c2t` compression option may be used to speedup the delivery of data with all three as well as the native 1333 bps cassette interface ROM routines.

8820 bps (used to burn CDs) and 9600 bps are not compatible with all II+s and //es.  If you plan to distribute your audio files, then use 8000 bps.

> 8820 bps and 1333 bps is not an option for disk images.

High-speed and compression options require `c2t`'s custom loader, and at this time that limits you to a single segment.  You can overcome this limitation by concatenating all your code together and creating your own code to shuffle your data around, or, pad each segment with enough zeros to align  subsequent segments with their target address and then use the compress option to minimize this overhead.

Multi-segment audio files can be created for the Apple-1, II, II+, and //e that can be loaded using the standard cassette interface ROM routines.

Examples of `c2t` in action:

- <http://asciiexpress.net/gameserver/readme.html>
- <http://asciiexpress.net/diskserver/readme.html>

For more info on `c2t` internals please read: <https://github.com/datajerk/c2t/raw/master/article/article.pdf>. 

### Why?

I created this because I needed a convenient way to get data loaded into my //e without dragging my computer out of my office (2nd floor) to my retro man cave (basement).  IOW, I needed an iPhone/iPad/mobile solution.  That, and CFFA3000 was sold out--at the time.


#### Yeah, but why?

You clearly do not understand the awesomeness of the Apple II, move along.


## Version

* c2t 0.997 (Sep 27 2017)
* c2t-96h 0.997 (Dec 31 2015)


## Binary Installation

Download <https://github.com/datajerk/c2t/archive/master.zip> and extract.

Both the archive and the repo `bin` directory contain macOS 64-bit (`c2t`) and Windows 32-bit (`c2t.exe`) binaries.  Just copy to any directory in your path.
> macOS users may need to adjust the permissions, e.g.:
>
```
sudo cp bin/c2t bin/c2t-96h /usr/local/bin
sudo chmod 755 /usr/local/bin/c2t-96h /usr/local/bin/c2t
```

An alternative and perhaps simplier install for macOS:

```
sudo curl https://raw.githubusercontent.com/datajerk/c2t/master/bin/c2t-96h >/usr/local/bin/c2t-96h
sudo curl https://raw.githubusercontent.com/datajerk/c2t/master/bin/c2t >/usr/local/bin/c2t
sudo chmod 755 /usr/local/bin/c2t-96h /usr/local/bin/c2t
```

## Build from Source

Prerequisites:

- `cl65` version 2.13.3 (<https://github.com/mrdudz/cc65-old/raw/master/cc65-sources-2.13.3.tar.bz2>)


```
git clone https://github.com/datajerk/c2t.git
```

### (UNIX, Linux, BSD, macOS (will not be universal), etc...)

```
make clean
make
```

> For Linux, read [ubuntu_notes.md](ubuntu_notes.md) for build tips.

### Universal macOS (x86_64/arm64)

```
make clean
make macos
```

### Windows

First install MinGW (<https://osdn.net/projects/mingw/>), then type from the root of this distribution:

```
PATH=C:\MinGW\bin;%PATH%
gcc -Wall -Wno-unused-value -Wno-unused-function -Wno-misleading-indentation -I. -O3 -static -o c2t c2t.c
```

To cross build for Windows from macOS, first install <http://crossgcc.rts-software.org/download/gcc-4.8.0-qt-4.8.4-win32/gcc-4.8.0-qt-4.8.4-for-mingw32.dmg>, then type:

```
make clean
make windows # or 'make dist' if you want both macOS and Windows built
```

> If macOS errors with _cannot be opened because the developer cannot be verified_ with the mingw32 gcc binaries, then type:
> 
```
sudo xattr -r -d com.apple.quarantine /usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32
```


## c2t-96h Version

`c2t-96h` is a hacked up version of `c2t` that fixes a few bugs (e.g. `.po` files) and adds better universal (should work on all Apple IIs) 9600 BPS code.  Both `-8` and `-f` activate this new 9600 BPS code.

`c2t-96h` will eventually replace `c2t`.  IOW, use `c2t-96h` for now.

> Older computers (e.g. circa 2008 MacBooks) have issues playing the `c2t-96h` generated output.  If you experience problems with `c2t-96h -f` or `c2t-96h -8` use `c2t -8`.  The merged `c2t` + `c2t-96h` version will support old and new computers.

Read <https://github.com/datajerk/c2t/raw/master/article/article.pdf> for details.


## Quick Start

To create an audio file that can auto extract to disk type:

`c2t-96h diskimage.dsk audiofilename.aif` or use `.wav`.

> `c2t-96h` supports `.dsk`, `.do`, and `.po` extensions.

> The disk image must be exactly 143360 bytes (140K).  800K disks are not supported.

> Use the `-n` option, e.g. `c2t-96h -n diskimage.dsk audiofilename.aif` if the disk ][ is emulated (e.g. CFFA3000, SDISK //, etc...)

To create an audio file for a single load binary than can auto extract and execute type:

`c2t-96h -2bc8 super_puckman,800 audiofilename.wav` or use `.aif`.

> The input image must be an Apple II binary with or without a 4-byte header.  The 4-byte header is the standard DOS header that defines the binary size and memory location.  Depending on how you extracted the single load binary you may or may not have this header.  If this header is missing you'll have to append to the binary name `,memorylocation` e.g. `,800`.

> A *Single Load Binary* is defined as a self contained executable that has no disk dependancies (e.g. data, overlays, game save data, high scores, etc...).  The binary must be 100% stateless or it will fail to function properly.  <http://asciiexpress.net/gameserver> has many examples.

In both cases you'll have a `.aif` or `.wav` formatted audio file.  It is critical that they file not be compress to MP3 or other format.  Read <https://github.com/datajerk/c2t/raw/master/article/article.pdf> for details.

To load up your disk or binary to a physical Apple //e or ][+:

* Connect an audio patch code from your phone, tablet, computer, etc... to the cassette-in port of the Apple II.
	
	> I just use Dropbox to quickly get these audio files to my iPhone.  I also play them from the Dropbox app.
	
* From the Apple II:
	* Remove any disks
	* Reboot or power on
	* CTRL-RESET to get to a `]` prompt
	* *Optionally* insert a blank disk if loading a disk image
	* At the `]` prompt type `LOAD` and press *RETURN*

* From your phone, tablet, computer *play* the audio file

Single load binaries will auto extract and execute.  Disk images will auto extract and expand to disk.


## Testing

Automated testing is only supported on macOS and requires the following:


* Virtual ][ 10.0.1 (<http://www.virtualii.com/>)
* Windows cross-compiling tools <http://crossgcc.rts-software.org/download/gcc-4.8.0-qt-4.8.4-win32/gcc-4.8.0-qt-4.8.4-for-mingw32.dmg>
* Docker Desktop for Mac (<https://hub.docker.com/editions/community/docker-ce-desktop-mac/>)

> You can edit `tests/test.md` if you do not want to test Windows binaries or want to use different images for test.

Build wine32 container (if testing Windows binaries):

```
# start docker first, make sure it is running
docker build --no-cache -t wine32 -f Dockerfile.wine32 .
```

To test, type:

```
make testclean # only once, unless you want to start over
make test
```

> If Virtual ][ crashes while testing, just `make test` again to restart failed test and continue where it left off.  Do not type `make testclean` again unless you want to start over.

Example output: <https://youtu.be/FCOb4f2hYN8>


## Tested disk ][ Configurations

> Most, if not all, disk ][ emulators (e.g. CFFA3000, SDISK //, etc...) do not support formatting.  The `-n` option must be used for testing in these cases.

The following configurations have been tested:

* Apple //e
	* Apple disk ][ verified (format and no-format)
	* Apple duodisk verified (format and no-format)
	* CFFA3000 3.1 verified with USB stick (no-format only)
	* CFFA3000 3.1 *failed* with IBM 4GB Microdrive (too slow)
	* Nishida Radio SDISK // verified (no-format only)
* Apple II+
	* Apple disk ][ verified (format and no-format)
	* Nishida Radio SDISK // verified (no-format only)
* Virtual ][ Emulator
	* Apple disk ][ verified (format and no-format)


## Synopsis

Output of `c2t -h`:
```
usage:  c2t      [-vh?]
        c2t      [-elp]         input[.mon],[addr] ... [output.mon]
        c2t {-1} [-cepr]        input[.mon],[addr] ... [output.[aif[f]|wav[e]]]
        c2t {-2} [-abcdef8pmqr] input[.mon],[addr] ... [output.[aif[f]|wav[e]]]
        c2t      [-n8]          input.dsk          ... [output.[aif[f]|wav[e]]]

        -1 or -2 for Apple I or II tape format
        -8 use 48k/8bit 8000 bps transfer (Apple II/II+/IIe 64K only)
           Implies -2a.  Negates -f and -d.
        -a assembly autoload and run (Apple II/II+/IIe 64K only)
        -b basic autoload and run (Apple II+/IIe 64K only)
           Implies -2a.
        -c compress data
        -d use fast 44.1k/16bit transfer (Apple II/II+/IIe 64K only)
           Implies -2a.  Negates -f and -8.  Use for burning CDs.
        -e pad with $00 to end on page boundary
        -f use faster 48k/8bit (9600 bps) transfer (Apple II/II+/IIe 64K only)
           Implies -2a.  Negates -8 and -d.  Unreliable on some systems.
        -h|? this help
        -l long monitor format (24 bytes/line)
        -m jump to monitor after autoload
        -n do not format disks
        -p pipe to stdout
        -q parameters and data only (for use with custom client)
        -r #, where # overrides the sample rate (e.g. -r 48000)
        -t 10 second preamble (default 4) for real tape use
        -v print version number and exit

input(s) without a .mon or .dsk extension is assumed to be a binary with a 4
byte header.  If the header is missing then you must append ,load_address to
each binary input missing a header, e.g. filename,800.  The load address
will be read as hex.

input(s) with a .mon extension expected input format:

        0280: A2 FF 9A 20 8C 02 20 4F
        0288: 03 4C 00 FF 20 9E 02 A9

A single input with a .dsk extension expected to be a 140K disk image.

output must have aiff, aif, wav, wave, or mon extention.
```

## Examples

------------------------------------------------------------------------------
```
Input:  Apple 1 monitor file with two segments.  First 4 lines:

0: 00 05 00 10 00 00 00 00
8: 00 00 00 00 00 00 00 00
280: A9 00 85 07 A9 00 A8 AA
288: 85 06 A5 00 85 04 A5 01

Command:

c2t -1 a1mt.mon a1mt.aif

Output:

Reading a1mt.mon, type monitor, segment 1, start: 0x0000, length: 16
Reading a1mt.mon, type monitor, segment 2, start: 0x0280, length: 290

Writing a1mt.aif as Apple I formatted aiff.

To load up and run on your Apple I, type:

        C100R
        0.FR 280.3A1R 
```
------------------------------------------------------------------------------
```
Input:  cc65/ca65 Apple II binary with DOS 4-byte header.  The DOS header
        contains the starting address of the program.

Command:

c2t -2 hello hello.wav

Output:

Reading hello, type binary, segment 1, start: 0x0803, length: 2958

Writing hello.wav as Apple II formatted wave.

To load up and run on your Apple II, type:

        CALL -151
        803.1390R 
        803G
```
------------------------------------------------------------------------------
```
Input:  cc65/ca65 Apple II binary with DOS 4-byte header.  The DOS header
        contains the starting address of the program.

Command:

c2t hello hello.mon

Output:

Reading hello, type binary, segment 1, start: 0x0803, length: 2958

Writing hello.mon as Apple formatted monitor.

Example hello.mon output:

0803: A2 FF 9A 2C 81
0808: C0 2C 81 C0 A9 91 A0 13
0810: 85 9B 84 9C A9 91 A0 13
0818: 85 96 84 97 A9 00 A0 D4
```
------------------------------------------------------------------------------
```
Input:  Binary game without DOS header that should be loaded at $801.

Command:

c2t -2 moon.patrol,801 moon.patrol.aif

Output:

Reading moon.patrol, type binary, segment 1, start: 0x0801, length: 18460

Writing moon.patrol.aif as Apple II formatted aiff.

To load up and run on your Apple II, type:

        CALL -151
        801.501CR 
        801G
```
------------------------------------------------------------------------------
```
Input:  Binary game without DOS header that should be loaded at $801 as fast
        as possible while being compatible with all Apple IIs.

Command:

c2t -8c moon.patrol,801 moon.patrol.aif

Reading moon.patrol, type binary, segment 1, start: 0x0801, length: 18460

Writing moon.patrol.aif as Apple II formatted aiff.

start: 0x7226, length: 18393, deflated: 0.36%, data time:18.95, inflate time:6.83
WARNING: compression disabled: no significant gain (18.11)

To load up and run on your Apple II, type:

        LOAD

NOTE:  Compression was disabled because it didn't help.
```
------------------------------------------------------------------------------
```
Input:  Binary game without DOS header that should be loaded at $800 as fast
        as possible while being compatible with all Apple IIs.

Command:

c2t -8c super_puckman,800 super_puckman.wav

Reading super_puckman, type binary, segment 1, start: 0x0800, length: 30719

Writing super_puckman.wav as Apple II formatted wave.

start: 0x886C, length: 12691, deflated: 58.69%, data time:13.25, inflate time:5.79

To load up and run on your Apple II, type:

        LOAD
```
------------------------------------------------------------------------------
```
Input:  Three binary files to be loaded at three different addresses.

c2t -2 foo,801 foo.obj,3ffd foo.pic,2000 foo.aif

Reading foo, type binary, segment 1, start: 0x0801, length: 91
Reading foo.obj, type binary, segment 2, start: 0x3FFD, length: 18947
Reading foo.pic, type binary, segment 3, start: 0x2000, length: 8192

Writing foo.aif as Apple II formatted aiff.

To load up and run on your Apple II, type:

        CALL -151
        801.85BR 3FFD.89FFR 2000.3FFFR 
```
------------------------------------------------------------------------------
```
Input:  DOS 3.3 140K diskette image to be loaded with maximum II
        compatibility.  Disk will be formatted first.

Command:

c2t -8 dos33.dsk dos33.wav

Output:

Reading dos33.dsk, type disk, segment 1, start: 0x1000, length: 28672
Reading dos33.dsk, type disk, segment 2, start: 0x1000, length: 28672
Reading dos33.dsk, type disk, segment 3, start: 0x1000, length: 28672
Reading dos33.dsk, type disk, segment 4, start: 0x1000, length: 28672
Reading dos33.dsk, type disk, segment 5, start: 0x1000, length: 28672

Writing dos33.wav as Apple II formatted wave.

Segment: 0, start: 0x459B, length: 19044, deflated: 33.58%, data time:19, inflate time:7.68
Segment: 1, start: 0x74A5, length:  7002, deflated: 75.58%, data time:7, inflate time:3.70
Segment: 2, start: 0x8514, length:  2795, deflated: 90.25%, data time:3, inflate time:2.28
Segment: 3, start: 0x6CD4, length:  9003, deflated: 68.60%, data time:9, inflate time:4.33
Segment: 4, start: 0x6DE6, length:  8729, deflated: 69.56%, data time:9, inflate time:4.27

To load up and run on your Apple II, type:

        LOAD
```
------------------------------------------------------------------------------
