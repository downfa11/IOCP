#include "IOCompletionPort.h"
#include<iostream>
#include<string>
const int SERVER_PORT = 11021;
const int MAX_CLIENT = 100; 

using namespace std;


int main() {

	IOCompletionPort ioCompletionPort;

	ioCompletionPort.InitSocket();

	ioCompletionPort.BindandListen(SERVER_PORT);

	ioCompletionPort.StartServer(MAX_CLIENT);

	cout << "아무 키나 누를 때까지 대기합니다." << endl;
	getchar();

    while (1) {
       /* for (ClientInfo& client : ClientInfos) {
            if (client.cliSocket != INVALID_SOCKET) {
                UpdateClientInfo(client);
            }
        }*/
        cout << "$ ";
        string command;
        getline(cin, command);

        //cout << command << endl;
        if (command == "clientCount") {
            cout<< "Connected Client : "<< ioCompletionPort.ClientCnt <<"s" << endl;
        }
        else if (command == "status") {
            for (auto& cli : ioCompletionPort.ClientInfos) {
                cout << cli.cliSocket <<"'s Position : "<<cli.x<<","<<cli.y << endl;
            }
            // 서버 상태 관련 명령 처리
        }
        else if (command == "show") {
            // 패킷 표시 명령 처리
        }
        else if (command == "exit")
        {
            ioCompletionPort.DestroyThread();
            break;
        }
    }

	return 0;
}