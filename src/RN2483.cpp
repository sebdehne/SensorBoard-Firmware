#include "RN2483.h"
#include "logger.h"
#include <Arduino.h>
#include "utils.h"

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
        return -1;
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
        return -1;
    }

    // terminate string
    receiveBuf[pos] = 0;

    return pos;
}

bool RN2483Class::responseEqualsOK(char *receiveBuf, size_t length) {
    char okStr[] = "ok";
    return strncmp(receiveBuf, okStr, 2);
}

bool RN2483Class::responseEqualsRadioRx(char *receiveBuf, size_t length) {
    char radioRxStr[] = "radio_rx ";
    return strncmp(receiveBuf, radioRxStr, 9);
}

bool RN2483Class::transmitMessage(uint8_t type, unsigned long length, uint8_t payload[]) {
    uint8_t data[255];

    data[0] = LORA_ADDR; // from addr
    data[1] = 1; // to addr
    data[2] = type;
    writeUint32(length, data, 3);

    char hexString[255 * 2 + 1]; // max 255 bytes (1 bytes takes 2 chars + 1 for \0)
    toHex((data + 7), length, hexString);

    char cmdString[sizeof(hexString) + 10];
    snprintf(cmdString, sizeof(cmdString), "radio tx %s", hexString);

    char receiveBuf[100];
    int bytesReceived = sendCommandRaw(cmdString, receiveBuf, 100);

    // TODO responseEqualsOK
    // listen and check for radio_tx_ok response as well? read doc

    return false;
    
}

RN2483Class RN2483;