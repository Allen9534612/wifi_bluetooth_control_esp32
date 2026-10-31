#include "servo_control.h"

ServoControl::ServoControl() : _angle(90) {}

void ServoControl::begin(int pin, int initialAngle) {
    _servo.attach(pin);
    setAngle(initialAngle);
}

void ServoControl::setAngle(int angle) {
    _angle = constrain(angle, 0, 180);
    _servo.write(_angle);
}

int ServoControl::getAngle() const {
    return _angle;
}
