#ifndef __AW_FSM__
#define __AW_FSM__
#include <list>
#include "aw_thread.h"
#include "aw_util.h"
using namespace std;

typedef void (*aw_fn_evt_release)(void *evt);
class aw_fsm_evt{
public:
    aw_fsm_evt();
    aw_fsm_evt(int type, void *ctx = NULL, void *ctx2 = NULL, aw_fn_evt_release cb = NULL);
    int type;
    void *ctx;
    void *ctx2;
    unsigned long start_time;
    unsigned long id;
    unsigned long long seq;
    int inner_type;
    aw_fn_evt_release evt_release;
};

class aw_fsm_table
{
public:
    int evt_type;   //�¼�
    int cur_state;  //��ǰ״̬
    int next_state;  //��һ��״̬

    // ���� AW_RET_SUCC��ת��״̬��������ת��
    int (*evt_process)(aw_fsm_evt evt, int fsm_state);
};

#define AW_FSM_DEBUG_EXPORT_EVT_NUMBER 0
#define AW_FSM_DEBUG_EXPORT_ALL_EVT 1


class aw_fsm{
public:
    ~aw_fsm();
    int Init(aw_fsm_table *table, int table_size);
    int Start(int init_state);
    int Stop();
    int IsStop();
    int GetState();
    int ActiveEvt(aw_fsm_evt e);
    int DelayActiveEvt(aw_fsm_evt e, int ms);
	int PriorityEvt(aw_fsm_evt e);
	int DelayPriorityEvt(aw_fsm_evt e, int ms);
	void clear_evt();
public:
    //
/*
�����ʱ����¼���״̬��������һ����ʱ����¼���״̬��ʱ�����ӣ����ٴ���֮ǰʱ����¼�
��ҪӦ�����¼�ȥ�غ��¼�ʱЧ�Լ��
          �����ʱ���¼�  |    ״̬��         |   �¼�������
--------------------------|-------------------|--------------
            |             |    fsm(seq=1)     |
ActiveSeqEvt(e1(seq=1))------> fsm(seq=1)     |
            |             |    fsm(seq=1)-------> evt_process(e1(seq=1))
            |             |    fsm(seq=2)<-------
ActiveSeqEvt(e2(seq=2))------> fsm(seq=2)     |
ActiveSeqEvt(e3(seq=2))------> fsm(seq=2)     |
            |             |    fsm(seq=2)-------> evt_process(e2(seq=2))
            |             |    fsm(seq=3)<-------
            |             |    fsm(seq=3)-------> ignore e3(seq=2)
            |             |    fsm(seq=3)     |

*/
    int ActiveSeqEvt(aw_fsm_evt e);
    int DelayActiveSeqEvt(aw_fsm_evt e, int ms);
	int PrioritySeqEvt(aw_fsm_evt e);
	int DelayPrioritySeqEvt(aw_fsm_evt e, int ms);

// �Զ��弤���¼������к�
    int ActiveSeqEvt(aw_fsm_evt e, unsigned long long evt_seq);
    int DelayActiveSeqEvt(aw_fsm_evt e, int ms, unsigned long long evt_seq);
	int PrioritySeqEvt(aw_fsm_evt e, unsigned long long seq);
	int DelayPrioritySeqEvt(aw_fsm_evt e, int ms, unsigned long long seq);

    int ChangeState(int state);
    int ProcessEvt (aw_fsm_evt e);
    int SetThreadName(const char *name);
    int Debug(int type);
    char thread_name[32];
    list<aw_fsm_evt> evt_list;
    list<aw_fsm_evt> evt_list_priority;

    int fsm_state;
    aw_fsm_table *fsm_table;
    int fsm_table_size;
    int thread_state;
    aw_thread_handle thread_handle;
    aw_mutex_t evt_mutex;
    unsigned long long evt_seq;
    unsigned long evt_id;

};
#endif
