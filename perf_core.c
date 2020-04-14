#include<sys/types.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/syscall.h>
#include<linux/perf_event.h>
#include<linux/hw_breakpoint.h>
void main() {
	struct perf_event_attr pe;
	memset(&pe, 0, sizeof(struct perf_event_attr));
	pe.type = PERF_TYPE_HARDWARE;
	pe.size = sizeof(struct perf_event_attr);
	pe.config = PERF_COUNT_HW_INSTRUCTIONS;
	pe.freq = 1;
	pe.sample_freq = 1000;
	pe.sample_type = PERF_SAMPLE_TIME;
	pe.disabled = 1;
	pe.exclude_kernel = 1;
	pe.exclude_hv = 1;
	int fd;
	int pid = 0; // all process
	int cpu = -1; // for cpu0
	fd = syscall(__NR_perf_event_open, &pe, pid, cpu, -1, 0);
	if (fd == -1) {
		printf("can not open perf syscall");
		exit(0);
	}
	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
	printf("measure the print\n");
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
	long long count;
        read(fd, &count, sizeof(long long));
	printf("%lld inst\n", count);
        read(fd, &count, sizeof(long long));
	printf("%lld inst\n", count);
        read(fd, &count, sizeof(long long));
	printf("%lld inst\n", count);
        read(fd, &count, sizeof(long long));
	printf("%lld inst\n", count);
}
