#ifndef NIXIE_CONTROLLER_H
#define NIXIE_CONTROLLER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ShiftRegister74HC595.h>
#include "config_manager.h"
#include "hardware_config.h"

class NixieController {
public:
  NixieController();
  void begin();
  void displayTime();
  void displayManualDigits();
  void setBrightness();
  void handleBlink();
  void clearDisplay();
  
  // 手动输入相关
  void setManualMode(bool enabled);
  void setManualDigit(int position, int digit);
  bool isManualMode() const { return manualMode; }
  
  // 防中毒相关
  void runAntiPoison();
  bool isAntiPoisonRunning() const { return antiPoisonRunning; }
  void checkAntiPoison();
  
  // IP地址显示相关
  void displayIPAddress(IPAddress ip);
  void displayIPAddressOctet(int octet, int startPosition);

  // 测试模式相关
  void setTestMode(bool enabled);
  bool isTestModeEnabled() const { return testMode; }
  void runTestSequence();

private:
  bool manualMode;
  int manualDigits[4];

  // 测试模式相关
  bool testMode;
  unsigned long lastTestUpdate;
  int currentTestDigit;
  
  // 防中毒相关
  bool antiPoisonRunning;
  unsigned long lastAntiPoisonTime;
  unsigned long antiPoisonStartTime;
  int currentAntiPoisonDigit;
  int antiPoisonDigits[4];
  
  // 驱动数据
  uint8_t data[5];

  // 时间控制
  unsigned long timeDispDelay;
  unsigned long timeBrightnessDelay;
  
  // 内部方法
  void displayNumber(int digit, int tubePosition);
  void updateShiftRegisters();
  void setDot(bool on);
};

extern NixieController nixieController;

#endif
