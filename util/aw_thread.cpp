#define _WINDOWS
#include "aw_thread.h"
#include "aw_util.h"
#define TRUE 1
#define FALSE 0

typedef struct tag_aw_thread_param{
	aw_thread_handle *h;
	aw_thread_cb cb;
	void *context;
}aw_thread_param;
static void * _aw_thread_cb(void * context)
{
	aw_thread_handle *handle = (aw_thread_handle *)context;
	handle->cb(handle->context);
	handle->init = FALSE;
	return 0;
}

int aw_thread_create(aw_thread_handle *h, aw_thread_cb cb, void * context)
{
	int ret = 0;
	if (h == NULL || cb == NULL)
	{
		return -1;
	}
	aw_thread_handle handle;
	handle.init = TRUE;
	handle.cb = cb;
	handle.context = context;
	*h = handle;
	
#ifdef _WINDOWS
	handle.handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_aw_thread_cb, h, 0, NULL);
#else
	pthread_t h2;
	ret=pthread_create(&h2, NULL, (void* (*)(void*))_aw_thread_cb, h);
	handle.handle  = h2;
#endif
	*h = handle;
	
	return ret;
}

int aw_thread_set_name(aw_thread_handle h, const char *name)
{
#ifdef _WINDOWS

#else
	pthread_setname_np(h.handle, name);
#endif
	return 0;
}

int aw_thread_is_running(aw_thread_handle h)
{
	return h.init == TRUE;
}

int aw_thread_wait(aw_thread_handle h)
{
	int ret ;
#ifdef _WINDOWS
	ret = WaitForSingleObject(h.handle, INFINITE);
#else
	ret = pthread_join(h.handle, NULL);
#endif
	h.init = FALSE;

	return ret;
}

int aw_thread_detach(aw_thread_handle h)
{
	int ret = 0;
#ifdef _WINDOWS
#else
	pthread_detach(h.handle);
#endif

	return ret;
}

