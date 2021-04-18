#ifndef _RN2483_H
#define _RN2483_H

#include <Arduino.h>

class RN2483Class
{
private:
    

public:
    RN2483Class();

    void setup();
    int readResponse(char *receiveBuf, size_t length, const unsigned long timeout);
    int sendCommandRaw(const char cmd[], char *receiveBuf, size_t length);
    bool consumeAndLog();

    bool transmitMessage(uint8_t type, ssize_t length, uint8_t payload[]);
    int receiveMessage(uint8_t *receiveBuf, const size_t length, const unsigned long timeout);

};

extern RN2483Class RN2483;

#endif