#include <unity.h>
#include <Arduino.h>
#include "MockGpio.h"
#include "MockTimer.h"
#include "StepperDriver.h"

static MockGpio gpio;
static MockTimer timer;
// stepPin=18, dirPin=19, isrSlot=0, enablePin=21
static StepperDriver stepper(18, 19, gpio, timer, /*isrSlot=*/0, /*enablePin=*/21);

void setUp() {}
void tearDown() {}

void test_not_running_after_begin() {
    TEST_ASSERT_FALSE(stepper.isRunning());
}

void test_set_position_positive() {
    stepper.setPosition(500);
    TEST_ASSERT_EQUAL_INT32(500, stepper.getPosition());
}

void test_set_position_zero() {
    stepper.setPosition(0);
    TEST_ASSERT_EQUAL_INT32(0, stepper.getPosition());
}

void test_set_position_negative() {
    stepper.setPosition(-250);
    TEST_ASSERT_EQUAL_INT32(-250, stepper.getPosition());
}

void test_set_position_stops_motion() {
    stepper.setPosition(0);
    stepper.setSpeed(500);
    stepper.move(1000);
    stepper.setPosition(0);
    TEST_ASSERT_FALSE(stepper.isRunning());
    TEST_ASSERT_EQUAL_INT32(0, stepper.getPosition());
}

void test_stop_when_idle_is_safe() {
    stepper.setPosition(0);
    stepper.stop();
    TEST_ASSERT_FALSE(stepper.isRunning());
}

void test_move_zero_steps_stays_idle() {
    stepper.setPosition(0);
    stepper.move(0);
    TEST_ASSERT_FALSE(stepper.isRunning());
}

void test_move_nonzero_starts_running() {
    stepper.setPosition(0);
    stepper.setSpeed(1000);
    stepper.move(200);
    TEST_ASSERT_TRUE(stepper.isRunning());
    stepper.stop();
}

static void runTests() {
    UNITY_BEGIN();
    RUN_TEST(test_not_running_after_begin);
    RUN_TEST(test_set_position_positive);
    RUN_TEST(test_set_position_zero);
    RUN_TEST(test_set_position_negative);
    RUN_TEST(test_set_position_stops_motion);
    RUN_TEST(test_stop_when_idle_is_safe);
    RUN_TEST(test_move_zero_steps_stays_idle);
    RUN_TEST(test_move_nonzero_starts_running);
    UNITY_END();
}

#ifdef ARDUINO
void setup() { delay(2000); stepper.begin(); stepper.enable(); runTests(); }
void loop() {}
#else
int main(int, char**) { stepper.begin(); stepper.enable(); runTests(); return 0; }
#endif
