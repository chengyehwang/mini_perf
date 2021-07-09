// https://www.itread01.com/content/1549589596.html
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<sys/mman.h>
enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM,
	ION_HEAP_TYPE_SYSTEM_CONTIG,
	ION_HEAP_TYPE_CARVEOUT,
	ION_HEAP_TYPE_CHUNK,
	ION_HEAP_TYPE_DMA,
	ION_HEAP_TYPE_CUSTOM, /*
			       * must be last so device specific heaps always
			       * are at the end of this enum
			       */
};
typedef int ion_user_handle_t;
struct ion_handle_data {
	ion_user_handle_t handle;
};
struct ion_fd_data {
    ion_user_handle_t handle;
    int fd;
};
struct ion_allocation_data {
    size_t len;
    size_t align;
    unsigned int flags;
    ion_user_handle_t handle;
};
#define ION_IOC_MAGIC		'I'

#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data)
#define ION_IOC_FREE		_IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_SHARE		_IOWR(ION_IOC_MAGIC, 4, struct ion_handle_data)

int main()
{
	struct ion_fd_data fd_data;
	struct ion_allocation_data ionAllocData;
	ionAllocData.len = 0x1000;
	ionAllocData.align = 0;
	ionAllocData.flags = ION_HEAP_TYPE_SYSTEM;

	int fd=open("/dev/ion",O_RDWR);
	ioctl(fd,ION_IOC_ALLOC, &ionAllocData);
	fd_data.handle = ionAllocData.handle;
	ioctl(fd,ION_IOC_SHARE,&fd_data);

	int *p = mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED,fd_data.fd,0);
	p[0]=99;
	perror("test");
	printf("hello all %d\n",p[0]);
}
