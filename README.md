# Xbox EEPROM Utility ![](/xbox_eeprom_utility/data/eeprom3.png?raw=true)
OG Xbox EEPROM Utility to dump, change and edit your eeprom based on an ESP32/ESP8266.
It is expandable to do some SMC related stuff too, like LED control of your console or even reboot/shutdown remotely

# XWiFi
XEU is combined with XWiFi, XWiFi is a XERC2 like option that let you turn on and off your console, ejecting the DVD drive and drive LED's in the console's off state. 
You can configure a standby led in the red/green value you wan't (PWM). XWiFi requires a pin compatible ESP chip and soldering is needed. For installation you could use any XERC2 installation diagram.
This is ONLY over WiFi. So no remote control!

# XEU Features
* Dumping and flashing EEPROM over WiFi
* Decrypting HDD Key
* Change HDD Key
* OTA Flashing (ArduinoOTA & HTTP File Upload)
* Inbuild Flashmode(Failsafe when flashing the wrong SPIFFS).

# XEU + XWiFi features
* All above.
* Turn console on/off over WiFi
* Eject DVD drive
* Adjustable PWM LED control (Standby Light)
* and more.

# How to use it?
## Using precompiled binary
In the release section you find different binaries for different boards. XEU with and without XWiFi from 1M to 4M binaries.
When using a precompiled binary check the pinout how to wire it up to your Xbox.\
When you power the ESP from your console, connect VCC to 3.3V on the Xbox motherboard.\
Use the esptool with following command:
> esptool.py -p /dev/YOUR_SERIAL_IF -b 115200 write_flash 0 xeu.esp01.1M.bin

Or the Nodemcu Flasher (Windows only):

https://github.com/nodemcu/nodemcu-flasher/raw/master/Win64/Release/ESP8266Flasher.exe

## Compile it yourself?
* Open with the Arduino IDE with installed ESP32 or ESP8266 support, including some libraries. (WiFiManager, ArduinoJson)
* Compile and flash it to your ESP32 or ESP8266 board.
* Upload the SPIFFS using "Tools -> ESP8266 Sketch Data Upload" or on the ESP32 with "Tools -> ESP32 Sketch Data Upload".
* Connect some wires to the Xbox Mainboard. DO NOT CONNECT +5V TO +3.3V!
* WeMos D1 mini(SDA Pin D2, SCL Pin D1, GND, 3.3V), ESP01(SDA Pin 2, SCL Pin 0, GND, 3.3V), ESP32 (SDA Pin 21, SCL Pin 22, GND, 3.3V)
* Turn console on and reset your ESP.
* Search for a new WiFi network and connect to 192.168.4.1.
* Configure your WiFi credentials from your Home WiFi.
* Reconnect to your main home network and open http://xeu.local/. (Bonjour installation on Windows required!)
* Win!

# Bugs?
Currently with the newest WiFiManager none on the ESP8266 based boards. On the ESP32 it doesn't always connect to your local WiFi!

# ESP-01 support
For the guys that need that thing more often, can order an adapter board for the ESP-01.
https://oshpark.com/shared_projects/FJLqjN4W

# XWiFi custom board for ESP-12F
This is a custom board featuring a ESP-12F board, you need 3x 10k 0603 resistors and a 100nF 0603 cap.
https://oshpark.com/shared_projects/aNDEvRKk
It's a 9 wire installation (Power Button, Eject Button, Power on, LED Red, LED Green, SDA, SCL, Power and Ground).
Make sure you mount it at the top side on your motherboard! (WiFi failure, SMBus interference)

# History
* v0.7.2 - Added XWiFi
* v0.7 - Added ESP32 support + updated C++ classes for the crypto
* v0.6 - New element to show Serial, MAC address, and HDD Key in a seperate "box".
Introducing the unsafe "Change Mode" where you can dump your Decrypted EEPROM.
* v0.5 - New pretty website + new name
* v0.4 - (unreleased)Added the ability to talk to the SMC
* v0.3 - Added "on the fly" HDD key changer
* v0.2 - Added ESP-01 functionality
* v0.1 - Initial Release

