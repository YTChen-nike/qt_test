#pragma once

typedef int (*aw_thread_cb)(void * context);
#ifdef _WINDOWS
#include <Windows.h>
#else
#include <pthread.h>
#endif
typedef struct _aw_thread_handle
{

#if (defined _WINDOWS)
	HANDLE handle;
#else
	pthread_t handle;
#endif
    int init;
    void * context;
	aw_thread_cb cb;
}aw_thread_handle;

int aw_thread_create(aw_thread_handle *h, aw_thread_cb cb, void * context);
int aw_thread_wait(aw_thread_handle h);
int aw_thread_detach(aw_thread_handle h);

int aw_thread_set_name(aw_thread_handle h, const char *name);
int aw_thread_is_running(aw_thread_handle h);

