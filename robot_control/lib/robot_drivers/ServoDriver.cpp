#include "ServoDriver.h"

ServoDriver::ServoDriver(uint8_t pin, uint8_t channel, ILedc& ledc)
    : _pin(pin), _channel(channel), _ledc(ledc) {}

void ServoDriver::begin() {
    _ledc.setup(_channel, SERVO_FREQ_HZ, LEDC_RESOLUTION);
    _ledc.attachPin(_pin, _channel);
    setAngle(90.0f);
}

void ServoDriver::detach() {
    _ledc.detachPin(_pin);
}

void ServoDriver::setAngle(float degrees) {
    degrees = constrain(degrees, MIN_ANGLE, MAX_ANGLE);
    _currentAngle = degrees;

    uint16_t us = (uint16_t)map((long)(degrees * 10),
                                (long)(MIN_ANGLE * 10), (long)(MAX_ANGLE * 10),
                                MIN_PULSE_US, MAX_PULSE_US);
    setMicroseconds(us);
}

void ServoDriver::setMicroseconds(uint16_t us) {
    us = constrain(us, MIN_PULSE_US, MAX_PULSE_US);
    _currentUs = us;
    _ledc.write(_channel, _usToDuty(us));
}
