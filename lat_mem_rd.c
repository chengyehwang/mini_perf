/*
 * lat_mem_rd.c - measure memory load latency
 *
 * usage: lat_mem_rd [-P <parallelism>] [-W <warmup>] [-N <repetitions>] size-in-MB stride [stride ...]
 *
 * Copyright (c) 1994 Larry McVoy.  Distributed under the FSF GPL with
 * additional restriction that results may published only if
 * (1) the benchmark is unmodified, and
 * (2) the version in the sccsid below is included in the report.
 * Support for this development by Sun Microsystems is gratefully acknowledged.
 */
char	*id = "$Id: s.lat_mem_rd.c 1.13 98/06/30 16:13:49-07:00 lm@lm.bitmover.com $\n";

#include "bench.h"
#define STRIDE  8
#define	LOWER	4096
//#define	LOWER	256
void	loads(size_t len, size_t range, size_t stride, 
	      int parallel, int warmup, int repetitions);
size_t	step(size_t k);

int	page = 8;

int fix_range = 0;

int scale = 1;

int infinit = 0;

void proc_exit(int s) {
    fflush(stdout);
    exit(0);
}

int
main(int ac, char **av)
{
    signal(SIGINT, proc_exit);
	int	i;
	int	c;
	int	parallel = 1;
	int	warmup = 0;
	int	repetitions = 100;
	size_t	len;
	size_t	range;
	size_t	stride;
	char   *usage = "[-P <page>] [-R <range>] [-I] [-N <repetitions>] len [stride...]\n";

	while (( c = getopt(ac, av, "P:IR:W:N:")) != EOF) {
		switch(c) {
		case 'P':
			page = atoi(optarg);
			break;
        case 'I':
            infinit = 1;
            break;
		case 'R':
			fix_range = atoi(optarg);
            if (fix_range > 1<<22) {
                scale = 100;
            } if (fix_range > 1<<18) {
                scale = 10;
            } else {
                scale = 1;
            }
			break;
		case 'N':
			repetitions = atoi(optarg);
			break;
		default:
			lmbench_usage(ac, av, usage);
			break;
		}
	}
	if (optind == ac) {
		lmbench_usage(ac, av, usage);
	}

        len = atoi(av[optind]) * 1024 * 1024;

    if (fix_range != 0) {
		loads(len, fix_range, STRIDE, parallel,
		    warmup, repetitions);
    } else if (optind == ac - 1) {
		fprintf(stdout, "\"stride=%u\n", STRIDE);
		for (range = LOWER; range <= len; range = step(range)) {
			loads(len, range, STRIDE, parallel, 
			      warmup, repetitions);
		}
	} else {
		for (i = optind + 1; i < ac; ++i) {
			stride = bytes(av[i]);
			fprintf(stdout, "\"stride=%zu\n", stride);
			for (range = LOWER; range <= len; range = step(range)) {
				loads(len, range, stride, parallel, 
				      warmup, repetitions);
			}
			fprintf(stdout, "\n");
		}
	}
	return(0);
}

#define	ONE	p = (char **)*p; /*printf("%x\n",p);*/
#define	FIVE	ONE ONE ONE ONE ONE
#define	TEN	FIVE FIVE
#define	FIFTY	TEN TEN TEN TEN TEN
#define	HUNDRED	FIFTY FIFTY


void
benchmark_loads(iter_t iterations, void *cookie)
{
	struct mem_state* state = (struct mem_state*)cookie;
	register char **p = (char**)state->base;
	register size_t i;

	while (iterations-- > 0) {
			HUNDRED;
	}

	use_pointer((void *)p);
}

void
loads(size_t len, size_t range, size_t stride, 
	int parallel, int warmup, int repetitions)
{
	double result;
	size_t count;
	struct mem_state state;

	state.width = 1;
	state.len = range;
	state.maxlen = len;
	state.line = stride;
	state.pagesize = page;

	long long int repeat= 100000000 / scale;
	/*
	 * Now walk them and time it.
	 */
	/* benchmp(line_initialize, benchmark_loads, mem_cleanup,
		0, parallel, warmup, repetitions, &state); */

	line_initialize(&state);
    double latency;
    while (1) {
	start(0);
    benchmark_loads(repeat, &state);
	result = stop(0, 0);
	int div = state.npages * state.nlines;
	latency = result / (repeat*100) * 1000;
	fprintf(stdout, "range: %9ld, div: %7d, count: %10lld, time: %7.3f ns\n", range, div, repeat*100, latency);
    if (!infinit) break;
    }

	if (latency > 100.0) {
	scale = 100;
	} else if (latency > 20.0) {
	scale = 10;
    } else {
    scale = 1;
	}
}

size_t
step(size_t k)
{
    for (int i = 30 ; i > 5 ; i--)
    {
        if (k >= (1<<i)) {
            return k += 1<<(i-1);
        }
    }
    return k * 2;
}
