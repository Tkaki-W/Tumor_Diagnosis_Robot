enum class Instructions : unsigned char
{
    dfuReboot = 0x02,             // DFUファームウェア書き込み用。普段は使用しない。
    passThrough = 0x03,             // 電源OFFになるまで、送られてくるコマンドをサーボにパススルーするモードに切り替える。Dynamixel Wizardが使用可能

    execute = 0x06,
    encoderEnable = 0x07,

    torqueEnable = 0x10,             // 全体のトルクをON/OFFする

    writeCurrent = 0x11,
    writePosition = 0x14,             // 指定のサーボモーターの位置変更司令。単位は0.1deg or 0.1mm
    writePositionWithTime = 0x13,
    writeTransitionTime = 0x15,             // 遷移時間(ms)司令
    writePID = 0x16,             // PIDゲイン変更コマンド

    readPosition = 0x24,             // 全サーボモーターの現在位置読み込み。単位は0.1deg or 0.1mm
    readPID = 0x26,             // 全サーボモーターのPID値取得
    readTemperature = 0x27,             // 全サーボモーターの温度取得。単位は度
    readError = 0x28,             // 全サーボモーターのエラーステータス取得
    readCurrent = 0x29,             // 全サーボモーターの電流取得。単位はmA

    readEncoder = 0x2A,

    setPeltierDuty = 0x3A,             // ペルチェ素子への出力変更。-100 ~ 100
    setVibrationEnable = 0x3B,             // 振動モーターのON/OFF

    getFrequency = 0x3C,             // 基板の制御周期を取得。単位はHzで、正常時は50Hz
    getADC = 0x3D,              // 基板のADコンバータの値を取得。0 ~ 4095

    ReturnPosition=0xA4,


    registerCommand = 0b01000000,
    setVibrationFrequency = 0x3E,
};