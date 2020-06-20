#include "stdio.h"
#include "aw_log.h"
#include "aw_util.h"
#include "cJSON.h"
#ifdef _WINDOWS
#include <windows.h>
DWORD g_startTime;
#else
#include <sys/time.h>
long g_startTime;
#endif

void AW_SYSTEM(const char* cmd)
{
	printf("[aw_system]%s\n",cmd);
	system(cmd);
}

long long TIME_MS_PRINT(const char* tag,const char *desc)
{
	long long time;

#ifdef _WINDOWS
	time = GetTickCount();
#else
	struct timeval t_time;
	gettimeofday(&t_time, NULL);
	time = ((long long) t_time.tv_sec) * 1000 + (long long) t_time.tv_usec / 1000;
#endif
    aw_log_tag(tag, "%lldms %s",time, desc);
	return time;
}
void TIME_STATISTICS_START() {
#ifdef _WINDOWS
	g_startTime = GetTickCount();
#else
	struct timeval t_time;
	gettimeofday(&t_time, NULL);
	g_startTime = ((long) t_time.tv_sec) * 1000 + (long) t_time.tv_usec / 1000;
#endif
}

void TIME_STATISTICS_END(const char *tag, const char *desc) {
#ifdef _WINDOWS
	aw_log_tag("time statistics","%s used:%ldms", desc, GetTickCount() - g_startTime);
#else
	struct timeval t_time;
	gettimeofday(&t_time, NULL);
	long nowTime = ((long) t_time.tv_sec) * 1000 + (long) t_time.tv_usec / 1000;
	aw_log_tag(tag,"%s used:%ldms", desc, nowTime - g_startTime);
#endif
}
unsigned long AwGetTickCount()
{
	return GetTickCount();
}

#ifndef _WINDOWS
unsigned long GetTickCount()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);
//	printf("ts.tv_sec %d %lu\n", ts.tv_sec, ts.tv_nsec);
	return (ts.tv_sec * 1000 + ts.tv_nsec/(1000*1000) );
}
#endif
#include <map>
using namespace std;
static map<void * ,string> g_mem_map;

#define AW_MEM_CHECK
#define AW_MEM_TRACE
#define AW_MEM_CYCLE_DUMP

#ifdef AW_MEM_CYCLE_DUMP
#include "aw_thread.h"
aw_thread_handle g_mem_dump_thread = {0};
unsigned long g_mem_malloc_size = 0;
aw_mutex_t g_mem_lock;
int g_mem_hook_flag = 0;
int g_mem_trace_flag = 0;
int g_mem_cycle_dump_flag = 0;


static void *  _cjson_hook_malloc_fn(size_t sz)
{
	return	AW_MALLOC(sz);
}

static void _cjson_hook_free_fn(void *ptr)
{
	return AW_FREE(ptr);
}



static int _aw_mem_dump_thread_cb(void * context)
{
	while(1)
	{
		aw_mem_dump();
		aw_sleep_ms(1000*10);
	}
}
static void _aw_start_mem_dump()
{
	if (!aw_thread_is_running(g_mem_dump_thread))
	{
		aw_thread_create((aw_thread_handle *)&g_mem_dump_thread, _aw_mem_dump_thread_cb, NULL);
		aw_thread_set_name(*(aw_thread_handle *)&g_mem_dump_thread, "mem_dump_thread");

	}
}
#endif
void aw_mem_dump()
{
	aw_mutex_lock(&g_mem_lock);
	map<void * ,string>::iterator iter;
	iter = g_mem_map.begin();
	aw_log_info("[MEM DUMP] total malloc %lu", g_mem_malloc_size);

	aw_log_info("[MEM DUMP] start");
	aw_log_info("[MEM DUMP][index][    size][      addr][function line]");
	int i = 0;
	while(iter != g_mem_map.end()) {
		aw_log_info("[MEM DUMP][%5d]%s",i, iter->second.c_str());
		iter++;
		i++;
	}
	aw_log_info("[MEM DUMP] end");
	aw_mutex_unlock(&g_mem_lock);
}
void aw_mem_uninit()
{

}
void aw_mem_init(int mem_hook_flag, int mem_trace_flag, int mem_cycle_dump_flag)
{
#ifdef MEM_HOOK_CJSON
	cJSON_Hooks hooks;
	hooks.malloc_fn = _cjson_hook_malloc_fn;
	hooks.free_fn = _cjson_hook_free_fn;
	cJSON_InitHooks(&hooks);
#endif	

	g_mem_malloc_size = 0;
	aw_thread_handle g_mem_dump_thread = {0};
	aw_mutex_init(&g_mem_lock);

	g_mem_hook_flag = mem_hook_flag;
	g_mem_trace_flag = mem_trace_flag;
	g_mem_cycle_dump_flag = mem_cycle_dump_flag;
	if (g_mem_cycle_dump_flag)
	{
		_aw_start_mem_dump();
	}

}

void *aw_malloc(unsigned int size, const char *function, int line)
{

	void *p = malloc(size);
#ifdef AW_MEM_CHECK
	if (g_mem_hook_flag)
	{
		aw_mutex_lock(&g_mem_lock);

		char trace[128] = {0};
		snprintf(trace, 127, "[%8u][%010p][%s %d]", size, p, function, line);
		string s = trace;
		g_mem_map.insert(pair<void *,string>(p,s));
		g_mem_malloc_size += size;

		aw_mutex_unlock(&g_mem_lock);

#ifdef AW_MEM_TRACE
		if (g_mem_trace_flag)
		{
			aw_log_info("[MEM TRACE][MALLOC][%10lu]%s", g_mem_malloc_size, trace);
		}
#endif
#endif
	}
	if (p == NULL)
	{
		aw_log_err("[%s][%d] malloc failed %d ", function, line, size);
		aw_mem_dump();
		aw_log_err("APP EXIT");
		exit(0);
	}


	return p;
}

void aw_free(void *p, const char *function, int line)
{

#ifdef AW_MEM_CHECK
	if (g_mem_hook_flag)
	{
		aw_mutex_lock(&g_mem_lock);
		map<void * ,string >::iterator it;
		it = g_mem_map.find(p);
		if(it==g_mem_map.end())
		{
			aw_log_err("free [%p] [%s] [%d] error failed", p, function, line);
		}
		else 
		{
			unsigned int size = 0;
			sscanf(it->second.c_str(), "[%u]", &size);
			g_mem_malloc_size -= size;
			g_mem_map.erase(it);

#ifdef AW_MEM_TRACE
			if (g_mem_trace_flag)
			{
				aw_log_info("[MEM TRACE][  FREE][%10lu]%s", g_mem_malloc_size, it->second.c_str());
			}
#endif
		}

		aw_mutex_unlock(&g_mem_lock);
	}
#endif
	if (p)
		free(p);

}
void aw_sync()
{
#ifndef _WINDOWS
	sync();
#endif
}


