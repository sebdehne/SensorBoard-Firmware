#ifndef _H_CHIP_CAP2
#define _H_CHIP_CAP2
#include <Arduino.h>
#include "Wire.h"

struct TempAndHumidity
{
    bool error;
    unsigned long temp;
    unsigned long humidity;
};


class ChipCap2Class
{
private:
    const byte i2c_addr = 0b0101000;

public:
    ChipCap2Class();
    TempAndHumidity read();
};

extern ChipCap2Class ChipCap2;

#endif