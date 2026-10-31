#ifndef TT_MOTOR_CONTROL_H
#define TT_MOTOR_CONTROL_H

#include <Arduino.h>

// 控制一顆 TT 馬達，透過常見的馬達驅動板（L298N、TB6612 等）：
// 2 個方向腳位（IN1/IN2）+ 1 個調速用 PWM 腳位。
// 如果有兩顆馬達（例如車子左右輪），就各自宣告一個物件、各呼叫一次 begin()。
class TtMotorControl {
public:
    TtMotorControl();

    // 接上驅動板的方向腳位（in1Pin/in2Pin）與調速腳位（pwmPin）。
    // autoStopMs：呼叫 forward()/backward() 後，如果超過這段時間
    // 沒有再收到新的 forward()/backward() 呼叫，就會自動 stop()
    // （類似斷線保護，避免遙控訊號中斷時馬達一直轉）。
    void begin(int in1Pin, int in2Pin, int pwmPin, unsigned long autoStopMs = 300);

    // 要在 loop() 裡持續呼叫，用來偵測並執行自動停止
    void handle();

    // 正轉，speed 範圍 0~255（預設全速）
    void forward(uint8_t speed = 255);

    // 反轉，speed 範圍 0~255（預設全速）
    void backward(uint8_t speed = 255);

    // 停止（放手滑行，不是主動煞車）
    void stop();

private:
    int _in1Pin;
    int _in2Pin;
    int _pwmPin;
    uint8_t _pwmChannel;

    unsigned long _autoStopMs;
    unsigned long _lastCommandMs;
    bool _running;

    // 由高往低配置 PWM channel，避開 ESP32Servo 函式庫從 channel 0
    // 往上配置伺服機時可能用到的 channel，兩邊才不會互相搶用衝突。
    static uint8_t _nextPwmChannel;
};

#endif
