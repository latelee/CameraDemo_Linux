/**
代码使用p[0]这类的形式，是因为这类的表现方式便于分析格式
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "yuv2rgb.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static long U[256], V[256], Y1[256], Y2[256];

void init_yuv422p_table(void)
{
    int i;
    static int init = 0;
    if (init == 1) return;
    // Initialize table
    for (i = 0; i < 256; i++)
    {
        V[i]  = 15938 * i - 2221300;
        U[i]  = 20238 * i - 2771300;
        Y1[i] = 11644 * i;
        Y2[i] = 19837 * i - 311710;
    }

    init = 1;
}

/**
内存分布
                    w
            +--------------------+
            |Y0Y1Y2Y3...         |
            |...                 |   h
            |...                 |
            |                    |
            +--------------------+
            |U0U1      |
            |...       |   h
            |...       |
            |          |
            +----------+
            |V0V1      |
            |...       |  h
            |...       |
            |          |
            +----------+
                w/2
*/
void yuv422p_to_rgb24(YUV_TYPE type, unsigned char* yuv422p, unsigned char* rgb, int width, int height)
{
    int y, cb, cr;
    int r, g, b;
    int i = 0;
    unsigned char* p_y;
    unsigned char* p_u;
    unsigned char* p_v;
    unsigned char* p_rgb;

    p_y = yuv422p;
    p_u = p_y + width * height;
    p_v = p_u + width * height / 2;

    if (type == FMT_YV16)
    {
        p_v = p_y + width * height;
        p_u = p_u + width * height / 2;
    }
    p_rgb = rgb;

    init_yuv422p_table();

    for (i = 0; i < width * height / 2; i++)
    {
        y  = p_y[0];
        cb = p_u[0];
        cr = p_v[0];

        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value

        // 此处可调整RGB排序，BMP图片排序为BGR
        // 默认排序为：RGB
        p_rgb[0] = r;
        p_rgb[1] = g;
        p_rgb[2] = b;

        y  = p_y[1];
        cb = p_u[0];
        cr = p_v[0];
        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value

        p_rgb[3] = r;
        p_rgb[4] = g;
        p_rgb[5] = b;

        p_y += 2;
        p_u += 1;
        p_v += 1;
        p_rgb += 6;
    }
}

/**
内存分布
                    w
            +---------------------+
            |Y0U0V0Y1U1V1Y2U2V2...|
            |...                  |   h
            |...                  |
            |                     |
            |YxUxVxYxUxVx         |
            |...                  |   h
            |...                  |
            |                     |
            +---------------------+

转换：
u0y0v0 ->r0 g0 b0
u0y1v0 -> r1 g1 b1
其它类型类似...
*/
void yuv422packed_to_rgb24(YUV_TYPE type, unsigned char* yuv422p, unsigned char* rgb, int width, int height)
{
    int y, cb, cr;
    int r, g, b;
    int i = 0;
    unsigned char* p;
    unsigned char* p_rgb;

    p = yuv422p;

    p_rgb = rgb;

    init_yuv422p_table();

    for (i = 0; i < width * height / 2; i++)
    {
        switch(type)
        {
        case FMT_YUYV:
            y  = p[0];
            cb = p[1];
            cr = p[3];
            break;
        case FMT_YVYU:
            y  = p[0];
            cr = p[1];
            cb = p[3];
            break;
        case FMT_UYVY:
            cb = p[0];
            y  = p[1];
            cr = p[2];
            break;
        case FMT_VYUY:
            cr = p[0];
            y  = p[1];
            cb = p[2];
            break;
        default:
            break;
        }

        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value

        // 此处可调整RGB排序，BMP图片排序为BGR
        // 默认排序为：RGB
        p_rgb[0] = r;
        p_rgb[1] = g;
        p_rgb[2] = b;

        switch(type)
        {
        case FMT_YUYV:
        case FMT_YVYU:
            y = p[2];
            break;
        case FMT_UYVY:
        case FMT_VYUY:
            y = p[3];
            break;
        default:
            break;
        }

        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value

        p_rgb[3] = r;
        p_rgb[4] = g;
        p_rgb[5] = b;

        p += 4;
        p_rgb += 6;
    }
}

/**
内存分布
                    w
            +--------------------+
            |Y0Y1Y2Y3...         |
            |...                 |   h
            |...                 |
            |                    |
            +--------------------+
            |U0V0U1V1            |
            |...                 |   h
            |...                 |
            |                    |
            +--------------------+
                w/2
UV交织为NV16，VU交织为NV61
可以与上一函数合并，但方便查看，还是不合并
*/
void yuv422sp_to_rgb24(YUV_TYPE type, unsigned char* yuv422sp, unsigned char* rgb, int width, int height)
{
    int y, cb, cr;
    int r, g, b;
    int i = 0;
    unsigned char* p_y;
    unsigned char* p_uv;
    unsigned char* p_rgb;

    p_y = yuv422sp;
    p_uv = p_y + width * height;    // uv分量在Y后面
    p_rgb = rgb;

    init_yuv422p_table();

    for (i = 0; i < width * height / 2; i++)
    {
        y  = p_y[0];
        if (type ==  FMT_NV16)
        {
            cb = p_uv[0];
            cr = p_uv[1];    // v紧跟u，在u的下一个位置
        }
        if (type == FMT_NV61)
        {
            cr = p_uv[0];
            cb = p_uv[1];    // u紧跟v，在v的下一个位置
        }

        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value

        // 此处可调整RGB排序，BMP图片排序为BGR
        // 默认排序为：RGB
        p_rgb[0] = r;
        p_rgb[1] = g;
        p_rgb[2] = b;

        y  = p_y[1];
        if (type ==  FMT_NV16)
        {
            cb = p_uv[0];
            cr = p_uv[1];
        }
        if (type ==  FMT_NV61)
        {
            cr = p_uv[0];
            cb = p_uv[1];
        }

        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value

        p_rgb[3] = r;
        p_rgb[4] = g;
        p_rgb[5] = b;

        p_y += 2;
        p_uv += 2;
        p_rgb += 6;
    }
}
////////////////////////////////////////////////////////////////////////////

static long int crv_tab[256];
static long int cbu_tab[256];
static long int cgu_tab[256];
static long int cgv_tab[256];
static long int tab_76309[256];
static unsigned char clp[1024];   //for clip in CCIR601

void init_yuv420p_table()
{
    long int crv,cbu,cgu,cgv;
    int i,ind;
    static int init = 0;

    if (init == 1) return;

    crv = 104597; cbu = 132201;  /* fra matrise i global.h */
    cgu = 25675;  cgv = 53279;

    for (i = 0; i < 256; i++)
    {
        crv_tab[i] = (i-128) * crv;
        cbu_tab[i] = (i-128) * cbu;
        cgu_tab[i] = (i-128) * cgu;
        cgv_tab[i] = (i-128) * cgv;
        tab_76309[i] = 76309*(i-16);
    }

    for (i = 0; i < 384; i++)
        clp[i] = 0;
    ind = 384;
    for (i = 0;i < 256; i++)
        clp[ind++] = i;
    ind = 640;
    for (i = 0;i < 384; i++)
        clp[ind++] = 255;

    init = 1;
}

/**
内存分布
                    w
            +--------------------+
            |Y0Y1Y2Y3...         |
            |...                 |   h
            |...                 |
            |                    |
            +--------------------+
            |U0U1      |
            |...       |   h/2
            |...       |
            |          |
            +----------+
            |V0V1      |
            |...       |  h/2
            |...       |
            |          |
            +----------+
                w/2
 */
void yuv420p_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer,unsigned char* rgbbuffer, int width,int height)
{
    int y1, y2, u, v;
    unsigned char *py1, *py2;
    int i, j, c1, c2, c3, c4;
    unsigned char *d1, *d2;
    unsigned char *src_u, *src_v;

    src_u = yuvbuffer + width * height;   // u
    src_v = src_u + width * height / 4;  //  v

    if (type == FMT_YV12)
    {
        src_v = yuvbuffer + width * height;   // v
        src_u = src_v + width * height / 4;  //  u
    }
    py1 = yuvbuffer;   // y
    py2 = py1 + width;
    d1 = rgbbuffer;
    d2 = d1 + 3 * width;

    init_yuv420p_table();

    for (j = 0; j < height; j += 2)
    {
        for (i = 0; i < width; i += 2)
        {
            u = *src_u++;
            v = *src_v++;

            c1 = crv_tab[v];
            c2 = cgu_tab[u];
            c3 = cgv_tab[v];
            c4 = cbu_tab[u];

            //up-left
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c1)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c4)>>16)];

            //down-left
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c1)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c4)>>16)];

            //up-right
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c1)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c4)>>16)];

            //down-right
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c1)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c4)>>16)];
        }
        d1  += 3*width;
        d2  += 3*width;
        py1 += width;
        py2 += width;
    }
}

void yuv420sp_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer,unsigned char* rgbbuffer, int width,int height)
{
    int y1, y2, u, v;
    unsigned char *py1, *py2;
    int i, j, c1, c2, c3, c4;
    unsigned char *d1, *d2;
    unsigned char *src_u;

    src_u = yuvbuffer + width * height;   // u

    py1 = yuvbuffer;   // y
    py2 = py1 + width;
    d1 = rgbbuffer;
    d2 = d1 + 3 * width;

    init_yuv420p_table();

    for (j = 0; j < height; j += 2)
    {
        for (i = 0; i < width; i += 2)
        {
            if (type ==  FMT_NV12)
            {
                u = *src_u++;
                v = *src_u++;      // v紧跟u，在u的下一个位置
            }
            if (type == FMT_NV21)
            {
                v = *src_u++;
                u = *src_u++;      // u紧跟v，在v的下一个位置
            }

            c1 = crv_tab[v];
            c2 = cgu_tab[u];
            c3 = cgv_tab[v];
            c4 = cbu_tab[u];

            //up-left
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c1)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c4)>>16)];

            //down-left
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c1)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c4)>>16)];

            //up-right
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c1)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c4)>>16)];

            //down-right
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c1)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c4)>>16)];
        }
        d1  += 3*width;
        d2  += 3*width;
        py1 += width;
        py2 += width;
    }
}

/**
yuv444
-->
u0y0v0 ->r0 g0 b0
u1y1v1 -> r1 g1 b1

...
*/
void yuv4444_to_rgb24(unsigned char *yuv, unsigned char *rgb, int width, int height)
{
    int y, cb, cr;
    int r, g, b;
    int i = 0;
    unsigned char* p_y;
    unsigned char* p_u;
    unsigned char* p_v;
    unsigned char* p_rgb;

    p_y = yuv;
    p_u = yuv+width*height;
    p_v = yuv+2*width*height;
    p_rgb = rgb;

    init_yuv422p_table();

    for (i = 0; i < width * height; i++)
    {
        y  = p_y[0];
        cb = p_u[0];
        cr = p_v[0];
        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value

        p_rgb[0] = r;
        p_rgb[1] = g;
        p_rgb[2] = b;;
        p_rgb += 3;
        p_y++;
        p_u++;
        p_v++;
    }
}

/**
只支持平面、半平面的格式
交织的不支持，因为不知道如何获取Y
*/
void y_to_rgb24(unsigned char *yuv, unsigned char *rgb, int width, int height)
{
    int y, cb, cr;
    int r, g, b;
    int i = 0;
    unsigned char* p_y;
    unsigned char* p_rgb;

    p_y = yuv;

    p_rgb = rgb;

    // 只转换Y的，U、V也需要，其值固定为128
    cb = 128;
    cr = 128;
    init_yuv422p_table();

    for (i = 0; i < width * height; i++)
    {
        y  = p_y[0];
        r = MAX (0, MIN (255, (V[cr] + Y1[y])/10000));   //R value
        b = MAX (0, MIN (255, (U[cb] + Y1[y])/10000));   //B value
        g = MAX (0, MIN (255, (Y2[y] - 5094*(r) - 1942*(b))/10000)); //G value
        p_rgb[0] = r;
        p_rgb[1] = g;
        p_rgb[2] = b;

        p_rgb += 3;
        p_y++;
    }
}

// 对外接口
int yuv_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer,unsigned char* rgbbuffer, int width, int height)
{
    int ret = 0;

    switch (type)
    {
    case FMT_YUV420:
    case FMT_YV12:
        yuv420p_to_rgb24(type, yuvbuffer, rgbbuffer, width, height);
        break;
    case FMT_YUV422:
    case FMT_YV16:
        yuv422p_to_rgb24(type, yuvbuffer, rgbbuffer, width, height);
        break;
    case FMT_YUYV:
    case FMT_YVYU:
    case FMT_UYVY:
    case FMT_VYUY:
        yuv422packed_to_rgb24(type, yuvbuffer, rgbbuffer, width, height);
        break;
    case FMT_NV12:
    case FMT_NV21:
        yuv420sp_to_rgb24(type, yuvbuffer, rgbbuffer, width, height);
        break;
    case FMT_NV16:
    case FMT_NV61:
        yuv422sp_to_rgb24(type, yuvbuffer, rgbbuffer, width, height);
        break;
    case FMT_YUV444:
        yuv4444_to_rgb24(yuvbuffer, rgbbuffer, width, height);
        break;
    case FMT_Y:
        y_to_rgb24(yuvbuffer, rgbbuffer, width, height);
        break;
    default:
        printf("unsupported yuv type!\n");
        ret = -1;
        break;
    }

    return ret;
}
