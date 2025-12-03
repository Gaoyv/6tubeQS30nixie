#include "SPI.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <ShiftRegister74HC595.h>
#include <RTC_RX8025T.h>
#include <TimeLib.h>


// 项目模块
#include "hardware_config.h"
#include "config_manager.h"
#include "nixie_controller.h"
#include "led_effects.h"
#include "web_server.h"
#include "time_manager.h"
#include "sensor_manager.h"

// 全局对象
WiFiManager wifiManager;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 全局变量
bool isblink = false;
bool boostEnabled = true;
Coloneffect coloneffect = COLON_BLINK;



void setup() {
  Serial.begin(115200);
  
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect("NixieClockAP");

  // 初始化各个模块
  configManager.begin();
  nixieController.begin();
  ledEffects.begin();
  timeManager.begin();
  sensorManager.begin();
  webServerManager.begin();


  // 初始化OTA
  ArduinoOTA.setHostname("Nixieclock");
  ArduinoOTA.begin();
  
  // 应用加载的配置
  Config& config = configManager.getConfig();
  ledEffects.setColor(config.color);
  ledEffects.setBrightness(map(config.brightness, 0, 100, 0, 255));
  ledEffects.setPowerState(config.powerState);
  coloneffect = static_cast<Coloneffect>(config.coloneffect);
  
  Serial.println("辉光管时钟初始化完成");
  
  // 等待WiFi连接并显示IP地址
  // 等待WiFi连接，最多等待8秒
  unsigned long wifiWaitStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiWaitStart < 8000)) {
    wifiManager.process();
    delay(100);
  }
  
  // 如果WiFi已连接，显示IP地址
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress localIP = WiFi.localIP();
    Serial.print("WiFi已连接，IP地址: ");
    Serial.println(localIP);
    
    // 确保辉光管已开机（如果系统还没有开机）
    // 临时启用辉光管以显示IP地址
    bool wasSystemOff = !sensorManager.isSystemOn();
    digitalWrite(BOOST_ENABLE_PIN, HIGH);
    Serial.println("启用辉光管电源，准备显示IP地址");
    delay(500); // 等待辉光管预热
    
    // 设置辉光管亮度
    int pwmValue;
    if (config.nixieBrightnessAuto) {
      // 自动亮度模式：使用环境光传感器
      int adcValue = analogRead(A0);
      pwmValue = map(adcValue, 0, 1023, 255, 20); 
    } else {
      // 手动亮度模式：使用用户设置的亮度百分比
      // 将0-100%映射到50-255的PWM值范围
      pwmValue = map(config.nixieBrightnessManual, 0, 100, 1, 255);
    }
    analogWrite(PWM_PIN, pwmValue);
    Serial.printf("设置辉光管亮度: PWM=%d\n", pwmValue);
    
    // 在六个辉光管上依次显示IP地址的完整四段
    // 例如192.168.5.28，依次显示192、168、5、28
    // 每个段从位置1开始显示，根据位数占用相应数量的辉光管
    
    // 显示第一段（如192）
    Serial.printf("显示第一段IP: %d\n", localIP[0]);
    nixieController.displayIPAddressOctet(localIP[0], 1);
    delay(2000);
    
    // 显示第二段（如168）
    Serial.printf("显示第二段IP: %d\n", localIP[1]);
    nixieController.displayIPAddressOctet(localIP[1], 1);
    delay(2000);
    
    // 显示第三段（如5）
    Serial.printf("显示第三段IP: %d\n", localIP[2]);
    nixieController.displayIPAddressOctet(localIP[2], 1);
    delay(2000);
    
    // 显示第四段（如28）
    Serial.printf("显示第四段IP: %d\n", localIP[3]);
    nixieController.displayIPAddressOctet(localIP[3], 1);
    delay(2000);
    
    Serial.println("IP地址显示完成");
    
    // 清除显示，准备进入正常时钟模式
    nixieController.clearDisplay();
    delay(500);
    
    // 如果系统原本是关机状态，恢复关机状态
    // 如果系统已经开机，则保持开机状态
    if (wasSystemOff && !sensorManager.isSystemOn()) {
      digitalWrite(BOOST_ENABLE_PIN, LOW);
      Serial.println("系统处于关机状态，关闭辉光管电源");
    } else {
      Serial.println("系统已开机，保持辉光管电源");
    }
  } else {
    Serial.println("WiFi未连接，跳过IP地址显示");
  }
}


void loop() {
  // 更新各个模块
  timeManager.update();
  sensorManager.update();  // 传感器管理器现在负责系统开关机逻辑
  
  // 处理网络相关
  if (timeManager.isWifiConnected()) {
    ArduinoOTA.handle();
    webServerManager.handleClient();
  } else {
    wifiManager.process();
  }
  
  // 只有在系统开机状态下才执行显示相关操作
  if (sensorManager.isSystemOn()) {
    // 设置辉光管亮度
    nixieController.setBrightness();
    
    // 显示时间或手动输入的数字或防中毒显示
    nixieController.checkAntiPoison();
    
    if (nixieController.isTestModeEnabled()) {
      nixieController.runTestSequence();
    } else if (nixieController.isAntiPoisonRunning()) {
      nixieController.runAntiPoison();
    } else if (nixieController.isManualMode()) {
      nixieController.displayManualDigits();
    } else {
      nixieController.displayTime();
    }

    // 处理冒号闪烁
    nixieController.handleBlink();

    // 更新背景灯效果
    ledEffects.update();
  }
  
  // 检查定时开关
  timeManager.checkTimer();
  
  // 定期保存配置
  configManager.checkAndSave();
}