#include "time_manager.h"
#include "config_manager.h"
#include "sensor_manager.h"
#include "hardware_config.h"
#include <ESP8266WiFi.h>
#include <Wire.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "cn.pool.ntp.org", 0, 60000);  // 时区偏移设为0，由配置控制
TimeManager timeManager;

TimeManager::TimeManager() 
  : wifiConnected(false), timeSynced(false), lastWifiCheck(0) {}

void TimeManager::begin() {
  Wire.begin(SDA, SCL);
  RTC_RX8025T.init();
}

void TimeManager::update() {
  checkWifiConnection();
}

void TimeManager::checkWifiConnection() {
  if (millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    wifiConnected = WiFi.status() == WL_CONNECTED;
  }
}

bool TimeManager::syncFromNTP() {
  Config& config = configManager.getConfig();
  
  timeClient.setPoolServerName(config.ntpServer);
  timeClient.begin();
  
  if (!timeClient.forceUpdate()) {
    Serial.println("NTP同步失败");
    return false;
  }

  // 获取UTC时间戳
  unsigned long utcEpoch = timeClient.getEpochTime();
  
  // 应用时区偏移
  unsigned long localEpoch = utcEpoch + config.timezoneOffset;
  
  // 设置系统时间为本地时间
  setTime(localEpoch);
  
  // 同步到RTC
  RTC_RX8025T.set(now());
  
  Serial.printf("NTP同步完成: UTC时间戳=%lu, 时区偏移=%d秒, 本地时间戳=%lu\n", 
                utcEpoch, config.timezoneOffset, localEpoch);
  return true;
}

tmElements_t TimeManager::getCurrentTime() {
  tmElements_t tm;
  RTC_RX8025T.read(tm);
  return tm;
}

void TimeManager::checkTimer() {
  Config& config = configManager.getConfig();
  if (!config.timerEnabled) return;
  
  tmElements_t now = getCurrentTime();
  int currentHour = now.Hour;
  int currentMinute = now.Minute;
  
  // 转换为分钟数便于比较
  unsigned long currentTime = currentHour * 60 + currentMinute;
  unsigned long onTime = config.onHour * 60 + config.onMinute;
  unsigned long offTime = config.offHour * 60 + config.offMinute;
  
  bool shouldBeOn = false;
  
  if (onTime < offTime) {
    // 同一天的时间段
    shouldBeOn = (currentTime >= onTime) && (currentTime < offTime);
  } else if (onTime > offTime) {
    // 跨天的时间段
    shouldBeOn = (currentTime >= onTime) || (currentTime < offTime);
  } else {
    // 相同时间，视为关闭
    shouldBeOn = false;
  }
  
  // 通过传感器管理器处理定时开关机，让传感器管理器决定最终的开关机状态
  sensorManager.setTimerPowerState(shouldBeOn);
}
