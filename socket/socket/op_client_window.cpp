#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#include <Ws2tcpip.h> // InetPton 함수가 선언된 헤더 파일
#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024
#define RLT_SIZE 4
#define OPSZ 4



void ErrorHandling(const char *message);


int main(int argc, char* argv[]) {
	WSAData wsaData;
	SOCKET hSocket;
	char opmsg[BUF_SIZE];
	int result, opndCnt, i;
	SOCKADDR_IN servAdr;
	if (argc != 3) {
		std::cout << "Usage : " << argv[0] << std::endl;
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2) ,&wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		ErrorHandling("Socket() error!");

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_port = htons(atoi(argv[2]));
	if (inet_pton(AF_INET, argv[1], &servAdr.sin_addr) <= 0) {
		std::cout << "Invalid IPv4 address!" << std::endl;
		exit(1);
	}

	if (connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("Connect() error!");
	else
		std::cout << "Connected..." << std::endl;

	std::cout << "Operand count :";
	std::cin >> opndCnt;
	opmsg[0] = (char)opndCnt;

	for (i = 0; i < opndCnt; i++)
	{
		std::cout << "Operand " << i+1 << " :";
		std::cin >> &opmsg[i * OPSZ + 1];

	}

	std::cout << "Operator: ";
	std::cin >> &opmsg[opndCnt * OPSZ + 2];
	send(hSocket, opmsg, opndCnt * OPSZ + 2, 0);
	recv(hSocket, (char*)&result, RLT_SIZE, 0);

	std::cout << "Operation result : " << result << std::endl;
	closesocket(hSocket);
	WSACleanup();
	return 0;

}

void ErrorHandling(const char *message) {
	fputs(message, stderr);
	fputs("\n", stderr);
	exit(1);
}