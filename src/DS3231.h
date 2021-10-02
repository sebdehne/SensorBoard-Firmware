#ifndef _DS3231_H
#define _DS3231_H
#include <Arduino.h>
#include "Wire.h"
#include "logger.h"


struct DateTime
{
    bool error;
    unsigned long secondsSince2000;
    byte seconds;
    byte minutes;
    byte hour;
    byte weekDay;
    byte date;
    byte month;
    byte year;
    bool alarm1Set;
};


class DS3231Class
{
private:
    const byte i2c_addr = 104;

    bool setAddrForRead(byte addr);
    unsigned long calcSecondsSince2000(DateTime dateTime);
    DateTime calcDateTime(unsigned long secondsSince2000);
    bool setTimeInternal(DateTime dateTime, bool writeAlarm1);
    void logTime(DateTime dateTime);

public:
    DS3231Class();

    bool hasTime();
    DateTime readTime();

    // setting the time clears also the alarms
    bool setTime(unsigned long secondsSince2000);
    bool setAlarm1(unsigned long deltaSeconds);
};

extern DS3231Class DS3231;

#endif