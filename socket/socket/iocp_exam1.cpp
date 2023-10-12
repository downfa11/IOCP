#include "IOCompletionPort.h"
#include<iostream>
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

	ioCompletionPort.DestroyThread();
	return 0;
}