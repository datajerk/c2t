/*

c2vip, Code to VIP Tape|Text, Version 0.2, Wed Jun 25 06:02:49 GMT 2014

Parts copyright (c) 2014 All Rights Reserved, Egan Ford (egan@sense.net)

THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Built on work by:
	* Paul Bourke (http://paulbourke.net/dataformats/audio/, AIFF and WAVE output code)
	* Malcolm Slaney and Ken Turkowski (Integer to IEEE 80-bit float code)

License:
	*  Do what you like, remember to credit all sources when using.

Description:
	This small utility will read COSMAC VIP binaries and output COSMAC VIP AIFF
	and WAV audio files for use with a cassette interface.

Features:
	*  Big and little-endian machine support.
		o  Little-endian tested.
	*  AIFF and WAVE output (both tested).
	*  Platforms tested:
		o  32-bit/64-bit x86 OS/X.
		o  32-bit/64-bit x86 Linux.
		o  32-bit x86 Windows/Cygwin.
		o  32-bit x86 Windows/MinGW.

Compile:
	OS/X:
		gcc -Wall -O -o c2vip c2vip.c
	Linux:
		gcc -Wall -O -o c2vip c2vip.c -lm
	Windows/Cygwin:
		gcc -Wall -O -o c2vip c2vip.c
	Windows/MinGW:
		PATH=C:\MinGW\bin;%PATH%
		gcc -Wall -O -static -o c2vip c2vip.c

Notes:
	*  Dropbox only supports .wav and .aiff (do not use .wave or .aif)

Not yet done:
	*  Test big-endian.
	*  gnuindent
    *  Redo malloc code in appendtone

Thinking about:
	*  Check for existing file and abort, or warn, or prompt.
	*  -q quiet option for Makefiles

Bugs:
	*  Probably

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "c2vip.h"

#define ABS(x) (((x) < 0) ? -(x) : (x))

#define VERSION "Version 0.2"
#define OUTFILE argv[argc-1]
#define BINARY 0
#define MONITOR 1
#define AIFF 2
#define WAVE 3
#define DSK 4 

#define WRITERBYTE(x) { \
	unsigned char wb_j, wb_temp=(x); \
	for(wb_j=0;wb_j<8;wb_j++) { \
		if(wb_temp & 1) \
			appendtone(&output,&outputlength,freq1,rate,0,1,&offset); \
		else \
			appendtone(&output,&outputlength,freq0,rate,0,1,&offset); \
		wb_temp>>=1; \
	} \
}

void usage();
char *getext(char *filename);
void appendtone(double **sound, long *length, int freq, int rate, double time, double cycles, int *offset);
void Write_AIFF(FILE * fptr, double *samples, long nsamples, int nfreq, int bits, double amp);
void Write_WAVE(FILE * fptr, double *samples, long nsamples, int nfreq, int bits, double amp);
void ConvertToIeeeExtended(double num, unsigned char *bytes);

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
	int i, j, c, outputtype, offset=0, fileoutput=1;
	int longmon=0, rate=48000, bits=8, freq0=2000, freq1=800;
	char *filetypes[] = {"binary","monitor","aiff","wave","disk"};
	char *ext;
	unsigned char pop, parity;
	unsigned int numseg = 0;
	segment *segments = NULL;

	opterr = 1;
	while((c = getopt(argc, argv, "vph?r:")) != -1)
		switch(c) {
			case 'v':		// version
				fprintf(stderr,"\n%s\n\n",VERSION);
				return 1;
				break;
			case 'p':		// stdout
				fileoutput = 0;
				break;
			case 'h':		// help
			case '?':
				usage();
				return 1;
			case 'r':		// override rate for -1/-2 only
				rate = atoi(optarg);
				break;
		}

	if(argc - optind < 1 + fileoutput) {
		usage();
		return 1;
	}

	// read intput files

	fprintf(stderr,"\n");
	for(i=optind;i<argc-fileoutput;i++) {
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
			if(argv[i][j] == ',')
				break;
			segments[numseg].filename[k++]=argv[i][j];
		}
		segments[numseg].filename[k] = '\0';
		// TODO: store as basename, check for MINGW compat

/*
		if((ext = getext(segments[numseg].filename)) != NULL)
			if(strcmp(ext,"mon") == 0)
				inputtype = MONITOR;
*/

		if ((ifp = fopen(segments[numseg].filename, "rb")) == NULL) {
			fprintf(stderr,"Cannot read: %s\n\n",segments[numseg].filename);
			return 1;
		}

		fprintf(stderr,"Reading %s, type %s, segment %d, start: ",segments[numseg].filename,filetypes[inputtype],numseg+1);

		if((data = malloc(64*1024*sizeof(char))) == NULL) {
			fprintf(stderr,"could not allocate 64K data\n");
			abort();
		}

		if(inputtype == BINARY) {
			segments[numseg].start = 0;
			segments[numseg].length = 0;
			while(fread(&b, 1, 1, ifp) == 1)
				data[segments[numseg].length++]=b;

			segments[numseg].data = data;
			fprintf(stderr,"0x%04X, length: %d\n",segments[numseg].start,segments[numseg].length);
		}

		fclose(ifp);
		numseg++;
	}
	fprintf(stderr,"\n");

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
		outputtype = MONITOR;
	}

	ofp=stdout;
	if(fileoutput) {
		if ((ofp = fopen(OUTFILE, "w")) == NULL) {
			fprintf(stderr,"\nCannot write: %s\n\n",OUTFILE);
			return 1;
		}
		fprintf(stderr,"Writing %s as %s formatted %s.\n\n",OUTFILE,"COSMAC VIP",filetypes[outputtype]);
	}
	else
		fprintf(stderr,"Writing %s as %s formatted %s.\n\n","STDOUT","COSMAC VIP",filetypes[outputtype]);

	if(outputtype == MONITOR) {
		int i, j, saddr;

		for(i=0;i<numseg;i++) {
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

	for(i=0;i<numseg;i++) {
		appendtone(&output,&outputlength,2000,rate,4.0,0,&offset);

		for(j=0;j<segments[i].length;j++) {
			// start bit
			appendtone(&output,&outputlength,freq1,rate,0,1,&offset);
			// data bits
			WRITERBYTE(segments[i].data[j]);
			// even parity
			pop = segments[i].data[j];
			parity = 0;
			for(;pop;parity=(parity==0))
				pop &= pop - 1;
			if(parity)
				appendtone(&output,&outputlength,freq1,rate,0,1,&offset);
			else
				appendtone(&output,&outputlength,freq0,rate,0,1,&offset);
		}
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

