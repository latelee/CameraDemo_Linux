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
 * @file   capture_sdl.c
 * @author Late Lee
 * @date   Sat May 29 2010
 *
 * @brief  none
 *
 * @test
 *         在程序目录中直接输入\p make 即可编译生成\p capture 可执行文件。\n
 *
 */

#include <signal.h>
#include "v4l2uvc.h"

/** 摄像头信息结构体 */
struct video_info* vd_info = NULL;
int fps = 0;
int g_fmt = 0;
int g_width = 0;
int g_height = 0;

FILE* fp = NULL;
char* file_name[16] = {0}; // = "raw.yuv";

/**
 * sig_int - 信号处理函数
 *
 * @param signum 参数，可不用理会
 * @note 本程序中只处理Ctrl+c中断(SIGINT)
 */
static void sig_int(int signum)
{
    debug_msg("\ncatch a SIGINT signal, you may be press Ctrl+C.\n");
    debug_msg("ready to quit\n");
    if (vd_info == NULL)
        goto end;

    vd_info->is_quit = 1;
    // stop....
    v4l2_close(vd_info);
    
end:
    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
    exit(0);
}

static void sig_alarm(int signum)
{
   printf("fps=%dfps\n",fps);
   fps = 0;

   alarm(1);
}

static int my_v4l2_process(struct video_info* vd_info)
{
    //debug_msg("my process....\n");
    
    
    //v4l2_get_pic(vd_info);
    
    //return 0;
    
    
    static int init = 0;
    static int cnt = 0;
    
    // 保存为yuv格式文件
    if (vd_info->format != V4L2_PIX_FMT_MJPEG)
    {
        strcpy(file_name, "ray.yuv");
        if (init == 0)
        {
            fp = fopen(file_name, "w");
            init = 1;
            if (NULL == fp)
                unix_error_ret("unable to open the file");
        }

        fwrite(vd_info->frame_buffer, 1, vd_info->frame_size_in, fp);
        debug_msg("writing %d...\n", cnt);
    }
    // JPEG
    else
    {
        sprintf(file_name, "%d.jpg", cnt++);

        fp = fopen(file_name, "w");
        if (NULL == fp)
            unix_error_ret("unable to open the file");

        fwrite(vd_info->tmp_buffer, 1, vd_info->buf.bytesused, fp);
        debug_msg("writing %d...\n", cnt);
        
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
    }

    
    return 0;
}

/**
 * get_info - 获取摄像头信息测试函数，由main函数调用
 *
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 */
static int get_info(char* videodevice)
{
    struct video_info* vd_info = (struct video_info *) calloc(1, sizeof(struct video_info));
    strcpy(vd_info->name, videodevice);
    
    if (v4l2_open(vd_info) <0)
		exit(1);
    if (v4l2_get_capability(vd_info) < 0)
		exit(1);
    if (v4l2_get_format(vd_info) < 0)
		exit(1);
    if (vd_info->is_streaming)  /* stop if it is still capturing */
        v4l2_off(vd_info);

    g_fmt = vd_info->format;
    g_width = vd_info->width;
    g_height = vd_info->height;
    
    if (vd_info->frame_buffer)
	free(vd_info->frame_buffer);
    if (vd_info->tmp_buffer)
	free(vd_info->tmp_buffer);
    vd_info->frame_buffer = NULL;
    
    v4l2_close(vd_info);

    free(vd_info);

    return 0;
}

/**
 * capture_v4l2simple - 最简单的测试函数
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note 
 * 1、先获取摄像头相关信息；
 * 
 */
int capture_v4l2simple(int argc, char* argv[])
{
    char videodevice[16] = {0};
    strcpy(videodevice, "/dev/video0");
    if (argc == 2)
    {
        strcpy(videodevice, argv[1]);
    }
    printf("willl open %s\n", videodevice);
    signal(SIGINT, sig_int);
    
    get_info(videodevice);

    // 在按回车之前捕获摄像头信息，之后继续后面的事
    msg_out("press enter key to capture !\n");
    getchar();

    signal(SIGALRM, sig_alarm);
	alarm(1);

    vd_info = (struct video_info *) calloc(1, sizeof(struct video_info));

    strcpy(vd_info->name, videodevice);
    
    vd_info->format	= g_fmt; //g_fmt; //V4L2_PIX_FMT_NV21;//V4L2_PIX_FMT_MJPEG; //V4L2_PIX_FMT_NV12;
    vd_info->width	= g_width; // 680
    vd_info->height	= g_height; //480;

    
    printf("got video fmt: 0x%x res: %dx%d\n", vd_info->format, vd_info->width, vd_info->height);
    v4l2_set_processcb(my_v4l2_process);
    
    if (v4l2_init(vd_info) < 0)
        return -1;
    
    while (1)
    {
        if (v4l2_grab(vd_info) < 0)
        {
            printf("Error grabbing \n");
            break;
        }
        
        fps++;
    }
    
    v4l2_close(vd_info);

    free(vd_info);
    
    return 0;
}