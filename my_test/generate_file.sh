#/bin/bash
#generate files starting from 1k to maxsize in exponential growth of power to 2
#usage: ./generate_file.sh <maxsize>
if [ $# -ne 1 ]
then
	echo "usage: ./generate_file.sh <maxsize>"
	exit
fi
for((i = 1; i < $1; i = i * 2))
do
	if [ "$i" -ge 1024 ]
	then
		let "size = $i / 1024"
		dd if=/dev/urandom of=file-$size-M bs=1024 count=$i
	else
		dd if=/dev/urandom of=file-$i-k bs=1024 count=$i
	fi
done
