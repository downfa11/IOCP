#pragma once
#pragma comment(lib,"ws2_32")

#include "ClientInfo.h"
#include "Define.h"
#include <thread>
#include<vector>

class IOCPServer {
public:
	IOCPServer(void) {}

	virtual ~IOCPServer(void)
	{		WSACleanup();
}

	bool InitSocket() {
		WSAData wsaData;
		int nRet = WSAStartup( MAKEWORD(2,2),& wsaData);
		if (nRet != 0) {
			printf("[error] WSAStartup() : %d\n", WSAGetLastError());
			return false;
		}

		ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
		if (ListenSocket == INVALID_SOCKET) {
			printf("[error] socket() : %d", WSAGetLastError());
			return false;
		}

		printf("Socket Init. \n");
		return true;
	}

	bool BindandListen(int BindPort) {
		SOCKADDR_IN ServerAddr;
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(BindPort);
		ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR_IN)) != 0) {
			printf("[error] bind() : %d", WSAGetLastError());
			return false;
		}

		if (listen(ListenSocket, 5) != 0) {
			printf("[error] listen() : %d", WSAGetLastError());
			return false;
		}

		printf("Server Enroll Successed. \n");
		return true;
	}

	bool StartServer(const UINT32 maxClientCount) {

		CreateClient(maxClientCount);
		IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
		if (IOCPHandle == NULL) {
			printf("[error] CreateIoCompletionPort() : %d\n", GetLastError());
			return false;
		}

		if (!CreateWorkerThread())
			return false;

		if (!CreateAccepterThread())
			return false;

		printf("Server Started. \n");
		return true;
	}

	void DestroyThread() {
		isWorkerRun = false;
		CloseHandle(IOCPHandle);

		for (auto& th : IOWorkerThreads) {
			if (th.joinable())
				th.join();
		}

		isAcceptRun = false;
		closesocket(ListenSocket);

		if (mAccepterThread.joinable())
			mAccepterThread.join();
	}

	bool SendMsg(const UINT32 sessionIndex_, const UINT32 dataSize, char* pData) {
		auto Client = GetClientInfo(sessionIndex_);
		return Client->SendMsg(dataSize, pData);
	}

	virtual void OnConnect(const UINT32 clientIndex_){}
	virtual void OnClose(const UINT32 clientIndex_) {}
	virtual void OnReceive(const UINT32 clientIndex_,const UINT32 size_, char* pData){}
private:

	std::vector<ClientInfo> ClientInfos;
	SOCKET ListenSocket = INVALID_SOCKET;
	int ClientCnt = 0;

	std::vector<std::thread> IOWorkerThreads;
	std::thread mAccepterThread;

	HANDLE IOCPHandle = INVALID_HANDLE_VALUE;
	bool isWorkerRun = true;
	bool isAcceptRun = true;


	void CreateClient(const UINT32 maxClientCount) {
		for (UINT32 i = 0; i < maxClientCount; i++)
		{
			ClientInfos.emplace_back();
			ClientInfos[i].Init(i);
		}
	}

	void CloseSocket(ClientInfo* ClientInfo, bool isForce = false) {
		auto ClientIndex = ClientInfo->GetIndex();
		ClientInfo->Close(isForce);
		OnClose(ClientIndex);
	}

	bool CreateWorkerThread() {
		unsigned int ThreadId = 0;
		for (int i = 0; i < MAX_WORKERTHREAD; i++)
			IOWorkerThreads.emplace_back([this]() {WorkerThread();  });

		printf("WorkerThread 시작...\n");
		return true;
	}

	ClientInfo* GetEmptyClientInfo() {
		for (auto& client : ClientInfos) {
			if (!client.IsConnetec())
				return &client;
		}
		return nullptr;
	}

	ClientInfo* GetClientInfo(const UINT32 sessionIndex) {
		return &ClientInfos[sessionIndex];
	}

	bool CreateAccepterThread() {
		mAccepterThread = std::thread([this]() { AccepterThread(); });

		printf("AccepterThread 시작..\n");
		return true;
	}

	void WorkerThread() {
		//Overlapped I/O작업에 대한 완료 통보를 받아 그에 해당하는 처리를 함
		ClientInfo* ClientInfo = nullptr;
		BOOL bSuccess = TRUE;
		DWORD dwIoSize = 0;
		LPOVERLAPPED lpOverlapped = NULL;

		while (isWorkerRun)
		{
			bSuccess = GetQueuedCompletionStatus(IOCPHandle,
				&dwIoSize,					// 실제로 전송된 바이트
				(PULONG_PTR)&ClientInfo,		// CompletionKey
				&lpOverlapped,				// Overlapped IO 객체
				INFINITE);					// 대기할 시간


			if (bSuccess && dwIoSize==0 && lpOverlapped==NULL)
			{
				isWorkerRun = false;
				continue;
			}

			if (NULL == lpOverlapped)
				continue;

			//client가 접속을 끊었을때..			
			if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
			{
				CloseSocket(ClientInfo);
				continue;
			}


			auto pOverlappedEx = (OverlappedEx*)lpOverlapped;

			//Overlapped I/O Recv 작업 결과 뒤 처리
			if (IOOperation::RECV == pOverlappedEx->m_Operation)
			{
				OnReceive(ClientInfo->GetIndex(), dwIoSize, ClientInfo->RecvBuffer());
				ClientInfo->BindRecv();
			}
			//Overlapped I/O Send 작업 결과 뒤 처리
			else if (IOOperation::SEND == pOverlappedEx->m_Operation)
			{
				delete[] pOverlappedEx->m_wsaBuf.buf;
				delete pOverlappedEx;
				ClientInfo->SendCompleted(dwIoSize);
			}
			//예외 상황
			else
				printf("Client Index(%d)에서 예외상황\n", ClientInfo->GetIndex());
		}
	}

	void AccepterThread(){
		SOCKADDR_IN		stClientAddr;
		int nAddrLen = sizeof(SOCKADDR_IN);

		while (isAcceptRun)
		{
			//접속을 받을 구조체의 인덱스를 얻어온다.
			ClientInfo* ClientInfo = GetEmptyClientInfo();
			if (NULL == ClientInfo)
			{
				printf("[에러] Client Full\n");
				return;
			}


			//클라이언트 접속 요청이 들어올 때까지 기다린다.
			auto newSocket = accept(ListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
			if (INVALID_SOCKET == newSocket)
			{
				continue;
			}

			if (ClientInfo->OnConnect(IOCPHandle, newSocket) == false)
			{
				ClientInfo->Close(true);
				return;
			}

			char clientIP[32] = { 0, };
			inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
			OnConnect(ClientInfo->GetIndex());

			//클라이언트 갯수 증가
			ClientCnt++;
		}
	}
};