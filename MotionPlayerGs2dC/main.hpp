/*
* @file    main.hpp
* @author
* @date    2025 / 07 / 09
* @brief
*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include "HandReceiver.hpp"
#include "Robot.hpp"
#include "SensorCSV.hpp"

struct MotionType
{
	uint8_t instruction;
	std::vector<IdDataType> pose;
	int time;
};

/* Variables -----------------------------------------------------------------*/
extern std::vector<IdDataType> targetCurrent;
extern Robot robot;

extern std::vector<MotionType> extTumamiMotion;
extern std::vector<MotionType> extFingerMotion;
extern std::vector<MotionType> fiveTypePalpation;

/* Functions Prototypes ------------------------------------------------------*/
//void extFingerSetup();
void PlayMotion(std::vector<MotionType> motions);
void tummiInitialize();
void tummiPartial(int target, bool vibration, int moveTimeMs);
void tummiPartial3(int target, int target2, bool vibration);
void tummiPartial2(int target, int target2, bool vibration);
void tummiPartial4(int target, int target2, int target3, int target4, bool vibration, int moveTimeMs);
void tummiPartial5(int target, bool vibration);
void tummiPartial6(int target, int target2, int target3, int target4, bool vibration);
void vibSweep(bool vibration);