#/bin/bash
#usage: ./cp_test.sh <hot/cold> <block size>
if [ $# -ne 2 ]
then
	echo "usage ./cp_test.sh <hot/cold> <block size>"
	exit
fi
for filename in file-*
do
	echo "copying file: $filename"
	if [ $1 == hot ]
	then
		echo "do copy with libAsyncOS, with hot disk cache"
		for (( i = 0; i < 5; i = i + 1))
		do
			../src/cp $filename $filename-cp
		done		
		for (( i = 0; i < 5; i = i + 1))
		do
			rm $filename-cp
			./cp_enable ../src/cp $filename $filename-cp
		done
		
		echo "do the original copy, with hot disk cache"
		for (( i = 0; i < 5; i = i + 1))
		do
			../org-cp $filename $filename-cp
		done
		for (( i = 0; i < 5; i = i + 1))
		do
			rm $filename-cp
			./cp_enable ../org-cp $filename $filename-cp
		done
		echo ""
		echo ""

	elif [ $1 == cold ]
	then
		echo "do copy with libAsyncOS, with cold disk cache"
		for(( i = 0; i < 5; i = i + 1))
		do
			if [ -e $filename-cp ]
			then
				rm $filename-cp
			fi
			sync
			sync
			sleep 1
			sync
			echo 3 > /proc/sys/vm/drop_caches
			sleep 1
			sync
			./cp_enable ../src/cp $filename $filename-cp
		done	

		echo "do the original copy, with cold disk cache"
		for(( i = 0; i < 5; i = i + 1))
		do
			if [ -e $filename-cp ]
			then
				rm $filename-cp
			fi
			sync
			sync
			sleep 1
			sync
			echo 3 > /proc/sys/vm/drop_caches
			sleep 1
			sync
			./cp_enable ../org-cp $filename $filename-cp
		done	
	else
		echo "usage ./cp_test.sh <hot/cold> <block size> "
		exit
	fi
done
