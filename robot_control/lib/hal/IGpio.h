#pragma once
#include <stdint.h>

class IGpio {
public:
    virtual void setOutput(uint8_t pin) = 0;
    virtual void write(uint8_t pin, uint8_t value) = 0;
    virtual ~IGpio() = default;
};
