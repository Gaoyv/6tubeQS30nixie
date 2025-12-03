#include "led_effects.h"
#include "hardware_config.h"
#include "config_manager.h"

extern Adafruit_NeoPixel strip;
LEDEffects ledEffects;

LEDEffects::LEDEffects() 
  : currentColor(0), currentBrightness(BRIGHTNESS), powerState(true),
    lastEffectUpdate(0), rainbowHue(0), breathingValue(128), 
    breathingDirection(true), ledUpdateDelay(0) {}

void LEDEffects::begin() {
  strip.begin();
  strip.setBrightness(0);
  strip.clear();
  strip.show();
}

void LEDEffects::update() {
  // 如果系统关机或亮度为0，不进行任何更新
  if (!powerState || currentBrightness == 0) {
    return;
  }
  
  Config& config = configManager.getConfig();
  unsigned long now = millis();
  bool needUpdate = false;
  
  // 根据不同效果设置不同的更新间隔
  unsigned long updateInterval = 50; // 默认50ms
  
  switch(config.effect) {
    case EFFECT_SOLID:
      updateInterval = 1000000; // 单色效果1秒更新一次即可
      break;
    case EFFECT_BREATHING:
      updateInterval = 30;   // 呼吸效果30ms更新
      break;
    case EFFECT_RAINBOW:
      updateInterval = 50;   // 彩虹效果50ms更新
      break;
    case EFFECT_GRADIENT:
      updateInterval = 1000; // 渐变效果1秒更新一次即可
      break;
    case EFFECT_PULSE:
      updateInterval = 80;   // 脉冲效果80ms更新
      break;
  }
  
  // 检查是否需要更新
  if (now - lastEffectUpdate >= updateInterval) {
    lastEffectUpdate = now;
    needUpdate = true;
    
    switch(config.effect) {
      case EFFECT_SOLID:
        updateSolidEffect();
        break;
      case EFFECT_BREATHING:
        updateBreathingEffect();
        break;
      case EFFECT_RAINBOW:
        updateRainbowEffect();
        break;
      case EFFECT_GRADIENT:
        updateGradientEffect();
        break;
      case EFFECT_PULSE:
        updatePulseEffect();
        break;
    }
  }
  
  // 只有在需要更新时才调用strip.show()
  if (needUpdate) {
    strip.show();
  }
}

void LEDEffects::setPowerState(bool state) {
  if (powerState != state) {  // 只有状态改变时才更新
    powerState = state;
    
    if (powerState) {
      strip.setBrightness(currentBrightness);
      // 立即应用当前效果
      Config& config = configManager.getConfig();
      switch(config.effect) {
        case EFFECT_SOLID:
          updateSolidEffect();
          break;
        case EFFECT_BREATHING:
          updateBreathingEffect();
          break;
        case EFFECT_RAINBOW:
          updateRainbowEffect();
          break;
        case EFFECT_GRADIENT:
          updateGradientEffect();
          break;
        case EFFECT_PULSE:
          updatePulseEffect();
          break;
      }
    } else {
      strip.setBrightness(0);
      strip.clear();
    }
    
    strip.show();
  }
}

void LEDEffects::setColor(uint32_t color) {
  if (currentColor != color) {  // 只有颜色改变时才更新
    currentColor = color;
    
    // 如果是静态效果，立即更新显示
    Config& config = configManager.getConfig();
    if (config.effect == EFFECT_SOLID || config.effect == EFFECT_GRADIENT) {
      if (powerState && currentBrightness > 0) {
        if (config.effect == EFFECT_SOLID) {
          updateSolidEffect();
        } else {
          updateGradientEffect();
        }
        strip.show();
      }
    }
  }
}

void LEDEffects::setBrightness(uint8_t brightness) {
  if (currentBrightness != brightness) {  // 只有亮度改变时才更新
    currentBrightness = brightness;
    
    if (powerState) {
      strip.setBrightness(currentBrightness);
      
      if (currentBrightness == 0) {
        strip.clear();
      } else {
        // 立即应用当前效果
        Config& config = configManager.getConfig();
        switch(config.effect) {
          case EFFECT_SOLID:
            updateSolidEffect();
            break;
          case EFFECT_BREATHING:
            updateBreathingEffect();
            break;
          case EFFECT_RAINBOW:
            updateRainbowEffect();
            break;
          case EFFECT_GRADIENT:
            updateGradientEffect();
            break;
          case EFFECT_PULSE:
            updatePulseEffect();
            break;
        }
      }
      
      strip.show();
    }
  }
}

void LEDEffects::setEffect(LightEffect effect) {
  // 重置效果状态
  rainbowHue = 0;
  breathingValue = 128;
  breathingDirection = true;
  lastEffectUpdate = 0; // 重置更新时间，确保立即更新新效果
  
  // 立即应用新效果
  if (powerState && currentBrightness > 0) {
    switch(effect) {
      case EFFECT_SOLID:
        updateSolidEffect();
        break;
      case EFFECT_BREATHING:
        updateBreathingEffect();
        break;
      case EFFECT_RAINBOW:
        updateRainbowEffect();
        break;
      case EFFECT_GRADIENT:
        updateGradientEffect();
        break;
      case EFFECT_PULSE:
        updatePulseEffect();
        break;
    }
    strip.show();
  }
}

void LEDEffects::updateSolidEffect() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, currentColor);
  }
}

void LEDEffects::updateBreathingEffect() {
  // 更新呼吸值
  if (breathingDirection) {
    breathingValue += 3;  // 稍微加快呼吸速度
    if (breathingValue >= 255) {
      breathingValue = 255;
      breathingDirection = false;
    }
  } else {
    breathingValue -= 3;
    if (breathingValue <= 30) {  // 最小值调整为30，避免完全熄灭
      breathingValue = 30;
      breathingDirection = true;
    }
  }
  
  // 计算调整后的颜色
  uint8_t r = (currentColor >> 16) & 0xFF;
  uint8_t g = (currentColor >> 8) & 0xFF;
  uint8_t b = currentColor & 0xFF;
  
  uint32_t adjustedColor = strip.Color(
    (r * breathingValue) / 255,
    (g * breathingValue) / 255,
    (b * breathingValue) / 255
  );
  
  // 应用到所有LED
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, adjustedColor);
  }
}

void LEDEffects::updateRainbowEffect() {
  // 更新彩虹色相
  rainbowHue += 512;  // 调整彩虹变化速度
  if (rainbowHue >= 65536) {
    rainbowHue = 0;
  }
  
  // 为每个LED设置不同的色相
  for (int i = 0; i < LED_COUNT; i++) {
    uint16_t pixelHue = rainbowHue + (i * 65536L / LED_COUNT);
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }
}

void LEDEffects::updateGradientEffect() {
  uint8_t r = (currentColor >> 16) & 0xFF;
  uint8_t g = (currentColor >> 8) & 0xFF;
  uint8_t b = currentColor & 0xFF;
  
  for (int i = 0; i < LED_COUNT; i++) {
    // 创建从暗到亮的渐变效果
    float intensity = (float)(i + 1) / LED_COUNT;
    
    uint32_t gradientColor = strip.Color(
      (uint8_t)(r * intensity),
      (uint8_t)(g * intensity),
      (uint8_t)(b * intensity)
    );
    
    strip.setPixelColor(i, gradientColor);
  }
}

void LEDEffects::updatePulseEffect() {
  static uint8_t pulsePos = 0;
  static int8_t pulseDirection = 1;
  
  // 更新脉冲位置
  pulsePos += pulseDirection;
  if (pulsePos >= LED_COUNT - 1) {
    pulseDirection = -1;
  } else if (pulsePos <= 0) {
    pulseDirection = 1;
  }
  
  // 计算每个LED的亮度
  for (int i = 0; i < LED_COUNT; i++) {
    int distance = abs(i - pulsePos);
    uint8_t intensity;
    
    if (distance == 0) {
      intensity = 255;  // 脉冲中心最亮
    } else if (distance == 1) {
      intensity = 128;  // 相邻位置中等亮度
    } else if (distance == 2) {
      intensity = 64;   // 再远一点较暗
    } else {
      intensity = 16;   // 其他位置很暗但不完全熄灭
    }
    
    uint8_t r = ((currentColor >> 16) & 0xFF) * intensity / 255;
    uint8_t g = ((currentColor >> 8) & 0xFF) * intensity / 255;
    uint8_t b = (currentColor & 0xFF) * intensity / 255;
    
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}