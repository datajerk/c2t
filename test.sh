#!/bin/bash

dsk_test()
{
	BASENAME=$(basename $1)
	FILETYPE=$(echo $BASENAME | awk -F. '{print $NF}')
	FILENAME=$(echo $BASENAME | sed "s/.$FILETYPE$//")

	dd if=/dev/zero of=test.$FILETYPE bs=1k count=140 >/dev/null 2>&1

	#./c2t-96h ${BASENAME} test.aif

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

PATH=~/wine/bin:$PATH

CMD=("./c2t-96h" "wine c2t-96h.exe")

for i in zork.dsk dangerous_dave.po
do
	for j in $(seq 0 $(( ${#CMD[@]} - 1 )) )
	do 
		echo ${CMD[$j]} $i test.aif
		eval ${CMD[$j]} $i test.aif
		if dsk_test $i
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

