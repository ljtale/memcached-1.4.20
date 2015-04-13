#ifndef _ASYNCOS_H
#define _ASYNCOS_H

#include <stdint.h>
#include <sys/syscall.h>

#ifndef NCPUS
#define NCPUS 4
#endif

//#define MY_PRINT
#ifdef MY_PRINT
#define my_fprintf(fmt, args...)  fprintf(fmt, ##args)
#else
#define my_fprintf(fmt, args...)
#endif

/* For debugging: cause issue() to complete synchronously. complete() becomes no-op */
/* #define MAKE_SYNC */

void asyncos_init(void);
void asyncos_start(void);

/*
 * issue syscall `callno' asyncronously, returning a slot handle that can later be
 * used to complete the syscall.
 */
int issue(int callno, ...);

/*
 * issue syscall `callno' in the background - no return value.
 */
void issue_nowait(int callno, ...);

/*
 * wait until syscall `slot' completes, and return the error code
 */
uint32_t complete32(int slot);
uint64_t complete64(int slot);
void *   completeP (int slot);

static inline int complete(int slot) { return complete32(slot); }

#endif

