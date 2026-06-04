#pragma once
#include <driver/gpio.h>
#include "IGpio.h"

class Esp32Gpio : public IGpio {
public:
    // Pure IDF config — no Arduino pinMode mixing.
    void setOutput(uint8_t pin) override {
        gpio_config_t io = {};
        io.pin_bit_mask  = 1ULL << pin;
        io.mode          = GPIO_MODE_OUTPUT;
        io.pull_up_en    = GPIO_PULLUP_DISABLE;
        io.pull_down_en  = GPIO_PULLDOWN_DISABLE;
        io.intr_type     = GPIO_INTR_DISABLE;
        gpio_config(&io);
    }

    // Direct register write: always in IRAM, safe from ISR and during flash ops.
    void IRAM_ATTR write(uint8_t pin, uint8_t value) override {
        if (pin < 32) {
            if (value) GPIO.out_w1ts     = (1UL  << pin);
            else       GPIO.out_w1tc     = (1UL  << pin);
        } else {
            if (value) GPIO.out1_w1ts.val = (1UL << (pin - 32));
            else       GPIO.out1_w1tc.val = (1UL << (pin - 32));
        }
    }
};
