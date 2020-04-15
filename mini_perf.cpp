#include<sys/types.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/syscall.h>
#include<linux/perf_event.h>
int main() {
    int cpu = 6;
    int counter = 5;
    int sample = 0x1000;
    struct perf_event_attr pe[cpu][counter];
    int fd[cpu][counter];
    for (int i=0 ; i<cpu ; i++)
        for (int j=0 ; j<counter ; j++)
        {
            perf_event_attr& ref = pe[i][j];
            memset(& ref, 0, sizeof(struct perf_event_attr));
            ref.type = PERF_TYPE_HARDWARE;
            switch(j)
            {
                case 0:
                    ref.config = PERF_COUNT_HW_CPU_CYCLES;
                    break;
                case 1:
                    ref.config = PERF_COUNT_HW_INSTRUCTIONS;
                    break;
                case 2:
                    ref.config = PERF_COUNT_HW_CACHE_MISSES;
                    break;
                case 3:
                    ref.config = PERF_COUNT_HW_BRANCH_MISSES;
                    break;
                case 4:
                    ref.config = PERF_COUNT_HW_BUS_CYCLES;
                    break;
            }
            fd[i][j] = syscall(__NR_perf_event_open, &ref, 0, i, -1, 0);
            if (fd[i][j] == -1) {
                printf("can not open perf %d %d by syscall",i,j);
                return -1;
            }
        }
    unsigned long long data[sample][cpu][counter];
    for (int k=0 ; k<sample ; k++)
    {
        usleep(1000);
        for (int i=0 ; i<cpu ; i++)
            for (int j=0 ; j<counter ; j++)
            {
                int re = read(fd[i][j], &(data[k][i][j]), sizeof(unsigned long long));
            }
    }
    FILE *writer = fopen("mini_perf.data", "wb");
    fwrite(data, sizeof(unsigned long long),sample * cpu * counter, writer);
    fclose(writer);
    return 0;
}
