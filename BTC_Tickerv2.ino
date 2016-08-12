// todo Add OTA Uptdate server after reset??
// if last reset cause then?
// try websocket version using ribuilder
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include "pitches.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>

#define OLED_RESET LED_BUILTIN

const unsigned char myBitmap [] PROGMEM = {0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xe0, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x00, 0x00, 
0x7f, 0xff, 0xfe, 0x00, 0x01, 0xff, 0xff, 0xff, 0x00, 0x03, 0xfe, 0x00, 0x7f, 0x80, 0x07, 0xf0, 
0x00, 0x1f, 0xc0, 0x0f, 0xe0, 0xce, 0x07, 0xe0, 0x0f, 0xc0, 0xcc, 0x03, 0xf0, 0x1f, 0x80, 0xcc, 
0x01, 0xf8, 0x3f, 0x00, 0xcc, 0x00, 0xf8, 0x3e, 0x1f, 0xff, 0x80, 0x7c, 0x7c, 0x1f, 0xff, 0xe0, 
0x7c, 0x7c, 0x1f, 0xff, 0xf0, 0x3e, 0x7c, 0x03, 0xc1, 0xf0, 0x3e, 0xf8, 0x03, 0xc1, 0xf0, 0x3e, 
0xf8, 0x03, 0xc1, 0xf0, 0x1e, 0xf8, 0x03, 0xc1, 0xf0, 0x1e, 0xf8, 0x03, 0xff, 0xe0, 0x1e, 0xf8, 
0x03, 0xff, 0xf0, 0x1f, 0xf8, 0x03, 0xff, 0xf8, 0x1f, 0xf8, 0x03, 0xc1, 0xf8, 0x1e, 0xf8, 0x03, 
0xc0, 0xfc, 0x1e, 0xf8, 0x03, 0xc0, 0x7c, 0x1e, 0xf8, 0x03, 0xc0, 0xfc, 0x3e, 0x7c, 0x03, 0xc1, 
0xf8, 0x3e, 0x7c, 0x1f, 0xff, 0xf8, 0x3e, 0x7c, 0x1f, 0xff, 0xf0, 0x7c, 0x3e, 0x1f, 0xff, 0xe0, 
0x7c, 0x3f, 0x00, 0xcc, 0x00, 0xf8, 0x1f, 0x80, 0xcc, 0x01, 0xf8, 0x0f, 0xc0, 0xcc, 0x03, 0xf0, 
0x0f, 0xe0, 0xcc, 0x07, 0xe0, 0x07, 0xf0, 0x00, 0x1f, 0xc0, 0x03, 0xfe, 0x00, 0x7f, 0x80, 0x01, 
0xff, 0xff, 0xff, 0x00, 0x00, 0x7f, 0xff, 0xfe, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x00, 0x00, 0x0f, 
0xff, 0xe0, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 
};

Adafruit_SSD1306 display(OLED_RESET);
char auth[34]; // authentication token for Blynk app
String id;
String value;
const char *host;
String url;
unsigned int oldtime = 0;
unsigned int frequency = 5000;
int lalarm;
int ualarm;
int alarm;
int price;
int currency;
bool firstconnect = true;
int port = 80;
uint16_t xoffset = 24;
uint16_t yoffset = 50;
bool saveConfig = false;

void setup() {

  Serial.begin(115200);
  Serial.println(F("Bitcoin Price Ticker"));

  //Initialise Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
  display.drawBitmap(44, 12, myBitmap, 40, 40, WHITE);
  display.display();
  display.setFont(&FreeSansBold18pt7b);
  display.setTextWrap(false);
  display.setTextColor(WHITE);

  Serial.println(F("Mounting FS..."));

  if (!SPIFFS.begin()){
    Serial.println(F("Failed to mount file system"));
    return;
  }
  if (SPIFFS.exists("/config.json")){
    if (!load_config(auth)) {
      Serial.println(F("Failed to load config"));
    } 
    else {
      Serial.println(F("Config loaded"));
    } 
  }
  WiFiManagerParameter Auth("Blynk_Token", "Key", auth, 33);
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(save_config_callback);
  wifiManager.addParameter(&Auth);
  wifiManager.autoConnect("Bitcoin-Ticker");  

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP()); 
  
  if (saveConfig){
    strcpy(auth, Auth.getValue());
    Serial.print(F("Token: "));
    Serial.println(auth);
    Serial.println(strlen(auth));
    save_config(auth);
  }
  
  set_source(0);
  
  Serial.println(auth);
  Blynk.config(auth);

}

void save_config_callback() {
  
  Serial.println(F("Should save config"));
  saveConfig = true;
  
}

bool save_config(char* auth){

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["auth"] = auth;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile){
    Serial.println(F("Failed to open config file for writing"));
    return false;
  }
  json.printTo(Serial);
  json.printTo(configFile);
  Serial.print(F("Save New Auth = "));
  Serial.println(auth);
  
  return true;
  
}

bool load_config(char* auth){
  
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println(F("Config file size is too large"));
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println(F("Failed to parse config file"));
    return false;
  }

  strcpy(auth, json["auth"]);

  Serial.print(F("Loaded auth: "));
  Serial.println(auth);

  return true;
  
}

void set_source(int source){

  switch (source){
    case 0:
      host = "api.coindesk.com";
      url = "/v1/bpi/currentprice.json";
      port = 80;
      break;
    case 1:
      host = "api.huobi.com";
      url = "/staticmarket/ticker_btc_json";
      port = 80;
      break;
    case 2:
      host = "www.okcoin.cn";
      url = "/api/v1/ticker.do?symbol=btc_cny";
      port = 443;
      break;
    case 3:
      host = "api.bitfinex.com";
      url = "/v1/pubticker/btcusd";
      port = 443;
      break;
    case 4:
      host = "api.bitfinex.com";
      url = "/v1/pubticker/ethusd";
      port = 443;
      break;
    case 5:
      host = "api.bitfinex.com";
      url = "/v1/pubticker/ltcusd";
      port = 443;
      break;
    default: 
      host = "api.coindesk.com";
      url = "/v1/bpi/currentprice.json";
      port = 80;
      break;
  }
}

bool get_price_ssl(){
  
  WiFiClientSecure client;
  //Serial.println(ESP.getFreeHeap());
  
  if (!request_data_ssl(&client))
    return false;
  
  if (!skip_headers_ssl(&client))
    return false;

  char response[1024];
  read_content_ssl(&client, response, sizeof(response));
  
  client.flush();
  client.stop();
  
  if (!do_json(response))
    return false;
    
  return true;
}

bool get_price(){

  WiFiClient client;

  if (!request_data(&client))
    return false;
  
  if (!skip_headers(&client))
    return false;
  
  char response[1024];  
  read_content(&client, response, sizeof(response));
  
  client.flush();
  client.stop(); 
  
  if (!do_json(response))
    return false;
    
  return true;
  
}

bool do_json(char* response){
  
  // See http://codebeautify.org/jsonviewer

  StaticJsonBuffer<512> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(response);

  if (!root.success()){
    Serial.println(F("JSON failed!"));
    return false;
  }
  Serial.print(F("JSON = "));
  Serial.println(jsonBuffer.size());
  char result[12];
  switch (currency){
    case 0://cd usd
        strncpy(result, root["bpi"]["USD"]["rate"], 7);
        break;
    case 1://cd eur
       strncpy(result, root["bpi"]["EUR"]["rate"], 7);     
       break;
    case 2://cd gbp
       strncpy(result, root["bpi"]["GBP"]["rate"], 7);
       break;
    case 3://houbi
       strncpy(result, root["ticker"]["last"], 7);
       break;
    case 4://okcoin
       strncpy(result, root["ticker"]["last"], 7);
       break;
    case 5://bitfinexBTCETHLTC
       strncpy(result, root["mid"], 7);
       break;
    default:
       return false;
  }
  result[7] = '\0';
  value = result;
  price = value.toInt();
  return true;
  
}

bool request_data(WiFiClient* client){
  
  Serial.println();
  Serial.print(F("connect to "));
  Serial.println(host);

  if (!client->connect(host, port)) {
    Serial.println(F("ERROR conn failed"));
    return false;
  }
  
  Serial.print(F("Requesting URL: "));
  Serial.println(url);
  client->print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  return true;
  
}

bool request_data_ssl(WiFiClientSecure* client){
  
  Serial.println();
  Serial.print(F("connect to "));
  Serial.println(host);

  if (!client->connect(host, port)) {
    Serial.println(F("ERROR conn failed"));
    return false;
  }
  
  Serial.print(F("Requesting URL: "));
  Serial.println(url);
  client->print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  return true;
}

bool skip_headers(WiFiClient* client){ 
  
  char endOfHeaders[] = "\r\n\r\n";
  bool ok = client->find(endOfHeaders);

  return ok;
}

bool skip_headers_ssl(WiFiClientSecure* client){ 
  
  char brace = '{';
  bool check = client->find("{");

  return check;
}

void read_content(WiFiClient* client, char* content, size_t maxsize){
  
  size_t length = client->readBytes(content, maxsize);
  content[length] = '\0';
  Serial.println(content); 
}

void read_content_ssl(WiFiClientSecure* client, char* content, size_t maxsize){

  content[0] = '{';
  size_t length = client->readBytes(&content[1], maxsize - 1);
  content[length + 1] = '\0';
  Serial.println();
  Serial.println(content);
  Serial.println();
  
}


void display_price(){

  if (value.length() > 5){
    value = value.substring(0, 5);
  }

  if (value.endsWith("."))
    value.remove(value.length() - 1);
    
  display.clearDisplay(); 
  display.setCursor(xoffset, yoffset);
  display.print(value);
  //display.drawRect(0,0,128,64,WHITE);
  display.display();
  int16_t boxX, boxY; 
  uint16_t boxw, boxh;
  display.getTextBounds((char*)value.c_str(), xoffset, yoffset, &boxX, &boxY, &boxw, &boxh);

  xoffset = 64 - (int)(boxw/2);  
  Blynk.virtualWrite(0, value," ", id);
  Blynk.syncVirtual(V4);  //alarm state check
}

BLYNK_CONNECTED() {
  
  if (firstconnect){
    Blynk.syncAll();
    firstconnect = false;
  } 
}

BLYNK_WRITE(V1){
  
  Serial.print(F("Got new Freq: "));
  Serial.println(param.asStr());
  frequency = param.asInt() * 1000; 
}

BLYNK_WRITE(V2){
  
  Serial.print(F("Got lalarm: "));
  Serial.println(param.asStr());
  lalarm = price + param.asInt();
  if (ualarm <= lalarm){
    ualarm = lalarm + 2;
    Blynk.virtualWrite(8, ualarm," ", id);
  }
  Blynk.virtualWrite(7, lalarm," ", id);
}

BLYNK_WRITE(V3){
  
  Serial.print("Got ualarm: ");
  Serial.println(param.asStr());
  ualarm = price + param.asInt();
  if (lalarm >= ualarm){
    lalarm = ualarm - 2;
    Blynk.virtualWrite(7, lalarm," ", id);
  }
  Blynk.virtualWrite(8, ualarm," ", id);
}

BLYNK_WRITE(V4){
  
  Serial.print(F("Alarm toggled??: "));
  Serial.println(param.asStr());
  alarm = param.asInt();
}

BLYNK_WRITE(V5){

  Serial.print(F("Reset: "));
    Serial.println(param.asInt());
  if (param.asInt() == 1){   
    reset_device();
  }
}

BLYNK_WRITE(V6){
  Serial.print(F("Currency toggled: "));
  Serial.println(param.asInt());
  switch (param.asInt())
    {
      case 1: { // coindesk
        Serial.println("USD");
        set_source(0);
        currency = 0;
        id = "USD";
        break;
      }
      case 2: {
        Serial.println("EUR");
        set_source(0);
        currency = 1;
        id = "EUR";
        break;
      }    
      case 3: {
        Serial.println("GBP");
        set_source(0);
        currency = 2;
        id = "GBP";
        break;
      }
      case 4: { // houbi
        Serial.println("CNY");
        set_source(1);
        currency = 3;
        id = "CNY";
        break;
      }
      case 5: { // okcoin
        Serial.println("CNY");
        set_source(2);
        currency = 4;
        id = "CNY";
        break;
      }
      case 6:{ // bitfinex
        Serial.println("bfxUSD");
        set_source(3);
        currency = 5;
        id = "USD";
        break;
      }
      case 7:{ // bitfinex eth
        Serial.println("bfxETHUSD");
        set_source(4);
        currency = 5;
        id = "USD";
        break;
      }
      case 8:{ // bitfinex eth
        Serial.println("bfxLTCUSD");
        set_source(5);
        currency = 5;
        id = "USD";
        break;
      }
      
      default:
        break;   
  }

  oldtime = 0;
  if (alarm == 1 && firstconnect == 0){
    alarm = 0;
    Blynk.virtualWrite(4,0);
    //lalarm = price - 100;
    Blynk.virtualWrite(7, lalarm," ", id);
    //ualarm = price + 100;
    Blynk.virtualWrite(8, ualarm," ", id);
  }
}

void play_alarm(bool up){

  tone(14, NOTE_G5, 150);
  delay(200);
  tone(14, NOTE_G5, 150);
  delay(200);
  tone(14, NOTE_A5, 300);
  delay(350);
  if (up)
    tone(14, NOTE_D6, 300);
  else
    tone(14, NOTE_D5, 150);
  delay(160);
  noTone(14);
  delay (10);
  pinMode(14, INPUT);
  return;

}

void loop() {
  
  Blynk.run();
  
  if ((millis() - oldtime) > frequency){
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("ERROR Wifi Disconnected"));
      delay(2000);
      return;
    }
    
    if (port == 80){
      if (!get_price()){
        delay(1000);
        return;
      }
    }
    else {
      if (!get_price_ssl()){
        delay(1000);
        return;
      }
    }

    display_price();
    
    if (alarm){
      if (price <= lalarm){
        Serial.println(F("Price lower! ALARM"));
        play_alarm(0);
      }
      if (price >= ualarm){
        Serial.println(F("Price higher! ALARM"));
        play_alarm(1);
      }
    }
     
    //value="";
    oldtime = millis();
  }  
  delay(10);
}

void reset_device(){

 WiFi.disconnect();
 delay(1000);
 ESP.restart();
 delay(5000);
}

