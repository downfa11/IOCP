#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#define BUF_SIZE 1024
#define RLT_SIZE 4
#define OPSZ 4



void ErrorHandling(const char* message);
int calc(int opnum, int opnds[], char oprator);

int main(int argc, char* argv[]) {
	WSAData wsaData;
	SOCKET hServSock, hClntSock;
	char opinfo[BUF_SIZE];
	int result, opndCnt, i;
	int recvCnt, recvLen;
	SOCKADDR_IN servAdr, clntAdr;
	int clntAdrsize;
	if (argc != 2)
		exit(1);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET)
		ErrorHandling("Socket() error!");

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));


	if(bind(hServSock,(SOCKADDR*)&servAdr,sizeof(hServSock))==SOCKET_ERROR)
		ErrorHandling("Bind() error!");

	if(listen(hServSock,5)==SOCKET_ERROR)
		ErrorHandling("Listen() error!");
	clntAdrsize = sizeof(clntAdr);

	for (i = 0; i < 5; i++)
	{
		opndCnt = 0;
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrsize);
		recv(hClntSock, (char*)&opndCnt, 1, 0);
		recvLen = 0;
		while ((opndCnt * OPSZ + 1) > recvLen)
		{
			recvCnt = recv(hClntSock, &opinfo[recvLen], BUF_SIZE - 1, 0);
			recvLen += recvCnt;
		}
		result = calc(opndCnt, (int*)opinfo, opinfo[recvLen - 1]);
		send(hClntSock, (char*)&result, sizeof(result), 0);
		closesocket(hClntSock);
	}
	closesocket(hServSock);
	WSACleanup();

	return 0;

}

int calc(int opnum, int opnds[], char op) {
	int result = opnds[0], i;
	switch (op)
	{
	case '+':
		for (i = 0; i < opnum; i++) result += opnds[i];
		break;
	case '-':
		for (i = 0; i < opnum; i++) result -= opnds[i];
		break;
	case '*':
		for (i = 0; i < opnum; i++) result *= opnds[i];
		break;

	}
	return result;
}
void ErrorHandling(const char* message) {
	fputs(message, stderr);
	fputs("\n", stderr);
	exit(1);
}