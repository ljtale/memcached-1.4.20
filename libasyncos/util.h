#ifndef _UTIL_H
#define _UTIL_H

/* some stuff here borrowed from linux kernel */

#include <stdint.h>
#include <stdbool.h>

#define COMPILE_ASSERT(condition) ((void)sizeof(char[1 - 2*!(condition)]))

static inline void barrier(void)
{
	__asm__ __volatile__ ( "" ::: "memory" );
}

static inline void read_barrier(void)
{
	barrier();
}

static inline void write_barrier(void)
{
	barrier();
}

static inline void full_barrier(void)
{
	barrier();
}

static inline void relax(void)
{
	__asm__ __volatile__ ( "pause" ::: "memory" );
}

static inline bool cas(int o, int n, int *p)
{
	return __sync_bool_compare_and_swap(p, o, n);
}

static inline uint64_t rdtsc()
{
	unsigned int lo,hi;
	__asm__ __volatile__ ( "lfence" ::: "memory" );
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	__asm__ __volatile__ ( "lfence" ::: "memory" );
	return ((uint64_t)hi << 32) | lo;
}

#endif

