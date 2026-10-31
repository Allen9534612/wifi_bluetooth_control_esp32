#ifndef CONFIG_H
#define CONFIG_H

// ===== BLE 遙控設定 =====
// 手機掃描 BLE 時看到的裝置名稱
#define BLE_DEVICE_NAME "ESP32-Controller-BLE"

// ===== TT 馬達腳位（L298N / TB6612 等驅動板）=====
// IN1/IN2：方向腳位，PWM：調速腳位
#define MOTOR1_IN1_PIN 16
#define MOTOR1_IN2_PIN 17 
#define MOTOR1_PWM_PIN 18

#define MOTOR2_IN1_PIN 25
#define MOTOR2_IN2_PIN 26
#define MOTOR2_PWM_PIN 27

// ===== 伺服機腳位 =====
#define SERVO1_PIN 19
#define SERVO1_INITIAL_ANGLE 90

#define SERVO2_PIN 32
#define SERVO2_INITIAL_ANGLE 90

// 伺服機「正轉/反轉」每次按鍵要增減的角度
#define SERVO_STEP_DEGREES 10

#endif
