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
#define STRIDE  (512/sizeof(char *))
#define	LOWER	8192
void	loads(size_t len, size_t range, size_t stride, 
	      int parallel, int warmup, int repetitions);
size_t	step(size_t k);

int	page = 64;

int fix_range = 0;

int scale = 1;

int
main(int ac, char **av)
{
	int	i;
	int	c;
	int	parallel = 1;
	int	warmup = 0;
	int	repetitions = 100;
	size_t	len;
	size_t	range;
	size_t	stride;
	char   *usage = "[-P <page>] [-R <range>] [-N <repetitions>] len [stride...]\n";

	while (( c = getopt(ac, av, "P:R:W:N:")) != EOF) {
		switch(c) {
		case 'P':
			page = atoi(optarg);
			break;
		case 'R':
			fix_range = atoi(optarg);
            scale = 100;
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
		for (int i=0; i<repetitions ; i++) {
			loads(len, fix_range, STRIDE, parallel, 
			      warmup, repetitions);
		}
    } else if (optind == ac - 1) {
		fprintf(stdout, "\"stride=%lu\n", STRIDE);
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

	int repeat= 10000000 / scale;
	/*
	 * Now walk them and time it.
	 */
	/* benchmp(line_initialize, benchmark_loads, mem_cleanup,
		0, parallel, warmup, repetitions, &state); */

	line_initialize(&state);
	start(0);
	benchmark_loads(repeat, &state);
	result = stop(0, 0);
	int div = state.npages * state.nlines;
	double latency = result / (repeat*100) * 1000;
	fprintf(stdout, "range: %9ld, div: %7d, count: %10d, time: %7.3f ns\n", range, div, repeat*100, latency);

	if (latency > 100) {
	scale = 100;
	} else if (latency > 10) {
	scale = 10;
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
