#!/bin/bash

TIFFDIR=tiffs
C2T=../bin/c2t

_gentiff()
{
	MACHINE=$1
	TIFF=$2
	DSK=$3
	LOAD="$4"
	DELAY=$5

	if [ "$DSK" = "1" ]
	then
		dd if=/dev/zero of=test.dsk bs=1k count=140 >/dev/null 2>&1
	fi

	if ! OUTPUT=$(osascript gentiff.scrp test.dsk $MACHINE $TIFF $DSK "$LOAD" $DELAY)
	then
		echo
		return 1
	fi

	#if echo $OUTPUT | grep ERROR >/dev/null 2>&1
	#then
	#	echo $OUTPUT
	#	echo
	#	return 1
	#fi

	if test -s $TIFF
	then
		return 0
	fi
	return 1
}

mkdir -p ${TIFFDIR}

TIFF=${TIFFDIR}/dskiie.tiff
if [ ! -s "$TIFF" ]
then
	eval $C2T images/zork.dsk test.aif
	if ! _gentiff iie $TIFF 1 "LOAD" 15
	then
		echo "$TIFF failed"
		exit 1
	fi
fi

TIFF=${TIFFDIR}/dskiip.tiff
if [ ! -s "$TIFF" ]
then
	eval $C2T images/zork.dsk test.aif
	if ! _gentiff iip $TIFF 1 "LOAD" 15
	then
		echo "$TIFF failed"
		exit 1
	fi
fi

TIFF=${TIFFDIR}/mpiie.tiff
if [ ! -s "$TIFF" ]
then
	eval $C2T -2bf images/moon.patrol,801 test.aif
	if ! _gentiff iie $TIFF 0 "LOAD" 15
	then
		echo "$TIFF failed"
		exit 1
	fi
fi

TIFF=${TIFFDIR}/mpii.tiff
if [ ! -s "$TIFF" ]
then
	eval $C2T -2af images/moon.patrol,801 test.aif
	if ! _gentiff ii $TIFF 0 "800.A00R 800G" 15
	then
		echo "$TIFF failed"
		exit 1
	fi
fi

TIFF=${TIFFDIR}/spiie.tiff
if [ ! -s "$TIFF" ]
then
	eval $C2T -2bc8 images/super_puckman,800 test.aif
	if ! _gentiff iie $TIFF 0 "LOAD" 12
	then
		echo "$TIFF failed"
		exit 1
	fi
fi

rm -f test.aif test.dsk

