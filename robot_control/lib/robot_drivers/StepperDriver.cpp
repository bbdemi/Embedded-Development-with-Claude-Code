#include "StepperDriver.h"

StepperDriver* StepperDriver::_instances[2] = {nullptr, nullptr};

StepperDriver::StepperDriver(uint8_t stepPin, uint8_t dirPin,
                             IGpio& gpio, ITimer& timer,
                             uint8_t isrSlot, uint8_t enablePin)
    : _stepPin(stepPin), _dirPin(dirPin),
      _enablePin(enablePin), _isrSlot(isrSlot),
      _gpio(gpio), _timer(timer)
{
    _mux = portMUX_INITIALIZER_UNLOCKED;
    if (_isrSlot < 2) _instances[_isrSlot] = this;
}

StepperDriver::~StepperDriver() {
    _timer.end();
    if (_isrSlot < 2) _instances[_isrSlot] = nullptr;
}

void StepperDriver::begin() {
    _gpio.setOutput(_stepPin);
    _gpio.setOutput(_dirPin);
    _gpio.write(_stepPin, 0);
    _gpio.write(_dirPin,  0);

    if (_enablePin != 0xFF) {
        _gpio.setOutput(_enablePin);
        disable(); // most drivers: active-low → HIGH = disabled
    }

    _timer.begin(_halfPeriodUs);
    _timer.attachInterrupt(_isrSlot == 0 ? _isr0 : _isr1);
    // Alarm stays disabled until moveTo/move is called
}

void StepperDriver::enable() {
    if (_enablePin != 0xFF) _gpio.write(_enablePin, 0); // active-low
}

void StepperDriver::disable() {
    if (_enablePin != 0xFF) _gpio.write(_enablePin, 1);
}

void StepperDriver::setSpeed(uint32_t stepsPerSec) {
    if (stepsPerSec == 0) return;
    // Cap at ~100 k steps/sec (10 µs half-period)
    uint32_t half = max(10u, 1000000u / (stepsPerSec * 2));
    portENTER_CRITICAL(&_mux);
    _halfPeriodUs = half;
    portEXIT_CRITICAL(&_mux);
    // setAlarmPeriod (timerAlarmWrite) acquires its own internal spinlock.
    // Calling it inside _mux would nest two spinlocks → deadlock on dual-core ESP32.
    _timer.setAlarmPeriod(half);
}

void StepperDriver::moveTo(int32_t position) {
    portENTER_CRITICAL(&_mux);
    bool samePos = (position == _currentPos);
    portEXIT_CRITICAL(&_mux);
    if (samePos) return;

    stop(); // disables alarm; ISR cannot fire after this returns

    bool forward = (position > _currentPos);
    _gpio.write(_dirPin, forward ? 1 : 0);
    delayMicroseconds(1); // ≥200 ns DIR setup time (A4988/DRV8825)

    portENTER_CRITICAL(&_mux);
    _targetPos = position;
    _stepPhase = false;
    _running   = true;
    _timer.enableAlarm();
    portEXIT_CRITICAL(&_mux);
}

void StepperDriver::move(int32_t steps) {
    moveTo(_currentPos + steps);
}

void StepperDriver::stop() {
    portENTER_CRITICAL(&_mux);
    if (!_running) {
        portEXIT_CRITICAL(&_mux);
        return;
    }
    _running   = false;
    _stepPhase = false;
    _timer.disableAlarm();
    _gpio.write(_stepPin, 0);
    portEXIT_CRITICAL(&_mux);
}

void StepperDriver::setPosition(int32_t pos) {
    stop();
    portENTER_CRITICAL(&_mux);
    _currentPos = pos;
    _targetPos  = pos;
    portEXIT_CRITICAL(&_mux);
}

// Two-phase ISR: fires at 2× step rate.
// Rising edge → step HIGH; falling edge → step LOW + count + check target.
void IRAM_ATTR StepperDriver::_handleISR() {
    portENTER_CRITICAL_ISR(&_mux);

    if (!_running) {
        portEXIT_CRITICAL_ISR(&_mux);
        return;
    }

    if (!_stepPhase) {
        _gpio.write(_stepPin, 1);
        _stepPhase = true;
    } else {
        _gpio.write(_stepPin, 0);
        _stepPhase = false;

        if (_targetPos > _currentPos) _currentPos++;
        else                          _currentPos--;

        if (_currentPos == _targetPos) {
            _running = false;
            // timerAlarmDisable intentionally omitted — not ISR-safe.
            // Alarm fires harmlessly (_running==false → early return) until
            // stop() or moveTo() is called from task context.
        }
    }

    portEXIT_CRITICAL_ISR(&_mux);
}

void IRAM_ATTR StepperDriver::_isr0() {
    if (_instances[0]) _instances[0]->_handleISR();
}

void IRAM_ATTR StepperDriver::_isr1() {
    if (_instances[1]) _instances[1]->_handleISR();
}
