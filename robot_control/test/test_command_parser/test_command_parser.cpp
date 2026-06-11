// Native Unity tests for CommandParser.
// A MockUartSeq feeds a pre-loaded byte string; real StepperDriver/ServoDriver
// instances (backed by mock HALs) let us observe the effects.

#include <unity.h>
#include <string>
#include "CommandParser.h"
#include "StepperDriver.h"
#include "ServoDriver.h"
#include "MockGpio.h"
#include "MockLedc.h"
#include "MockTimer.h"

// ── MockUartSeq ───────────────────────────────────────────────────────────────
// Wraps a std::string and vends one byte at a time via the IUart interface.

class MockUartSeq : public IUart {
    std::string _data;
    size_t      _pos = 0;
public:
    explicit MockUartSeq(const std::string& data) : _data(data) {}
    int  available()                            override { return (_pos < _data.size()) ? 1 : 0; }
    int  read()                                 override { return (_pos < _data.size()) ? (uint8_t)_data[_pos++] : -1; }
    void begin(uint32_t)                        override {}
    void end()                                  override {}
    void write(uint8_t)                         override {}
    void write(const uint8_t*, size_t)          override {}
};

// ── Test fixture helpers ──────────────────────────────────────────────────────

struct Fixture {
    MockGpio   gpio;
    MockTimer  timer;
    MockLedc   ledc;
    StepperDriver stepper{18, 19, gpio, timer, 0, 21};
    ServoDriver   servo  {13, 0,  ledc};

    void init() {
        stepper.begin();
        stepper.setSpeed(1000);
        servo.begin();
    }
};

// ── Servo tests ───────────────────────────────────────────────────────────────

void test_parser_servo_angle_midrange() {
    Fixture f; f.init();
    MockUartSeq uart("v90\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, f.servo.getAngle());
}

void test_parser_servo_angle_zero() {
    Fixture f; f.init();
    MockUartSeq uart("v0\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, f.servo.getAngle());
}

void test_parser_servo_angle_max() {
    Fixture f; f.init();
    MockUartSeq uart("v180\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 180.0f, f.servo.getAngle());
}

// Two servo commands: second one must win.
void test_parser_servo_sequential_last_wins() {
    Fixture f; f.init();
    MockUartSeq uart("v45\nv120\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f, f.servo.getAngle());
}

// ── Stepper tests ─────────────────────────────────────────────────────────────

void test_parser_stepper_move_positive_starts_motion() {
    Fixture f; f.init();
    MockUartSeq uart("s200\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_TRUE(f.stepper.isRunning());
}

void test_parser_stepper_move_negative_starts_motion() {
    Fixture f; f.init();
    MockUartSeq uart("s-50\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_TRUE(f.stepper.isRunning());
}

void test_parser_stepper_move_zero_stays_idle() {
    Fixture f; f.init();
    MockUartSeq uart("s0\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FALSE(f.stepper.isRunning());
}

void test_parser_stop_halts_running_stepper() {
    Fixture f; f.init();
    MockUartSeq uart("s200\nx\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FALSE(f.stepper.isRunning());
}

// ── Edge-case tests ───────────────────────────────────────────────────────────

// Unknown command chars must be silently ignored — no crash, no state change.
void test_parser_unknown_command_ignored() {
    Fixture f; f.init();
    MockUartSeq uart("z99\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FALSE(f.stepper.isRunning());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, f.servo.getAngle()); // begin() sets 90
}

// Partial input (no trailing newline) must not execute — waits for newline.
void test_parser_partial_input_waits_for_newline() {
    Fixture f; f.init();
    MockUartSeq uart("v45"); // no '\n'
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, f.servo.getAngle());
}

// A new command char flushes the pending one even without a newline.
void test_parser_new_command_flushes_pending() {
    Fixture f; f.init();
    MockUartSeq uart("v45v120\n");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f, f.servo.getAngle());
}

// poll() called on an empty stream must not crash.
void test_parser_empty_stream_is_noop() {
    Fixture f; f.init();
    MockUartSeq uart("");
    CommandParser parser(uart, f.stepper, f.servo);
    parser.poll();
    TEST_ASSERT_FALSE(f.stepper.isRunning());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, f.servo.getAngle());
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_parser_servo_angle_midrange);
    RUN_TEST(test_parser_servo_angle_zero);
    RUN_TEST(test_parser_servo_angle_max);
    RUN_TEST(test_parser_servo_sequential_last_wins);

    RUN_TEST(test_parser_stepper_move_positive_starts_motion);
    RUN_TEST(test_parser_stepper_move_negative_starts_motion);
    RUN_TEST(test_parser_stepper_move_zero_stays_idle);
    RUN_TEST(test_parser_stop_halts_running_stepper);

    RUN_TEST(test_parser_unknown_command_ignored);
    RUN_TEST(test_parser_partial_input_waits_for_newline);
    RUN_TEST(test_parser_new_command_flushes_pending);
    RUN_TEST(test_parser_empty_stream_is_noop);

    return UNITY_END();
}
