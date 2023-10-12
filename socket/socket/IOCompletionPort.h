#pragma once
#pragma comment(lib,"ws2_32")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iostream>
#include<thread>
#include<vector>

#define MAX_SOCKBUF 1024
#define MAX_WORKERTHREAD 4 //쓰레드풀에 넣을 쓰레드의 수

using namespace std;
enum class IOOperation { //작업 동작의 종류
	RECV, SEND
};

struct OverlappedEx { //WSAOVERLAPPED 구조체를 확장해서 필요한 정보를 더 넣음
	WSAOVERLAPPED m_Overlapped; // Overlapped IO 구조체
	SOCKET m_cliSocket;
	WSABUF m_wsaBuf;
	char m_Buf[MAX_SOCKBUF]; //data buffer
	IOOperation m_Operation;
};

struct ClientInfo {
	SOCKET cliSocket;
	OverlappedEx RecvOverlappedEx;
	OverlappedEx SendOverlappedEx;

	ClientInfo() {
		ZeroMemory(&RecvOverlappedEx, sizeof(OverlappedEx));
		ZeroMemory(&SendOverlappedEx, sizeof(OverlappedEx));
		cliSocket = INVALID_SOCKET;
	}
};

class IOCompletionPort {

public:


	vector<ClientInfo> ClientInfos;
	SOCKET ListenSocket = INVALID_SOCKET;
	int ClientCnt = 0;

	vector<thread> IOWorkerThreads;
	thread mAccepterThread;

	HANDLE IOCPHandle = INVALID_HANDLE_VALUE; // CompletionPort 객체 handle

	bool isWorkerRun = true; // 작업 쓰레드 동작 flag
	bool isAccepterRun = true; // 접속 쓰레드 동작 flag
	char SocketBuf[1024] = { 0, }; //socket buffer

	IOCompletionPort(void) {}

	~IOCompletionPort(void) {
		WSACleanup(); //winsock의 사용을 끝낸다.
	}

	bool InitSocket() {
		WSADATA wsaData;
		int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != ret)
		{
			cout << "WSAStartup() fail. : " << WSAGetLastError();
			return false;
		}

		ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == ListenSocket)
		{
			cout << "socket() fail. : " << WSAGetLastError();
			return false;
		}

		cout << "socket init success." << endl;
		return true;
	}

	bool BindandListen(int BindPort) {
		SOCKADDR_IN ServerAddr;
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(BindPort);
		ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		int ret = bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR_IN));
		if (0 != ret) {
			cout << "bind() fail. : " << WSAGetLastError();
			return false;
		}

		ret = listen(ListenSocket, 5);
		// IOCompletionPort socket을 등록하고 접속 대기 큐를 5개로 설정한다.
		if (0 != ret) {
			cout << "listen() fail. : " << WSAGetLastError();
			return false;
		}

		cout << "server enroll success." << endl;
		return true;
	}

	bool StartServer(const int maxCount) {
		CreateClient(maxCount);
		IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
		if (IOCPHandle == NULL) {
			cout << "CreateIoCompletionPort() fail. : " << GetLastError();
			return false;
		}

		bool ret = CreateWorkerThread();
		if (ret == false)
			return false;

		ret = CreateAccepterThread();
		if (ret == false)
			return false;

		cout << "server start." << endl;
		return true;
	}

	void DestroyThread() {
		isWorkerRun = false;
		CloseHandle(IOCPHandle);
		for (auto& th : IOWorkerThreads) {
			if (th.joinable())
				th.join();
		}

		isAccepterRun = false;
		closesocket(ListenSocket);

		if (mAccepterThread.joinable())
			mAccepterThread.join();
	}
private:

	void CreateClient(const int maxClientCount) {
		for (int i = 0; i < maxClientCount; i++)
			ClientInfos.emplace_back();
	}

	//WaitingThread Queue에서 대기할 쓰레드들을 생성
	bool CreateWorkerThread() {
		unsigned int threadId = 0;
		//대기상태로 넣을 쓰레드를 생성하길 권장하는 수 = cpu*2+1
		for (int i = 0; i < MAX_WORKERTHREAD; i++)
			IOWorkerThreads.emplace_back([this]() { WorkerThread(); });

		cout << "WorkerThread start.." << endl;
		return true;
	}

	bool CreateAccepterThread() {
		mAccepterThread = thread([this]() {AccepterThread(); });
		cout << "AccepterThread start.." << endl;
		return true;
	}

	ClientInfo* GetEmptyClientInfo() {
		for (auto& client : ClientInfos) {
			if (client.cliSocket == INVALID_SOCKET)
				return &client;
		}
	}


		bool BindIOCompletionPort(ClientInfo * clientInfo) {
			//Completion Port 객체와 소켓, CompletionKey를 연결시키는 역할
			auto hIOCP = CreateIoCompletionPort((HANDLE)clientInfo->cliSocket, IOCPHandle, (ULONG_PTR)(clientInfo), 0);
			if (hIOCP == NULL || IOCPHandle != hIOCP) {
				cout << "CreateIoCompletionPort() fail. : " << GetLastError();
				return false;
			}

			return true;

		}

		bool BindRecv(ClientInfo* clientinfo) {
			DWORD dwFlag = 0;
			DWORD dwRecvNumBytes = 0;

			clientinfo->RecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
			clientinfo->RecvOverlappedEx.m_wsaBuf.buf = clientinfo->RecvOverlappedEx.m_Buf;
			clientinfo->RecvOverlappedEx.m_Operation = IOOperation::RECV;

			int ret = WSARecv(clientinfo->cliSocket, &(clientinfo->RecvOverlappedEx.m_wsaBuf), 1, &dwRecvNumBytes, &dwFlag,
				(LPWSAOVERLAPPED) & (clientinfo->RecvOverlappedEx), NULL);

			//socket_error면 client socket이 끊어진걸로 처리한다
			if (ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
				cout << "WSARecv() fail : " << WSAGetLastError();
				return false;
			}

			return true;
		
		}

		bool SendMessage(ClientInfo* clientinfo, char* message, int len) {
		
			DWORD dwRecvNumBytes = 0;

			CopyMemory(clientinfo->SendOverlappedEx.m_Buf, message, len);

			clientinfo->SendOverlappedEx.m_wsaBuf.len = len;
			clientinfo->SendOverlappedEx.m_wsaBuf.buf = clientinfo->SendOverlappedEx.m_Buf;
			clientinfo->SendOverlappedEx.m_Operation = IOOperation::SEND;

			int ret = WSASend(clientinfo->cliSocket, &(clientinfo->SendOverlappedEx.m_wsaBuf), 1, &dwRecvNumBytes, 0,
				(LPWSAOVERLAPPED) & (clientinfo->SendOverlappedEx), NULL);

			if (ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
				cout << "WSASend() fail : " << WSAGetLastError();
				return false;
			}
			return true;
		
		}
		// Overlapped IO 작업에 대한 완료 통보를 받아서 그 처리를 하는 함수
		void WorkerThread() {
			ClientInfo* clientinfo = NULL; //CompletionKey를 받을 포인터
			bool success;
			DWORD dwIoSize = 0;
			LPOVERLAPPED lpOverlapped = NULL;

			while (isWorkerRun)
			{
				// 쓰레드들은 WaitingThread Queue에 대기상태로 들어가게 된다.
				// 완료된 IO 작업이 발생하면 IOCP Queue에서 완료된 작업을 가져와서 작업을 수행.
				// PostQueueCompletionStatus() 함수에 의해 사용자 메세지가 도착하면 쓰레드 종료.

				success = GetQueuedCompletionStatus(IOCPHandle, &dwIoSize, (PULONG_PTR)&clientinfo, &lpOverlapped, INFINITE);
				// PULONG_PTR : CompletonKey, LPOVERLLAPED : Overlapped IO 객체

				if (success && dwIoSize == 0 && lpOverlapped == NULL) //사용자 쓰레드 종료 메세지 처리
				{
					isWorkerRun = false;
					continue;
				}

				if (lpOverlapped == NULL)
					continue;

				if (!success || (dwIoSize == 0 && success)) {
					cout << "socket" << (int)clientinfo->cliSocket << " disconnected." << endl;
					closesocket(clientinfo->cliSocket);
					continue;
				}

				OverlappedEx* overlappedEx = (OverlappedEx*)lpOverlapped;
				if (overlappedEx->m_Operation == IOOperation::RECV) // recv 작업 결과 뒷처리
				{
					overlappedEx->m_Buf[dwIoSize] = NULL;
					cout << "[수신] bytes : " << dwIoSize << " message : " << overlappedEx->m_Buf << endl;

					SendMessage(clientinfo, overlappedEx->m_Buf, dwIoSize);
					BindRecv(clientinfo);
				}
				else if (overlappedEx->m_Operation == IOOperation::SEND)
					cout << "[수신] bytes : " << dwIoSize << " message : " << overlappedEx->m_Buf << endl;
				else cout << "[예외사항] : " << (int)clientinfo->cliSocket << "에서 발생함." << endl;
			}
		}

		void AccepterThread() {
			SOCKADDR_IN clientAddr;
			int addrlen = sizeof(SOCKADDR_IN);

			while (isAccepterRun) {
				ClientInfo* clientinfo = GetEmptyClientInfo();
				if (clientinfo == NULL) {
					cout << "client full" << endl;
					return;
				}

				//접속 요청이 들어올떄까지 기다린다.
				clientinfo->cliSocket = accept(ListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
				if (clientinfo->cliSocket == INVALID_SOCKET)
					continue;

				//IO Completion Port 객체와 소켓을 연결
				bool ret = BindIOCompletionPort(clientinfo);
				if (!ret)
					return;

				//recv Overlapped IO 작업을 요청
				ret = BindRecv(clientinfo);
				if (!ret)
					return;

				char clientIP[32] = { 0, };
				inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, 32 - 1);
				cout << "client connect. IP : " << clientIP << " socket : " << (int)clientinfo->cliSocket << endl;
				ClientCnt++;
			}
		}
		void CloseSocket(ClientInfo* clientinfo, bool isForce = false) {
			struct linger stLinger = { 0,0 }; //SO_DONTLINGER 로 설정
			if (isForce)
				stLinger.l_onoff = 1;

			shutdown(clientinfo->cliSocket, SD_BOTH); //송수신 모두 우아한 연결중단

			setsockopt(clientinfo->cliSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
			closesocket(clientinfo->cliSocket);

			clientinfo->cliSocket = INVALID_SOCKET;
		}
};