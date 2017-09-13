#pragma once

#define INVALID_SOCKET -1
#define INVALID_NETID  -1
#define RCV_BUF_SZ		1024*16

#include <string>
#include <winsock2.h>
#include <map>
#include <queue>
#include <mutex>

enum NetEventType
{
	CONNECT_FAILED,
	CONNECT_SUCC,
	RECEIVE_MSG,
	CONNECT_CLOSED,
};

struct NetEvent
{
	int				mNetID;
	NetEventType	mType;
	std::string		mMsg;
};

struct MsgHead
{
	unsigned int dwType;				// 请求类型ID
	unsigned int dwLength;				// 实际数据长度(不包括本请求头的长度2个unsigned int)
};

class NetMgr;
class Socket
{
public:
	enum Status
	{
		INIT		= 0,
		CONNECTING	= 1,
		CONNECTED	= 2,
		CLOSED		= 3,
	};

	Socket(int NetID, int FD, Status status, NetMgr* mgr);

	void Run();

	void Loop();

	void Receive();

	void Send();

	void PushMsg(std::string&& data);

	std::string PopMsg();

public:
	int						mNetID;
	int						mFd;
	Status					mStatus;
	NetMgr*					mMgr;

	std::queue<std::string> mMsgList;
	std::mutex				mListMutex;
	bool					mClosed;

	char					mBuf[RCV_BUF_SZ];
	int						mCursor;
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class NetMgr
{
public:
	static NetMgr* GetInstance();

	bool Init();

	int CreateConnect(std::string Ip, short Port);

	void PushEvent(const NetEvent& e);

	NetEvent PopEvent();

	void SendData(int id, std::string& data);

private:
	int mNextNetID = 1;
	std::map<int, Socket*> mNetMap;

	std::queue<NetEvent>		mEventList;
	std::mutex					mListMutex;
};