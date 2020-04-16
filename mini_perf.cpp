#include<sys/types.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/syscall.h>
#include<linux/perf_event.h>

using namespace std;

#define ATRACE_MESSAGE_LEN 256
int     atrace_marker_fd = -1;

void trace_init()
{
  atrace_marker_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
  if (atrace_marker_fd == -1)   { /* do error handling */ }
}

inline void trace_counter(const char *name, const int value)
{
    char buf[ATRACE_MESSAGE_LEN];
    int len = snprintf(buf, ATRACE_MESSAGE_LEN, "C|%d|%s|%i", getpid(), name, value);
    write(atrace_marker_fd, buf, len);
}
int main() {
    trace_init();
    int cpu = 6;
    int counter = 5;
    int sample = 0x1000;
    struct perf_event_attr pe[cpu][counter];
    int fd[cpu][counter];
    for (int i=0 ; i<cpu ; i++)
    {
        int fd_prev = -1;
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
            fd[i][j] = syscall(__NR_perf_event_open, &ref, -1, i, fd_prev, 0);
            fd_prev = fd[i][j];
            if (fd[i][j] == -1) {
                printf("can not open perf %d %d by syscall",i,j);
                return -1;
            }
        }
    }
    unsigned long long data[sample][cpu][counter+1];
    for (int k=0 ; k<sample ; k++)
    {
        usleep(1000);
        trace_counter("pmu_index", k);
        for (int i=0 ; i<cpu ; i++)
            //for (int j=0 ; j<counter ; j++)
            {
                int j = 0;
                int re = read(fd[i][j], &(data[k][i][j]), (counter+1) * sizeof(unsigned long long));
            }
    }
    FILE *writer = fopen("mini_perf.data", "wb");
    fwrite(data, sizeof(unsigned long long),sample * cpu * counter, writer);
    fclose(writer);
    return 0;
}

