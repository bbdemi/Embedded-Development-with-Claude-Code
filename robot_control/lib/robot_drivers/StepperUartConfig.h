#pragma once
#include <stdint.h>
#include "IUart.h"

// Configures a TMC2209 stepper driver over its single-wire UART interface.
// Separates driver configuration from motion control (StepperDriver).
// slaveAddr : MS1/MS2 pins on the TMC2209 select address 0–3 (default 0).
// rsense    : sense resistor in milliohms — sets current scale (default 110 mΩ).
class StepperUartConfig {
public:
    explicit StepperUartConfig(IUart& uart, uint8_t slaveAddr = 0, uint16_t rsense = 110);

    void begin(uint32_t baudRate);
    void end();

    // Set microstepping resolution. Valid values: 1, 2, 4, 8, 16, 32, 64, 128, 256.
    void setMicrosteps(uint16_t microsteps);

    // Set run and hold currents in milliamps.
    // holdMA = 0  → minimum hold current (reduces heat when idle).
    // holdDelay   : 0–15; each tick ≈ 2^18 clock cycles before switching to hold.
    void setCurrent(uint16_t runMA, uint16_t holdMA = 0, uint8_t holdDelay = 6);

    // Low-level helpers for raw access.
    void sendText(const char* cmd);
    void sendBytes(const uint8_t* data, size_t len);

private:
    IUart&   _uart;
    uint8_t  _slaveAddr;
    uint16_t _rsense;
    uint32_t _chopconf;  // cached so setMicrosteps + setCurrent share CHOPCONF state

    void    writeRegister(uint8_t reg, uint32_t value);
    static uint8_t calcCRC(const uint8_t* data, uint8_t len);
    static uint8_t currentToCS(uint16_t ma, uint16_t rsense_mOhm, uint16_t vref_mV);
};
