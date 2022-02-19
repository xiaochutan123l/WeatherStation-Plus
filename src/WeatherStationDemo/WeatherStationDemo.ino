/**The MIT License (MIT)

Copyright (c) 2018 by Daniel Eichhorn - ThingPulse

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at https://thingpulse.com
*/

#include <Arduino.h>

#include <ESPWiFi.h>
#include <ESPHTTPClient.h>
#include <JsonListener.h>
#include <ESP8266WebServer.h>
// time
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()

#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"
#include "SH1106Wire.h"
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "token.h"
//Humidity
#include <SimpleDHT.h>
#include "AutoWiFi.h"
//https://cxybb.com/article/qq_38113006/119026279
//中断https://blog.51cto.com/u_15127590/4036514
//D1:5       D2:4       D4:2      D5:14       D6:12       D7:13     D8:15 

/***************************
 * Begin Settings
 **************************/

// WIFI
String WIFI_SSID = SSID;
String WIFI_PWD = PWD;

//#define TZ              2       // (utc+) TZ in hours
#define TZ              0       // (utc+) TZ in hours
//#define DST_MN          60      // use 60mn for summer time in some countries
#define DST_MN          60

// Setup
const int UPDATE_INTERVAL_SECS = 20 * 60; // Update every 20 minutes

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
//const int I2C_DISPLAY_ADDRESS = 0x7b;
#if defined(ESP8266)
const int SDA_PIN = D3;
const int SDC_PIN = D4;
#else
const int SDA_PIN = 5; //D3;
const int SDC_PIN = 4; //D4;
#endif
// Humiity sensor pin.
int pinDHT11 = D0;

byte temperature = 0;
byte humidity = 0;
int err = SimpleDHTErrSuccess;

// Light Sensor pin.
//int pinLight = D7;
int pinLight = A0;
int lightValue = 0;

//const int KEY1_PIN = D5;
//const int KEY1_PIN = 14;
volatile int KEY1_INTERRUPT = 0;
int KEY1_EVENT = 0;
volatile int KEY2_INTERRUPT = 0;
int KEY2_EVENT = 0;

volatile int CURRENT_FRAME_NUM = 1;
volatile bool INIT_FINISH = false;

#define RC_STATE_NONE       0
#define RC_STATE_START      1
#define RC_STATE_SHUTDOWN   2
#define RC_STATE_FINISH     3
#define RC_STATE_ERROR      4

const char* host = "192.168.2.118";
const int port = 9998;

// OpenWeatherMap Settings
// Sign up here to get an API key:
// https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_APP_ID = APP_ID;
/*
Go to https://openweathermap.org/find?q= and search for a location. Go through the
result set and select the entry closest to the actual location you want to display 
data for. It'll be a URL like https://openweathermap.org/city/2657896. The number
at the end is what you assign to the constant below.
 */
String OPEN_WEATHER_MAP_LOCATION_ID = LOCATION_ID;

// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
String OPEN_WEATHER_MAP_LANGUAGE = "de";
const uint8_t MAX_FORECASTS = 4;

const boolean IS_METRIC = true;

// Adjust according to your language
const String WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/***************************
 * End Settings
 **************************/
 // Initialize the oled display for address 0x3c
 // sda-pin=14 and sdc-pin=12
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
//SH1106Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
//SH1106Wire       display(0x3c, SDA, SCL, GEOMETRY_128_64, I2C_ONE, -1);
OLEDDisplayUi   ui( &display );

// Initialize the humidity sensor.
SimpleDHT11 dht11(pinDHT11);

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapCurrent currentWeatherClient;

OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecast forecastClient;

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;
// for humidity update, update for each 3 Min.
long timeSinceLastHUpdate = 0;
const int HumidityUpdateInterval = 3 * 60 * 1000;

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
//void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
//void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();
void drawRemoteControl(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawRemoteControlDetails(OLEDDisplay *display, String text, int16_t x, int16_t y);
void updateDisplay();
void updataHumidity();
void updateLightValue();

//void drawAutoWiFi(IPAddress ip, char* autoSSID, char* autoPWD);

// Interrupts
void ICACHE_RAM_ATTR ISR1();
void ICACHE_RAM_ATTR ISR2();
void ISR_EVENT_HANDLER();

void REMOTE_CONTROL();
int RC_STATE = 0;
const int RC_Freez_Time = 8000;
int RC_Freez_Begin = 0;
bool RC_Freez = false;

// Dark mode: energy saving.
void Dark_Mode();
const int dark_value = 100;

// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
//FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast };
FrameCallback frames[] = { drawCurrentWeather, drawDateTime, drawRemoteControl};
int numberOfFrames = 3;
volatile const int RC_FRAME = 3;

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println();

  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  //TODO: conncet with defualt(from EEPROM), if not available, then using Auto WiFi
  //      and record to EEPROM to update the newest default WiFI configuration.
  // Auto connection using webserver.
  AutoWifi autoWifi;
  //char * ip[sizeof(autoWifi.getMyIP())];
  //snprintf(ip, sizeof(autoWifi.getMyIP()), autoWifi.getMyIP());
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Please connect my WiFi:");
  //display.setFont(ArialMT_Plain_16);
  display.drawString(0, 16, "SSID:");
  display.drawString(32, 16, autoWifi.AutoSSID);
  display.drawString(0, 32, "PWD:");
  display.drawString(32, 32, autoWifi.AutoPWD);
  display.drawString(0, 48, "URL:");
  display.drawString(32, 48, autoWifi.getMyIP().toString());
  display.display();
  
  autoWifi.start();
  WIFI_SSID = autoWifi.ssid;
  WIFI_PWD = autoWifi.pwd;
  Serial.begin(9600);
  Serial.println("successfully get SSID and PWD");
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PWD);
  
  //display.clear();
  //display.drawString(10, 10, "Successfully get SSID and PWD");
  //display.display();
  
  autoWifi.close();

  
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  int counter = 0;
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();

    counter++;
  }

  
 
  // Get time from network time service
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");

  ui.setTargetFPS(30);

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, numberOfFrames);

  ui.setOverlays(overlays, numberOfOverlays);

  // TODO: Disable Auto transition, change to push bottom trigger.
  //       Push once jump to next frame.
  ui.disableAutoTransition();
  /*
  // Manuell Controll
  ui.nextFrame();
  ui.previousFrame();
  */
  // Inital UI takes care of initalising the display too.
  ui.init();

  Serial.println("");

  updateData(&display);

  //TODO: Add two Interrupts.
  pinMode(D5, INPUT_PULLUP);
  attachInterrupt(D5,ISR1,FALLING);
  pinMode(D6, INPUT_PULLUP);
  attachInterrupt(D6,ISR2,FALLING);

  updataHumidity();

  // Initialize Light sensor.
  pinMode(pinLight, INPUT);
  updateLightValue();
  
  INIT_FINISH = true;
  Serial.println("init finished");
}

void loop() {

  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
    // delete it, just for test.
  }

  if (millis() - timeSinceLastHUpdate > HumidityUpdateInterval) {
    updataHumidity();
    timeSinceLastHUpdate = millis();
  }
  
  updateDisplay();
  
  ISR_EVENT_HANDLER();

  if (lightValue <= dark_value)
  {
    Dark_Mode();  
  }
}

void updateDisplay(){
  /* 刷新OLED屏幕，返回值为在当前设置的刷新率下显示完当前图像后所剩余的时间 */
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
}

void ISR_EVENT_HANDLER(){
  /*
  Key 1 for frame transition.
  Key 2 only available in remote control frame, 
  */
  if (KEY1_INTERRUPT == 1){
  //Add debounce algorithm if it has impact.
  //疯狂连按没有问题，因为每次中断函数把参数置1，所以连按还是1， 结果部分中断没有被记录，但是反而让操作更ruboust。
    KEY1_INTERRUPT = 0;
    KEY1_EVENT += 1;
  }
  if (KEY1_EVENT >= 1){
    ui.nextFrame();
    KEY1_EVENT -= 1;
    CURRENT_FRAME_NUM = (CURRENT_FRAME_NUM % numberOfFrames) + 1;
  }
  if ((CURRENT_FRAME_NUM == 3) && (KEY2_INTERRUPT == 1)){
        REMOTE_CONTROL();
        KEY2_INTERRUPT = 0;
  }
}

void ICACHE_RAM_ATTR ISR1(){
  //ISR函数里所使用的变量应声明为volatile类型
  if (INIT_FINISH){
    KEY1_INTERRUPT = 1;
  }
}

void ICACHE_RAM_ATTR ISR2(){
  if (INIT_FINISH && CURRENT_FRAME_NUM == RC_FRAME){
    KEY2_INTERRUPT = 1;
  }
}

void REMOTE_CONTROL(){
  WiFiClient client;
  
  RC_STATE = RC_STATE_START;
  updateDisplay();
  delay(500);
  
  if (client.connect(host, port)){
    // we are connected to the host!
    Serial.println("TCP connection established.");
    while (client.connected() || client.available()){
      if (client.connected()){
        RC_STATE = RC_STATE_SHUTDOWN;
        updateDisplay();
        delay(500);
        client.println("shutsown");
        Serial.println("Sent command");
        RC_STATE = RC_STATE_FINISH;
        break;
      }
    }
  }
  else{
    // connection failure
    RC_STATE = RC_STATE_ERROR;
    Serial.println("TCP connection error.");
  }
  client.stop();
  updateDisplay();
  delay(500);
  Serial.println("\n[Disconnected]");
  RC_Freez = true;
  RC_Freez_Begin = millis();
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateData(OLEDDisplay *display) {
  drawProgress(display, 10, "Updating time...");
  drawProgress(display, 30, "Updating weather...");
  currentWeatherClient.setMetric(IS_METRIC);
  currentWeatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient.updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  drawProgress(display, 50, "Updating forecasts...");
  forecastClient.setMetric(IS_METRIC);
  forecastClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);

  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done...");
  delay(1000);
}

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];


  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = WDAY_NAMES[timeInfo->tm_wday];

  sprintf_P(buff, PSTR("%s, %02d/%s/%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mday, MONTH_NAMES[timeInfo->tm_mon], timeInfo->tm_year + 1900);
  //display->drawString(64 + x, 5 + y, String(buff));
  display->drawString(64 + x, y, String(buff));
  display->setFont(ArialMT_Plain_24);

  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(64 + x, 20 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  
  //String tmp = String(int(temperature)) + (IS_METRIC ? "°C" : "°F") + " " + String(int(humidity)) + "%";
  int base = 45;
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  String tmp = String(int(temperature)) + "°";
  String hum = String(int(humidity));
  display->drawXbm(base + x, 30 + y, Humidity_Symbol_width, Humidity_Symbol_height, thermoMeter);
  display->drawString(base + 16 + x, 29 + y, tmp);
  display->drawXbm(base + 16 + 30 + x, 30 + y, Humidity_Symbol_width, Humidity_Symbol_height, waterDrop);
  display->drawString(base + 16 + 30 + 18 + x, 29 + y, hum);
  
  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = String(int(currentWeather.temp)) + (IS_METRIC ? "°C" : "°F");
  //String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(60 + x, y, temp);

  display->setFont(Meteocons_Plain_42);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //display->drawString(32 + x, 0 + y, currentWeather.iconMeteoCon);
  display->drawString(x, y, currentWeather.iconMeteoCon);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(buff));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  // TODO: just for testing the suitable light value, later to restore the field.
  String temp = String(int(currentWeather.temp)) + (IS_METRIC ? "°C" : "°F");
  updateLightValue();
  //String temp = String(lightValue);
  display->drawString(128, 54, temp);
  display->drawHorizontalLine(0, 52, 128);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}

void drawRemoteControl(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{ 
  if (RC_Freez){
      if (millis() - RC_Freez_Begin >= RC_Freez_Time){
          RC_STATE = RC_STATE_NONE;
          RC_Freez = false;
        }
  }
  String text;
  switch(RC_STATE){
    case RC_STATE_NONE:
      text = "Push bottom to shutdown the PC";
      break;
    case RC_STATE_START:
      text = "Start connecting PC";
      break;
    case RC_STATE_SHUTDOWN:
      text = "Shutdowning PC...";
      break;
    case RC_STATE_FINISH:
      text = "Done...";
      break;  
    case RC_STATE_ERROR:
      text = "Error: PC is down, start it first";
      break;
  }
  drawRemoteControlDetails(display, text, x, y);
}

void drawRemoteControlDetails(OLEDDisplay *display, String text, int16_t x, int16_t y){
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 10 + y, 128, text);
}

void updataHumidity() {
  // read without samples.
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); 
    Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); 
    Serial.println(SimpleDHTErrDuration(err)); 
    delay(1000);
    return;
  }
  
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); 
  Serial.print(" *C, "); 
  Serial.print((int)humidity); 
  Serial.println(" H");
  
  // DHT11 sampling rate is 1HZ.
  delay(1500);
}

void updateLightValue(){
  int readValue = 0;
  for (int i = 0; i < 2; i++){
    readValue += analogRead(pinLight);
    }
  lightValue = int(readValue / 2);

  Serial.println(lightValue);
}

void Dark_Mode()
{
  display.displayOff();
  int counter = 0;
  while(1)
  {
    updateLightValue();
    if (lightValue > dark_value)
    {
      // Observe for multiple times to eliminate the vibration.
      for (int i = 0; i < 5; i++)
      {
        updateLightValue();
        if (lightValue > dark_value)
        {
          counter += 1;
        }
        delay(50);
      }

      if (counter >= 5)
      {
        display.displayOn();
        break;
      }
    }
    delay(500);
  }
}
