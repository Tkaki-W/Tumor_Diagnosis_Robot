/*
* @file    motion.cpp
* @author
* @date    2025 / 07 / 09
* @brief
*/
/* Includes ------------------------------------------------------------------*/
#include "main.hpp"
//強く押し込みたいならここの左の電流値を変える必要はある...100以下でお願いします
/* Variables -----------------------------------------------------------------*/
std::vector<IdDataType> targetCurrent = {
	{1, 50},//左屈曲
	{2, 50},//左回転
	{3, 50},//左並進
	{4, 50},//右屈曲
	{5, 50},//右回転
	{6, 50},//右並進
	{7, 350},//左対立
	{8, 100},//左回転
	{9, 50},//左屈曲根元
	{10, 50},//左屈曲指先
	{11, 350},//右対立
	{12, 100},//右回転
	{13, 50},//右屈曲根元
	{14, 50},//右屈曲指先
	{15, 50},//左屈曲指先
	{16, 50}//右屈曲指先
};

std::vector<MotionType> extTumamiMotion = {
	//7 90, 8 -90, 9 90 10 -90, 11 -90, 12 90, 13 -90, 14 90 
	// 外指と中指でつまみ
	{ 1, { { 7, 150}, { 8, 150} }, 100}, // 7,8番を弱く
	{ 0, { { 7, -150} }, 500}, // 指を上げる
	{ 0, { { 2, 860} , {8, -150}, {10, -50} }, 2000},
	{ 0, { { 7, 0} }, 500}, // 指を下げる
	{ 0, { { 1, 360} }, 1000},
};
std::vector<MotionType> extFingerMotion = {
	//7 90, 8 -90, 9 90 10 -90, 11 -90, 12 90, 13 -90, 14 90 
	{ 0, { {7, 0}, {8, 0}, {9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0} }, 500}, // 初期姿勢
	{ 0, { {9, 220}, {10, 220}, {13, -220}, {14, -220} }, 1000}, // 指先を曲げる
	{ 1, { {7, 100}, {11, 100} }, 100}, // 付け根の力を抜く
	{ 1, { {9, 10}, {10, 10}, {13, 10}, {14, 10} }, 100}, // 指先を弱める
	{ 0, { {7, 650}, {11, -650} }, 1000}, // 付け根を落とす
	
	{ 1, { {8, 600}, {12, 600} }, 200}, // さする方向の力を強める
	{ 3, { {8, 0}, {12, 0}, {8, 80}, {12, -80} }, 500}, // さする
	{ 3, { {8, 80}, {12, -80}, {8, -20}, {12, 20} }, 500}, // さする
	{ 3, { {8, -20}, {12, 20}, {8, 80}, {12, -80} }, 500}, // さする
	{ 3, { {8, 80}, {12, -80}, {8, 0}, {12, 0} }, 500}, // さする

	{ 1, { {9, 40}, {13, 40} }, 100}, // 指先を少し強める
	{ 0, { {9, 50}, {13, -50},}, 400}, // ぱたぱたと押し込む
	{ 0, { {9, 250}, {13, -250},}, 400}, // ぱたぱたと押し込む
	{ 0, { {9, 50}, {13, -50},}, 400}, // ぱたぱたと押し込む
	{ 0, { {9, 250}, {13, -250},}, 400}, // ぱたぱたと押し込む
	{ 0, { {9, 220}, {13, -220},}, 400}, // ぱたぱたと押し込む
};

std::vector<MotionType> fiveTypePalpation = 
{
	// 初期位置
	// +, -, +, -, +, +
	{ 2, { {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0} ,{15,0},{16,0}}, 100},

	// さする動作
	{ 2, { {1, 0}, {2, -450}, {3, 50}, {4, 0}, {5, 0}, {6, 80} ,{15,0 },{16,0}}, 1000},
	{ 2, { {4, -250} }, 500}, //右指は添える
	{ 4, { {1, -30} }, 100}, // 1を弱める
	{ 2, { {1, 0}, {3, 180} }, 500},
	{ 2, { {1, -80}, {3, 0} }, 500},
	{ 2, { {1, -80}, {3, 0} }, 500},
	{ 2, { {1, 0}, {3, 180} }, 500},
	{ 2, { {1, 0}, {3, 180} }, 500},
	{ 2, { {1, -80}, {3, 0} }, 500},
	{ 2, { {1, -80}, {3, 0} }, 500},
	{ 2, { {1, 0}, {3, 180} }, 500},
	{ 2, { {1, 0}, {3, 180} }, 500},
	{ 2, { {1, 0}, {3, 0} }, 2000},

	// 押す動作
	{ 4, { {1, 50} }, 100}, // 1を強める
	{ 2, { {1, -100} }, 600},
	{ 2, { {1, 20} }, 400},
	{ 2, { {1, -100} }, 600},
	{ 2, { {1, 20} }, 400},
	{ 2, { {1, -100} }, 600},
	{ 2, { {1, 20} }, 1000},


	// つまむ動作 左指のほうが腫瘍に近いため、傾きを緩くする
	{ 2, { {1, 0}, {2, -600}, {3, 100}, {4, 0}, {5, 600}, {6, 0} ,{15,0},{16,0}}, 500},
	{ 2, { {1, 30}, {4, -50} }, 2000},

	// 揺らす動作
	{ 2, { {3, 110}, {6, 30} }, 300},
	{ 2, { {3, 180}, {6, 0} }, 300},
	{ 2, { {3, 180}, {6, 0} }, 300},
	{ 2, { {3, 40}, {6, 50} }, 300},
	{ 2, { {3, 40}, {6, 50} }, 300},
	{ 2, { {3, 180}, {6, 0} }, 300},
	{ 2, { {3, 180}, {6, 0} }, 300},
	{ 2, { {3, 40}, {6, 50} }, 300},
	{ 2, { {3, 40}, {6, 50} }, 300},
	{ 2, { {3, 50}, {6, 30} }, 1000},

	// こねる動作
	//{ 2, { {1, 0}, {4, -30} }, 500},
	{ 4, { {2, 100}, {5, 100} }, 500},
	{ 2, { {2, -600}, {5, 550} }, 500},
	{ 2, { {2, -400}, {5, 300} }, 500},
	{ 2, { {2, -600}, {5, 550} }, 500},
	{ 2, { {2, -400}, {5, 350} }, 500},
	{ 2, { {2, -600}, {5, 550} }, 500},
	{ 2, { {2, -400}, {5, 350} }, 500},
	{ 2, { {2, -600}, {5, 550} }, 500},
	{ 2, { {15, -30}, {16, 30} }, 500},
	/*	{2, {{1, 0}, {4, -30}}, 1000},
	{ 2, { {1, 50}, {4, -80} }, 200},
	{ 2, { {1, 0}, {4, -30} }, 200},
	{ 2, { {1, 50}, {4, -80} }, 200},
	{ 2, { {1, 0}, {4, -30} }, 200},
	{ 2, { {1, 50}, {4, -80} }, 200},
	{ 2, { {1, 0}, {4, -30} }, 200},*/

	// 終了姿勢
	{ 2, { {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0} ,{ 15,0 },{16,0} }, 1500},

	{ 4, {
	{7, 350},
	{8, 300},
	{9, 50},
	{10, 50},
	{11, 350},
	{12, 300},
	{13, 50},
	{14, 50} }, 100},

	{ 2, { {7, 0}, {8, 0}, {9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0} }, 1500}, // 初期姿勢

};

void PlayMotion(std::vector<MotionType> motions)
{
	for (uint8_t i = 0; i < motions.size(); i++) {
		if (motions[i].instruction == 0) {
			// 通常の位置制御
			robot.writePosition(motions[i].pose);
			std::this_thread::sleep_for(std::chrono::milliseconds(motions[i].time));
		}
		else if (motions[i].instruction == 1) {
			// 電流指定
			robot.writeCurrent(motions[i].pose);
			std::this_thread::sleep_for(std::chrono::milliseconds(motions[i].time));
		}
		else if (motions[i].instruction == 2) {
			// コマンド登録
			robot.convertMotionToCommand(motions[i].pose, motions[i].time);
		}
		else if (motions[i].instruction == 3) {
			// 分割位置制御
			// モーション内の初期位置と目標位置を取得
			std::vector<IdDataType> start, target;

			for (uint8_t p = 0; p < motions[i].pose.size(); p++) {
				uint8_t id = motions[i].pose[p].id;
				int position = motions[i].pose[p].data;

				bool startDetected = false;
				// startに登録が無いか検索
				for (uint8_t s = 0; s < start.size(); s++) {
					if (id == start[s].id) {
						startDetected = true;
						break;
					}
				}
				// 無かったら登録
				if (!startDetected) {
					start.push_back({ id, position });
					continue;
				}

				// 有ったらターゲットを更新
				bool targetDetected = false;
				for (uint8_t t = 0; t < target.size(); t++) {
					if (id == target[t].id) {
						targetDetected = true;
						target[t].data = position;
						break;
					}
				}
				// 無かったら登録
				if (!targetDetected) {
					target.push_back({ id, position });
				}
			}

			for (int time = 0; time <= motions[i].time; time += 10)
			{
				// startリストから対象角度を計算
				std::vector<IdDataType> pose;

				for (int s = 0; s < start.size(); s++) {
					for (int t = 0; t < target.size(); t++) {
						if (start[s].id == target[t].id) {
							// 位置を計算
							int position = start[s].data + (double)(target[t].data - start[s].data) / (double)motions[i].time * (double)time;
							pose.push_back({ start[s].id, position });
//							std::cout << (int)start[s].id << ", " << position << std::endl;
						}
					}
				}
				robot.writePosition(pose);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

		}
		else if (motions[i].instruction == 4) {
			// コマンド登録
			robot.convertMotionToCommand(motions[i].pose, motions[i].time, (uint8_t)Instructions::writeCurrent);
		}
	}
}

void tummiInitialize()
{
	robot.writePosition({
			   {1, 208},
			   {2, 2},
			   {3, 20},
			   {4, -173},
			   {5, 2},
			   {6, 20},
		});
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	
	robot.writePosition({
			   {1, 0},
			   {2, -876},
			   {3, 20},
			   {4, -0},
			   {5, 837},
			   {6, 20},
		});
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void tummiPartial(int target, bool vibration, int moveTimeMs)
{	
	robot.executeCommand(0);
	robot.vibrationEnable(vibration ? 1 : 0);
	robot.executeCommand(1);
	robot.convertMotionToCommand({
	   {1, target},
	   {4, -target},
		}, moveTimeMs);
}
void tummiPartial3(int target, int target2, bool vibration)
{
	robot.vibrationEnable(vibration ? 1 : 0);
	robot.convertMotionToCommand({
	   {3, target2},
	    {1, target},
		}, 1000);
}
void tummiPartial4(int target, int target2, int target3, int target4, bool vibration, int moveTimeMs)
{
	robot.vibrationEnable(vibration ? 1 : 0);
	robot.convertMotionToCommand({
		{1, target},
		{4, -target2},
		{3, target3},
		{6, target4},
		}, moveTimeMs);
}

void tummiPartial5(int target, bool vibration)
{
	robot.vibrationEnable(vibration ? 1 : 0);
	robot.convertMotionToCommand({
	   {1, target},
		}, 1000);

}

void tummiPartial6(int target, int target2, int target3, int target4, bool vibration)
{
	robot.vibrationEnable(vibration ? 1 : 0);
	robot.convertMotionToCommand({
		{1, target},
		{4, -target2},
		{15, -target3},
		{16, -target4},
		}, 1000);
}



void vibSweep(bool vibration)
{
	robot.vibrationEnable(vibration ? 1 : 0);

	for (int freq = 40; freq <= 55; freq++)
	{
		robot.setFrequency(freq);

		std::cout << "Frequency : "
			<< freq << " Hz" << std::endl;

		std::this_thread::sleep_for(
			std::chrono::milliseconds(3000)
		);
	}

	
}



/*std::vector<MotionType> extFingerMotion = {
		{ { {7, 0}, {8, 0}, {9, 0}, {10, -900}, {11, 0}, {12, 0}, {13, 0}, {14, 900} }, 3000},
		{ { {7, 600}, {11, -600} }, 1000 },
		{ { {8, -40}, {12, 30} }, 1000 },
		{ { {10, 600}, {14, 450} }, 1000 },
		{ { {9, 100}, {13, -100} }, 1000 },
		{ { {10, 300}, {14, 150} }, 1000 },
		{ { {9, 200}, {13, -200} }, 1000 },
		{ { {10, 100}, {14, 0} }, 1000 },
		{ { {9, 400}, {13, -400} }, 3000 },
		{ { {9, 200}, {13, -200} }, 1000 },
		{ { {9, 400}, {13, -400} }, 3000 },
		{ { {9, 200}, {13, -200} }, 1000 },
		{ { {9, 400}, {13, -400} }, 3000 },
		{ { {9, 300}, {13, -300} }, 1000 },
		{ { {8, -80}, {12, -30} }, 500 },
		{ { {9, 350}, {13, -350} }, 500 },
		{ { {7, 550}, {11, -550} }, 500 },
		{ { {8, -120}, {12, -80} }, 500 },
		{ { {9, 300}, {13, -300} }, 500 },
		{ { {7, 450}, {11, -450} }, 500 },
		{ { {8, -150}, {12, -120} }, 500 },
		{ { {9, 350}, {13, -350} }, 500 },
		{ { {7, 350}, {11, -350} }, 500 },
		{ { {8, -50}, {12, 0} }, 200 },
		{ { {7, 450}, {11, -450} }, 500 },
		{ { {8, -80}, {12, -30} }, 500 },
		{ { {9, 350}, {13, -350} }, 500 },
		{ { {7, 550}, {11, -550} }, 500 },
		{ { {8, -120}, {12, -80} }, 500 },
		{ { {9, 300}, {13, -300} }, 500 },
		{ { {7, 450}, {11, -450} }, 500 },
		{ { {8, -150}, {12, -120} }, 500 },
		{ { {9, 350}, {13, -350} }, 500 },
		{ { {7, 350}, {11, -350} }, 500 },
		{ { {9, -100} }, 1000 },
		{ { {9, 350} }, 2000 },
		{ { {9, -100} }, 1000 },
		{ { {9, 350} }, 2000 },
};

void extFingerSetup()
{
	for (uint8_t i = 0; i < extFingerMotion.size(); i++) {
		robot.writePosition(extFingerMotion[i].motions);
		std::this_thread::sleep_for(std::chrono::milliseconds(extFingerMotion[i].time));
	}
}*/
