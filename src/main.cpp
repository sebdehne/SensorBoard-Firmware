#include <Arduino.h>

#include "config.h"
#include "RN2483.h"
#include "logger.h"
#include <Wire.h>
#include "DS3231.h"
#include "ChipCap2.h"
#include "utils.h"

void blink(int times, int delayMS);
void ledOff();
void ledOn();
char buf[100];
void handleCmd(const char cmd[]);
void loop_receiver();
void loop_sender();
void loop_i2c();
bool waitingForAlarm = false;

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  blink(2, 250);

  // setup Serial
  Serial.begin(115200);
  while (!Serial)
  {
    ;
  }
  Serial.println("OK");

  RN2483.setup();

  analogReadResolution(12);

  Wire.begin(); // used by DS3231 & ChipCap2

  while (!DS3231.hasTime())
  {
    if (!DS3231.setTime(670238823)) // TODO get from server
    {
      Serial.println("Could not set time");
      delay(1000);
    }
  }

  Log.log("Setup done!");
}

void loop()
{
  // 0) measure temp
  TempAndHumidity tempAndHumidity = ChipCap2.read();
  Serial.print("Temp: ");
  Serial.println(tempAndHumidity.temp);
  Serial.print("Humidity: ");
  Serial.println(tempAndHumidity.humidity);

  // measure V_battery
  unsigned long adcBattery = analogRead(PIN_A3);

  // measure V_light sensor
  unsigned long adcLight = analogRead(PIN_A2);

  // config radio:
  handleCmd("mac pause");
  handleCmd("radio set pwr -3");

  // send data over radio
  uint8_t ping_data[2];
  ping_data[0] = 1;
  ping_data[1] = 2;

  if (!RN2483.transmitMessage(0, 2, ping_data))
    Log.log("Could not send ping");

  uint8_t receiveBuf[255];
  int bytesReceived = RN2483.receiveMessage(receiveBuf, sizeof(receiveBuf), 1000);
  if (bytesReceived > 0)
  {
    int type = receiveBuf[2];
    int payloadLen = toUInt(receiveBuf, 3);
    if (type != 1)
    {
      Log.log("Invalid response type");
    }
    else
    {
      if (receiveBuf[7] == 1 && receiveBuf[8] == 2)
      {
        Log.log("Pong response valid");
      }
      else
      {
        Log.log("Pong response invalid");
      }
    }
  }

  //char buf[100];
  //snprintf(buf, 100, "radio tx %08lx%08lx%08lx%08lx", tempAndHumidity.humidity, tempAndHumidity.temp, adcLight, adcBattery);
  //handleCmd(buf);
  // wait for ack, otherwise retry after random delay
  // but give up after X seconds

  // if timestamp in ack is too different, adjust clock

  // 2) blink LED
  blink(2, 500);

  // 3) set alarm (=> cuts the power)
  if (!DS3231.setAlarm1(20))
  {
    Serial.println("Could not set alarm");
  }

#ifdef DO_NOT_SLEEP
  delay(10000);
#endif
#ifndef DO_NOT_SLEEP
  ledOn();
  while (1)
    ;
#endif
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
