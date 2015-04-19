#Makefile for C/C++ program in this folder
all:
	gcc -Wall -pthread -o asyncos_overhead asyncos_overhead.c ../src/libasyncos.c -I../lib
	g++ -Wall -O3 -o perf_compute perf_compute.cpp

clean:
	rm -rf asyncos_overhead perf_compute
