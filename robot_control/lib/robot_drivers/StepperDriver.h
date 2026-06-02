#pragma once
#include <Arduino.h>
#include "IGpio.h"
#include "ITimer.h"

// Step/direction stepper driver. All hardware access goes through IGpio and
// ITimer, so the same class compiles for ESP32 (real HAL) and native tests
// (mock HAL) without any #ifdef guards.
// isrSlot: 0 or 1 — selects which static ISR entry point to use (max 2 instances).
class StepperDriver {
public:
    StepperDriver(uint8_t stepPin, uint8_t dirPin,
                  IGpio& gpio, ITimer& timer,
                  uint8_t isrSlot = 0, uint8_t enablePin = 0xFF);
    ~StepperDriver();

    void begin();
    void enable();
    void disable();

    void setSpeed(uint32_t stepsPerSec);
    void moveTo(int32_t position);
    void move(int32_t steps);
    void stop();

    bool isRunning() const {
        portENTER_CRITICAL(&_mux);
        bool r = _running;
        portEXIT_CRITICAL(&_mux);
        return r;
    }
    int32_t getPosition() const {
        portENTER_CRITICAL(&_mux);
        int32_t p = _currentPos;
        portEXIT_CRITICAL(&_mux);
        return p;
    }

    void setPosition(int32_t pos);

private:
    static void IRAM_ATTR _isr0();
    static void IRAM_ATTR _isr1();
    static StepperDriver* _instances[2];

    void IRAM_ATTR _handleISR();

    const uint8_t _stepPin;
    const uint8_t _dirPin;
    const uint8_t _enablePin;
    const uint8_t _isrSlot;

    IGpio&  _gpio;
    ITimer& _timer;

    volatile int32_t  _currentPos  = 0;
    volatile int32_t  _targetPos   = 0;
    volatile bool     _running     = false;
    volatile bool     _stepPhase   = false;

    volatile uint32_t _halfPeriodUs = 500;
    mutable portMUX_TYPE _mux;
};
