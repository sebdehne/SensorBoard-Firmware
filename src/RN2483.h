#ifndef _RN2483_H
#define _RN2483_H

#include <Arduino.h>

class RN2483Class
{
private:
    

public:
    RN2483Class();

    void setup();
    int readResponse(char *receiveBuf, size_t length);
    int sendCommandRaw(const char cmd[], char *receiveBuf, size_t length);

    bool transmitMessage(uint8_t type, unsigned long length, uint8_t payload[]);
    bool responseEqualsOK(char *receiveBuf, size_t length);
    bool responseEqualsRadioRx(char *receiveBuf, size_t length);

};

extern RN2483Class RN2483;

#endif