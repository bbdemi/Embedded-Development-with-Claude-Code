#pragma once
#include "ITimer.h"

class MockTimer : public ITimer {
public:
    bool   alarmEnabled  = false;
    void (*attachedIsr)() = nullptr;

    void begin(uint32_t)                override {}
    void attachInterrupt(void (*isr)()) override { attachedIsr = isr; }
    void setAlarmPeriod(uint32_t)       override {}
    void enableAlarm()                  override { alarmEnabled = true; }
    void disableAlarm()                 override { alarmEnabled = false; }
    void end()                          override {}
};
