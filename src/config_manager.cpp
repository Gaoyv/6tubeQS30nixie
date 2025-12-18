#include "config_manager.h"
#include "hardware_config.h"
#include <Adafruit_NeoPixel.h>

ConfigManager configManager;

ConfigManager::ConfigManager() : shouldSaveConfig(false), lastSaveTime(0) {}

void ConfigManager::begin() {
  EEPROM.begin(sizeof(Config));
  loadConfig();
}

void ConfigManager::loadConfig() {
  EEPROM.get(0, config);
  
  // 验证配置的有效性
  bool configValid = true;
  
  // 检查各个字段的有效性
  if (config.brightness > 100 || 
      config.effect > 4 || 
      config.onHour > 23 || config.onMinute > 59 ||
      config.offHour > 23 || config.offMinute > 59 ||
      config.presenceOnTime > 60 ||
      config.nixieBrightnessManual > 100 ||
      config.timezoneOffset < -43200 || config.timezoneOffset > 50400 ||  // -12到+14小时
      config.antiPoisonMode > 1 ||
      config.antiPoisonInterval > 30 ||
      config.coloneffect > 2) {
    configValid = false;
  }

  // 单独检查 blinkInterval，如果无效则重置为默认值，不影响整体配置有效性
  if (config.blinkInterval != 500 && config.blinkInterval != 1000) {
    config.blinkInterval = 500; // 默认为 500ms (1秒周期)
    shouldSaveConfig = true;    // 标记需要保存
  }
  
  // 如果读取的配置无效，使用默认值
  if (!configValid) {
    Serial.println("配置无效，使用默认配置");
    setDefaultConfig();
    saveConfig();
  } else {
    Serial.println("配置加载成功");
  }
}

void ConfigManager::setDefaultConfig() {
  config.color = (50 << 16) | (50 << 8) | 255; // RGB(50, 50, 255)
  config.brightness = 60;
  config.powerState = true;
  config.effect = EFFECT_SOLID;
  config.timerEnabled = false;
  config.onHour = 8;
  config.onMinute = 0;
  config.offHour = 22;
  config.offMinute = 0;
  config.presenceWakeEnabled = true;
  config.presenceOnTime = 30;        // 默认30分钟
  config.nixieBrightnessAuto = true; // 默认自动亮度
  config.nixieBrightnessManual = 80; // 默认手动亮度80%
  config.is24HourFormat = true;      // 默认24小时制
  config.timezoneOffset = 8 * 3600;  // 默认东八区(北京时间)
  config.antiPoisonEnabled = false;
  config.antiPoisonMode = MODE_SEQUENTIAL;
  config.antiPoisonInterval = 5;
  config.coloneffect = COLON_BLINK;
  config.nixieAutoMinLDR = 0;
  config.nixieAutoMaxLDR = 100;
  config.blinkInterval = 500;
}

void ConfigManager::saveConfig() {
  Serial.println("保存配置到EEPROM...");
  EEPROM.put(0, config);
  EEPROM.commit();
  shouldSaveConfig = false;
  Serial.println("配置保存完成");
}

void ConfigManager::markForSave() {
  shouldSaveConfig = true;
}

void ConfigManager::checkAndSave() {
  if (shouldSaveConfig && millis() - lastSaveTime > CONFIG_SAVE_DELAY) {
    Serial.println("触发定期配置保存");
    saveConfig();
    lastSaveTime = millis();
  }
}
