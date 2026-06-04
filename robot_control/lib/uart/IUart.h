#pragma once
#include <stdint.h>
#include <stddef.h>

class IUart {
public:
    virtual void begin(uint32_t baudRate) = 0;
    virtual void end() = 0;

    virtual void    write(uint8_t byte) = 0;
    virtual void    write(const uint8_t* buf, size_t len) = 0;
    virtual int     read() = 0;       // returns -1 when no data available
    virtual int     available() = 0;

    virtual ~IUart() = default;
};
