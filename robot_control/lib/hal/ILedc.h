




#pragma once
#include <stdint.h>

class ILedc {
public:
    virtual void setup(uint8_t channel, uint32_t freqHz, uint8_t resolution) = 0;
    virtual void attachPin(uint8_t pin, uint8_t channel) = 0;
    virtual void detachPin(uint8_t pin) = 0;
    virtual void write(uint8_t channel, uint32_t duty) = 0;
    virtual ~ILedc() = default;
};
