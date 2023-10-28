#include<iostream>
#include<Windows.h>


DWORD WINAPI ThreadFunction(LPVOID pParam)
{
	puts("*** Begin Thread ***");
	for (int i = 0; i < 5; i++)
	{
		printf("[Worker Thread] %d\n", i);
	}
	puts("*** end Thread ***");
	
	return 0;
}
int main(int argc) {

	DWORD dwThreadID = 0;
	HANDLE hThread = ::CreateThread(NULL, 0, ThreadFunction, 0, 0, &dwThreadID);

	for (int i = 0; i < 10; i++)
		printf("[Main Thread] %d\n", i);

	::CloseHandle(hThread);
	
	
	// ::Sleep(10); // 주석처리

	return 0;
}