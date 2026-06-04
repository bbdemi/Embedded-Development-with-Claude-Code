#pragma once
#include "IGpio.h"

class MockGpio : public IGpio {
public:
    uint8_t lastPin   = 0;
    uint8_t lastValue = 0;
    void setOutput(uint8_t)              override {}
    void write(uint8_t pin, uint8_t val) override { lastPin = pin; lastValue = val; }
};
