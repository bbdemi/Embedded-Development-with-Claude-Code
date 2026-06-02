# Robot Embedded Firmware

## Hardware
- **MCU**: ESP32 (Arduino framework, not ESP-IDF)
- **Language**: C++
- **Stepper driver**: Step/dir interface — A4988 / DRV8825 / TMC2209
- **Servo**: Standard PWM servo via ESP32 LEDC peripheral

## Project structure
```
robot_control/
├── robot_control.ino   # Entry point, pin assignments, demo
├── StepperDriver.h/.cpp  # Non-blocking stepper via hardware timer ISR
├── ServoDriver.h/.cpp    # Servo via ESP32 LEDC (50 Hz, 16-bit)
```

## Key conventions
- Stepper uses ESP32 hardware timers 0 and 1 (supports 2 simultaneous steppers)
- Two-phase ISR: fires at 2× step rate for clean pulse generation
- Servo LEDC channels: each servo needs a unique channel (0–15)
- Enable pin is active-low (standard for most step/dir drivers)
- All motion is non-blocking — poll `isRunning()` instead of `delay()`

## Pin assignments (robot_control.ino)
| Signal    | GPIO |
|-----------|------|
| STEP      | 18   |
| DIR       | 19   |
| ENABLE    | 21   |
| SERVO PWM | 13   |
