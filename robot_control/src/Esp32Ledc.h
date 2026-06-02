#pragma once
#include <Arduino.h>
#include "ILedc.h"

class Esp32Ledc : public ILedc {
public:
    void setup(uint8_t channel, uint32_t freqHz, uint8_t resolution) override {
        ledcSetup(channel, freqHz, resolution);
    }
    void attachPin(uint8_t pin, uint8_t channel) override { ledcAttachPin(pin, channel); }
    void detachPin(uint8_t pin)                  override { ledcDetachPin(pin); }
    void write(uint8_t channel, uint32_t duty)   override { ledcWrite(channel, duty); }
};
