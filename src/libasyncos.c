#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#include <config.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sched.h>

#include "libasyncos.h"
#include "libasyncos-util.h"

#define HAVE_LOCAL

//#define NSLOTS (NCPUS-1) /* 1 per core? */
#define NSLOTS 3
#define CACHE_LINE_SZ 64 /* XXX: machine dependent */
#if defined(HAVE_LOCAL)
//# define LSLOTS (16-NSLOTS)
#define LSLOTS (6-NSLOTS)
#else
# define LSLOTS (0)
#endif


struct pthread{
	pthread_t thread;
	int status;
};
static struct pthread asyncos_thread[NCPUS-1];
//static pthread_t asyncos_thread[NCPUS-1];
/*global counter for available work thread*/
static int available_thread = NCPUS-1;

static pthread_barrier_t thread_bar;

/* cacheline size and aligned to prevent false sharing */
struct slot {
	int status;
	int sc;
	uint64_t ret;
	union {
		unsigned long arg0;
		unsigned long err;
	};
	unsigned long arg1;
	unsigned long arg2;
	unsigned long arg3;
	unsigned long arg4;
	unsigned long arg5;
#if defined(__arm__)
	char ___padding[24];
#endif
};

enum slot_status {
	SC_FREE,
	SC_ALLOCATED,
	SC_POSTED,
	SC_COMPLETE,
	SC_INIT,
	SC_ACQUIRED,
};

enum pthread_status{
	P_INIT,
	P_RUNNING,
	P_FREE,
};

static struct slot slots[NSLOTS+LSLOTS] __attribute__ ((aligned (CACHE_LINE_SZ)));

static int
asyncos_pin(pthread_t thread, int cpu)
{
	int ret;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	ret = pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
	if (ret)
		errno = ret;
	my_fprintf(stderr,"asyncos_pin: thread %ld pinning returns %d.\n",thread, ret);
	return ret;
}

/* wait until we're scheduled on the correct core */
static int
asyncos_pin_wait(unsigned long cpuid)
{
	int i;

	for (i = 0; i < 10000; i++) {
		if (sched_getcpu() == cpuid) {
			return 0;
		}
		pthread_yield();
	}

	return 1;
}

/* ------ server side ------ */
#if 0
static void *asyncos_mainloop(void *slot)
{
	struct slot *s = &slots[(long)slot];

	assert(!asyncos_pin_wait((unsigned long)slot + 1));

	pthread_barrier_wait(&thread_bar);

	s->status = SC_FREE; /* mark thread as running */

	for (;;) {
		int wait;

		while (s->status != SC_POSTED) {
			relax();
		}

		full_barrier();

		wait = s->ret;
		if (wait) {
			s->ret = syscall(s->sc, s->arg0, s->arg1, s->arg2, s->arg3, s->arg4, s->arg5);
			s->err = errno;
		} else {
			(void)syscall(s->sc, s->arg0, s->arg1, s->arg2, s->arg3, s->arg4, s->arg5);
		}

		write_barrier(); // ensure results observed before completion signal
		s->status = wait ? SC_COMPLETE : SC_FREE;
		write_barrier();
	}

	return NULL;
}
#endif

static inline void 
increase_thread_num(){
	int count;
	if(available_thread >= 0){
		do{
			count = available_thread;
		}while(!cas(count, count+1, &available_thread));
	} else{
		fprintf(stderr,"worker thread number negative before increasing!\n");
		exit(1);
	}
}
	
static inline void 
decrease_thread_num(){
	int count;
	if(available_thread > 0){
		do{
			count = available_thread;
		}while(!cas(count, count-1, &available_thread));
	} else{
		fprintf(stderr,"worker thread number negative after decreasing\n");
		exit(1);
	}
}
static void *asyncos_mainloop(void *cpuID){
	struct slot *s;
	if(asyncos_pin_wait((unsigned long)cpuID+1)){
		fprintf(stderr,"pinning failed\n");
		return NULL;
	}
	assert(!asyncos_pin_wait((unsigned long)cpuID+1));
	unsigned long cpu = (unsigned long)cpuID;
	pthread_barrier_wait(&thread_bar);
	my_fprintf(stderr,"asyncos_mainloop: cpuID %lu released..\n",(unsigned long)cpuID);
	for(;;){
		int slot;
		int wait;
		/*spin on the slots to find an available slot*/
		do{
			for(slot = 0; slot < NSLOTS; slot++){
				if(slots[slot].status == SC_POSTED){
					if(cas(SC_POSTED, SC_ACQUIRED, &slots[slot].status)){
						s = &slots[slot];
#ifdef HAVE_LOCAL
						decrease_thread_num();
#endif
						goto thread_run;
					}
				}
				read_barrier();
			}
		}while(1);
thread_run:	
		wait = s->ret;	
		if(wait){
			s->ret = syscall(s->sc, s->arg0, s->arg1, s->arg2, s->arg3, s->arg4, s->arg5);
			s->err = errno;
		} else{
			(void)syscall(s->sc, s->arg0, s->arg1, s->arg2, s->arg3, s->arg4, s->arg5);	
		}
		write_barrier();
		s->status = wait ? SC_COMPLETE : SC_FREE;
#ifdef HAVE_LOCAL
		increase_thread_num();
#endif
		asyncos_thread[cpu].status = P_FREE;
		write_barrier();
	}
	return NULL;
}

/* ------ user side ------ */

void
asyncos_start(void)
{
	int i;

	/* release the worker threads */
	pthread_barrier_wait(&thread_bar);
	my_fprintf(stderr,"asyncos_start: main thread released...\n");
	/* wait for them to get running */
	for (i = 0; i < NCPUS-1; i++) {
		do {
			relax();
		} while (asyncos_thread[i].status != P_INIT);
		asyncos_thread[i].status = P_FREE;
	}
}

void
asyncos_init(void)
{
	unsigned long i;
	int ret;

	/* sanity checks */
	COMPILE_ASSERT(sizeof(struct slot) == CACHE_LINE_SZ);
	COMPILE_ASSERT(((unsigned long)slots % CACHE_LINE_SZ) == 0);

	/*initialize the slot status to FREE*/
	for(i = 0; i < NSLOTS+LSLOTS; i++){
		slots[i].status = SC_FREE;
	}
	write_barrier();
	pthread_barrier_init(&thread_bar, NULL, NCPUS);
	/*creat the worker threads according to number of cores*/
	/* start a bunch of helper threads and pin them */
	for (i = 0; i < NCPUS-1; i++) {
		asyncos_thread[i].status = P_INIT;
		ret = pthread_create(&asyncos_thread[i].thread, NULL,
				asyncos_mainloop, (void *)i);
		if (ret) {
			perror("libasyncos: pthread_create() failed");
			exit(1);
		}
	
		ret = asyncos_pin(asyncos_thread[i].thread, i+1);
		if (ret) {
			perror("libasyncos: pthread_setaffinity() failed");
			exit(1);
		}
		my_fprintf(stderr, "asyncos_init: thread %ld pinned successfully\n",asyncos_thread[i].thread);
	}

	/* pin ourselves */
	ret = asyncos_pin(pthread_self(), 0);
	if (ret) {
		perror("libasyncos: pthread_setaffinity() failed");
		exit(1);
	}
	assert(!asyncos_pin_wait(0));
}

static inline void
__wait_complete(int slot)
{
	struct slot *s = &slots[slot];

	while (s->status != SC_COMPLETE) {
		relax();
	}

	read_barrier();
}

static inline int
__get_free_slot(int wait)
{
	int slot;

	do {
		for (slot = 0; slot < NSLOTS; slot++) {
			if (cas(SC_FREE, SC_ALLOCATED, &slots[slot].status)) {
				return slot;
			}
		}
		relax();
	} while (wait);
/*force the number of syscalls less than slot number*/
	if(!wait && slot == NSLOTS){
		fprintf(stderr, "forbidden, no slots available");
		exit(1);
	}
#if defined(HAVE_LOCAL)
	for (slot = NSLOTS; slot < NSLOTS+LSLOTS; slot++) {
		if (slots[slot].status == SC_FREE) {
			slots[slot].status = SC_ALLOCATED;
			return slot;
		}
	}
#endif

	fprintf(stderr, "*** libasyncos: deadlock - no slots available!\n");
	exit(1);
}

int
issue(int sc, ...)
{
	va_list args;
	int slot = __get_free_slot(0);
	struct slot *s = &slots[slot];

#if defined(HAVE_LOCAL)
	if (slot >= NSLOTS) {
		va_start(args, sc);
		s->ret = syscall(sc,
				va_arg(args, unsigned long),
				va_arg(args, unsigned long),
				va_arg(args, unsigned long),
				va_arg(args, unsigned long),
				va_arg(args, unsigned long),
				va_arg(args, unsigned long)
			);
		va_end(args);
		s->err = errno;
		s->status = SC_COMPLETE;
		return slot;
	}
#endif

	write_barrier();

	s->sc = sc;
	s->ret = 1; /* wait on the result */

	va_start(args, sc);
	s->arg0 = va_arg(args, unsigned long);
	s->arg1 = va_arg(args, unsigned long);
	s->arg2 = va_arg(args, unsigned long);
	s->arg3 = va_arg(args, unsigned long);
	s->arg4 = va_arg(args, unsigned long);
	s->arg5 = va_arg(args, unsigned long);
	va_end(args);

	write_barrier(); // ensure args observed before syscall posted
	s->status = SC_POSTED;
	write_barrier();

#ifdef MAKE_SYNC
	__wait_complete(slot);
#endif

	return slot;
}

void
issue_nowait(int sc, ...)
{
	va_list args;
	int slot = __get_free_slot(1);
	struct slot *s = &slots[slot];

	write_barrier();

	s->sc = sc;
	s->ret = 0; /* do not wait for the result */

	va_start(args, sc);
	s->arg0 = va_arg(args, unsigned long);
	s->arg1 = va_arg(args, unsigned long);
	s->arg2 = va_arg(args, unsigned long);
	s->arg3 = va_arg(args, unsigned long);
	s->arg4 = va_arg(args, unsigned long);
	s->arg5 = va_arg(args, unsigned long);
	va_end(args);

	write_barrier(); // ensure args observed before syscall posted
	s->status = SC_POSTED;
	write_barrier();
}

static inline uint64_t
__complete(int slot)
{
	uint64_t ret;
	struct slot *s = &slots[slot];

/*the following code will never be reached under the
new version of code, by Jie*/
#if defined(HAVE_LOCAL)
	if (slot >= NSLOTS) {
		s->status = SC_FREE;
		errno = s->err;
		return s->ret;
	}
#endif

/*the main thread will check if there are available worker
thread and allocated but not acquired slot*/
check_complete:
	if(slots[slot].status ==  SC_COMPLETE){
		ret = s->ret;
		errno = s->err;
		full_barrier(); // ensure we grab ret before marking slot free
		s->status = SC_FREE;
		write_barrier();

		return ret;
	} else{/*the current slot is not completed yet, just do something locally to improve CPU utilization*/
		int s;
		struct slot *local_s;
		for(s = 0; s < NSLOTS; s++){
			if(available_thread <= 0 && slots[s].status == SC_POSTED){
				if(cas(SC_POSTED, SC_ACQUIRED, &slots[s].status)){
					local_s= &slots[s];
					/*execute the syscall locally*/
					local_s->ret = syscall(local_s->sc, local_s->arg0, local_s->arg1, local_s->arg2, local_s->arg3, local_s->arg4, local_s->arg5);
					local_s->err = errno;
					local_s->status = SC_COMPLETE;
					write_barrier();
					break;
				}
			}
			read_barrier();
		}
		goto check_complete;
	}
}

uint32_t complete32(int slot)
{
	return (uint32_t)__complete(slot);
}

uint64_t complete64(int slot)
{
	return (uint64_t)__complete(slot);
}

void * completeP(int slot)
{
	return (void *)(unsigned long)__complete(slot);
}

