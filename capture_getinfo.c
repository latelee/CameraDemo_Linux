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
 * capture_v4l2getinfo - 最简单的测试函数
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note 
 * 1、只获取摄像头相关信息；
 * 
 */
int capture_v4l2getinfo(int argc, char* argv[])
{
    char videodevice[16] = {0};
    strcpy(videodevice, "/dev/video0");
    if (argc == 2)
    {
        strcpy(videodevice, argv[1]);
    }
    printf("willl open %s\n", videodevice);

    get_info(videodevice);
    
    return 0;
}