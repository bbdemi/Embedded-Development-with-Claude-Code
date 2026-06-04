#pragma once
#include <Arduino.h>
#include "IUart.h"

// Wraps one of the ESP32's three hardware UART peripherals.
// uartNum: 0 = Serial, 1 = Serial1, 2 = Serial2
// txPin / rxPin: pass -1 to keep the hardware default pins.
class Esp32Uart : public IUart {
public:
    Esp32Uart(uint8_t uartNum, int txPin = -1, int rxPin = -1)
        : _serial(uartNum == 0 ? Serial : uartNum == 1 ? Serial1 : Serial2)
        , _txPin(txPin), _rxPin(rxPin) {}

    void begin(uint32_t baudRate) override {
        if (_txPin >= 0 && _rxPin >= 0)
            _serial.begin(baudRate, SERIAL_8N1, _rxPin, _txPin);
        else
            _serial.begin(baudRate);
    }

    void end() override { _serial.end(); }

    void write(uint8_t byte) override               { _serial.write(byte); }
    void write(const uint8_t* buf, size_t len) override { _serial.write(buf, len); }
    int  read()      override { return _serial.read(); }
    int  available() override { return _serial.available(); }

private:
    HardwareSerial& _serial;
    int _txPin;
    int _rxPin;
};
