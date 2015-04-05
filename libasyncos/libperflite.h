#ifndef _LIBPERFLITE_H
#define _LIBPERFLITE_H

#include <stdint.h>

int perflite_init(void);
uint64_t perflite_cycles(void);
void perflite_bench(void);
// uint64_t perflite_rdtsc(void);
// uint64_t perflite_rdtsc_sync(void);

#if defined(__arm__)

static inline uint64_t
perflite_rdtsc(void)
{
	unsigned long t;
	asm volatile ( "mrc p15, 0, %0, c9, c13, 0" : "=r" (t));
	return t;
}

static inline uint64_t
perflite_rdtsc_sync(void)
{
	unsigned long t;
	asm volatile ( "dsb ish" ::: "memory" );
	t = perflite_rdtsc();
	asm volatile ( "dsb ish" ::: "memory" );
	return t;
}

#elif defined(__x86__) || defined(__x86_64__)

static inline uint64_t
perflite_rdtsc(void)
{
	uint32_t lo, hi;
	asm volatile ( "rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static inline uint64_t
perflite_rdtsc_sync(void)
{
	uint64_t t;
	asm volatile ( "mfence" ::: "memory" );
	t = perflite_rdtsc();
	asm volatile ( "mfence" ::: "memory" );
	return t;
}

#endif

#endif /* _LIBPERFLITE_H */

