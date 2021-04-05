#include <Arduino.h>

#include "config.h"
#include "RN2483.h"
#include "logger.h"
#include <Wire.h>
#include "DS3231.h"
#include "ChipCap2.h"

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
  // takes 2000ms to come here after power up, due to bootloader

  // setup Serial
  Serial.begin(115200);

  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  RN2483.setup();

  analogReadResolution(12);

  Wire.begin(); // used by DS3231 & ChipCap2

  while (!DS3231.hasTime())
  {
    if (!DS3231.setTime(670238823))
    {
      Serial.println("Could not set time");
      delay(1000);
    }
  }

  Serial.println("Setup done!");
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
  unsigned long adcBattery = analogRead(PIN_A2);

  // measure V_light sensor
  unsigned long adcLight = analogRead(PIN_A3);

  // config radio:
  handleCmd("mac pause");
  handleCmd("radio set pwr -3");

  // send data over radio
  char buf[100];
  snprintf(buf, 100, "radio tx %08lx%08lx%08lx%08lx", tempAndHumidity.humidity, tempAndHumidity.temp, adcLight, adcBattery);
  handleCmd(buf);
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

  ledOn();
  while (1)
    ;
}

void loop_receiver()
{
  handleCmd("radio rx 2000");
  RN2483.readResponse(buf, 100);
  Serial.println(buf);
}

void handleCmd(const char cmd[])
{
  Serial.print(cmd);
  Serial.print(": ");
  RN2483.sendCommandRaw(cmd, buf, 100);
  Serial.println(buf);
}

void ledOn()
{
  digitalWrite(LED_BUILTIN, LOW);
}

void ledOff()
{
  digitalWrite(LED_BUILTIN, HIGH);
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
