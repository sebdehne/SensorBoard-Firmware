#include "ChipCap2.h"
#include "logger.h"

ChipCap2Class::ChipCap2Class() {}

TempAndHumidity ChipCap2Class::read()
{
    TempAndHumidity result;
    result.error = true;

#ifdef DEBUG
    // send MR when not powering down
    Wire.beginTransmission(i2c_addr);
    Wire.endTransmission(true);
    delay(50); // tested that 10 is too short. 50 works.
#endif

    // read temp/humidity
    Wire.requestFrom(i2c_addr, 4);

    if (!Wire.available())
    {
        Serial.println("Could not read byte 1");
        return result;
    }
    result.humidity = Wire.read();

    if (result.humidity & (1 << 6)) {
        Log.log("Stale data received");
    }
    if (result.humidity & (1 << 7)) {
        Log.log("In Command mode");
    }

    result.humidity = result.humidity << 8;
    if (!Wire.available())
    {
        Serial.println("Could not read byte 2");
        return result;
    }
    result.humidity += Wire.read();
    // remove status bits:
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
