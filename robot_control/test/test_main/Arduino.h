#pragma once
// Minimal native mock. Only the symbols robot_drivers actually use remain here.
// GPIO, timer, and LEDC stubs are gone — those now live in src/Esp32*.h which
// native tests never include. The drivers call through IGpio/ITimer/ILedc
// interfaces, so tests inject mock objects instead.
#include <stdint.h>
#include <algorithm>
using std::max;
using std::min;

// FreeRTOS critical-section — no-op on native
typedef struct { int owner; int count; } portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED {0, 0}
#define portENTER_CRITICAL(m)     ((void)(m))
#define portEXIT_CRITICAL(m)      ((void)(m))
#define portENTER_CRITICAL_ISR(m) ((void)(m))
#define portEXIT_CRITICAL_ISR(m)  ((void)(m))

#define IRAM_ATTR

#define constrain(amt, lo, hi) ((amt)<(lo)?(lo):((amt)>(hi)?(hi):(amt)))
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
inline void delayMicroseconds(uint32_t) {}
