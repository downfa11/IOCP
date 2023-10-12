#include<EchoServer.h>
#include<string>
#include<iostream>

const int SERVER_PORT = 11021;
const int MAX_CLIENT = 100;

int main() {
	EchoServer server;

	server.InitSocket();

	server.BindandListen(SERVER_PORT);

	std::cout << "아무 키나 누를떄까지 대기합니다." << std::endl;
	
	while(1) {
		std::string input;
		std::getline(std::cin, input);

		if (input == "quit")
			break;
	}
	server.Close();

	return 0;

}