// Native Unity tests for ServoDriver and StepperDriver.
// Uses real driver objects wired to mock HAL implementations — no #ifdef stubs,
// no direct .cpp includes. The HAL layer makes this clean.

#include <unity.h>
#include "ServoDriver.h"
#include "StepperDriver.h"

// ── Mock HAL implementations ──────────────────────────────────────────────────

class MockLedc : public ILedc {
public:
    uint8_t  lastChannel = 0;
    uint32_t lastDuty    = 0;
    void setup(uint8_t, uint32_t, uint8_t) override {}
    void attachPin(uint8_t, uint8_t)       override {}
    void detachPin(uint8_t)                override {}
    void write(uint8_t ch, uint32_t duty)  override { lastChannel = ch; lastDuty = duty; }
};

class MockGpio : public IGpio {
public:
    uint8_t lastPin   = 0;
    uint8_t lastValue = 0;
    void setOutput(uint8_t)                   override {}
    void write(uint8_t pin, uint8_t val)      override { lastPin = pin; lastValue = val; }
};

class MockTimer : public ITimer {
public:
    bool     alarmEnabled = false;
    void   (*attachedIsr)() = nullptr;

    void begin(uint32_t)                override {}
    void attachInterrupt(void (*isr)()) override { attachedIsr = isr; }
    void setAlarmPeriod(uint32_t)       override {}
    void enableAlarm()                  override { alarmEnabled = true; }
    void disableAlarm()                 override { alarmEnabled = false; }
    void end()                          override {}
};

// ── Servo tests ───────────────────────────────────────────────────────────────

void test_servo_initial_angle_is_90() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, s.getAngle());
}

void test_servo_set_angle_midrange() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setAngle(45.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.0f, s.getAngle());
}

void test_servo_set_angle_zero() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setAngle(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.getAngle());
}

void test_servo_set_angle_max() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setAngle(180.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 180.0f, s.getAngle());
}

void test_servo_clamps_above_180() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setAngle(270.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 180.0f, s.getAngle());
}

void test_servo_clamps_below_0() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setAngle(-45.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.getAngle());
}

void test_servo_microseconds_passthrough() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setMicroseconds(1500);
    TEST_ASSERT_EQUAL_UINT16(1500, s.getMicroseconds());
}

void test_servo_microseconds_clamp_below_min() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setMicroseconds(100);
    TEST_ASSERT_EQUAL_UINT16(ServoDriver::MIN_PULSE_US, s.getMicroseconds());
}

void test_servo_microseconds_clamp_above_max() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin(); s.setMicroseconds(3000);
    TEST_ASSERT_EQUAL_UINT16(ServoDriver::MAX_PULSE_US, s.getMicroseconds());
}

void test_servo_begin_calls_ledc_write() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin();
    // begin() calls setAngle(90) which calls setMicroseconds(1500) which calls ledc.write
    TEST_ASSERT(ledc.lastDuty > 0);
}

// ── Stepper state tests ───────────────────────────────────────────────────────

void test_stepper_not_running_after_begin() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin();
    TEST_ASSERT_FALSE(s.isRunning());
}

void test_stepper_set_position_positive() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setPosition(500);
    TEST_ASSERT_EQUAL_INT32(500, s.getPosition());
    TEST_ASSERT_FALSE(s.isRunning());
}

void test_stepper_set_position_negative() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setPosition(-250);
    TEST_ASSERT_EQUAL_INT32(-250, s.getPosition());
}

void test_stepper_stop_when_idle_is_safe() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.stop();
    TEST_ASSERT_FALSE(s.isRunning());
}

void test_stepper_move_zero_stays_idle() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setPosition(0); s.move(0);
    TEST_ASSERT_FALSE(s.isRunning());
}

void test_stepper_move_enables_timer_alarm() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000); s.move(200);
    TEST_ASSERT_TRUE(timer.alarmEnabled);
    TEST_ASSERT_TRUE(s.isRunning());
}

void test_stepper_stop_disables_timer_alarm() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000); s.move(200); s.stop();
    TEST_ASSERT_FALSE(timer.alarmEnabled);
    TEST_ASSERT_FALSE(s.isRunning());
}

void test_stepper_enable_pulls_pin_low() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, /*enablePin=*/21);
    s.begin(); s.enable();
    TEST_ASSERT_EQUAL_UINT8(21, gpio.lastPin);
    TEST_ASSERT_EQUAL_UINT8(0, gpio.lastValue); // active-low
}

void test_stepper_disable_pulls_pin_high() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, /*enablePin=*/21);
    s.begin(); s.enable(); s.disable();
    TEST_ASSERT_EQUAL_UINT8(21, gpio.lastPin);
    TEST_ASSERT_EQUAL_UINT8(1, gpio.lastValue);
}

// ── Stepper ISR simulation tests ──────────────────────────────────────────────
// The MockTimer captures the ISR function pointer from attachInterrupt().
// Calling it manually simulates timer ticks, exercising the full ISR path
// on native without any hardware.

void test_stepper_isr_advances_position_forward() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000);
    s.move(5); // target = +5; needs 10 ISR fires (5 steps × 2 phases)

    for (int i = 0; i < 10; i++) timer.attachedIsr();

    TEST_ASSERT_EQUAL_INT32(5, s.getPosition());
    TEST_ASSERT_FALSE(s.isRunning());
}

void test_stepper_isr_advances_position_backward() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000);
    s.move(-3);

    for (int i = 0; i < 6; i++) timer.attachedIsr();

    TEST_ASSERT_EQUAL_INT32(-3, s.getPosition());
    TEST_ASSERT_FALSE(s.isRunning());
}

void test_stepper_isr_partial_move_still_running() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000);
    s.move(10);

    for (int i = 0; i < 8; i++) timer.attachedIsr(); // only 4 of 10 steps

    TEST_ASSERT_TRUE(s.isRunning());
    TEST_ASSERT_EQUAL_INT32(4, s.getPosition());
}

void test_stepper_isr_extra_ticks_are_noop_after_stop() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000);
    s.move(2);

    for (int i = 0; i < 4; i++) timer.attachedIsr(); // completes move
    TEST_ASSERT_FALSE(s.isRunning());

    int32_t posAfterComplete = s.getPosition();
    timer.attachedIsr(); // extra tick — must be a no-op
    timer.attachedIsr();
    TEST_ASSERT_EQUAL_INT32(posAfterComplete, s.getPosition());
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_servo_initial_angle_is_90);
    RUN_TEST(test_servo_set_angle_midrange);
    RUN_TEST(test_servo_set_angle_zero);
    RUN_TEST(test_servo_set_angle_max);
    RUN_TEST(test_servo_clamps_above_180);
    RUN_TEST(test_servo_clamps_below_0);
    RUN_TEST(test_servo_microseconds_passthrough);
    RUN_TEST(test_servo_microseconds_clamp_below_min);
    RUN_TEST(test_servo_microseconds_clamp_above_max);
    RUN_TEST(test_servo_begin_calls_ledc_write);

    RUN_TEST(test_stepper_not_running_after_begin);
    RUN_TEST(test_stepper_set_position_positive);
    RUN_TEST(test_stepper_set_position_negative);
    RUN_TEST(test_stepper_stop_when_idle_is_safe);
    RUN_TEST(test_stepper_move_zero_stays_idle);
    RUN_TEST(test_stepper_move_enables_timer_alarm);
    RUN_TEST(test_stepper_stop_disables_timer_alarm);
    RUN_TEST(test_stepper_enable_pulls_pin_low);
    RUN_TEST(test_stepper_disable_pulls_pin_high);

    RUN_TEST(test_stepper_isr_advances_position_forward);
    RUN_TEST(test_stepper_isr_advances_position_backward);
    RUN_TEST(test_stepper_isr_partial_move_still_running);
    RUN_TEST(test_stepper_isr_extra_ticks_are_noop_after_stop);

    return UNITY_END();
}
