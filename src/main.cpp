#include <Arduino.h>

#include "config.h"
#include "RN2483.h"
#include "logger.h"
#include <Wire.h>
#include "DS3231.h"
#include "ChipCap2.h"
#include "utils.h"
#include "SmartHomeServerClient.h"
#include "flash.h"

void blink(int times, int delayMS);
void ledOff();
void ledOn();
char buf[100];
unsigned long sleepTimeInSeconds = 20;
bool initialSetup = false;

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  analogReadResolution(12);

  // init random seed
  randomSeed(analogRead(PIN_A0));

#ifdef DEBUG
  // setup Serial
  Serial.begin(115200);
  while (!Serial)
  {
    ;
  }
  Serial.println("OK");
#endif

  FlashUtils.init();

  RN2483.setup();
  // config radio:
  RN2483.sendCommandRaw("mac pause", buf, sizeof(buf));
  RN2483.sendCommandRaw("radio set pwr -3", buf, sizeof(buf));
  RN2483.sendCommandRaw("radio set sf sf7", buf, sizeof(buf));

  Wire.begin(); // used by DS3231 & ChipCap2

  while (!DS3231.hasTime() || FlashUtils.readUserdata(0) == 255)
  {
    initialSetup = true;
    Log.log("Sending ping to request time from server");
    // need to (re-)initialize
    if (SmartHomeServerClient.ping())
    {
      InboundPacketHeader inboundPacketHeader = SmartHomeServerClient.receivePong();
      if (!inboundPacketHeader.receiveError)
      {
        FlashUtils.prepareWritingUserdata();
        FlashUtils.write(inboundPacketHeader.to);
        FlashUtils.finishWriting();

        char buf[100];
        snprintf(buf, 100, "Got time: %lu", inboundPacketHeader.timestamp);
        Log.log(buf);
        if (!DS3231.setTime(inboundPacketHeader.timestamp))
        {
          Log.log("Could not set time");
          delay(10000);
        }
        else
        {
          blink(4, 250); // initial setup done
        }
      }
    }
    else
    {
      Log.log("Could not send ping");
      delay(1000);
    }
  }

  // set LoRa addr
  SmartHomeServerClient.setLoraAddr(FlashUtils.readUserdata(0));

  Log.log("Setup done!");
}

void loop()
{
  // 0) measure temp
  TempAndHumidity tempAndHumidity = ChipCap2.read();

  // measure V_battery
  unsigned long adcBattery = analogRead(PIN_A3);

  // measure V_light sensor
  unsigned long adcLight = analogRead(PIN_A2);

  // communicate with server
  for (int i = 0; i < LORA_RETRY_COUNT; i++)
  {
    bool sent = SmartHomeServerClient.sendSensorData(
        tempAndHumidity.temp,
        tempAndHumidity.humidity,
        adcBattery,
        adcLight,
        sleepTimeInSeconds,
        FIRMWARE_VERSION);
    if (sent)
    {
      SensorDataResponse sensorDataResponse = SmartHomeServerClient.receiveSensorDataResponse();
      if (!sensorDataResponse.receiveError)
      {
        if (sensorDataResponse.sleepTimeInSeconds > 0)
        {
          Log.log("Adjusting sleepTimeInSeconds now");
          sleepTimeInSeconds = sensorDataResponse.sleepTimeInSeconds;
        }
        if (sensorDataResponse.timeAdjustmentRequired)
        {
          Log.log("Adjusting time now");
          DS3231.setTime(sensorDataResponse.timestamp);
        }
        if (sensorDataResponse.firmwareUpdateRequired)
        {
          SmartHomeServerClient.upgradeFirmware();
        }

        break;
      }
    }

    delay(LORA_RETRY_DELAY);
  }

  // 2) set alarm (=> cuts the power)
  if (!DS3231.setAlarm1(sleepTimeInSeconds))
  {
    Log.log("Could not set alarm, resetting in 10 seconds...");
    // well, then we just sleep with power on...
  }

#ifndef DEBUG
  if (initialSetup)
  {
    ledOn();
  }
#endif
  delay(sleepTimeInSeconds * 1000);
  NVIC_SystemReset();
}

void ledOn()
{
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff()
{
  digitalWrite(LED_BUILTIN, LOW);
}

void blink(int times, int delayMS)
{
  while (times-- > 0)
  {
    ledOn();
    delay(delayMS);
    ledOff();
    delay(delayMS);
  }
}
