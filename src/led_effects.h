#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "config_manager.h"

class LEDEffects {
public:
  LEDEffects();
  void begin();
  void update();
  void setPowerState(bool state);
  void setColor(uint32_t color);
  void setBrightness(uint8_t brightness);
  void setEffect(LightEffect effect);
  
  bool getPowerState() const { return powerState; }
  uint32_t getCurrentColor() const { return currentColor; }
  uint8_t getCurrentBrightness() const { return currentBrightness; }

private:
  uint32_t currentColor;
  uint8_t currentBrightness;
  bool powerState;
  
  // 效果状态变量
  unsigned long lastEffectUpdate;
  uint16_t rainbowHue;
  uint8_t breathingValue;
  bool breathingDirection;
  unsigned long ledUpdateDelay;
  
  void updateSolidEffect();
  void updateBreathingEffect();
  void updateRainbowEffect();
  void updateGradientEffect();
  void updatePulseEffect();
};

extern LEDEffects ledEffects;

#endif
