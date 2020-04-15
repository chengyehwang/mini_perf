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
	int cpu = 8;
	int counter = 6;
	int sample = 0x1000;
	struct perf_event_attr pe[cpu][counter];
	int fd[cpu][counter];
	for (int i=0 ; i<cpu ; i++)
		for (int j=0 ; j<counter ; j++)
		{
			perf_event_attr& ref = pe[i][j];
			memset(& ref, 0, sizeof(struct perf_event_attr));
			ref.type = PERF_TYPE_HARDWARE;
			ref.config = PERF_COUNT_HW_CPU_CYCLES;
			fd[i][j] = syscall(__NR_perf_event_open, &pe, 0, i, -1, 0);
			if (fd[i][j] == -1) {
				printf("can not open perf by syscall");
				exit(0);
			}
		}
	unsigned long long data[sample][cpu][counter];
	for (int k=0 ; k<sample ; k++)
		for (int i=0 ; i<cpu ; i++)
			for (int j=0 ; j<counter ; j++)
			{
				int re = read(fd[i][j], &(data[k][i][j]), sizeof(unsigned long long));
			}
	FILE *writer = fopen("mini_perf.bin", "wb");
	fwrite(data, sizeof(unsigned long long),sample * cpu * counter, writer);
	fclose(writer);
	return 0;
}
