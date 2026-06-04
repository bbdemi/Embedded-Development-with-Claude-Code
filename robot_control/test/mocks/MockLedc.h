#pragma once
#include "ILedc.h"

class MockLedc : public ILedc {
public:
    uint8_t  lastChannel = 0;
    uint32_t lastDuty    = 0;
    void setup(uint8_t, uint32_t, uint8_t) override {}
    void attachPin(uint8_t, uint8_t)       override {}
    void detachPin(uint8_t)                override {}
    void write(uint8_t ch, uint32_t duty)  override { lastChannel = ch; lastDuty = duty; }
};
