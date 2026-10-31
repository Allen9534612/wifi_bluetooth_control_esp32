#include "tt_motor_control.h"

static const int PWM_FREQ_HZ = 1000;
static const int PWM_RESOLUTION_BITS = 8; // 配合 speed 參數 0~255 的範圍

uint8_t TtMotorControl::_nextPwmChannel = 15;

TtMotorControl::TtMotorControl()
    : _in1Pin(-1), _in2Pin(-1), _pwmPin(-1), _pwmChannel(0),
      _autoStopMs(100), _lastCommandMs(0), _running(false) {}

void TtMotorControl::begin(int in1Pin, int in2Pin, int pwmPin, unsigned long autoStopMs) {
    _in1Pin = in1Pin;
    _in2Pin = in2Pin;
    _pwmPin = pwmPin;
    _autoStopMs = autoStopMs;
    _pwmChannel = _nextPwmChannel--;

    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);

    ledcSetup(_pwmChannel, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(_pwmPin, _pwmChannel);

    stop();
}

void TtMotorControl::handle() {
    if (_running && millis() - _lastCommandMs >= _autoStopMs) {
        stop();
    }
}

void TtMotorControl::forward(uint8_t speed) {
    // 如果正轉/反轉方向跟馬達實際轉向相反，把 IN1/IN2 接線對調即可
    digitalWrite(_in1Pin, HIGH);
    digitalWrite(_in2Pin, LOW);
    ledcWrite(_pwmChannel, speed);
    _running = true;
    _lastCommandMs = millis();
}

void TtMotorControl::backward(uint8_t speed) {
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, HIGH);
    ledcWrite(_pwmChannel, speed);
    _running = true;
    _lastCommandMs = millis();
}

void TtMotorControl::stop() {
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    ledcWrite(_pwmChannel, 0);
    _running = false;
}
