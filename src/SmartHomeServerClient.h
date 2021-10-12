#ifndef _SMARTHOME_CLIENT_H
#define _SMARTHOME_CLIENT_H

#include "RN2483.h"
#include "flash.h"


struct InboundPacketHeader
{
    bool receiveError;
    uint8_t type;
    uint8_t to;
    uint8_t from;
    unsigned long timestamp;
    unsigned long payloadLength;
};

struct SensorDataResponse
{
    bool receiveError;
    bool firmwareUpdateRequired;
    bool timeAdjustmentRequired;
    bool triggerReset;
    unsigned long sleepTimeInSeconds;
    unsigned long timestamp;
};

struct FirmwareInfoResponse
{
    bool receiveError;
    unsigned long totalLength;
    unsigned long crc32;
};


class SmartHomeServerClientClass
{
private:
    uint8_t loraAddr = 0;

    bool sendMessage(uint8_t type, unsigned char *payload, size_t payloadLength);
    InboundPacketHeader receiveMessage(uint8_t *payloadBuffer, size_t payloadBufferLength, const unsigned long timeout);
    bool hasValidTimestamp(InboundPacketHeader inboundPacketHeader);

    FirmwareInfoResponse getFirmwareInfo();

public:
    SmartHomeServerClientClass();

    int headerSize = 11;

    bool ping();
    InboundPacketHeader receivePong();
    void setLoraAddr(uint8_t addr);

    bool sendSensorData(
        unsigned long temp, 
        unsigned long humidity, 
        unsigned long adcBattery,
        unsigned long adcLight,
        unsigned long sleepTimeInSeconds,
        uint8_t firmwareVersion,
        bool temperatureError);
    SensorDataResponse receiveSensorDataResponse();

    void upgradeFirmware();

};

extern SmartHomeServerClientClass SmartHomeServerClient;


#endif