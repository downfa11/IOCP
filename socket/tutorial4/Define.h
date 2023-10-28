#pragma once
#include<WinSock2.h>
#include<WS2tcpip.h>

const int MAX_SOCKBUF = 256;
const int MAX_SOCK_SENDBUF = 4096;
const int MAX_WORKERTHREAD = 4; //쓰레드풀에 넣을 쓰레드 수

enum class IOOperation {
	RECV,SEND
};

struct OverlappedEx {
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET m_clisock; // 클라이언트의 소켓
	WSABUF m_wsaBuf; // Overlapped I/O 소켓 버퍼
	IOOperation m_Operation;
};