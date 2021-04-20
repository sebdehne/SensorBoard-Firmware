#include "RN2483.h"
#include "logger.h"
#include <Arduino.h>
#include "utils.h"

RN2483Class::RN2483Class() {}

void RN2483Class::setup()
{
    int bytesRead;
    char buf[100];
    char buf2[100];

    // @  0ms power on
    // @ 99ms TX LOW->HIGH
    // @257ms transmits its version number
    // (but Seeeduino XIAO bootloader consumes 2000ms anyway)

    // setup UART to RN2483A module
    while (1)
    {
        Log.log("Setting up communication to RN2483...");
        Serial1.end();

        // reset D2
        pinMode(2, OUTPUT);
        digitalWrite(2, LOW);
        delay(500); // TODO too long
        digitalWrite(2, HIGH);
        Serial1.begin(57600);

        // read its version number
        Log.log("Reset done, waiting for version string...");
        bytesRead = readResponse(buf, sizeof(buf), 2000);
        snprintf(buf2, sizeof(buf2), "Reset done, waiting for version string...got %u : %s", bytesRead, buf);
        Log.log(buf2);
        if (bytesRead > 0)
        {
            if (strncmp("RN2483", buf, 6) == 0)
            {
                break;
            }
        }
    }
    Log.log("Setting up communication to RN2483...Done");
}

bool RN2483Class::consumeAndLog()
{
    uint8_t hexString[1024];
    int bytesRead = 0;
    while (Serial1.available())
    {
        hexString[bytesRead++] = Serial1.read();
    }
    if (bytesRead > 0)
    {
        hexString[bytesRead] = 0;
        char buf[1024];
        snprintf(buf, sizeof(buf), "sinked text: %s", hexString);
        Log.log(buf);
        return true;
    }

    return false;
}

int RN2483Class::sendCommandRaw(const char cmd[], char *receiveBuf, size_t length)
{
    consumeAndLog();

    // send cmd
    Serial1.println(cmd);

    int bytesReceived = readResponse(receiveBuf, length, 1000);

    char logBuf[100];
    if (bytesReceived > 0)
    {
        snprintf(logBuf, 100, "LoRa cmd: %s -> %u: %s", cmd, bytesReceived, receiveBuf);
        Log.log(logBuf);
    }
    else
    {
        snprintf(logBuf, 100, "LoRa cmd: %s -> %u", cmd, bytesReceived);
        Log.log(logBuf);
    }

    return bytesReceived;
}

int RN2483Class::readResponse(char *receiveBuf, size_t length, const unsigned long timeout)
{
    // read response
    unsigned long start = millis();
    int bytesRead = 0;
    int read = 0;
    do
    {
        read = Serial1.read();
        if (read >= 0)
        {
            receiveBuf[bytesRead] = read;
            bytesRead++;
            if (read == '\n')
            {
                break;
            }
        }
    } while (millis() - start < timeout);

    if (bytesRead == 0)
    {
        Log.log("readResponse() - nothing received");
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

bool RN2483Class::transmitMessage(uint8_t type, ssize_t length, uint8_t payload[])
{
    uint8_t data[255];

    data[0] = 1;         // to addr
    data[1] = LORA_ADDR; // from addr
    data[2] = type;
    writeUint32(length, data, 3);
    for (ssize_t i = 0; i < length; i++)
    {
        data[i + 7] = payload[i];
    }

    char hexString[255 * 2 + 1]; // max 255 bytes (1 bytes takes 2 chars + 1 for \0)
    toHex(data, length + 7, hexString);

    char cmdString[sizeof(hexString) + 10];
    snprintf(cmdString, sizeof(cmdString), "radio tx %s", hexString);

    char receiveBuf[100];

    // send cmd and check for "ok"
    int bytesReceived = sendCommandRaw(cmdString, receiveBuf, 100);
    if (bytesReceived > 0)
    {
        if (strncmp("ok", receiveBuf, 2) != 0)
        {
            char buf[100];
            snprintf(buf, 100, "Unexpected response: %s", receiveBuf);
            Log.log(buf);
            return false;
        }
    }
    else
    {
        Log.log("No valid response received after radio tx cmd");
        return false;
    }

    // receive "radio_tx_ok"
    bytesReceived = readResponse(receiveBuf, 100, 1000);
    if (bytesReceived > 0)
    {
        if (strncmp("radio_tx_ok", receiveBuf, 11) != 0)
        {
            char buf[100];
            snprintf(buf, 100, "Unexpected response: %s", receiveBuf);
            Log.log(buf);
            return false;
        }
    }
    else
    {
        Log.log("No valid response received while waiting for radio_tx_ok");
        return false;
    }

    return true;
}

int RN2483Class::receiveMessage(uint8_t *receiveBuf, const size_t length, const unsigned long timeout)
{
    const unsigned long startTime = millis();
    char buffer[(255 * 2) + 10 + 1];

    int hexStartPos = -1;
    while (millis() - startTime < timeout)
    {
        int bytesReceived = sendCommandRaw("radio rx 0", buffer, sizeof(buffer));
        if (bytesReceived <= 0)
        {
            return -1;
        }
        if (strncmp(buffer, "ok", 2) != 0)
        {
            continue;
        }

        int duration = millis() - startTime;
        if (duration > timeout)
        {
            return -1;
        }

        char buf[1000];
        snprintf(buf, 1000, "About to listen for incoming RF... timeout: %u", timeout - duration);
        Log.log(buf);
        bytesReceived = readResponse(buffer, sizeof(buffer), timeout - duration);
        if (bytesReceived <= 0)
        {
            continue;
        }

        snprintf(buf, 1000, "Received: %s", buffer);
        Log.log(buf);

        if (strncmp(buffer, "radio_rx ", 9) != 0)
            continue;

        // find the start pos of the hex string
        int pos = -1;
        while (++pos < sizeof(buffer))
        {
            char c = buffer[pos];
            if (c >= '0' && c <= '9')
            {
                hexStartPos = pos;
                break;
            }
        }

        if (hexStartPos < 0)
        {
            Log.log("Could not find start pos");
            continue;
        }

        // decode hex string
        bytesReceived = fromHex(buffer + hexStartPos, receiveBuf, length);
        if (bytesReceived < 0)
        {
            snprintf(buf, 1000, "Could parse hex string: %s", buffer + hexStartPos);
            Log.log(buf);
            continue;
        }

        if (bytesReceived < 7)
        {
            Log.log("Message too short");
            continue;
        }

        if (*receiveBuf != LORA_ADDR)
        {
            Log.log("Msg not for me");
            continue;
        }

        return bytesReceived;
    }

    Log.log("Timeout while receiving a message");
    sendCommandRaw("radio rxstop", buffer, sizeof(buffer));
    return -1;
}

RN2483Class RN2483;