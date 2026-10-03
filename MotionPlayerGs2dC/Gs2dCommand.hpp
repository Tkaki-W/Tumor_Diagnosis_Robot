/*
* @file    Gs2dCommand.hpp
* @author
* @date    2025/07/09
* @brief   
*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

/* Classes -------------------------------------------------------------------*/
class CallbackEventArgs//受信した通信データを保存するためのクラス
{

public:
	uint8_t instruction;
	uint16_t length;
	std::vector<uint8_t> data = {};

	CallbackEventArgs(uint8_t instruction, uint16_t length)
	{
		this->instruction = instruction;
		this->length = length;
	}
	~CallbackEventArgs() {}

	void addData(uint8_t b) { data.push_back(b); }
};
using CallbackType = void(*)(CallbackEventArgs);


class Gs2dProtocol
{
private:
	uint8_t receiveStatus = 0;
	uint8_t instruction = 0;
	int		length = 0;

	std::vector<uint8_t> dataArray;

public:
	std::vector<uint8_t> generateCommand(uint8_t instruction, std::vector<uint8_t> parameter)
	{
		uint8_t length = parameter.size();

		std::vector<uint8_t> data = { 0xFF, 0xFD, (uint8_t)(length + 2), 0x00, instruction };
		if(length != 0) std::copy(parameter.begin(), parameter.end(), std::back_inserter(data));

		data.push_back(0);
		data[data.size() - 1] = calculateCheckSum(data);

		return data;
	}

	void processCommand(uint8_t data)
	{
		if (receiveStatus == 0) dataArray.clear();
		dataArray.push_back(data);

		switch (receiveStatus)
		{
		case 0:
			receiveStatus = (data == 0xFF) ? (receiveStatus + 1) : 0;
			break;

		case 1: // Header 0xFF
			receiveStatus = (data == 0xFD) ? (receiveStatus + 1) : 0;
			break;

		case 2: // Length Low Byte
			length = data; receiveStatus++;
			break;

		case 3: // Length High Byte
			length += (data << 8); receiveStatus++;
			break;

		case 4: // Instruction
			instruction = data;
			if (length == 2) receiveStatus += 2;
			else receiveStatus++;
			break;

		case 5: // Parameter
			if (dataArray.size() == length + 3) receiveStatus++;
			break;

		case 6: // CheckSum
			receiveStatus = 0;
			if (data != calculateCheckSum(dataArray)) break;

			CallbackEventArgs command(instruction, length - 2);
			for (uint8_t i = 5; i < length +3; i++) {
				command.addData(dataArray[i]);
			}

			if (callback) callback(command);
			break;
		}
	}

	uint8_t calculateCheckSum(std::vector<uint8_t> data)
	{
		uint32_t sum = 0;
		for (uint8_t i = 0; i < data.size() - 1; i++) {
			sum += data[i];
		}
		return (sum & 0xFF);
	}

	CallbackType callback = 0;
};