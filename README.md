# Environment Sensor 2.0 - Firmware

Arduino based firmware for my [Environment Sensor 2.0](https://github.com/sebdehne/SensorBoard-Hardware)

Features:
- Measure temperature, relativ humidity, light and battery voltage
- Send measurements to server via RF, using [RN2483](https://www.microchip.com/wwwproducts/en/RN2483) LoRa wireless module
- Support for Over-the-air (OTA) firmware updates over RF
- [All communication over LoRa secured by AES-256 encryption](https://dehnes.com/software/2021/04/18/secure-wireless-communication-for-iot-devices.html)
- Ultra low power sleep (~0.9µA) by using external RTC [DS3231SN#](https://no.mouser.com/ProductDetail/Maxim-Integrated/DS3231SN?qs=1eQvB6Dk1vhUlr8%2FOrV0Fw%3D%3D) to disable power

Dependencies:
- [ArduinoOTA](https://github.com/jandrassy/ArduinoOTA) - for writing the firmware to flash
- [CRC32](https://github.com/bakercp/CRC32)
- [rweather/Crypto](https://github.com/rweather/arduinolibs)
