#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "aw_util.h"
#include "aw_log.h"
#include "aw_ipc.h"
#include "aw_thread.h"
#define _WINDOWS
#ifdef _WINDOWS
#include <winsock2.h>
#include "windows.h"
#define MSG_NOSIGNAL 0
#define close closesocket
#define SHUT_RDWR SD_BOTH
#define socklen_t int
#define AW_SOCKET_PROTO IPPROTO_TCP
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "netinet/in.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#define AW_SOCKET_PROTO 0
#endif
#define TAG_IPC "AW_IPC"
//ceshi
#include <QDebug>


#ifndef MIN
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#endif

#define AW_IPC_ROLE_SERVER 0
#define AW_IPC_ROLE_CLIENT  1

#define AW_IPC_STATE_IDLE 0
#define AW_IPC_STATE_CONNECTED 1
#define AW_IPC_STATE_EXIT 2

#define DEFAULT_MAX_RECV_BUF_LEN		(2*1024*1024)
#define DEFAULT_RECV_TEMP_BUF_LEN		(1*1024*1024)

#define INVALID_FD (-1)

#define IPC_WORKSPACE "/data/"
aw_ipc::aw_ipc()
{
	mMaxRecvBufSize = DEFAULT_MAX_RECV_BUF_LEN;
	mSocketRecvBufSize = DEFAULT_RECV_TEMP_BUF_LEN;
}

int aw_ipc::SetThreadName(const char *name)
{
	strncpy(mThreadName, name, sizeof(mThreadName));
	return 0;
}

int aw_ipc::SetRecvBufSize(int maxRecvBufSize, int socketRecvBufSize)
{
	mMaxRecvBufSize	= maxRecvBufSize;
	mSocketRecvBufSize	= socketRecvBufSize;
	return 0;
}

int aw_ipc::SetTimeout(int second)
{
	if (mType == AW_IPC_TYPE_SOCKET)
	{
		return AW_RET_SUCC;
	}
	else
	{
		int ret = 0;
		return ret;
	}
}
int aw_ipc::Send(const void * data, int size)
{
    if (mFd < 0)
    {
//        aw_log_err("send failed(fd is invalid)\n");
        return AW_RET_ERR;
    }
	if (mType == AW_IPC_TYPE_SOCKET || mType == AW_IPC_TYPE_UNIX_SOCKET)
	{
		aw_log_tag(TAG_IPC, "[%s]ipc send start", mThreadName);
		int lastSize = size;
        //aw_log_err("[socket] send %d %s\n", size, data);
        while(lastSize >0)
       	{
       		int sendSize = size;
			/*
       		if (lastSize > 1024)
       		{
	       		sendSize = 1024;
       		}else{
				sendSize = lastSize;
			}
			*/
			send(mFd, (const char *)data, sendSize, MSG_NOSIGNAL);
			lastSize -= sendSize;
			data = (char *)data + sendSize;
		}
		aw_log_tag(TAG_IPC, "[%s]ipc send finish %d", mThreadName, size);
		return size;
	}
	else{
		int rc = AW_RET_SUCC;
	    return rc;
	}

}
int aw_ipc::Recv(void * data, int size)
{
	int rc = AW_RET_SUCC;
	if (mType == AW_IPC_TYPE_SOCKET || mType == AW_IPC_TYPE_UNIX_SOCKET)
	{

		aw_log_tag(TAG_IPC, "[%s]ipc recv start", mThreadName);
		rc = recv(mFd, (char *)data, size, 0);
		aw_log_tag(TAG_IPC, "[%s]ipc recv finish %d", mThreadName, rc);

		if (rc <= 0)
		{
			close(mFd);
			mFd = INVALID_FD;
		}
		return rc;
	}
	else
	{
	}

    return rc;
}
aw_ipc::~aw_ipc(){
	SetRecvCb(NULL, NULL);
	mState = AW_IPC_STATE_EXIT;
	//if(mType == AW_IPC_TYPE_SOCKET)
	{

		if (mFd >= 0)
		{
			// aw_log_info("close %d socket ", mFd);
#ifndef _WINDOWS
			shutdown(mFd, SHUT_RDWR);
#endif
//ceshi
#ifdef _WINDOWS
            WSADATA wsaData;
            if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 )
            {
                WSACleanup( );
                return;
            }
#endif
			close(mFd);
			mFd = INVALID_FD;
		}
		if (mListenFd >= 0)
		{
#ifndef _WINDOWS
			shutdown(mListenFd, SHUT_RDWR);
#endif
//ceshi
#ifdef _WINDOWS
            WSADATA wsaData;
            if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 )
            {
                WSACleanup( );
                return;
            }
#endif
			close(mListenFd);
			mListenFd = INVALID_FD;
		}
	}

	if (mThreadHandle)
	{
		aw_thread_wait(*(aw_thread_handle *)mThreadHandle);
		AW_FREE(mThreadHandle);
	}
}



static int _aw_recv_thread_cb(void * context)
{

	aw_ipc * ipc = (aw_ipc *)context;
	aw_log_info("[ipc] thread(%s) start", ipc->mThreadName);
	char *buf = NULL;
	char *temp_buf = NULL;

	//if (ipc->mType == AW_IPC_TYPE_SOCKET || ipc->mType == AW_IPC_TYPE_UNIX_SOCKET)
	{

		int temp_len = 0;
		int recv_len = 0;
		int i;

		buf = (char *)AW_MALLOC(ipc->mMaxRecvBufSize);
		if (buf == NULL)
		{
    		aw_log_err("malloc buf %d failed", ipc->mMaxRecvBufSize);
			goto EXIT;
		}
		temp_buf = (char *)AW_MALLOC(ipc->mSocketRecvBufSize);
		if (temp_buf == NULL)
		{
    		aw_log_err("malloc temp_buf %d failed", ipc->mSocketRecvBufSize);
			goto EXIT;
		}
		for(;;)
		{
			if (ipc->mState == AW_IPC_STATE_EXIT)
			{
				goto EXIT;
			}

			if (ipc->mRole == AW_IPC_ROLE_SERVER)
			{
				if (ipc->mFd == INVALID_FD)
				{
					// reinit listen socket
					switch(ipc->mType)
					{

						case AW_IPC_TYPE_SOCKET:
						{
							if (ipc->mListenFd == INVALID_FD)
							{

								aw_ipc_server *ipc_server = (aw_ipc_server *)ipc;
								ipc->mListenFd = socket(AF_INET, SOCK_STREAM, 0);
								int val = 1;
								setsockopt(ipc->mListenFd, SOL_SOCKET, SO_REUSEADDR, (const char *)&val, sizeof(val));
								ipc_server->Bind(ipc->mUrl);
							}
							struct sockaddr_in addr;
							memset(&addr, 0, sizeof(addr));
							addr.sin_family = AF_INET;
							addr.sin_addr.s_addr = INADDR_ANY;
							addr.sin_port = htons(ipc->mPort);
							socklen_t addr_len = sizeof(addr);
							ipc->mFd = accept(ipc->mListenFd, (struct sockaddr *)&addr, &addr_len);
						}
						break;
#ifndef _WINDOWS
						case AW_IPC_TYPE_UNIX_SOCKET:
							{
								if (ipc->mListenFd == INVALID_FD)
								{
									aw_ipc_server *ipc_server = (aw_ipc_server *)ipc;
									ipc->mListenFd = socket(AF_UNIX, SOCK_STREAM, 0);
									int val = 1;
                                    setsockopt(ipc->mListenFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val));
									ipc_server->Bind(ipc->mUrl);
								}
								ipc->mFd = accept(ipc->mListenFd, NULL, NULL);
							}
							break;
#endif
					}

					// stop listen, only keep one socket connected
					close(ipc->mListenFd);
					ipc->mListenFd = INVALID_FD;
					aw_log_info("accept new socket");
				}
				else{

				}
				ipc->mState = AW_IPC_STATE_CONNECTED;
			}
			else if (ipc->mRole == AW_IPC_ROLE_CLIENT)
			{
				if (ipc->mFd == INVALID_FD){
					switch(ipc->mType){

						case AW_IPC_TYPE_SOCKET:
						{
							ipc->mFd = socket(AF_INET, SOCK_STREAM, AW_SOCKET_PROTO);
							int val = 1;
							setsockopt(ipc->mFd, SOL_SOCKET, SO_REUSEADDR, (const char *)&val, sizeof(val));
						}
						break;
#ifndef _WINDOWS
						case AW_IPC_TYPE_UNIX_SOCKET:
						{
							ipc->mFd = socket(AF_UNIX, SOCK_STREAM, 0);
							int val = 1;
                            setsockopt(ipc->mFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val));
						}

#endif
					}
					if (ipc->mState == AW_IPC_STATE_IDLE)
					{
						aw_sleep_ms(1000);
						aw_ipc_client *client = (aw_ipc_client *)ipc;
						client->Connect(client->mUrl);
						aw_log_info("Connect new socket");
						continue;
					}
				}
			}


			while (recv_len < ipc->mMaxRecvBufSize - 1)
			{

				if (temp_len <= 0)
				{

					//aw_log_info("startrecv %d socket ", ipc->mFd);

					temp_len = ipc->Recv(temp_buf,
						MIN(ipc->mSocketRecvBufSize, (ipc->mMaxRecvBufSize - 1 - recv_len)));
					aw_log_tag(TAG_IPC, "[%s]recv %d url %s\n" , ipc->mThreadName, temp_len, ipc->mUrl);
/*
					char *t = temp_buf;
					while (temp_buf +  temp_len > t)
					{
						aw_log_err("[socket] recv %d %d %s\n", temp_len, strlen(t) + 1,t);
						t = t+ strlen(t) + 1;
					}
*/
					//aw_log_info("end recv %d socket ", ipc->mFd);
					if (ipc->mState == AW_IPC_STATE_EXIT)
					{
						goto EXIT;
						return AW_RET_ERR;
					}

					if (temp_len <= 0)
					{
						close(ipc->mFd);
						ipc->mFd = INVALID_FD;
						ipc->mState = AW_IPC_STATE_IDLE;
						aw_log_info("[ipc] recv %d socket closed url %s", temp_len,ipc->mUrl);
						break;
					}
				}

				if(ipc->mEndMark == 0){
				i = strnlen(temp_buf,temp_len);
				}
				else{
					i = (int)(strchr(temp_buf,ipc->mEndMark) - temp_buf);
				}
/*
				for (i=0; i<temp_len; i++)
				{

					if (0x00 == temp_buf[i])
						break;
				}
				*/

				if (i < temp_len)
				{
					if (i > 0)
					{
						memcpy(buf + recv_len, temp_buf, i);
						recv_len += i;
					}

					temp_len -= i + 1;
					memmove(temp_buf, temp_buf + i + 1, temp_len);
					if (recv_len > 0)
						break;
				}
				else
				{

					memcpy(buf + recv_len, temp_buf, temp_len);
					recv_len += temp_len;
					temp_len = 0;
				}

			}

			if (recv_len > 0)
				buf[recv_len] = '\0';

			if (recv_len)
			{
				aw_log_tag(TAG_IPC, "[%s]callback+ %d\n", ipc->mThreadName ,recv_len + 1);
				ipc->mCallback(buf, recv_len + 1, ipc->mThreadContext);
				aw_log_tag(TAG_IPC, "[%s]callback- %d\n", ipc->mThreadName ,recv_len + 1);

			}else
			{
				ipc->mCallback(NULL, 0, ipc->mThreadContext);
			}
			recv_len = 0;

		}
	}
EXIT:
	AW_FREE(buf);
	AW_FREE(temp_buf);


	aw_log_info("[ipc] thread(%s) stop", ipc->mThreadName);
	return AW_RET_SUCC;
}

void aw_ipc::SetRecvCb(aw_ipc_recv_cb cb, void *context)
{
	mCallback = cb;
	mThreadContext = context;
	if (mThreadHandle == NULL )
	{
		mThreadHandle = (aw_thread_handle *)AW_MALLOC(sizeof(aw_thread_handle));
		if (mThreadHandle == NULL)
		{
			aw_log_err("malloc thread handle failed");
	     	return ;
		}
		memset(mThreadHandle, 0, sizeof(aw_thread_handle));
		aw_thread_create((aw_thread_handle *)mThreadHandle, _aw_recv_thread_cb, (void *)this);
		aw_thread_set_name(*(aw_thread_handle *)mThreadHandle, mThreadName);

	}
}

aw_ipc_server* aw_ipc_server::Create(int type)
{

		int fd;
		switch(type)
		{

			case AW_IPC_TYPE_SOCKET:
				{
//ceshi
#ifdef _WINDOWS
                    WORD wVersionRequested;
                    WSADATA wsaData;
                    int err;
                    wVersionRequested = MAKEWORD( 1, 1 );
                    err = WSAStartup(wVersionRequested, &wsaData);
                    if(err != 0){
                        perror("WSAStartup error");
                    }
#endif
					fd = socket(AF_INET, SOCK_STREAM, AW_SOCKET_PROTO);
                    //ceshi
                    qDebug() << "fd:" << fd;
					int val = 1;
					setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&val, sizeof(val));
					int sock_buf_size = 10*1024*1024;

					setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
					setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));

					break;
				}
#ifndef _WINDOWS
			case AW_IPC_TYPE_UNIX_SOCKET:
				{
					fd= socket(AF_UNIX, SOCK_STREAM, 0);
					int sock_buf_size = 10*1024*1024;
					setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
					setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
					break;
				}
#endif
			default:
				aw_log_err("invalid type %d\n", type);
					return NULL;
		}

    if (fd < 0) {
        aw_log_err("socket init failed\n");
        return NULL;
    }
    aw_ipc_server* server = new aw_ipc_server();
	server->mType = type;

	server->mFd = INVALID_FD;
	server->mListenFd = fd;
	server->mType = type;
	server->mRole = AW_IPC_ROLE_SERVER;
    server->mCallback = NULL;
    return server;
}



int aw_ipc_server::Bind(const char *url)
{
    if (mListenFd < 0)
    {
        aw_log_err("fd is invalid\n");
        return AW_RET_ERR;
    }
	if (mType == AW_IPC_TYPE_SOCKET)
	{
		const char *pPort = strrchr(url, ':');
		if (pPort == NULL)
		{
	        aw_log_err("url(%s) without port\n", url);
			return AW_RET_ERR;
		}
		mPort = atoi(pPort + 1);

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(mPort);
		aw_log_info("bind port %d ",  mPort);

		if (-1 == bind(mListenFd, (struct sockaddr *)&addr, sizeof(addr)))
		{
			aw_log_err("bind failed");
	        return AW_RET_ERR;
		}
		if (-1 == listen(mListenFd, 2))
		{
			aw_log_err("listen failed");
	        return AW_RET_ERR;
		}

	}
#ifndef _WINDOWS
	else if (mType == AW_IPC_TYPE_UNIX_SOCKET)
	{

		int fd;
		char path[128];


		const char *pPort = strrchr(url, ':');
		if (pPort == NULL)
		{
	        aw_log_err("url(%s) without port\n", url);
			return AW_RET_ERR;
		}
		pPort = pPort + 1;

		sprintf(path,"%s/%s",IPC_WORKSPACE,pPort);
		// printf("%s\n",path);
		struct sockaddr_un un;
		int len, rval;
		unlink(path); 			  /* in case it already exists */
		memset(&un, 0, sizeof(un));
		un.sun_family = AF_UNIX;
		strcpy(un.sun_path, path);
		if (bind(mListenFd, (struct sockaddr *)&un, sizeof(un)) < 0)
		{
				aw_log_err("bind failed");
				return AW_RET_ERR;
		}
		if (-1 == listen(mListenFd, 2))
		{
			aw_log_err("listen failed");
	        return AW_RET_ERR;
		}

	}
#endif
	if (mUrl != url)
	{
		strncpy(mUrl, url, sizeof(mUrl));
	}

    return AW_RET_SUCC;
}

aw_ipc_client* aw_ipc_client::Create(int type)
{
	int fd;
	switch(type)
	{
		case AW_IPC_TYPE_SOCKET:
			{
				fd= socket(AF_INET, SOCK_STREAM, AW_SOCKET_PROTO);
				int sock_buf_size = 10*1024*1024;
				setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
				setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
			}
			break;

			case AW_IPC_TYPE_UNIX_SOCKET:
				{
					fd= socket(AF_UNIX, SOCK_STREAM, 0);
					int sock_buf_size = 10*1024*1024;
					setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
					setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
				}
			break;
		default:
			aw_log_err("invalid type %d\n", type);
				return NULL;
	}

    if (fd < 0) {
        aw_log_err("socket init failed\n");
        return NULL;
    }
    aw_ipc_client* client = new aw_ipc_client();
    client->mFd = fd;
	client->mListenFd = INVALID_FD;
	client->mType = type;
	client->mRole = AW_IPC_ROLE_CLIENT;
	client->mState = AW_IPC_STATE_IDLE;

    return client;
}

int aw_ipc_client::Connect(const char *url, int timeout_ms)
{
	unsigned long tick = AwGetTickCount();
	int ret = -1;
	while(timeout_ms < 0 || AwGetTickCount() - tick < timeout_ms)
	{
		ret = Connect(url);
		if (ret == AW_RET_SUCC)
		{
			break;
		}
		aw_sleep_ms(1000);
	}
	return ret;
}
int aw_ipc_client::Connect(const char *url)
{

	strncpy(mUrl, url, sizeof(mUrl));
	if (mState == AW_IPC_STATE_CONNECTED)
	{
		return AW_RET_SUCC;
	}
	if (mType == AW_IPC_TYPE_SOCKET)
	{

		char ip[32];
		const char *pIp = strstr(url, "tcp://");
		if (pIp == NULL)
		{
	        aw_log_err("url(%s) without ip\n", url);
			return -1;
		}

		strncpy(ip, pIp + strlen("tcp://"),sizeof(ip));
		char *pPort = strstr(ip, ":");
		if (pPort == NULL)
		{
	        aw_log_err("url(%s) without port\n", url);
			return AW_RET_ERR;
		}
		*pPort = '\0';

		mPort = atoi(pPort + 1);


    	struct hostent *hptr = gethostbyname(ip);
	    if(hptr == NULL)
		{
			aw_log_err("get host by name failed (%s)", ip);
			return AW_RET_ERR;
	    }

        // aw_log_info("IP addr : %s\n", inet_ntoa( *(struct in_addr*)hptr->h_addr_list[0] ) );

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
	    addr.sin_addr.s_addr = *((unsigned long*)hptr->h_addr_list[0]);
		//addr.sin_addr.s_addr =  htonl( htonl(inet_addr(ip)));
		addr.sin_port = htons(mPort);

		if (-1 == connect(mFd, (struct sockaddr *)&addr, sizeof(addr)))
		{
//			aw_log_err("connect failed");
			close(mFd);
			mFd = socket(AF_INET, SOCK_STREAM, 0);
			return AW_RET_ERR;
		}
		mState = AW_IPC_STATE_CONNECTED;
	}
#ifndef _WINDOWS
	else if (mType == AW_IPC_TYPE_UNIX_SOCKET)
	{

		char path[128];


		const char *pPort = strrchr(url, ':');
		if (pPort == NULL)
		{
			aw_log_err("url(%s) without port\n", url);
			return AW_RET_ERR;
		}
		pPort = pPort + 1;
		sprintf(path,"%s/%s",IPC_WORKSPACE,pPort);
		// printf("%s\n",path);
		int len;
		struct sockaddr_un un;
		memset(&un, 0, sizeof(un));
		un.sun_family = AF_UNIX;
		strcpy(un.sun_path, path);
		if (connect(mFd, (struct sockaddr *)&un, sizeof(un)) < 0)
		{
			aw_log_err("connect url(%s) failed\n", url);
			return AW_RET_ERR;
		}
		mState = AW_IPC_STATE_CONNECTED;
	}
#endif
    return AW_RET_SUCC;

}

int aw_ipc_client::Send(const void * data, int size)
{
	if (mType == AW_IPC_TYPE_SOCKET && mState == AW_IPC_STATE_IDLE)
	{
		aw_log_warn("mType == AW_IPC_TYPE_SOCKET && mState == AW_IPC_STATE_IDLE\n");
		return -2;
	}

	return this->aw_ipc::Send(data, size);
}
void aw_ipc::SetEndMark(char end_mark)
{
	mEndMark = end_mark;
}
