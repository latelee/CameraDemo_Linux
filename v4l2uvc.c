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
    /* open it --no block */
    vd_info->camfd = open(device, O_RDWR | O_NONBLOCK, 0);
    if (vd_info->camfd < 0)
        unix_error_ret("can not open the device");
    debug_msg("open success!\n");
    debug_msg("===============================\n\n");

    return 0;
}

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
        free(vd_info->frame_buffer);
    if (vd_info->tmp_buffer)
        free(vd_info->tmp_buffer);
    vd_info->frame_buffer = NULL;
    /* it is a good thing to unmap! */
    for (i = 0; i < NB_BUFFER; i++)
    {
        if (-1 == munmap(vd_info->mem[i], vd_info->buf.length))
            unix_error_ret("munmap");
    }

    close(vd_info->camfd);
    debug_msg("close OK!\n");

    return 0;
}

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
int v4l2_init(struct video_info* vd_info, uint32 format,
              uint32 width, uint32 height)
{
    int i;
    /* initialize sth */
    vd_info->format	= format;
    vd_info->width	= width;
    vd_info->height	= height;
    vd_info->is_quit = 0;
    vd_info->frame_size_in = (vd_info->width * vd_info->height << 1);
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
    case V4L2_PIX_FMT_YUYV:
        vd_info->frame_buffer =
            (uint8 *)calloc(1,(size_t)vd_info->frame_size_in);
        if (vd_info->frame_buffer == NULL)
            unix_error_ret("unable alloc frame_buffer");
        break;
    default:
        msg_out("error!\n");
        return -1;
        break;
    }
    /* open it tag: try nonblock*/
    vd_info->camfd = open(device, O_RDWR | O_NONBLOCK, 0);
    if (vd_info->camfd < 0)
        unix_error_ret("can not open the device");

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
    vd_info->fmt.fmt.pix.width	= width;
    vd_info->fmt.fmt.pix.height	= height;
    vd_info->fmt.fmt.pix.field	=V4L2_FIELD_ANY;
    vd_info->fmt.fmt.pix.pixelformat = format;
    if (-1 == ioctl(vd_info->camfd, VIDIOC_S_FMT, &vd_info->fmt))
        unix_error_ret("unable to set format ");

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
        /*debug_msg("length: %u offset: %u\n",
          vd_info->buf.length, vd_info->buf.m.offset);*/
        /* map it, 0 means anywhere */
        vd_info->mem[i] =
            mmap(0, vd_info->buf.length, PROT_READ, MAP_SHARED,
                 vd_info->camfd, vd_info->buf.m.offset);
        /* MAP_FAILED = (void *)-1 */
        if (MAP_FAILED == vd_info->mem[i])
            unix_error_ret("unable to map buffer");
        // debug_msg("buffer mapped in addr:%p.\n", vd_info->mem[i]);
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
         保存为jpg格式，即为一张图片。
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
    return 0;
}

/** 
 * 
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
    if (!vd_info->is_streaming) /* if stream is off, start it */
    {
        if (v4l2_on(vd_info))  /* failed */
            goto err;
    }
    /* msg_out("start...\n"); */
    memset(&vd_info->buf, 0, sizeof(struct v4l2_buffer));
    vd_info->buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd_info->buf.memory = V4L2_MEMORY_MMAP;
    /* get data from buffers */
    if (-1 == ioctl(vd_info->camfd, VIDIOC_DQBUF, &vd_info->buf))
    {
        msg_out("unable to dequeue buffer\n");
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

        /* here decode MJPEG,so we can dispaly it */
        if (jpeg_decode(&vd_info->frame_buffer, vd_info->tmp_buffer,
                        &vd_info->width, &vd_info->height) < 0 )
        {
            msg_out("decode jpeg error\n");
            goto err;
        }
        break;
    case V4L2_PIX_FMT_YUYV:
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
    v4l2_process(vd_info);
    /* queue buffer again */
    if (-1 == ioctl(vd_info->camfd, VIDIOC_QBUF, &vd_info->buf))
    {
        fprintf(stderr,"requeue error\n");
        goto err;
    }

    debug_msg("frame:%d\n", count++);
    debug_msg("frame size in: %d KB\n", vd_info->frame_size_in>>10);

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
    debug_msg("driver:%s\n",vd_info->cap.driver);
    debug_msg("card:%s\n",vd_info->cap.card);
    debug_msg("bus_info:%s\n",vd_info->cap.bus_info);
    debug_msg("version:%d\n",vd_info->cap.version);

	/*
     * 0x1:CAPTURE
     * 0x04000000:STREAMING
     */

    debug_msg("capability:%x\n",vd_info->cap.capabilities);
    debug_msg("query capability success\n");
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
    debug_msg("format(num):%d\n",vd_info->format);

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
    vd_info->field 		= vd_info->fmt.fmt.pix.field;
    vd_info->bytes_per_line= vd_info->fmt.fmt.pix.bytesperline;
    vd_info->size_image 	= vd_info->fmt.fmt.pix.sizeimage;
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
