#pragma once
#include "IUart.h"
#include "StepperDriver.h"
#include "ServoDriver.h"

// Parses single-character commands from a UART byte stream.
// Format: <cmd><int>\n   where cmd is 's' (move steps), 'v' (servo angle), 'x' (stop).
// Call poll() from loop() — it drains all available bytes without blocking.
class CommandParser {
public:
    CommandParser(IUart& stream, StepperDriver& stepper, ServoDriver& servo);
    void poll();

private:
    void _feed(char c);
    void _execute();
    void _resetVal();

    IUart&        _stream;
    StepperDriver& _stepper;
    ServoDriver&   _servo;

    char _cmd      = 0;
    int  _val      = 0;
    bool _negative = false;
    bool _hasDigit = false;
};
