#include "RN2483.h"
#include "logger.h"
#include <Arduino.h>

RN2483Class::RN2483Class() {}

void RN2483Class::setup()
{
    int bytesRead;
    char buf[100];

    // @  0ms power on
    // @ 99ms TX LOW->HIGH
    // @257ms transmits its version number
    // (but Seeeduino XIAO bootloader consumes 2000ms anyway)

    // setup UART to RN2483A module
    while (1)
    {
        Serial.println("Setting up communication to RN2483...");
        Serial1.end();

        // reset
        pinMode(8, OUTPUT);
        digitalWrite(8, LOW);
        delay(500);
        digitalWrite(8, HIGH);
        delay(100);

        Serial1.begin(57600);
        while (Serial1.available())
        {
            Serial.println("Need to read");
            Serial1.read();
        }

        bytesRead = RN2483.sendCommandRaw("sys get hweui", buf, 16 + 2);
        if (bytesRead > 0)
        {
            break;
        }
    }
    Serial.println("Setting up communication to RN2483...Done");
}

int RN2483Class::sendCommandRaw(const char cmd[], char *receiveBuf, size_t length)
{
    while (Serial1.available())
    {
        Serial1.read();
        Serial.println("Consuming before writing");
    }

    // send cmd
    Serial1.println(cmd);

    return readResponse(receiveBuf, length);
}

int RN2483Class::readResponse(char *receiveBuf, size_t length)
{
    // read response
    unsigned long start = millis();
    int bytesRead = 0;
    int read = 0;
    do {
        read = Serial1.read();
        if (read >= 0) {
            receiveBuf[bytesRead] = read;
            bytesRead++;
            if (read == '\n') {
                break;
            }
        }
    } while(millis() - start < 2000);

    if (bytesRead == 0)
    {
        Serial.println("Could not read any bytes");
        return 0;
    }

    // find string end
    int pos = -1;
    for (int i = 0; i < bytesRead; i++)
    {
        if (receiveBuf[i] == '\n' || receiveBuf[i] == '\r')
        {
            pos = i;
            break;
        }
    }

    if (pos == -1)
    {
        Serial.println("Could not find LF");
        return 0;
    }

    // terminate string
    receiveBuf[pos] = 0;

    return pos;
}

RN2483Class RN2483;