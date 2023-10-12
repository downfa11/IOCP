#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024
void ErrorHandling(const char* message);

int main(int argc, char* argv[]) {

	WSAData wsaData;
	SOCKET servSock, clntSock;
	SOCKADDR_IN servAdr, clntAdr;
	TIMEVAL timeout;
	fd_set reads, cpyReads;

	int addressSize;
	int strLen, fdNum, i;
	char buf[BUF_SIZE];
	
	if (argc!= 2) {
		std::cout << "Usage : " << argv[0]<<std::endl;
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	servSock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(servSock, (SOCKADDR*)&servAdr, sizeof(servSock)) == SOCKET_ERROR)
		ErrorHandling("Bind() error!");

	if (listen(servSock, 5) == SOCKET_ERROR)
		ErrorHandling("Listen() error!");

	FD_ZERO(&reads);
	FD_SET(servSock, &reads); //������ ���� ���θ� �����ϴ� ������� ���� ������ ������.
	// Ŭ���̾�Ʈ�� �����û�� �������� �������� �̷������ ������, ���� �������� ���ŵ� �����Ͱ� �����Ѵ� = �����û�� �־���.

	while (1) {
		cpyReads = reads;
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if ((fdNum = select(0, &cpyReads, 0, 0, &timeout) == SOCKET_ERROR))
			break;

		if (fdNum == 0)
			break;

		for (i = 0;  i< reads.fd_count; i++)
		{
			if (FD_ISSET(reads.fd_array[i], &cpyReads))
			{
				if (reads.fd_array[i] == servSock) // ���������� ���º�ȭ�� �´°�? -> �����û�� ����
				{
					addressSize = sizeof(clntAdr);
					clntSock = accept(servSock, (SOCKADDR*)&clntAdr, &addressSize);
					FD_SET(clntSock, &reads);
					std::cout << "Connected Client :" << clntSock << std::endl;
				}
				else // ���º�ȭ�� �߻��� ������ ������ �ƴ� ��� = ������ �����Ͱ� �ִ� ���
				{
					strLen = recv(reads.fd_array[i], buf, BUF_SIZE - 1, 0);
					if (strLen == 0) {
						FD_CLR(reads.fd_array[i], &reads);
						closesocket(cpyReads.fd_array[i]);
						std::cout << "closed client : " << cpyReads.fd_array[i] << std::endl;

					}
					else send(reads.fd_array[i], buf, strLen, 0);
					//���ڿ����� �������Ḧ �ǹ��ϴ� EOF���� �Ǻ��ؾ���.
				}
			}
		}
	}

	closesocket(servSock);
	WSACleanup();
	return 0;
}

void ErrorHandling(const char* message) {
	std::cout << message << std::endl;
	exit(1);
}