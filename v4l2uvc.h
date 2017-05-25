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
 * @file   v4l2uvc.h
 * @author Late Lee <www.latelee.org>
 * @date   Tue Jan 18 2011
 * 
 * @brief
 * vedio capture for Vedio Surveillance System(VSS)
 * using v4l2 APIs
 *
 *
 */

#ifndef _V4L2UVC_H
#define _V4L2UVC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>          /* ioctl */
#include <sys/mman.h>           /* for MAP_SHARED etc */
#include <sys/select.h>         /* select,etc. */
#include <fcntl.h>              /* open, etc */
#include <errno.h>
#include <linux/videodev2.h>
#include "my-types.h"

/* for debug */
#define DEBUG
#include "debug-msg.h"

/** 缓冲区个数 */
#define NB_BUFFER 4

// 默认的
#define DEFAULT_VIDEO "/dev/video0"

/** 自定义的摄像头信息结构体 */
struct video_info
{
    char name[16];              /**< 摄像头文件，由调用者指定，否则使用默认值 */
    int camfd;                  /**< 摄像头文件描述符，由open系统调用指定 */
    struct v4l2_capability cap;	/**< 摄像头capability(属性) */
    struct v4l2_format fmt;	    /**<  摄像头格式，使用该结构体对摄像头进行设置 */
    struct v4l2_requestbuffers rb;	/**< 请求缓冲，一般不超过5个 */
    struct v4l2_buffer buf;	    /**< buffer */
    enum v4l2_buf_type type;	/**< 控制命令字？ */
    void* mem[NB_BUFFER];       /**< main buffers */
    int mem_mapped;             /**< 标志所有的mem是否均由mmap申请 */
    uint8* tmp_buffer;          /**< 临时缓冲区，针对MJPEG格式而设 */
    uint8* frame_buffer;        /**< 一帧图像缓冲区 */
    uint32 frame_size_in;	    /**< 一帧图像大小(如果是yuv422格式，大小为宽*高*2) */

    uint32 format;              /**< 摄像头支持的格式，如MJPEG、YUYV等 */
    int width;                  /**< 图像宽 */
    int height;	                /**< 图像高 */
    int is_streaming;           /**< 开始采集 */
    int is_quit;                /**< 退出显示 */

    enum v4l2_field field;
    uint32 bytes_per_line;
    uint32 size_image;
    enum v4l2_colorspace color_space;
    uint32 priv;
};

typedef int (*v4l2_process_cb_ptr)(struct video_info* vd_info);

// 设置用户级别的处理回调函数
int v4l2_set_processcb(v4l2_process_cb_ptr ptr);

int v4l2_open(struct video_info* vd_info);
int v4l2_close(struct video_info* vd_info);

int v4l2_init(struct video_info* vd_info);
int v4l2_on(struct video_info* vd_info);
int v4l2_off(struct video_info* vd_info);

int v4l2_grab(struct video_info* vd_info);

void v4l2_capture(struct video_info* vd_info);

int v4l2_process(struct video_info* vd_info);

int v4l2_get_pic(struct video_info* vd_info);

int v4l2_get_capability(struct video_info* vd_info);
int v4l2_get_format(struct video_info* vd_info);
int v4l2_set_foramt(struct video_info* vd_info,
                    uint32 width, uint32 height,uint32 format);

#endif  /* _V4L2UVC_H */
