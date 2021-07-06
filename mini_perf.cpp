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
#include<signal.h>
#include<sys/time.h>

using namespace std;
#define ATRACE_MESSAGE_LEN 256
int     atrace_marker_fd = -1;

bool debug=false;
bool trace=false;
bool flow=false;
bool print=false;
int user=false;
int pids=-1;
int interval = 10; // ms
int duration = 10; // s
int sample;
const int counter_max=6; // max 6 counter per group
const int group_max=8; // max 8 group
const int cpu_max=8;
int group=0;
int group_index[group_max+1];
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

bool child_finish = false;

void proc_exit(int) {
    child_finish = true;
}

unsigned data_group(unsigned sample_i, unsigned cpu_i, unsigned group_i) {
    unsigned r1 = sample_i * (1 + cpu * group_index[group]);
    unsigned r2 = 1 + cpu_i * group_index[group];
    unsigned r3 = group_index[group_i];
    return (r1 + r2 + r3);
}
unsigned data_time(unsigned sample_i) {
    unsigned r1 = sample_i * (1 + cpu * group_index[group]);
    return (r1);
}
int perf(int pid=-1) {
    if (pid != -1)
        printf("profile pid %d\n",pid);
    struct perf_event_attr pe[cpu][group][counter_max];
    int fd[cpu][group][counter_max];
    int group_num[group];

    trace_init();
    #define EVENT_TYPE_TABLE_ENTRY(NAME, TYPE, CONFIG, COM, S) {.config = CONFIG, .type=TYPE, .name = NAME, .comm = COM},
    struct event_t {
        unsigned long long config;
        unsigned int type;
        const char *name;
        const char *comm;
    };
    event_t event_name[] = {
        #include "event_type_table.h"
        {.config = 0, .type = 0, .name = "", .comm = ""}
    };

    // search counter id from counter name
    event_t *group_counter[group][counter_max];
    for (int group_i = 0 ; group_i < group; group_i++)
    {
        group_num[group_i] = 0;
        for (int count_i = 0 ; count_i < counter_max ; count_i++)
        {
            if (strlen(group_name[group_i][count_i]) == 0)
            {
                group_counter[group_i][count_i] = NULL;
                continue;
            }
            group_num[group_i] += 1;
            for (int i = 0 ; ;i++)
            {
                //printf("%s %d\n",event_name[i].name,event_name[i].id);
                if (strcmp(group_name[group_i][count_i], event_name[i].name)==0)
                {
                    group_counter[group_i][count_i] = &(event_name[i]);
                    printf("name=%s config=%llu\n",group_name[group_i][count_i],group_counter[group_i][count_i]->config);
                    break;
                }
                if (strcmp(event_name[i].name,"")==0) {
                    printf("%s is missed\n",group_name[group_i][count_i]);
                    exit(0);
                }
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
                ref.type = group_counter[group_i][count_i]->type;
                ref.size = sizeof(ref);
                ref.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_TOTAL_TIME_ENABLED ;
                ref.config = group_counter[group_i][count_i]->config;
                ref.sample_freq = 1;
                ref.freq = 1;
                int fd_ref = syscall(__NR_perf_event_open, &ref, pid, cpu_id[cpu_i], fd_prev, 0);
                fd[cpu_i][group_i][count_i] = fd_ref;
                fd_prev = fd[cpu_i][group_i][0];
                if (fd_ref == -1) {
                    printf("can not open perf pid %d, cpu %d config %llu by syscall\n",pid, cpu_id[cpu_i], group_counter[group_i][count_i]->config);
                    fd[cpu_i][group_i][0] = -1;
                    break;
                }
                else {
                    printf("open perf pid %d, cpu %d config %llu by syscall\n",pid, cpu_id[cpu_i], group_counter[group_i][count_i]->config);
                }
                ioctl(fd_ref, PERF_EVENT_IOC_RESET, 0);
                ioctl(fd_ref, PERF_EVENT_IOC_ENABLE, 0);
            }
        }
    }
    // record counters
    group_index[0] = 0;
    for (int i=0 ; i < group ; i++)
    {
        group_index[i+1] = group_index[i] + group_num[i] + 3;
    }
    struct timeval tp;
    gettimeofday(&tp, NULL);
    unsigned long long us_start = tp.tv_sec * 1000000 + tp.tv_usec;
    unsigned data_total = sample * (1 + cpu * group_index[group]);
    unsigned long long data[data_total];
    memset(data, 0, sizeof(unsigned long long) * data_total);
    for (int sample_i = 0 ; sample_i < sample ; sample_i++)
    {
        if (child_finish) {
            sample = sample_i;
            break;
        }
        usleep(interval * 1000);
        if(debug) {
            printf("finish read sample %d / %d\n",sample_i,sample);
        }
        int ret = trace_counter("pmu_index", sample_i);
        if (ret < 0)
        {
            sample_i = -1;
            continue;
        }
        for (int cpu_i = 0 ; cpu_i < cpu ; cpu_i++)
            for (int group_i = 0 ; group_i < group ; group_i++)
            {

                if (fd[cpu_i][group_i][0] == -1) continue;
                int re = read(fd[cpu_i][group_i][0], data+data_group(sample_i,cpu_i,group_i), (group_num[group_i]+3) * sizeof(unsigned long long));
                if (re == -1) {fd[cpu_i][group_i][0] = -1;} // read err
                if (print) {
			printf("time_enabled time_running %llu %llu #\n",data[data_group(sample_i,cpu_i,group_i)+1],data[data_group(sample_i,cpu_i,group_i)+2]);
                    for(int m = 0 ; m < group_num[group_i] ; m++) {
                        printf(" %10lld %20s_g%d_cpu%d #\n",data[data_group(sample_i,cpu_i,group_i)+3+m], group_name[group_i][m],group_i,cpu_i);
                    }
                }
            }
        struct timeval tp;
        gettimeofday(&tp, NULL);
        unsigned long long us = tp.tv_sec * 1000000 + tp.tv_usec - us_start;
        data[data_time(sample_i)] = us;
        if (print) {
            printf("time %lld us\n",us);
        }
    }

    // close all file handler
    for (int cpu_i = 0 ; cpu_i < cpu ; cpu_i++)
    {
        for (int group_i = 0 ; group_i < group ; group_i++)
        {
            for (int count_i=0 ; count_i < group_num[group_i] ; count_i++) {
                int fd_ref = fd[cpu_i][group_i][count_i];
                if (fd_ref != -1) {
                    close(fd_ref);
                }
            }
        }
    }
    // write data to file
#ifdef HOST
    FILE *writer = fopen("mini_perf.data", "wb");
#else
    FILE *writer = fopen("/data/local/tmp/mini_perf.data", "wb");
#endif
    data_total = sample * (1 + cpu * group_index[group]);
    fwrite(data, sizeof(unsigned long long), data_total, writer);
    fclose(writer);

#ifdef HOST
    writer = fopen("mini_perf.last_data", "wb");
#else
    writer = fopen("/data/local/tmp/mini_perf.last_data", "wb");
#endif
    data_total = (1 + cpu * group_index[group]);
    unsigned long long *last_data = data + (sample -1) * data_total ;
    fwrite(data, sizeof(unsigned long long), data_total, writer);
    fwrite(last_data, sizeof(unsigned long long), data_total, writer);
    fclose(writer);

    // wirte head to file
#ifdef HOST
    writer = fopen("mini_perf.head", "w");
#else
    writer = fopen("/data/local/tmp/mini_perf.head", "w");
#endif
    fprintf(writer, "count: time\n");
    for (int cpu_i = 0 ; cpu_i < cpu ; cpu_i++)
    {
        char cpu_name[10]="";
        if (cpu_id[cpu_i] != -1) {
            sprintf(cpu_name, "_cpu%d",cpu_id[cpu_i]);
        }
        for (int group_i = 0 ; group_i < group ; group_i++)
        {
            fprintf(writer, "count: group_g%d%s\n",group_i,cpu_name);
            fprintf(writer, "count: time_enabled_g%d%s\n",group_i,cpu_name);
            fprintf(writer, "count: time_running_g%d%s\n",group_i,cpu_name);
            for(int count_i=0 ; count_i < group_num[group_i] ; count_i++)
            {
                fprintf(writer, "count: %s_g%d%s\n",group_name[group_i][count_i],group_i,cpu_name);
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
    bool cpu_all = false;
    struct option longopts [] = {
        {"group",required_argument, NULL,0},
        {"interval",required_argument, NULL,0},
        {"duration",required_argument, NULL,0},
        {"cache6",no_argument, NULL,0},
        {"cache4",no_argument, NULL,0},
        {"cache3",no_argument, NULL,0},
        {0,0,0,0}
    };
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "ftdc:asue:p:",longopts,&option_index)) != -1) {
        switch (opt) {
            case 0:
                if (strcmp(longopts[option_index].name,"cache3")==0) {
                    char x0[] = "raw-inst-retired,raw-cpu-cycles";
                    group_parsing(x0);
                    char x1[] = "raw-inst-retired,raw-l1i-cache,raw-l1d-cache";
                    group_parsing(x1);
                    char x2[] = "raw-l1i-cache,raw-l1i-cache-refill";
                    group_parsing(x2);
                    char x3[] = "raw-l1d-cache,raw-l1d-cache-refill";
                    group_parsing(x3);
                    char x4[] = "raw-l2d-cache,raw-l2d-cache-refill";
                    group_parsing(x4);
                    char x5[] = "raw-l3d-cache,raw-l3d-cache-refill,raw-l2d-tlb-refill";
                    group_parsing(x5);
                    cpu_select = 0xff;
                }
                else if (strcmp(longopts[option_index].name,"cache6")==0) {
                    char x0[] = "raw-inst-retired,raw-cpu-cycles,raw-l1i-cache,raw-l1d-cache,raw-l1i-cache-refill,raw-l1d-cache-refill";
                    group_parsing(x0);
                    char x1[] = "raw-l2d-tlb-refill,raw-l2d-cache,raw-l2d-cache-refill,raw-l3d-cache,raw-l3d-cache-refill";
                    group_parsing(x1);
                    cpu_select = 0xff;
                }
                else if (strcmp(longopts[option_index].name,"cache4")==0) {
                    char x0[] = "raw-inst-retired,raw-cpu-cycles,raw-l1i-cache,raw-l1d-cache";
                    group_parsing(x0);
                    char x1[] = "raw-l1i-cache,raw-l1d-cache,raw-l1i-cache-refill,raw-l1d-cache-refill";
                    group_parsing(x1);
                    char x2[] = "raw-l2d-cache,raw-l2d-cache-refill,raw-l3d-cache,raw-l3d-cache-refill";
                    group_parsing(x2);
                    cpu_select = 0xff;
                }
                else if (strcmp(longopts[option_index].name,"group")==0)
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
            case 's': print = true; break;
            case 'u': user = true; break;
            case 'e': group_parsing(optarg); break;
            case 'p': pids = atoi(optarg); break;
            default: printf("-e exe_file\n-g: debug\n-t: trace\n-i interval(ms)\n-s sample\n-c: cpu\n"); return(0);
        }
    }
    if (optind < argc) {
        strcpy(exe_path, argv[optind]);
    }
    printf("exe_path %s\n",exe_path);
    printf("cpu_select %d\n",cpu_select);
    if (cpu_all) {
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

    signal(SIGINT, proc_exit);
    if (strlen(exe_path)>0) {
        signal(SIGCHLD, proc_exit);
        pid_t pid = fork();
        if (pid > 0) { // parent process
            if (!flow) {
                if (user)
                    perf(pid);
                else
                    perf();
            }
            int status;
            if (!child_finish)
                waitpid(pid, &status, 0);
        } else if (pid ==0) { // child process
            printf("child process: /bin/sh -c %s",exe_path);
            execl("/bin/sh", "-c", exe_path,NULL);
        } else {
            printf("fail in process fork\n");
        }
    } else {
        perf(pids);
    }
}

