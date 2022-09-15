/*

c2t, Code to Tape|Text, Version 0.997, Wed Sep 27 15:27:56 GMT 2017

Parts copyright (c) 2011-2017 All Rights Reserved, Egan Ford (egan@sense.net)

THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Built on work by:
	* Mike Willegal (http://www.willegal.net/appleii/toaiff.c)
	* Paul Bourke (http://paulbourke.net/dataformats/audio/, AIFF and WAVE output code)
	* Malcolm Slaney and Ken Turkowski (Integer to IEEE 80-bit float code)
	* Lance Leventhal and Winthrop Saville (6502 Assembly Language Subroutines, CRC 6502 code)
    * Piotr Fusik (http://atariarea.krap.pl/x-asm/inflate.html, inflate 6502 code)
    * Rich Geldreich (http://code.google.com/p/miniz/, deflate C code)
    * Mike Chambers (http://rubbermallet.org/fake6502.c, 6502 simulator)

License:
	*  Do what you like, remember to credit all sources when using.

Description:
	This small utility will read Apple I/II binary and
	monitor text files and output Apple I or II AIFF and WAV
	audio files for use with the Apple I and II cassette
	interface.

Features:
	*  Apple I, II, II+, IIe support.
	*  Big and little-endian machine support.
		o  Little-endian tested.
	*  AIFF and WAVE output (both tested).
	*  Platforms tested:
		o  32-bit/64-bit x86 OS/X.
		o  32-bit/64-bit x86 Linux.
		o  32-bit x86 Windows/Cygwin.
		o  32-bit x86 Windows/MinGW.
	*  Multi-segment tapes.

Compile:
	OS/X:
		gcc -Wall -O -o c2t c2t.c
	Linux:
		gcc -Wall -O -o c2t c2t.c -lm
	Windows/Cygwin:
		gcc -Wall -O -o c2t c2t.c
	Windows/MinGW:
		PATH=C:\MinGW\bin;%PATH%
		gcc -Wall -O -static -o c2t c2t.c

Notes:
	*  Virtual ][ only supports .aif (or .cass)
	*  Dropbox only supports .wav and .aiff (do not use .wave or .aif)

Not yet done:
	*  Test big-endian.
	*  gnuindent
    *  Redo malloc code in appendtone

Thinking about:
	*  Check for existing file and abort, or warn, or prompt.
	*  -q quiet option for Makefiles
	*  autoload support for basic programs

Bugs:
	*  Probably

*/

#if defined(_WIN32)
#include "miniz_win32.h"
#else
#include "miniz.h"
#endif

#include <fake6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <c2t.h>

#define ABS(x) (((x) < 0) ? -(x) : (x))

#define VERSION "Version 0.997"
#define OUTFILE argv[argc-1]
#define BINARY 0
#define MONITOR 1
#define AIFF 2
#define WAVE 3
#define DSK 4 

#define WRITEBYTE(x) { \
	unsigned char wb_j, wb_temp=(x); \
	for(wb_j=0;wb_j<8;wb_j++) { \
		if(wb_temp & 0x80) \
			appendtone(&output,&outputlength,freq1,rate,0,1,&offset); \
		else \
			appendtone(&output,&outputlength,freq0,rate,0,1,&offset); \
		wb_temp<<=1; \
	} \
}

void usage();
char *getext(char *filename);
void appendtone(double **sound, long *length, int freq, int rate, double time, double cycles, int *offset);
void Write_AIFF(FILE * fptr, double *samples, long nsamples, int nfreq, int bits, double amp);
void Write_WAVE(FILE * fptr, double *samples, long nsamples, int nfreq, int bits, double amp);
void ConvertToIeeeExtended(double num, unsigned char *bytes);
uint8_t read6502(uint16_t address);
void write6502(uint16_t address, uint8_t value);

unsigned char ram[65536];
int square = 0;

typedef struct seg {
	int start;
	int length;
	int codelength;
	unsigned char *data;
	char filename[256];
} segment;

int main(int argc, char **argv)
{
	FILE *ofp;
	double *output = NULL, amp=0.75;
	long outputlength=0;
	int i, c, model=0, outputtype, offset=0, fileoutput=1, warm=0, dsk=0, noformat=0, k8=0, qr=0;
	int autoload=0, basicload=0, compress=0, fast=0, cd=0, tape=0, endpage=0, longmon=0, rate=11025, bits=8, freq0=2000, freq1=1000, freq_pre=770, freq_end=770;
	char *filetypes[] = {"binary","monitor","aiff","wave","disk"};
	char *modeltypes[] = {"\b","I","II"};
	char *ext;
	unsigned int numseg = 0;
	segment *segments = NULL;

	opterr = 1;
	while((c = getopt(argc, argv, "12vabcftdpn8meh?lqr:")) != -1)
		switch(c) {
			case '1':		// apple 1
				rate = 8000;
				model = 1;
				break;
			case '2':		// apple 2
				model = 2;
				break;
			case 'v':		// version
				fprintf(stderr,"\n%s\n\n",VERSION);
				return 1;
				break;
			case 'a':		// assembly autoloader
				model = 2;
				autoload = 1;
				break;
			case 'b':		// basic autoloader
				model = 2;
				basicload = autoload = 1;
				break;
			case 'c':		// compression
				model = 2;
				autoload = compress = 1;
				break;
			case 'f':		// hifreq
				rate = 48000;
				model = 2;
				autoload = fast = 1;
				cd = k8 = 0;
				break;
			case 'd':		// hifreq CD
				rate = 44100;
				bits = 16;
				amp = 1.0;
				model = 2;
				cd = autoload = 1;
				fast = k8 = 0;
				break;
			case 't':		// 10 sec leader
				tape = 6;
				amp = 1.0;
				break;
			case 'm':		// drop to monitor after load
				warm = 1;
				break;
			case 'e':		// end on page boundary 
				endpage = 1;
				break;
			case 'p':		// stdout
				fileoutput = 0;
				break;
			case 'n':
				noformat = 1;
				break;
			case '8':		// 8k
				rate = 48000;
				model = 2;
				autoload = k8 = 1;
				fast = cd = 0;
				break;
			case 'h':		// help
			case '?':
				usage();
				return 1;
			case 'q':		// qr code support
				rate = 48000;
				model = 2;
				autoload = k8 = qr = 1;
				fast = cd = 0;
				break;
			case 'l':		// long mon lines
				longmon = 1;
				break;
			case 'r':		// override rate for -1/-2 only
				rate = atoi(optarg);
				autoload = basicload = k8 = qr = fast = cd = 0;
				break;
		}

	if(argc - optind < 1 + fileoutput) {
		usage();
		return 1;
	}

	// read intput files

	fprintf(stderr,"\n");
	for(i=optind;i<argc-fileoutput;i++) {
		char *start;
		unsigned char b, *data;
		int j, k, inputtype=BINARY;
		segment *tmp;
		FILE *ifp;

		if((tmp = realloc(segments, (numseg+1) * sizeof(segment))) == NULL) {
			fprintf(stderr,"could not allocate segment %d\n",numseg+1);
			abort();
		}
		segments = tmp;

		k=0;
		for(j=0;j<strlen(argv[i]);j++) {
			if(argv[i][j] == ',') {
				j++; // skip over comma, if present
				break;
			}
			segments[numseg].filename[k++]=argv[i][j];
		}
		segments[numseg].filename[k] = '\0';
		// TODO: store as basename, check for MINGW compat

		start = &argv[i][j]; // points at the start address or at '\0' if no start address given
		if(!*start)
			segments[numseg].start = -1;
		else
			segments[numseg].start = (int)strtol(start, (char **)NULL, 16);

		if((ext = getext(segments[numseg].filename)) != NULL)
			if(strcmp(ext,"mon") == 0)
				inputtype = MONITOR;

		if((ext = getext(segments[numseg].filename)) != NULL)
			if(strcmp(ext,"dsk") == 0)
				inputtype = DSK;

//TODO: Windows needs "rb", check UNIX/Linux

		if ((ifp = fopen(segments[numseg].filename, "rb")) == NULL) {
			fprintf(stderr,"Cannot read: %s\n\n",segments[numseg].filename);
			return 1;
		}

		fprintf(stderr,"Reading %s, type %s, segment %d, start: ",segments[numseg].filename,filetypes[inputtype],numseg+1);

//hack to support dumping disks for testing, should be 48, not 140 (really should be dynamic)

		if((data = malloc(140*1024*sizeof(char))) == NULL) {
			fprintf(stderr,"could not allocate 140K data\n");
			abort();
		}

		if(inputtype == DSK) {
			dsk = 1;
			segments[numseg].length = 0;
			for(i=0;i<5;i++) {
				//segments[numseg].start=i*(140 * 1024 / 5);
				segments[numseg].start=0x1000;

				while(fread(&b, 1, 1, ifp) == 1 && segments[numseg].length < (140 * 1024 / 5))
					data[segments[numseg].length++]=b;

				segments[numseg].data = data;
				fprintf(stderr,"0x%04X, length: %d\n",segments[numseg].start,segments[numseg].length);

				if(segments[numseg].length != (140 * 1024 / 5)) {
					fprintf(stderr,"\n%s segment too short (< %d) for file type DISK\n\n",segments[numseg].filename,140*1024/5);
					return 1;
				}

				if(i==4)
					break;

				numseg++;
				if((tmp = realloc(segments, (numseg+1) * sizeof(segment))) == NULL) {
					fprintf(stderr,"could not allocate segment %d\n",numseg+1);
					abort();
				}
				segments = tmp;
				strcpy(segments[numseg].filename,segments[numseg-1].filename);
				segments[numseg].length = 0;
				if((data = malloc(48*1024*sizeof(char))) == NULL) {
					fprintf(stderr,"could not allocate 48K data\n");
					abort();
				}
				data[segments[numseg].length++]=b;

				fprintf(stderr,"Reading %s, type %s, segment %d, start: ",segments[numseg].filename,filetypes[inputtype],numseg+1);
			}
		}

		if(inputtype == BINARY) {
			if(segments[numseg].start == -1) {
				fread(&b, 1, 1, ifp);
				segments[numseg].start = b;
				fread(&b, 1, 1, ifp);
				segments[numseg].start |= b << 8;
				fread(&b, 1, 1, ifp);
				segments[numseg].length = b;
				fread(&b, 1, 1, ifp);
				segments[numseg].length |= b << 8;
			}

			segments[numseg].length=0;
			while(fread(&b, 1, 1, ifp) == 1)
				data[segments[numseg].length++]=b;

			segments[numseg].data = data;
			fprintf(stderr,"0x%04X, length: %d\n",segments[numseg].start,segments[numseg].length);
		}

		if(inputtype == MONITOR) {
			int byte, naddr;
			char addrs[8], s;

			segments[numseg].start = -1;
			segments[numseg].length = 0;

			while(fscanf(ifp,"%s ",addrs) != EOF) {
				naddr = (int)strtol(addrs, (char **)NULL, 16);
				if(segments[numseg].start == -1)
					segments[numseg].start = naddr;
	
				if(naddr != segments[numseg].start + segments[numseg].length) { // multi segment
					segments[numseg].data = data;
					fprintf(stderr,"0x%04X, length: %d\n",segments[numseg].start,segments[numseg].length);
					numseg++;
					if((tmp = realloc(segments, (numseg+1) * sizeof(segment))) == NULL) {
						fprintf(stderr,"could not allocate segment %d\n",numseg+1);
						abort();
					}
					segments = tmp;
					if((data = malloc(48*1024*sizeof(char))) == NULL) {
						fprintf(stderr,"could not allocate 48K data\n");
						abort();
					}
					segments[numseg].start = naddr;
					segments[numseg].length = 0;
					strcpy(segments[numseg].filename,segments[numseg-1].filename);
					fprintf(stderr,"Reading %s, type %s, segment %d, start: ",segments[numseg].filename,filetypes[inputtype],numseg+1);
				}
	
				while (fscanf(ifp, "%x%c", &byte, &s) != EOF) {
					data[segments[numseg].length++]=byte;
					if (s == '\n' || s == '\r')
						break;
				}
			}
			segments[numseg].data = data;
			fprintf(stderr,"0x%04X, length: %d\n",segments[numseg].start,segments[numseg].length);
		}

		fclose(ifp);
		numseg++;
	}
	fprintf(stderr,"\n");

	if(dsk) {
		fast=autoload=cd=tape=0;
		model=2;

		if(numseg != 5) {
			fprintf(stderr,"Number of segments != 5 and/or not of length %d\n\n",140*1024/5);
			return 1;
		}
		else {
			for(i=0;i<5;i++) {
				if(segments[i].length != 140*1024/5) {
					fprintf(stderr,"Number of segments != 5 and/or not of length %d\n\n",140*1024/5);
					return 1;
				}
			}	
		}
	}

	if(endpage)
		for(i=0;i<numseg;i++) {
			int pad = (0xFF - ((segments[i].length + segments[i].start - 1) & 0xFF));

			segments[i].length += pad;
			while(pad--)
				segments[i].data[segments[i].length - pad - 1] = 0;
		}

	if(numseg > 1 || model == 1) {
		if(autoload)
			fprintf(stderr,"WARNING: number of segments > 1 or model = 1: autoload and fast disabled.\n\n");
		autoload = fast = 0;
	}

	if(fileoutput) {
		if((ext = getext(OUTFILE)) == NULL) {
			usage();
			return 1;
		}
		else {
			if(strcmp(ext,"aiff") == 0 || strcmp(ext,"aif") == 0)
				outputtype = AIFF;
			else if(strcmp(ext,"wave") == 0 || strcmp(ext,"wav") == 0)
				outputtype = WAVE;
			else if(strcmp(ext,"mon") == 0)
				outputtype = MONITOR;
			else {
				usage();
				return 1;
			}
		}
	}
	else {
/*
		if(!model)
			outputtype = MONITOR;
		else
			outputtype = AIFF;
*/
		outputtype = MONITOR;
	}

	if(outputtype != MONITOR && !model) {
		fprintf(stderr,"\nYou must specify -1 or -2 for Apple I or II tape format, exiting.\n\n");
		return 1;
	}

	// TODO: check for existing file and abort, or warn, or prompt

	ofp=stdout;
	if(fileoutput) {
		if ((ofp = fopen(OUTFILE, "w")) == NULL) {
			fprintf(stderr,"\nCannot write: %s\n\n",OUTFILE);
			return 1;
		}
		fprintf(stderr,"Writing %s as Apple %s formatted %s.\n\n",OUTFILE,modeltypes[model],filetypes[outputtype]);
	}
	else
		fprintf(stderr,"Writing %s as Apple %s formatted %s.\n\n","STDOUT",modeltypes[model],filetypes[outputtype]);

	if(outputtype == MONITOR) {
		int i, j, saddr;
		// unsigned long cmp_len;
		size_t cmp_len;
		unsigned char *cmp_data;

		for(i=0;i<numseg;i++) {
			if(compress) {
				cmp_data = tdefl_compress_mem_to_heap(segments[i].data, segments[i].length, &cmp_len, TDEFL_MAX_PROBES_MASK);
				free(segments[i].data);
				segments[i].data = cmp_data;
				segments[i].length = cmp_len;
			}
			saddr = segments[i].start;
			fprintf(ofp,"%04X:", saddr);
			for(j=0;j<segments[i].length;j++) {
				fprintf(ofp," %02X", segments[i].data[j]);
				if(++saddr % (8+(24*longmon)) == 0 && j < segments[i].length - 1)
					fprintf(ofp,"\n%04X:",saddr);
			}
			fprintf(ofp,"\n");
		}

		fclose(ofp);
		return 0;
	}

	// write out code
	if(!autoload  && !dsk) {
		int i, j;
		//unsigned long cmp_len;
		size_t cmp_len;
		unsigned char *cmp_data;
		char checksum;

		for(i=0;i<numseg;i++) {
			// header
			if(model == 1) {
				appendtone(&output,&outputlength,1000,rate,4.0+tape,0,&offset);
				appendtone(&output,&outputlength,2000,rate,0,1,&offset);
			}
			else {
				appendtone(&output,&outputlength,770,rate,4.0+tape,0,&offset);
				appendtone(&output,&outputlength,2500,rate,0,0.5,&offset);
				appendtone(&output,&outputlength,2000,rate,0,0.5,&offset);
			}
			checksum = 0xff;

			if(compress) {
				cmp_data = tdefl_compress_mem_to_heap(segments[i].data, segments[i].length, &cmp_len, TDEFL_MAX_PROBES_MASK);
				free(segments[i].data);
				segments[i].data = cmp_data;
				segments[i].length = cmp_len;
			}
			for(j=0;j<segments[i].length;j++) {
				WRITEBYTE(segments[i].data[j]);
				checksum ^= segments[i].data[j];
			}
	
			// checksum/endbits
			if(model == 2)
				WRITEBYTE(checksum);
			appendtone(&output,&outputlength,1000,rate,0,1,&offset);
		}

		// friendly help
		fprintf(stderr,"To load up and run on your Apple %s, type:\n\n",modeltypes[model]);
		if(model == 1)
			fprintf(stderr,"\tC100R\n\t");
		else
			fprintf(stderr,"\tCALL -151\n\t");

		for(i=0;i<numseg;i++)
			fprintf(stderr,"%X.%XR ",segments[i].start,segments[i].start+segments[i].length-1);
		fprintf(stderr,"\n");

		if(numseg == 1) {
			if(model == 1)
				fprintf(stderr,"\t%XR\n",segments[0].start);
			else
				fprintf(stderr,"\t%XG\n",segments[0].start);
		}
		fprintf(stderr,"\n");
	}

	if(autoload) {
		char eta[40], loading[]=" LOADING ";
		unsigned char byte, checksum, *cmp_data, table[12];
		unsigned long ones=0, zeros=0;
		size_t cmp_len;
		unsigned int length, move_len;
		int i, j;

		appendtone(&output,&outputlength,770,rate,4.0+tape,0,&offset);
		appendtone(&output,&outputlength,2500,rate,0,0.5,&offset);
		appendtone(&output,&outputlength,2000,rate,0,0.5,&offset);

		// compute uncompressed ETA
		for(j=0;j<segments[0].length;j++) {
			byte=segments[0].data[j];
			for(i=0;i<8;i++) {
				if(byte & 0x80)
					ones++;
				else
					zeros++;
				byte <<= 1;
			}
		}

		if(fast) {
			freq0 = 12000;
			freq1 = 8000;
			freq_pre = 6000;
			freq_end = 2000;
		}
		if(k8) {
			freq0 = 12000;
			freq1 = 6000;
			freq_pre = 2000;
			freq_end = 770;
		}
		if(cd) {
			freq0 = 11025;
			freq1 = 7350;
			freq_pre = 5512;
			freq_end = 2000;
		}

		if(compress) {
			unsigned long cmp_ones=0, cmp_zeros=0;
			double inflate_time = 0;
			unsigned int endj;

			cmp_data = tdefl_compress_mem_to_heap(segments[0].data, segments[0].length, &cmp_len, TDEFL_MAX_PROBES_MASK);

			for(j=0;j<cmp_len;j++) {
				byte=cmp_data[j];
				for(i=0;i<8;i++) {
					if(byte & 0x80)
						cmp_ones++;
					else
						cmp_zeros++;
					byte <<= 1;
				}
			}

			// we need to append inflate/decompress code to end of data
			for(j=0;j<sizeof(inflatecode)/sizeof(char);j++) {
				byte=inflatecode[j];
				for(i=0;i<8;i++) {
					if(byte & 0x80)
						cmp_ones++;
					else
						cmp_zeros++;
					byte <<= 1;
				}
			}

			//compute inflate time

			//load up inflate data
			checksum = 0xff;
			for(j=0;j<cmp_len;j++) {
				ram[0xBA00 - cmp_len + j] = cmp_data[j];
				checksum ^= cmp_data[j];
			}
			//load up inflate code
			for(j=0;j<sizeof(inflatecode)/sizeof(char);j++) {
				ram[0xBA00 + j] = inflatecode[j];
				checksum ^= inflatecode[j];
			}
			ram[0xBA00 + j] = checksum;
			endj = 0xBA00 + j + 1;

			if(k8) {
				for(j=(0x823 - 0x80C);j<sizeof(fastload8000)/sizeof(char);j++)
					ram[0xBE80 - (0x823 - 0x80C) + j] = fastload8000[j];
				ram[0xBE80 - (0x823 - 0x80C) + j++] = (0xBA00 - cmp_len) & 0xFF;
				ram[0xBE80 - (0x823 - 0x80C) + j++] = (0xBA00 - cmp_len) >> 8;
				ram[0xBE80 - (0x823 - 0x80C) + j++] = endj & 0xFF;
				ram[0xBE80 - (0x823 - 0x80C) + j++] = endj >> 8;
				ram[0x00] = 0xFF;
				ram[0xBF09] = 0x00; //BRK

				reset6502();
				exec6502(0xBEE3);

				if(ram[0x00] != 0)
					fprintf(stderr,"WARNING: simulated checksum failed: %02X\n",ram[0x00]);

				inflate_time += clockticks6502/1023000.0;
			}

			//zero page src
			ram[0x0] = (0xBA00 - cmp_len) & 0xFF;
			ram[0x1] = (0xBA00 - cmp_len) >> 8;
			//zero page dst
			ram[0x2] = (segments[0].start) & 0xFF; 
			ram[0x3] = (segments[0].start) >> 8;
			//setup JSR
			ram[0xBF00] = 0x20; // JSR $9B00
			ram[0xBF01] = 0x00;
			ram[0xBF02] = 0xBA;
			ram[0xBF03] = 0x00; //BRK to stop simulation
			//run it
			reset6502();
			exec6502(0xBF00);
			//compare (just to be safe)
			for(j=0;j<segments[0].length;j++)
				if(ram[segments[0].start + j] != segments[0].data[j]) {
					fprintf(stderr,"WARNING: simulated inflate failed at %04X\n",j+0x1000);
					break;
				}
			inflate_time += clockticks6502/1023000.0;

			fprintf(stderr,"start: 0x%04X, length: %5d, deflated: %.02f%%, data time:%.02f, inflate time:%.02f\n",(unsigned int)(0xB9FF - cmp_len),(unsigned int)cmp_len,100.0*(1-cmp_len/(float)segments[0].length),cmp_ones/(float)freq1 + cmp_zeros/(float)freq0,inflate_time);

			if((ones/(float)freq1 + zeros/(float)freq0) < inflate_time + (cmp_ones/(float)freq1 + cmp_zeros/(float)freq0)) {
				fprintf(stderr,"WARNING: compression disabled: no significant gain (%.02f)\n",ones/(float)freq1 + zeros/(float)freq0);
				compress = 0;
			}
			else {
				free(segments[0].data);
				segments[0].data = cmp_data;
				segments[0].codelength = segments[0].length;
				segments[0].length = cmp_len;
				ones=cmp_ones;
				zeros=cmp_zeros;
			}
			fprintf(stderr,"\n");
		}

		sprintf(eta,", ETA %d SEC. ",(int) (ones/(float)freq1 + zeros/(float)freq0 + 0.5 + 0.25 + (3.75 * ((k8|cd|fast) == 0))) );

		length = sizeof(basic)/sizeof(char) + sizeof(table)/sizeof(char) + strlen(loading) + strlen(segments[0].filename) + strlen(eta) + 1;

		move_len = (0x823 - 0x80C);
		if(fast)
			length += sizeof(fastload9600)/sizeof(char);
		else
			if(k8)
				length += sizeof(fastload8000)/sizeof(char);
			else
				if(cd)
					length += sizeof(fastloadcd)/sizeof(char);
				else {
					length += sizeof(autoloadcode)/sizeof(char);
					move_len = (0x81A - 0x80C);
				}

		if(fast | k8 | cd) {
			if(length - sizeof(basic)/sizeof(char) - move_len > 384) {
				segments[0].filename[strlen(segments[0].filename) - (length - sizeof(basic)/sizeof(char) - move_len - 384)] = '\0';
				fprintf(stderr,"WARNING: BF00 page overflow: truncating display filename to %s\n\n",segments[0].filename);
				length = 384 + sizeof(basic)/sizeof(char) + move_len;
			}
		}
		else {
			if(length - sizeof(basic)/sizeof(char) - move_len > 256) {
				segments[0].filename[strlen(segments[0].filename) - (length - sizeof(basic)/sizeof(char) - move_len - 256)] = '\0';
				fprintf(stderr,"WARNING: BF00 page overflow: truncating display filename to %s\n\n",segments[0].filename);
				length = 256 + sizeof(basic)/sizeof(char) + move_len;
			}
		}

		freq0 = 2000;
		freq1 = 1000;
		checksum = 0xff;

		if(basicload) { // write basic stub
			header[0] = length & 0xFF;
			header[1] = length >> 8;
			for(i=0;i<3;i++) {
				WRITEBYTE(header[i]);
				checksum ^= header[i];
			}
			WRITEBYTE(checksum);

			appendtone(&output,&outputlength,1000,rate,0,1,&offset);
			appendtone(&output,&outputlength,770,rate,4.0,0,&offset);
			appendtone(&output,&outputlength,2500,rate,0,0.5,&offset);
			appendtone(&output,&outputlength,2000,rate,0,0.5,&offset);

			// write out basic program
			checksum = 0xff;
			for(i=0;i<sizeof(basic)/sizeof(char);i++) {
				WRITEBYTE(basic[i]);
				checksum ^= basic[i];
			}
		}
		else { // write out JMP 80C NOP NOP ...
			unsigned char patch[] = {0x4C,0x0C,0x08,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA};
			for(i=0;i<sizeof(patch)/sizeof(char);i++) {
				WRITEBYTE(patch[i]);
				checksum ^= patch[i];
			}
		}

		// write out move and load code
		if(compress) {
			unsigned int cmp_start = 0xBA00 - segments[0].length;

			//load start
			table[0] = cmp_start & 0xff;
			table[1] = cmp_start >> 8;

			//load end
			table[2] = (cmp_start + segments[0].length + sizeof(inflatecode)/sizeof(char) + 1) & 0xff;
			table[3] = (cmp_start + segments[0].length + sizeof(inflatecode)/sizeof(char) + 1) >> 8;

			//inflate src
			table[4] = cmp_start & 0xff;
			table[5] = cmp_start >> 8;

			//inflate end
			table[8] = (segments[0].start + segments[0].codelength) & 0xff;
			table[9] = (segments[0].start + segments[0].codelength) >> 8;
		}
		else {
			//load start
			table[0] = segments[0].start & 0xff;
			table[1] = segments[0].start >> 8;

			//load end
			table[2] = (segments[0].start + segments[0].length + 1) & 0xff;
			table[3] = (segments[0].start + segments[0].length + 1) >> 8;
		}
		//JMP to code, inflate dst
		table[6] = segments[0].start & 0xff;
		table[7] = segments[0].start >> 8;
		table[10] = compress;
		table[11] = warm;

		if(fast)
			for(i=0;i<sizeof(fastload9600)/sizeof(char);i++) {
				WRITEBYTE(fastload9600[i]);
				checksum ^= fastload9600[i];
			}
		else
			if(k8)
				for(i=0;i<sizeof(fastload8000)/sizeof(char);i++) {
					WRITEBYTE(fastload8000[i]);
					checksum ^= fastload8000[i];
				}
			else
				if(cd)
					for(i=0;i<sizeof(fastloadcd)/sizeof(char);i++) {
						WRITEBYTE(fastloadcd[i]);
						checksum ^= fastloadcd[i];
					}
				else
					for(i=0;i<sizeof(autoloadcode)/sizeof(char);i++) {
						WRITEBYTE(autoloadcode[i]);
						checksum ^= autoloadcode[i];
					}

		// append table
		for(i=0;i<sizeof(table)/sizeof(char);i++) {
			WRITEBYTE(table[i]);
			checksum ^= table[i];
		}

		// append LOADING...
		loading[0] = 0x0D;
		for(i=0;i<strlen(loading);i++) {
			byte = toupper(loading[i]) + 0x80;
			if(loading[i] == '_')
				byte = toupper(' ') + 0x80;
			WRITEBYTE(byte);
			checksum ^= byte;
		}

		// append to loader the name of the file
		for(i=0;i<strlen(segments[0].filename);i++) {
			byte = toupper(segments[0].filename[i]) + 0x80;
			if(segments[0].filename[i] == '_')
				byte = toupper(' ') + 0x80;
			WRITEBYTE(byte);
			checksum ^= byte;
		}

		// append to loader the ETA
		for(i=0;i<strlen(eta);i++) {
			byte = toupper(eta[i]) + 0x80;
			WRITEBYTE(byte);
			checksum ^= byte;
		}

		// append to NULL to LOADING string
		WRITEBYTE(0x00);
		checksum ^= 0x00;

		// it's a wrap!
		WRITEBYTE(0xff);
		checksum ^= 0xff;

		if(!basicload) {
			int pad = (0xFF - (length & 0xFF));

			if(!(fast|cd|k8))
				pad += 0x100;

			length += pad;
			while(pad--)
				WRITEBYTE(0x00);
		}

		WRITEBYTE(checksum);

		appendtone(&output,&outputlength,1000,rate,0,1,&offset);
		if(fast || cd || k8)
			appendtone(&output,&outputlength,freq_pre,rate,0.25,0,&offset);
		else {
			appendtone(&output,&outputlength,770,rate,4.0,0,&offset);
			appendtone(&output,&outputlength,2500,rate,0,0.5,&offset);
			appendtone(&output,&outputlength,2000,rate,0,0.5,&offset);
		}

		// now the code
		if(fast) {
			freq0 = 12000;
			freq1 = 8000;
		}
		if(cd) {
			freq0 = 11025;
			freq1 = 7350;
		}
		if(k8) {
			freq0 = 12000;
			freq1 = 6000;
		}

		if(qr) {
			char loading[]="LOADING ";
			outputlength = 0;

			// 0.25 sec
			appendtone(&output,&outputlength,freq_pre,rate,0.25,0,&offset);

			checksum = 0xff;

			// parameters, 12 bytes
			for(i=0;i<sizeof(table)/sizeof(char);i++) {
				WRITEBYTE(table[i]);
				checksum ^= table[i];
			}

			// LOADING
			for(i=0;i<strlen(loading);i++) {
				byte = loading[i] + 0x80;
				WRITEBYTE(byte);
				checksum ^= byte;
			}

			// append to loader the name of the file
			for(i=0;i<strlen(segments[0].filename);i++) {
				byte = toupper(segments[0].filename[i]) + 0x80;
				if(segments[0].filename[i] == '_')
					byte = toupper(' ') + 0x80;
				WRITEBYTE(byte);
				checksum ^= byte;
			}

			// append to loader the ETA
			for(i=0;i<strlen(eta);i++) {
				byte = toupper(eta[i]) + 0x80;
				WRITEBYTE(byte);
				checksum ^= byte;
			}

			for(i=0;i<60-strlen(segments[0].filename)-strlen(eta)-strlen(loading);i++) {
				WRITEBYTE(0x00);
				checksum ^= 0x00;
			}

			WRITEBYTE(checksum);

			// end of parameters
			appendtone(&output,&outputlength,freq_end,rate,0,2,&offset);

			// time to processes
			appendtone(&output,&outputlength,freq_pre,rate,0.25,0,&offset);
		}

		checksum = 0xff;
		for(j=0;j<segments[0].length;j++) {
			WRITEBYTE(segments[0].data[j]);
			checksum ^= segments[0].data[j];
		}

		if(compress) {
			for(j=0;j<sizeof(inflatecode)/sizeof(char);j++) {
				WRITEBYTE(inflatecode[j]);
				checksum ^= inflatecode[j];
			}
		}

		if(fast + cd + k8 == 0) {	// hack so that standard method matches others
			WRITEBYTE(0x00);
			WRITEBYTE(0x00);
		}

		WRITEBYTE(checksum);

		if(fast || cd || k8)
			//appendtone(&output,&outputlength,freq_end,rate,0,1,&offset);
			appendtone(&output,&outputlength,freq_end,rate,0,10,&offset);
		else
			//appendtone(&output,&outputlength,1000,rate,0,1,&offset);
			appendtone(&output,&outputlength,1000,rate,0,10,&offset);

		if(!qr) {
			if(basicload) {
				fprintf(stderr,"To load up and run on your Apple %s, type:\n\n\tLOAD\n",modeltypes[model]);
				if(warm)
					fprintf(stderr,"\t%XG\n",segments[0].start);
			}
			else {
				fprintf(stderr,"To load up and run on your Apple %s, type:\n\n\t800.%XR 800G\n",modeltypes[model],0x800 + length + 1);
			}
		}
		else {
			fprintf(stderr,"To load up and run on your Apple %s, use the client disk.\n",modeltypes[model]);
		}
		fprintf(stderr,"\n");
	}

	if(dsk) {
		char eta[40];
		unsigned char byte, checksum=0xff, *cmp_data, start_table[21], *diskloadcode;
		unsigned long ones=0, zeros=0, diskloadcode_len;
		size_t cmp_len;
		unsigned int length, start_table_len = 0;
		int i, j;
		double inflate_times[5];

		if(k8) {
			diskloadcode = diskload8000;
			diskloadcode_len = sizeof(diskload8000)/sizeof(char);
		}
		else {
			diskloadcode = diskload9600;
			diskloadcode_len = sizeof(diskload9600)/sizeof(char);
		}

		rate = 48000;
		appendtone(&output,&outputlength,770,rate,4.0+tape,0,&offset);
		appendtone(&output,&outputlength,2500,rate,0,0.5,&offset);
		appendtone(&output,&outputlength,2000,rate,0,0.5,&offset);

		for(j=0;j<sizeof(diskloadcode2)/sizeof(char);j++) {
			byte=diskloadcode2[j];
			for(i=0;i<8;i++) {
				if(byte & 0x80)
					ones++;
				else
					zeros++;
				byte <<= 1;
			}
		}

		// compute pad length, assuming 4 pages max for code
		zeros += 8*(4 * 256 - sizeof(diskloadcode2)/sizeof(char));

		for(j=0;j<sizeof(diskloadcode3)/sizeof(char);j++) {
			byte=diskloadcode3[j];
			for(i=0;i<8;i++) {
				if(byte & 0x80)
					ones++;
				else
					zeros++;
				byte <<= 1;
			}
		}

		for(j=0;j<sizeof(dosboot1)/sizeof(char);j++) {
			byte=dosboot1[j];
			for(i=0;i<8;i++) {
				if(byte & 0x80)
					ones++;
				else
					zeros++;
				byte <<= 1;
			}
		}

		for(j=0;j<sizeof(dosboot2)/sizeof(char);j++) {
			byte=dosboot2[j];
			for(i=0;i<8;i++) {
				if(byte & 0x80)
					ones++;
				else
					zeros++;
				byte <<= 1;
			}
		}

		freq0 = 12000;
		freq1 = 8000;
		if(k8)
			freq1 = 6000;
		sprintf(eta,"%d SEC. ",(int) (ones/(float)freq1 + zeros/(float)freq0 + 0.5 + 0.25));

		//length = sizeof(basic)/sizeof(char) + sizeof(diskloadcode)/sizeof(char);
		length = sizeof(basic)/sizeof(char) + diskloadcode_len;
		header[0] = length & 0xFF;
		header[1] = length >> 8;

		freq0 = 2000;
		freq1 = 1000;
		for(i=0;i<3;i++) {
			WRITEBYTE(header[i]);
			checksum ^= header[i];
		}
		WRITEBYTE(checksum);

		appendtone(&output,&outputlength,1000,rate,0,1,&offset);
		appendtone(&output,&outputlength,770,rate,4.0,0,&offset);
		appendtone(&output,&outputlength,2500,rate,0,0.5,&offset);
		appendtone(&output,&outputlength,2000,rate,0,0.5,&offset);

		// write out basic program
		checksum = 0xff;
		for(i=0;i<sizeof(basic)/sizeof(char);i++) {
			WRITEBYTE(basic[i]);
			checksum ^= basic[i];
		}

		// patch in ETA
		for(i=0;i<strlen(eta);i++)
			diskloadcode[0x84F - 0x80C + i] = eta[i] + 0x80;

		// write out move and load code
		//for(i=0;i<sizeof(diskloadcode)/sizeof(char);i++) {
		for(i=0;i<diskloadcode_len;i++) {
			WRITEBYTE(diskloadcode[i]);
			checksum ^= diskloadcode[i];
		}

		// end of basic and diskloadcode
		WRITEBYTE(0xff);
		checksum ^= 0xff;

		WRITEBYTE(checksum);

		appendtone(&output,&outputlength,1000,rate,0,1,&offset);
		square=0;
		freq0 = 12000;
		if(k8) {
			freq1 = 6000;
			appendtone(&output,&outputlength,2000,rate,0.25,0,&offset);
		}
		else {
			freq1 = 8000;
			appendtone(&output,&outputlength,6000,rate,0.25,0,&offset);
		}

		checksum = 0xff;
		for(i=0;i<sizeof(dosboot1)/sizeof(char);i++) {
			WRITEBYTE(dosboot1[i]);
			checksum ^= dosboot1[i];
		}

		// time to compress and compute start location and length
		// patch loadcode2 with start locations and ETA
		for(i=0;i<numseg;i++) {
			int k, err;
			double orig_len;
			unsigned char checksum=0xff;

			inflate_times[i] = 0;
			
			cmp_data = tdefl_compress_mem_to_heap(segments[i].data, segments[i].length, &cmp_len, TDEFL_MAX_PROBES_MASK);

			//compute inflate time
			//load up inflate code
			for(j=0;j<sizeof(diskloadcode3)/sizeof(char);j++)
				ram[0x9B00 + j] = diskloadcode3[j];
			//load up inflate data
			for(j=0;j<cmp_len;j++) {
				ram[0x8FFF - cmp_len + j] = cmp_data[j];
				checksum ^= cmp_data[j];
			}
			ram[0x8FFF] = checksum;

			//compute chksum time
			if(k8) {
				for(j=(0x859 - 0x80C);j<diskloadcode_len;j++)
					ram[0x9000 - (0x859 - 0x80C) + j] = diskloadcode[j];
				ram[0x00] = (0x8FFF - cmp_len) & 0xFF;
				ram[0x01] = (0x8FFF - cmp_len) >> 8;
				ram[0x02] = 0x00;
				ram[0x03] = 0x90;
				ram[0x04] = 0xFF;
				ram[0x9089] = 0x85; //STA
				ram[0x908A] = 0x04; //zero page $04
				ram[0x908B] = 0x00; //BRK

				reset6502();
				exec6502(0x9065);

				if(ram[0x04] != 0)
					fprintf(stderr,"WARNING: simulated checksum failed: %02X\n",ram[0x04]);

				inflate_times[i] += clockticks6502/1023000.0;
			}

			//zero page src
			ram[0x10] = (0x8FFF - cmp_len) & 0xFF;
			ram[0x11] = (0x8FFF - cmp_len) >> 8;
			//zero page dst
			ram[0x12] = 0x00;
			ram[0x13] = 0x10;
			//setup JSR
			ram[0x9000] = 0x20; // JSR $9B00
			ram[0x9001] = 0x00;
			ram[0x9002] = 0x9B;
			ram[0x9003] = 0x00; //BRK to stop simulation
			//run it
			reset6502();
			exec6502(0x9000);
			//compare (just to be safe)
			err=0;
			for(j=0;j<7 * 4096;j++)
				if(ram[0x1000 + j] != segments[i].data[j]) {
					err = 1;
					break;
				}
			if(err)
				fprintf(stderr,"WARNING: simulated inflate failed at %04X\n",j+0x1000);
			inflate_times[i] += clockticks6502/1023000.0;

			free(segments[i].data);
			segments[i].data = cmp_data;
			orig_len = segments[i].length;
			segments[i].length = cmp_len;
			segments[i].start = 0x8FFF - segments[i].length;

			// compress ?
			// need to see what is faster, defaulting to compress for now
			// if not compressed do not set start location, change asm code to check for 0,0
			// and not use inflate code

			// where to load data
			start_table[start_table_len++] = segments[i].start & 0xFF;
			start_table[start_table_len++] = segments[i].start >> 8;

			ones = zeros = 0;
			for(j=0;j<segments[i].length;j++) {
				byte=segments[i].data[j];
				for(k=0;k<8;k++) {
					if(byte & 0x80)
						ones++;
					else
						zeros++;
					byte <<= 1;
				}
			}
			sprintf(eta,"%d",(int) (ones/(float)freq1 + zeros/(float)freq0 + 0.5 + 0.25));

			// ETA
			start_table[start_table_len++] = eta[0] + 0x80;
			if(eta[1] != 0)
				start_table[start_table_len++] = eta[1] + 0x80;
			else
				start_table[start_table_len++] = 0;

			fprintf(stderr,"Segment: %d, start: 0x%04X, length: %5d, deflated: %.02f%%, data time:%s, inflate time:%.02f\n",i,segments[i].start,segments[i].length,100.0*(1-segments[i].length/orig_len),eta,inflate_times[i]);
		}
		fprintf(stderr,"\n");

		for(i=0;i<sizeof(diskloadcode2)/sizeof(char);i++) {
			WRITEBYTE(diskloadcode2[i]);
			checksum ^= diskloadcode2[i];
		}

		start_table[start_table_len++] = noformat;

		for(i=0;i<start_table_len;i++) {
			WRITEBYTE(start_table[i]);
			checksum ^= start_table[i];
		}

		for(i=0;i<4*256 - sizeof(diskloadcode2)/sizeof(char) - start_table_len;i++) {
			WRITEBYTE(0x00);
			checksum ^= 0x00;
		}

		for(i=0;i<sizeof(diskloadcode3)/sizeof(char);i++) {
			WRITEBYTE(diskloadcode3[i]);
			checksum ^= diskloadcode3[i];
		}

		for(i=0;i<sizeof(dosboot2)/sizeof(char);i++) {
			WRITEBYTE(dosboot2[i]);
			checksum ^= dosboot2[i];
		}

		WRITEBYTE(checksum);
		if(k8) {
			appendtone(&output,&outputlength,770,rate,0,2,&offset);
			appendtone(&output,&outputlength,2000,rate,0.3,0,&offset);
		}
		else {
			appendtone(&output,&outputlength,2000,rate,0,1,&offset);
			appendtone(&output,&outputlength,6000,rate,0.1,0,&offset);
		}

		for(i=0;i<numseg;i++) {
			//appendtone(&output,&outputlength,6000,rate,1,0,&offset);

//timing
			if(i==0) {
				if(!noformat)
					j=28;
				else
					j=0;
			}
			else {
				//j = 6 + ceil(inflate_times[i-1]);  // 6 = write track time, may need to make it 7
				// disk ][ verified (format and no-format)
				// Virtual ][ emulator verified (format and no-format, 8K only)
				// CFFA3000 3.1 failed, needs more time

				j = ceil(6.5 + inflate_times[i-1]);  // 6 = write track time, may need to make it 7
				// disk ][ verified (format and no-format)
				// Apple duodisk verified (format and no-format)
				// CFFA3000 3.1 verified with USB stick (no-format only)
				// CFFA3000 3.1 failed with IBM 4GB Microdrive (too slow)
				// Nishida Radio SDISK // (no-format only)
			}
			if(i==1) // seek time for track 0, just in case
				j+=2;

/* count down code
			for(;j>=0;j--) {
				checksum = 0xff;
				WRITEBYTE(j/10 + 48 + 0x80);
				checksum ^= (j/10 + 48 + 0x80);
				WRITEBYTE(j%10 + 48 + 0x80);
				checksum ^= (j%10 + 48 + 0x80);
				WRITEBYTE(0x00);
				checksum ^= 0x00;
				WRITEBYTE(checksum);
				appendtone(&output,&outputlength,2000,rate,0,1,&offset);
				appendtone(&output,&outputlength,6000,rate,1,0,&offset);
			}
*/

			if(k8)
				appendtone(&output,&outputlength,2000,rate,j,0,&offset);
			else
				appendtone(&output,&outputlength,6000,rate,j,0,&offset);

			checksum = 0xff;
			for(j=0;j<segments[i].length;j++) {
				WRITEBYTE(segments[i].data[j]);
				checksum ^= segments[i].data[j];
			}
			WRITEBYTE(checksum);
			if(k8)
				//appendtone(&output,&outputlength,770,rate,0,2,&offset);
				appendtone(&output,&outputlength,770,rate,0,10,&offset);
			else
				//appendtone(&output,&outputlength,2000,rate,0,1,&offset);
				appendtone(&output,&outputlength,2000,rate,0,10,&offset);
		}

		fprintf(stderr,"To load up and run on your Apple %s, type:\n\n\tLOAD\n\n",modeltypes[model]);
	}

	// append zero to zero out last wave
	appendtone(&output,&outputlength,0,rate,0,1,&offset);

	// 0.1 sec quiet to help some emulators
	appendtone(&output,&outputlength,0,rate,0.1,0,&offset);

	// 0.4 sec quiet to help some IIs
	// appendtone(&output,&outputlength,0,rate,0.4,0,&offset);

	// write it
	if(outputtype == AIFF)
		Write_AIFF(ofp,output,outputlength,rate,bits,amp);
	else if(outputtype == WAVE)
		Write_WAVE(ofp,output,outputlength,rate,bits,amp);

	fclose(ofp);
	return 0;
}

void appendtone(double **sound, long *length, int freq, int rate, double time, double cycles, int *offset)
{
	long i, n=time*rate;
	static long grow = 0;
	double *tmp = NULL;

	if(freq && cycles)
		n=cycles*rate/freq;

	if(n == 0)
		n=cycles;

/*
	if((tmp = (double *)realloc(*sound, (*length + n) * sizeof(double))) == NULL)
		abort();
	*sound = tmp;
*/

// new code for speed up Windows realloc
	if(*length + n > grow) {
		grow = *length + n + 10000000;
		if((tmp = (double *)realloc(*sound, (grow) * sizeof(double))) == NULL)
			abort();
		*sound = tmp;
	}

//tmp -> (*sound)
	if(square) {
		int j;

		if(freq)
			for (i = 0; i < n; i++) {
				for(j = 0;j < rate / freq / 2;j++)
					(*sound)[*length + i++] = 1;
				for(j = 0;j < rate / freq / 2;j++)
					(*sound)[*length + i++] = -1;
				i--;
			}
		else
			for (i = 0; i < n; i++)
				(*sound)[*length + i] = 0;
	}
	else
		for(i=0;i<n;i++)
			(*sound)[*length+i] = sin(2*M_PI*i*freq/rate + *offset*M_PI);

	if(cycles - (int)cycles == 0.5)
		*offset = (*offset == 0);

	*length += n;
}

char *getext(char *filename)
{
	char stack[256], *rval;
	int i, sp = 0;

	for(i=strlen(filename)-1;i>=0;i--) {
		if(filename[i] == '.')
			break;
		stack[sp++] = filename[i];
	}
	stack[sp] = '\0';

	if(sp == strlen(filename) || sp == 0)
		return(NULL);

	if((rval = (char *)malloc(sp * sizeof(char))) == NULL)
		; //do error code

	rval[sp] = '\0';
	for(i=0;i<sp+i;i++)
		rval[i] = stack[--sp];

	return(rval);
}

void usage()
{
	fprintf(stderr,"%s",usagetext);
}

// Code below from http://paulbourke.net/dataformats/audio/
/*
   Write an AIFF sound file
   Only do one channel, only support 16 bit.
   Supports sample frequencies of 11, 22, 44KHz (default).
   Little/big endian independent!
*/

// egan: changed code to support any Hz and 8 bit.

void Write_AIFF(FILE * fptr, double *samples, long nsamples, int nfreq, int bits, double amp)
{
	unsigned short v;
	int i;
	unsigned long totalsize;
	double themin, themax, scale, themid;
	unsigned char bit80[10];

	// Write the form chunk
	fprintf(fptr, "FORM");
	totalsize = 4 + 8 + 18 + 8 + (bits / 8) * nsamples + 8;
	fputc((totalsize & 0xff000000) >> 24, fptr);
	fputc((totalsize & 0x00ff0000) >> 16, fptr);
	fputc((totalsize & 0x0000ff00) >> 8, fptr);
	fputc((totalsize & 0x000000ff), fptr);
	fprintf(fptr, "AIFF");

	// Write the common chunk
	fprintf(fptr, "COMM");
	fputc(0, fptr);				// Size
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(18, fptr);
	fputc(0, fptr);				// Channels = 1
	fputc(1, fptr);
	fputc((nsamples & 0xff000000) >> 24, fptr);	// Samples
	fputc((nsamples & 0x00ff0000) >> 16, fptr);
	fputc((nsamples & 0x0000ff00) >> 8, fptr);
	fputc((nsamples & 0x000000ff), fptr);
	fputc(0, fptr);				// Size = 16
	fputc(bits, fptr);

	ConvertToIeeeExtended(nfreq, bit80);
	for (i = 0; i < 10; i++)
		fputc(bit80[i], fptr);

	// Write the sound data chunk
	fprintf(fptr, "SSND");
	fputc((((bits / 8) * nsamples + 8) & 0xff000000) >> 24, fptr);	// Size
	fputc((((bits / 8) * nsamples + 8) & 0x00ff0000) >> 16, fptr);
	fputc((((bits / 8) * nsamples + 8) & 0x0000ff00) >> 8, fptr);
	fputc((((bits / 8) * nsamples + 8) & 0x000000ff), fptr);
	fputc(0, fptr);				// Offset
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(0, fptr);				// Block
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(0, fptr);

	// Find the range
	themin = samples[0];
	themax = themin;
	for (i = 1; i < nsamples; i++) {
		if (samples[i] > themax)
			themax = samples[i];
		if (samples[i] < themin)
			themin = samples[i];
	}
	if (themin >= themax) {
		themin -= 1;
		themax += 1;
	}
	themid = (themin + themax) / 2;
	themin -= themid;
	themax -= themid;
	if (ABS(themin) > ABS(themax))
		themax = ABS(themin);
//  scale = amp * 32760 / (themax);
	scale = amp * ((bits == 16) ? 32760 : 124) / (themax);

	// Write the data
	for (i = 0; i < nsamples; i++) {
		if (bits == 16) {
			v = (unsigned short) (scale * (samples[i] - themid));
			fputc((v & 0xff00) >> 8, fptr);
			fputc((v & 0x00ff), fptr);
		} else {
			v = (unsigned char) (scale * (samples[i] - themid));
			fputc(v, fptr);
		}
	}
}

/*
   Write an WAVE sound file
   Only do one channel, only support 16 bit.
   Supports any (reasonable) sample frequency
   Little/big endian independent!
*/

// egan: changed code to support 8 bit.

void Write_WAVE(FILE * fptr, double *samples, long nsamples, int nfreq, int bits, double amp)
{
	unsigned short v;
	int i;
	unsigned long totalsize, bytespersec;
	double themin, themax, scale, themid;

	// Write the form chunk
	fprintf(fptr, "RIFF");
	totalsize = (bits / 8) * nsamples + 36;
	fputc((totalsize & 0x000000ff), fptr);	// File size
	fputc((totalsize & 0x0000ff00) >> 8, fptr);
	fputc((totalsize & 0x00ff0000) >> 16, fptr);
	fputc((totalsize & 0xff000000) >> 24, fptr);
	fprintf(fptr, "WAVE");
	fprintf(fptr, "fmt ");		// fmt_ chunk
	fputc(16, fptr);			// Chunk size
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(1, fptr);				// Format tag - uncompressed
	fputc(0, fptr);
	fputc(1, fptr);				// Channels
	fputc(0, fptr);
	fputc((nfreq & 0x000000ff), fptr);	// Sample frequency (Hz)
	fputc((nfreq & 0x0000ff00) >> 8, fptr);
	fputc((nfreq & 0x00ff0000) >> 16, fptr);
	fputc((nfreq & 0xff000000) >> 24, fptr);
	bytespersec = (bits / 8) * nfreq;
	fputc((bytespersec & 0x000000ff), fptr);	// Average bytes per second
	fputc((bytespersec & 0x0000ff00) >> 8, fptr);
	fputc((bytespersec & 0x00ff0000) >> 16, fptr);
	fputc((bytespersec & 0xff000000) >> 24, fptr);
	fputc((bits / 8), fptr);		// Block alignment
	fputc(0, fptr);
	fputc(bits, fptr);			// Bits per sample
	fputc(0, fptr);
	fprintf(fptr, "data");
	totalsize = (bits / 8) * nsamples;
	fputc((totalsize & 0x000000ff), fptr);	// Data size
	fputc((totalsize & 0x0000ff00) >> 8, fptr);
	fputc((totalsize & 0x00ff0000) >> 16, fptr);
	fputc((totalsize & 0xff000000) >> 24, fptr);

	// Find the range
	themin = samples[0];
	themax = themin;
	for (i = 1; i < nsamples; i++) {
		if (samples[i] > themax)
			themax = samples[i];
		if (samples[i] < themin)
			themin = samples[i];
	}
	if (themin >= themax) {
		themin -= 1;
		themax += 1;
	}
	themid = (themin + themax) / 2;
	themin -= themid;
	themax -= themid;
	if (ABS(themin) > ABS(themax))
		themax = ABS(themin);
//  scale = amp * 32760 / (themax);
	scale = amp * ((bits == 16) ? 32760 : 124) / (themax);

	// Write the data
	for (i = 0; i < nsamples; i++) {
		if (bits == 16) {
			v = (unsigned short) (scale * (samples[i] - themid));
			fputc((v & 0x00ff), fptr);
			fputc((v & 0xff00) >> 8, fptr);
		} else {
			v = (unsigned char) (scale * (samples[i] - themid));
			fputc(v + 0x80, fptr);
		}
	}
}


/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
#define HUGE_VAL HUGE
#endif							/*HUGE_VAL */

#define FloatToUnsigned(f) ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

void ConvertToIeeeExtended(double num, unsigned char *bytes)
{
	int sign;
	int expon;
	double fMant, fsMant;
	unsigned long hiMant, loMant;

	if (num < 0) {
		sign = 0x8000;
		num *= -1;
	} else {
		sign = 0;
	}

	if (num == 0) {
		expon = 0;
		hiMant = 0;
		loMant = 0;
	} else {
		fMant = frexp(num, &expon);
		if ((expon > 16384) || !(fMant < 1)) {	/* Infinity or NaN */
			expon = sign | 0x7FFF;
			hiMant = 0;
			loMant = 0;			/* infinity */
		} else {				/* Finite */
			expon += 16382;
			if (expon < 0) {	/* denormalized */
				fMant = ldexp(fMant, expon);
				expon = 0;
			}
			expon |= sign;
			fMant = ldexp(fMant, 32);
			fsMant = floor(fMant);
			hiMant = FloatToUnsigned(fsMant);
			fMant = ldexp(fMant - fsMant, 32);
			fsMant = floor(fMant);
			loMant = FloatToUnsigned(fsMant);
		}
	}

	bytes[0] = expon >> 8;
	bytes[1] = expon;
	bytes[2] = hiMant >> 24;
	bytes[3] = hiMant >> 16;
	bytes[4] = hiMant >> 8;
	bytes[5] = hiMant;
	bytes[6] = loMant >> 24;
	bytes[7] = loMant >> 16;
	bytes[8] = loMant >> 8;
	bytes[9] = loMant;
}

uint8_t read6502(uint16_t address)
{
	return ram[address];
}

void write6502(uint16_t address, uint8_t value)
{
	ram[address] = value;
}
