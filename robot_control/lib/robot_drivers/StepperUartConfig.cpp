#include "StepperUartConfig.h"
#include <string.h>

// TMC2209 register addresses
static constexpr uint8_t  REG_CHOPCONF     = 0x6C;
static constexpr uint8_t  REG_IHOLD_IRUN  = 0x10;
// CHOPCONF default: toff=3, hstrt=4, hend=1, TBL=2, CHM=0 (datasheet reset value)
static constexpr uint32_t CHOPCONF_DEFAULT = 0x10000053UL;

// MRES field occupies bits 27:24 of CHOPCONF.
// Maps requested microsteps to the 4-bit MRES encoding.
static uint8_t microstepsToMres(uint16_t ms) {
    switch (ms) {
        case 256: return 0;
        case 128: return 1;
        case  64: return 2;
        case  32: return 3;
        case  16: return 4;
        case   8: return 5;
        case   4: return 6;
        case   2: return 7;
        default:  return 8; // full step
    }
}

StepperUartConfig::StepperUartConfig(IUart& uart, uint8_t slaveAddr, uint16_t rsense)
    : _uart(uart), _slaveAddr(slaveAddr), _rsense(rsense), _chopconf(CHOPCONF_DEFAULT) {}

void StepperUartConfig::begin(uint32_t baudRate) { _uart.begin(baudRate); }
void StepperUartConfig::end()                    { _uart.end(); }

void StepperUartConfig::setMicrosteps(uint16_t microsteps) {
    _chopconf &= ~(0x0FUL << 24);
    _chopconf |=  ((uint32_t)microstepsToMres(microsteps) << 24);
    writeRegister(REG_CHOPCONF, _chopconf);
}

void StepperUartConfig::setCurrent(uint16_t runMA, uint16_t holdMA, uint8_t holdDelay) {
    // VSENSE bit (17): use 180 mV reference for Rsense < 100 mΩ, else 325 mV.
    // Must be written to CHOPCONF before IHOLD_IRUN so the driver uses the right Vref.
    bool vsense = (_rsense < 100);
    if (vsense)
        _chopconf |=  (1UL << 17);
    else
        _chopconf &= ~(1UL << 17);
    writeRegister(REG_CHOPCONF, _chopconf);

    uint16_t vref = vsense ? 180 : 325;
    uint8_t irun  = currentToCS(runMA,  _rsense, vref);
    uint8_t ihold = (holdMA == 0) ? 0 : currentToCS(holdMA, _rsense, vref);

    uint32_t val = ((uint32_t)(holdDelay & 0x0F) << 16)
                 | ((uint32_t)(irun       & 0x1F) <<  8)
                 | ((uint32_t)(ihold      & 0x1F));
    writeRegister(REG_IHOLD_IRUN, val);
}

// TMC2209 write datagram (8 bytes):
//   [0] 0x05        sync + reserved
//   [1] slave addr
//   [2] reg | 0x80  register address with write flag
//   [3..6]          32-bit data, MSB first
//   [7]             CRC-8 over bytes 0–6
void StepperUartConfig::writeRegister(uint8_t reg, uint32_t value) {
    uint8_t pkt[8] = {
        0x05,
        _slaveAddr,
        static_cast<uint8_t>(reg | 0x80),
        static_cast<uint8_t>(value >> 24),
        static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >>  8),
        static_cast<uint8_t>(value),
        0x00
    };
    pkt[7] = calcCRC(pkt, 7);
    _uart.write(pkt, 8);
}

// CS = (I_RMS * (Rsense + 20 mΩ) * sqrt(2) / Vref) * 32 − 1
// sqrt(2) approximated as 1414/1000 to stay in integer arithmetic.
// uint64_t avoids overflow: 2000 mA * 150 mΩ * 32 * 1414 ≈ 13.6 × 10⁹.
uint8_t StepperUartConfig::currentToCS(uint16_t ma, uint16_t rsense_mOhm, uint16_t vref_mV) {
    uint64_t num    = (uint64_t)ma * (rsense_mOhm + 20) * 32 * 1414;
    uint64_t den    = (uint64_t)vref_mV * 1000000ULL;
    uint64_t csPlus1 = num / den;
    if (csPlus1 == 0) return 0;
    uint64_t cs = csPlus1 - 1;
    return (uint8_t)(cs > 31 ? 31 : cs);
}

// CRC-8/SMBus (poly 0x07) over `len` bytes, LSB first within each byte.
uint8_t StepperUartConfig::calcCRC(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if ((crc >> 7) ^ (byte & 0x01))
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
            byte >>= 1;
        }
    }
    return crc;
}

void StepperUartConfig::sendText(const char* cmd) {
    _uart.write(reinterpret_cast<const uint8_t*>(cmd), strlen(cmd));
}

void StepperUartConfig::sendBytes(const uint8_t* data, size_t len) {
    _uart.write(data, len);
}
