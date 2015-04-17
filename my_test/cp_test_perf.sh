#/bin/bash
#usage: ./cp_test.sh <hot/cold> <block size>
if [ $# -ne 2 ]
then
	echo "usage ./cp_test.sh <hot/cold> <block size>"
	exit
fi
#overhead of cp_enable
echo "Overhead of cp enable:"
for(( c = 0; c < 5; c = c + 1))
do
	./cp_enable
done
echo ""
echo ""
filesize=( 1-k 2-k 4-k 8-k 16-k 32-k 64-k 128-k 256-k 512-k 1-M 2-M 4-M 8-M 16-M 32-M 64-M 128-M 256-M 512-M )
for((order = 0; order < 20; order = order + 1))
do
	filename=file-${filesize[$order]}
	echo "copying file: $filename"
	if [ $1 == hot ]
	then
		echo "do copy with libAsyncOS, with hot disk cache"
		for (( i = 0; i < 5; i = i + 1))
		do
			../src/cp $filename cp-$filename
		done		
		for (( i = 0; i < 5; i = i + 1))
		do
			rm cp-$filename
			./cp_enable ../src/cp $filename cp-$filename
		done
		
		echo "do the original copy, with hot disk cache"
		for (( i = 0; i < 5; i = i + 1))
		do
			../org-cp $filename cp-$filename
		done
		for (( i = 0; i < 5; i = i + 1))
		do
			rm cp-$filename
			./cp_enable ../org-cp $filename cp-$filename
		done
		rm cp-$filename
		echo ""
		echo ""

: <<'END'
	elif [ $1 == cold ]
	then
		echo "do copy with libAsyncOS, with cold disk cache"
		for(( i = 0; i < 5; i = i + 1))
		do
			if [ -e cp-$filename ]
			then
				rm cp-$filename
			fi
			sync
			sync
			sleep 1
			sync
			echo 3 > /proc/sys/vm/drop_caches
			sleep 1
			sync
			./cp_enable ../src/cp $filename cp-$filename
		done	
		rm cp-$filename
		echo "do the original copy, with cold disk cache"
		for(( i = 0; i < 5; i = i + 1))
		do
			if [ -e cp-$filename ]
			then
				rm cp-$filename
			fi
			sync
			sync
			sleep 1
			sync
			echo 3 > /proc/sys/vm/drop_caches
			sleep 1
			sync
			./cp_enable ../org-cp $filename cp-$filename
		done	
		rm cp-$filename
	else
		echo "usage ./cp_test.sh <hot/cold> <block size> "
		exit
END
	fi
done
