#ifndef _DS3231_H
#define _DS3231_H
#include <Arduino.h>
#include "Wire.h"

class DS3231Class
{
private:
    const byte i2c_addr = 104;
    byte seconds;
    byte minutes;
    byte hour;
    byte weekDay;
    byte date;
    byte month;
    byte year;

    bool setAddrForRead(byte addr);

public:
    DS3231Class();
    void setup();

    bool hasTime();
    void readTime();
    unsigned long getSecondsSince2000();

    //                   0-59,         0-59,     00-23,          1-7,     01-31,      01-12,     00-99
    void setTime(byte seconds, byte minutes, byte hour, byte weekDay, byte date, byte month, byte year);

    byte getSeconds();
    byte getMinutes();
    byte getHour();
    byte getWeekDay();
    byte getDate();
    byte getMonth();
    byte getYear();

    void setAlarm1(unsigned long seconds);
};

extern DS3231Class DS3231;

#endif