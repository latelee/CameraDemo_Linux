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
#include <strings.h>
#include <ctype.h>
#include <signal.h>
#include <linux/fb.h>

#include "v4l2uvc.h"
#include "utils.h"

#include "fb_utils.h"
#include "yuv2rgb.h"


/** 摄像头信息结构体 */
static struct video_info* vd_info = NULL;
static int fps = 0;
static int g_fmt = -1;
static char g_fmt_name[16] = {0};
static int g_width = -1;
static int g_height = -1;
static char videodevice[16] = {0};
static enum v4l2_driver_type g_driver_type = V4L2_DRIVER_UNKNOWN;

enum SAVE_FILE_T{
    SVAETYPE_YUV = 0,
    SVAETYPE_JPG,
};

static int g_save_type = -1;
static FILE* fp = NULL;
static char file_name[16] = "raw.yuv";

static int g_need_display = 0;

int g_debug;

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
    fb_release();
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

    // 用户没有指定格式/分辨率，则从获取到的值中拿
    if (g_fmt == -1)
        g_fmt = vd_info->format;
    if (g_width == -1)
        g_width = vd_info->width;
    if (g_height == -1)
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
    
    if (g_save_type == SVAETYPE_YUV)
    {
        sprintf(file_name, "raw.yuv");
        if (init == 0)
        {
            fp = fopen(file_name, "w");
            init = 1;
            if (NULL == fp)
                unix_error_ret("unable to open the file");
        }

        fwrite(vd_info->frame_buffer, 1, vd_info->frame_size_in, fp);
        if (0 && fp)
        {
            fclose(fp);
            fp = NULL;
        }
    }
    // JPEG
    else if (g_save_type == SVAETYPE_JPG)
    {
        sprintf(file_name, "%d.jpg", cnt);

        fp = fopen(file_name, "w");
        if (NULL == fp)
            unix_error_ret("unable to open the file");

        fwrite(vd_info->tmp_buffer, 1, vd_info->buf.bytesused, fp);
        //fwrite(vd_info->tmp_buffer, 1, g_width*g_height*3/2, fp);
        
        
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
    }

    debug_msg("writing frame %d...\n", cnt);
    cnt++;
    
    return 0;
}

static int my_display_process(struct video_info* vd_info)
{
    static unsigned char* rgb_buffer = NULL;
    unsigned char* tmp_p = NULL;
    int rgb_size = 0;
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
    
    width = vd_info->width;
    height = vd_info->height;
    rgb_size = width*height*3;
    
    if (rgb_buffer == NULL)
    {
        rgb_buffer = (unsigned char*)malloc(rgb_size);
        if (rgb_buffer == NULL)
        {
            return -1;
        }
    }
    // 更改格式
    if (vd_info->frame_format == V4L2_PIX_FMT_YUYV)
    {
        yuv_to_rgb24(FMT_YUYV, vd_info->frame_buffer, rgb_buffer, width, height);
    }
    else if (vd_info->frame_format == V4L2_PIX_FMT_NV12)
    {
        yuv_to_rgb24(FMT_NV12, vd_info->frame_buffer, rgb_buffer, width, height);
    }
    else if (vd_info->frame_format == V4L2_PIX_FMT_NV21)
    {
        yuv_to_rgb24(FMT_NV21, vd_info->frame_buffer, rgb_buffer, width, height);
    }
    else if (vd_info->frame_format == V4L2_PIX_FMT_NV16)
    {
        yuv_to_rgb24(FMT_NV16, vd_info->frame_buffer, rgb_buffer, width, height);
    }
    else if (vd_info->frame_format == V4L2_PIX_FMT_NV61)
    {
        yuv_to_rgb24(FMT_NV61, vd_info->frame_buffer, rgb_buffer, width, height);
    }

#if 0

    //printf("video: %dx%d fb: %dx%d\n", width, height, fb->width, fb->height);
    tmp_pp = rgb16_buffer;
    while (y < height)
    {
        unsigned short  color;
        tmp_p = rgb_buffer + y*width*3;
        for (x=0; x < width; x++)
        {
            // LCD为rgb565格式，所以要转换
            color = make16color(tmp_p[x*3], tmp_p[x*3+1], tmp_p[x*3+2]);
            *((unsigned short*)tmp_pp) = color;
            tmp_pp += 2;
		}
        y++;
    }
    memcpy(fb->fbmem, rgb16_buffer, width*height);
    

#else
    // TODO：这里使用逐个像素描点，需要找个好的方法
    while (y < height)
    {
        unsigned short  color;
        tmp_p = rgb_buffer + y*width*3;
        for (x=0; x < width; x++)
        {
            // LCD为rgb565格式，所以要转换
            color = make16color(tmp_p[x*3], tmp_p[x*3+1], tmp_p[x*3+2]);
            fb_pixel(x, y, color); 
		}
        y++;
    }
#endif

    return 0;
}

static int my_v4l2_process(struct video_info* vd_info)
{
    if (g_need_display)
        my_display_process(vd_info);
    

    if (g_save_type != -1)
        my_save_file(vd_info);

    return 0;
}

void usage(char* name)
{
    printf("%s: A video capture tool for linux platform version: 1.1\n\n", name);
    printf("usage:\n");
    printf("%s -d [device] -s [format] -w [width] -h [height] -f [save file] --fb(display to framebuffer)\n", name);
    printf("\t -d\tvideo device, eg:/dev/video0\n");
    printf("\t -s\tvideo format, eg:[mjpeg|yuyv|nv12|nv21|nv16|nv61]\n");
    printf("\t -w\tvideo width, eg:480\n");
    printf("\t -h\tvideo height, eg:320\n");
    printf("\t -f\tsave file(yuv or jpg file), eg:[yuv|jpg]\n");
    printf("\t --fb\tdisplay video to frame buffer device\n");
    printf("\t --help\t show help info\n");
    printf("default: %s -d /dev/video0 -w 480 -h 320\n", name);
    
    exit(0);
}


int parsecmd(int argc, char *argv[])
{
    int i = 0;

    strcpy(videodevice, "/dev/video0");

    if (argc > 1) {
        for (i = 1; i < argc; i++)
        {
            if ((strcmp(argv[i], "-D")==0) || (strcmp(argv[i], "--debug")==0))
            {
                g_debug = 1;
            }
        }
    }
    
    // 
    /* parse parameters, maybe not the best way but... */
    for (i = 1; i < argc; i++)
    {
        if (g_debug)
            printf("arg %d: \"%s\"\n",i,argv[i]);
        // help
        if (strcmp(argv[i],"--help")==0)
        {
            usage(argv[0]);
            return 0;
        }
        // debug
        else if ((strcmp(argv[i],"-D")==0) || (strcmp(argv[i],"--debug")==0))
        {
            g_debug=1;
        } 
        // string
        else if ((strcmp(argv[i],"-d")==0))
        {
            if (i+1<argc)
            {
                strncpy(videodevice, argv[i+1], 16);
                if (g_debug)
                    printf("Used videodevice: %s\n", videodevice);
                i++;
                continue;
            } else {
                printf("Error: videodevice for -d missing.\n");
                return 1;
            }
        }
        else if ((strcmp(argv[i],"-s")==0))
        {
            if (i+1<argc)
            {
                strncpy(g_fmt_name, argv[i+1], 16);
                if (strcasecmp(g_fmt_name, "nv12") == 0)
                    g_fmt = V4L2_PIX_FMT_NV12;
                else if (strcasecmp(g_fmt_name, "nv12") == 0)
                    g_fmt = V4L2_PIX_FMT_NV12;
                else if (strcasecmp(g_fmt_name, "nv21") == 0)
                    g_fmt = V4L2_PIX_FMT_NV21;
                else if (strcasecmp(g_fmt_name, "nv16") == 0)
                {
                    g_fmt = V4L2_PIX_FMT_NV16;
                }
                else if (strcasecmp(g_fmt_name, "nv61") == 0)
                    g_fmt = V4L2_PIX_FMT_NV61;
                else if (strcasecmp(g_fmt_name, "mjpeg") == 0)
                    g_fmt = V4L2_PIX_FMT_MJPEG;
                else if (strcasecmp(g_fmt_name, "yuyv") == 0)
                {
                    g_fmt = V4L2_PIX_FMT_YUYV;
                }
                // to add....
                
                if (g_debug)
                    printf("Used format: %s(0x%x)\n", g_fmt_name, g_fmt);
                i++;
                continue;
            }
        }
        else if ((strcmp(argv[i],"-f")==0))
        {
            if (i+1<argc)
            {
                if (strcasecmp(argv[i+1], "yuv") == 0)
                    g_save_type = SVAETYPE_YUV;
                else if (strcasecmp(argv[i+1], "jpg") == 0)
                    g_save_type = SVAETYPE_JPG;
                // to add....
                
                if (g_debug)
                    printf("Used format: %s(0x%x)\n", argv[i+1], g_save_type);
                i++;
                continue;
            }
        }
        // number
        else if ((strcmp(argv[i],"-w")==0) || (strcmp(argv[i],"--width"))==0)
        {
            if (i+1<argc && isdigit(argv[i+1][0])) {
                g_width = atoi(argv[i+1]);
                if (g_debug)
                    printf("width: %d\n", g_width);
                i++;
                continue;
            }
        }
        else if ((strcmp(argv[i],"-h")==0) || (strcmp(argv[i],"--height"))==0)
        {
            if (i+1<argc && isdigit(argv[i+1][0])) {
                g_height = atoi(argv[i+1]);
                if (g_debug)
                    printf("height: %d\n", g_height);
                i++;
                continue;
            }
        }
        // only one
        else if (strcmp(argv[i],"--fb")==0)
        {
            g_need_display = 1;
        }
        else
        {
            printf("Unknown parameter \"%s\". Use -h for help.\n",argv[i]);
            return 1;
        }
    }
    
    return 0;
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
    parsecmd(argc, argv);
    
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
    vd_info->width	= g_width; //g_width; // 640
    vd_info->height	= g_height;//g_height; //480;
    vd_info->driver_type = g_driver_type;

    
    printf("====info video fmt: 0x%x res: %dx%d driver type: %d\n", 
            vd_info->format, vd_info->width, vd_info->height, vd_info->driver_type);
            
    //v4l2_set_processcb(my_v4l2_process);
    
    if (v4l2_init(vd_info) < 0)
        return -1;
    
    if (g_need_display)
    {
        fb_init();	// 标准framebuffer显示
    }
    
    while (1)
    {
        if (v4l2_readframe(vd_info) < 0)
        {
            printf("Error grabbing will continue\n");
            //break;
        }
        my_v4l2_process(vd_info);
        fps++;
    }
    
    v4l2_close(vd_info);

    free(vd_info);
    
    return 0;
}