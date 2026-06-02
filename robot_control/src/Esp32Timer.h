#pragma once
#include <Arduino.h>
#include "ITimer.h"

class Esp32Timer : public ITimer {
public:
    explicit Esp32Timer(uint8_t timerNum) : _timerNum(timerNum) {}

    void begin(uint32_t halfPeriodUs) override {
        // 80 MHz / 80 = 1 MHz → 1 µs resolution
        _timer = timerBegin(_timerNum, 80, true);
        timerAlarmWrite(_timer, halfPeriodUs, true);
    }

    void attachInterrupt(void (*isr)()) override {
        timerAttachInterrupt(_timer, isr, true);
    }

    void setAlarmPeriod(uint32_t halfPeriodUs) override {
        timerAlarmWrite(_timer, halfPeriodUs, true);
    }

    void enableAlarm()  override { timerAlarmEnable(_timer); }
    void disableAlarm() override { timerAlarmDisable(_timer); }

    void end() override {
        if (!_timer) return;
        timerDetachInterrupt(_timer);
        timerEnd(_timer);
        _timer = nullptr;
    }

private:
    uint8_t     _timerNum;
    hw_timer_t* _timer = nullptr;
};
