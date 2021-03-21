#include <Arduino.h>

#include "config.h"

uint8_t RTS = 8;  // REQUEST TO SEND
uint8_t CTS = 9;  // CLEAR TO SEND

void blink(int times, int delayMS);

void setup()
{
  // setup Serial
  Serial.begin(115200);
#ifdef DEBUG
  while (!Serial)
    ;
#endif

  // setup UART to RN2483A module
  Serial1.setTimeout(200);
  Serial1.begin(57600);
  pinMode(8, RTS); // D8 = UART-RTS
  pinMode(9, CTS); // D9 = UART-CTS
  digitalWrite(CTS, LOW); // do not send now
  delay(200); // TODO needed?
  
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Hello world!");

  // try to get version from the module
  digitalWrite(CTS, HIGH);
  Serial1.println("sys get ver");
  Serial1.flush();
  while(Serial1.available() == 0)
  ;
  char buf[100];
  size_t read = Serial1.readBytes(buf, 100);
  digitalWrite(CTS, LOW);
  Serial.println(read);

  Serial.println("Setup done!");
}

void loop()
{
  blink(2, 500);
  delay(1000);
}

void blink(int times, int delayMS)
{
  while (times-- > 0)
  {
    digitalWrite(LED_BUILTIN, 1);
    delay(delayMS);
    digitalWrite(LED_BUILTIN, 0);
    delay(delayMS);
  }
}