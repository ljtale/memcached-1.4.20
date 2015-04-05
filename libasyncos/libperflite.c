#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/syscall.h>

#include <linux/perf_event.h>
#include "libperflite.h"

static int perf_fd = -1;
static bool perflite_initialized = false;

static struct perf_event_attr perf_attr = {
	.type = PERF_TYPE_HARDWARE,
	.size = sizeof(struct perf_event_attr),
	.config = PERF_COUNT_HW_CPU_CYCLES,
	.pinned = 1,
};

int
perflite_init(void)
{
	perflite_initialized = true;

	perf_fd = syscall(SYS_perf_event_open, &perf_attr,
			0,  /* pid */
			-1, /* cpu */
			-1, /* group_fd */
			0   /* flags */
		);

	if (perf_fd < 0) {
		return perf_fd;
	}
	return 0;
}

uint64_t
perflite_cycles(void)
{
	uint64_t data;
	int r;

	if (!perflite_initialized) {
		r = perflite_init();
		if (r < 0) {
			perror("failed opening perf()");
			return 0;
		}
	}

	if (perf_fd < 0) {
		return 0;
	}

	r = read(perf_fd, &data, sizeof(data));
	if (r != sizeof(data)) {
		perror("error reading cycles");
		return 0;
	}

	return data;
}

void
perflite_bench(void)
{
	uint64_t t1, t2, avg = 0;
	int n = 10000;
	int i;

	for (i = 0; i < n; i++) {
		t1 = perflite_cycles();
		t2 = perflite_cycles();
		avg += t2 - t1;
	}

	printf("average cost (cycles): %llu\n", (unsigned long long)(avg/n));
}

