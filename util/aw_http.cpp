#include <pthread.h>
#include "aw_util.h"
#include "aw_http.h"

static void _mg_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
	aw_http_util *http_util = (aw_http_util *)nc->mgr->user_data;
	//printf("nc %p ev %d\n", nc, ev);
	struct http_message *hm = (struct http_message *) ev_data;
	aw_http_req_param req_param = {0};
	if (MG_EV_HTTP_REPLY == ev ||MG_EV_CLOSE == ev)
	{
		map<void *, aw_http_req_param>::iterator iter;
		aw_mutex_lock(&http_util->m_mutex);
		iter = http_util->m_req_map.find(nc);
		if(iter == http_util->m_req_map.end()){
			aw_mutex_unlock(&http_util->m_mutex);
			return ;
		}
		else {
			req_param = iter->second;
		}
		aw_mutex_unlock(&http_util->m_mutex);
	}
	switch (ev) {
    case MG_EV_HTTP_REPLY:
	{
		nc->flags |= MG_F_CLOSE_IMMEDIATELY;
		int code = 0;
		if (req_param.cb)
		req_param.cb(req_param.id, code, (void *)hm->body.p, (unsigned int)hm->body.len, req_param.ctx);
		aw_mutex_lock(&http_util->m_mutex);
		http_util->m_req_map.erase(nc);
		aw_mutex_unlock(&http_util->m_mutex);
	}
  break;
    case MG_EV_CLOSE:
	{
    	int code = -1;
		if (req_param.cb)
		req_param.cb(req_param.id, code, (void *)0, (unsigned int)0, req_param.ctx);
		aw_mutex_lock(&http_util->m_mutex);
		http_util->m_req_map.erase(nc);
		aw_mutex_unlock(&http_util->m_mutex);
	}
      break;
    default:
      break;
  }
}

unsigned int aw_http_util::post(const char *url, char *post_data, fn_http_callback cb, void *ctx)
{
	struct mg_connection *c = mg_connect_http(&m_mgr, _mg_ev_handler, url, NULL, post_data);
	aw_http_req_param req_param;
	aw_mutex_lock(&m_mutex);
	unsigned int req_id = m_req_id++;
	req_param.id = req_id;
	req_param.nc = c;
	req_param.cb = cb;
	req_param.ctx = ctx;
	m_req_map[c] = req_param;
	aw_mutex_unlock(&m_mutex);
	return req_id;
}

int _mg_mgr_poll_thread(void * context)
{
	aw_http_util *util= (aw_http_util*)context;
	while (util->m_exit_flag == 0) {
		mg_mgr_poll(&util->m_mgr, 1000);
	}
	
	return 0;
}
aw_http_util::~aw_http_util()
{	
	m_exit_flag = 1;
	aw_thread_wait(m_thread_handle);
	m_req_map.clear();
	mg_mgr_free(&m_mgr);
}
aw_http_util::aw_http_util(){
	aw_mutex_init(&m_mutex);
	mg_mgr_init(&m_mgr, this);
	m_exit_flag = 0;
	aw_thread_create(&m_thread_handle, _mg_mgr_poll_thread, this);
	aw_thread_set_name(m_thread_handle, "aw_http_util");

}

