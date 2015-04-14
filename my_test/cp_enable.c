#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sched.h>

#define P "/sys/kernel/debug/tracing/tracelite/"
#define BZ 1024

void cat(char *file, int fdout)
{
	int fd, n;
	char buf[BZ];

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		perror("cat open failed");
		return;
	}

	while (((n = read(fd, buf, BZ)) > 0)) {
		if (write(fdout, buf, n) != n) {
			perror("cat write failed");
			return;
		}
	}

	if (n < 0) {
		perror("cat read failed");
	}

	close(fd);
}

void echo(char *file, unsigned long val)
{
	int fd, n;
	char buf[BZ];

	fd = open(file, O_WRONLY);
	if (fd < 0) {
		perror("echo open failed");
		return;
	}

	n = snprintf(buf, BZ, "%lu\n", val);

	if (write(fd, buf, n) != n) {
		perror("echo write failed");
	}

	close(fd);
}

int main(int argc, char **argv)
{
	pid_t pid;
	int st;
	int idx = 1;
	struct timeval t1, t2, tt;
	int trace_fd = STDERR_FILENO;
	int enable = 1;

	while (idx < argc) {
		if (!strcmp(argv[idx], "--nolog")) {
			trace_fd = -1;
		} else if (!strcmp(argv[idx], "--notrace")) {
			enable = 0;
		} else if (!strcmp(argv[idx], "--log")) {
			if ((trace_fd = open(argv[++idx], O_WRONLY)) < 0) {
				perror("failed to open log output");
				trace_fd = STDERR_FILENO;
			}
		} else if (!strcmp(argv[idx], "--async")) {
			enable = 2;
		} else {
			break;
		}
		idx++;
	}

	if (idx == argc) {
		fprintf(stderr, "%s [--nolog] [--log <file>] <prog> [opts..]\n", argv[0]);
		exit(1);
	}


//	echo(P"enable", 0);
//	echo(P"trace", 0); /* kill the trace */
//	echo(P"events_lost", 0); /* reset */

	gettimeofday(&t1, NULL);
	pid = fork();

	if (pid == 0) {

		/* pin forked thread to CPU 0 */
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(0, &cpuset);
		if (sched_setaffinity(getpid(), sizeof(cpuset), &cpuset)) {
			perror("failed setting affinity");
		}
/*
		echo(P"filter", getpid());
		echo(P"enable", enable);
*/
		if (execvp(argv[idx], &argv[idx])) {
			perror("exec() failed");
			exit(1);
		}
	}
	waitpid(pid, &st, 0);
//	echo(P"enable", 0);
	gettimeofday(&t2, NULL);
/*
	if (trace_fd >= 0) {
		cat(P"trace", trace_fd);
	}
*/
	timersub(&t2, &t1, &tt);
	fprintf(stderr, "tt: %llu microseconds\n", (unsigned long long)tt.tv_sec * 1000ULL*1000ULL + tt.tv_usec);
	return 0;
}

