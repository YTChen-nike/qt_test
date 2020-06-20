#ifndef __AW_IPC_H__
#define __AW_IPC_H__


#define AW_IPC_TYPE_SOCKET 0           // raw socket
#define AW_IPC_TYPE_UNIX_SOCKET 2           // local socket


//#define AW_IPC_TYPE_PUB 2				// broadcast

typedef void (*aw_ipc_recv_cb)(void *data, int size, void *context);


class aw_ipc{
public:
    aw_ipc();
    int SetTimeout(int s);
    int SetRecvBufSize(int maxRecvBufSize, int socketRecvBufSize);
    int SetThreadName(const char *name);
	int Send(const void * data, int size);
	int Recv(void * data, int size);
	void SetRecvCb(aw_ipc_recv_cb cb, void* context);
	void SetEndMark(char end_mark);
	~aw_ipc();
	int mFd;
	int mListenFd;
    int mPort;
    int mType;
    int mState;
    int mRole;
    char mUrl[256];
    char mThreadName[32];
	aw_ipc_recv_cb mCallback;
	void* mThreadHandle;
	void* mThreadContext;
    int mMaxRecvBufSize;
    int mSocketRecvBufSize;
	char mEndMark = 0;
private:

};

class aw_ipc_server:public aw_ipc{
public:
	static aw_ipc_server* Create(int type);
	int Bind(const char *url);
};

class aw_ipc_client:public aw_ipc{
public:

	static aw_ipc_client* Create(int type);
	int Connect(const char *url);
	int Connect(const char *url, int timeout_ms);

	int Send(const void * data, int size);

private:
};

#endif
