#ifndef _SMARTHOME_CLIENT_H
#define _SMARTHOME_CLIENT_H

#include "RN2483.h"

struct InboundPacketHeader
{
    bool receiveError;
    uint8_t type;
    unsigned long timestamp;
    unsigned long payloadLength;
};

struct SensorDataResponse
{
    bool receiveError;
    bool firmwareUpdateRequired;
    bool timeAdjustmentRequired;
    unsigned long sleepTimeInSeconds;
    unsigned long timestamp;
};

class SmartHomeServerClientClass
{
private:
    bool sendMessage(uint8_t type, unsigned char *payload, size_t payloadLength);
    InboundPacketHeader receiveMessage(uint8_t *payloadBuffer, size_t payloadBufferLength, const unsigned long timeout);
    bool hasValidTimestamp(InboundPacketHeader inboundPacketHeader);

public:
    SmartHomeServerClientClass();

    int headerSize = 11;

    bool ping();
    InboundPacketHeader receivePong();

    bool sendSensorData(
        unsigned long temp, 
        unsigned long humidity, 
        unsigned long adcBattery,
        unsigned long adcLight,
        unsigned long sleepTimeInSeconds,
        uint8_t firmwareVersion);

    SensorDataResponse receiveSensorDataResponse();

};

extern SmartHomeServerClientClass SmartHomeServerClient;


#endif