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
 
#ifdef CAPTURE_FB


/** 摄像头信息结构体 */
struct video_info* vd_info = NULL;

/**
 * sig_int - 信号处理函数
 *
 * @param signum 参数，可不用理会
 * @note 本程序中只处理Ctrl+c中断(SIGINT)
 */
static void sig_int(int signum)
{
    debug_msg("\ncatch a SIGINT signal, you may be press Ctrl+C.\n");
    vd_info->is_quit = 1;
    debug_msg("ready to quit\n");

}

/**
 * get_info - 获取摄像头信息测试函数，由main函数调用
 *
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 */
static int get_info(void)
{
    vd_info = (struct video_info *) calloc(1, sizeof(struct video_info));
    if (v4l2_open(vd_info) <0)
		exit(1);
    if (v4l2_get_capability(vd_info) < 0)
		exit(1);
    if (v4l2_get_format(vd_info) < 0)
		exit(1);
    if (vd_info->is_streaming)  /* stop if it is still capturing */
	v4l2_off(vd_info);

    if (vd_info->frame_buffer)
	free(vd_info->frame_buffer);
    if (vd_info->tmp_buffer)
	free(vd_info->tmp_buffer);
    vd_info->frame_buffer = NULL;
    close(vd_info->camfd);

    free(vd_info);

    return 0;
}

/**
 * display - 使用FB显示图像
 *
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note todo
 */
static int display(void)
{
    return 0;
}

/**
 * capture_fb - 捕获摄像头并显示在FB设备上
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note 测试过程 \n
 * 1、先获取摄像头相关信息；
 * 2、等待用户输入回车，
 * 3、回车后，采集数据并显示，按Ctrl+c中断，或按SDL窗口关闭程序，否则一直显示
 */
int capture_fb(int argc, char* argv[])
{
    signal(SIGINT, sig_int);
    get_info();
    msg_out("I am ready!\n");
    msg_out("press enter key to capture and display!\n");
    getchar();

    display();
    return 0;
}
#else
int capture_fb(int argc, char* argv[])
{
    msg_out("framebuffer not available here, exiting..\n");
    return 0;
}
#endif
