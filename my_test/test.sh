#/bin/bash
if [ $# -ne 1 ]
then
	echo "usage: ./test.sh <blocksize>"
	exit
fi

echo "test hot disk cache with block size: $1"
./cp_test_perf.sh hot $1 noperf &> perf-result/noperf-hot-$1
./cp_test_perf.sh hot $1 perf &> perf-result/perf-hot-$1
echo ""
#echo "test cold disk cache with block size: $1"
#./cp_test_perf.sh cold $1 whatever &> perf-result/perf-cold-$1
