/*
* @file    RLBridge.hpp
* @author
* @date    2026/09/26
* @brief   Python(強化学習側)とTCP通信でfloatを1個ずつやり取りするだけの、シンプルな橋渡しクラス
*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

/* Classes -------------------------------------------------------------------*/
// Pythonと「float 1個を送る、float 1個を受け取る」だけのやり取りをするクラス
class RLBridge
{
private:
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;
	WSAData wsaData;

public:
	// 指定ポートで待ち受け、Pythonからの接続が来るまで待つ
	bool open(int port)
	{
		WSAStartup(MAKEWORD(2, 0), &wsaData);

		listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket == INVALID_SOCKET) return false;

		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
		if (listen(listenSocket, 1) == SOCKET_ERROR) return false;

		std::cout << "[RLBridge] Pythonからの接続を待っています (port " << port << ")..." << std::endl;
		clientSocket = accept(listenSocket, NULL, NULL);
		std::cout << "[RLBridge] Pythonと接続しました" << std::endl;

		return clientSocket != INVALID_SOCKET;
	}

	// Pythonから角度の補正値(float 1個)を受け取る
	float receiveAction()
	{
		float value = 0.0f;
		recv(clientSocket, (char*)&value, sizeof(float), 0);
		return value;
	}

	// 観測値(float 1個。中身は今後決める、今は仮の値でよい)をPythonへ送る
	void sendObservation(float value)
	{
		send(clientSocket, (char*)&value, sizeof(float), 0);
	}

	void close()
	{
		if (clientSocket != INVALID_SOCKET) closesocket(clientSocket);
		if (listenSocket != INVALID_SOCKET) closesocket(listenSocket);
		WSACleanup();
	}
};
