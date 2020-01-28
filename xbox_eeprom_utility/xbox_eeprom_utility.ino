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
#include <WiFiManager.h>
#include <Wire.h>

#include <EEPROM.h>
#include <FS.h>

#include <ArduinoOTA.h>

static const char PAYLOAD_VERSION[] = "V0.5";

#ifdef __cplusplus
extern "C" {
#endif

#include "sha1.h"
#include "xbox.h"

#ifdef __cplusplus
}
#endif

/* Uncomment this when using an esp8266-01.  */
#define ESP01 1
/* Uncomment when you want to test it without an Xbox */
//#define TESTMODE 1

/* Uncomment this when you power your esp8266 from your Xbox (added delay) */
#define POWERFROMXBOX 1

/* i2c pins depending on your esp8266 */
#ifdef ESP01
#define sda 2
#define scl 0
#else
#define sda D2
#define scl D1
#endif

#define eep_addr 0x54
#define smc_addr 0x10
int found_eeprom = 0;
int found_smc = 0;

#define REG_PWR_CTRL 0x02
#define REG_LED_MODE 0x07
#define REG_LED_SEQ 0x08
#define REG_SCRATCH 0x1B

#define PWR_REBOOT 0x01
#define LED_MODE_SEQ 0x01
#define LED_ORANGE 0xCC
#define SKIP_ANIMATION 0x04
#define RUN_DASH 0x08

typedef struct
{
    unsigned char HMAC_SHA1_Hash[20];
    unsigned char Confounder[8];
    unsigned char HDDKey[16];
    unsigned char XBERegion[4];
} eepromdata;

int xversion = -1;
unsigned char curHDKey[16];

ESP8266WebServer server(80);

byte xReadEEPROM(int i2c, unsigned int addr){
  byte r = 0xFF;
  Wire.beginTransmission(i2c);
  Wire.write((int)(addr & 0xFF));
  Wire.endTransmission();

  Wire.requestFrom(i2c, 1);

  if(Wire.available()) r = Wire.read();

  return r;
}

void xWriteEEPROM(int i2c, unsigned int addr, byte data){
  Wire.beginTransmission(i2c);
  Wire.write((int)(addr & 0xFF));
  Wire.write(data);
  Wire.endTransmission();

  delay(5); // Give the EEPROM some time to store it.
}

void readEEPROMFromXbox(byte *ram){
  int i;
  for(i=0;i<0x100;i++){
#ifdef TESTMODE
    ram[i] = EEPROM.read(i);
#else
    ram[i] = xReadEEPROM(eep_addr, i);
#endif
  }
}

void writeEEPROMToXbox(byte *ram){
  for(int i=0;i<0x100;i++){
#ifdef TESTMODE
    EEPROM.write(i, ram[i]);
#else
    xWriteEEPROM(eep_addr, i, ram[i]);
#endif
  }
#ifdef TESTMODE
  EEPROM.commit();
#endif
}

class XboxEeprom: public Stream
{
  protected:
    size_t eepromsize;
    int i = 0;

  public:
    XboxEeprom (size_t size): eepromsize(size) { }

    virtual int available() {
      return eepromsize;
    }

    virtual int read(){
      eepromsize--;
      i++;
#ifdef TESTMODE
      return EEPROM.read(i-1);
#else
      return xReadEEPROM(eep_addr, i-1);
#endif
    }
    virtual int peek(){
#ifdef TESTMODE
      return EEPROM.read(i-1);
#else
      return xReadEEPROM(eep_addr, i-1);
#endif
    }
    virtual void flush(){
      eepromsize = 0;
    }
    virtual size_t write (uint8_t data) {
      return 0;
    }
    size_t size() {
      return eepromsize;
    }
    const char *name(){
      return "eeprom.bin";
    }
    virtual void close(){
      i=0;
    }
};

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".bin")) return "application/octet-stream";
  return "text/plain";
}

bool handleFileRead(String path){
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  if(path.endsWith("eeprom.bin")){
    XboxEeprom eeprom(0x100);
    size_t sizeSent = server.streamFile(eeprom, contentType);
    eeprom.close();
    return true;
  }
  
  if(SPIFFS.exists(path)){
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

bool IsUploadSuccess = false;
void handleFileUpload(){
  HTTPUpload &upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    //Do nothing
  }
  else if(upload.status == UPLOAD_FILE_WRITE){
    byte ram[0x100];
    byte ram1[0x100];
    if(upload.currentSize == 0x100){
      memcpy(ram, upload.buf, upload.currentSize);
      writeEEPROMToXbox(ram);
      delay(200);
      readEEPROMFromXbox(ram1);
      if(memcmp(ram, ram1, 0x100) != 0){
        IsUploadSuccess = false;
      }
      else{
        IsUploadSuccess = true;
      }
    }
  }
  else if(upload.status = UPLOAD_FILE_END){
    if(IsUploadSuccess){
      server.send(200, "text/html", "success");
    }
    else{
      server.send(418, "text/html", "failed");
    }
  }
}

char hex2char(char hex){
  char ret;
  if(hex >= 0x30 && hex <= 0x39) ret = hex - 0x30;
  else if(hex >= 0x41 && hex <= 0x46) ret = hex - 0x41 + 10;
  else if(hex >= 0x61 && hex <= 0x66) ret = hex - 0x61 + 10;

  return ret;
}

//You could do here more -- https://xboxdevwiki.net/PIC
void handleSMCRequests(){
  if(server.hasArg("reset")){
    if(found_smc){
      /* tells the SMC to reset the xbox*/
      Wire.beginTransmission(smc_addr);
      Wire.write(REG_PWR_CTRL);
      Wire.write(PWR_REBOOT);
      Wire.endTransmission();
    }
    server.send(200, "text/html", "OK");
  }
}

void handleHDDKeyUpdate(){
  if(server.hasArg("hdkey")){
    char hdkey_str[35];
    char hdkey[16];
    String(server.arg("hdkey")).toCharArray(hdkey_str, 33);
    
    for(int i=0;i<32;i+=2){
      char first = hex2char(hdkey_str[i]);
      char second = hex2char(hdkey_str[i+1]);
      hdkey[i/2] = ((first << 4) | (second & 0xF));
    }

    memset(hdkey_str, 0, 35);

    char key_str[2];
    for(int i=0;i<16;i++){
      sprintf(&hdkey_str[i*2], "%02X", hdkey[i]);
    }

    setHDDKey((unsigned char *)hdkey);
 
    server.send(200, "text/html", hdkey_str);
  }
  char curHDKeyStr[35];
  getHDDKey();
  for(int i = 0;i<16;i++){
    sprintf(&curHDKeyStr[i*2], "%02X", curHDKey[i]);
  }
  
  server.send(200, "text/html", curHDKeyStr);
}

void XDecrypt(unsigned char *eep){
  eepromdata data;

  unsigned char key_hash[20];
  unsigned char hash_confirm[20];

  int verloop = VERSION_10;
  while(verloop <= VERSION_DBG){
    memcpy(&data, eep, sizeof(eepromdata));
    xbx_hmac(verloop, key_hash, &data.HMAC_SHA1_Hash, 20, NULL);
    RC4KEY pRC4;
    InitRC4(key_hash, 20, &pRC4);
    RC4Crypt((unsigned char*)&data+20, 28, &pRC4);

    xbx_hmac(verloop, hash_confirm, &data.Confounder, 8, &data.HDDKey, 16, &data.XBERegion, 4, NULL);
    if(memcmp(hash_confirm, &data.HMAC_SHA1_Hash, 20) == 0){
      break;
    }
    verloop++;
  }

  xversion = verloop;

  memcpy(eep, &data, sizeof(eepromdata));
}

void XEncrypt(unsigned char *eep){
  eepromdata data;

  unsigned char key_hash[20];
  memcpy(&data, eep, sizeof(eepromdata));

  Serial.print("** xversion: ");
  Serial.print(xversion);
  Serial.println();

  xbx_hmac(xversion, key_hash, &data.Confounder, 8, &data.HDDKey, 16, &data.XBERegion, 4, NULL);
  memcpy(data.HMAC_SHA1_Hash, key_hash, 0x14);
  xbx_hmac(xversion, key_hash, &data.HMAC_SHA1_Hash, 20, NULL);
  
  RC4KEY pRC4;
  InitRC4(key_hash, 20, &pRC4);
  RC4Crypt((unsigned char *)&data+20, 28, &pRC4);

  memcpy(eep, &data, sizeof(eepromdata));
}

void setHDDKey(unsigned char *hdkey){
  unsigned char d[256];
  readEEPROMFromXbox(d);
  XDecrypt(d);
  eepromdata data;
  memcpy(&data, d, sizeof(eepromdata));

  memcpy(data.HDDKey, hdkey, 16);

  memcpy(d, &data, sizeof(eepromdata));
  XEncrypt(d);
  writeEEPROMToXbox(d);
}

void getHDDKey(void){
  unsigned char d[256];
  readEEPROMFromXbox(d);
  char key_str[2];
  XDecrypt(d);
  eepromdata data;
  memcpy(&data, d, sizeof(eepromdata));
  memcpy(curHDKey, data.HDDKey, 16);
}


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println("EXEC");
#ifndef ESP01
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#endif

/* This waits 1 second, so the Xbox can boot up. (Or up and down 3x when eeprom bricked)*/
#ifdef POWERFROMXBOX
  delay(1000);
#endif

#ifdef TESTMODE
  EEPROM.begin(0x100);
  found_eeprom = 1;
  found_smc = 0;
#else
  Wire.begin(sda, scl);
  Wire.beginTransmission(eep_addr);
  if(Wire.endTransmission() == 0){
    found_eeprom = 1;
  }

  Wire.beginTransmission(smc_addr);
  if(Wire.endTransmission() == 0){
    found_smc = 1;
  }
#endif

  /* Indicate found EEPROM by Status LED. */
#ifndef ESP01
  if(found_eeprom){
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
  }
#endif

  /* better way to say the user the eeprom/smc was found (especially on ESP-01) */
  if(found_smc){
    /* Give the Xbox some time */
#ifndef POWERFROMXBOX
    delay(1000);
#endif

    Wire.beginTransmission(smc_addr);
    Wire.write(REG_LED_SEQ);
    Wire.write(LED_ORANGE); // blinks slowly orange
    Wire.endTransmission();

    Wire.beginTransmission(smc_addr);
    Wire.write(REG_LED_MODE);
    Wire.write(LED_MODE_SEQ); // changes to blinky lights
    Wire.endTransmission();
  }
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("xbxdmp");
  MDNS.begin("xbxdmp");
  MDNS.addService("http", "tcp", 80);
  SPIFFS.begin(); 
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("xbxdmp");
  ArduinoOTA.begin();
  
  if(found_eeprom){
    
    server.onNotFound([]() {
      if(!handleFileRead(server.uri()))
        server.send(404, "text/plain", "404: Not Found");
    });
    server.on("/", HTTP_POST,
      [](){ server.send(200); },
      handleFileUpload
    );
    server.on("/upd", HTTP_GET, handleHDDKeyUpdate);
    server.on("/smc", HTTP_GET, handleSMCRequests);
  }
  else{
    server.onNotFound([]() {
      server.send(404, "text/plain", "404: Not Found");
    });
  }
  
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  ArduinoOTA.handle();
}
