#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/file.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <linux/version.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <dirent.h>
#include <inttypes.h>
#include <termios.h>
#include <unistd.h>

#ifdef PLATFORM_RK30

#include "ion.h"
#include "android_pmem.h"

#include "v4l2uvc.h"

#define ION_DEVICE          "/dev/ion"

//static void *m_v4l2Buffer[4];
static int v4l2Buffer_phy_addr = 0;
static void* mmap_addr = NULL;
static int iIonFd = -1;
struct ion_allocation_data ionAllocData;
struct ion_fd_data fd_data;
struct ion_handle_data handle_data;

static int total_size = 0x260000; // 足够yuv422格式缓冲区4个

static int iPmemFd = -1;

// 通过ion申请内存
static int malloc_buffer_rk30(struct video_info* vd_info)
{
    int err,size;
	struct pmem_region sub;
    iIonFd = open(ION_DEVICE, O_RDONLY|O_CLOEXEC);
    if(iIonFd < 0 )
    {
        printf("%s: Failed to open ion device - %s",
                __FUNCTION__, strerror(errno));
        iIonFd = -1;
        err = -1;
        goto exit1;
    }
    
    ionAllocData.len = total_size; //vd_info->frame_size_in;  ???
    
    printf("ionAllocData.len: %d\n", ionAllocData.len);
    ionAllocData.align = 4*1024; // 4K ??
    ionAllocData.flags = 1 << 0;
    err = ioctl(iIonFd, ION_IOC_ALLOC, &ionAllocData);
    if(err < 0)
    {
        printf("%s: ION_IOC_ALLOC failed to alloc 0x%x bytes with error - %s", 
            __FUNCTION__, ionAllocData.len, strerror(errno));
        
        err = -errno;
        goto exit2;
    }

    fd_data.handle = ionAllocData.handle;
    handle_data.handle = ionAllocData.handle;

    err = ioctl(iIonFd, ION_IOC_MAP, &fd_data);
    if(err < 0)
    {
        printf("%s: ION_IOC_MAP failed with error - %s",
                __FUNCTION__, strerror(errno));
        ioctl(iIonFd, ION_IOC_FREE, &handle_data);
        err = -errno;
       goto exit2;
    }
    mmap_addr = mmap(0, ionAllocData.len, PROT_READ|PROT_WRITE,
            MAP_SHARED, fd_data.fd, 0);
    if(mmap_addr == MAP_FAILED)
    {
        printf("%s: Failed to map the allocated memory: %s",
                __FUNCTION__, strerror(errno));
        err = -errno;
        ioctl(iIonFd, ION_IOC_FREE, &handle_data);
        goto exit2;
    }
    size = ionAllocData.len;
    err = ioctl(fd_data.fd, PMEM_GET_PHYS, &sub);
    if (err < 0)
    {
        printf(" PMEM_GET_PHY_ADDR failed, limp mode\n");
        ioctl(iIonFd, ION_IOC_FREE, &handle_data);
        goto exit2;
    }
    err = ioctl(iIonFd, ION_IOC_FREE, &handle_data);
    if(err)
    {
        printf("%s: ION_IOC_FREE failed with error - %s",
                __FUNCTION__, strerror(errno));
        err = -errno;
    }
    else
    {
        printf("%s: Successfully allocated 0x%x bytes, mIonFd=%d, SharedFd=%d",
                __FUNCTION__,ionAllocData.len, iIonFd, fd_data.fd);
    }
    
    //memset(m_v4l2Buffer[0], 0x00, size);
    printf("sub.offset = 0x%x  size: %d\n",sub.offset, size);
	v4l2Buffer_phy_addr = sub.offset;
    
    close(iIonFd);
    iIonFd = -1;
        
    return 0;
exit3:
    printf("exit3.....  \n");
	munmap(vd_info->mem[0], size);
exit2:
    printf("exit2.....  \n");
    if(iPmemFd > 0){
    	close(iPmemFd);
    	iPmemFd = -1;
        }
    if(iIonFd > 0){
    	close(iIonFd);
    	iIonFd = -1;
        }
exit1:
exit:
    printf("ll exit1 exit.....  \n");
    return err;
}

static int mmap_buffer_rk30(struct video_info* vd_info)
{
    int i = 0;
    int phy_addr = v4l2Buffer_phy_addr;
    int err = -1;
    
    memset(&vd_info->rb, 0, sizeof(struct v4l2_requestbuffers));
    vd_info->rb.count	= NB_BUFFER; /* 4 buffers */
    vd_info->rb.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // MJPEG格式的，使用MMAP
    if (vd_info->driver_type == V4L2_DRIVER_UVC)
    {
        vd_info->rb.memory	= V4L2_MEMORY_MMAP;
    }
    else
    {
        vd_info->rb.memory	= V4L2_MEMORY_OVERLAY;
    }
    //vd_info->rb.memory	= V4L2_MEMORY_OVERLAY; //V4L2_MEMORY_MMAP; //V4L2_MEMORY_OVERLAY;
    if (-1 == ioctl(vd_info->camfd, VIDIOC_REQBUFS, &vd_info->rb))
        unix_error_ret("unable to allocte buffers");

	printf("creqbuf.count = %d\n", vd_info->rb.count);
    
    for (i = 0; i < (int)vd_info->rb.count; i++)
    {
        memset(&vd_info->buf, 0, sizeof(struct v4l2_buffer));
        vd_info->buf.index	= i;
        vd_info->buf.type	= vd_info->rb.type;
        vd_info->buf.memory = vd_info->rb.memory;

        if (ioctl(vd_info->camfd, VIDIOC_QUERYBUF, &vd_info->buf) < 0)
        {
            unix_error_ret("VIDIOC_QUERYBUF failed");
        }

        vd_info->buf.m.offset = phy_addr + i * vd_info->buf.length;

        //m_v4l2Buffer[i] =(void*)((int)m_v4l2Buffer[0] + i*vd_info->buf.length);
        
        vd_info->mem[i] = (void*)((int)mmap_addr + i*vd_info->buf.length);
        
        printf("ll debug: mem[%d] = %p buffer.length: %d\n", i, vd_info->mem[i], vd_info->buf.length);
        err = ioctl(vd_info->camfd, VIDIOC_QBUF, &vd_info->buf);
        if (err < 0)
        {
            unix_error_ret("VIDIOC_QBUF failed");
        }
    }

    return 0;
}

int request_buffer_rk30(struct video_info* vd_info)
{
    int ret = 0;
    ret = malloc_buffer_rk30(vd_info);
    ret |= mmap_buffer_rk30(vd_info);
    
    return ret;
}

int release_rk30(struct video_info* vd_info)
{
    if(iIonFd > 0)
    {
        munmap(mmap_addr, ionAllocData.len);

        close(iIonFd);
        iIonFd = -1;
    }
    return 0;
}

#endif // PLATFORM_RK30

