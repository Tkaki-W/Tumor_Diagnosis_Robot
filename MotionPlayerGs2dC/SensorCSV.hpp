/*
* @file    SensorCSV.hpp
* @author
* @date    2025 / 07 / 10
* @brief
*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include "HandReceiver.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <inttypes.h>
#include <cstdio>    // C‚ÌFILE*, fwrite‚È‚Ç

#define BUFFER_SIZE (120000)

/* Class ---------------------------------------------------------------------*/
class SensorStreamWriter
{
private:
    //    std::ofstream file;
    bool isOpen = false;
    uint64_t currentCount = 0;
    //FILE* fp;
    FILE* fp = nullptr;

    char stringBuffer[BUFFER_SIZE];
    int bufferOffset = 0;

public:
    SensorStreamWriter() {}
    ~SensorStreamWriter() { close(); }

    void close()
    {
        if (fp != nullptr)
        {
            fclose(fp);
            fp = nullptr;
            isOpen = false;
        }
    }

    void generateFile(std::string directory)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm lt;
        localtime_s(&lt, &time);
        std::stringstream ss;

        int year = lt.tm_year + 1900;
        int month = lt.tm_mon + 1;
        int day = lt.tm_mday;
        int hour = lt.tm_hour;
        int minute = lt.tm_min;
        int second = lt.tm_sec;

        ss << directory + "\\handpushing_";
        ss << std::setfill('0') << std::right << std::setw(2) << year << "-";
        ss << std::setfill('0') << std::right << std::setw(2) << month << "-";
        ss << std::setfill('0') << std::right << std::setw(2) << day << "_";
        ss << std::setfill('0') << std::right << std::setw(2) << hour << "-";
        ss << std::setfill('0') << std::right << std::setw(2) << minute << "-";
        ss << std::setfill('0') << std::right << std::setw(2) << second << ".csv";
        //        file.open(ss.str());
        fopen_s(&fp, ss.str().c_str(), "w");
        isOpen = true;
    }

    uint64_t getCount() { return currentCount; }

    void HapticInfoToStream(HandInfo info)
    {
        // accel
        bufferOffset += snprintf(stringBuffer + bufferOffset, BUFFER_SIZE - bufferOffset, "%d,%d,%d,", (int)currentCount, info.sequence, info.fingerInfos[0].accelSequence);
        for (uint8_t i = 0; i < 5; i++)
        {
            for (uint8_t t = 0; t < 3; t++) {

                bufferOffset += snprintf(stringBuffer + bufferOffset, BUFFER_SIZE - bufferOffset, "%f,", info.fingerInfos[i].accel[t]);
            }
        }

        // force
        bufferOffset += snprintf(stringBuffer + bufferOffset, BUFFER_SIZE - bufferOffset, "%d,", info.fingerInfos[0].forceSequence);
        for (uint8_t i = 0; i < 5; i++)
        {
            for (uint8_t t = 0; t < 3; t++) {
                bufferOffset += snprintf(stringBuffer + bufferOffset, BUFFER_SIZE - bufferOffset, "%f,", info.fingerInfos[i].force[t]);
            }
        }

        // temp
        bufferOffset += snprintf(stringBuffer + bufferOffset, BUFFER_SIZE - bufferOffset, "%d,", info.fingerInfos[0].temperatureSequence);
        for (uint8_t i = 0; i < 5; i++)
        {
            for (uint8_t t = 0; t < 6; t++) {
                bufferOffset += snprintf(stringBuffer + bufferOffset, BUFFER_SIZE - bufferOffset, "%f,", info.fingerInfos[i].temperature[t]);
            }
        }

        int hour = info.hour;
        int minute = info.minute;
        int second = info.second;
        int ms = info.ms;

        bufferOffset += snprintf(stringBuffer + bufferOffset, BUFFER_SIZE - bufferOffset, "%d,%d,%d,%d\n", hour, minute, second, ms);
        currentCount++;
    }

    void WriteStream()
    {
        size_t written = fwrite(stringBuffer, sizeof(char), bufferOffset, fp);
        bufferOffset = 0;
    }
};