#include "SmartHomeServerClient.h"
#include "logger.h"
#include "DS3231.h"
#include "utils.h"

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

    if (ping_data[0] == FIRMWARE_VERSION)
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

        for (int i = 0; i < inboundPacketHeader.payloadLength; i++)
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
    for (int i = 0; i < payloadLength; i++)
    {
        data[i + headerSize] = payload[i];
    }

    return RN2483.transmitData(data, payloadLength + headerSize);
}

SmartHomeServerClientClass SmartHomeServerClient;