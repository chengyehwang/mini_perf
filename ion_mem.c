#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<ion/ion.h>
#include<linux/ion.h>
unsigned char *ion_mem(int len) {
    int prot = PROT_READ | PROT_WRITE;
    int map_flags = MAP_SHARED;
    int alloc_flags = 0;
    int heap_mask = 1;

    int align = 0;

    int fd, ret;
    int map_fd;
    unsigned char *ptr;
    fd = ion_open();
    ion_user_handle_t handle;
    ret = ion_alloc(fd, len, align, heap_mask, alloc_flags, &handle);

    ret = ion_map(fd, handle, len, prot, map_flags, 0, &ptr, &map_fd);
    return ptr;
}
int main()
{
    unsigned char * p = ion_mem(1024*1024);
    p[0] = 'a';
    printf("hello all %d\n",p[0]);
}
