#pragma once
#define WIN32_LEAN_AND_MEAN
#include<windows.h>

struct PacketData {
	int SessionIndex = 0;
	int DataSize = 0;
	char* mPacketData = nullptr;

	void Set(PacketData& value) {
		SessionIndex = value.SessionIndex;
		DataSize = value.DataSize;

		mPacketData = new char[value.DataSize];
		CopyMemory(mPacketData, value.mPacketData, value.DataSize);

	}

	void Set(int sesionIdex, int dataSize_, char* Data) {
		SessionIndex = sesionIdex;
		DataSize = dataSize_;

		mPacketData = new char[dataSize_];
		CopyMemory(mPacketData, Data, dataSize_);
	}

	void Release() {
		delete mPacketData;
	}
};