#ifndef _AUTOWIFI_H_
#define _AUTOWIFI_H_

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef STASSID
#define STASSID "ESPWebServer"
#define STAPSK  "12345678";
#endif

//const String postForms;

class AutoWifi
{
public:
  static String ssid;
  static String pwd;
  const char* AutoSSID = STASSID;
  const char* AutoPWD = STAPSK;
  //static bool successFlag = false;
  static bool successFlag;
  static ESP8266WebServer *server;
  
  AutoWifi();
  void start();
  static void handleRoot();
  static void handleSubmit();
  static String readSSID(String httpText);
  static String readPWD(String httpText);
  IPAddress getMyIP();
  bool success();
  void close();
  };

#endif /*!_AUTOWIFI_H_*/
