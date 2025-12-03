#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

enum LightEffect {
  EFFECT_SOLID,
  EFFECT_BREATHING,
  EFFECT_RAINBOW,
  EFFECT_GRADIENT,
  EFFECT_PULSE
};

enum AntiPoisonMode {
  MODE_SEQUENTIAL,
  MODE_RANDOM
};

enum Coloneffect {
  COLON_DISABLE,
  COLON_ENABLE,
  COLON_BLINK
};

struct Config {
  uint32_t color;
  uint8_t brightness;
  bool powerState;
  LightEffect effect;
  bool timerEnabled;
  int onHour;
  int onMinute;
  int offHour;
  int offMinute;
  bool presenceWakeEnabled;
  uint8_t presenceOnTime;        // 人在开机时间 (0-60分钟)
  bool nixieBrightnessAuto;      // 辉光管亮度自动模式
  uint8_t nixieBrightnessManual; // 辉光管手动亮度 (0-100%)
  bool is24HourFormat;           // 时间格式：true=24小时制，false=12小时制
  int32_t timezoneOffset;        // 时区偏移秒数 (-43200 到 +50400，即-12到+14小时)
  bool antiPoisonEnabled;
  uint8_t antiPoisonMode;
  uint8_t antiPoisonInterval;
  int coloneffect;
  char ntpServer[64];
  uint16_t nixieAutoMinLDR;      // 自动亮度下限阈值 (对应最亮)
  uint16_t nixieAutoMaxLDR;      // 自动亮度上限阈值 (对应最暗)
};

class ConfigManager {
public:
  ConfigManager();
  void begin();
  void loadConfig();
  void saveConfig();
  void markForSave();
  void checkAndSave();
  
  Config& getConfig() { return config; }
  bool shouldSave() const { return shouldSaveConfig; }

private:
  Config config;
  bool shouldSaveConfig;
  unsigned long lastSaveTime;
  
  void setDefaultConfig();
};

extern ConfigManager configManager;

#endif
