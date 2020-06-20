#ifndef __AW_HTTP_H__
#define __AW_HTTP_H__
#include "mongoose.h"
#include "aw_thread.h"
#include "aw_util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <map>
using namespace std;

typedef void (*fn_http_callback)(unsigned int id, int code, void *data, unsigned int len, void *ctx);
class aw_http_req_param{
public:
		unsigned int id;
	struct mg_connection *nc;
	fn_http_callback cb;
	void *ctx;
};

class aw_http_util{
public:
    aw_http_util();
    ~aw_http_util();
    unsigned int post(const char *url, char *post_data, fn_http_callback cb, void *ctx);
    struct mg_mgr m_mgr;
    aw_mutex_t m_mutex;
	aw_thread_handle m_thread_handle;
	int m_exit_flag;
	unsigned int m_req_id;
	map<void *, aw_http_req_param> m_req_map;
};
#endif
