#include "sensor_manager.h"
#include "hardware_config.h"
#include "config_manager.h"
#include "led_effects.h"
#include "nixie_controller.h"

SensorManager sensorManager;

SensorManager::SensorManager() 
  : lastPresenceTime(0), presenceOnStartTime(0), presenceState(false), 
    lastPresenceState(false), systemPowerState(POWER_OFF) {}

void SensorManager::begin() {
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(BOOST_ENABLE_PIN, OUTPUT);
  
  // 根据人在检测开关决定初始状态
  Config& config = configManager.getConfig();
  if (config.presenceWakeEnabled) {
    // 如果启用了人在检测，初始状态为关机，等待检测到人在
    systemPowerState = POWER_OFF;
    digitalWrite(BOOST_ENABLE_PIN, LOW);
    Serial.println("人在检测已启用，等待检测到人在后开机");
  } else {
    // 如果没有启用人在检测，默认为开机状态
    systemPowerState = POWER_ON_TIMER;
    digitalWrite(BOOST_ENABLE_PIN, HIGH);
    Serial.println("人在检测未启用，系统默认开机");
  }
}

void SensorManager::update() {
  handlePresenceSensor();
  updateSystemPower();
}

void SensorManager::handlePresenceSensor() {
  lastPresenceState = presenceState;
  presenceState = digitalRead(SENSOR) == HIGH;
  
  if (presenceState) {
    lastPresenceTime = millis();
  }
}

void SensorManager::updateSystemPower() {
  Config& config = configManager.getConfig();
  
  // 只有在启用了人在检测开关时才处理人在检测逻辑
  if (!config.presenceWakeEnabled) {
    // 如果未启用人在检测，系统保持当前状态（默认开机）
    return;
  }
  
  // 检查人在传感器状态变化
  if (presenceState && !lastPresenceState) {
    // 检测到人在，立即开机或重新计时
    turnSystemOn(POWER_ON_PRESENCE);
    presenceOnStartTime = millis();
    Serial.println("检测到人在，系统开机");
  }
  
  // 人在开机状态下的超时检查
  if (systemPowerState == POWER_ON_PRESENCE) {
    if (presenceState) {
      // 继续检测到人在，重新开始计时
      presenceOnStartTime = millis();
    } else {
      // 检查是否超时
      unsigned long presenceTimeout = config.presenceOnTime * 60000UL; // 转换为毫秒
      if (millis() - presenceOnStartTime > presenceTimeout) {
        Serial.println("人在开机超时，系统关机");
        turnSystemOff();
      }
    }
  }
}

void SensorManager::setTimerPowerState(bool on) {
  Config& config = configManager.getConfig();
  
  if (on) {
    // 定时开机请求
    if (systemPowerState == POWER_OFF) {
      // 只有在关机状态下才响应定时开机
      if (!config.presenceWakeEnabled || !presenceState) {
        // 如果没有启用人在开机，或者当前没有人在，则定时开机
        turnSystemOn(POWER_ON_TIMER);
        Serial.println("定时开机");
      }
    }
    // 如果系统已经是开机状态，不需要再次开机
  } else {
    // 定时关机请求
    if (systemPowerState == POWER_ON_TIMER) {
      // 只关闭定时开机状态，人在开机状态不受影响
      if (!config.presenceWakeEnabled || !presenceState) {
        // 如果没有启用人在开机，或者当前没有人在，则关机
        turnSystemOff();
        Serial.println("定时关机");
      }
    }
  }
}

void SensorManager::turnSystemOn(SystemPowerState reason) {
  systemPowerState = reason;
  digitalWrite(BOOST_ENABLE_PIN, HIGH);
  
  // 恢复显示
  if (nixieController.isManualMode()) {
    nixieController.displayManualDigits();
  } else {
    nixieController.displayTime();
  }
  
  // 恢复LED状态
  Config& config = configManager.getConfig();
  if (config.powerState) {
    ledEffects.setPowerState(true);
  }
}

void SensorManager::turnSystemOff() {
  systemPowerState = POWER_OFF;
  digitalWrite(BOOST_ENABLE_PIN, LOW);
  nixieController.clearDisplay();
  ledEffects.setPowerState(false);
}
