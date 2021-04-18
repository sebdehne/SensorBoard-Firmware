#include "ChipCap2.h"
#include "logger.h"

ChipCap2Class::ChipCap2Class() {}

TempAndHumidity ChipCap2Class::read()
{
    TempAndHumidity result;
    result.error = true;

    Wire.requestFrom(i2c_addr, 4);

    if (!Wire.available())
    {
        Serial.println("Could not read byte 1");
        return result;
    }
    result.humidity = Wire.read();
    result.humidity = result.humidity << 8;
    if (!Wire.available())
    {
        Serial.println("Could not read byte 2");
        return result;
    }
    result.humidity += Wire.read();
    // ignore status bits:
    result.humidity = result.humidity & 0b11111111111111;

    if (!Wire.available())
    {
        Serial.println("Could not read byte 3");
        return result;
    }
    result.temp = Wire.read();
    result.temp = result.temp << 8;

    if (!Wire.available())
    {
        Serial.println("Could not read byte 4");
        return result;
    }
    result.temp += Wire.read();
    result.temp = result.temp >> 2;

    // convert relativeHumidity
    result.humidity = (result.humidity * 100 * 100) / 16384;
    result.temp = ((result.temp * 100 * 165) / 16384) - 4000;
    result.error = false;

    return result;
}

ChipCap2Class ChipCap2;
