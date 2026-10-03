/*
* @file    Robot.hpp
* @author
* @date    2025/07/09
* @brief
*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include "WindowsSerial.hpp"
#include "Instruction.hpp"
#include "Gs2dCommand.hpp"
#include <thread>
#include <cstdio>
#include <optional>
#include <cstdint>
#include <chrono>

/* Classes -------------------------------------------------------------------*/
class IdDataType
{
public:
	uint8_t id;
	int data;

	IdDataType(uint8_t id, int position)
	{
		this->id = id;
		this->data = position;
	}
};

class Robot
{
private:
	SerialPort serialPort;
	Gs2dProtocol robot;
	bool exitFlag = false;
	std::thread th;

public:
	void generateCommand(uint8_t instruction, std::vector<uint8_t> parameters)
	{
		std::vector<uint8_t> data = robot.generateCommand(instruction, parameters);
		serialPort.write(data.data(), data.size());
	}

	void convertMotionToCommand(std::vector<IdDataType> motions, uint16_t delay, uint8_t instruction = (uint8_t)Instructions::writePosition)
	{
		std::vector<uint8_t> data;
		
		for (uint8_t i = 0; i < motions.size(); i++) {
			data.push_back(motions[i].id);
			data.push_back(motions[i].data & 0xFF);
			data.push_back((motions[i].data >> 8) & 0xFF);
		}
		data.push_back(delay & 0xFF);
		data.push_back((delay >> 8) & 0xFF);
		generateCommand(instruction | static_cast<uint8_t>(Instructions::registerCommand), data);
	}

	void writePosition(std::vector<IdDataType> motions)
	{
		std::vector<uint8_t> data;

		for (uint8_t i = 0; i < motions.size(); i++) {
			data.push_back(motions[i].id);
			data.push_back(motions[i].data & 0xFF);
			data.push_back((motions[i].data >> 8) & 0xFF);
		}
		generateCommand(static_cast<uint8_t>(Instructions::writePosition), data);
	}

	void writePositionWithTime(
		std::vector<IdDataType> motions,
		uint16_t transitionTime)
	{
		std::vector<uint8_t> data;

		for (uint8_t i = 0; i < motions.size(); i++)
		{
			// モータID
			data.push_back(motions[i].id);

			// 目標位置
			data.push_back(
				motions[i].data & 0xFF
			);
			data.push_back(
				(motions[i].data >> 8) & 0xFF
			);

			// 移動時間 [ms]
			data.push_back(
				transitionTime & 0xFF
			);
			data.push_back(
				(transitionTime >> 8) & 0xFF
			);
		}

		generateCommand(
			static_cast<uint8_t>(
				Instructions::writePositionWithTime
				),
			data
		);
	}

	void setFrequency(int vibrationFrequency)
	{
		std::vector<uint8_t> data;
		data.push_back(vibrationFrequency & 0xFF);
		data.push_back((vibrationFrequency >> 8) & 0xFF);
		generateCommand(static_cast<uint8_t>(Instructions::setVibrationFrequency), data);
	}

	void writeCurrent(std::vector< IdDataType> targetCurrent)
	{
		std::vector<uint8_t> data;

		for (uint8_t i = 0; i < targetCurrent.size(); i++) {
			data.push_back(targetCurrent[i].id);
			data.push_back(targetCurrent[i].data & 0xFF);
			data.push_back((targetCurrent[i].data >> 8) & 0xFF);
		}
		generateCommand(static_cast<uint8_t>(Instructions::writeCurrent), data);
	}

	void setTorque(uint8_t flag)
	{
		std::vector<uint8_t> data = { flag };
		generateCommand(static_cast<uint8_t>(Instructions::torqueEnable), data);
	}
	
	void executeCommand(uint8_t flag)
	{
		std::vector<uint8_t> data = { flag };
		generateCommand(static_cast<uint8_t>(Instructions::execute), data);
	}

	void vibrationEnable(uint8_t flag)
	{
		std::vector<uint8_t> data = { flag, 0, 0 };
		generateCommand(static_cast<uint8_t>(Instructions::setVibrationEnable) | static_cast<uint8_t>(Instructions::registerCommand), data);
	}

	void readPosition()
	{
		std::vector<uint8_t> data;
		generateCommand(static_cast<uint8_t>(Instructions::readPosition), data);
	}

	void encoderEnable(uint8_t flag)
	{
		std::vector<uint8_t> data = { flag };
		generateCommand(static_cast<uint8_t>(Instructions::encoderEnable), data);
	}

	void open(const char* port)
	{
		serialPort.open(port);
	}

	void DataReceivedHandler()
	{
		uint8_t data[1024];
		while (!exitFlag)
		{
			if (!serialPort.isConnected()) continue;

			int len = serialPort.read(data, 1024);
			if (len < 0) continue;
			for (int16_t i = 0; i < len; i++) {
				robot.processCommand(data[i]);
			}
		}
//		std::cout << " Robot Thread Finish" << std::endl;
	}

	void close()
	{
		// 先に受信threadへ終了を通知
		exitFlag = true;

		// COMを閉じる
		serialPort.close();

		// threadが終わるのを待つ
		if (th.joinable())
		{
			th.join();
		}
	}

	Robot() : th(&Robot::DataReceivedHandler, this){}
	~Robot() { close(); }

	void setCommandHandler(CallbackType callback) 
	{
		robot.callback = callback;
	}
};