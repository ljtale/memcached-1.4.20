#/bin/bash
if [ $# -ne 1 ]
then
	echo "usage: ./test.sh <blocksize>"
fi

echo "test hot disk cache with block size: $1"
./cp_test.sh hot $1

echo ""
echo ""
echo ""
echo ""
echo "test cold disk cache with block size: $1"
./cp_test.sh cold $1
echo ""
echo ""
echo ""
echo ""

