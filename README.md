# Xbox EEPROM Utility
OG Xbox EEPROM Utility to dump, change and edit your eeprom based on an ESP32/ESP8266.
It is expandable to do some SMC related stuff too, like LED control of your console or even reboot/shutdown remotelly

# How to use it?
## Using precompiled binary
You can use the precompiled binary that fits ESP-01 with 1MB SPI flash.\
You also can flash the binary to a D1 mini or similar.\
When using the precompiled binary connect GND, SCL(Pin 0), and SDA(Pin 2) to your Xbox.\
When you power the ESP from your console connect VCC to 3.3V on the Xbox motherboard.\
Use the esptool with following command:\
> esptool.py -p /dev/YOUR_SERIAL_IF -b 115200 write_flash 0 firmware.bin

## Compile it yourself?
* Open with the Arduino IDE with installed ESP32 or ESP8266 support, including some libraries. (WiFiManager)
* Compile and flash it to your ESP32 or ESP8266 board.
* Upload the SPIFFS using "Tools -> ESP8266 Sketch Data Upload" or on the ESP32 with "Tools -> ESP32 Sketch Data Upload".
* Connect some wires to the Xbox Mainboard. DO NOT CONNECT +5V TO +3.3V!
* WeMos D1 mini(SDA Pin D2, SCL Pin D1, GND, 3.3V), ESP01(SDA Pin 2, SCL Pin 0, GND, 3.3V), ESP32 (SDA Pin 21, SCL Pin 22, GND, 3.3V)
* Turn console on and reset your ESP.
* Search for a new WiFi network and connect to 192.168.4.1.
* Configure your WiFi credentials from your Home WiFi.
* Reconnect to your main home network and open http://xeu.local/.
* Win!

# Bugs?
Currently with the newest WiFiManager none on the ESP8266 based boards. On the ESP32 it doesn't always connect to your local WiFi!

# ESP-01 support
For the guys that need that thing more often, can order an adapter board for the ESP-01.
https://oshpark.com/shared_projects/FJLqjN4W

# History
* v0.7 - Added ESP32 support + updated C++ classes for the crypto
* v0.6 - New element to show Serial, MAC address, and HDD Key in a seperate "box".\
Introducing the unsafe "Change Mode" where you can dump your Decrypted EEPROM.
* v0.5 - New pretty website + new name
* v0.4 - (unreleased)Added the ability to talk to the SMC
* v0.3 - Added "on the fly" HDD key changer
* v0.2 - Added ESP-01 functionality
* v0.1 - Initial Release
