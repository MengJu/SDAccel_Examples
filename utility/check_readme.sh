#!/bin/bash

# Check if all source files have the correct license

LICENSE=$1
TYPES="c cpp h cl"

echo "-----------------------"
echo "--  CHECKING READMEs --"
echo "-----------------------"

FAIL=0

check_file() {
	ignore=0

	for i in $IGNORE; do
		if [[ $1 =~ ^descirption.json ]]; then 
			ignore=1
		fi
	done

	if [[ $VERBOSE == "true" ]]; then
		echo -n "Checking $1 ... "
	fi
	if [[ $ignore == 1 ]]; then
		if [[ $VERBOSE == "true" ]]; then
			echo "SKIP"
		fi
	else
		pushd . > /dev/null 
		cd $(dirname $1)
		mv README.md README.md.check > /dev/null
		make README.md 2>/dev/null 1>&2 || make README.md
		diff README.md README.md.check 2>/dev/null 1>&2
		if [[ $? == 0 ]]; then
			if [[ $VERBOSE == "true" ]]; then
				echo "PASS"
			fi
		else
			if [[ $VERBOSE == "true" ]]; then
				echo "FAIL"
				diff README.md README.md.check
			else
				echo "$1"
			fi
			(( FAIL += 1 ))
		fi
		mv README.md.check README.md
		popd >/dev/null
	fi
}


VCS_FILES=$(git ls-files)

for f in $VCS_FILES; do
	if [[ $f == */description.json ]]; then
		check_file $f
	fi
done

if [[ $FAIL != 0 ]]; then
	echo "ERROR: Readme check failed"
	echo "ERROR: please fix the README.md in these files"
fi

exit $FAIL
