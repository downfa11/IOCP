#pragma once
#include "Define.h"
#include<iostream>

class ClientInfo {
public:
	ClientInfo() {
		ZeroMemory(&RecvOverlappedEx, sizeof(OverlappedEx));
		sock = INVALID_SOCKET;

	}

	void Init(int index) {
		mIndex = index;
	}

	int GetIndex() { return mIndex; }

	bool IsConnetec() { return sock != INVALID_SOCKET; }

	SOCKET GetSocket() { return sock; }

	char* RecvBuffer() { return RecvBuf; }

	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_) {
		sock = socket_;
		Clear();

		if (BindIOCompletionPort(iocpHandle_) == false)
			return false;

		return BindRecv();
	}

	void Clear(){
	}

	void Close(bool IsForce = false) {
		struct linger Linger = { 0,0 };
		if (IsForce)
			Linger.l_onoff = 1;

		shutdown(sock, SD_BOTH);
		setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&Linger, sizeof(Linger));
		sock = INVALID_SOCKET;
	}

	bool BindIOCompletionPort(HANDLE iocpHandle_) {
		auto hIOCP = CreateIoCompletionPort((HANDLE)GetSocket(), iocpHandle_, (ULONG_PTR)this, 0);
		if (hIOCP == INVALID_HANDLE_VALUE)
			return false;

		return true;
	}

	bool BindRecv() {
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		RecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
		RecvOverlappedEx.m_wsaBuf.buf = RecvBuf;
		RecvOverlappedEx.m_Operation = IOOperation::RECV;

		int ret = WSARecv(sock, &(RecvOverlappedEx.m_wsaBuf), 1, &dwRecvNumBytes, &dwFlag, (LPOVERLAPPED)&RecvOverlappedEx, NULL);

		if (ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
			std::cout << "[error] WSARecv() fail. : " << WSAGetLastError() << std::endl;
			return false;

		}
		return true;
	}
	

	bool SendMessage(const int dataSize_, char* message) {
		auto SendOverlappedEx = new OverlappedEx;
		ZeroMemory(SendOverlappedEx, sizeof(OverlappedEx));
		SendOverlappedEx->m_wsaBuf.len = dataSize_;
		SendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
		CopyMemory(SendOverlappedEx->m_wsaBuf.buf, message, dataSize_);

		DWORD dwRecvNumBytes = 0;
		int ret = WSASend(sock, &(SendOverlappedEx->m_wsaBuf), 1, &dwRecvNumBytes, 0, (LPOVERLAPPED)&SendOverlappedEx, NULL);
	
		if (ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
			std::cout << "[error] WSASend() fail : " << WSAGetLastError() << std::endl;
			return false;
		}
		return true;
	}

	void SendCompleted(const int dataSize) {
		std::cout << "[송신완료] bytes : " << dataSize << std::endl;
	}

private:
	int mIndex = 0;
	SOCKET sock;
	OverlappedEx RecvOverlappedEx;
	char RecvBuf[MAX_SOCKBUF];
};