#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

class WebServerManager {
public:
  WebServerManager();
  void begin();
  void handleClient();
  
private:
  void handleRoot();
  void handleSet();
  void handlePower();
  void handleManual();
  void handleSyncNtp();
  void handleTest();
  
  String generateHTML();
  String getSystemStatus();
};

extern WebServerManager webServerManager;
extern ESP8266WebServer server;

#endif
