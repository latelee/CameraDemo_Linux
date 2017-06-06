/**
 * @file   time_utils.h
 * @author Late Lee 
 * @date   2012-6-19 18:34:43
 * @brief  
 *         本文件是一些常用的时间函数，所指时间是当地时间(local time)，并非UTC。
 *
 * @note
    windows平台获取GMT时间和当地时间代码
    SYSTEMTIME stUTC;
    GetSystemTime(&stUTC);

    SYSTEMTIME stLocal;
    GetLocalTime(&stLocal);
 * @log
          2012-07-16
          添加将时间转换成字符串函数
          2013-04-30
          添加两个sleep函数
          2013-05-10
          添加获取时区函数，根据时区调整时间函数
 */
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "my_types.h"

/* 该结构体与Windows平台SYSTEMTIME结构体一致 */
typedef struct _TIME_STRUCT {
    uint16 ts_year;     /* year */
    uint16 ts_mon;      /* month */
    uint16 ts_wday;     /* day of week, since Sunday, range from 0 to 6 */
    uint16 ts_day;      /* day of month, range from 1 to 31 */
    uint16 ts_hour;     /* hour */
    uint16 ts_min;      /* minute */
    uint16 ts_sec;      /* second */
    uint16 ts_millsec;  /* million second */
	int16  ts_isdst;   /* aylight saving time */
} TIME_STRUCT;

typedef struct _TIMETV_STRUCT {
    uint32 sec;
    uint32 msec;
} TIMETV_STRUCT;

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @brief 获取系统时间，转换成64位数据，精确到毫秒。
 * 
 * @param time_low[OUT]   低32位时间
 * @param time_high[OUT]  高32位时间
 *
 * @note
 *  1、获取的是当地时间，并不是GMT时间
 */
int get_system_time(uint32* time_low, uint32* time_high);

/** 
 * @brief 获取日期、时间，放到时间结构体。
 * 
 * @param ts[OUT]         时间结构体
 *
 * @note
 *  1、获取的是当地时间，并不是GMT时间
 */
void get_system_datetime(TIME_STRUCT* ts);

/** 
 * @brief 将64位时间值转换成日期、时间格式。
 * 
 * @param time_low[IN]   低32位时间
 * @param time_high[IN]  高32位时间
 * @param ts[OUT]        时间结构体
 *
 */
void convert_ms2time(uint32 time_low, uint32 time_high, TIME_STRUCT* ts);

/** 
 * @brief 将日期、时间转换成64位时间值。
 * 
 * @param ts[IN]          时间结构体
 * @param time_low[OUT]   低32位时间
 * @param time_high[OUT]  高32位时间
 *
 *
 */
void convert_time2ms(TIME_STRUCT* ts, uint32* time_low, uint32* time_high);

/** 
 * @brief 将当前时间转换成字符串，精确到毫秒。
 * 
 * @param strtime[OUT]         转换成时间的字符串
 *
 * @note
 *       缓冲区由调用者保证大小(至少23字节)
 */
void get_strtime(char* strtime);

int get_timeval(TIMETV_STRUCT* t);

int set_timeval(const TIMETV_STRUCT* t);

int convert_time2tm(TIMETV_STRUCT t, TIME_STRUCT* ts);

int convert_tm2time(TIME_STRUCT ts, TIMETV_STRUCT* t);

/** 
 * @brief 通过时区来调整时间
 * 
 * @param old_time[IN]   原始时间结构体指针
 * @param new_time[OUT]   原始时间结构体指针
 * @param timezone[IN]   时区值
 *
 * @return 0: 成功 -1：失败
 *
 * @note old_time和new_time可以是同一结构体指针
 * @todo：更强大的功能是任意调整年、月、日、时。。。。。这些
 */
void adjust_timezone(TIME_STRUCT* old_time, TIME_STRUCT* new_time, int timezone);

/** 
 * @brief 获取时区
 * 
 * @param timezone[OUT]   时区值
 *
 * @return 0: 成功 -1：失败
 */
int get_timezone(int* timezone);

unsigned int get_tick_count();
unsigned int get_tick_count_us();

void get_run_time();

int64_t getutime(void);

void my_sleep(int ms);

void my_usleep(int usec);

#ifdef __cplusplus
}
#endif

#endif
