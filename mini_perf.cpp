#include<sys/types.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/syscall.h>
#include<linux/perf_event.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<getopt.h>

using namespace std;
#define ATRACE_MESSAGE_LEN 256
int     atrace_marker_fd = -1;

bool debug=false;
bool trace=false;
bool flow=false;
int interval = 1; // ms
int duration = 10; // s
int sample;
const int counter_max=6; // max 6 counter per group
const int group_max=8; // max 8 group
const int cpu_max=8;
int group=0;
int cpu = 0;
int cpu_id[cpu_max]; // 8 core
char group_name[group_max][counter_max][30] = {0};
void trace_init()
{
  if (!trace) return;
  atrace_marker_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
  if (atrace_marker_fd == -1)   { /* do error handling */ }
}

inline int trace_counter(const char *name, const int value)
{
    if (!trace) return 0;
    char buf[ATRACE_MESSAGE_LEN];
    int len = snprintf(buf, ATRACE_MESSAGE_LEN, "C|%d|%s|%i", getpid(), name, value);
    int ret = write(atrace_marker_fd, buf, len);
    return ret;
}

int perf(int pid=-1) {
    struct perf_event_attr pe[cpu][group][counter_max];
    int fd[cpu][group][counter_max];
    int group_num[group];

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
    int group_counter[group][counter_max];
    for (int group_i = 0 ; group_i < group; group_i++)
    {
        group_num[group_i] = 0;
        for (int count_i = 0 ; count_i < counter_max ; count_i++)
        {
            if (strlen(group_name[group_i][count_i]) == 0)
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
                exit(0);
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
#ifdef HOST
                ref.type = PERF_TYPE_HARDWARE;
#else
                ref.type = PERF_TYPE_RAW;
#endif
                ref.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_RUNNING;
                ref.config = group_counter[group_i][count_i];
                ref.freq = 1;
                fd[cpu_i][group_i][count_i] = syscall(__NR_perf_event_open, &ref, pid, cpu_id[cpu_i], fd_prev, 0);
                fd_prev = fd[cpu_i][group_i][0];
                if (fd[cpu_i][group_i][count_i] == -1) {
                    printf("can not open perf pid %d, cpu %d count %d by syscall\n",pid, cpu_id[cpu_i], group_counter[group_i][count_i]);
                    exit(0);
                }
            }
        }
    }

    // record counters
    int group_index[group+1];
    group_index[0] = 0;
    for (int i=0 ; i < group ; i++)
    {
        group_index[i+1] = group_index[i] + group_num[i] + 2;
    }
    unsigned long long data[sample][cpu][group_index[group]];
    for (int k = 0 ; k < sample ; k++)
    {
        usleep(interval * 1000);
        if(debug) {
            printf("finish read sample %d / %d\n",k,sample);
        }
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
            fprintf(writer, "count: group%d_cpu%d\n",group_i,cpu_id[cpu_i]);
            fprintf(writer, "count: time_cpu%d\n",cpu_id[cpu_i]);
            for(int count_i=0 ; count_i < group_num[group_i] ; count_i++)
            {
                fprintf(writer, "count: %s_cpu%d\n",group_name[group_i][count_i],cpu_id[cpu_i]);
            }
        }
    }
    fclose(writer);
    return 0;
}

char exe_path[100] = "";
void group_parsing(char *string) {
    int index = 0;
    char * token = strtok(string, ",");
    strcpy(group_name[group][index++], token);

    while( token != NULL) {
        token = strtok(NULL, ",");
        if (token != NULL) {
            strcpy(group_name[group][index++], token);
        }
    }
    group ++;
}

int main(int argc, char* argv[]) {
    int opt;
    int cpu_select = -1;
    struct option longopts [] = {
        {"group",required_argument, NULL,0},
        {"interval",required_argument, NULL,0},
        {"duration",required_argument, NULL,0},
        {0,0,0,0}
    };
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "ftdc:ae:",longopts,&option_index)) != -1) {
        switch (opt) {
            case 0:
                if (strcmp(longopts[option_index].name,"group")==0)
                    group_parsing(optarg);
                else if (strcmp(longopts[option_index].name,"interval")==0)
                    interval = atoi(optarg);
                else if (strcmp(longopts[option_index].name,"duration")==0) {
                    duration = atoi(optarg);
                }
                break;

            case 'f': flow = true; break;
            case 't': trace = true; break;
            case 'd': debug = true; break;
            case 'c': cpu_select = strtol(optarg, NULL, 16); break;
            case 'a': cpu_select = 0xff; break;
            case 'e': group_parsing(optarg); break;
            default: printf("-e exe_file\n-g: debug\n-t: trace\n-i interval(ms)\n-s sample\n-c: cpu\n"); return(0);
        }
    }
    if (optind < argc) {
        strcpy(exe_path, argv[optind]);
    }
    printf("exe_path %s\n",exe_path);
    printf("cpu_select %d\n",cpu_select);
    if (cpu_select == 0xff && strlen(exe_path)>0) {
        cpu_id[0] = -1;
        cpu=1;
    } else {
        cpu = 0;
        for (int i=0 ; i<8 ; i++)
        {
            if (cpu_select & (1<<i)) {
                cpu_id[cpu] = i;
                cpu ++;
            }
        }
    }
    for (int i=0 ; i<cpu ; i++)
    {
        printf("cpu %d is selected\n",cpu_id[i]);
    }

    // sample data est
    sample = duration * 1000 / interval;

    if (strlen(exe_path)>0) {
        pid_t pid;
        pid = fork();
        if (pid > 0) { // parent process
            if (!flow) {
                perf(pid);
            }
            int status;
            waitpid(pid, &status, 0);
        } else if (pid ==0) { // child process
            execl("/bin/sh", "-c", exe_path,NULL);
        } else {
            printf("fail in process fork\n");
        }
    } else {
        perf();
    }
}

