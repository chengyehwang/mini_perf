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

inline int trace_counter(const char *name, const int value)
{
    char buf[ATRACE_MESSAGE_LEN];
    int len = snprintf(buf, ATRACE_MESSAGE_LEN, "C|%d|%s|%i", getpid(), name, value);
    int ret = write(atrace_marker_fd, buf, len);
    return ret;
}
int main() {
    trace_init();
    int cpu = 6;
    int group = 1;
    int counter = 5;
    int sample = 0x1000;
    struct perf_event_attr pe[cpu][counter];
    int fd[cpu][counter];
    int group_counter[group][counter] = {
        {   PERF_COUNT_HW_CPU_CYCLES,
            PERF_COUNT_HW_INSTRUCTIONS,
            PERF_COUNT_HW_CACHE_MISSES,
            PERF_COUNT_HW_BRANCH_MISSES,
            PERF_COUNT_HW_BUS_CYCLES
         }
    };
    for (int cpu_i=0 ; cpu_i<cpu ; cpu_i++)
    {
        int fd_prev = -1;
        for (int count_j=0 ; count_j<counter ; count_j++)
        {
            perf_event_attr& ref = pe[cpu_i][count_j];
            memset(& ref, 0, sizeof(struct perf_event_attr));
            ref.type = PERF_TYPE_HARDWARE;
            ref.read_format = PERF_FORMAT_GROUP;
            ref.config = group_counter[0][count_j];
            fd[cpu_i][count_j] = syscall(__NR_perf_event_open, &ref, -1, cpu_i, fd_prev, 0);
            fd_prev = fd[cpu_i][0];
            if (fd[cpu_i][count_j] == -1) {
                printf("can not open perf %d %d by syscall",cpu_i,count_j);
                return -1;
            }
        }
    }
    unsigned long long data[sample][cpu][counter+1];
    for (int k=0 ; k<sample ; k++)
    {
        usleep(1000);
        int ret = trace_counter("pmu_index", k);
        if (ret <0)
        {
            k = -1;
            continue;
        }
        for (int i=0 ; i<cpu ; i++)
            //for (int j=0 ; j<counter ; j++)
            {
                int j = 0;
                int re = read(fd[i][j], &(data[k][i][j]), (counter+1) * sizeof(unsigned long long));
            }
    }
    FILE *writer = fopen("mini_perf.data", "wb");
    fwrite(data, sizeof(unsigned long long),sample * cpu * (counter+1), writer);
    fclose(writer);
    return 0;
}

