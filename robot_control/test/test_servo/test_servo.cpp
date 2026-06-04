#include <unity.h>
#include <Arduino.h>
#include "MockLedc.h"
#include "ServoDriver.h"

static MockLedc ledc;
static ServoDriver servo(13, 0, ledc);

void setUp() {}
void tearDown() {}

void test_initial_angle_after_begin() {
    servo.begin();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 90.0f, servo.getAngle());
}

void test_set_angle_midrange() {
    servo.setAngle(45.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 45.0f, servo.getAngle());
}

void test_set_angle_zero() {
    servo.setAngle(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, servo.getAngle());
}

void test_set_angle_max() {
    servo.setAngle(180.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 180.0f, servo.getAngle());
}

void test_set_angle_clamps_above_max() {
    servo.setAngle(270.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 180.0f, servo.getAngle());
}

void test_set_angle_clamps_below_min() {
    servo.setAngle(-30.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, servo.getAngle());
}

void test_set_microseconds_midrange() {
    servo.setMicroseconds(1500);
    TEST_ASSERT_EQUAL_UINT16(1500, servo.getMicroseconds());
}

void test_set_microseconds_clamps_below_min() {
    servo.setMicroseconds(100);
    TEST_ASSERT_EQUAL_UINT16(ServoDriver::MIN_PULSE_US, servo.getMicroseconds());
}

void test_set_microseconds_clamps_above_max() {
    servo.setMicroseconds(3000);
    TEST_ASSERT_EQUAL_UINT16(ServoDriver::MAX_PULSE_US, servo.getMicroseconds());
}

static void runTests() {
    UNITY_BEGIN();
    RUN_TEST(test_initial_angle_after_begin);
    RUN_TEST(test_set_angle_midrange);
    RUN_TEST(test_set_angle_zero);
    RUN_TEST(test_set_angle_max);
    RUN_TEST(test_set_angle_clamps_above_max);
    RUN_TEST(test_set_angle_clamps_below_min);
    RUN_TEST(test_set_microseconds_midrange);
    RUN_TEST(test_set_microseconds_clamps_below_min);
    RUN_TEST(test_set_microseconds_clamps_above_max);
    UNITY_END();
}

#ifdef ARDUINO
void setup() { delay(2000); servo.begin(); runTests(); }
void loop() {}
#else
int main(int, char**) { servo.begin(); runTests(); return 0; }
#endif
