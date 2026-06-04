// Native Unity tests for ServoDriver, StepperDriver, and StepperUartConfig.
// Uses real driver objects wired to mock HAL implementations — no #ifdef stubs,
// no direct .cpp includes. The HAL layer makes this clean.

#include <unity.h>
#include <vector>
#include <string>
#include "ServoDriver.h"
#include "StepperDriver.h"
#include "StepperUartConfig.h"

// ── Mock HAL + UART implementations ──────────────────────────────────────────

class MockUart : public IUart {
public:
    bool                 beginCalled = false;
    uint32_t             baudRate    = 0;
    std::vector<uint8_t> written;

    void begin(uint32_t baud) override { beginCalled = true; baudRate = baud; }
    void end()                override {}
    void write(uint8_t byte)  override { written.push_back(byte); }
    void write(const uint8_t* buf, size_t len) override {
        written.insert(written.end(), buf, buf + len);
    }
    int  read()      override { return -1; }
    int  available() override { return 0; }

    std::string writtenAsString() const {
        return std::string(written.begin(), written.end());
    }
};

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

// ── Reference CRC helper (mirrors StepperUartConfig::calcCRC exactly) ─────────
// Duplicated here so tests can compute expected CRC values without needing to
// friend or expose the private static method.
static uint8_t ref_crc(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if ((crc >> 7) ^ (b & 0x01))
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
            b >>= 1;
        }
    }
    return crc;
}

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

// Duty for 0 degrees must map to MIN_PULSE_US (500 us) duty count.
// _usToDuty(500) = 500 * 65536 / 20000 = 1638.
void test_servo_duty_at_0_degrees() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin();
    s.setAngle(0.0f);
    TEST_ASSERT_EQUAL_UINT32(1638u, ledc.lastDuty);
}

// Duty for 180 degrees must map to MAX_PULSE_US (2500 us) duty count.
// _usToDuty(2500) = 2500 * 65536 / 20000 = 8192.
void test_servo_duty_at_180_degrees() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin();
    s.setAngle(180.0f);
    TEST_ASSERT_EQUAL_UINT32(8192u, ledc.lastDuty);
}

// After multiple setAngle calls, getAngle must reflect only the last one.
void test_servo_angle_reflects_last_set() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin();
    s.setAngle(30.0f);
    s.setAngle(120.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f, s.getAngle());
}

// setMicroseconds at exactly MIN boundary must not be clamped.
void test_servo_microseconds_at_min_boundary() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin();
    s.setMicroseconds(ServoDriver::MIN_PULSE_US);
    TEST_ASSERT_EQUAL_UINT16(ServoDriver::MIN_PULSE_US, s.getMicroseconds());
}

// setMicroseconds at exactly MAX boundary must not be clamped.
void test_servo_microseconds_at_max_boundary() {
    MockLedc ledc; ServoDriver s(13, 0, ledc);
    s.begin();
    s.setMicroseconds(ServoDriver::MAX_PULSE_US);
    TEST_ASSERT_EQUAL_UINT16(ServoDriver::MAX_PULSE_US, s.getMicroseconds());
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

// moveTo the current position must be a no-op (no alarm, not running).
void test_stepper_moveto_same_position_is_noop() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin();
    s.setPosition(42);
    s.moveTo(42);
    TEST_ASSERT_FALSE(s.isRunning());
    TEST_ASSERT_FALSE(timer.alarmEnabled);
}

// setPosition while running must halt motion and set the new origin.
void test_stepper_set_position_cancels_running_move() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000); s.move(1000);
    TEST_ASSERT_TRUE(s.isRunning());
    s.setPosition(0);
    TEST_ASSERT_FALSE(s.isRunning());
    TEST_ASSERT_EQUAL_INT32(0, s.getPosition());
}

// stop() mid-move must freeze position at wherever the ISR left it.
void test_stepper_stop_mid_move_freezes_position() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000);
    s.move(10);

    // Fire 6 ticks → 3 complete steps (positions 1, 2, 3).
    for (int i = 0; i < 6; i++) timer.attachedIsr();
    TEST_ASSERT_EQUAL_INT32(3, s.getPosition());

    s.stop();
    TEST_ASSERT_FALSE(s.isRunning());
    TEST_ASSERT_EQUAL_INT32(3, s.getPosition()); // position must not drift after stop
}

// Round-trip: move forward N then backward N must return to zero.
void test_stepper_round_trip_returns_to_zero() {
    MockGpio gpio; MockTimer timer;
    StepperDriver s(18, 19, gpio, timer, 0, 21);
    s.begin(); s.setSpeed(1000);

    s.move(5);
    for (int i = 0; i < 10; i++) timer.attachedIsr();
    TEST_ASSERT_EQUAL_INT32(5, s.getPosition());

    s.move(-5);
    for (int i = 0; i < 10; i++) timer.attachedIsr();
    TEST_ASSERT_EQUAL_INT32(0, s.getPosition());
    TEST_ASSERT_FALSE(s.isRunning());
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

// ── StepperUartConfig protocol tests ─────────────────────────────────────────

void test_uart_config_begin_opens_uart() {
    MockUart uart;
    StepperUartConfig config(uart);
    config.begin(115200);
    TEST_ASSERT_TRUE(uart.beginCalled);
    TEST_ASSERT_EQUAL_UINT32(115200, uart.baudRate);
}

void test_uart_config_send_text_writes_bytes() {
    MockUart uart;
    StepperUartConfig config(uart);
    config.begin(9600);
    config.sendText("HELLO");
    TEST_ASSERT_EQUAL_STRING("HELLO", uart.writtenAsString().c_str());
}

void test_uart_config_send_bytes_writes_exact_packet() {
    MockUart uart;
    StepperUartConfig config(uart);
    config.begin(9600);
    uint8_t pkt[] = {0xAB, 0x10, 0x00};
    config.sendBytes(pkt, 3);
    TEST_ASSERT_EQUAL_UINT8(0xAB, uart.written[0]);
    TEST_ASSERT_EQUAL_UINT8(0x10, uart.written[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, uart.written[2]);
    TEST_ASSERT_EQUAL_size_t(3, uart.written.size());
}

// ── calcCRC gold-standard test ────────────────────────────────────────────────
// Verified by hand: CRC-8/SMBus (poly 0x07, LSB-first per byte) over
// {0x05, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00} = 0x50.
// This is the datagram for writeRegister(REG_IHOLD_IRUN=0x10, 0) on slave 0.
// Exercised indirectly: call setCurrent(0, 0) and inspect uart.written[7].
void test_crc_known_zero_data_datagram() {
    MockUart uart;
    // rsense=110 mOhm → vsense=false → vref=325; currentToCS(0, ...)=0.
    // CHOPCONF is written first (8 bytes), then IHOLD_IRUN (8 bytes).
    // IHOLD_IRUN packet when runMA=0, holdMA=0, holdDelay=0:
    //   value = (0<<16)|(0<<8)|0 = 0x00000000
    //   raw pkt[0..6] = {0x05, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00}
    //   expected CRC = 0x50
    StepperUartConfig config(uart, /*slave=*/0, /*rsense=*/110);
    config.begin(115200);
    config.setCurrent(/*runMA=*/0, /*holdMA=*/0, /*holdDelay=*/0);

    // Written bytes: 8 (CHOPCONF) + 8 (IHOLD_IRUN) = 16 total.
    TEST_ASSERT_EQUAL_size_t(16, uart.written.size());

    // Verify IHOLD_IRUN packet structure (bytes 8..15).
    const uint8_t* ihold_pkt = uart.written.data() + 8;
    TEST_ASSERT_EQUAL_UINT8(0x05, ihold_pkt[0]); // sync byte
    TEST_ASSERT_EQUAL_UINT8(0x00, ihold_pkt[1]); // slave address
    TEST_ASSERT_EQUAL_UINT8(0x90, ihold_pkt[2]); // REG_IHOLD_IRUN (0x10) | 0x80
    TEST_ASSERT_EQUAL_UINT8(0x00, ihold_pkt[3]); // value MSB
    TEST_ASSERT_EQUAL_UINT8(0x00, ihold_pkt[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, ihold_pkt[5]);
    TEST_ASSERT_EQUAL_UINT8(0x00, ihold_pkt[6]); // value LSB
    TEST_ASSERT_EQUAL_UINT8(0x50, ihold_pkt[7]); // pre-computed CRC
}

// ── writeRegister packet structure ────────────────────────────────────────────
// Any register write must produce exactly 8 bytes with the correct framing:
// [0]=0x05, [1]=slave, [2]=reg|0x80, [3..6]=value MSB-first, [7]=CRC(pkt[0..6]).
void test_write_register_packet_structure() {
    MockUart uart;
    StepperUartConfig config(uart, /*slave=*/0, /*rsense=*/110);
    config.begin(115200);
    // setMicrosteps triggers exactly one writeRegister call (CHOPCONF).
    config.setMicrosteps(256); // MRES=0, no change to MRES bits, value=CHOPCONF_DEFAULT=0x10000053
    TEST_ASSERT_EQUAL_size_t(8, uart.written.size());

    TEST_ASSERT_EQUAL_UINT8(0x05, uart.written[0]);               // sync
    TEST_ASSERT_EQUAL_UINT8(0x00, uart.written[1]);               // slave addr
    TEST_ASSERT_EQUAL_UINT8(0xEC, uart.written[2]);               // 0x6C | 0x80
    // Data bytes MSB-first: 0x10000053
    TEST_ASSERT_EQUAL_UINT8(0x10, uart.written[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, uart.written[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, uart.written[5]);
    TEST_ASSERT_EQUAL_UINT8(0x53, uart.written[6]);
    // CRC must match reference computation over bytes 0..6
    uint8_t expected_crc = ref_crc(uart.written.data(), 7);
    TEST_ASSERT_EQUAL_UINT8(expected_crc, uart.written[7]);
}

// Non-default slave address must appear in byte [1] of every packet.
void test_write_register_uses_correct_slave_address() {
    MockUart uart;
    StepperUartConfig config(uart, /*slave=*/3, /*rsense=*/110);
    config.begin(115200);
    config.setMicrosteps(16);
    TEST_ASSERT_EQUAL_UINT8(0x03, uart.written[1]);
}

// ── setMicrosteps MRES encoding ───────────────────────────────────────────────
// Each valid microstep value maps to a 4-bit MRES code in CHOPCONF bits 27:24.
// The encoded value can be read back from the written CHOPCONF packet byte [3].

// Helper: extract MRES from the written CHOPCONF packet.
// byte[3] is bits 31:24 of the 32-bit value; MRES occupies bits 27:24 = lower nibble.
static uint8_t mres_from_packet(const std::vector<uint8_t>& pkt) {
    // pkt[3] = value bits 31:24; MRES is bits 27:24 = lower nibble of pkt[3]
    return pkt[3] & 0x0F;
}

void test_microsteps_256_encodes_mres_0() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(256);
    TEST_ASSERT_EQUAL_UINT8(0, mres_from_packet(uart.written));
}

void test_microsteps_128_encodes_mres_1() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(128);
    TEST_ASSERT_EQUAL_UINT8(1, mres_from_packet(uart.written));
}

void test_microsteps_64_encodes_mres_2() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(64);
    TEST_ASSERT_EQUAL_UINT8(2, mres_from_packet(uart.written));
}

void test_microsteps_32_encodes_mres_3() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(32);
    TEST_ASSERT_EQUAL_UINT8(3, mres_from_packet(uart.written));
}

void test_microsteps_16_encodes_mres_4() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(16);
    TEST_ASSERT_EQUAL_UINT8(4, mres_from_packet(uart.written));
}

void test_microsteps_8_encodes_mres_5() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(8);
    TEST_ASSERT_EQUAL_UINT8(5, mres_from_packet(uart.written));
}

void test_microsteps_4_encodes_mres_6() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(4);
    TEST_ASSERT_EQUAL_UINT8(6, mres_from_packet(uart.written));
}

void test_microsteps_2_encodes_mres_7() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(2);
    TEST_ASSERT_EQUAL_UINT8(7, mres_from_packet(uart.written));
}

void test_microsteps_1_encodes_mres_8_full_step() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(1);
    TEST_ASSERT_EQUAL_UINT8(8, mres_from_packet(uart.written));
}

// Invalid value (not a power-of-two between 1 and 256) also falls through to
// the default: case which returns MRES=8 (full step). Must not crash.
void test_microsteps_invalid_value_encodes_mres_8() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200); config.setMicrosteps(7); // not a valid microstep count
    TEST_ASSERT_EQUAL_UINT8(8, mres_from_packet(uart.written));
    TEST_ASSERT_EQUAL_size_t(8, uart.written.size()); // still sends one packet
}

// ── currentToCS boundary tests ────────────────────────────────────────────────
// Exercised through setCurrent: inspect the IHOLD_IRUN packet (bytes 8..15).
// IHOLD_IRUN value layout: bits 19:16=holdDelay, bits 12:8=IRUN, bits 4:0=IHOLD.

// Helper: parse IRUN (bits 12:8) from written IHOLD_IRUN packet bytes.
// bytes[8..15] is the IHOLD_IRUN packet; value bytes are [11..14].
static uint8_t irun_from_written(const std::vector<uint8_t>& w) {
    // value bytes: w[11]=bits31:24, w[12]=bits23:16, w[13]=bits15:8, w[14]=bits7:0
    // IRUN occupies bits 12:8 = lower 5 bits of w[13]
    return w[13] & 0x1F;
}

static uint8_t ihold_from_written(const std::vector<uint8_t>& w) {
    // IHOLD occupies bits 4:0 of the 32-bit value = lower 5 bits of w[14]
    return w[14] & 0x1F;
}

static uint8_t holddelay_from_written(const std::vector<uint8_t>& w) {
    // holdDelay occupies bits 19:16 = lower 4 bits of w[12]
    return w[12] & 0x0F;
}

// 0 mA run current → IRUN=0.
void test_current_zero_run_ma_gives_irun_0() {
    MockUart uart; StepperUartConfig config(uart, 0, /*rsense=*/110);
    config.begin(115200);
    config.setCurrent(/*runMA=*/0, /*holdMA=*/0, /*holdDelay=*/0);
    TEST_ASSERT_EQUAL_UINT8(0, irun_from_written(uart.written));
}

// 0 mA hold current (holdMA=0 branch) → IHOLD=0.
void test_current_zero_hold_ma_gives_ihold_0() {
    MockUart uart; StepperUartConfig config(uart, 0, /*rsense=*/110);
    config.begin(115200);
    config.setCurrent(800, /*holdMA=*/0, /*holdDelay=*/0);
    TEST_ASSERT_EQUAL_UINT8(0, ihold_from_written(uart.written));
}

// 800 mA run, rsense=110 mOhm, vref=325 mV (vsense=false):
// CS = floor(800 * 130 * 32 * 1414 / (325 * 1000000)) - 1
//    = floor(4706192000 / 325000000) - 1 = 14 - 1 = 13.
void test_current_800ma_110mohm_gives_irun_13() {
    MockUart uart; StepperUartConfig config(uart, 0, /*rsense=*/110);
    config.begin(115200);
    config.setCurrent(800, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(13, irun_from_written(uart.written));
}

// Very high current saturates CS at 31 (5-bit max).
// 2000 mA → csPlus1 = floor(2000*130*32*1414/325000000) = floor(36.2) = 36
// cs = 36 - 1 = 35 → capped to 31.
void test_current_2000ma_saturates_to_cs_31() {
    MockUart uart; StepperUartConfig config(uart, 0, /*rsense=*/110);
    config.begin(115200);
    config.setCurrent(2000, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(31, irun_from_written(uart.written));
}

// holdDelay field must be packed into bits 19:16 of IHOLD_IRUN.
void test_current_hold_delay_packed_correctly() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200);
    config.setCurrent(800, 0, /*holdDelay=*/6);
    TEST_ASSERT_EQUAL_UINT8(6, holddelay_from_written(uart.written));
}

// holdDelay is masked to 4 bits — values above 15 are truncated.
void test_current_hold_delay_masked_to_4_bits() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200);
    config.setCurrent(800, 0, /*holdDelay=*/0xFF);
    TEST_ASSERT_EQUAL_UINT8(0x0F, holddelay_from_written(uart.written));
}

// runMA > holdMA: both values must be independently encoded.
void test_current_run_greater_than_hold_both_encoded() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200);
    // runMA=800 → IRUN=13; holdMA=200 mA: CS=floor(200*130*32*1414/325000000)-1
    // = floor(1176548000/325000000)-1 = floor(3.62)-1 = 3-1 = 2
    config.setCurrent(800, /*holdMA=*/200, /*holdDelay=*/0);
    TEST_ASSERT_EQUAL_UINT8(13, irun_from_written(uart.written));
    TEST_ASSERT_EQUAL_UINT8(2,  ihold_from_written(uart.written));
}

// ── setCurrent VSENSE / CHOPCONF interaction ──────────────────────────────────
// rsense >= 100 mOhm → VSENSE bit (bit 17 of CHOPCONF) must be 0.
// rsense <  100 mOhm → VSENSE bit must be 1.

// Helper: extract VSENSE bit from the first (CHOPCONF) packet.
static bool vsense_from_written(const std::vector<uint8_t>& w) {
    // CHOPCONF packet: w[0..7]; value is w[3..6] MSB-first.
    // VSENSE = bit 17 of the 32-bit value.
    // bit17 lives in the second-most-significant byte (w[4]), bit 1.
    return (w[4] & 0x02) != 0;
}

void test_current_rsense_110_clears_vsense_bit() {
    MockUart uart; StepperUartConfig config(uart, 0, /*rsense=*/110);
    config.begin(115200);
    config.setCurrent(800, 0, 0);
    TEST_ASSERT_FALSE(vsense_from_written(uart.written));
}

void test_current_rsense_75_sets_vsense_bit() {
    MockUart uart; StepperUartConfig config(uart, 0, /*rsense=*/75);
    config.begin(115200);
    config.setCurrent(800, 0, 0);
    TEST_ASSERT_TRUE(vsense_from_written(uart.written));
}

// rsense exactly 100 mOhm is not < 100, so VSENSE must be cleared.
void test_current_rsense_100_boundary_clears_vsense() {
    MockUart uart; StepperUartConfig config(uart, 0, /*rsense=*/100);
    config.begin(115200);
    config.setCurrent(800, 0, 0);
    TEST_ASSERT_FALSE(vsense_from_written(uart.written));
}

// setCurrent always writes CHOPCONF first, then IHOLD_IRUN → 16 bytes total.
void test_current_writes_two_packets_total_16_bytes() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200);
    config.setCurrent(800, 0, 6);
    TEST_ASSERT_EQUAL_size_t(16, uart.written.size());
}

// Both written packets must have internally consistent CRCs.
void test_current_both_packets_have_valid_crcs() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200);
    config.setCurrent(800, 0, 6);
    TEST_ASSERT_EQUAL_size_t(16, uart.written.size());

    uint8_t crc0 = ref_crc(uart.written.data(),     7);
    uint8_t crc1 = ref_crc(uart.written.data() + 8, 7);
    TEST_ASSERT_EQUAL_UINT8(crc0, uart.written[7]);
    TEST_ASSERT_EQUAL_UINT8(crc1, uart.written[15]);
}

// setMicrosteps written packet must also have a valid CRC.
void test_set_microsteps_packet_has_valid_crc() {
    MockUart uart; StepperUartConfig config(uart, 0, 110);
    config.begin(115200);
    config.setMicrosteps(16);
    TEST_ASSERT_EQUAL_size_t(8, uart.written.size());
    uint8_t expected_crc = ref_crc(uart.written.data(), 7);
    TEST_ASSERT_EQUAL_UINT8(expected_crc, uart.written[7]);
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int, char**) {
    UNITY_BEGIN();

    // Servo
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
    RUN_TEST(test_servo_duty_at_0_degrees);
    RUN_TEST(test_servo_duty_at_180_degrees);
    RUN_TEST(test_servo_angle_reflects_last_set);
    RUN_TEST(test_servo_microseconds_at_min_boundary);
    RUN_TEST(test_servo_microseconds_at_max_boundary);

    // Stepper state
    RUN_TEST(test_stepper_not_running_after_begin);
    RUN_TEST(test_stepper_set_position_positive);
    RUN_TEST(test_stepper_set_position_negative);
    RUN_TEST(test_stepper_stop_when_idle_is_safe);
    RUN_TEST(test_stepper_move_zero_stays_idle);
    RUN_TEST(test_stepper_move_enables_timer_alarm);
    RUN_TEST(test_stepper_stop_disables_timer_alarm);
    RUN_TEST(test_stepper_enable_pulls_pin_low);
    RUN_TEST(test_stepper_disable_pulls_pin_high);
    RUN_TEST(test_stepper_moveto_same_position_is_noop);
    RUN_TEST(test_stepper_set_position_cancels_running_move);
    RUN_TEST(test_stepper_stop_mid_move_freezes_position);
    RUN_TEST(test_stepper_round_trip_returns_to_zero);

    // Stepper ISR simulation
    RUN_TEST(test_stepper_isr_advances_position_forward);
    RUN_TEST(test_stepper_isr_advances_position_backward);
    RUN_TEST(test_stepper_isr_partial_move_still_running);
    RUN_TEST(test_stepper_isr_extra_ticks_are_noop_after_stop);

    // UartConfig — basic I/O
    RUN_TEST(test_uart_config_begin_opens_uart);
    RUN_TEST(test_uart_config_send_text_writes_bytes);
    RUN_TEST(test_uart_config_send_bytes_writes_exact_packet);

    // calcCRC gold-standard
    RUN_TEST(test_crc_known_zero_data_datagram);

    // writeRegister packet structure
    RUN_TEST(test_write_register_packet_structure);
    RUN_TEST(test_write_register_uses_correct_slave_address);

    // setMicrosteps MRES encoding
    RUN_TEST(test_microsteps_256_encodes_mres_0);
    RUN_TEST(test_microsteps_128_encodes_mres_1);
    RUN_TEST(test_microsteps_64_encodes_mres_2);
    RUN_TEST(test_microsteps_32_encodes_mres_3);
    RUN_TEST(test_microsteps_16_encodes_mres_4);
    RUN_TEST(test_microsteps_8_encodes_mres_5);
    RUN_TEST(test_microsteps_4_encodes_mres_6);
    RUN_TEST(test_microsteps_2_encodes_mres_7);
    RUN_TEST(test_microsteps_1_encodes_mres_8_full_step);
    RUN_TEST(test_microsteps_invalid_value_encodes_mres_8);

    // currentToCS + setCurrent bit packing
    RUN_TEST(test_current_zero_run_ma_gives_irun_0);
    RUN_TEST(test_current_zero_hold_ma_gives_ihold_0);
    RUN_TEST(test_current_800ma_110mohm_gives_irun_13);
    RUN_TEST(test_current_2000ma_saturates_to_cs_31);
    RUN_TEST(test_current_hold_delay_packed_correctly);
    RUN_TEST(test_current_hold_delay_masked_to_4_bits);
    RUN_TEST(test_current_run_greater_than_hold_both_encoded);

    // VSENSE / CHOPCONF interaction
    RUN_TEST(test_current_rsense_110_clears_vsense_bit);
    RUN_TEST(test_current_rsense_75_sets_vsense_bit);
    RUN_TEST(test_current_rsense_100_boundary_clears_vsense);
    RUN_TEST(test_current_writes_two_packets_total_16_bytes);
    RUN_TEST(test_current_both_packets_have_valid_crcs);
    RUN_TEST(test_set_microsteps_packet_has_valid_crc);

    return UNITY_END();
}
