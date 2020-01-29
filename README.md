# Xbox EEPROM Utility
Original Xbox EEPROM Utility to dump, change and edit your eeprom based on an ESP8266.
It is expandable to do some SMC related stuff too, like LED control of your console or even reboot/shutdown remotelly

# How to use it?
* Open with the Arduino IDE with installed esp8266 support.
* Compile and flash it to your esp8266 board.
* Upload the SPIFFS using "Tools -> ESP8266 Sketch Data Upload".
* Connect some wires to the Xbox Mainboard (SDA, SCL, GND).
* Turn console on and reset your esp.
* Search for a new WiFi network and connect to xbxdmp.
* Configure your WiFi credentials from your Home WiFi.
* Reconnect to your main home network and open http://xbxdmp.local/.
* Win!

# Bugs?
Sometimes the ESP8266 doesn't find the EEPROM from your Xbox. Maybe a bug in the esp Wire Library.
When this happens try to reset your esp8266. Two blinking lights on the esp8266 means that it found your Xbox correctly.

# ESP-01 support
For the guys that need that thing more often, can order an adapter board for the ESP-01.
https://oshpark.com/shared_projects/FJLqjN4W

# History
* v0.5 - New pretty website + new name
* v0.4 - (unreleased)Added the ability to talk to the SMC
* v0.3 - Added "on the fly" HDD key changer
* v0.2 - Added ESP-01 functionality
* v0.1 - Initial Release
