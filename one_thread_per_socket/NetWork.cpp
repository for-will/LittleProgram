#include "NetWork.h"
#include <WS2tcpip.h>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")

Socket::Socket(int id, int fd, Status status, NetMgr* mgr)
{
	mNetID = id;
	mFd = fd;
	mStatus = status;
	mMgr = mgr;
	mCursor = 0;
	mClosed = false;
}

void Socket::Run()
{
	auto t = std::thread(std::bind(&Socket::Loop, this));
	t.detach();
}

void Socket::Loop()
{
	fd_set readset;
	fd_set writeset;
	fd_set exceptset;
	timeval to;
	to.tv_sec = 0;
	to.tv_usec = 50 * 1000;
	while (!mClosed)
	{
		if (mStatus!=CONNECTED && mStatus!=CONNECTING)
		{
			break;
		}

		Send();

		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_ZERO(&exceptset);
		if (mStatus == CONNECTING)
		{
			FD_SET(mFd, &writeset);
			FD_SET(mFd, &exceptset);
		}
		if (mStatus == CONNECTED)
		{
			FD_SET(mFd, &readset);
		}

		int ret = select(mFd + 1, &readset, &writeset, &exceptset, &to);
		if (ret <= 0)
		{
			continue;
		}

		if (FD_ISSET(mFd, &readset))
		{
			Receive();
		}

		if (FD_ISSET(mFd, &writeset))
		{
			mStatus = CONNECTED;
			NetEvent e;
			e.mNetID = mNetID;
			e.mType = CONNECT_SUCC;
			mMgr->PushEvent(e);
		}

		if (FD_ISSET(mFd, &exceptset))
		{
			mStatus = CLOSED;
			NetEvent e;
			e.mNetID = mNetID;
			e.mType = CONNECT_FAILED;
			mMgr->PushEvent(e);
		}
	}
	closesocket(mFd);
	mStatus = CLOSED;
}

void Socket::Receive()
{
	while (true)
	{
		int ret = recv(mFd, mBuf + mCursor, RCV_BUF_SZ - mCursor, 0);
		if (ret < 0)
		{
			if (GetLastError() == WSAEWOULDBLOCK)
			{
				continue;
			}
			mStatus = CLOSED;

			NetEvent e;
			e.mNetID = mNetID;
			e.mType = CONNECT_CLOSED;
			mMgr->PushEvent(e);
			return;
		}

		mCursor += ret;

		while (mCursor > sizeof(MsgHead))
		{
			auto head = (MsgHead*)mBuf;
			int len = head->dwLength + sizeof(MsgHead);
			if (len > mCursor)
			{
				break;
			}

			NetEvent e;
			e.mNetID = mNetID;
			e.mType = RECEIVE_MSG;
			e.mMsg.assign(mBuf, mBuf + len);
			mMgr->PushEvent(e);

			memmove(mBuf, mBuf + len, mCursor - len);
			mCursor -= len;
		}
		break;
	}
}

void Socket::Send()
{
	while (!mMsgList.empty())
	{
		auto data = PopMsg();
		int offset = 0;
		while (true)
		{
			int ret = send(mFd, data.c_str() + offset, data.size() - offset, 0);
			if (ret < 0)
			{
				break;
			}
			offset += ret;
			if (offset == data.size())
			{
				break;
			}
		}
	}
}

void Socket::PushMsg(std::string&& data)
{
	std::lock_guard<std::mutex> guard(mListMutex);
	mMsgList.push(data);
}

std::string Socket::PopMsg()
{
	std::lock_guard<std::mutex> guard(mListMutex);
	auto data = mMsgList.front();
	mMsgList.pop();
	return data;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
NetMgr* NetMgr::GetInstance()
{
	static NetMgr* instance = nullptr;
	if (instance == nullptr)
	{
		instance = new NetMgr;
		instance->Init();
	}
	return instance;
}


bool NetMgr::Init()
{
	WSADATA		wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	return WSAStartup(wVersionRequested, &wsaData);
}

int NetMgr::CreateConnect(std::string Ip, short Port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == fd)
	{
		return INVALID_NETID;
	}

	unsigned long ul = 1;
	if (SOCKET_ERROR == ioctlsocket(fd, FIONBIO, &ul))
	{
		return INVALID_NETID;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;

	if (1 != inet_pton(AF_INET, Ip.c_str(), &addr.sin_addr))
	{
		return INVALID_NETID;
	}
	addr.sin_port = htons((u_short)Port);
	int ret = connect(fd, (sockaddr*)&addr, sizeof(sockaddr));

	Socket::Status status = Socket::CONNECTING;
	if (ret == SOCKET_ERROR)
	{
		if (WSAEWOULDBLOCK != GetLastError())
		{
			closesocket(fd);
			return INVALID_NETID;
		}
		status = Socket::CONNECTING;
	}
	else
	{
		status = Socket::CONNECTED;
	}
	int id = mNextNetID++;
	auto p = new Socket(id, fd, status, this);
	p->Run();
	mNetMap[id] = p;
	return id;
}

void NetMgr::PushEvent(const NetEvent& e)
{
	std::lock_guard<std::mutex> guard(mListMutex);
	mEventList.push(e);
}

NetEvent NetMgr::PopEvent()
{
	std::lock_guard<std::mutex> guard(mListMutex);
	if (mEventList.empty())
	{
		NetEvent e;
		e.mNetID = INVALID_NETID;
		return e;
	}
	auto e = mEventList.front();
	mEventList.pop();
	return e;
}

void NetMgr::SendData(int id, std::string& data)
{
	if (!mNetMap.count(id))
	{
		return;
	}
	mNetMap[id]->PushMsg(std::move(data));
}

