#include "CommandParser.h"

CommandParser::CommandParser(IUart& stream, StepperDriver& stepper, ServoDriver& servo)
    : _stream(stream), _stepper(stepper), _servo(servo) {}

void CommandParser::poll() {
    while (_stream.available()) {
        _feed((char)_stream.read());
    }
}

void CommandParser::_resetVal() {
    _val = 0;
    _negative = false;
    _hasDigit = false;
}

void CommandParser::_execute() {
    if (_cmd == 0) return;
    int val = _negative ? -_val : _val;
    if      (_cmd == 's') _stepper.move(val);
    else if (_cmd == 'v') _servo.setAngle((float)val);
    else if (_cmd == 'x') _stepper.stop();
    _cmd = 0;
    _resetVal();
}

void CommandParser::_feed(char c) {
    if (c == 's' || c == 'v' || c == 'x') {
        if (_cmd != 0) _execute();
        _cmd = c;
        _resetVal();
    } else if (c == '-' && !_hasDigit) {
        _negative = true;
    } else if (c >= '0' && c <= '9') {
        _hasDigit = true;
        _val = _val * 10 + (c - '0');
    } else if (c == '\n' || c == '\r') {
        if (_cmd != 0) _execute();
    }
    // other chars ignored
}
