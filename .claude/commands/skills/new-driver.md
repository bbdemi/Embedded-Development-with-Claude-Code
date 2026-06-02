# New Driver Scaffold

Scaffold a new ESP32 Arduino C++ driver for: $ARGUMENTS

## Steps

1. Parse the component name from $ARGUMENTS (e.g. "MPU6050", "HC-SR04", "AS5600")
2. Identify the likely communication bus from the component name if known, otherwise ask
3. Create `robot_control/<ComponentName>Driver.h` and `robot_control/<ComponentName>Driver.cpp`

## Code rules

- Class: `<ComponentName>Driver`
- `bool begin()` — initializes bus, verifies device is present, returns false if not found
- Public methods return data or bool status — no Serial.print inside driver
- Fixed-width types only: `uint8_t`, `int16_t`, `float`
- All register addresses as `static constexpr uint8_t` in the header
- Non-blocking reads using `millis()` not `delay()`
- Match the style of `StepperDriver` and `ServoDriver` already in this project

## After creating files, show

- Public API (method signatures only, one line each)
- Which pins or bus address it uses
- A 5-line usage snippet for `robot_control.ino`
