#include <Arduino.h>

#include "config.h"
#include "RN2483.h"
#include "logger.h"
#include <Wire.h>
#include "DS3231.h"

void blink(int times, int delayMS);
void ledOff();
void ledOn();
char buf[100];
void handleCmd(const char cmd[]);
void loop_receiver();
void loop_sender();
void loop_i2c();

void setup()
{
  // takes 2000ms to come here after power up, due to bootloader

  // setup Serial
  Serial.begin(115200);
  if (!Serial)
    ;

  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  RN2483.setup();

  DS3231.setup();

  Serial.println("Setup done!");
}

void loop()
{

#ifdef RECEIVER
  loop_receiver();
#endif
#ifdef SENDER
  loop_sender();
#endif
#ifdef I2C
  loop_i2c();
#endif
}

void loop_i2c()
{
  bool hasTime = DS3231.hasTime();
  Serial.print("Has time: ");
  Serial.println(hasTime);
  if (!hasTime)
  {
    Serial.println("Setting time...");
    DS3231.setTime(50, 59, 23, 0, 28, 2, 1);
    Serial.println("Setting time...Done");
  }

  DS3231.readTime();
  Serial.print("Time: 20");
  Serial.print(DS3231.getYear());
  Serial.print("-");
  Serial.print(DS3231.getMonth());
  Serial.print("-");
  Serial.print(DS3231.getDate());
  Serial.print("T");
  Serial.print(DS3231.getHour());
  Serial.print(":");
  Serial.print(DS3231.getMinutes());
  Serial.print(":");
  Serial.print(DS3231.getSeconds());
  Serial.println();

  Serial.print("Timestamp: ");
  Serial.println(DS3231.getSecondsSince2000());

  delay(1000);
}

void loop_receiver()
{
  handleCmd("radio rx 2000");
  RN2483.readResponse(buf, 100);
  Serial.println(buf);
}

void loop_sender()
{
  handleCmd("radio tx 01");
  RN2483.readResponse(buf, 100);
  Serial.println(buf);
  delay(1000);
  handleCmd("radio tx 00");
  RN2483.readResponse(buf, 100);
  Serial.println(buf);
  delay(1000);
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
