#pragma once
#include <stdint.h>

class ITimer {
public:
    virtual void begin(uint32_t halfPeriodUs) = 0;
    virtual void attachInterrupt(void (*isr)()) = 0;
    virtual void setAlarmPeriod(uint32_t halfPeriodUs) = 0;
    virtual void enableAlarm() = 0;
    virtual void disableAlarm() = 0;
    virtual void end() = 0;
    virtual ~ITimer() = default;
};
