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
 *         输入\p ./capture即可运行程序，在运行程序之前，请确保插入USB摄像头，并查看\p /dev 目录下
 *         是否生成\p video 设备文件(使用命令<tt>ls -l /dev | grep video</tt>)，经测试，在PC中，/dev/video文件
 *         是摄像头设备的链接文件名称，程序中即使用这个文件名。\n
 *         程序测试环境如下：\n
 *         虚拟机fc10 \n
 *         [latelee@latelee camera-pc-utf]$ uname -a \n
 *         Linux latelee.latelee.org 2.6.27.5-117.fc10.i686 #1 SMP Tue Nov 18 12:19:59 EST 2008 i686 i686 i386 GNU/Linux \n
 *         [latelee@latelee camera-pc-utf]$ rpm -q "SDL" \n
 *         SDL-1.2.13-6.fc10.i386 \n
 *         [latelee@latelee camera-pc-utf]$ rpm -q "glibc" \n
 *         glibc-2.9-2.i686 \n
 *         测试图示：
 *         @image html pic-640x480.png "租房一角(640x480)"
 *         @image html pic-1024x768.png "指定范围太大，失败(1024x768)"
 *
 */

#include <signal.h>
#include "v4l2uvc.h"
 
#ifdef CAPTURE_SDL
 
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL_timer.h>
#include <SDL/SDL_syswm.h>

/** SDL 视频标志 */
static Uint32 SDL_VIDEO_Flags =
    SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_RESIZABLE;

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
 * display - 使用SDL显示图像
 *
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note 本函数涉及许多SDL库的类型及函数
 */
static int display(void)
{

    SDL_Surface *pscreen = NULL;
    SDL_Overlay *overlay = NULL;
    SDL_Rect drect;
    SDL_Event sdlevent;
    SDL_mutex *affmutex = NULL;
    unsigned char frmrate;
    unsigned char *p = NULL;
    uint32 currtime;
    uint32 lasttime;
    char* status = NULL;

    /************* Test SDL capabilities ************/
    /* memory leak here */

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
	fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
	exit(1);
    }

    /* it need to alloc space! */
    vd_info = (struct video_info *) calloc(1, sizeof(struct video_info));

    /* init the camera,you can change the last three params!!! */
    if (v4l2_init(vd_info, V4L2_PIX_FMT_YUYV, 640,480) < 0)
	return EXIT_FAILURE;

    pscreen =
	SDL_SetVideoMode(vd_info->width, vd_info->height, 0,
			 SDL_VIDEO_Flags);

    overlay =
	SDL_CreateYUVOverlay(vd_info->width, vd_info->height,
			     SDL_YUY2_OVERLAY, pscreen);
    /* here?? */
    p = (unsigned char *) overlay->pixels[0];
    drect.x = 0;
    drect.y = 0;
    drect.w = pscreen->w;
    drect.h = pscreen->h;

    lasttime = SDL_GetTicks();

    affmutex = SDL_CreateMutex();
    /* big loop */
    while (!vd_info->is_quit)
    {
	while (SDL_PollEvent(&sdlevent))
	{
	    if (sdlevent.type == SDL_QUIT)
	    {
		vd_info->is_quit = 1;
		break;
	    }
	}
	currtime = SDL_GetTicks();
	if (currtime - lasttime > 0)
	{
	    frmrate = 1000/(currtime - lasttime);
	}
	lasttime = currtime;

	if (v4l2_readframe(vd_info) < 0)
	{
	    printf("Error grabbing \n");
	    break;
	}

	SDL_LockYUVOverlay(overlay);
	/* frame_buffer to p */
	memcpy(p, vd_info->frame_buffer,
	       vd_info->width * (vd_info->height) * 2);
	SDL_UnlockYUVOverlay(overlay);

	SDL_DisplayYUVOverlay(overlay, &drect); /* dispaly it */
	status = (char *)calloc(1, 20*sizeof(char));
	sprintf(status, "come on fps:%d",frmrate);
	SDL_WM_SetCaption(status, NULL);

	SDL_Delay(10);
    }

    SDL_DestroyMutex(affmutex);
    SDL_FreeYUVOverlay(overlay);

    v4l2_close(vd_info);
    free(status);
    free(vd_info);

    printf("Clean Up done Quit \n");
    SDL_Quit();
    return 0;
}

/**
 * capture_sdl - SDL捕获显示主函数
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note 测试过程 \n
 * 1、先获取摄像头相关信息；
 * 2、等待用户输入回车，
 * 3、回车后，采集数据并显示，按Ctrl+c中断，或按SDL窗口关闭程序，否则一直显示
 */
int capture_sdl(int argc, char* argv[])
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
int capture_sdl(int argc, char* argv[])
{
    msg_out("sdl not available here, exiting..\n");
    return 0;
}
#endif
