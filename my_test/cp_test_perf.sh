#/bin/bash
#usage: ./cp_test.sh <hot/cold> <block size>
if [ $# -ne 2 ]
then
	echo "usage ./cp_test.sh <hot/cold> <block size>"
	exit
fi
#no enable overhead any more, we measure the time directly inside cp

#overhead of worker threads creation
echo "Overhead of worker threads, we can use this data as a possible reference:"
echo "asyncos_overhead start..."
for(( c = 0; c < 10; c = c + 1))
do
	./asyncos_overhead
done
echo "asyncos_overhead end..."
echo ""
echo ""

test_times=10
sleep_time=4

filesize=( 1-k 2-k ) #4-k 8-k 16-k 32-k 64-k 128-k 256-k 512-k 1-M 2-M 4-M 8-M 16-M 32-M 64-M 128-M 256-M 512-M )
for((order = 0; order < ${#filesize[@]}; order = order + 1))
do
	filename=file-${filesize[$order]}
	echo "file_size_$order: $filename"
	if [ $1 == hot ]
	then
		echo "async_cp_hot: "
		echo "repeating async copying 10 times starts..."
		for (( i = 0; i < 10; i = i + 1))
		do
			../src/cp $filename cp-$filename
			rm cp-$filename
		done
		echo "repeating async copying 10 times ends..."		

		echo "async_cp_hot actually starts..."
		for (( i = 0; i < $test_times; i = i + 1))
		do
			# we always test copy before the target does not exist
			rm cp-$filename
:<<'COM1'
			../perf stat \
			-e cache-misses \
			-e branch-misses \
			-e L1-dcache-load-misses \
			-e L1-dcache-store-misses \
			-e L1-icache-load-misses \
COM1
			../perf stat -d ../src/cp $filename cp-$filename
		done
		echo "async_cp_hot ends..."
		rm cp-$filename
		echo ""


		echo "orig_cp_hot: "
		echo "repeating orig copying 10 times starts..."
		for (( i = 0; i < 10; i = i + 1))
		do
			../org-cp $filename cp-$filename
			rm cp-$filename
		done
		echo "repeating orig copying 10 times ends..."		

		echo "orig_cp_hot actually starts..."
		for (( i = 0; i < $test_times; i = i + 1))
		do
			rm cp-$filename
:<<'COM2'
			../perf stat \
			-e cache-misses \
			-e branch-misses \
			-e L1-dcache-load-misses \
			-e L1-dcache-store-misses \
			-e L1-icache-load-misses \
COM2
			../perf stat -d ../org-cp $filename cp-$filename
		done
		rm cp-$filename
		echo ""
	elif [ $1 == cold ]
	then
		echo "async_cp_cold:"
		for(( i = 0; i < $test_times; i = i + 1))
		do
			echo "sync and drop the cache, wait for a few secs..."
			sync
			sync
			echo 3 > /proc/sys/vm/drop_caches
			sync
			sync
			sleep $sleep_time
			sync
			sync
			echo "async_cp_colde $i actually starts..."
			../perf stat \
			-e cache-misses \
			-e branch-misses \
			-e L1-dcache-load-misses \
			-e L1-dcache-store-misses \
			-e L1-icache-load-misses \
			../src/cp $filename cp-$filename
			rm cp-$filename
			echo "async_cp_cold $i ends..."
			echo 3 > /proc/sys/vm/drop_caches
			sync
		done	
		echo ""


		echo "orig_cp_cold"
		for(( i = 0; i < $test_times; i = i + 1))
		do
			echo "sync and drop the cache, wait for a few secs..."
			sync
			sync
			echo 3 > /proc/sys/vm/drop_caches
			sync
			sync
			sleep $sleep_time
			sync
			sync
			echo "orig_cp_colde $i actually starts..."
			../perf stat \
			-e cache-misses \
			-e branch-misses \
			-e L1-dcache-load-misses \
			-e L1-dcache-store-misses \
			-e L1-icache-load-misses \
			../org-cp $filename cp-$filename
			rm cp-$filename
			echo "orig_cp_cold $i ends..."
			echo 3 > /proc/sys/vm/drop_caches
			sync
		done	
		echo ""
	else
		echo "usage ./cp_test.sh <hot/cold> <block size> "
		exit
	fi
done
