#include<iostream>
#include<WinSock2.h>
#include<string>
#include<sstream>
#include<fstream>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

int main() {

    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
        return 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        return 0;

    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(8080);
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
        WSACleanup;
        return 0;
    }

    cout << "Ready to Server." << endl;

    while (1) {
        SOCKET cliSock = accept(sock, NULL, NULL);
        if (cliSock == INVALID_SOCKET)
            cerr << "Fail to accept the client." << endl;

        else {

            char request[2048];
            int bytesReceived = recv(cliSock, request, sizeof(request), 0);

            if (bytesReceived == SOCKET_ERROR)
                cerr << "Failed to receive data from the client." << std::endl;

            else {
                
                request[bytesReceived] = '\0';
                istringstream requestStream(request);
                string requestLine;
                string filename;

                if (getline(requestStream, requestLine) && requestLine.compare(0, 4, "GET ") == 0) {

                    size_t start = requestLine.find(' ') + 2;
                    size_t end = requestLine.find(' ', start);

                    if (start != string::npos && end != string::npos) {

                        filename = requestLine.substr(start, end - start);
                        cout << "Requested file: " << filename << endl;
                    }

                    else {
                        cerr << "Invalid HTTP request format." << endl;
                        closesocket(cliSock);
                        continue;
                    }
                }
                else {
                    cerr << "Invalid HTTP request." << endl;
                    closesocket(cliSock);
                    continue;
                }

                ifstream file(filename, ios::binary);
                if (file.is_open()) {

                    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

                    // C++ 17 파일 시스템 라이브러리인 ifstream - ifstreambuf_iterator<>()
                    // Argument ( 시작조건, 종료 조건(이때는 파일 끝에 도달시 멈춘다.))

                    string response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + to_string(content.length()) + "\r\n"
                        "\r\n" + content;

                    send(cliSock, response.c_str(), response.length(), 0);
                }

                else {
                    string NotFound = "HTTP/1.1 404 Not Found\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n"
                        "<html><body><h1>404 Not Found</h1></body></html>";

                    send(cliSock, NotFound.c_str(), NotFound.length(), 0);
                }
            }

            closesocket(cliSock);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}