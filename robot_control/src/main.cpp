#include <Arduino.h>
#include "Esp32Gpio.h"
#include "Esp32Timer.h"
#include "Esp32Ledc.h"
#include "Esp32Uart.h"
#include "StepperDriver.h"
#include "StepperUartConfig.h"
#include "ServoDriver.h"

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

// ── Driver instances ───────────────────────────────────────────────────────────
StepperDriver stepper(STEP_PIN, DIR_PIN, gpio, timer0, /*isrSlot=*/0, EN_PIN);
ServoDriver   servo(SERVO_PIN, LEDC_CH, ledc);

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

    if (Serial.available()) {
        char cmd = Serial.read();
        int  val = Serial.parseInt();

        if      (cmd == 's') { Serial.printf("Moving %d steps\n", val); stepper.move(val); }
        else if (cmd == 'v') { Serial.printf("Servo → %.1f°\n", (float)val); servo.setAngle((float)val); }
        else if (cmd == 'x') { stepper.stop(); Serial.println("Stepper stopped."); }
    }

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
