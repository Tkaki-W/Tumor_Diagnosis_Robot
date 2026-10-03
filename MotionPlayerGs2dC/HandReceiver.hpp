/*
* @file    UdpReceiver.hpp
* @author
* @date    2025 / 07 / 10
* @brief
*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <inttypes.h>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <fcntl.h>
#include <conio.h>
#include "squeue.hpp"
#pragma comment(lib, "ws2_32.lib")

/* Structures ----------------------------------------------------------------*/
// 指先の情報
typedef struct FingerInfo
{
	int sensorStatus;
	int accelSequence;
	std::array<float, 3> accel;

	int forceSequence;
	int forceRecordNumber;
	std::array<float, 3> force;

	int temperatureSequence;
	std::array<float, 6> temperature;
};

// 指先5本を含めた手の情報
struct HandInfo
{
	int sequence;
	std::array<FingerInfo, 5> fingerInfos;
	int hour, minute, second, ms;
};

/* Class ---------------------------------------------------------------------*/
class HandReceiver
{
private:
	// Udp関係
	int receiveSocket;
	sockaddr_in receiveAddress;
	WSAData wsaData;
	bool isOpen = false;

	// Thread関係
	std::thread th;
	bool exitFlag = false;

	// Haptic関係
	ThreadSafeQueue<HandInfo> hapticInfo;

	// キュー関係
	std::queue<uint8_t> receivedData;

public:
	HandReceiver() : th(&HandReceiver::receiverThread, this) {}
	~HandReceiver() { close(); }

	bool open(std::string address, int port)
	{
		if (isOpen) return false;

		// WinSockt初期化
		if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) return false;
		receiveSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // UDP
		if (receiveSocket == INVALID_SOCKET) { WSACleanup(); return false; }

		// アドレス設定
		receiveAddress.sin_family = AF_INET; // IPv4
		receiveAddress.sin_port = htons(port);
		InetPtonA(AF_INET, address.c_str(), &(receiveAddress.sin_addr));

		// ソケットへバインド
		if (bind(receiveSocket, (struct sockaddr*)&receiveAddress, sizeof(receiveAddress)) == SOCKET_ERROR)
		{
			closesocket(receiveSocket);
			WSACleanup();
			return false;
		}

		isOpen = true;
		return true;
	}

	bool infoDataAvailable() { return (hapticInfo.size() != 0); }
	HandInfo popInfo() {
		HandInfo result = hapticInfo.pop();
		return result;
	}
	int size() { return hapticInfo.size(); }
	/*
	void close()
	{
		if (!isOpen) return;
		exitFlag = true;
	}
	*/
	void close()
	{
		exitFlag = true;

		if (isOpen)
		{
			// recvfromを解除する
			shutdown(
				receiveSocket,
				SD_BOTH
			);
		}

		if (th.joinable())
		{
			th.join();
		}

		isOpen = false;
	}
	int popInt(void)
	{
		if (receivedData.size() < 4) return 0;
		int result = 0;
		for (int i = 0; i < 4; i++) {
			result += (receivedData.front() << (i * 8));
			receivedData.pop();
		}
		return result;
	}

	float popFloat(void)
	{
		if (receivedData.size() < 4) return 0;
		uint8_t tmp[4];
		for (int i = 0; i < 4; i++) {
			tmp[i] = receivedData.front();
			receivedData.pop();
		}
		float result;
		std::memcpy(&result, tmp, sizeof(float));
		return result;
	}

	bool checkExit(void)
	{

	}

	void receiverThread(void)
	{
		uint8_t buffer[1024];
		sockaddr_in senderAddress;
		int senderAddressSize = sizeof(senderAddress);

		char exitCode[4] = { 'e', 'x', 'i', 't' };
		uint8_t exitStatus = 0;

		while (!exitFlag)
		{
			if (!isOpen) continue;

			// 受信
			int bytesReceived = recvfrom(receiveSocket, (char*)buffer, 344, 0,
				(SOCKADDR*)&senderAddress, &senderAddressSize);
			if (bytesReceived != SOCKET_ERROR) {
				for (uint32_t i = 0; i < bytesReceived; i++) {
					receivedData.push(buffer[i]);

					// Exitコード
					if (buffer[i] == exitCode[exitStatus]) {
						exitStatus++;
					}
					else {
						exitStatus = 0;
					}
					if (exitStatus >= 4) exitFlag = true;
				}
			}
			else {
				WSAGetLastError();
				continue;
			}



			// 暫定344バイト受信したらデータ？ -> 実機計測だと344だった。
			if (receivedData.size() >= 344) {
				int packFormat = 0b00111001110111111;
				int seq = popInt();
				HandInfo handInf;

				auto now = std::chrono::system_clock::now();
				auto time = std::chrono::system_clock::to_time_t(now);
				std::tm lt;
				localtime_s(&lt, &time);
				handInf.hour = lt.tm_hour;
				handInf.minute = lt.tm_min;
				handInf.second = lt.tm_sec;
				handInf.ms = (uint64_t)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()) % 1000;

				for (int fingerId = 0; fingerId < 5; fingerId++)
				{
					std::vector<int> intValues;
					std::vector<float> floatValues;

					for (int8_t i = 16; i >= 0; i--) {
						if (((packFormat >> i) & 1) == 0) intValues.push_back(popInt());
						else floatValues.push_back(popFloat());
					}

					handInf.sequence = seq;
					handInf.fingerInfos[fingerId].sensorStatus = intValues[0];
					handInf.fingerInfos[fingerId].accelSequence = intValues[1];
					std::copy_n(floatValues.begin(), 3, handInf.fingerInfos[fingerId].accel.begin());
					handInf.fingerInfos[fingerId].forceSequence = intValues[2];
					handInf.fingerInfos[fingerId].forceRecordNumber = intValues[3];
					std::copy_n(floatValues.begin() + 3, 3, handInf.fingerInfos[fingerId].force.begin());
					handInf.fingerInfos[fingerId].temperatureSequence = intValues[4];
					std::copy_n(floatValues.begin() + 6, 6, handInf.fingerInfos[fingerId].temperature.begin());
				}

				hapticInfo.push(handInf);
			}
		}

		closesocket(receiveSocket);
		WSACleanup();
//		std::cout << " Receiver Thread Finish" << std::endl;
	}
};