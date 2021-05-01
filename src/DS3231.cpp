#include "DS3231.h"

DS3231Class::DS3231Class() {}


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

DateTime DS3231Class::readTime()
{
    DateTime dateTime;
    dateTime.error = true;

    if (!setAddrForRead(0))
    {
        return dateTime;
    }
    Wire.requestFrom(i2c_addr, 16);

    if (!Wire.available())
    {
        Serial.println("Could not read seconds");
        return dateTime;
    }
    int read = Wire.read();
    dateTime.seconds = read & 0b1111;
    read = read >> 4;
    read = read * 10;
    dateTime.seconds += read;

    if (!Wire.available())
    {
        Serial.println("Could not read minutes");
        return dateTime;
    }
    read = Wire.read();
    dateTime.minutes = read & 0b1111;
    read = read >> 4;
    read = read * 10;
    dateTime.minutes += read;

    if (!Wire.available())
    {
        Serial.println("Could not read hour");
        return dateTime;
    }
    read = Wire.read();

    dateTime.hour = read & 0b1111;
    if (read & 0b010000)
    {
        dateTime.hour += 10;
    }
    else if (read & 0b100000)
    {
        dateTime.hour += 20;
    }

    if (!Wire.available())
    {
        Serial.println("Could not read day");
        return dateTime;
    }
    dateTime.weekDay = Wire.read();

    if (!Wire.available())
    {
        Serial.println("Could not read date");
        return dateTime;
    }
    read = Wire.read();
    dateTime.date = read & 0b1111;
    read = read >> 4;
    dateTime.date += (read * 10);

    if (!Wire.available())
    {
        Serial.println("Could not read month");
        return dateTime;
    }
    read = Wire.read();
    dateTime.month = read & 0b1111;
    if (read & 0x10000)
    {
        dateTime.month += 10;
    }

    if (!Wire.available())
    {
        Serial.println("Could not read year");
        return dateTime;
    }
    read = Wire.read();
    dateTime.year = read & 0b1111;
    read = read >> 4;
    dateTime.year += read * 10;

    // skip 8 bytes
    for (int i = 0; i < 8; i++)
    {
        if (!Wire.available())
        {
            Serial.println("Could skip bytes...");
            return dateTime;
        }
        Wire.read();
    }

    if (!Wire.available())
    {
        Serial.println("Could read reg2");
        return dateTime;
    }
    byte reg2 = Wire.read();

    dateTime.secondsSince2000 = calcSecondsSince2000(dateTime);
    dateTime.error = false;
    dateTime.alarm1Set = reg2 & 0b00000001;

    return dateTime;
}

bool DS3231Class::setTimeInternal(DateTime dateTime, bool writeAlarm1)
{
    byte write = 0;
    Wire.beginTransmission(i2c_addr);
    if (writeAlarm1)
    {
        Wire.write(0x07);
    }
    else
    {
        Wire.write(0);
    }

    // write seconds
    write = (dateTime.seconds / 10) << 4;
    write += dateTime.seconds % 10;
    Wire.write(write);

    // write minutes
    write = (dateTime.minutes / 10) << 4;
    write += dateTime.minutes % 10;
    Wire.write(write);

    // write hour
    write = 0;
    if (dateTime.hour > 19)
    {
        write |= 0b100000;
    }
    else if (dateTime.hour > 9)
    {
        write |= 0b010000;
    }
    write += dateTime.hour % 10;
    Wire.write(write);

    if (!writeAlarm1)
    {
        // Write weekDay
        Wire.write(dateTime.weekDay);
    }

    // write date
    write = (dateTime.date / 10) << 4;
    write += (dateTime.date % 10);
    Wire.write(write);

    if (!writeAlarm1)
    {
        // write month
        write = (dateTime.month / 10) << 4;
        write += dateTime.month % 10;
        Wire.write(write);

        // write year
        write = (dateTime.year / 10) << 4;
        write += dateTime.year % 10;
        Wire.write(write);
    }

    Wire.endTransmission(false);

    // read control registers
    Wire.beginTransmission(i2c_addr);
    Wire.write(0x0E);
    Wire.endTransmission(false);
    Wire.requestFrom(i2c_addr, 2);
    byte reg1 = 0;
    byte reg2 = 0;
    if (!Wire.available())
    {
        Serial.println("Error reading control-reg1");
        return false;
    }
    reg1 = Wire.read();
    if (!Wire.available())
    {
        Serial.println("Error reading control-reg2");
        return false;
    }
    reg2 = Wire.read();

    // configure control regs:
    if (writeAlarm1)
    {
        reg1 = reg1 | (1 << 0);    // set    A1IE
        reg2 = reg2 & (0xff << 1); // clear  A1F
    }
    else
    {
        // initial time setup
        reg1 = 0b00011101; // keep A1IE 1
        reg2 = 0b00000001; // keep A1F 1
    }

    // Write back control registers:
    Wire.beginTransmission(i2c_addr);
    Wire.write(0x0E);
    Wire.write(reg1);
    Wire.write(reg2);
    Wire.endTransmission(true);
    return true;
}

unsigned long DS3231Class::calcSecondsSince2000(DateTime dateTime)
{
    unsigned long leapYears = (dateTime.year / 4) + 1;
    unsigned long nonLeapYears = dateTime.year - leapYears;
    unsigned long days = (leapYears * 366) + (nonLeapYears * 365);

    for (int i = 1; i < dateTime.month; i++)
    {
        if (i == 4 || i == 6 || i == 9 || i == 11)
        {
            days += 30;
        }
        else if (i == 2)
        {
            days += 29;
            if (dateTime.year % 4)
            {
                days--;
            }
        }
        else
        {
            days += 31;
        }
    }

    days += (dateTime.date - 1);

    return (days * 86400) + (dateTime.hour * 3600) + (dateTime.minutes * 60) + dateTime.seconds;
}

DateTime DS3231Class::calcDateTime(unsigned long secondsSince2000)
{

    DateTime dateTime;

    unsigned long days = secondsSince2000 / 86400;
    int remainingSeconds = secondsSince2000 % 86400;
    dateTime.hour = remainingSeconds / 3600;
    remainingSeconds = remainingSeconds % 3600;
    dateTime.minutes = remainingSeconds / 60;
    dateTime.seconds = remainingSeconds % 60;

    int fourYearsPeriodes = days / (3 * 365 + 366);
    days = days % (3 * 365 + 366);
    dateTime.year = fourYearsPeriodes * 4;
    if (days > 366)
    {
        dateTime.year += 1;
        days -= 366;
    }
    if (days > 365)
    {
        dateTime.year += 1;
        days -= 365;
    }
    if (days > 365)
    {
        dateTime.year += 1;
        days -= 365;
    }
    dateTime.month = 1;
    unsigned daysInMonth;
    while (1)
    {
        if (dateTime.month == 4 || dateTime.month == 6 || dateTime.month == 9 || dateTime.month == 11)
        {
            daysInMonth = 30;
        }
        else if (dateTime.month == 2)
        {
            if (dateTime.year % 4)
            {
                daysInMonth = 28;
            }
            else
            {
                daysInMonth = 29;
            }
        }
        else
        {
            daysInMonth = 31;
        }

        if (days >= daysInMonth)
        {
            days -= daysInMonth;
            dateTime.month++;
        }
        else
        {
            break;
        }
    }
    dateTime.date = days + 1;

    return dateTime;
}

bool DS3231Class::setTime(unsigned long secondsSince2000)
{
    DateTime dateTime = calcDateTime(secondsSince2000);
    return setTimeInternal(dateTime, false);
}

bool DS3231Class::setAlarm1(unsigned long deltaSeconds)
{
    DateTime dateTime = readTime();
    if (dateTime.error) {
        return false;
    }
    unsigned long alarmAt = dateTime.secondsSince2000 + deltaSeconds;
    DateTime alarmDateTime = calcDateTime(alarmAt);
    return setTimeInternal(alarmDateTime, true);
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

DS3231Class DS3231;