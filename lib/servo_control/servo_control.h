#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <Arduino.h>
#include <ESP32Servo.h>

// 控制一顆 180 度伺服機（SG90 等常見角度型伺服機）
class ServoControl {
public:
    ServoControl();

    // 接上指定腳位，並轉到 initialAngle（預設置中 90 度）
    void begin(int pin, int initialAngle = 90);

    // 設定角度，範圍會被限制在 0~180 度
    void setAngle(int angle);

    // 讀取目前設定的角度
    int getAngle() const;

private:
    Servo _servo;
    int _angle;
};

#endif
