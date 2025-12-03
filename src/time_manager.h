#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <RTC_RX8025T.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeManager {
public:
  TimeManager();
  void begin();
  void update();
  bool syncFromNTP();
  void checkTimer();
  
  tmElements_t getCurrentTime();
  bool isWifiConnected() const { return wifiConnected; }
  bool isTimeSynced() const { return timeSynced; }

private:
  bool wifiConnected;
  bool timeSynced;
  unsigned long lastWifiCheck;
  
  void checkWifiConnection();
};

extern TimeManager timeManager;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

#endif
