#ifdef ESP32

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <WiFiScan.h>
#include <ETH.h>
#include <WiFiClient.h>
#include <WiFiSTA.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiGeneric.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

#else

#include <ESP8266mDNS.h>
#include <WiFiServerSecure.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266NetBIOS.h>

#endif

#ifndef ESP32
#include <flash_hal.h>
#endif

#include <WiFiManager.h>
#include <Wire.h>

#include <EEPROM.h>
#include <FS.h>

#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "sha1.hpp"
#include "xbox.hpp"
#include "flash.h"

static const char PAYLOAD_VERSION[] = "V0.7.2";

#ifdef ESP32
WebServer server(80);
#else
ESP8266WebServer server(80);
#endif

/* Uncomment this when using an ESP-01.  */
//#define ESP01 1
/* Uncomment when you want to test it without an Xbox */
//#define TESTMODE 1
/* Uncomment this when you power your ESP32 or ESP8266 board from your Xbox (added delay) */
//#define POWERFROMXBOX 1
/* Uncomment this when you installed the wires for XWiFi */
#define XWIFI 1

#ifdef XWIFI
#ifdef ESP01
#error Can't use XWiFi with a low pin count ESP device
#endif
#ifdef ESP32
#error XWiFi isn't tested on the ESP32
#endif
#ifdef TESTMODE
#error Can't test XEU with XWiFi in Testmode
#endif
#endif

/* i2c pins depending on your ESP32 or ESP8266 */
#ifdef ESP01
#define sda 2
#define scl 0
#elif ESP32
#define sda 21
#define scl 22
#else
#define sda 4
#define scl 5
#endif

#define eep_addr 0x54
#define smc_addr 0x10
int found_eeprom = 0;
int found_smc = 0;
int g_spiffs_not_flashed = 0;

#define REG_PWR_CTRL 0x02
#define REG_LED_MODE 0x07
#define REG_LED_SEQ 0x08
#define REG_SCRATCH 0x1B

#define PWR_REBOOT 0x01
#define LED_MODE_SEQ 0x01
#define LED_ORANGE 0xFF
#define SKIP_ANIMATION 0x04
#define RUN_DASH 0x08

#ifdef XWIFI

#define PWR_BTN 0
#define EJB_BTN 2
#define LED_RED 14
#define LED_GRN 12
#define ISPWR 13

typedef struct {
  bool standby_light;
  int standby_red_val;
  int standby_green_val;
  bool boot_led_fade;
} Configuration;

Configuration conf_;
bool resetFlag = false;
#endif

int xversion = -1;
XboxCrypto::eepromdata curSetting;

byte xReadEEPROM(int i2c, unsigned int addr) {
  byte r = 0xFF;
  Wire.beginTransmission(i2c);
  Wire.write((int)(addr & 0xFF));
  Wire.endTransmission();

  Wire.requestFrom(i2c, 1);

  if (Wire.available()) r = Wire.read();

  return r;
}

void xWriteEEPROM(int i2c, unsigned int addr, byte data) {
  Wire.beginTransmission(i2c);
  Wire.write((int)(addr & 0xFF));
  Wire.write(data);
  Wire.endTransmission();

  delay(5); // Give the EEPROM some time to store it.
}

class XboxEeprom: public Stream
{
  protected:
    size_t eepromsize;
    int i = 0;
#ifdef TESTMODE
    bool hasWritten = false;
#endif

  public:
    XboxEeprom (size_t size): eepromsize(size) { }

    virtual int available() {
      return eepromsize;
    }

    virtual int read() {
      eepromsize--;
      i++;
#ifdef TESTMODE
      return EEPROM.read(i - 1);
#else
      return xReadEEPROM(eep_addr, i - 1);
#endif
    }

    virtual int read(uint8_t *buffer, size_t size) {
      for (size_t u = 0; u < size; u++, i++, eepromsize--) {
#ifdef TESTMODE
        buffer[u] = EEPROM.read(i - 1);
#else
        buffer[u] = xReadEEPROM(eep_addr, i - 1);
#endif
      }
    }
    virtual int peek() {
#ifdef TESTMODE
      return EEPROM.read(i - 1);
#else
      return xReadEEPROM(eep_addr, i - 1);
#endif
    }
    virtual void flush() {
      eepromsize = 0;
    }
    virtual size_t write (uint8_t *buffer, size_t size) {
      if (size > 0x100) {
        return 0;
      }

      for (int i = 0; i < 0x100; i++) {
#ifdef TESTMODE
        EEPROM.write(i, buffer[i]);
#else
        xWriteEEPROM(eep_addr, i, buffer[i]);
#endif
      }
#ifdef TESTMODE
      hasWritten = true;
#endif
      return size;
    }
    virtual size_t write (uint8_t data) {
      return 0;
    }
    size_t size() {
      return eepromsize;
    }
    const char *name() {
      return "eeprom.bin";
    }
    virtual void close() {
      i = 0;
#ifdef TESTMODE
      if (hasWritten) {
        EEPROM.commit();
      }
#endif
    }
};

void readEEPROMFromXbox(byte *ram) {
  int i;
  XboxEeprom eeprom(0x100);
  for (i = 0; i < 0x100; i++) {
    ram[i] = eeprom.read();
  }
  eeprom.close();
}

void readDecrEEPROMFromXbox(byte *ram) {
  int i;
  XboxEeprom eeprom(0x100);
  for (i = 0; i < 0x100; i++) {
    ram[i] = eeprom.read();
  }
  eeprom.close();

  XboxCrypto *xbx = new XboxCrypto();
  xbx->decrypt(ram);

  delete []xbx;
}

void writeEEPROMToXbox(byte *ram) {
  XboxEeprom eeprom(0x100);
  eeprom.write(ram, 0x100);
  eeprom.close();
}

void writeDecrEEPROMToXbox(byte *ram) {
  unsigned char tmp[48];
  XboxEeprom eeprom(0x100);
  eeprom.read(tmp, 48);
  eeprom.close();
  XboxCrypto *xbx = new XboxCrypto();
  if (xbx->decrypt(tmp) > 0) {
    if (xbx->encrypt(ram) == 0) {
      eeprom.write(ram, 0x100);
      eeprom.close();
    }
  }
  delete []xbx;
}

class DecrEeprom : public Stream
{
  protected:
    size_t eepromsize;
    int i = 0;
    bool hasWritten = false;
    uint8_t dram[0x100];
  public:
    DecrEeprom (size_t size): eepromsize(size) {
      readDecrEEPROMFromXbox(dram);
    }

    virtual int available() {
      return eepromsize;
    }
    virtual int read() {
      eepromsize--;
      i++;
      return dram[i - 1];

    }
    virtual int peek() {
      return dram[i - 1];
    }

    virtual void flush() {
      eepromsize = 0;
    }

    virtual size_t write(uint8_t *buffer, size_t size) {
      hasWritten = true;
      memcpy(dram, buffer, size);
    }

    virtual size_t write(uint8_t data) {
      return 0;
    }

    size_t size() {
      return eepromsize;
    }

    const char *name() {
      return "decr_eeprom.bin";
    }

    virtual void close() {
      i = 0;

      if (hasWritten) {
        writeDecrEEPROMToXbox(dram);
      }

      hasWritten = false;

      memset(dram, 0, 0x100);
    }
};

#ifdef XWIFI
bool isConsoleOn(void)
{
  return digitalRead(ISPWR);
}

bool toggleConsoleOnOff(void)
{
  pinMode(PWR_BTN, OUTPUT);
  digitalWrite(PWR_BTN, LOW);
  delay(120);
  digitalWrite(PWR_BTN, HIGH);
  pinMode(PWR_BTN, INPUT);
}

bool toggleEject(void)
{
  pinMode(EJB_BTN, OUTPUT);
  digitalWrite(EJB_BTN, LOW);
  delay(500);
  digitalWrite(EJB_BTN, HIGH);
  pinMode(EJB_BTN, INPUT);
}

int g_ledctrl = 1;
bool ledCtrlToggle(void) {
  if (g_ledctrl) {
    pinMode(LED_RED, INPUT);
    pinMode(LED_GRN, INPUT);
    g_ledctrl = 0;
  }
  else
  {
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    g_ledctrl = 1;
  }
}

bool ledCtrlRedGreen(int red, int green) {
  if (g_ledctrl) {
    ledCtrlToggle();
  }
  if (red == 1023) {
    digitalWrite(LED_RED, LOW);
  }
  else if (green == 1023) {
    digitalWrite(LED_GRN, LOW);
  }

  if (red >= 0) analogWrite(LED_RED, red);
  if (green >= 0) analogWrite(LED_GRN, green);
  if ((red == 0) && (green == 0)) {
    ledCtrlToggle();
  }
}

bool ledCtrlStandby(bool state) {
  if (state) {
    ledCtrlRedGreen(conf_.standby_red_val, conf_.standby_green_val);
  }
  else if (state == false) {
    ledCtrlRedGreen(0, 0);
  }
}

bool ledCtrlFade(bool green, bool red, int duration) {
  int delay_time = (duration / 50) / 2;
  for (int i = 0, o = 0; o < 2;) {
    ledCtrlRedGreen(green ? i : 0, red ? i : 0);
    delay(delay_time);
    if (i == 1000) o++;
    if (o == 0) i += 50;
    else if (o == 1) i -= 50;
    if (i == 0 && o == 1) o++;
  }
  ledCtrlRedGreen(0, 0);
}

#endif

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".bin")) return "application/octet-stream";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  if (path.endsWith("decr_eeprom.bin")) {
#ifdef XWIFI
    int wasOff = 0;
    if (!isConsoleOn()) {
      toggleConsoleOnOff();
      wasOff = 1;
      delay(1000);
    }
#endif
    DecrEeprom deeprom(0x100);
    size_t sizeSent = server.streamFile(deeprom, contentType);
    deeprom.close();
#ifdef XWIFI
    if (wasOff) {
      toggleConsoleOnOff();
      wasOff = 0;
    }
#endif
    return true;
  }
  if (path == "/eeprom.bin") {
#ifdef XWIFI
    int wasOff = 0;
    if (!isConsoleOn()) {
      toggleConsoleOnOff();
      wasOff = 1;
      delay(1000);
    }
#endif
    XboxEeprom eeprom(0x100);
    size_t sizeSent = server.streamFile(eeprom, contentType);
    eeprom.close();
#ifdef XWIFI
    if (wasOff) {
      toggleConsoleOnOff();
      wasOff = 0;
    }
#endif
    return true;
  }
  /* This is the flashing interface when no SPIFFS is flashed. So you can flash the interface without any Serial cable */
  if (g_spiffs_not_flashed) {
    char file[sizeof(flash)];
    memcpy_P(file, flash, sizeof(flash));
#ifdef ESP32
    server.send(200, "text/html", file);
#else
    server.send(200, "text/html", file, sizeof(flash));
#endif
    return true;
  }
  if (SPIFFS.exists(path)) {
#ifdef XWIFI
    if (path.endsWith("config.json")) return false; /* hide config file from accessing it. Access it through http://xeu.local/config */
    if (path.endsWith("index2.html")) return false; /* redirect to a XWiFi compatible interface */
    if (path.endsWith("index.html")) {
      path = "/index2.html";
    }
#endif
#ifndef XWIFI
    if (path.endsWith("index2.html")) return false;  /* hide XWiFi compatible interface from the normal XEU */
#endif
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

bool IsUploadSuccess = false;
void handleFileUpload() {
  HTTPUpload &upload = server.upload();
  int wasOff = 0;
  if (upload.status == UPLOAD_FILE_START) {
    //Do nothing

    //with XWiFi turn Xbox on when off.
#ifdef XWIFI
    if (!isConsoleOn()) {
      toggleConsoleOnOff();
      wasOff = 1;
      delay(1000);
    }
#endif
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    byte ram[0x100];
    byte ram1[0x100];
    if (upload.currentSize == 0x100) {
      memcpy(ram, upload.buf, upload.currentSize);

      writeEEPROMToXbox(ram);
      delay(200);
      readEEPROMFromXbox(ram1);

      if (memcmp(ram, ram1, 0x100) != 0) {
        IsUploadSuccess = false;
      }
      else {
        IsUploadSuccess = true;
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
#ifdef XWIFI
    if (wasOff) {
      delay(1000);
      toggleConsoleOnOff();
      wasOff = 0;
    }
#endif
    if (IsUploadSuccess) {
      server.send(200, "text/html", "success");
    }
    else {
      server.send(418, "text/html", "failed");
    }
  }
}

/* handleUpdateUpload -- Update through uploading. Works with Firmware and SPIFFS*/
bool updateError = false;
void handleUpdateUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String updatefile = upload.filename;
    Serial.println("Update started:");
    if (updatefile.endsWith("spiffs.bin")) {
#ifdef ESP32
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
        Serial.println("error");
        updateError = true;
      }
#else
      String fsBlockSizeStr = String((int)&_FS_block);
      if(updatefile.indexOf(fsBlockSizeStr) < 0){
        Serial.printf("Update Error! Please download and install xeu.%s.spiffs.bin\n", fsBlockSizeStr.c_str());
        updateError = true;
        return;
      }
      size_t fsSize = ((size_t) &_FS_end - (size_t) &_FS_start);
      close_all_fs();
      if (!Update.begin(fsSize, U_FS)) {
        Serial.println("error");
        updateError = true;
      }
#endif
    }
    else {
#ifdef ESP32
      if(updatefile.endsWith("esp32.bin")){
        Serial.printf("Trying to flash %s\n", updatefile.c_str());
      }
      else{
        Serial.println(F("Error. Please flash any firmware. Make sure you use the right one."));
        updateError = true;
        return;
      }
#else
      if((updatefile.endsWith("1M.bin")) || (updatefile.endsWith("2M.bin")) || (updatefile.endsWith("4M.bin")) || (updatefile.endsWith("8M.bin"))){
        Serial.printf("Trying to flash %s\n", updatefile.c_str());
      }
      else{
        Serial.println(F("Error. Please flash any xeu.board.1M.bin to xeu.board.8M.bin. Make sure you use the right one."));
        Serial.println(F("When you don't know which one, flash any 1M binary."));
        updateError = true;
        return;
      }
#endif
      unsigned int maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace, U_FLASH)) {
        Serial.println("error");
        updateError = true;
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE && updateError == false) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println("upload error");
      updateError = true;
    }
    else {
      Serial.print(".");
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println(" Flashing successfully");
      Serial.println("Rebooting now");
      delay(500);
#ifdef ESP32
      ESP.restart();
#else
      ESP.reset();
#endif
    }
  }
}

char hex2char(char hex) {
  char ret;
  if (hex >= 0x30 && hex <= 0x39) ret = hex - 0x30;
  else if (hex >= 0x41 && hex <= 0x46) ret = hex - 0x41 + 10;
  else if (hex >= 0x61 && hex <= 0x66) ret = hex - 0x61 + 10;

  return ret;
}

//You could do here more -- https://xboxdevwiki.net/PIC
void handleSMCRequests() {
  if (server.hasArg("reset")) {
    if (found_smc) {
      /* tells the SMC to reset the xbox*/
      Wire.beginTransmission(smc_addr);
      Wire.write(REG_PWR_CTRL);
      Wire.write(PWR_REBOOT);
      Wire.endTransmission();
    }
    server.send(200, "text/html", "OK");
  }
}
#ifdef XWIFI
void handleXWiFi() {
  if (server.hasArg("ison")) {
    if (isConsoleOn()) {
      server.send(200, "text/html", "YES");
    }
    else {
      server.send(200, "text/html", "NO");
    }
    return;
  }

  if (server.hasArg("pwr")) {
    toggleConsoleOnOff();
  }

  if (server.hasArg("eject")) {
    toggleEject();
  }

  if (server.hasArg("ledg")) {
    int pwm = server.arg("ledg").toInt();
    ledCtrlRedGreen(-1, pwm);
  }
  if (server.hasArg("ledr")) {
    int pwm = server.arg("ledr").toInt();
    ledCtrlRedGreen(pwm, -1);
  }
  if (server.hasArg("bakeep")) {
    int wasOff = 0;
    if (!isConsoleOn()) {
      toggleConsoleOnOff();
      wasOff = 1;
      delay(1000);
    }

    byte eeprom[0x100];
    readEEPROMFromXbox(eeprom);

    if (!SPIFFS.exists("/bak_eeprom.bin")) {
      File bak = SPIFFS.open("/bak_eeprom.bin", "w");
      if (!bak) {
        server.send(200, "text/html", "FAILED");
        return;
      }
      bak.write(eeprom, 0x100);
      bak.close();
    }
    else {
      server.send(200, "text/html", "FAILED");
    }

    if (wasOff) {
      delay(1000);
      toggleConsoleOnOff();
      wasOff = 0;
    }
  }

  if (server.hasArg("resteep")) {
    int wasOff = 0;
    if (!isConsoleOn()) {
      toggleConsoleOnOff();
      wasOff = 1;
      delay(1000);
    }

    byte eeprom[0x100];
    if (SPIFFS.exists("/bak_eeprom.bin")) {
      File bak = SPIFFS.open("/bak_eeprom.bin", "r");
      if (!bak) {
        server.send(200, "text/html", "FAILED");
        return;
      }
      bak.read(eeprom, 0x100);
      bak.close();
      writeEEPROMToXbox(eeprom);
    }
    else {
      server.send(200, "text/html", "FAILED");
      return;
    }

    if (wasOff) {
      delay(1000);
      toggleConsoleOnOff();
      wasOff = 0;
    }
  }

  server.send(200, "text/html", "OK");
}

void saveConfiguration() {
  StaticJsonDocument<256> doc;
  doc["standby_red_val"] = conf_.standby_red_val;
  doc["standby_light"] = conf_.standby_light ? 1 : 0;
  doc["standby_green_val"] = conf_.standby_green_val;
  doc["boot_led_fade"] = conf_.boot_led_fade ? 1 : 0;

  char *tmp = (char*)malloc(0x100);
  memset(tmp, 0, 0x100);
  serializeJson(doc, tmp, 0x100);

  File conf = SPIFFS.open("/config.json", "w");
  conf.printf(tmp);
  conf.close();
  free(tmp);
}

void loadConfiguration() {
  StaticJsonDocument<256> doc;
  byte *tmp = (uint8*)malloc(0x100);
  File conf = SPIFFS.open("/config.json", "r");
  conf.read(tmp, 0x100);
  conf.close();
  deserializeJson(doc, tmp);
  conf_.standby_light = doc["standby_light"] ? true : false;
  conf_.standby_red_val = doc["standby_red_val"];
  conf_.standby_green_val = doc["standby_green_val"];
  conf_.boot_led_fade = doc["boot_led_fade"] ? true : false;
  free(tmp);
}

void handleConfig() {
  int saveConfig = 0;

  if (server.hasArg("reset")) {
    server.send(200, "text/html", "OK");
    resetFlag = true;
    return;
  }

  if (server.hasArg("delBackup")) {
    SPIFFS.remove("/bak_eeprom.bin");
  }

  if (server.hasArg("factory")) {
    SPIFFS.remove("/config.json");
    server.send(200, "text/html", "OK");
    resetFlag = true;
    return;
  }

  if (server.hasArg("standby_light")) {
    if ((server.arg("standby_light") == "0") || (server.arg("standby_light") == "1")) {
      String val = server.arg("standby_light");
      conf_.standby_light = val.toInt() ? true : false;
    }
  }

  if (server.hasArg("standby_red_val")) {
    String val = server.arg("standby_red_val");
    if (val != "") {
      conf_.standby_red_val = val.toInt();
    }
  }

  if (server.hasArg("standby_green_val")) {
    String val = server.arg("standby_green_val");
    if (val != "") {
      conf_.standby_green_val = val.toInt();
    }
  }

  if (server.hasArg("boot_led_fade")) {
    String val = server.arg("boot_led_fade");
    if (val == "0" || val == "1") {
      conf_.boot_led_fade = val.toInt() ? true : false;
    }
  }

  if (server.hasArg("save_config")) {
    saveConfiguration();
  }

  if (server.hasArg("load_config")) {
    loadConfiguration();
  }

  File conf = SPIFFS.open("/config.json", "r");
  server.streamFile(conf, "application/json");
  conf.close();
}

#endif

void handlePROGMEMUpdate() {
  char file[sizeof(flash)];
  memcpy_P(file, flash, sizeof(flash));
#ifdef ESP32
  server.send(200, "text/html", file);
#else
  server.send(200, "text/html", file, sizeof(flash));
#endif
}

void handleHDDKeyUpdate() {
  StaticJsonDocument<256> root;
  unsigned char ram[0x100];
  readEEPROMFromXbox(ram);

  /* Serial */
  char SerialNumber[12];
  memcpy(SerialNumber, ram + 0x34, 12);
  SerialNumber[12] = '\0';
  root["serial"] = SerialNumber;

  /* MAC Address */
  char mac[6];
  memcpy(mac, ram + 0x40, 6);
  mac[6] = '\0';
  char macaddrStr[18];
  sprintf(macaddrStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  root["macaddr"] = macaddrStr;

  /* HDD Key */
  char curHDKeyStr[35];
  getHDDKey();
  for (int i = 0; i < 16; i++) {
    sprintf(&curHDKeyStr[i * 2], "%02X", curSetting.HDDKey[i]);
  }
  root["hdkey"] = curHDKeyStr;

  /* XVersion detection is available until eeprom decryption */
  Serial.print("Xbox Version: ");
  switch (xversion) {
    case VERSION_10:
      {
        root["xversion"] = "v1.0";
        Serial.println("v1.0");
        break;
      }
    case VERSION_11:
      {
        root["xversion"] = "v1.1 - v1.5";
        Serial.println("v1.1 - v1.5");
        break;
      }
    case VERSION_16:
      {
        root["xversion"] = "v1.6 / v1.6b";
        Serial.println("v1.6 / v1.6b");
        break;
      }

    case VERSION_DBG:
      {
        root["xversion"] = "DEBUG KIT";
        Serial.println("DEBUG KIT");
        break;
      }

    default:
      {
        root["xversion"] = "UNDEFINED";
        Serial.println("UNDEFINED");
        break;
      }
  }
  Serial.print("SN: ");
  Serial.println(SerialNumber);
  Serial.print("MAC Address: ");
  Serial.println(macaddrStr);
  Serial.print("HDD Key: ");
  Serial.println(curHDKeyStr);

  if (server.hasArg("hdkey")) {
    char hdkey_str[35];
    char hdkey[16];
    String(server.arg("hdkey")).toCharArray(hdkey_str, 33);

    for (int i = 0; i < 32; i += 2) {
      char first = hex2char(hdkey_str[i]);
      char second = hex2char(hdkey_str[i + 1]);
      hdkey[i / 2] = ((first << 4) | (second & 0xF));
    }

    /* This is for checks*/
    memset(hdkey_str, 0, 35);
    char key_str[2];
    for (int i = 0; i < 16; i++) {
      sprintf(&hdkey_str[i * 2], "%02X", hdkey[i]);
    }

    setHDDKey((unsigned char *)hdkey);
    root["hdkey"] = hdkey_str;
  }

  char *buf = (char *) malloc(256);
  serializeJson(root, buf, 256);

  server.send(200, "application/json", buf);
  free(buf); //forgot that -- so it cost me nerves debugging this cuz running out of heap. ups...
}

void setHDDKey(unsigned char *hdkey) {
  unsigned char d[256];
  readEEPROMFromXbox(d);

  XboxCrypto::eepromdata data;

  XboxCrypto *xbx = new XboxCrypto();
  if (xbx->decrypt(d) > 0) {
    memcpy(&data, d, sizeof(XboxCrypto::eepromdata));
    memcpy(data.HDDKey, hdkey, 16);
    memcpy(d, &data, sizeof(XboxCrypto::eepromdata));
    if (xbx->encrypt(d) == 0) {
      writeEEPROMToXbox(d);
    }
  }
  delete []xbx;
}

void getHDDKey(void) {
  unsigned char d[256];
  readEEPROMFromXbox(d);
  XboxCrypto *xbx = new XboxCrypto();
  if (xbx->decrypt(d) > 0) {
    memcpy(&curSetting, d, sizeof(XboxCrypto::eepromdata));
    xversion = xbx->getVersion();
  }
  delete []xbx;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
#ifdef XWIFI
  Serial.printf("Starting payload version: %s with XWiFi\n", PAYLOAD_VERSION);
#else
  Serial.printf("Starting payload version: %s\n", PAYLOAD_VERSION);
#endif
  SPIFFS.begin();
  if (!(SPIFFS.exists("/index.html"))) {
    Serial.println("SPIFFS is not flashed. Formating it..");
    SPIFFS.format();
    g_spiffs_not_flashed = 1;
    Serial.println("Done.");

  }
#ifdef XWIFI
  StaticJsonDocument<256> cnf;

  if (!(SPIFFS.exists("/config.json"))) {
    cnf["standby_light"] = 0;
    cnf["standby_red_val"] = 100;
    cnf["standby_green_val"] = 100;
    cnf["boot_led_fade"] = 1;

    char *tmp = (char*)malloc(0x100);
    serializeJson(cnf, tmp, 0x100);

    File conf = SPIFFS.open("/config.json", "w");
    conf.write(tmp, strlen(tmp));
    conf.close();
    free(tmp);
  }

  loadConfiguration();
  if (conf_.boot_led_fade) {
    ledCtrlFade(false, true, 2500);
  }

#endif

#ifndef ESP01
#ifndef ESP32
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#endif
#endif

  /* This waits 3 second, so the Xbox can boot up. (Or up and down 3x when eeprom bricked)*/
#ifdef POWERFROMXBOX
#ifndef XWIFI
  delay(3000);
#endif
#endif

#ifdef TESTMODE
  EEPROM.begin(0x100);
  found_eeprom = 1;
  found_smc = 0;
#else
  Wire.begin(sda, scl);
#ifndef XWIFI
  Wire.beginTransmission(eep_addr);
  if (Wire.endTransmission() == 0) {
    found_eeprom = 1;
  }

  Wire.beginTransmission(smc_addr);
  if (Wire.endTransmission() == 0) {
    found_smc = 1;
  }
#endif
#endif

  /* better way to say the user the eeprom/smc was found (especially on ESP-01) */
  if (found_smc) {
    /* Give the Xbox some time */
#ifndef POWERFROMXBOX
    delay(1000);
#endif

#ifdef ESP01
    Wire.beginTransmission(smc_addr);
    Wire.write(REG_LED_SEQ);
    Wire.write(LED_ORANGE); // solid orange
    Wire.endTransmission();

    Wire.beginTransmission(smc_addr);
    Wire.write(REG_LED_MODE);
    Wire.write(LED_MODE_SEQ); // changes to custom lights
    Wire.endTransmission();
#endif

  }
  /*
     For configuring WiFi. Makes an Access Point
     when no config is saved in flash.
     Else it connects to the Access Point you provided.
  */
  WiFiManager wifiManager;
#ifdef ESP32
  wifiManager.setHostname("XEU-ESP32");
#else
  wifiManager.setHostname("XEU-ESP8266");
#endif
  wifiManager.autoConnect("xeu");

  MDNS.begin("xeu");
  MDNS.addService("http", "tcp", 80);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("xeu");
  ArduinoOTA.begin();

#ifdef XWIFI
  if (!isConsoleOn()) {
    found_eeprom = 1; // Check if console isn't on.
    found_smc = 1;  //  And set the flags
  }
#endif
#ifndef XWIFI
  if (found_eeprom) {
#endif
    /* For Website from SPIFFS */
    server.onNotFound([]() {
      if (!handleFileRead(server.uri()))
        server.send(404, "text/plain", "404: Not Found");
    });
    /* For EEPROM flashing */
    server.on("/", HTTP_POST,
    []() {
      server.send(200);
    },
    handleFileUpload
             );
    /* For Web Update */
    server.on("/update", HTTP_POST,
    []() {
      server.send(200);
    },
    handleUpdateUpload
             );
    server.on("/update", HTTP_GET, handlePROGMEMUpdate);

    /* for updating HDD key and console information */
    server.on("/upd", HTTP_GET, handleHDDKeyUpdate);
    /* for sending commands to the SMC */
    server.on("/smc", HTTP_GET, handleSMCRequests);
#ifdef XWIFI
    /* XWiFi commands */
    server.on("/xwifi", HTTP_GET, handleXWiFi);
    /* XWiFi configuration */
    server.on("/config", HTTP_GET, handleConfig);
#endif
#ifndef XWIFI
  }
  else {
    server.onNotFound([]() {
      server.send(404, "text/plain", "404: No EEPROM Found");
    });
  }
#endif
  server.begin();
}

int retStatus = -1;

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  ArduinoOTA.handle();
#ifdef XWIFI
  if (resetFlag) {
#ifdef ESP32
    ESP.restart();
#else
    ESP.reset();
#endif
  }
  if (conf_.standby_light) {
    if (!isConsoleOn()) {
      ledCtrlStandby(true);
    }
    else if (isConsoleOn()) {
      ledCtrlStandby(false);
      ledCtrlToggle();
    }
  }
  else {
    ledCtrlStandby(false);
    ledCtrlToggle();
  }


#endif
}
