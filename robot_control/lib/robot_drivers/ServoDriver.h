#pragma once
#include <Arduino.h>
#include "ILedc.h"

// Servo driver using ESP32 LEDC peripheral (50 Hz, 16-bit resolution).
// All LEDC calls go through ILedc, so the class is portable and fully
// unit-testable without hardware.
class ServoDriver {
public:
    static constexpr uint16_t MIN_PULSE_US = 500;
    static constexpr uint16_t MAX_PULSE_US = 2500;
    static constexpr float    MIN_ANGLE    = 0.0f;
    static constexpr float    MAX_ANGLE    = 180.0f;

    ServoDriver(uint8_t pin, uint8_t channel, ILedc& ledc);

    void begin();
    void detach();

    void setAngle(float degrees);
    void setMicroseconds(uint16_t us);

    float    getAngle()        const { return _currentAngle; }
    uint16_t getMicroseconds() const { return _currentUs; }

private:
    static constexpr uint32_t SERVO_FREQ_HZ   = 50;
    static constexpr uint8_t  LEDC_RESOLUTION = 16;

    static constexpr uint32_t _usToDuty(uint16_t us) {
        return (uint32_t)us * 65536u / 20000u;
    }

    const uint8_t _pin;
    const uint8_t _channel;
    ILedc&   _ledc;
    float    _currentAngle = 90.0f;
    uint16_t _currentUs    = 1500;
};
