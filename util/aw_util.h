//
// Created by karl.wang on 2018/3/17.
//

#ifndef __AW_UTIL_H_
#define __AW_UTIL_H_
#define _WINDOWS
#ifdef _WINDOWS
#include <Windows.h>
#define aw_mutex_t CRITICAL_SECTION
#define aw_mutex_init(lock) InitializeCriticalSection(lock)
#define aw_mutex_uninit(lock) DeleteCriticalSection(lock)
#define aw_mutex_lock(lock) EnterCriticalSection(lock)
#define aw_mutex_unlock(lock) LeaveCriticalSection(lock)
#define aw_sleep_ms(s) Sleep(s);
#else
#include <unistd.h>
#include <time.h>
#include <pthread.h>

unsigned long GetTickCount();
#define aw_mutex_t pthread_mutex_t
#define aw_mutex_init(lock) pthread_mutex_init(lock, NULL)
#define aw_mutex_uninit(lock) pthread_mutex_destroy(lock)
#define aw_mutex_lock(lock) pthread_mutex_lock(lock)
#define aw_mutex_unlock(lock) pthread_mutex_unlock(lock)
#define aw_sleep_ms(s) usleep (s*1000)
#define TRUE 1
#define FALSE 0
#endif
unsigned long AwGetTickCount();
#define AW_TRUE 1
#define AW_FALSE 0


#ifdef _WINDOWS
#define API_EXPORT
#else
#define API_EXPORT __attribute__ ((visibility("default")))
#endif


void aw_mem_init(int mem_hook_flag, int mem_trace_flag, int mem_cycle_dump_flag);
void aw_mem_uninit();

void aw_mem_dump();

void *aw_malloc(unsigned int size, const char *function, int line);
void aw_free(void *p, const char *function, int line);

void aw_sync();

#define AW_MALLOC(size) aw_malloc(size, __FUNCTION__, __LINE__)
#define AW_FREE(p) aw_free(p, __FUNCTION__, __LINE__)
#define AW_RET_SUCC 0
#define AW_RET_ERR -1

long long TIME_MS_PRINT(const char* tag,const char *desc);

void AW_SYSTEM(const char *cmd);
void TIME_STATISTICS_START();

void TIME_STATISTICS_END(const char *tag, const char *desc);
#define TIME_LIMIT
inline int time_limit()
{
#ifdef TIME_LIMIT
#ifndef _WINDOWS
    struct tm * t;
    time_t	tt;

    time( &tt );
    t				= localtime( &tt );

    int 	year	= t->tm_year + 1900;
    int 	month	= t->tm_mon + 1;
    int 	day 	= t->tm_mday;
    // LOGD("time: %d %d %d", year, month, day);

    if ( year <= 2018 && month <= 7 && day <= 31 )
    {
        return 0;
    }
    else
    {
        return - 1;
    }
#endif
	return 0;
#else
    return 0;
#endif

}
#endif // __AW_UTIL_H_
