#pragma once
#include "IOCPServer.h"
#include "Packet.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>

class EchoServer : public IOCPServer {
private:
	bool isRunProcessThread = false;
	std::thread ProcessThread;
	std::mutex mLock;
	std::deque<PacketData> PacketDataQueue;

	void ProcessPacket() {
		while (isRunProcessThread) {
			auto packetdata = DequePacketData();
			if (packetdata.DataSize != 0)
				SendMsg(packetdata.SessionIndex, packetdata.DataSize, packetdata.mPacketData);
			else std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	PacketData DequePacketData() {
		PacketData packetData;
		std::lock_guard<std::mutex> guard(mLock);
		if (PacketDataQueue.empty())
			return packetData;

		PacketDataQueue.front().Release();
		PacketDataQueue.pop_front();

		return packetData;
	}
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;

	virtual void OnConnect(const UINT32 clientIndex_) override {
		std::cout << "[OnConnect] Index "<< clientIndex_<<std::endl;
	}

	virtual void OnClose(const UINT32 clientIndex_) override {
		std::cout << "[OnDisConnect] Index " << clientIndex_ << std::endl;
	}

	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) {
		std::cout << "[OnReceive] Index " << clientIndex_ <<", dataSize : " <<size_<< std::endl;
		PacketData packet;
		packet.Set(clientIndex_, size_, pData_);
		std::lock_guard<std::mutex> guard(mLock);
		PacketDataQueue.push_back(packet);
	}

	void Run(const UINT32 maxClient) {
		isRunProcessThread = true;
		ProcessThread = std::thread( [this]() {ProcessPacket();}    );

		StartServer(maxClient);
	}

	void End() {
		isRunProcessThread = false;
		if (ProcessThread.joinable())
			ProcessThread.join();

		DestroyThread();
	}
};