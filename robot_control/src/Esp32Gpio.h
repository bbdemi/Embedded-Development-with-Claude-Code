#pragma once
#include <Arduino.h>
#include <driver/gpio.h>
#include "IGpio.h"

class Esp32Gpio : public IGpio {
public:
    void setOutput(uint8_t pin) override {
        pinMode(pin, OUTPUT);
    }
    void write(uint8_t pin, uint8_t value) override {
        gpio_set_level((gpio_num_t)pin, value);
    }
};
