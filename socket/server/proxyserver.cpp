#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<iostream>
#include<WinSock2.h>
#include<string>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

int main() {

	int ProxyPort = 80;
	int SeverPort = 8080;

	WSAData wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
		return 0;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(ProxyPort);
	sockAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		
		return 0;
	}
	if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return 0;
	}

	cout << "Ready to ProxyServer." << endl;

    while (1)
    {

        SOCKET cliSock = accept(sock, NULL, NULL);
        if (cliSock == INVALID_SOCKET)
            cerr << "Fail to accept the client." << endl;
        else {

            char buffer[2048];
            int bytesRead = recv(cliSock, buffer, sizeof(buffer), 0);
            if (bytesRead == SOCKET_ERROR) {
                cerr << "Failed to receive data from the client." << std::endl;
            }
            else {
                buffer[bytesRead] = '\0';

                string request(buffer, buffer + bytesRead);

                SOCKADDR_IN serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(SeverPort);
                serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

                int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (serverSocket < 0) {
                    cerr << "Error: Unable to create server socket" << endl;
                    return 1;
                }

                if (connect(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) < 0) {
                    cerr << "Error: Connection to the server failed" << endl;
                    return 1;
                }
                send(serverSocket, request.c_str(), request.size(), 0);
                while (true) {
                    bytesRead = recv(serverSocket, buffer, sizeof(buffer), 0);
                    if (bytesRead <= 0) {
                        break;
                    }

                    send(cliSock, buffer, bytesRead, 0);
                }
                closesocket(serverSocket);
           
            }
            closesocket(cliSock);
            
        }
    }

    closesocket(sock);
    WSACleanup();
	return 0;
}