#include <Arduino.h>

#include "config.h"
#include "RN2483.h"
#include "logger.h"

void blink(int times, int delayMS);
void ledOff();
void ledOn();
char buf[100];
int bytesRead;
void handleCmd(const char cmd[]);
void loop_receiver();
void loop_sender();

void setup()
{
  // setup Serial
  Serial.begin(115200);

  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // setup UART to RN2483A module
  while (1)
  {
    Serial.println("Setting up communication to RN2483");
    RN2483.setup();
    bytesRead = RN2483.sendCommandRaw("sys get hweui", buf, 16 + 2);
    if (bytesRead > 0)
    {
      break;
    }
  }
  handleCmd("mac pause");

  Serial.println("Setup done!");
}

void loop()
{

#ifdef RECEIVER
  loop_receiver();
#endif
#ifndef RECEIVER
  loop_sender();
#endif
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
