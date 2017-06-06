#ifndef DEBUG_TIME_H
#define DEBUG_TIME_H

/////////////////////////////////////////////
// 函数计时宏
// 在使用本文前请先包含my_types.h、time_utils.h文件。

#define ENABLE_DEBUG_TIMER

#define ENABLE_DEBUG_MARK

//------------------------------------------

/**
 * 计时器宏使用示例1：
 * ...
 * tS(MyMark);
 * foo();
 * tE(MyMark);
 * ...
 * 即可打印出foo函数执行时间
 *
 * 计时器宏使用示例2：
 * ...
 * tSS(MyMark,10);
 * bar();
 * tEE(MyMark,10);
 * ...
 * 即可打印出bar函数执行时间
 */

#define tS L_TIMER_START
#define tE L_TIMER_END

#define tSS L_TIMER_START_EX
#define tEE L_TIMER_END_EX

#ifdef ENABLE_DEBUG_TIMER

    #define L_TIMER_START(id) \
        \
        DWORD dwStart##id = get_tick_count_us();

    #define TIME_SPAN(id) dwDiff##id
    
    #define L_TIMER_END(id) \
        \
        char szMsg##id[256];\
        unsigned int dwDiff##id = (unsigned int)(get_tick_count_us() - dwStart##id);\
        sprintf(szMsg##id, "{%s} %lu us (%lu ms)\r\n", #id, dwDiff##id, (unsigned int)(dwDiff##id/1000));\
        printf("%s", szMsg##id);

#else

    #define L_TIMER_START(id)
    #define L_TIMER_END(id)

#endif



#ifdef ENABLE_DEBUG_TIMER

    #define L_TIMER_START_EX(id,T) \
        \
        DWORD dwStart##id##T = get_tick_count_us();\
        static DWORD dwTimeMs##id##T = 0;\
        static DWORD dwCount##id##T = 0;\
        unsigned int dwDiff##id##T = 0;

    #define TIME_SPAN_T(id,T) dwDiff##id##T

    #define L_TIMER_END_EX(id,T) \
        \
        char szMsg##id##T[256];\
        dwTimeMs##id##T += (get_tick_count_us() - dwStart##id##T);\
        dwCount##id##T++;\
        if ( dwCount##id##T >= T )\
        { \
            dwDiff##id##T = (unsigned int)((float)dwTimeMs##id##T / dwCount##id##T); \
            sprintf(szMsg##id##T, "{%s,%s} %lu us (%lu ms)\r\n", #id, #T, dwDiff##id##T, (unsigned int)(dwDiff##id##T/1000)); \
            printf("%s", szMsg##id##T);\
            dwTimeMs##id##T = 0;\
            dwCount##id##T = 0;\
        }

#else

    #define L_TIMER_START_EX(id,T)
    #define L_TIMER_END_EX(id,T)

#endif

#endif /* DEBUG_TIME_H */
