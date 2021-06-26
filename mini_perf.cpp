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
    #define EVENT_TYPE_TABLE_ENTRY(NAME, TYPE, ID, COM, S) {.id = ID, .name = NAME, .com = COM},
    struct event_t {
        int id;
        const char *name;
        const char *com;
    };
    event_t event_name[] = {
        #include "event_type_table.h"
        {.id = 0, .name = "", .com = ""}
    };
    //for(int i=0;i<256;i++)
    //    printf("%x ",(unsigned long long) (event_name[i]));

    int cpu = 1;
    int group = 2;
    int counter = 2;
    int sample = 0x1000;
    struct perf_event_attr pe[cpu][group][counter];
    int fd[cpu][group][counter];
    char group_name[group][counter][20] = {
        {
            "instructions",
            "cache_misses"
        },
        {
            "instructions",
            "branch_misses"
        }
    };
    int group_counter[group][counter]={0};
    for (int group_i=0;group_i<group;group_i++)
    {
        for (int count_i=0;count_i<counter;count_i++)
        {
            for (int i=0;;i++)
            {
                if (event_name[i].id==0)
                    break;
                if (strcmp(group_name[group_i][count_i], event_name[i].name)==0)
                {
                    group_counter[group_i][count_i] = event_name[i].id;
                }
            }
            if (group_counter[group_i][count_i] == 0)
            {
                printf("%s is missed\n",group_name[group_i][count_i]);
                return 0;
            }
        }
    }
    for (int cpu_i=0 ; cpu_i<cpu ; cpu_i++)
    {
        for (int group_i=0 ; group_i < group ; group_i++)
        {
            int fd_prev = -1;
            for (int count_i=0 ; count_i<counter ; count_i++)
            {
                perf_event_attr& ref = pe[cpu_i][group_i][count_i];
                memset(& ref, 0, sizeof(struct perf_event_attr));
                ref.type = PERF_TYPE_HARDWARE;
                ref.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_RUNNING;
                ref.config = group_counter[0][count_i];
                ref.freq = 1;
                fd[cpu_i][group_i][count_i] = syscall(__NR_perf_event_open, &ref, -1, cpu_i, fd_prev, 0);
                fd_prev = fd[cpu_i][group_i][0];
                if (fd[cpu_i][group_i][count_i] == -1) {
                    printf("can not open perf %d %d by syscall",cpu_i,count_i);
                    return -1;
                }
            }
        }
    }
    unsigned long long data[sample][cpu][group][counter+2];
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
            for (int j=0 ; j<group ; j++)
            {
                int re = read(fd[i][j][0], &(data[k][i][j][0]), (counter+2) * sizeof(unsigned long long));
            }
    }
    FILE *writer = fopen("mini_perf.data", "wb");
    fwrite(data, sizeof(unsigned long long),sample * cpu * group * (counter+2), writer);
    fclose(writer);
    writer = fopen("mini_perf.head", "w");
    for (int cpu_i = 0 ; cpu_i < cpu ; cpu_i++)
    {
        for (int group_i=0 ; group_i<group ; group_i++)
        {
            fprintf(writer, "count: group%d_cpu%d\n",group_i,cpu_i);
            fprintf(writer, "count: time_cpu%d\n",cpu_i);
            for(int count_i=0 ; count_i<counter ; count_i++)
            {
                fprintf(writer, "count: %s_cpu%d\n",group_name[group_i][count_i],cpu_i);
            }
        }
    }
    fclose(writer);
    return 0;
}

