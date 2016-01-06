#!/bin/bash

dsk_test()
{
	BASENAME=$(basename $1)
	FILETYPE=$(echo $BASENAME | awk -F. '{print $NF}')
	FILENAME=$(echo $BASENAME | sed "s/.$FILETYPE$//")

	dd if=/dev/zero of=test.$FILETYPE bs=1k count=140 >/dev/null 2>&1

	./c2t-96h ${BASENAME} test.aif

	osascript test.scrp test.${FILETYPE}

	S1=$(md5sum ${BASENAME} | awk '{print $1}')
	S2=$(md5sum test.$FILETYPE | awk '{print $1}')

	echo "$BASENAME $S1 $S2"

	if [ "$S1" = "$S2" ]
	then
		rm -f test.$FILETYPE test.aif
		return 0
	fi
	return 1
}

for i in zork.dsk dangerous_dave.po
do
	if dsk_test $i
	then
		echo "$i passed"
	else
		echo "$i failed"
		exit 1
	fi
done

exit 0

