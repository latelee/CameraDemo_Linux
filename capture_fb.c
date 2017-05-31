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
#include <linux/fb.h>

#include "v4l2uvc.h"
#include "utils.h"

/** 摄像头信息结构体 */
static struct video_info* vd_info = NULL;
static int fps = 0;
static int g_fmt = 0;
static int g_width = 0;
static int g_height = 0;

static enum v4l2_driver_type g_driver_type = V4L2_DRIVER_UNKNOWN;

static FILE* fp = NULL;
static char file_name[16] = "raw.yuv";

static int g_dispfd =-1;

#define DISP_DEV_NAME    "/dev/graphics/fb0"

#define FBIOSET_ENABLE			0x5019	

#define  FB_NONSTAND 0x20   // 这个实际是显示屏的格式，0x20表示VN12 0x10表示NV16

#define RK_FBIOSET_CONFIG_DONE		0x4628
#define RK_FBIOPUT_COLOR_KEY_CFG	0x4626
struct color_key_cfg {
	unsigned int win0_color_key_cfg;
	unsigned int win1_color_key_cfg;
	unsigned int win2_color_key_cfg;
};

// 图像显示的框对应于屏的偏移量
static int cory = 0;
static int corx = 0;

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
    
    if (g_dispfd > 0)
    {
		int disable = 0;
		printf("Close disp\n");
		ioctl(g_dispfd, FBIOSET_ENABLE,&disable);
		close(g_dispfd);
		g_dispfd = -1;
	}
    
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
    
    if (v4l2_enum_format(vd_info) < 0)
        exit(1);
    if (v4l2_get_format(vd_info) < 0)
		exit(1);
    if (vd_info->is_streaming)  /* stop if it is still capturing */
        v4l2_off(vd_info);

    g_fmt = vd_info->format;
    g_width = vd_info->width;
    g_height = vd_info->height;
    g_driver_type = vd_info->driver_type;
    
    
    if (vd_info->frame_buffer)
	free(vd_info->frame_buffer);
    if (vd_info->tmp_buffer)
	free(vd_info->tmp_buffer);
    vd_info->frame_buffer = NULL;
    
    v4l2_close(vd_info);

    free(vd_info);

    return 0;
}

// todo：由用户指定保存为什么格式
static int my_save_file(struct video_info* vd_info)
{
    static int init = 0;
    static int cnt = 0;
    
    
    // JPEG
    if (vd_info->format == V4L2_PIX_FMT_MJPEG)
    {
        sprintf(file_name, "%d.jpg", cnt);

        fp = fopen(file_name, "w");
        if (NULL == fp)
            unix_error_ret("unable to open the file");

        //fwrite(vd_info->tmp_buffer, 1, vd_info->buf.bytesused, fp);
        fwrite(vd_info->tmp_buffer, 1, g_width*g_height*3/2, fp);
        
        
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
    }
    // 保存为yuv格式文件
    else
    {
        //strcpy(file_name, "raw.yuv");
        sprintf(file_name, "%d.yuv", cnt);
        if (0 || init == 0)
        {
            fp = fopen(file_name, "w");
            init = 1;
            if (NULL == fp)
                unix_error_ret("unable to open the file");
        }

        fwrite(/*vd_info->frame_buffer*/vd_info->mem[vd_info->buf.index], 1, vd_info->frame_size_in, fp);
        if (0 && fp)
        {
            fclose(fp);
            fp = NULL;
        }
        
    }
    debug_msg("writing %d...\n", cnt);
    cnt++;
    
    return 0;
}

static int my_display_process(struct video_info* vd_info)
{
    int data[2];
	struct fb_var_screeninfo var;
    
    // 注：根据fb要求，这里指定的是buffer的offset。由内核fb驱动读取对应的数据
    // 数据格式为yuv420sp
    
    //printf("put addr %p to kernel idx: %d.\n", vd_info->buf.m.offset, vd_info->buf.index);
    // tmp...
    
    //yuyv2yuv420sp(vd_info->tmp_buffer, vd_info->frame_buffer, g_width, g_height);
    //my_save_file(vd_info);
    
    data[0] = (int)vd_info->buf.m.offset;
    data[1] = (int)(data[0] + g_width * g_height);

    if (ioctl(g_dispfd, 0x5002, data) == -1)
    {
       printf("%s ioctl fb1 queuebuf fail!\n",__FUNCTION__);
       return -1;
    }

    if (ioctl(g_dispfd, FBIOGET_VSCREENINFO, &var) == -1)
    {
        printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
        return -1;
    }

    var.xres_virtual = g_width;	//win0 memery x size
    var.yres_virtual = g_height;	 //win0 memery y size
    var.xoffset = 0;   //win0 start x in memery
    var.yoffset = 0;   //win0 start y in memery
    var.nonstd = ((cory<<20)&0xfff00000) + (( corx<<8)&0xfff00) +FB_NONSTAND;
    var.grayscale = ((g_height<<20)&0xfff00000) + (( g_width<<8)&0xfff00) + 0;   //win0 xsize & ysize
    var.xres = g_width;	 //win0 show x size
    var.yres = g_height;	 //win0 show y size
    var.bits_per_pixel = 16;
    var.activate = FB_ACTIVATE_FORCE;
    if (ioctl(g_dispfd, FBIOPUT_VSCREENINFO, &var) == -1)
    {
        printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
        return -1;
    }
    if (ioctl(g_dispfd,RK_FBIOSET_CONFIG_DONE, NULL) < 0)
    {
        perror("set config done failed");
    }
    
    return 0;
}
static int my_v4l2_process(struct video_info* vd_info)
{
    //debug_msg("my process....\n");
	

    //return 0;
    
    my_display_process(vd_info);
    
    #if 0
    // tmp...
    int i = 0;
    char* ptr = vd_info->tmp_buffer;
    ptr = vd_info->frame_buffer;
    if (ptr)
    {
        for (i = 1; i < 16*3 + 1; i++)
        {
            printf("%x ", ptr[i]);
            if (i%16 == 0) printf("\n");
        }
    }
    
    
    my_save_file(vd_info);
    return 0;
    #endif
    
    return 0;
}

int create_display(int corx ,int cory,int preview_w,int preview_h)
{
	int err = 0;
	struct fb_var_screeninfo var;
	unsigned int panelsize[2];
	struct color_key_cfg clr_key_cfg;

	if(g_dispfd !=-1)
		goto exit;
	g_dispfd = open(DISP_DEV_NAME,O_RDWR, 0);
	if (g_dispfd < 0) {
		printf("%s Could not open display device\n",__FUNCTION__);
		err = -1;
		goto exit;
	}
	if(ioctl(g_dispfd, 0x5001, panelsize) < 0)
	{
		printf("%s Failed to get panel size\n",__FUNCTION__);
		err = -1;
		goto exit1;
	}
	if(panelsize[0] == 0 || panelsize[1] ==0)
	{
		panelsize[0] = preview_w;
		panelsize[1] = preview_h;
	}

	if (ioctl(g_dispfd, FBIOGET_VSCREENINFO, &var) == -1) {
		printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
		err = -1;
		goto exit;
	}
	printf("preview_w = %d,preview_h =%d,panelsize[1] = %d,panelsize[0] = %d\n",preview_w,preview_h,panelsize[1],panelsize[0]);
	//var.xres_virtual = preview_w;	//win0 memery x size
	//var.yres_virtual = preview_h;	 //win0 memery y size
	var.xoffset = 0;   //win0 start x in memery
	var.yoffset = 0;   //win0 start y in memery
	var.nonstd = ((cory<<20)&0xfff00000) + ((corx<<8)&0xfff00) +FB_NONSTAND; //win0 ypos & xpos & format (ypos<<20 + xpos<<8 + format)
	var.grayscale = ((preview_h<<20)&0xfff00000) + (( preview_w<<8)&0xfff00) + 0;	//win0 xsize & ysize
	var.xres = preview_w;	 //win0 show x size
	var.yres = preview_h;	 //win0 show y size
	var.bits_per_pixel = 16;
	var.activate = FB_ACTIVATE_FORCE;
	
	printf("var.nonstd=%d, var.grayscale=%d....\n",var.nonstd,var.grayscale);  
	printf("var.xres=%d, var.yres=%d....\n",var.xres,var.yres); 
	
	if (ioctl(g_dispfd, FBIOPUT_VSCREENINFO, &var) == -1) {
		printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
		err = -1;
		goto exit;
	}
	
	clr_key_cfg.win0_color_key_cfg = 0;		//win0 color key disable
	clr_key_cfg.win1_color_key_cfg = 0x01000000; 	// win1 color key enable
	clr_key_cfg.win2_color_key_cfg = 0;  
	if (ioctl(g_dispfd,RK_FBIOPUT_COLOR_KEY_CFG, &clr_key_cfg) == -1)
    {
        printf("%s set fb color key failed!\n",__FUNCTION__);
        err = -1;
    }

	return 0;
exit1:
	if (g_dispfd > 0)
	{
		close(g_dispfd);
		g_dispfd = -1;
	}
exit:
	return err;
}


/**
 * capture_fb - 采集并显示在framebuffer上
 *
 * @return 成功返回0，否则返回－1，并提示出错信息
 * @note 
 * 1、先获取摄像头相关信息；
 * 
 */
int capture_fb(int argc, char* argv[])
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
    
    vd_info->format	= g_fmt; //V4L2_PIX_FMT_YUYV; //g_fmt; //g_fmt; //V4L2_PIX_FMT_NV21;//V4L2_PIX_FMT_MJPEG; //V4L2_PIX_FMT_NV12;
    vd_info->width	= 640; //g_width; // 640
    vd_info->height	= 480;//g_height; //480;
    vd_info->driver_type = g_driver_type;

    
    printf("====info video fmt: 0x%x res: %dx%d driver type: %d\n", 
            vd_info->format, vd_info->width, vd_info->height, vd_info->driver_type);
            
    v4l2_set_processcb(my_v4l2_process);
    
    if (v4l2_init(vd_info) < 0)
        return -1;
    
    create_display(corx, cory, vd_info->width, vd_info->height);
    
    while (1)
    {
        if (v4l2_grab(vd_info) < 0)
        {
            printf("Error grabbing will continue\n");
            //break;
        }
        
        fps++;
    }
    
    v4l2_close(vd_info);

    free(vd_info);
    
    return 0;
}