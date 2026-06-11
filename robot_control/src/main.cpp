#include <Arduino.h>
#include "Esp32Gpio.h"
#include "Esp32Timer.h"
#include "Esp32Ledc.h"
#include "Esp32Uart.h"
#include "StepperDriver.h"
#include "StepperUartConfig.h"
#include "ServoDriver.h"
#include "CommandParser.h"

// ── Pin assignments ────────────────────────────────────────────────────────────
static constexpr uint8_t STEP_PIN  = 27;
static constexpr uint8_t DIR_PIN   = 32;
static constexpr uint8_t EN_PIN    = 26;
static constexpr uint8_t SERVO_PIN = 13;
static constexpr uint8_t LEDC_CH   = 0;

// ── HAL instances (ESP32 concrete implementations) ────────────────────────────
static Esp32Gpio  gpio;
static Esp32Timer timer0(0);
static Esp32Ledc  ledc;

// TODO: set uartNum, TX pin, RX pin and baud rate to match your custom board.
static Esp32Uart        stepperUart(/*uartNum=*/1, /*TX=*/17, /*RX=*/16);
static StepperUartConfig stepperConfig(stepperUart);

// UART0 wraps the USB Serial port — used by CommandParser to receive commands.
static Esp32Uart cmdSerial(/*uartNum=*/0);

// ── Driver instances ───────────────────────────────────────────────────────────
StepperDriver stepper(STEP_PIN, DIR_PIN, gpio, timer0, /*isrSlot=*/0, EN_PIN);
ServoDriver   servo(SERVO_PIN, LEDC_CH, ledc);
CommandParser cmdParser(cmdSerial, stepper, servo);

void runDemo();

// ── Setup ──────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    stepperConfig.begin(115200); // TODO: match your driver's baud rate
    stepperConfig.setMicrosteps(16);

    stepper.begin();
    stepper.enable();
    stepper.setSpeed(1000);

    servo.begin();

    Serial.println("Robot control ready.");
    runDemo();
}

// ── Loop ───────────────────────────────────────────────────────────────────────
void loop() {
    if (!stepper.isRunning()) {
        Serial.printf("Stepper stopped at position: %ld\n", stepper.getPosition());
    }

    cmdParser.poll();

    delay(100);
}

// ── Demo sequence ──────────────────────────────────────────────────────────────
void runDemo() {
    for (int angle = 0; angle <= 180; angle += 10) { servo.setAngle((float)angle); delay(50); }
    servo.setAngle(90.0f);

    stepper.setSpeed(800);
    stepper.move(200);
    while (stepper.isRunning()) delay(10);

    delay(500);
    stepper.move(-200);
    while (stepper.isRunning()) delay(10);

    Serial.println("Demo complete.");
}
