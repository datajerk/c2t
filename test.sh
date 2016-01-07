#!/bin/bash

# Path to wine binary for testing Windows binary
PATH=~/wine/bin:$PATH

# list of commands to test, must be space delimited
CMD=("./c2t-96h" "wine c2t-96h.exe")

# list of disks to test, must be space delimited.
# Disks (and paths to disks) cannot have spaces in names.
IMAGES="disks/zork.dsk disks/dangerous_dave.po"

_test()
{
	IMAGE=$1
	DONE=$2
	BASENAME=$(basename $1)
	FILETYPE=$(echo $BASENAME | awk -F. '{print $NF}')

	dd if=/dev/zero of=test.$FILETYPE bs=1k count=140 >/dev/null 2>&1

	if ! OUTPUT=$(osascript test.scrp test.${FILETYPE} $DONE)
	then
		echo
		return 1
	fi

	if echo $OUTPUT | grep ERROR >/dev/null 2>&1
	then
		echo $OUTPUT
		echo
		return 1
	fi

	S1=$(md5sum ${IMAGE} | awk '{print $1}')
	S2=$(md5sum test.$FILETYPE | awk '{print $1}')

	echo "$IMAGE $S1 $S2"

	if [ "$S1" = "$S2" ]
	then
		rm -f test.$FILETYPE test.aif
		return 0
	fi
	return 1
}

for i in $IMAGES
do
	for j in $(seq 0 $(( ${#CMD[@]} - 1 )) )
	do 
		echo ${CMD[$j]} $i test.aif
		eval ${CMD[$j]} $i test.aif
		if _test $i dskappleiie.tiff
		then
			echo "$i passed"
			echo
		else
			echo "$i failed"
			exit 1
		fi
	done
done

exit 0

