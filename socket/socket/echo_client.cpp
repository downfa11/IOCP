#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include<thread>

#pragma comment(lib, "ws2_32")


void ClientThread() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup() failed." << std::endl;
        return;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed." << std::endl;
        WSACleanup();
        return;
    }

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(11021);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "connect() failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::cout << "Connected to the server." << std::endl;
    std::string message;
    char buffer[1024];

    while (true) {
        // 메시지 입력
        std::cout << "Enter a message (or 'exit' to quit): ";
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        // 메시지 전송
        send(clientSocket, message.c_str(), message.length(), 0);

        // 서버로부터 응답 수신
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::cout << "Server response: " << buffer << std::endl;
        }
        else if (bytesRead == 0) {
            std::cerr << "Connection closed by server." << std::endl;
            break;
        }
        else {
            std::cerr << "recv() failed." << std::endl;
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    std::thread clientThread(ClientThread);
    clientThread.join();

    return 0;
}

