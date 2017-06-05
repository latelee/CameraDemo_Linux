/**
 * @file   fb_utils.c
 * @author Late Lee <www.latelee.org>
 * @date   2011.06
 * 
 * @brief
 * framebuffer utils(I take it from a small project named 'ripple'), 
 * test on my LCD(rgb565).
 *
 * @note 
		   1、如果需要显示图形界面或中英文字符，须与fb_graphic.c一起使用，初始化步骤如下：
		   	fb_init();	// 初始化frambuffer(即LCD)
			graphic_init();	// 初始化图形界面
			color_init(palette, NR_COLORS);	// 初始化颜色值(与图形相关函数最后一个参数
			                                   即为palette数组中的颜色值索引，须定义palette)
 * @log
 *     1. 添fb变量放到本文件中
       2. 添加几个刷新指定区域的函数(原来的刷新函数针对整个屏幕)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "fb_utils.h"

FRAMEBUFFER *fb = NULL;

int fb_init(void)
{
	static int fb_inited = 0;
	char *dev, *env;
	int y = 0, addr = 0;

	if (fb_inited)
		return 0;

	fb = malloc(sizeof(FRAMEBUFFER));
	if (!fb) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		goto out;
	}
    
    fb->fbmem = NULL;
    fb->fd = -1;

	env = getenv("FRAMEBUFFER");
	if (env)
		dev = env;
	else
		dev = DEFAULT_FRAMEBUFFER;
	if ((fb->fd = open(dev, O_RDWR)) == -1) {
		fprintf(stderr, "open %s: %s\n", dev, strerror(errno));
		goto out;
	}
	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->fbinfo)) {
		fprintf(stderr, "ioctl FBIOGET_VSCREENINFO: %s\n",
				strerror(errno));
		goto out;
	}
	fb->width  = fb->fbinfo.xres;
	fb->height = fb->fbinfo.yres;
	fb->bytes_per_pixel  = fb->fbinfo.bits_per_pixel/8;
	fb->fbsize = fb->width * fb->height * fb->bytes_per_pixel;
	fb->fbmem =
		mmap(NULL, fb->fbsize, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd,
			 0);
	if (fb->fbmem == MAP_FAILED) {
		fprintf(stderr, "mmap: %s\n", strerror(errno));
		goto out;
	}
	fb->fbclone = malloc(fb->fbsize);
	if (!fb->fbclone) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		goto out;
	}
	/**
	 * I assume fb->fbinfo.yres equals fb->fbinfo.yres_virtual
	 *
	 */
	/* pointer to every line(row?) */
	fb->line_addr = (unsigned char**)malloc (sizeof (unsigned int) * fb->height);
	for (y = 0; y < fb->height; y++, addr += fb->width * fb->bytes_per_pixel) {
		fb->line_addr[y] = fb->fbmem + addr;
	}
	fb_inited = 1;

	fb->init=fb_init;
	fb->release=fb_release;
	fb->snap=fb_snap;
	fb->refresh=fb_refresh;
	fb->refresh_region=fb_refresh_region;
	fb->blit=fb_blit;
	fb->convert=fb_convert;
	fb_clear();
    
    return 0;
out:
    if (fb)
    {
        if (fb->fd != -1) close(fb->fd);
        if (fb->fbmem != NULL) free(fb->fbmem);
        free(fb);
        fb = NULL;
        
    }
	return 0;
}

void fb_release(void)
{
	if (fb) {
		munmap(fb->fbmem, fb->fbsize);
		free(fb->line_addr);
		free(fb->fbclone);
		close(fb->fd);
		free(fb);
	}
}

void fb_snap(void)
{
	memcpy(fb->fbclone, fb->fbmem, fb->fbsize);
}

/**
 * fb_refresh - refresh the whole framebuffer(fb->fbsize)
 *
 */
void fb_refresh(void)
{
	memcpy(fb->fbmem, fb->fbclone, fb->fbsize);
}

/**
 * fb_refresh_region - Refresh some regions specified by @y and @num
 * @y: which line
 * @num: how many lines, eg 25*3(every character takes 25 lines)
 */
void fb_refresh_region(int y, int num)
{
	int tmp;
	tmp = fb->width * fb->bytes_per_pixel * num;
	memset(fb->line_addr[y], 0x00, tmp); /* still ok here */
	//memset(fb->fbclone, 0x0, tmp);
	//memcpy(fb->line_addr[y], fb->fbclone, tmp);
}

/**
 * fb_refresh_region_xy - Refresh the regions specified by (@x1, @y), (@x2, @y) and @num
 * @y: start line
 * @x1, @x2: end
 * @num: how many lines
 *
 * Why need fb->bytes_per_pixel?
 * --> 1 bytes?
 */
void fb_refresh_region_xy(int y, int x1, int x2, int num)
{
	int tmp;
	tmp = (x2-x1) * fb->bytes_per_pixel;
	while (num) {
		memset(fb->line_addr[y++] + x1*fb->bytes_per_pixel, 0x00, tmp);
		num--;
	}	
}

/**
 * fb_clear - Clear (or refresh) the whole framebuffer
 *
 * Just takes zero to it
 */
void fb_clear(void)
{
	memset(fb->fbclone, 0x0, fb->fbsize);
	memcpy(fb->fbmem, fb->fbclone, fb->fbsize);
}

/**
 * fb_pixel - Set pixel in (@x, @y) with @color
 * @x, @y: x and y
 * @color: color(which must be rgb565 in my LCD), 16 bits
 *
 * Why do not need fb->bytes_per_pixel?
 * --> Because @color takes 2 bytes
 */
void fb_pixel(int x, int y, int color)
{
	//*( (unsigned short*)fb->line_addr[y] + x*fb->bytes_per_pixel ) = color;
	*( (unsigned short*)fb->line_addr[y] + x ) = color;
}

/**
 *fb_rgb565 - make RGB888 to RGB565
 *@value: RGB888, eg 0xff0000(red) -> 0xf800(red)
 */
inline unsigned short fb_rgb565(int value)
{
	unsigned char red, green, blue;
	red = (value >> 16) & 0xff;
	green = (value >> 8) & 0xff;
	blue = value & 0xff;
	return ((((red >> 3) & 0x1f) << 11) |
			(((green >> 2) & 0x3f) << 5) | ((blue >> 3) & 0x1f));
}

inline unsigned char make8color(unsigned char r, unsigned char g,
								unsigned char b)
{
	return ((((r >> 5) & 0x07) << 5) |
			(((g >> 5) & 0x07) << 2) | ((b >> 6) & 0x03));
}

inline unsigned short make15color(unsigned char r, unsigned char g,
								  unsigned char b)
{
	return ((((r >> 3) & 0x1f) << 10) |
			(((g >> 3) & 0x1f) << 5) | ((b >> 3) & 0x1f));
}

inline unsigned short make16color(unsigned char r, unsigned char g, unsigned char b)
{
	return ((((r >> 3) & 0x1f) << 11) |
			(((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f));
}

void *fb_convert(unsigned char *rgbbuff, unsigned long count, int bpp)
{
	unsigned long i;
	void *fbbuff = NULL;
	unsigned char *c_fbbuff;
	unsigned short *s_fbbuff;
	unsigned int *i_fbbuff;

	switch (bpp) {

	case 8:
		c_fbbuff = (unsigned char *) malloc(count * sizeof(unsigned char));
		for (i = 0; i < count; i++)
			c_fbbuff[i] =
				make8color(rgbbuff[i * 3], rgbbuff[i * 3 + 1],
						   rgbbuff[i * 3 + 2]);
		fbbuff = (void *) c_fbbuff;
		break;
	case 15:
		s_fbbuff =
			(unsigned short *) malloc(count * sizeof(unsigned short));
		for (i = 0; i < count; i++)
			s_fbbuff[i] =
				make15color(rgbbuff[i * 3], rgbbuff[i * 3 + 1],
							rgbbuff[i * 3 + 2]);
		fbbuff = (void *) s_fbbuff;
		break;
	case 16:
		s_fbbuff =
			(unsigned short *) malloc(count * sizeof(unsigned short));
		for (i = 0; i < count; i++)
			s_fbbuff[i] =
				make16color(rgbbuff[i * 3], rgbbuff[i * 3 + 1],
							rgbbuff[i * 3 + 2]);
		fbbuff = (void *) s_fbbuff;
		break;
	case 24:
	case 32:
		i_fbbuff = (unsigned int *) malloc(count * sizeof(unsigned int));
		for (i = 0; i < count; i++)
			i_fbbuff[i] = ((rgbbuff[i * 3] << 16) & 0xFF0000) |
				((rgbbuff[i * 3 + 1] << 8) & 0xFF00) |
				(rgbbuff[i * 3 + 2] & 0xFF);
		fbbuff = (void *) i_fbbuff;
		break;
	default:
		fprintf(stderr, "Unsupported video mode! You've got: %dbpp\n",
				bpp);
		exit(1);
	}
	return fbbuff;
}

void fb_blit(unsigned char *image, int x, int y, int width, int height)
{
	int i, xc, yc;
	unsigned char *fbptr;
#if 10
	if (!fb)
		return;
	if (fb->fbinfo.xres == width && fb->fbinfo.yres == height) {
		memcpy(fb->fbmem, image, fb->fbsize);
	} else {
		fb_snap();
		xc = (width > fb->fbinfo.xres) ? fb->fbinfo.xres : width;
		yc = (height > fb->fbinfo.yres) ? fb->fbinfo.yres : height;
		fbptr =
			fb->fbclone + (y * fb->fbinfo.xres +
						   x) * fb->fbinfo.bits_per_pixel / 8;
		for (i = 0; i < yc;
			 i++, fbptr +=
			 fb->fbinfo.xres * fb->fbinfo.bits_per_pixel / 8, image +=
			 width * fb->fbinfo.bits_per_pixel / 8)
			memcpy(fbptr, image, xc * fb->fbinfo.bits_per_pixel / 8);
		memcpy(fb->fbmem, fb->fbclone, fb->fbsize);
	}
#endif
}
