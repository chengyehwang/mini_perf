#include<sys/types.h>
#include<sys/ioctl.h>
#include<sys/mman.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/syscall.h>
#include<linux/perf_event.h>
#include<linux/hw_breakpoint.h>
int main() {
	struct perf_event_attr pe;
	memset(&pe, 0, sizeof(struct perf_event_attr));
	pe.type = PERF_TYPE_HARDWARE;
	pe.size = sizeof(struct perf_event_attr);
	pe.config = PERF_COUNT_HW_CPU_CYCLES;
	//pe.config = PERF_COUNT_HW_INSTRUCTIONS;
	//pe.freq = 0;
	pe.sample_freq = 1000;
	pe.sample_type = PERF_SAMPLE_TIME;
	pe.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED;
	pe.disabled = 1;
	pe.exclude_kernel = 1;
	pe.exclude_hv = 1;
	pe.mmap = 1;
	int fd;
	int pid = 0; // this process
	int cpu = -1; // all cpu
	fd = syscall(__NR_perf_event_open, &pe, pid, cpu, -1, 0);
	if (fd == -1) {
		printf("can not open perf by syscall");
		exit(0);
	}
	void *addr = mmap(NULL, 0x3000 , PROT_READ,
                       MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		printf("can not map by mmap");
		exit(0);
	}
	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
	for(int i=1;i<10;i++) {
		
		sleep(1);
		struct packet{
			long long value;
			long long time;
		} data;
		data.time = 0;
		data.value = 0;
		struct perf_event_mmap_page a;
        	int re = read(fd, &data, sizeof(struct packet));
		//int re = 0;
		//data.time = *(volatile long long *) addr;
		printf("re %d time %lld value %lld\n",
			re, data.time, data.value);
	}
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
	return 0;
}
