#include "aw_log.h"
#include "aw_fsm.h"
#include "aw_util.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define FSM_THREAD_STATE_START 0
#define FSM_THREAD_STATE_RUNNING 1
#define FSM_THREAD_STATE_STOP 2

// mileseconds
#define FSM_EVT_WAIT_TIME_SLICE 5

#define FSM_EVT_INNER_TYPE_NORMAL 0
#define FSM_EVT_INNER_TYPE_SEQ 1
#define FSM_TAG "AW_FSM"
#define EVT_TIME_TAG "AW_FSM_EVT_TIME"

aw_fsm_evt::aw_fsm_evt()
{
}

aw_fsm_evt::aw_fsm_evt(int _type, void *_ctx, void *_ctx2, aw_fn_evt_release cb)
{
	type = _type;
	ctx = _ctx;
	ctx2 = _ctx2;
	evt_release = cb;
}


aw_fsm::~aw_fsm()
{
	if (!IsStop())
	{
		Stop();
	}
}
int aw_fsm::Init(aw_fsm_table *table, int table_size)
{
	fsm_table_size = table_size;
	fsm_table = table;
	thread_state = FSM_THREAD_STATE_STOP;
	strncpy(thread_name, "thread_aw_fsm",sizeof(thread_name));
	evt_list.clear();
	evt_seq = 0;
	aw_mutex_init(&evt_mutex);
	return AW_RET_SUCC;
}

void ConsumeList(aw_fsm * fsm, list<aw_fsm_evt> *evt_list)
{
	if (evt_list->empty())
	{
		return;
	}

	aw_fsm_evt evt;
	int isDelayEvt;
	aw_mutex_lock(&fsm->evt_mutex);
	evt = evt_list->front();

	// filter expired seq
	if (evt.inner_type == FSM_EVT_INNER_TYPE_SEQ && evt.seq < fsm->evt_seq )
	{
		aw_log_tag(FSM_TAG ,"[%s] filter evt %x %llu %llu",fsm->thread_name, evt.type, evt.seq, fsm->evt_seq );
		// 过期事件不处理，并释放事件资源
		if (evt.evt_release != NULL)
			evt.evt_release(&evt);
		evt_list->pop_front();
		aw_mutex_unlock(&fsm->evt_mutex);
		return;
	}
	isDelayEvt = evt.start_time > GetTickCount();
	// 未到处理时间，且队列中不只一个消息，先出队再入队。
	if (isDelayEvt)
	{
		if (evt_list->size() > 1)
		{
			evt_list->pop_front();
			evt_list->push_back(evt);
//				aw_log_info("pop evt %x", evt.type);

		}
//			aw_log_info("delay evt %x", evt.type);
	}
	else{
		evt_list->pop_front();
	}
	aw_mutex_unlock(&fsm->evt_mutex);

	if (!isDelayEvt)
	{
		aw_log_tag(FSM_TAG, "[%s] total %d process evt %x %llu",fsm->thread_name, evt_list->size(), evt.type, fsm->evt_seq);

		// 时序事件处理前先更新状态机时序
		if (evt.inner_type == FSM_EVT_INNER_TYPE_SEQ)
		{
			fsm->evt_seq++;
		}
		fsm->ProcessEvt(evt);
		// 处理完成后释放事件资源
		if (evt.evt_release != NULL)
			evt.evt_release(&evt);
	}
}

int aw_fsm_thread(void * context)
{


	aw_fsm *fsm = (aw_fsm *)context;
	aw_log_info("[%s] thread start",fsm->thread_name);
	fsm->thread_state = FSM_THREAD_STATE_RUNNING;
	while (!fsm->IsStop())
	{
		usleep(1000*FSM_EVT_WAIT_TIME_SLICE);
		if (fsm->evt_list_priority.size() > 100)
		{
			aw_log_warn("[%s] priority evt list size(%d) beyond 100", fsm->thread_name, fsm->evt_list_priority.size());
		}
		ConsumeList(fsm, &fsm->evt_list_priority);
		
		if (fsm->evt_list.size() > 100)
		{
			aw_log_warn("[%s] evt list size(%d) beyond 100", fsm->thread_name, fsm->evt_list.size());
		}
		ConsumeList(fsm, &fsm->evt_list);
	}



	// 释放队列中所有的事件
	while(!fsm->evt_list_priority.empty())
	{
		aw_fsm_evt evt;

		evt = fsm->evt_list_priority.front();

		if (evt.evt_release)
			evt.evt_release(&evt);
		fsm->evt_list_priority.pop_front();
	}
	while(!fsm->evt_list.empty())
	{
		aw_fsm_evt evt;

		evt = fsm->evt_list.front();

		if (evt.evt_release)
			evt.evt_release(&evt);
		fsm->evt_list.pop_front();
	}




	aw_log_info("[%s] thread stop", fsm->thread_name);

	return AW_RET_SUCC;
}

int aw_fsm::Debug(int type)
{
	if (type == AW_FSM_DEBUG_EXPORT_EVT_NUMBER)
	{
		aw_mutex_lock(&evt_mutex);
		aw_log_info("fsm[%s] %d evts in queue", thread_name, evt_list.size());
		aw_log_info("fsm[%s] %d evts in priority queue", thread_name, evt_list_priority.size());
		aw_mutex_unlock(&evt_mutex);
	}
	else if (type == AW_FSM_DEBUG_EXPORT_ALL_EVT){
		aw_mutex_lock(&evt_mutex);
		aw_log_info("fsm[%s] %d evts in queue", thread_name, evt_list.size());
		for(list<aw_fsm_evt>::iterator it= evt_list.begin(); it != evt_list.end();)
		{
			aw_fsm_evt evt = *it;
			aw_log_info("fsm[%s] queue:evt type(0x%x) start_time(%lu) id(%d)", thread_name, evt.type, evt.start_time, evt.id);
    	    it++;
		}
		aw_log_info("fsm[%s] %d evts in priority queue", thread_name, evt_list_priority.size());
		for(list<aw_fsm_evt>::iterator it= evt_list_priority.begin(); it != evt_list_priority.end();)
		{
			aw_fsm_evt evt = *it;
			aw_log_info("fsm[%s] priority queue:evt type(0x%x) start_time(%lu) id(%d)", thread_name, evt.type, evt.start_time, evt.id);
    	    it++;
		}
		aw_mutex_unlock(&evt_mutex);
	}
}
void aw_fsm::clear_evt()
{
	while(!evt_list.empty())
	{
		aw_fsm_evt evt;

		evt = evt_list.front();

		if (evt.evt_release)
			evt.evt_release(&evt);
		evt_list.pop_front();
	}

}

int aw_fsm::SetThreadName(const char *name)
{
	strncpy(thread_name, name, sizeof(thread_name));
	return 0;
}
int aw_fsm::Start(int state)
{
	if (thread_state == FSM_THREAD_STATE_STOP)
	{
		thread_state = FSM_THREAD_STATE_START;
		fsm_state = state;
		aw_thread_create(&thread_handle, aw_fsm_thread, this);
		aw_thread_set_name(thread_handle, thread_name);
		usleep(1000*1);
		while(thread_state != FSM_THREAD_STATE_RUNNING)
		{
			usleep(1000*1);
			aw_log_info("[%s] wait thread running", thread_name);
		}
		return AW_RET_SUCC;
	}
	else
	{
		aw_log_err("[%s] start failed, fsm is running", thread_name);
		return AW_RET_ERR;
	}
}
int aw_fsm::IsStop()
{
	return thread_state == FSM_THREAD_STATE_STOP;
}
int aw_fsm::Stop()
{
	if (thread_state == FSM_THREAD_STATE_RUNNING)
	{
		thread_state = FSM_THREAD_STATE_STOP;
		
		aw_log_info("[%s] start wait thread", thread_name);
		aw_thread_wait(thread_handle);
		aw_log_info("[%s] end wait thread", thread_name);
		return 0;
	}
	else{
		aw_log_err("[%s] stop failed, fsm is stopped", thread_name);
		return AW_RET_ERR;
	}
}

int aw_fsm::ChangeState (int state)
{

	if (fsm_state  != state)
	{
		aw_log_info("[%s] change state 0x%x to 0x%x ", thread_name, fsm_state, state);
		fsm_state = state;
	}
	return AW_RET_SUCC;
}

int aw_fsm::DelayActiveSeqEvt(aw_fsm_evt e, int ms)
{
	DelayActiveSeqEvt(e,ms,evt_seq);
	return AW_RET_SUCC;
}

int aw_fsm::ActiveSeqEvt (aw_fsm_evt e)
{
	ActiveSeqEvt(e, evt_seq);
	return AW_RET_SUCC;
}

int aw_fsm::DelayActiveSeqEvt(aw_fsm_evt e, int ms, unsigned long long seq)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time = GetTickCount() + ms;
	e.seq = seq;
	e.inner_type = FSM_EVT_INNER_TYPE_SEQ;
	e.id = evt_id++;
	evt_list.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu", thread_name, e.id);
	return AW_RET_SUCC;
}

int aw_fsm::ActiveSeqEvt (aw_fsm_evt e, unsigned long long seq)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time= GetTickCount();
	e.seq = seq;
	e.inner_type = FSM_EVT_INNER_TYPE_SEQ;
	e.id = evt_id++;
	evt_list.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu", thread_name, e.id);
	return AW_RET_SUCC;
}
int aw_fsm::DelayActiveEvt(aw_fsm_evt e, int ms)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time = GetTickCount() + ms;
	e.inner_type = FSM_EVT_INNER_TYPE_NORMAL;
	e.id = evt_id++;
	evt_list.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu", thread_name, e.id);
	return AW_RET_SUCC;
}

int aw_fsm::ActiveEvt (aw_fsm_evt e)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time= GetTickCount();
	e.inner_type = FSM_EVT_INNER_TYPE_NORMAL;
	e.id = evt_id++;
	evt_list.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu type :0x%x", thread_name, e.id, e.type);
	return AW_RET_SUCC;
}

int aw_fsm::PrioritySeqEvt(aw_fsm_evt e)
{
	PrioritySeqEvt(e, evt_seq);
	return AW_RET_SUCC;
}

int aw_fsm::DelayPrioritySeqEvt(aw_fsm_evt e, int ms)
{
	DelayPrioritySeqEvt(e, ms, evt_seq);
	return AW_RET_SUCC;
}

int aw_fsm::DelayPriorityEvt(aw_fsm_evt e, int ms)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time= GetTickCount() + ms;
	e.inner_type = FSM_EVT_INNER_TYPE_NORMAL;
	e.id = evt_id++;
	evt_list_priority.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu type :0x%x", thread_name, e.id, e.type);
	return AW_RET_SUCC;
}

int aw_fsm::PriorityEvt(aw_fsm_evt e)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time= GetTickCount();
	e.inner_type = FSM_EVT_INNER_TYPE_NORMAL;
	e.id = evt_id++;
	evt_list_priority.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu type :0x%x", thread_name, e.id, e.type);
	return AW_RET_SUCC;
}

int aw_fsm::DelayPrioritySeqEvt(aw_fsm_evt e, int ms, unsigned long long seq)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time= GetTickCount() + ms;
	e.inner_type = FSM_EVT_INNER_TYPE_NORMAL;
	e.id = evt_id++;
	evt_list_priority.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu type :0x%x", thread_name, e.id, e.type);
	return AW_RET_SUCC;
}

int aw_fsm::PrioritySeqEvt(aw_fsm_evt e, unsigned long long seq)
{
	aw_mutex_lock(&evt_mutex);
	e.start_time= GetTickCount();
	e.seq = seq;
	e.inner_type = FSM_EVT_INNER_TYPE_NORMAL;
	e.id = evt_id++;
	evt_list_priority.push_back(e);
	aw_mutex_unlock(&evt_mutex);
	aw_log_tag(EVT_TIME_TAG, "[%s] active evt %lu type :0x%x", thread_name, e.id, e.type);
	return AW_RET_SUCC;
}

int aw_fsm::ProcessEvt(aw_fsm_evt e)
{
	if (fsm_table != NULL || fsm_table_size <= 0)
	{

	}
	for (int i = 0; i < fsm_table_size; i++)
	{
		if (fsm_table[i].evt_type == e.type && fsm_table[i].cur_state == fsm_state)
		{
			unsigned long time = GetTickCount();
			if (aw_tag_check(EVT_TIME_TAG))
			{
				aw_log_tag(EVT_TIME_TAG, "[%s] process evt %ld", thread_name, e.id);
			}
			int ret = fsm_table[i].evt_process(e, fsm_state);
			if (aw_tag_check(EVT_TIME_TAG))
			{
				char text[128];
				sprintf(text, "[%s] evt(id:%ld type:0x%x) process used %ldms", thread_name, e.id, e.type, GetTickCount() - time);
				aw_log_tag(EVT_TIME_TAG, text);
			}
			unsigned long used_time = GetTickCount() - time;
			if (used_time > 100)
			{
				aw_log_warn("[%s] evt(id:%ld type:0x%x) process used %lums", thread_name, e.id, e.type, used_time);
			}
			if (fsm_table[i].next_state != fsm_state && ret == AW_RET_SUCC)
			{
				ChangeState(fsm_table[i].next_state);
			}
			break;
		}
	}
	return AW_RET_SUCC;
}
