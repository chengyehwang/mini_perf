#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<ion.h>
#include<linux/ion.h>
unsigned char *ion_mem(int len) {
    int prot = PROT_READ | PROT_WRITE;
    int map_flags = MAP_SHARED;
    int alloc_flags = 0;
    int heap_mask = ION_HEAP_SYSTEM_CONTIG_MASK; // continuous mem

    int align = 0;

    int fd, ret;
    int map_fd;
    unsigned char *ptr;
    fd = ion_open();
    ion_user_handle_t handle;
    ret = ion_alloc(fd, len, align, heap_mask, alloc_flags, &handle);
    if (ret) { // try another mem
	  heap_mask = ION_HEAP_SYSTEM_MASK; // not continuous mem
	  ret = ion_alloc(fd, len, align, heap_mask, alloc_flags, &handle);
    }

    ret = ion_map(fd, handle, len, prot, map_flags, 0, &ptr, &map_fd);
    return ptr;
}
#ifdef ION_TEST
int main()
{
    unsigned char * p = ion_mem(1024*1024);
    for(int i = 0 ; i < 1024 * 1024 ; i++) {
	  p[i] = (char)i;
    }
    unsigned int sum = 0;
    for(int i = 0 ; i < 1024 * 1024 ; i++) {
	  sum += p[i];
    }
    p[0] = 'a';
    printf("hello all %d %d\n",p[0],sum);
}
#endif
