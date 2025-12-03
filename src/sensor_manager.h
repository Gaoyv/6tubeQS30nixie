#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

enum SystemPowerState {
  POWER_OFF,           // 系统关机
  POWER_ON_TIMER,      // 定时开机
  POWER_ON_PRESENCE    // 人在开机
};

class SensorManager {
public:
  SensorManager();
  void begin();
  void update();
  
  bool isPresenceDetected() const { return presenceState; }
  bool isSystemOn() const { return systemPowerState != POWER_OFF; }
  SystemPowerState getPowerState() const { return systemPowerState; }
  void setTimerPowerState(bool on);  // 由时间管理器调用
  
private:
  unsigned long lastPresenceTime;
  unsigned long presenceOnStartTime;
  bool presenceState;
  bool lastPresenceState;
  SystemPowerState systemPowerState;
  
  void handlePresenceSensor();
  void updateSystemPower();
  void turnSystemOn(SystemPowerState reason);
  void turnSystemOff();
};

extern SensorManager sensorManager;

#endif
