#include "SmartHomeServerClient.h"
#include "logger.h"
#include "DS3231.h"
#include "utils.h"
#include <ArduinoOTA.h>

SmartHomeServerClientClass::SmartHomeServerClientClass() {}

bool SmartHomeServerClientClass::ping()
{

    uint8_t ping_data[1];
    ping_data[0] = FIRMWARE_VERSION;

    return sendMessage(0, ping_data, sizeof(ping_data));
}

InboundPacketHeader SmartHomeServerClientClass::receivePong()
{
    uint8_t ping_data[1];
    InboundPacketHeader inboundPacketHeader = receiveMessage(
        ping_data,
        sizeof(ping_data),
        2000);
    if (inboundPacketHeader.receiveError)
    {
        return inboundPacketHeader;
    }

    if (inboundPacketHeader.type == 1 && ping_data[0] == FIRMWARE_VERSION)
    {
        Log.log("Pong response valid!!!!!!!!!!!!!");
    }
    else
    {
        Log.log("Pong response invalid");
    }
    return inboundPacketHeader;
}

bool SmartHomeServerClientClass::sendSensorData(
    unsigned long temp,
    unsigned long humidity,
    unsigned long adcBattery,
    unsigned long adcLight,
    unsigned long sleepTimeInSeconds,
    uint8_t firmwareVersion)
{
    uint8_t payload[5 * 4 + 1];
    writeUint32(temp, payload, 0);
    writeUint32(humidity, payload, 4);
    writeUint32(adcBattery, payload, 8);
    writeUint32(adcLight, payload, 12);
    writeUint32(sleepTimeInSeconds, payload, 16);
    payload[20] = firmwareVersion;

    return sendMessage(2, payload, sizeof(payload));
}

SensorDataResponse SmartHomeServerClientClass::receiveSensorDataResponse()
{
    uint8_t payload[6];
    SensorDataResponse sensorDataResponse;
    sensorDataResponse.receiveError = true;

    InboundPacketHeader inboundPacketHeader = receiveMessage(payload, sizeof(payload), 2000);
    if (inboundPacketHeader.receiveError)
    {
        return sensorDataResponse;
    }

    if (!hasValidTimestamp(inboundPacketHeader))
    {
        Log.log("Invalid timestamp in response");
        return sensorDataResponse;
    }

    if (inboundPacketHeader.type != 3)
    {
        Log.log("Invalid response type received");
        return sensorDataResponse;
    }

    sensorDataResponse.receiveError = false;
    sensorDataResponse.firmwareUpdateRequired = payload[0];
    sensorDataResponse.timeAdjustmentRequired = payload[1];
    sensorDataResponse.sleepTimeInSeconds = toUInt(payload, 2);
    sensorDataResponse.timestamp = inboundPacketHeader.timestamp;
    char buf[100];
    snprintf(
        buf,
        sizeof(buf),
        "SensorDataResponse received. updateFirmware=%d adjustTime=%d sleepTimeInSeconds=%lu",
        sensorDataResponse.firmwareUpdateRequired,
        sensorDataResponse.timeAdjustmentRequired,
        sensorDataResponse.sleepTimeInSeconds);
    Log.log(buf);

    return sensorDataResponse;
}

InboundPacketHeader SmartHomeServerClientClass::receiveMessage(
    uint8_t *payloadBuffer,
    size_t payloadBufferLength,
    const unsigned long timeout)
{
    const unsigned long startTime = millis();

    InboundPacketHeader inboundPacketHeader;
    inboundPacketHeader.receiveError = true;

    uint8_t receiveBuffer[RN2483.maxDataSize];

    while (millis() - startTime < timeout)
    {

        int bytesReceived = RN2483.receiveData(
            receiveBuffer,
            sizeof(receiveBuffer),
            timeout - (millis() - startTime));

        if (bytesReceived < headerSize)
        {
            Log.log("Ignoring too few bytes (or none)");
            continue;
        }
        if (receiveBuffer[0] != LORA_ADDR)
        {
            Log.log("Ignoring message not for me");
            continue;
        }
        inboundPacketHeader.type = receiveBuffer[2];
        inboundPacketHeader.timestamp = toUInt(receiveBuffer, 3);
        inboundPacketHeader.payloadLength = toUInt(receiveBuffer, 7);
        if (inboundPacketHeader.payloadLength > payloadBufferLength)
        {
            Log.log("payloadBufferLength too small");
            return inboundPacketHeader;
        }

        for (unsigned int i = 0; i < inboundPacketHeader.payloadLength; i++)
        {
            (*(payloadBuffer++)) = receiveBuffer[headerSize + i];
        }
        inboundPacketHeader.receiveError = false;
        return inboundPacketHeader;
    }

    Log.log("receiveMessage() - timeout");
    return inboundPacketHeader;
}

bool SmartHomeServerClientClass::hasValidTimestamp(InboundPacketHeader inboundPacketHeader)
{
    DateTime dateTime = DS3231.readTime();

    long delta = dateTime.secondsSince2000 - inboundPacketHeader.timestamp;

    if (delta < 30 && delta > -30)
    {
        return true;
    }
    return false;
}

bool SmartHomeServerClientClass::sendMessage(uint8_t type, unsigned char *payload, size_t payloadLength)
{
    uint8_t data[255];
    if (payloadLength + headerSize > RN2483.maxDataSize)
    {
        Log.log("Payload too large");
        return false;
    }
    DateTime time = DS3231.readTime();

    data[0] = 1;         // to addr
    data[1] = LORA_ADDR; // from addr
    data[2] = type;
    writeUint32(time.secondsSince2000, data, 3);
    writeUint32(payloadLength, data, 7);
    for (unsigned int i = 0; i < payloadLength; i++)
    {
        data[i + headerSize] = payload[i];
    }

    return RN2483.transmitData(data, payloadLength + headerSize);
}

void SmartHomeServerClientClass::upgradeFirmware()
{
    char logBuf[1024];
    Log.log("Start firmware upgrade");

    delay(500); // going too fast after sensorDataResponse doesn't work
    FirmwareInfoResponse firmwareInfoResponse = getFirmwareInfo();
    if (firmwareInfoResponse.receiveError)
    {
        Log.log("Abort - firmwareInfoResponse.receiveError");
        return;
    }
    Log.log("Got firmwareInfoResponse, getting the data now...");

    delay(500); // going too fast after firmwareInfoResponse doesn't work
    const uint8_t maxFirmwareBytesPerResponse = 255 - headerSize - CryptUtil.encryptionOverhead - 1; //  215
    const int packetsPerAck = 5;
    const unsigned long receiveBytesWithoutAck = maxFirmwareBytesPerResponse * packetsPerAck;
    unsigned long bytesWrittenToFlash = 0;
    uint8_t receiveBuffer[maxFirmwareBytesPerResponse + 1];
    unsigned long awaitingIncomingBytes = 0;
    int sequentNumber = 0;
    int retryAttempts = LORA_RETRY_COUNT;
    bool failed = false;
    while (true)
    {
        if (failed)
        {
            failed = false;
            delay(LORA_RETRY_DELAY);
            retryAttempts--;
            awaitingIncomingBytes = 0;
        }

        if (retryAttempts < 0)
        {
            Log.log("Failed. Ran out of retries");
            return;
        }

        if (awaitingIncomingBytes == 0)
        {
            if ((firmwareInfoResponse.totalLength - bytesWrittenToFlash) > receiveBytesWithoutAck)
            {
                awaitingIncomingBytes = receiveBytesWithoutAck;
            }
            else
            {
                awaitingIncomingBytes = firmwareInfoResponse.totalLength - bytesWrittenToFlash;
            }

            uint8_t firmwareDataPayload[9];
            writeUint32(bytesWrittenToFlash, firmwareDataPayload, 0);
            writeUint32(awaitingIncomingBytes, firmwareDataPayload, 4);
            firmwareDataPayload[8] = maxFirmwareBytesPerResponse;
            if (!sendMessage(6, firmwareDataPayload, sizeof(firmwareDataPayload)))
            {
                Log.log("Failed. Could not send firmwareDataRequest");
                failed = true;
                continue;
            }
            sequentNumber = 0;
        }

        InboundPacketHeader inboundPacketHeader = receiveMessage(
            receiveBuffer,
            sizeof(receiveBuffer),
            2000);

        if (inboundPacketHeader.receiveError)
        {
            Log.log("receiveError");
            failed = true;
            continue;
        }
        if (!hasValidTimestamp(inboundPacketHeader))
        {
            Log.log("Invalid timestamp in response");
            failed = true;
            continue;
        }
        if (inboundPacketHeader.type != 7)
        {
            Log.log("Invalid response type");
            failed = true;
            continue;
        }
        if (receiveBuffer[0] != sequentNumber)
        {
            // out of sync, abort
            Log.log("Out of sync");
            failed = true;
            continue;
        }

        if (bytesWrittenToFlash == 0)
        {
            InternalStorage.open(firmwareInfoResponse.totalLength);
        }

        // ok all good, accept the bytes
        unsigned long firmwareBytesReceived = inboundPacketHeader.payloadLength - 1;
        snprintf(logBuf, sizeof(logBuf), "OK (%i) accepted %lu bytes", sequentNumber, firmwareBytesReceived);
        Log.log(logBuf);
        for (unsigned long j = 0; j < firmwareBytesReceived; j++)
        {
            InternalStorage.write(receiveBuffer[j + 1]);
            bytesWrittenToFlash++;
            awaitingIncomingBytes--;
        }
        sequentNumber++;
        retryAttempts = LORA_RETRY_COUNT;

        if (bytesWrittenToFlash == firmwareInfoResponse.totalLength)
        {
            InternalStorage.close();
            break;
        }
    }

    // check crc32
    crc32.reset();
    byte *addr = (byte *)InternalStorage.STORAGE_START_ADDRESS;
    for (unsigned long i = 0; i < firmwareInfoResponse.totalLength; i++)
    {
        crc32.update(*addr);
        addr++;
    }
    unsigned long firmwareChecksum = crc32.finalize();

    if (firmwareChecksum != firmwareInfoResponse.crc32)
    {
        snprintf(
            logBuf,
            sizeof(logBuf),
            "CRC32-error: firmwareSize=%lu receivedCRC32=%lu firmwareChecksum=%lu",
            firmwareInfoResponse.totalLength,
            firmwareInfoResponse.crc32,
            firmwareChecksum);
        Log.log(logBuf);
    }
    else
    {
        Log.log("Rebooting to new firmware now");
        Serial.flush();
        delay(1000);
        InternalStorage.apply();
    }
}

FirmwareInfoResponse SmartHomeServerClientClass::getFirmwareInfo()
{
    FirmwareInfoResponse firmwareInfoResponse;
    firmwareInfoResponse.receiveError = true;

    for (int i = 0; i < LORA_RETRY_COUNT; i++)
    {
        uint8_t payload[0];
        if (sendMessage(4, payload, 0))
        {
            uint8_t firmwareInfoData[8];
            InboundPacketHeader inboundPacketHeader = receiveMessage(
                firmwareInfoData,
                sizeof(firmwareInfoData),
                2000);
            if (!inboundPacketHeader.receiveError)
            {
                if (!hasValidTimestamp(inboundPacketHeader))
                {
                    Log.log("Invalid timestamp");
                }
                else if (inboundPacketHeader.type != 5)
                {
                    Log.log("Invalid response type");
                }
                else
                {
                    firmwareInfoResponse.receiveError = false;
                    firmwareInfoResponse.totalLength = toUInt(firmwareInfoData, 0);
                    firmwareInfoResponse.crc32 = toUInt(firmwareInfoData, 4);
                    return firmwareInfoResponse;
                }
            }
        }

        Log.log("Retry getFirmwareInfo()");
        delay(LORA_RETRY_DELAY);
    }
    return firmwareInfoResponse;
}


SmartHomeServerClientClass SmartHomeServerClient;