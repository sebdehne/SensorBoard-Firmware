#include "DS3231.h"

DS3231Class::DS3231Class() {}

void DS3231Class::setup()
{
    Wire.begin();
}

bool DS3231Class::setAddrForRead(byte addr)
{
    Wire.beginTransmission(i2c_addr);
    Wire.write(addr);
    byte result = Wire.endTransmission(false);
    if (result != 0)
    {
        Serial.println("Error setAddrForRead()");
        return false;
    }
    else
    {
        return true;
    }
}

bool DS3231Class::hasTime()
{
    // Change addr-pointer to 0x0F (control byte)
    if (!setAddrForRead(0x0F))
    {
        return false;
    }

    // read the 0x0F (control)
    Wire.requestFrom(i2c_addr, 1);
    int control = 0;
    while (Wire.available())
    {
        control = Wire.read();
        break;
    }
    if (control < 0)
    {
        Serial.println("Error in hasTime() - 2");
        return false;
    }

    // eval the OSF bit
    return !(control & 0b10000000);
}

void DS3231Class::readTime()
{
    if (!setAddrForRead(0))
    {
        return;
    }
    Wire.requestFrom(104, 7);

    if (!Wire.available())
    {
        Serial.println("Could not read seconds");
        return;
    }
    int read = Wire.read();
    seconds = read & 0b1111;
    read = read >> 4;
    read = read * 10;
    seconds += read;

    if (!Wire.available())
    {
        Serial.println("Could not read minutes");
        return;
    }
    read = Wire.read();
    minutes = read & 0b1111;
    read = read >> 4;
    read = read * 10;
    minutes += read;

    if (!Wire.available())
    {
        Serial.println("Could not read hour");
        return;
    }
    read = Wire.read();

    hour = read & 0b1111;
    if (read & 0b010000)
    {
        hour += 10;
    }
    else if (read & 0b100000)
    {
        hour += 20;
    }

    if (!Wire.available())
    {
        Serial.println("Could not read day");
        return;
    }
    weekDay = Wire.read();

    if (!Wire.available())
    {
        Serial.println("Could not read date");
        return;
    }
    read = Wire.read();
    date = read & 0b1111;
    read = read >> 4;
    date += (read * 10);

    if (!Wire.available())
    {
        Serial.println("Could not read month");
        return;
    }
    read = Wire.read();
    month = read & 0b1111;
    if (read & 0x10000)
    {
        month += 10;
    }

    if (!Wire.available())
    {
        Serial.println("Could not read year");
        return;
    }
    read = Wire.read();
    year = read & 0b1111;
    read = read >> 4;
    year += read * 10;
}

void DS3231Class::setTime(byte seconds, byte minutes, byte hour, byte weekDay, byte date, byte month, byte year)
{
    byte write = 0;
    Wire.beginTransmission(i2c_addr);
    Wire.write(0);

    // write seconds
    write = (seconds / 10) << 4;
    write += seconds % 10;
    Wire.write(write);

    // write minutes
    write = (minutes / 10) << 4;
    write += minutes % 10;
    Wire.write(write);

    // write hour
    write = 0;
    if (hour > 19)
    {
        write |= 0b100000;
    }
    else if (hour > 9)
    {
        write |= 0b010000;
    }
    write += hour % 10;
    Wire.write(write);

    // Write day
    Wire.write(weekDay);

    // write date
    write = (date / 10) << 4;
    write += (date % 10);
    Wire.write(write);

    // write month
    write = (month / 10) << 4;
    write += month % 10;
    Wire.write(write);

    // write year
    write = (year / 10) << 4;
    write += year % 10;
    Wire.write(write);

    Wire.endTransmission(false);

    // Clear all bits in the CONTROL byte
    Wire.beginTransmission(i2c_addr);
    Wire.write(0x0F);
    Wire.write(0);
    Wire.endTransmission(true);
}

byte DS3231Class::getSeconds()
{
    return seconds;
}
byte DS3231Class::getMinutes()
{
    return minutes;
}
byte DS3231Class::getHour()
{
    return hour;
}
byte DS3231Class::getWeekDay()
{
    return weekDay;
}
byte DS3231Class::getDate()
{
    return date;
}
byte DS3231Class::getMonth()
{
    return month;
}
byte DS3231Class::getYear()
{
    return year;
}

unsigned long DS3231Class::getSecondsSince2000()
{
    unsigned long leapYears = (year / 4) + 1;
    unsigned long nonLeapYears = year - leapYears;
    unsigned long days = (leapYears * 366) + (nonLeapYears * 365);

    for (int i = 1; i < month; i++)
    {
        if (i == 4 || i == 6 || i == 9 || i == 11)
        {
            days += 30;
        }
        else if (i == 2)
        {
            days += 29;
            if (year % 4) {
                days--;
            }
        }
        else
        {
            days += 31;
        }
    }

    days += (date - 1);

    return (days * 86400) + (hour * 3600) + (minutes * 60) + seconds;
}

DS3231Class DS3231;