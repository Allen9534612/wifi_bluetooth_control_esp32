#include <Arduino.h>
#include "config.h"
#include "ble_control.h"
#include "tt_motor_control.h"
#include "servo_control.h"

BleControl bleControl;
TtMotorControl motor1;
TtMotorControl motor2;
ServoControl servo1;
ServoControl servo2;

void onButtonPressed(const String &button) {
    Serial.print("收到按鍵: ");
    Serial.println(button);

    if (button.length() == 0) {
        return;
    }

    switch (button.charAt(0)) {
        //挖了個空給你寫
    }
}

void setup() {
    Serial.begin(115200);

    bleControl.onButtonPressed(onButtonPressed);
    bleControl.begin(BLE_DEVICE_NAME);

    motor1.begin(MOTOR1_IN1_PIN, MOTOR1_IN2_PIN, MOTOR1_PWM_PIN);
    motor2.begin(MOTOR2_IN1_PIN, MOTOR2_IN2_PIN, MOTOR2_PWM_PIN);

    servo1.begin(SERVO1_PIN, SERVO1_INITIAL_ANGLE);
    servo2.begin(SERVO2_PIN, SERVO2_INITIAL_ANGLE);
}

void loop() {
    bleControl.handle();
    motor1.handle();
    motor2.handle();
}
