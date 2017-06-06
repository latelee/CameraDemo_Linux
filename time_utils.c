#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef linux
#include <sys/time.h>
#include <unistd.h>
#endif

#include "time_utils.h"

#ifdef WIN32
/* windows平台gettimeofday的实现，时区参数只能为NULL */
static int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year     = wtm.wYear - 1900;
    tm.tm_mon      = wtm.wMonth - 1;
    tm.tm_mday     = wtm.wDay;
    tm.tm_hour     = wtm.wHour;
    tm.tm_min      = wtm.wMinute;
    tm.tm_sec      = wtm.wSecond;
    tm.tm_isdst    = -1;
    clock = mktime(&tm);
    tp->tv_sec = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return 0;
}
#endif

int get_system_time(uint32* time_low, uint32* time_high)
{
    time_t second = 0;
    struct timeval tv;
    uint64 millsecond = 0;

    if ( time_low == NULL || time_high == NULL)
        return -1;

    // get seconds
    second = time(NULL);

    // get microsecond
    gettimeofday(&tv, NULL);


    // convert to million second
    millsecond = tv.tv_sec;//second;
    millsecond *= 1000;
    millsecond += (tv.tv_usec / 1000);

    *time_low = (uint32)(millsecond);
    *time_high = (uint32)(millsecond>>32);

    return 0;
}

void get_system_datetime(TIME_STRUCT* ts)
{
    /*
    Windows平台简单的方法：
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    memcpy(ts, &wtm, sizeof(SYSTEMTIME));
    */
    /*
    struct timeval tv;
    struct tm* timeinfo = NULL;
    time_t second = 0;
    second = time(NULL);
    timeinfo = localtime(&second);
    gettimeofday(&tv, NULL);
    */
    struct timeval tv;
    struct tm* timeinfo = NULL;
    time_t second = 0;
    
    gettimeofday(&tv, NULL);
    second = tv.tv_sec;
    timeinfo = localtime(&second);

    ts->ts_year = timeinfo->tm_year + 1900;
    ts->ts_mon  = timeinfo->tm_mon + 1;
    ts->ts_wday = timeinfo->tm_wday;
    ts->ts_day  = timeinfo->tm_mday;
    ts->ts_hour = timeinfo->tm_hour;
    ts->ts_min  = timeinfo->tm_min;
    ts->ts_sec  = timeinfo->tm_sec;
    ts->ts_millsec =  (uint16)(tv.tv_usec / 1000);
	ts->ts_isdst = timeinfo->tm_isdst;
}

void convert_ms2time(uint32 time_low, uint32 time_high, TIME_STRUCT* ts)
{
    uint64 millsecond = 0;
    time_t second = 0;
    struct tm* timeinfo = NULL;

    if (ts == NULL)
        return;

    millsecond = time_high;
    millsecond <<= 32;
    millsecond |= time_low;

    ts->ts_millsec = (uint32)(millsecond % 1000);
    second = (time_t)((millsecond - ts->ts_millsec) / 1000);

    timeinfo = localtime(&second);
    ts->ts_year = timeinfo->tm_year + 1900;
    ts->ts_mon  = timeinfo->tm_mon + 1;
    ts->ts_wday = timeinfo->tm_wday;
    ts->ts_day  = timeinfo->tm_mday;
    ts->ts_hour = timeinfo->tm_hour;
    ts->ts_min  = timeinfo->tm_min;
    ts->ts_sec  = timeinfo->tm_sec;
}

// 注：VS打印64位：printf("%I64u\n", millsecond);
void convert_time2ms(TIME_STRUCT* ts, uint32* time_low, uint32* time_high)
{
    struct tm tm_time;
    time_t time = 0;
    uint64 millsecond = 0;

    if ((NULL == ts) || (NULL ==time_low) || (NULL == time_high))
        return;

    tm_time.tm_year = ts->ts_year - 1900;
    tm_time.tm_mon = ts->ts_mon - 1;
    tm_time.tm_mday = ts->ts_day;
    tm_time.tm_hour = ts->ts_hour;
    tm_time.tm_min = ts->ts_min;
    tm_time.tm_sec = ts->ts_sec;

    time = mktime(&tm_time);    /* 得到秒数 */
    /*
    不能合并为下式
        millsecond = time*1000 + ts->ts_millsec;
    */
    millsecond = time;
    millsecond *= 1000;
    millsecond += ts->ts_millsec;

    *time_low = (uint32)(millsecond);
    *time_high = (uint32)(millsecond >> 32);
}

void get_strtime(char* strtime)
{
    uint32 time1, time2;
    TIME_STRUCT tmptime;
    get_system_time(&time1, &time2);
    convert_ms2time(time1, time2, &tmptime);

    sprintf(strtime, "%04d-%02d-%02d %02d:%02d:%02d:%03d", tmptime.ts_year, tmptime.ts_mon,
                                                     tmptime.ts_day, tmptime.ts_hour,
                                                     tmptime.ts_min, tmptime.ts_sec,
                                                     tmptime.ts_millsec);
}

int get_timeval(TIMETV_STRUCT* t)
{ 
    struct timeval tv;
    if (0 != gettimeofday(&tv , NULL))
    {   
        return -1;
    }

    t->sec = tv.tv_sec;
    t->msec = tv.tv_usec / 1000;

    return 0;
}

int set_timeval(const TIMETV_STRUCT* t)
{
    uint64_t time = t->sec;
    time = time * 1000 + t->msec;
    struct timeval tv;
    tv.tv_sec = time / 1000;
    tv.tv_usec = (time % 1000) * 1000;
    if(settimeofday(&tv, NULL) < 0)
    {
        return -1;
    }
    return 0;
}

int convert_time2tm(TIMETV_STRUCT t, TIME_STRUCT* ts)
{
    uint64_t time = t.sec;
    struct tm cDateTime;

    time = time * 1000 + t.msec;

    ts->ts_millsec = time % 1000;
    time_t tTotalSecond = time / 1000;

    localtime_r((const time_t*)&tTotalSecond, &cDateTime);
    ts->ts_year = cDateTime.tm_year + 1900;
    ts->ts_mon  = cDateTime.tm_mon + 1;
    ts->ts_day = cDateTime.tm_mday;
    ts->ts_hour = cDateTime.tm_hour;
    ts->ts_min = cDateTime.tm_min;
    ts->ts_sec = cDateTime.tm_sec;

    return 0;
}

int convert_tm2time(TIME_STRUCT ts, TIMETV_STRUCT* t)
{
    struct tm tm_time;
    uint64_t timems;
    time_t time;

    memset(&tm_time, 0, sizeof(tm_time));
    tm_time.tm_year = ts.ts_year - 1900;
    tm_time.tm_mon = ts.ts_mon - 1;
    tm_time.tm_mday = ts.ts_day;
    tm_time.tm_hour = ts.ts_hour;
    tm_time.tm_min = ts.ts_min;
    tm_time.tm_sec = ts.ts_sec;
    time = mktime(&tm_time);
    timems = time;
    timems *= 1000;

    t->sec = timems / 1000;
    t->msec = timems % 1000;

    return 0;
}

unsigned int get_tick_count()
{
#ifdef WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
// 注：需rt库
#if 0
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned int time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return time;
#endif
}

unsigned int get_tick_count_us()
{
#ifdef WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000*1000 + tv.tv_usec;
#endif
// 注：需rt库
#if 0
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned int time = ts.tv_sec * 1000*1000 + ts.tv_nsec;
    return time;
#endif
}

// 获取系统开机时间
void get_run_time()
{
#ifdef WIN32
    DWORD time = GetTickCount();
    int nSecond;
    int nMinute;
    int nHour;
    int nDay;

    nSecond = time/1000%60;
    nMinute = time/1000/60%60;
    nHour   = time/1000/60/60%24;
    nDay    = time/1000/60/60/24%24;

    //printf("%d %d %d %d %d\n", time, nDay, nHour, nMinute, nSecond);
#endif
}

int64_t getutime(void)
{
#ifdef WIN32
    HANDLE proc;
    FILETIME c, e, k, u;
    proc = GetCurrentProcess();
    GetProcessTimes(proc, &c, &e, &k, &u);
    return ((int64_t) u.dwHighDateTime << 32 | u.dwLowDateTime) / 10;
#else
    return 0;
#endif
}

void my_sleep(int ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void my_usleep(int usec)
{
/*
    struct timespec ts = { usec / 1000000, usec % 1000000 * 1000 };
    while (nanosleep(&ts, &ts) < 0 && errno == EINTR);
    return 0;
*/
#ifdef WIN32
    Sleep(usec / 1000);
#else
    usleep(usec);
#endif
}

void adjust_timezone(TIME_STRUCT* old_time, TIME_STRUCT* new_time, int timezone)
{
    uint32_t time_high = 0;
    uint32_t time_low = 0;

    convert_time2ms(old_time, &time_low, &time_high);
    printf("111 time_low: %u time_high: %u\n", time_low, time_high);
    time_low += timezone * 60 * 60 * 1000;
    printf("222 time_low: %u time_high: %u\n", time_low, time_high);
    convert_ms2time(time_low, time_high, new_time);
}

// 获取时区
int get_timezone(int* timezone)
{
#ifdef WIN32
    int tmp_time_zone = 0;
    TIME_ZONE_INFORMATION tzi;

    GetTimeZoneInformation(&tzi);

    tmp_time_zone = tzi.Bias/ -60;
    *timezone = tmp_time_zone;

    return 0;
#else
    const char* date_cmd = "date +%z";
    FILE *fp = NULL;
    char timezone_info[16] = {0};
    int tmp_time_zone = 0;

    fp = popen(date_cmd, "r");
    if (fp == NULL)
    {
        return -1;
    }
    fread(timezone_info, sizeof(timezone_info), 1, fp);
    pclose(fp);

    timezone_info[strlen(timezone_info) - 1] = '\0';

    if (isdigit(timezone_info[1]) && isdigit(timezone_info[2]))
    {
        tmp_time_zone = (timezone_info[1] - '0') * 10 + (timezone_info[2] - '0');
        if (timezone_info[0] == '-')
        {
            tmp_time_zone = - tmp_time_zone;
        }
        *timezone = tmp_time_zone;

        return 0;
    }
    else
    {
        *timezone = 0;
        return -1;
    }

    return 0;
#endif
}
