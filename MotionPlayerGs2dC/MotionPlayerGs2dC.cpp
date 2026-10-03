/*
* @file    MotionPlayerGs2dC.cpp
* @author
* @date    2025 / 07 / 09
* @brief
*/

/* Includes ------------------------------------------------------------------*/
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>
#include "main.hpp"
#include "RLBridge.hpp"
#include <conio.h>
#include <thread>



#define DEMO_MODE (0) // 0 : つまみ計測, 1 : 外指含めたデモ 2 : 外指中指つまみ
#define RL_TRAINING_MODE (1) // 0 : 従来通りの固定つまみ試行, 1 : 強化学習エピソードループ

/* Variables -----------------------------------------------------------------*/
Robot robot;
HandReceiver udp;
SensorStreamWriter writer;      // haptic
RLBridge rlBridge;               // 強化学習(Python)との通信
//EncoderCSV encoderWriter;       // encoder

std::ofstream encoderFile;
std::ofstream motionEventFile;

int motionCount = 0;
int motionMax = 15;
int demoVibMax = 2;
// サーボが停止していると判定する閾値( Goal Position )
//int servoStopThreshold = 2048;
//int servoStopCount = 0;

//bool endFlag = false;

int robotCount = 0;

bool exitFlag = false;
bool startFlag = false;
bool saveCSVExitFlag = false;

// ============================================================
// つまみ実験の設定
// ============================================================

// つまみ位置
const int PINCH_TARGET = 30;

// 離した位置
const int RELEASE_TARGET = -30;

// 目標位置まで動く時間
// 大きくするほどゆっくり動く
const int PINCH_MOVE_MS = 3000;
const int RELEASE_MOVE_MS = 3000;

// 目標位置に到達した後、つまみ続ける時間
const int PINCH_HOLD_MS = 5000;

// 離した後、次の試行まで待つ時間
const int REST_MS = 3000;

// 通信・動作時間の余裕
const int MOTION_MARGIN_MS = 300;

// ============================================================
// 強化学習(RL_TRAINING_MODE)の設定
// ============================================================

// Pythonと通信するポート番号
const int RL_PORT = 12345;

// フェーズ2(微調整)で、何回小さく角度を補正するか
const int RL_FINE_TUNE_STEPS = 20;

// 微調整1回あたりの時間[ms]
const int RL_STEP_TIME_MS = 100;

/*
constexpr int ENC_BUFFER_SIZE = 1024 * 1024;
char encoderBuffer[ENC_BUFFER_SIZE];
int encoderBufferOffset = 0;
*/
const char* port = "\\\\.\\COM10";
//const char* port = "\\\\.\\COM7";

std::string encoderDirectory = "C:\\Users\\takan\\Desktop\\岩田研\\卒論\\エンコーダ";
//std::string encoderDirectory = "C:\\Users\\shiki\\Documents";

std::string sensorDirectory = "C:\\Users\\msiwa\\OneDrive\\ドキュメント\\MATLAB\\触診AI実験\\hapticclnfos";
//std::string sensorDirectory = "C:\\Users\\shiki\\Documents";

/* Main Program --------------------------------------------------------------*/
// コマンド受信関数
// readEncoderコマンドが来たらデータを保存

struct EncoderInfo
{
    int hour, minute, second, ms;
    uint16_t goalEncoder;
    uint16_t presentEncoder;
    int16_t pos[17];   // 1〜16を使う
    bool hasPos[17];
    int posHour = -1;
    int posMinute = -1;
    int posSecond = -1;
    int posMs = -1;
};

ThreadSafeQueue<EncoderInfo> encoderQueue;

/*
void WriteEncoderStream()
{
    if (encoderBufferOffset > 0)
    {
        encoderFile.write(encoderBuffer, encoderBufferOffset);
        encoderBufferOffset = 0;
    }
}
*/
void commandReceivedHandler(CallbackEventArgs command)
{
    uint8_t instruction = command.instruction - 0x80;
    if (!startFlag) return;

    // 最新のモータ位置
    static int16_t latestPos[17] = {};
    static bool hasLatestPos[17] = {};

    // 最新のエンコーダ値
    static uint16_t latestGoalEncoder = 0;
    static uint16_t latestPresentEncoder = 0;
    static bool hasLatestEncoder = false;

    // 最新のエンコーダ受信時刻
    static int encHour = -1;
    static int encMinute = -1;
    static int encSecond = -1;
    static int encMs = -1;


    // =========================================================
    // Encoderデータを受信
    // → 最新値だけ保存する
    // → ここではCSVには書かない
    // =========================================================
    if (instruction == (uint8_t)Instructions::readEncoder)
    {
        if (command.data.size() < 4) return;

        latestGoalEncoder =
            command.data[0] | (command.data[1] << 8);

        latestPresentEncoder =
            command.data[2] | (command.data[3] << 8);

        hasLatestEncoder = true;


        // Encoderを受信した時刻
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm lt;
        localtime_s(&lt, &time);

        encHour = lt.tm_hour;
        encMinute = lt.tm_min;
        encSecond = lt.tm_sec;
        encMs =
            (uint64_t)(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()
                ).count()
                ) % 1000;


        robotCount++;
        /*
        // 終了判定は今まで通り
        if (servoStopThreshold <= latestGoalEncoder)
        {
            servoStopCount++;

            if (servoStopCount > 400)
                endFlag = true;
        }
        else
        {
            servoStopCount = 0;
        }
        */
        return;
    }


    // =========================================================
    // Positionデータを受信
    // → 新しいデータなので、このタイミングでCSVへ送る
    // =========================================================
    if (instruction == (uint8_t)Instructions::readPosition)
    {
        if (command.data.size() < 3) return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm lt;
        localtime_s(&lt, &time);

        int posHour = lt.tm_hour;
        int posMinute = lt.tm_min;
        int posSecond = lt.tm_sec;

        int posMs =
            (uint64_t)(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()
                ).count()
                ) % 1000;


        // 新しく届いた位置データを保存
        for (size_t index = 0;
            index + 2 < command.data.size();
            index += 3)
        {
            uint8_t id = command.data[index];

            int16_t pos =
                (int16_t)(
                    (uint16_t)command.data[index + 1] |
                    ((uint16_t)command.data[index + 2] << 8)
                    );

            if (id >= 1 && id <= 16)
            {
                latestPos[id] = pos;
                hasLatestPos[id] = true;
            }
        }


        // まだEncoderを1回も受信していなければ書かない
        if (!hasLatestEncoder)
            return;


        // =====================================================
        // ここでCSV 1行分を作る
        // =====================================================
        EncoderInfo enc = {};

        // 最新Encoderデータの受信時刻
        enc.hour = encHour;
        enc.minute = encMinute;
        enc.second = encSecond;
        enc.ms = encMs;

        // 最新Encoder値
        enc.goalEncoder = latestGoalEncoder;
        enc.presentEncoder = latestPresentEncoder;

        // 今回のPosition受信時刻
        enc.posHour = posHour;
        enc.posMinute = posMinute;
        enc.posSecond = posSecond;
        enc.posMs = posMs;


        // pos1～pos16
        for (int id = 1; id <= 16; id++)
        {
            if (hasLatestPos[id])
            {
                enc.pos[id] = latestPos[id];
                enc.hasPos[id] = true;
            }
            else
            {
                enc.hasPos[id] = false;
            }
        }


        // ★ 新しいPositionを受信したときだけ1回追加
        encoderQueue.push(enc);

        return;
    }
}
void EncoderInfoToCSV(EncoderInfo enc)
{
    encoderFile
        << enc.hour << ","
        << enc.minute << ","
        << enc.second << ","
        << enc.ms << ","
        << enc.goalEncoder << ","
        << enc.presentEncoder << ","
        << enc.posHour << ","
        << enc.posMinute << ","
        << enc.posSecond << ","
        << enc.posMs;

    
    for (int id = 1; id <= 16; id++)
    {
        if (enc.hasPos[id])
            encoderFile << "," << enc.pos[id];
        else
            encoderFile << ",";
    }

    encoderFile << "\n";
}
void WriteMotionEvent(
    int trial,
    const std::string& event,
    int target)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm lt;
    localtime_s(&lt, &time);

    int ms =
        (uint64_t)(
            std::chrono::duration_cast<
            std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count()
            ) % 1000;

    motionEventFile
        << lt.tm_hour << ","
        << lt.tm_min << ","
        << lt.tm_sec << ","
        << ms << ","
        << trial << ","
        << event << ","
        << target
        << "\n";

    motionEventFile.flush();
}

void WaitAndReadPosition(int durationMs)
{
    auto start =
        std::chrono::steady_clock::now();

    while (
        std::chrono::steady_clock::now()
        - start
        < std::chrono::milliseconds(durationMs))
    {
        robot.readPosition();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }
}

// つまむ動作1回ぶん(残差RLのエピソード)
// フェーズ1: 目標位置付近まで一気に接近
// フェーズ2: 少しずつ角度を補正しながら微調整(Pythonとのやり取り)
void RunPinchRLEpisode()
{
    // フェーズ1: 接近
    robot.executeCommand(0);
    tummiPartial(PINCH_TARGET, false, PINCH_MOVE_MS);
    WaitAndReadPosition(PINCH_MOVE_MS + MOTION_MARGIN_MS);

    // フェーズ2: 微調整
    int currentTarget = PINCH_TARGET;

    for (int step = 0; step < RL_FINE_TUNE_STEPS; step++)
    {
        float delta = rlBridge.receiveAction();
        currentTarget += (int)delta;

        robot.executeCommand(0);
        tummiPartial(currentTarget, false, RL_STEP_TIME_MS);
        std::this_thread::sleep_for(std::chrono::milliseconds(RL_STEP_TIME_MS));

        float observation = 0.0f; // TODO: 触覚センサの値が決まったら置き換える
        rlBridge.sendObservation(observation);
    }

    // 離す
    robot.executeCommand(0);
    tummiPartial(RELEASE_TARGET, false, RELEASE_MOVE_MS);
    WaitAndReadPosition(RELEASE_MOVE_MS + MOTION_MARGIN_MS);
}

void SaveCSV()
{
    int writecount = 0;

    while (!saveCSVExitFlag || udp.size() != 0 || !encoderQueue.empty())
    {
        if (udp.infoDataAvailable())
        {
            writer.HapticInfoToStream(udp.popInfo());
        }

        EncoderInfo enc;
        if (encoderQueue.try_pop(enc))
        {
            EncoderInfoToCSV(enc);
        }

        if (++writecount > 100)
        {
            writecount = 0;
            writer.WriteStream();
            encoderFile.flush();
        }
    }

    writer.WriteStream();
    encoderFile.flush();

    saveCSVExitFlag = false;
}

int main()
{
    // 時間からファイル名を生成してCSV作成
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

    ss << encoderDirectory + "\\enc_";
    ss << std::setfill('0') << std::right << std::setw(2) <<year << "-";
    ss << std::setfill('0') << std::right << std::setw(2) << month << "-";
    ss << std::setfill('0') << std::right << std::setw(2) << day << "_";
    ss << std::setfill('0') << std::right << std::setw(2) << hour << "-";
    ss << std::setfill('0') << std::right << std::setw(2) << minute << "-";
    ss << std::setfill('0') << std::right << std::setw(2) << second << ".csv";

    std::stringstream eventSs;

    eventSs << encoderDirectory + "\\motion_event_";
    eventSs << std::setfill('0') << std::right
        << std::setw(2) << year << "-";
    eventSs << std::setfill('0') << std::right
        << std::setw(2) << month << "-";
    eventSs << std::setfill('0') << std::right
        << std::setw(2) << day << "_";
    eventSs << std::setfill('0') << std::right
        << std::setw(2) << hour << "-";
    eventSs << std::setfill('0') << std::right
        << std::setw(2) << minute << "-";
    eventSs << std::setfill('0') << std::right
        << std::setw(2) << second << ".csv";

    motionEventFile.open(eventSs.str());

    motionEventFile
        << "hour,minute,second,ms,"
        << "trial,event,target\n";

    motionEventFile.flush();

#if DEMO_MODE == 0
    encoderFile.open(ss.str());
    encoderFile << "hour,minute,second,ms,goalEncoder,presentEncoder,posHour,posMinute,posSecond,posMs";
    for (int id = 1; id <= 16; id++)
    {
        encoderFile << ",pos" << id;
    }
    encoderFile << "\n";
#endif
    // UDP起動
    udp.open("192.168.1.4", 9999);

    // 書き込み監視スレッド（UDPクラスにまとめて良い気がする...）
#if DEMO_MODE == 0
    writer.generateFile(sensorDirectory);
#endif
    std::thread csvThread(SaveCSV);

    // COMと受信関数起動
    robot.open(port);
    robot.setCommandHandler(commandReceivedHandler);
    
    // トルクON,電流設定、モーションバッファクリア
    robot.setTorque(1);
    robot.writeCurrent(targetCurrent);
    robot.executeCommand(0);

    // バイブレーションのFreqを100Hzへ
    robot.setFrequency(0);

    // 0点に移動
    robot.writePosition({
               {1, 0},
               {2, 0},
               {3, 0},
               {4, 0},
               {5, 0},
               {6, 0},
              {7, 0},
 //              {7, -450},
               {8, 0},
//               {9, -200},
               {9, 0},
               {10, 0},
               {11, 0},
//               {11, 450},
               {12, 0},
               {13, 0},
//               {13, 200},
               {14, 0},
               {15, 0},
               {16, 0}
        });

#if DEMO_MODE == 0
    std::cout << "Press Enter to phtm";
    std::cin.get();

#elif DEMO_MODE == 1
    std::cout << "Press Enter to Futo-momo-move";
    std::cin.get();

    robot.writePosition({
               {1, 0},
               {2, 0},
               {3, 0},
               {4, 0},
               {5, 0},
               {6, 0},
               {7, 0},
               //{7, -450},
               {8, 0},
//               {9, -200},
               {9, 0},
               {10, 0},
               {11, 0},
//               {11, 450},
               {12, 0},
               {13, 0},
//               {13, 200},
               {14, 0},
               {15, 0},
               {16, 0},
        });

    std::cout << "Press Enter to Futo-momo-motion";
    std::cin.get();
#endif

    // センサストリームを起動


    // つまみモーション初期位置へ移動
#if DEMO_MODE == 1
   // PlayMotion(extFingerMotion);
    PlayMotion(fiveTypePalpation);
    robot.executeCommand(1);
#elif DEMO_MODE == 2
    PlayMotion(extTumamiMotion);
#else
    //tummiInitialize();
#endif
    std::cout << "Finished. Press Enter to start tumami-motion";
    startFlag = true;
    std::cin.get();
#if DEMO_MODE != 0
    return 0;
#endif
    // つまみモーション初回登録
    /*
    tummiPartial(300, false);
    tummiPartial(0, false);
    std::cout << "Play Count : " << motionCount << "/" << motionMax;
    motionCount++;
    if (motionCount > motionMax) exitFlag = true;
    robot.executeCommand(1);
    */

    int vibFlag;

    /*
    int vibFlag;

    robot.vibrationEnable(1);

    for (int freq = 40; freq <= 55; freq++)
    {
        robot.setFrequency(freq);

        std::cout << "Frequency = "
            << freq << " Hz" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }

    // 振動OFF
    robot.vibrationEnable(0);

    // 離す動作
    tummiPartial(0, false);
    robot.executeCommand(1);

    // 離す動作が終わるまで待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    */
    /*
    robot.setFrequency(100);
    for (vibFlag = 0; vibFlag <= demoVibMax; vibFlag++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2400));
        tummiPartial(300, true);
        tummiPartial(0, false);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2400));

    robot.setFrequency(50);

    for (vibFlag = 0; vibFlag <= demoVibMax; vibFlag++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2400));
        tummiPartial(300, true);
        tummiPartial(0, false);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2400));

    robot.setFrequency(150);

    for (vibFlag = 0; vibFlag <= demoVibMax; vibFlag++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2400));
        tummiPartial(300, true);
        tummiPartial(0, false);
    }

    std::this_thread:c0:sleep_for(std::chrono::milliseconds(2400));
    */
    /*
    robot.convertMotionToCommand({
    {2, -600},
    {5, 600},
    }, 1200);
    
    for (vibFlag = 0; vibFlag <= demoVibMax; vibFlag++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2400));
        tummiPartial(350,  true);

        tummiPartial(0,  false);
    }
    */
    //２指で斜め45度でつまむとき，対向つまみから斜め45度にしてつまむ用意をした姿勢
    
    robot.convertMotionToCommand({
    {2, -550},
    {5, 600},
    {3, 30},
    {6, 0},
    {15, -30},
    {16, 45},
        }, 6000);
    
    robot.executeCommand(1);

    // 45°姿勢への移動完了を待つ
    WaitAndReadPosition(6300);
    /*
    robot.convertMotionToCommand({
      {2, 0},
      {3, 120},
      {5, 0},
      {6, 150},
      {15, 45},
      {16, 45},
        }, 6000);
    
    */
    
    /*
    robot.convertMotionToCommand({
    {1, -45},
    {2, 0},
    {3, 120},
    {5, 0},
    {6, 150},
    {15, -30},
    {16, 45},
        }, 6000);

        */

#if RL_TRAINING_MODE == 1
    // 強化学習エピソードループ(Pythonと接続して、つまむ動作を繰り返す)
    rlBridge.open(RL_PORT);

    motionCount = 0;
    while (motionCount < motionMax)
    {
        std::cout << "Episode " << motionCount + 1 << "/" << motionMax << std::endl;
        RunPinchRLEpisode();
        motionCount++;
    }

    rlBridge.close();
#else
    motionCount = 0;

    while (motionCount < motionMax)
    {
        const int trial = motionCount + 1;

        std::cout
            << "Trial "
            << trial
            << "/"
            << motionMax
            << std::endl;


        // ==========================================
        // 1. つまむ
        // ==========================================

        // 前回の登録済みコマンドをクリア
        robot.executeCommand(0);

        WriteMotionEvent(
            trial,
            "pinch_command",
            PINCH_TARGET
        );

        tummiPartial(
            PINCH_TARGET,
            false,
            PINCH_MOVE_MS
        );

        //robot.executeCommand(1);

        // つまみ位置まで移動するのを待つ
        WaitAndReadPosition(
            PINCH_MOVE_MS
            + MOTION_MARGIN_MS
        );


        // ==========================================
        // 2. 5秒間つまみ続ける
        // ==========================================

        WriteMotionEvent(
            trial,
            "pinch_hold_start",
            PINCH_TARGET
        );

        WaitAndReadPosition(
            PINCH_HOLD_MS
        );


        // ==========================================
        // 3. 離す
        // ==========================================

        robot.executeCommand(0);

        WriteMotionEvent(
            trial,
            "release_command",
            RELEASE_TARGET
        );

        tummiPartial(
            RELEASE_TARGET,
            false,
            RELEASE_MOVE_MS
        );

        //robot.executeCommand(1);

        WaitAndReadPosition(
            RELEASE_MOVE_MS
            + MOTION_MARGIN_MS
        );


        // ==========================================
        // 4. 離したことを記録
        // ==========================================

        WriteMotionEvent(
            trial,
            "release_complete",
            RELEASE_TARGET
        );


        // ==========================================
        // 5. 離した状態で3秒待つ
        //    MarkerDetectorはここで再初期化可能
        // ==========================================

        WaitAndReadPosition(
            REST_MS
        );

        WriteMotionEvent(
            trial,
            "rest_complete",
            RELEASE_TARGET
        );


        motionCount++;

        std::cout
            << "Play Count : "
            << motionCount
            << "/"
            << motionMax
            << std::endl;
    }
#endif

    // モーションをループ
    /*
    while (!exitFlag)
    {
        const int trial = motionCount + 1;

        // ========================================
        // つまんでいない区間
        // ここでMarkerDetectorを再初期化できる
        // ========================================

        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(3000))
        {
            robot.readPosition();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        }
        


        //tummiPartial(300, true); 2025年7月
        // tummiPartial2(100, -100, true);
        /*
        tummiPartial(150, true);
        tummiPartial(-30, false);
        */
        
        /*
        //可動性上面さするファントム位置2
        tummiPartial3(45, 130, false);
        tummiPartial3(45, 110, false);
        */
        /*
        //可動性片方の指45度で押す位置1
        tummiPartial3(45, 60, false);
        tummiPartial3(45, 80, false);
        */
        /*
        //可動性つまみファントム位置1
        WriteMotionEvent(
            trial,
            "pinch_command",
            180
        );

        tummiPartial(180, false, 5000);
        
        WriteMotionEvent(
            trial,
            "release_command",
            -30
        );

        tummiPartial(-30, false, 5000);
        

        /*
        //可動性揺らすファントム位置1
        tummiPartial4(30, 30,15,100, false);
        tummiPartial4(30, 30, 100, 15, false);
        */
        /*
                tummiPartial4(150, 150,15,100, false);
        tummiPartial4(150, 150, 100, 15, false);
        //可動性こねるファントム位置1
        tummiPartial6(50,50, 60, 0, false);
        tummiPartial6(0, 80, 0, -60, false);
        */
        /*
        //可動性上から押すファントム位置2
        tummiPartial5(150, false);
        tummiPartial5(-45,  false);
        */

        /*
        motionCount++;
        std::cout
            << "Play Count : "
            << motionCount
            << "/"
            << motionMax
            << std::endl;
        if (motionCount >= motionMax) exitFlag = true;
    }
    */

    // サーボのGoalPosition依存で待つように変更
   //while(!endFlag) std::this_thread::sleep_for(std::chrono::milliseconds(10));
//    std::this_thread::sleep_for(std::chrono::milliseconds(4800));
// 実験終了時刻を記録
    WriteMotionEvent(
        motionCount,
        "experiment_end",
        RELEASE_TARGET
    );

    // CSV保存スレッド終了
    saveCSVExitFlag = true;

    udp.close();
    robot.close();

    // 残っているデータを全部保存
    csvThread.join();

    encoderFile.flush();
    motionEventFile.flush();

    encoderFile.close();
    motionEventFile.close();

    writer.close();

    std::cout << std::endl;
    std::cout << "Finished." << std::endl;

    std::cin.get();
/*
    saveCSVExitFlag = true;

    udp.close();
    robot.close();

    csvThread.join();
    std::cout << std::endl;
    std::cout << "Finished. " << std::endl;
    std::cin.get();
    //WriteEncoderStream();
    encoderFile.close();
    writer.close();
    */
}