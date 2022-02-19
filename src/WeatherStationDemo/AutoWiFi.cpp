#include "AutoWiFi.h"
#include "Arduino.h"

ESP8266WebServer webServer(80); //Server on port 80

const String postForms = "<html>\
    <head>\
      <title>ESP8266 Web Server POST handling</title>\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
      </style>\
    </head>\
    <body>\
      <h1>Please tell me your WiFi SSID and Password!</h1><br>\
      <form method=\"post\" enctype=\"text/plain\" action=\"/submit/\">\
        <h2>SSID:</h2> <input type=\"text\" name=\"ssid\"><br>\
        <h2>Password:</h2> <input type=\"text\" name=\"psw\"><br>\
        <input type=\"submit\" value=\"Submit\">\
      </form>\
    </body>\
  </html>";

String AutoWifi::ssid = "";
String AutoWifi::pwd = "";
ESP8266WebServer * AutoWifi::server = &webServer;
bool AutoWifi::successFlag = false;

AutoWifi::AutoWifi()
{
  ssid = pwd = "";  
  //server = &webServer;
  successFlag = false;
  //Serial.begin(9600);
  //Serial.println("");
  WiFi.mode(WIFI_AP);           //Only Access point
  WiFi.softAP(AutoSSID, AutoPWD);  //Start HOTspot removing password will disable security

  IPAddress myIP = WiFi.softAPIP(); //Get IP address
  //Serial.print("HotSpt IP:");
  //Serial.println(myIP);
 
  server->on("/", handleRoot);      //Which routine to handle at root location
  server->on("/submit/", handleSubmit);

  server->begin();                  //Start server
  //Serial.println("HTTP server started");
}

void AutoWifi::close()
{
  server->close();  
}
  
void AutoWifi::start()
{
  while (!successFlag)
  {
    server->handleClient();          //Handle client requests
  }
}
  
void AutoWifi::handleRoot()
{
  server->send(200, "text/html", postForms);
}

void AutoWifi::handleSubmit()
{
  if (server->method() != HTTP_POST) {
    server->send(405, "text/plain", "Method Not Allowed");
  } else {
    //String message = "POST form was:\n";
    //for (uint8_t i = 0; i < server->args(); i++) {
    //  message += "-" + String(i) + "- " + server->argName(i) + ": " + server->arg(i);
    //}
    //server->send(200, "text/plain", message);
  //}
    String httpText = server->arg(0);
    ssid = readSSID(httpText);
    pwd = readPWD(httpText);
    if (ssid.length() > 0 and pwd.length() > 0)
    {
      //Serial.print("ssid: ");
      //Serial.println(ssid);
      //Serial.print("pwd: ");
      //Serial.println(pwd);
      server->send(200, "text/plain", "done!");
      }
    successFlag = true;
  }
}

String AutoWifi::readSSID(String httpText)
{
  // TODO: change to c++ supported function to get the index.
  int s_index = httpText.indexOf("=");
  int e_index = httpText.indexOf("\n");
  if (httpText.substring(s_index + 1, e_index - 1))
  {
    return httpText.substring(s_index + 1, e_index - 1);    
    }
  else
  {
    server->send(200, "text/plain", "SSID Extraction Error");
    return "";
    }
}

String AutoWifi::readPWD(String httpText)
{
  int s_index = httpText.lastIndexOf("=");
  if (httpText.substring(s_index + 1))
  {
    return httpText.substring(s_index + 1);    
    }
  else
  {
    server->send(200, "text/plain", "PSW Extraction Error");
    return "";
    }
}

IPAddress AutoWifi::getMyIP()
{
  return WiFi.softAPIP();
}

bool AutoWifi::success()
{
  return successFlag;
}
