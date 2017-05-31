/*
 *                           _/_/     _/_/_/     _/_/_/_/_/
 *                        _/    _/  _/              _/
 *                       _/         _/             _/
 *                      _/            _/_/        _/
 *                     _/                 _/     _/
 *                     _/   _/      _/    _/    _/
 *                      _/_/         _/_/    _/_/_/
 */
/**
 *                      Copyleft (C) 2010  Late Lee
 *        This program is tested on LINUX PLATFORM, WITH GCC 4.x.
 *        The program is distributed in the hope that it will be
 *        useful, but WITHOUT ANY WARRANTY. Please feel free to 
 *        use the program, and I feel free to ignore the related
 *        issues. Any questions or suggestions, or bugs, please 
 *        contact me at
 *        <$ echo -n "aHR0cDovL3d3dy5sYXRlbGVlLm9yZwo=" | base64 -d>
 *        or e-mail to 
 *        <$ echo -n "bGF0ZWxlZUAxNjMuY29tCg==" | base64 -d>
 *        if you want to do this.
 *
 * @file   v4l2uvc.c
 * @author Late Lee <latelee@163.com>
 * @date   Sat Apr 24 2010
 *
 * @brief
 * vedio capture for Vedio Surveillance System(VSS)
 * using v4l2 APIs
 *
 */
#include "v4l2uvc.h"
#include "utils.h"

static v4l2_process_cb_ptr v4l2_processcb = v4l2_process;

/** 
 * v4l2_open - 打开摄像头
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功返回值为0，否则为-1，并提示出错信息
 * @note vd_info结构体中的摄像头描述符由此函数指定
 */
int v4l2_open(struct video_info* vd_info)
{
    if (!strcmp(vd_info->name, ""))
        strcpy(vd_info->name, DEFAULT_VIDEO);

    /* open it --no block */
    vd_info->camfd = open(vd_info->name, O_RDWR /*| O_NONBLOCK | O_CLOEXEC*/, 0);
    if (vd_info->camfd < 0)
        unix_error_ret("can not open the device");
    debug_msg("open success!\n");
    debug_msg("===============================\n\n");

    return 0;
}

extern int release_rk30(struct video_info* vd_info);

/** 
 * v4l2_close - 关闭摄像头，并释放内存
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功关闭返回值为0，否则为-1，并提示出错信息
 */
int v4l2_close(struct video_info* vd_info)
{
    uint16 i = 0;
    if (vd_info->is_streaming)  /* stop if it is still capturing */
        v4l2_off(vd_info);

    if (vd_info->frame_buffer)
    {
        free(vd_info->frame_buffer);
        vd_info->frame_buffer = NULL;
    }
    if (vd_info->tmp_buffer)
    {
        free(vd_info->tmp_buffer);
        vd_info->tmp_buffer = NULL;
    }

#ifdef PLATFORM_RK30
    release_rk30(vd_info);
#endif
    if (vd_info->mem_mapped)
    {
        for (i = 0; i < NB_BUFFER; i++)
        {
            printf("munamp[%d]...\n", i);
            if (vd_info->mem[i])
                if (-1 == munmap(vd_info->mem[i], vd_info->buf.length))
                    unix_error_ret("munmap");
        }
    }

    close(vd_info->camfd);
    debug_msg("close OK!\n");

    return 0;
}

static int malloc_userdata(struct video_info* vd_info)
{
    // 默认w*h*2，即YUV422格式，后面根据实际情况填写
    vd_info->frame_size_in = (vd_info->width * vd_info->height << 1);
    //printf("will alloc size: %d\n", vd_info->frame_size_in);
    switch (vd_info->format)    /** 'format' will be also ok */
    {
    case V4L2_PIX_FMT_MJPEG:
        vd_info->tmp_buffer =
            (uint8 *)calloc(1, (size_t)vd_info->frame_size_in);
        if (vd_info->tmp_buffer == NULL)
            unix_error_ret("unable alloc tmp_buffer");
        vd_info->frame_buffer =
            (uint8 *)calloc(1, (size_t)vd_info->width*(vd_info->height+8) * 2);
        if (vd_info->frame_buffer == NULL)
            unix_error_ret("unable alloc frame_buffer");
        break;
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YYUV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
        vd_info->frame_buffer =
            (uint8 *)calloc(1,(size_t)vd_info->frame_size_in);
        if (vd_info->frame_buffer == NULL)
            unix_error_ret("unable alloc frame_buffer");
        break;
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
        vd_info->frame_size_in = (vd_info->width * vd_info->height * 3 / 2);
        //printf("will alloc size: %d\n", vd_info->frame_size_in);
        vd_info->frame_buffer =
            (uint8 *)calloc(1,(size_t)vd_info->frame_size_in);
        if (vd_info->frame_buffer == NULL)
            unix_error_ret("unable alloc frame_buffer");
        break;
    default:
        msg_out("v4l2 init error! not support format: 0x%x\n", vd_info->format);
        return -1;
        break;
    }
    
    return 0;
}
static int setformat(struct video_info* vd_info)
{
    if (-1 == ioctl(vd_info->camfd, VIDIOC_QUERYCAP, &vd_info->cap))
        unix_error_ret("query camera failed");
    if (0 == (vd_info->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        debug_msg("video capture not supported.\n");
        return -1;
    }

    /* set format */
    /* it would be safe to use 'v4l2_format' */
    memset(&vd_info->fmt, 0, sizeof(struct v4l2_format));
    vd_info->fmt.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd_info->fmt.fmt.pix.width	= vd_info->width;
    vd_info->fmt.fmt.pix.height	= vd_info->height;
    vd_info->fmt.fmt.pix.field	=V4L2_FIELD_ANY;
    vd_info->fmt.fmt.pix.pixelformat = vd_info->format;
    if (-1 == ioctl(vd_info->camfd, VIDIOC_S_FMT, &vd_info->fmt))
        unix_error_ret("unable to set format ");
    
    return 0;
}

static int request_buffer_normal(struct video_info* vd_info)
{
    int i = 0;
    /* request buffers */
    memset(&vd_info->rb, 0, sizeof(struct v4l2_requestbuffers));
    vd_info->rb.count	= NB_BUFFER; /* 4 buffers */
    vd_info->rb.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd_info->rb.memory	= V4L2_MEMORY_MMAP;
    if (-1 == ioctl(vd_info->camfd, VIDIOC_REQBUFS, &vd_info->rb))
        unix_error_ret("unable to allocte buffers");

    /* map the buffers(4 buffer) */
    for (i = 0; i < NB_BUFFER; i++)
    {
        memset(&vd_info->buf, 0, sizeof(struct v4l2_buffer));
        vd_info->buf.index	= i;
        vd_info->buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vd_info->buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == ioctl(vd_info->camfd, VIDIOC_QUERYBUF, &vd_info->buf))
            unix_error_ret("unable to query buffer");
        debug_msg("length: %u offset: %p\n",
          vd_info->buf.length, vd_info->buf.m.offset);
        /* map it, 0 means anywhere */
        vd_info->mem[i] =
            mmap(0, vd_info->buf.length, PROT_READ, MAP_SHARED,
                 vd_info->camfd, vd_info->buf.m.offset);
        /* MAP_FAILED = (void *)-1 */
        if (MAP_FAILED == vd_info->mem[i])
            unix_error_ret("unable to map buffer");
        debug_msg("buffer mapped in addr:%p.\n", vd_info->mem[i]);
    }

    /* queue the buffers */
    for (i = 0; i < NB_BUFFER; i++)
    {
        memset(&vd_info->buf, 0, sizeof(struct v4l2_buffer));
        vd_info->buf.index	= i;
        vd_info->buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vd_info->buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == ioctl(vd_info->camfd, VIDIOC_QBUF, &vd_info->buf))
            unix_error_ret("unable to queue the buffers");
    }
    
    vd_info->mem_mapped = 1;
    return 0;
}

#ifdef PLATFORM_RK30
extern int request_buffer_rk30(struct video_info* vd_info);
#endif

/** 
 * v4l2_init - 以指定参数初始化摄像头，该函数包括了打开摄像头
 * 
 * @param vd_info 摄像头信息结构体
 * @param format  摄像头支持的格式，如V4L2_PIX_FMT_YUYV
 * @param width   图像宽，如640
 * @param height  图像高，如480
 * 
 * @return 成功初始化则返回0，否则返回-1，并提示出错信息
 */
int v4l2_init(struct video_info* vd_info)
{
    int ret = 0;
    
    vd_info->is_quit = 0;
    
    ret = malloc_userdata(vd_info);
    if (ret < 0)
        unix_error_ret("malloc_userdata failed");
    
    ret = v4l2_open(vd_info);
    if (ret < 0)
        unix_error_ret("v4l2_open failed");

    ret = setformat(vd_info);
    if (ret < 0)
        unix_error_ret("setformat failed");

    
#ifdef PLATFORM_RK30
    if (vd_info->driver_type == V4L2_DRIVER_UVC) // UVC驱动使用普通的mmap // || vd_info->format == V4L2_PIX_FMT_MJPEG)
    {
        ret = request_buffer_normal(vd_info);
        if (ret < 0)
            unix_error_ret("request_buffer_normal failed");
    }
    else
    {
        ret = request_buffer_rk30(vd_info);
        if (ret < 0)
            unix_error_ret("request_buffer_rk30 failed");
    }

#else
    ret = request_buffer_normal(vd_info);
    if (ret < 0)
        unix_error_ret("request_buffer_normal failed");
#endif
    debug_msg("v4l2 init OK!\n");
    debug_msg("===============================\n\n");

    return 0;
}

/** 
 * v4l2_on - 启动摄像头采集数据
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功开始采集则返回0，否则返回-1，并提示出错信息
 */
int v4l2_on(struct video_info* vd_info)
{
    vd_info->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == ioctl(vd_info->camfd, VIDIOC_STREAMON, &vd_info->type))
        unix_error_ret("unable to start capture");
    vd_info->is_streaming = 1;
    debug_msg("stream on OK!!\n");
    debug_msg("===============================\n\n");

    return 0;
}

/** 
 * v4l2_off - 停止采集摄像头数据
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功则返回0，否则返回-1，并提示出错信息
 */
int v4l2_off(struct video_info* vd_info)
{
    vd_info->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vd_info->camfd, VIDIOC_STREAMOFF, &vd_info->type))
        unix_error_ret("unable to stop capture");
    vd_info->is_streaming = 0;
    debug_msg("stream off OK!\n");
    debug_msg("===============================\n\n");

    return 0;
}

/** 
 * v4l2_get_pic - 获取图片
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功则返回0，否则返回-1，并提示出错信息
 */
int v4l2_get_pic(struct video_info* vd_info)
{
    switch(vd_info->format){
    case V4L2_PIX_FMT_MJPEG:
        get_picture(vd_info->tmp_buffer,vd_info->buf.bytesused);
        break;
    case V4L2_PIX_FMT_YUYV:
        get_pictureYV2(vd_info->frame_buffer,vd_info->width,vd_info->height);
        break;
    default:
	break;
    }
    return 0;
}


/** 
 * v4l2_process - 摄像头数据处理
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功则返回0，否则返回-1，并提示出错信息
 * @note 如果摄像头为MJPEG格式，则可以直接将tmp_buff的数据写到文件，
         保存为jpg格式，即为一张图片。这里只是默认空处理
 */
int v4l2_process(struct video_info* vd_info)
{
    //v4l2_get_pic(vd_info);

    /* OK here! */
#if 0
    FILE* fp;
    char* file_name = "raw.jpg";
    fp = fopen(file_name, "w");
    if (NULL == fp)
        unix_error_ret("unable to open the file");
    fwrite(vd_info->tmp_buffer, vd_info->width, vd_info->height*2, fp);
    debug_msg("writing...\n");
    fclose(fp);
    #endif
    
    //debug_msg("default process func...\n");
    
    return 0;
}

int v4l2_set_processcb(v4l2_process_cb_ptr ptr)
{
    v4l2_processcb =  ptr;
    
    return 0;
}


/** 
 * v4l2_grab - 采集摄像头数据
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功则返回0，否则返回-1，并提示出错信息
 */
int v4l2_grab(struct video_info* vd_info)
{
#define HEADFRAME1 0xaf
    static int count = 0;
    int rett;
    
    if (!vd_info->is_streaming) /* if stream is off, start it */
    {
        if (v4l2_on(vd_info))  /* failed */
            goto err;
    }

    /* msg_out("start...\n"); */
    memset(&vd_info->buf, 0, sizeof(struct v4l2_buffer));
    vd_info->buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd_info->buf.memory = V4L2_MEMORY_MMAP;//V4L2_MEMORY_OVERLAY; //V4L2_MEMORY_MMAP;
    rett = ioctl(vd_info->camfd, VIDIOC_DQBUF, &vd_info->buf);
    //printf("DQBUF ret: %d: %d:%s\n", rett, errno, strerror(errno));
    /* get data from buffers */
    if (-1 == rett)
    {
        perror("unable to dequeue buffer: ");
        goto err;
    }

    switch (vd_info->format)
    {
    case V4L2_PIX_FMT_MJPEG:
        if (vd_info->buf.bytesused <= HEADFRAME1)
        {
            msg_out("ignore empty frame...\n");
            return 0;
        }
        /* we can save tmp_buff to a jpg file,just write it! */
        memcpy(vd_info->tmp_buffer, vd_info->mem[vd_info->buf.index],
               vd_info->buf.bytesused);
        //printf("decoding frame %d...\n", count);
        /* here decode MJPEG,so we can dispaly it */
        // tocheck：在一款摄像头，解码输出的是yuyv格式(非平面)
        if (jpeg_decode(&vd_info->frame_buffer, vd_info->tmp_buffer,
                        &vd_info->width, &vd_info->height) < 0 )
        {
            msg_out("decode jpeg error\n");
            goto err;
        }
        
        break;
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YYUV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
        if (vd_info->buf.bytesused > vd_info->frame_size_in)
            memcpy(vd_info->frame_buffer, vd_info->mem[vd_info->buf.index],
                   (size_t)vd_info->frame_size_in);
        else
            memcpy(vd_info->frame_buffer, vd_info->mem[vd_info->buf.index],
                   (size_t)vd_info->buf.bytesused);
        break;
    default:
        goto err;
        break;
    }

    /* here you can process the frame! */
    v4l2_processcb(vd_info);
    /* queue buffer again */
    if (-1 == ioctl(vd_info->camfd, VIDIOC_QBUF, &vd_info->buf))
    {
        fprintf(stderr,"requeue error\n");
        goto err;
    }

    //debug_msg("frame:%d\n", count++);
    // vd_info->buf.bytesused
    //debug_msg("frame size in: %d %d (%dKB)\n", vd_info->buf.bytesused, vd_info->frame_size_in, vd_info->frame_size_in>>10);

    count++;
    return 0;
err:
    vd_info->is_quit = 1;
    return -1;
}

/** 
 * v4l2_capture - 采集摄像头数据
 * 
 * @param vd_info 摄像头信息结构体
 * @note 该函数为测试函数，实际中未使用
 */
void v4l2_capture(struct video_info* vd_info)
{
    uint16 count = 10;
//    int ret;
    debug_msg("start capture\n");
    while (count-- > 0)
    {
        while (1)
        {
#if 0
            fd_set fds;
            struct timeval tv;
            FD_ZERO(&fds);
            FD_SET(vd_info->camfd, &fds);
            /* timeout */
            tv.tv_sec	= 2;
            tv.tv_usec	= 0;
            ret = select(vd_info->camfd, &fds, NULL, NULL, &tv);
            if (-1 == ret)      /* error */
            {
                if (EINTR == errno)
                    continue;
                fprintf(stderr,"select error\n");
                exit(EXIT_FAILURE);
            }
            if (0 == ret)       /* timeout */
            {
                fprintf(stderr,"select timeout\n");
                exit(EXIT_FAILURE);

            }
#endif
            /* good */
            if (v4l2_grab(vd_info))
                break;
            /* try again if errno = EAGAIN */
        }
    }
}

/** 
 * v4l2_get_capability - 获取摄像头capability
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功则返回0，否则返回－1，并提示出错信息
 */
int v4l2_get_capability(struct video_info* vd_info)
{
    memset(&vd_info->cap, 0, sizeof(struct v4l2_capability));
    if ( -1 == ioctl(vd_info->camfd, VIDIOC_QUERYCAP,&(vd_info->cap)))
        unix_error_ret("VIDIOC_QUERYCAP");
    
    if (!strncmp(vd_info->cap.driver, "uvc", 3))
    {
        vd_info->driver_type = V4L2_DRIVER_UVC; // UVC驱动
    }
    debug_msg("driver:%s\n",vd_info->cap.driver);
    debug_msg("card:%s\n",vd_info->cap.card);
    debug_msg("bus_info:%s\n",vd_info->cap.bus_info);
    debug_msg("version:%d\n",vd_info->cap.version);

	/*
     * 0x1:CAPTURE
     * 0x04000000:STREAMING
     */

    debug_msg("capability:%x\n",vd_info->cap.capabilities);
    
    v4l2_std_id std;
    int ret = 0;
    do {
        ret = ioctl(vd_info->camfd, VIDIOC_QUERYSTD, &std);
    } while (ret == -1 && errno == EAGAIN);
    printf("get std: 0x%x NTSC: 0x%x PAL: 0x%x\n", std, V4L2_STD_NTSC, V4L2_STD_PAL);
    switch (std){
        case V4L2_STD_NTSC:
            printf("NTSC.\n");
        break;
        case V4L2_STD_PAL:
            printf("PAL.\n");
        break;
    }

    debug_msg("query capability success\n");
    debug_msg("===============================\n\n");

    return 0;
}

static int enum_frame_sizes(int dev, int pixfmt, int idx)
{
    int ret;
    struct v4l2_frmsizeenum fsize;
    memset(&fsize, 0, sizeof(fsize));
    fsize.index = idx;
    fsize.pixel_format = pixfmt;
    while ((ret = ioctl(dev, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0)
    {
        //printf("%s: type: %d\n", __func__, fsize.type);
        if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
        {
            printf("{ discrete: width = %u, height = %u}\n",
                   fsize.discrete.width, fsize.discrete.height);

            // get frame rate
            // ret = enum_frame_intervals(dev,pixfmt,fsize.discrete.width,
            //    fsize.discrete.height);

            if (ret != 0)
            {
                printf("  Unable to enumerate frame sizes.\n");
            }
        }
        else if (fsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS)
        {
            printf("{ continuous: min { width = %u, height = %u } .. "
                   "max { width = %u, height = %u } }\n",
                   fsize.stepwise.min_width, 
                   fsize.stepwise.min_height,
                   fsize.stepwise.max_width, 
                   fsize.stepwise.max_height);
            printf("  Refusing to enumerate frame intervals.\n");
            break;
        }
        else if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
        {
            printf("{ stepwise: min { width = %u, height = %u } .. "
                   "max { width = %u, height = %u } / "
                   "stepsize { width = %u, height = %u } }\n",
                   fsize.stepwise.min_width, 
                   fsize.stepwise.min_height,
                   fsize.stepwise.max_width, 
                   fsize.stepwise.max_height,
                   fsize.stepwise.step_width,
                   fsize.stepwise.step_height);
            printf("  Refusing to enumerate frame intervals.\n");
            break;
        }
        else
        {
            printf("no type");
        }
        fsize.index++;
    }
    
    return 0;
    // 如果出错，则不打印信息
    if (ret != 0 || errno != EINVAL)
    {
        printf("ERROR enumerating frame sizes: %d\n", errno);
        return errno;
    }
    return 0;
}

int v4l2_enum_format(struct video_info* vd_info)
{
    struct v4l2_fmtdesc fmt_desc;
    int ret = -1;

    memset(&fmt_desc, 0, sizeof(struct v4l2_fmtdesc));
    fmt_desc.index=0;
    fmt_desc.type= V4L2_BUF_TYPE_VIDEO_CAPTURE; //V4L2_BUF_TYPE_VIDEO_CAPTURE; 

    printf("Support format:\n");
    while((ret = ioctl(vd_info->camfd, VIDIOC_ENUM_FMT, &fmt_desc)) != -1)
    {
        
        printf("%d.%s\n",fmt_desc.index,fmt_desc.description);
        printf("{ pixelformat = ''%c%c%c%c'', description = ''%s'' }\n", \
        fmt_desc.pixelformat & 0xFF, \
        (fmt_desc.pixelformat >> 8) & 0xFF, \
        (fmt_desc.pixelformat >> 16) & 0xFF, \
        (fmt_desc.pixelformat >> 24) & 0xFF,\
        fmt_desc.description);
        ret = enum_frame_sizes(vd_info->camfd, fmt_desc.pixelformat, fmt_desc.index);
        if (ret != 0)
            printf("Unable to enumerate frame sizes\n");
        
        fmt_desc.index++;
    }
    
    debug_msg("===============================\n\n");
    return 0;
}

/** 
 * v4l2_get_format - 获取摄像头格式
 * 
 * @param vd_info 摄像头信息结构体
 * 
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note
 * 1、在RF6.02中获得到格式为YUYV，但在FC10中却是MJPEG，未知原因
 * 2、v4l2_pix_format结构体成员pixelformat，在某些平台上却是pixelfmt，具体参考vedeodev2.h
 *    头文件，原因未找到
 * 3、获取可读性的格式的代码片段是参考网上资料的
 */
int v4l2_get_format(struct video_info* vd_info)
{
    /* see what format it has */
    /*
     * I have two camera,
     * one is MJPG,
     * the other is YUYV
     */
  /* NOTE:
   * YUYV in RedFlag6.02
   * MJPEG in Fedora10
   * what is the matter？
   * who tell me?
   *
   */
    memset(&vd_info->fmt,0,sizeof(struct v4l2_format));
    vd_info->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vd_info->camfd, VIDIOC_G_FMT, &(vd_info->fmt)))
        unix_error_ret("can not get format");
    /*
     * v4l2_pix_format has "pixelformat" or "pixelfmt"
     * see vedeodev2.h get a correct one!!
     * redflag with 2.6.28:pixelformat
     * fc10 with 2.6.33: pixelfmt
     * ??? glic???
     */
    vd_info->format = vd_info->fmt.fmt.pix.pixelformat;
    debug_msg("format(num):0x%x\n",vd_info->format);

	/*
     * pixel fmt is 32 bit
     * it is a four character code:abcd.dcba
     * so it is easy to get the code for a integer
     * see videodev2.h for more information
     */
    char code[5];
    int i=0;
    for (i=0; i<4; i++)
        code[i] = (vd_info->format & (0xff<<i*8)) >> i*8;
    code[4] = 0;
    debug_msg("fomat(human readable):%s\n",code);
    /* get more information here */
    vd_info->width 		= vd_info->fmt.fmt.pix.width;
    vd_info->height 		= vd_info->fmt.fmt.pix.height;
    
    // field参考v4l2_field
    vd_info->field 		= vd_info->fmt.fmt.pix.field;
    vd_info->bytes_per_line= vd_info->fmt.fmt.pix.bytesperline;
    vd_info->size_image 	= vd_info->fmt.fmt.pix.sizeimage;
    
    // colorspace参考枚举类型v4l2_colorspace 7表示JPEG 8表示RGB
    vd_info->color_space 	= vd_info->fmt.fmt.pix.colorspace;
    vd_info->priv		= vd_info->fmt.fmt.pix.priv;
    debug_msg("width:%d\n",vd_info->width);
    debug_msg("height:%d\n",vd_info->height);
    debug_msg("field:%d (1 if no fields)\n",vd_info->field);
    debug_msg("bytesperline:%d\n",vd_info->bytes_per_line);
    debug_msg("sizeimage:%d\n",vd_info->size_image);
    debug_msg("colorspace:%d(8 if RGB colorspace)\n",vd_info->color_space);
    debug_msg("private field:%d\n",vd_info->priv);
    
    
    debug_msg("get fomat OK!\n");
    debug_msg("===============================\n\n");

    return 0;
}

/** 
 * v4l2_set_format - 设置摄像头格式
 * 
 * @param vd_info 摄像头信息结构体
 * @param format  格式，如V4L2_PIX_FMT_YUYV
 * @param width   图像宽度
 * @param height  图像高度
 * 
 * @return 成功返回0，否则返回－1，并提示出错信息
 */
int v4l2_set_foramt(struct video_info* vd_info,uint32 format,
                    uint32 width, uint32 height)
{
    debug_msg("set format test\n");
    memset(&vd_info->fmt,0,sizeof(struct v4l2_format));
    vd_info->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd_info->fmt.fmt.pix.width	= width;
    vd_info->fmt.fmt.pix.height	= height;
    vd_info->fmt.fmt.pix.pixelformat= format;
    vd_info->fmt.fmt.pix.field	= V4L2_FIELD_ANY;

    if (-1 == ioctl(vd_info->camfd, VIDIOC_S_FMT, &(vd_info->fmt)))
        unix_error_ret("unable to set format");
    debug_msg("set format success\n");

    return 0;
}
