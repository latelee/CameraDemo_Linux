/**
 * @file   fb_utils.h
 * @author Late Lee <www.latelee.org>
 * @date   2011.06
 * 
 * @brief
 * framebuffer utils(I take it from a small project named 'ripple'), 
 * test on my LCD(rgb565).
 *
 * @log
 *     1. 添fb变量放到对应的.c文件，本头文件只作声明
 */

#ifndef FB_UTILS_H
#define FB_UTILS_H

#include <linux/fb.h>


#define DEFAULT_FRAMEBUFFER     "/dev/fb0"

typedef struct FRAMEBUFFER {
	int fd;
	int width;		/* xres */
	int height;		/* yres */
	int bytes_per_pixel;	/* bits_per_pixel */
	unsigned int fbsize;	/* size(in byte) */
	unsigned char *fbmem;	/* memory address(which returned by mmap) */
	unsigned char *fbclone;
	unsigned char **line_addr;	/* line address */
	struct fb_var_screeninfo fbinfo;
	int (*init)(void);
	void (*release)(void);
	void (*snap)(void);
	void (*refresh)(void);
	void (*refresh_region)(int y, int num);
	void (*blit)(unsigned char *image, int x, int y, int width, int height);
	void *(*convert)(unsigned char *rgbbuff, unsigned long count, int bpp);
} FRAMEBUFFER;

extern FRAMEBUFFER *fb;

int  fb_init(void);
void fb_release(void);
void fb_snap(void);
void fb_refresh(void);
void fb_refresh_region(int y, int num);
void fb_refresh_region_xy(int y, int x1, int x2, int num);
void fb_clear(void);
void fb_blit(unsigned char *image, int x, int y, int width, int height);
void *fb_convert(unsigned char *rgbbuff, unsigned long count, int bpp);
void fb_pixel(int x, int y, int color);
unsigned short make16color(unsigned char r, unsigned char g, unsigned char b);
unsigned short fb_rgb565(int value); /* just for my LCD */

#endif /* FB_UTILS_H */
