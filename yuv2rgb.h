/**
 * @file   yuv2rgb.h
 * @author Late Lee
 * @date   2012.01.20
 * 
 * @brief  
 *         YUV422P/YUV420P/YUV422SP转RGB24实现代码
 *         YUV各种格式之间转换的实现
 *
 * @note
 *         1、在Windows、Linux编译测试通过
 *         2、代码中使用的YUV420P、YUV422P是平面(planar)格式，不是打包(packed)格式
 *
 *         3、qcif: 176*144
              cif: 352*288
           4、实现代码尽量使用直观方式，以方便了解YUV内部布局，没有深入优化。
           5、提供的函数没有进行严格的参数检查，需要调用者细心使用。
 * @todo  确定好RGB排序到底是什么
 *
 * @log   2013-10-26 参考422p函数，实现422sp转换422p格式函数。将初始化接口隐藏，不对外公开
 *        2014-02-10 封装统一一个函数。
 *        2014-08-03 更新YUV转RGB函数。添加UYVY格式接口。
 *        2014-08-04 完成422打包格式的接口。4种格式测试通过。 完成YUV444格式。 SP、P互转接口。
 *        2014-08-05 完成422SP、420SP格式的转换。打包格式使用查表法。
 *        2014-08-06 Y转换RGB
 *        2017-05-27 添加packed与planar格式之间的转换
 *        2017-05-30(端午) 添加packed与半平面格式的转换
 *
 * 笔记：
            每个Y、U、V、R、G、B均占用1个字节
            YUV422P平面格式
            Y、U、V分开存储，Y占w*h，U、V分别占w*h/2。每行Y有w个，U、V各w/2个。
            两个Y共用一个U和V，两个像素有Y分量2个，U、V分量各一个，共4字节，因此一个像素占2字节。

            内存分布如下：
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

            第一个像素：Y0 U0 V0
            第二个像素：Y1 U0 V0
            第三个像素：Y2 U1 V1
            第四个像素：Y3 U1 V1

			每种格式分布见转换函数处
 */

#ifndef _YUV2RGB_H_
#define _YUV2RGB_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    FMT_Y = 0,
    
    FMT_YUV420, // yuv 平面格式 y.... u.... v....
    FMT_YV12,  //               y.... v.... u....
    FMT_NV12, //    半平面格式  y.... uvuv .....
    FMT_NV21, //    半平面格式  y.... vuvu .....
    
    FMT_YUV422, // yuv 平面格式 y.... u.... v....
    FMT_YV16,   //               y.... v.... u....
    FMT_NV16,   //    半平面格式  y.... uvuv .....
    FMT_NV61,   //    半平面格式  y.... vuvu .....
    FMT_YUYV,   // 打包格式      yuyvyuyv....
    FMT_YVYU,   // ....
    FMT_UYVY,
    FMT_VYUY,


    FMT_YUV444,

    FMT_RGB24,
    FMT_BGR24,

}YUV_TYPE;

/** 
 * @brief YUV转RGB24(查表法)
 * 
 * @param type       YUV格式类型 支持格式见YUV_TYPE定义
 * @param yuvbuffer  YUV格式缓冲区
 * @param rgbbuffer  RGB24格式缓冲区
 * @param width      图像宽
 * @param height     图像高
 *
 * @return 0: OK -1: failed
 *
 * @note
 *        1、YUV422 buffer: w * h * 2 YUV420 buffer: w * h * 3 / 2
 *        2、rgbbuffer数据排序为RGB，如保存BMP，需要调整为BGR
 *        3、总转换入口函数
 */
int yuv_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer,unsigned char* rgbbuffer, int width, int height);

/**
 * @brief  YUV422P转换为RGB24(查表法)
 * 
 * @param  type[IN]          YUV格式类型
 * @param  yuvbuffer[IN]    YUV422P平面格式缓冲区
 * @param  rgbbuffer[OUT]   RGB缓冲区
 * @param  width[IN]        图像宽
 * @param  height[IN]       图像高
 * 
 * @return no
 *
 * @note
 *         1.YUV422P格式YUV缓冲区大小为w * h * 2
 *         2.rgbbuffer数据排序为RGB，如保存BMP，需要调整为BGR
 */
void yuv422p_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer, unsigned char* rgbbuffer, int width, int height);

/**
 * @brief  YUV422P打包格式转换为RGB24(查表法)
 * 
 * @param  type[IN]         YUV格式类型
 * @param  yuvbuffer[IN]    YUV422P打包格式缓冲区
 * @param  rgbbuffer[OUT]   RGB缓冲区
 * @param  width[IN]        图像宽
 * @param  height[IN]       图像高
 * 
 * @return no
 *
 * @note
 *         1.YUV422P格式YUV缓冲区大小为w * h * 2，支持YUYV、YVYU、UYVY、VYUY
 *         2.rgbbuffer数据排序为RGB，如保存BMP，需要调整为BGR
 */
void yuv422packed_to_rgb24(YUV_TYPE type, unsigned char *yuv, unsigned char *rgb, int width, int height);

/** 
 * @brief YUV422SP转RGB24(查表法)
 *
 * @param type[IN]   YUV格式类型
 * @param yuvbuffer  YUV422SP格式缓冲区
 * @param rgbbuffer  RGB24格式缓冲区
 * @param width      图像宽
 * @param height     图像高
 *
 *
 * @note
 *        1、YUV422SP格式YUV缓冲区大小为w * h * 2，支持NV16、NV61
 *        2、rgbbuffer数据排序为RGB，如保存BMP，需要调整为BGR
 */
void yuv422sp_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer, unsigned char* rgbbuffer, int width, int height);

/** 
 * @brief YUV420P转RGB24(查表法)
 * 
 * @param type[IN]   YUV格式类型
 * @param yuvbuffer  YUV420P格式缓冲区
 * @param rgbbuffer  RGB24格式缓冲区
 * @param width      图像宽
 * @param height     图像高
 *
 *
 * @note
 *        1、YUV420P格式YUV缓冲区大小为w * h * 3 / 2
 *        2、rgbbuffer数据排序为RGB，如保存BMP，需要调整为BGR
 */
void yuv420p_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer,unsigned char* rgbbuffer, int width, int height);

void yuv420sp_to_rgb24(YUV_TYPE type, unsigned char* yuvbuffer,unsigned char* rgbbuffer, int width,int height) ;
//void yuv420sp_to_rgb24(YUV_TYPE type, unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height);

////////////////////////////////////////////////////////////////////////////////////////////////////
void yuv422p_to_yuv422sp(YUV_TYPE type, unsigned char* yuv422p, unsigned char* yuv422sp, int width, int height);
void yuv422sp_to_yuv422p(YUV_TYPE type, unsigned char* yuv422sp, unsigned char* yuv422p, int width, int height);

// 将420平面格式转换成sp格式，根据type参数确定是nv12还是nv21
void yuv420p_to_yuv420sp(YUV_TYPE type, unsigned char* yuv420p, unsigned char* yuv420sp, int width, int height);
void yuv420sp_to_yuv420p(YUV_TYPE type, unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height);

// yuv平面格式转换成打包格式
void yuv422p_to_yuv422packed(YUV_TYPE type1, YUV_TYPE type2, unsigned char* yuv422p, unsigned char* yuv, int width, int height);
void yuv422packed_to_yuv422p(YUV_TYPE type1, YUV_TYPE type2, unsigned char* yuv422p, unsigned char* yuv, int width, int height);
void yuv422packed_to_yuv422sp(YUV_TYPE type1, YUV_TYPE type2, unsigned char* yuv422sp, unsigned char* yuv, int width, int height);

// 打包格式转打包格式
void yuv422packed_to_422packed(YUV_TYPE type1, YUV_TYPE type2, unsigned char* yuv422p, unsigned char* yuv, int width, int height);

// yuv422 to yuv420
void yuv422packed_to_yuv420p(YUV_TYPE type1, YUV_TYPE type2, unsigned char* yuv420p, unsigned char* yuv, int width, int height);
void yuv422packed_to_yuv420sp(YUV_TYPE type1, YUV_TYPE type2, unsigned char* yuv420sp, unsigned char* yuv, int width, int height);

void yuv422p_to_yuv420p(YUV_TYPE type1, YUV_TYPE type2, unsigned char* yuv420p, unsigned char* yuv, int width, int height);


void yu_to_yv(YUV_TYPE type, unsigned char* yu, unsigned char* yv, int width, int height);
void yv_to_yu(YUV_TYPE type, unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height);

// YUV格式转换成y，yuv包括各种各样
void yuv_to_y(YUV_TYPE type, unsigned char* yuv, unsigned char* y, int width, int height);


////////////////////////////////////////////////////////////////////////////////////////////////////
void swaprgb(unsigned char* rgb, int len);

////////////////////////////////////////////////////////////////////////////////////////////////////
void yuv420_to_rgb24_1(unsigned char* yuv420, unsigned char* rgb, int width, int height);

void yuv420_to_rgb24_2(unsigned char *yuv420, unsigned char *rgb24, int width, int height) ;

void yuv420_to_rgb24_3(unsigned char* yuv, unsigned char* rgb, int width, int height);

void yuv4444_to_rgb24(unsigned char *yuv, unsigned char *rgb, int width, int height);

// 高最好是10的整数倍
void save_yuv_file(const char* filename, int width, int height, int type);

void change_yuv_file(const char* filename, const char* file_out, int width, int height, int type);

#ifdef __cplusplus
}
#endif

#endif /* _YUV2RGB_H_ */