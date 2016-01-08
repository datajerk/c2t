#!/bin/bash

# Path to wine binary for testing Windows binary
PATH=~/wine/bin:$PATH

TESTS=test.md
IMAGES=images
TIFFS=tiffs

_test()
{
	IMAGE=$1
	DONE=$2
	MACHINE=$3
	LOAD="$4"
	TIMEOT=$5
	FILETYPE=$(echo $IMAGE | tr '[:upper:]' '[:lower:]' | awk -F. '{print $NF}')

	DSK=0
	if [ "$FILETYPE" = "po" -o "$FILETYPE" = "do" -o "$FILETYPE" = "dsk" ]
	then
		DSK=1
	fi

	if [ "$DSK" = "1" ]
	then
		dd if=/dev/zero of=test.$FILETYPE bs=1k count=140 >/dev/null 2>&1
	fi

	if ! OUTPUT=$(osascript test.scrp test.${FILETYPE} $DONE $MACHINE $DSK "$LOAD" $TIMEOT)
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

	if [ "$DSK" = "0" ]
	then
		rm -f test.aif
		return 0
	fi

	S1=$(md5sum ${IMAGE} | awk '{print $1}')
	S2=$(md5sum test.$FILETYPE | awk '{print $1}')

	echo "$IMAGE $S1 $S2" >>test.log

	if [ "$S1" = "$S2" ]
	then
		rm -f test.$FILETYPE test.aif
		return 0
	fi
	return 1
}

>test.log

echo "Tests:"
echo
grep '^|' $TESTS
echo

IFS=$'\n'
for i in $(grep '^| [0-9]' $TESTS | perl -p -e 's/\| +/|/g' | perl -p -e 's/ +\|/|/g' | sed 's/\|/:/g')
do
	IFS=:
	set -- $i

	N=$2
	CMD="$3"
	INPUT=$4
	MACHINE=$5
	LOAD="$6"
	DONE=$7
	OFFSET=$8
	TIMEOT=$9

	#echo $N : $CMD : $INPUT : $MACHINE : $LOAD : $DONE : $OFFSET : $TIMEOT

	if grep "^$N:" passed >/dev/null 2>&1
	then
		echo "Test $N already passed"
		continue
	fi

	EC=0
	echo "Running test $N..."
	echo $* >>test.log
	echo "  $CMD $IMAGES/$INPUT test.aif"
	eval "$CMD" $IMAGES/$INPUT test.aif >>test.log 2>&1
	echo "  _test ${IMAGES}/${INPUT} ${TIFFS}/${DONE} $MACHINE $LOAD $TIMEOT"
	if _test ${IMAGES}/${INPUT} ${TIFFS}/${DONE} $MACHINE "$LOAD" $TIMEOT
	then
		echo "  passed"
		echo "$N:$(date)" >>passed
	else
		echo "  FAILED"
		EC=1
		break
	fi
done

unset IFS
exit $EC

