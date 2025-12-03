#include "web_server.h"
#include "config_manager.h"
#include "led_effects.h"
#include "nixie_controller.h"
#include "time_manager.h"
#include "sensor_manager.h"
#include "hardware_config.h"
#include <ESP8266WiFi.h>

ESP8266WebServer server(80);
WebServerManager webServerManager;

WebServerManager::WebServerManager() {}

void WebServerManager::begin() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/set", HTTP_GET, [this]() { handleSet(); });
  server.on("/power", HTTP_GET, [this]() { handlePower(); });
  server.on("/manual", HTTP_GET, [this]() { handleManual(); });
  server.on("/syncNtp", HTTP_GET, [this]() { handleSyncNtp(); });
  server.on("/test", HTTP_GET, [this]() { handleTest(); });
  server.begin();
}

void WebServerManager::handleClient() {
  server.handleClient();
}

void WebServerManager::handleRoot() {
  String html = generateHTML();
  server.send(200, "text/html; charset=utf-8", html);
}

void WebServerManager::handleSet() {
  Serial.println("收到配置设置请求");
  Config& config = configManager.getConfig();
  
  if (server.hasArg("color")) {
    String colorStr = server.arg("color");
    if(colorStr.length() == 6) {
      long color = strtol(colorStr.c_str(), NULL, 16);
      uint32_t newColor = ((color >> 16) & 0xFF) << 16 | ((color >> 8) & 0xFF) << 8 | (color & 0xFF);
      config.color = newColor;
      ledEffects.setColor(newColor);
      configManager.markForSave();
    }
  }

  if (server.hasArg("brightness")) {
    int newBrightness = server.arg("brightness").toInt();
    newBrightness = (newBrightness == 0) ? 0 : max(1, newBrightness);
    config.brightness = newBrightness;
    uint8_t mappedBrightness = map(config.brightness, 0, 100, 0, 255);
    ledEffects.setBrightness(mappedBrightness);
    configManager.markForSave();
  }
  
  if (server.hasArg("effect")) {
    config.effect = static_cast<LightEffect>(server.arg("effect").toInt());
    ledEffects.setEffect(config.effect);
    configManager.markForSave();
  }
  
  if (server.hasArg("timer")) {
    config.timerEnabled = server.arg("timer") == "1";
    configManager.markForSave();
  }

  if (server.hasArg("onTime") && server.hasArg("offTime")) {
    String onTime = server.arg("onTime");
    String offTime = server.arg("offTime");
    
    config.onHour = onTime.substring(0, 2).toInt();
    config.onMinute = onTime.substring(3).toInt();
    config.offHour = offTime.substring(0, 2).toInt();
    config.offMinute = offTime.substring(3).toInt();
    
    configManager.markForSave();
  }

  if (server.hasArg("presenceWake")) {
    config.presenceWakeEnabled = server.arg("presenceWake") == "1";
    configManager.markForSave();
  }

  if (server.hasArg("presenceOnTime")) {
    int time = server.arg("presenceOnTime").toInt();
    if (time >= 0 && time <= 60) {
      config.presenceOnTime = time;
      configManager.markForSave();
    }
  }

  if (server.hasArg("nixieBrightnessAuto")) {
    config.nixieBrightnessAuto = server.arg("nixieBrightnessAuto") == "1";
    configManager.markForSave();
  }

  if (server.hasArg("nixieBrightnessManual")) {
    int brightness = server.arg("nixieBrightnessManual").toInt();
    if (brightness >= 0 && brightness <= 100) {
      config.nixieBrightnessManual = brightness;
      configManager.markForSave();
    }
  }

  if (server.hasArg("nixieAutoMinLDR")) {
    int val = server.arg("nixieAutoMinLDR").toInt();
    if (val >= 0 && val <= 1023) {
      config.nixieAutoMinLDR = val;
      configManager.markForSave();
    }
  }

  if (server.hasArg("nixieAutoMaxLDR")) {
    int val = server.arg("nixieAutoMaxLDR").toInt();
    if (val >= 0 && val <= 1023) {
      config.nixieAutoMaxLDR = val;
      configManager.markForSave();
    }
  }

  if (server.hasArg("antiPoison")) {
    config.antiPoisonEnabled = server.arg("antiPoison") == "1";
    configManager.markForSave();
  }

  if (server.hasArg("antiPoisonMode")) {
    config.antiPoisonMode = server.arg("antiPoisonMode").toInt();
    configManager.markForSave();
  }

  if (server.hasArg("antiPoisonInterval")) {
    config.antiPoisonInterval = server.arg("antiPoisonInterval").toInt();
    configManager.markForSave();
  }
  
  if(server.hasArg("coloneffect")) {
    config.coloneffect = server.arg("coloneffect").toInt();
    // 立即更新全局变量，使冒号效果立即生效
    extern Coloneffect coloneffect;
    coloneffect = static_cast<Coloneffect>(config.coloneffect);
    configManager.markForSave();
  }

  if (server.hasArg("is24HourFormat")) {
    config.is24HourFormat = server.arg("is24HourFormat") == "1";
    configManager.markForSave();
  }

  if (server.hasArg("timezoneOffset")) {
    int32_t offset = server.arg("timezoneOffset").toInt();
    // 验证时区偏移范围 (-12到+14小时)
    if (offset >= -43200 && offset <= 50400) {
      config.timezoneOffset = offset;
      configManager.markForSave();
    }
  }

  if (server.hasArg("ntpServer")) {
    String serverAddr = server.arg("ntpServer");
    if (serverAddr.length() < 64) {
      strcpy(config.ntpServer, serverAddr.c_str());
      configManager.markForSave();
    }
  }

  Serial.println("配置设置请求处理完成");
  server.send(200, "text/plain", "OK");
}

void WebServerManager::handlePower() {
  Config& config = configManager.getConfig();
  bool currentState = ledEffects.getPowerState();
  bool newState = !currentState;
  
  ledEffects.setPowerState(newState);
  config.powerState = newState;  // 保存状态到配置
  configManager.markForSave();   // 标记需要保存配置
  
  server.send(200, "text/plain", newState ? "on" : "off");
}

void WebServerManager::handleManual() {
  if (server.hasArg("auto")) {
    nixieController.setManualMode(false);
  } else {
    nixieController.setManualMode(true);
    for (int i = 0; i < 6; i++) {
      String argName = "d" + String(i+1);
      if (server.hasArg(argName)) {
        int digit = server.arg(argName).toInt();
        nixieController.setManualDigit(i, digit);
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

void WebServerManager::handleSyncNtp() {
  if (timeManager.syncFromNTP()) {
    server.send(200, "text/plain", "success");
  } else {
    server.send(500, "text/plain", "failed");
  }
}

void WebServerManager::handleTest() {
  nixieController.setTestMode(true);
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>辉光管测试</title>
  <style>
    body { font-family: sans-serif; text-align: center; padding: 20px; background: #121212; color: #fff; }
    .card { background: #1e1e1e; padding: 20px; border-radius: 10px; max-width: 400px; margin: 0 auto; }
    h1 { color: #3498db; }
    p { margin: 20px 0; }
    .btn { background: #3498db; color: white; border: none; padding: 10px 20px; border-radius: 5px; text-decoration: none; display: inline-block; }
  </style>
</head>
<body>
  <div class="card">
    <h1>辉光管测试中...</h1>
    <p>正在顺序点亮数字 0-9，每个数字持续 1 秒。</p>
    <p>测试结束后将自动恢复正常显示。</p>
    <a href="/" class="btn">返回主页</a>
  </div>
</body>
</html>
)=====";
  server.send(200, "text/html", html);
}
