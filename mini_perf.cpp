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
    const int cpu = 1;
    const int group = 2;
    const int counter = 5;
    const int sample = 0x1000;
    struct perf_event_attr pe[cpu][group][counter];
    int fd[cpu][group][counter];
    int group_num[group] = {0};
    const char *group_name[group][counter] = {
#ifdef HOST
        {
            "instructions",
            "cache_misses"
        },
        {
            "instructions",
            "branch_misses"
        }
#else
        {
            "raw-inst-retired",
            "raw-l1i-cache",
            "raw-l1i-cache-refill",
            "raw-l1d-cache",
            "raw-l1d-cache-refill"
        },
        {
            "raw-inst-retired",
            "raw-l2d-cache",
            "raw-l2d-cache-refill",
            "raw-l3d-cache",
            "raw-l3d-cache-refill"
        }
#endif
    };

    trace_init();
    #define EVENT_TYPE_TABLE_ENTRY(NAME, TYPE, ID, COM, S) {.id = ID, .name = NAME, .comm = COM},
    struct event_t {
        int id;
        const char *name;
        const char *comm;
    };
    event_t event_name[] = {
        #include "event_type_table.h"
        {.id = -1, .name = "", .comm = ""}
    };

    // search counter id from counter name
    int group_counter[group][counter]={-1};
    for (int group_i = 0 ; group_i < group; group_i++)
    {
        for (int count_i = 0 ; count_i < counter ; count_i++)
        {
            if (group_name[group_i][count_i] == 0)
            {
                group_counter[group_i][count_i] = -1;
                continue;
            }
            group_num[group_i] += 1;
            for (int i = 0 ; ;i++)
            {
                //printf("%s %d\n",event_name[i].name,event_name[i].id);
                if (event_name[i].id == -1)
                    break;
                if (strcmp(group_name[group_i][count_i], event_name[i].name)==0)
                {
                    group_counter[group_i][count_i] = event_name[i].id;
                }
            }
            if (group_counter[group_i][count_i] == -1)
            {
                printf("%s is missed\n",group_name[group_i][count_i]);
                return 0;
            }
            else
            {
                printf("name=%s id=%d\n",group_name[group_i][count_i],group_counter[group_i][count_i]);
            }
        }
        printf("group %d: num=%d\n",group_i, group_num[group_i]);
    }

    // register counter
    for (int cpu_i = 0 ; cpu_i < cpu ; cpu_i++)
    {
        for (int group_i = 0 ; group_i < group ; group_i++)
        {
            int fd_prev = -1;
            for (int count_i=0 ; count_i < group_num[group_i] ; count_i++)
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
                    printf("can not open perf cpu %d count %d by syscall",cpu_i,group_counter[0][count_i]);
                    return -1;
                }
            }
        }
    }

    // record counters
    int group_index[group+1] = {0};
    for (int i=0 ; i < group ; i++)
    {
        group_index[i+1] = group_index[i] + group_num[i] + 2;
    }
    unsigned long long data[sample][cpu][group_index[group]];
    for (int k = 0 ; k < sample ; k++)
    {
        usleep(1000);
        int ret = trace_counter("pmu_index", k);
        if (ret < 0)
        {
            k = -1;
            continue;
        }
        for (int i = 0 ; i < cpu ; i++)
            for (int j = 0 ; j < group ; j++)
            {
                int re = read(fd[i][j][0], &(data[k][i][group_index[j]]), (group_num[j]+2) * sizeof(unsigned long long));
            }
    }

    // write data to file
    FILE *writer = fopen("mini_perf.data", "wb");
    fwrite(data, sizeof(unsigned long long),sample * cpu * group_index[group], writer);
    fclose(writer);

    // wirte head to file
    writer = fopen("mini_perf.head", "w");
    for (int cpu_i = 0 ; cpu_i < cpu ; cpu_i++)
    {
        for (int group_i = 0 ; group_i < group ; group_i++)
        {
            fprintf(writer, "count: group%d_cpu%d\n",group_i,cpu_i);
            fprintf(writer, "count: time_cpu%d\n",cpu_i);
            for(int count_i=0 ; count_i < group_num[group_i] ; count_i++)
            {
                fprintf(writer, "count: %s_cpu%d\n",group_name[group_i][count_i],cpu_i);
            }
        }
    }
    fclose(writer);
    return 0;
}

