/**
 * @file   time_utils.h
 * @author Late Lee 
 * @date   2012-6-19 18:34:43
 * @brief  
 *         ���ļ���һЩ���õ�ʱ�亯������ָʱ���ǵ���ʱ��(local time)������UTC��
 *
 * @note
    windowsƽ̨��ȡGMTʱ��͵���ʱ�����
    SYSTEMTIME stUTC;
    GetSystemTime(&stUTC);

    SYSTEMTIME stLocal;
    GetLocalTime(&stLocal);
 * @log
          2012-07-16
          ��ӽ�ʱ��ת�����ַ�������
          2013-04-30
          �������sleep����
          2013-05-10
          ��ӻ�ȡʱ������������ʱ������ʱ�亯��
 */
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "my_types.h"

/* �ýṹ����Windowsƽ̨SYSTEMTIME�ṹ��һ�� */
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
 * @brief ��ȡϵͳʱ�䣬ת����64λ���ݣ���ȷ�����롣
 * 
 * @param time_low[OUT]   ��32λʱ��
 * @param time_high[OUT]  ��32λʱ��
 *
 * @note
 *  1����ȡ���ǵ���ʱ�䣬������GMTʱ��
 */
int get_system_time(uint32* time_low, uint32* time_high);

/** 
 * @brief ��ȡ���ڡ�ʱ�䣬�ŵ�ʱ��ṹ�塣
 * 
 * @param ts[OUT]         ʱ��ṹ��
 *
 * @note
 *  1����ȡ���ǵ���ʱ�䣬������GMTʱ��
 */
void get_system_datetime(TIME_STRUCT* ts);

/** 
 * @brief ��64λʱ��ֵת�������ڡ�ʱ���ʽ��
 * 
 * @param time_low[IN]   ��32λʱ��
 * @param time_high[IN]  ��32λʱ��
 * @param ts[OUT]        ʱ��ṹ��
 *
 */
void convert_ms2time(uint32 time_low, uint32 time_high, TIME_STRUCT* ts);

/** 
 * @brief �����ڡ�ʱ��ת����64λʱ��ֵ��
 * 
 * @param ts[IN]          ʱ��ṹ��
 * @param time_low[OUT]   ��32λʱ��
 * @param time_high[OUT]  ��32λʱ��
 *
 *
 */
void convert_time2ms(TIME_STRUCT* ts, uint32* time_low, uint32* time_high);

/** 
 * @brief ����ǰʱ��ת�����ַ�������ȷ�����롣
 * 
 * @param strtime[OUT]         ת����ʱ����ַ���
 *
 * @note
 *       �������ɵ����߱�֤��С(����23�ֽ�)
 */
void get_strtime(char* strtime);

int get_timeval(TIMETV_STRUCT* t);

int set_timeval(const TIMETV_STRUCT* t);

int convert_time2tm(TIMETV_STRUCT t, TIME_STRUCT* ts);

int convert_tm2time(TIME_STRUCT ts, TIMETV_STRUCT* t);

/** 
 * @brief ͨ��ʱ��������ʱ��
 * 
 * @param old_time[IN]   ԭʼʱ��ṹ��ָ��
 * @param new_time[OUT]   ԭʼʱ��ṹ��ָ��
 * @param timezone[IN]   ʱ��ֵ
 *
 * @return 0: �ɹ� -1��ʧ��
 *
 * @note old_time��new_time������ͬһ�ṹ��ָ��
 * @todo����ǿ��Ĺ�������������ꡢ�¡��ա�ʱ������������Щ
 */
void adjust_timezone(TIME_STRUCT* old_time, TIME_STRUCT* new_time, int timezone);

/** 
 * @brief ��ȡʱ��
 * 
 * @param timezone[OUT]   ʱ��ֵ
 *
 * @return 0: �ɹ� -1��ʧ��
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
