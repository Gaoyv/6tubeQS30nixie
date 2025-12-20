#include "nixie_controller.h"
#include "hardware_config.h"
#include "config_manager.h"
#include "time_manager.h"

extern Coloneffect coloneffect;
extern bool isblink;

NixieController nixieController;

NixieController::NixieController() 
  : manualMode(false), testMode(false), lastTestUpdate(0), currentTestDigit(0),
    antiPoisonRunning(false), lastAntiPoisonTime(0), antiPoisonStartTime(0), 
    currentAntiPoisonDigit(0), timeDispDelay(0), timeBrightnessDelay(0) {
  
  for (int i = 0; i < 4; i++) {
    manualDigits[i] = -1;
    antiPoisonDigits[i] = 0;
  }
  for (int i = 0; i < 5; i++) {
    data[i] = 0;
  }
}

void NixieController::begin() {
  pinMode(SHCP, OUTPUT);
  pinMode(DS, OUTPUT);
  pinMode(STCP, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(BOOST_ENABLE_PIN, OUTPUT);
  pinMode(COLON_PIN, OUTPUT);
  
  digitalWrite(COLON_PIN, LOW);
  analogWriteFreq(100000);
  
  clearDisplay();
  delay(1000); // 等待辉光管预热
  digitalWrite(BOOST_ENABLE_PIN, HIGH);
}

void NixieController::displayTime() {
  if (millis() - timeDispDelay > 1000) {
    timeDispDelay = millis();

    tmElements_t now = timeManager.getCurrentTime();
    int hours = now.Hour;
    int minutes = now.Minute;
    
    Config& config = configManager.getConfig();
    
    // 处理12/24小时制
    if (!config.is24HourFormat) {
      // 12小时制转换
      if (hours == 0) {
        hours = 12;  // 午夜12点
      } else if (hours > 12) {
        hours -= 12; // 下午时间
      }
      // hours == 12 时保持12（中午12点）
    }

    displayNumber(hours / 10, 0);
    displayNumber(hours % 10, 1);
    displayNumber(minutes / 10, 2);
    displayNumber(minutes % 10, 3);

    updateShiftRegisters();
  }
}

void NixieController::displayManualDigits() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    
    clearDisplay();
    for (int i = 0; i < 4; i++) {
      if (manualDigits[i] >= 0 && manualDigits[i] <= 9) {
        displayNumber(manualDigits[i], i);
      }
    }
    updateShiftRegisters();
  }
}

void NixieController::setBrightness() {
  if (millis() - timeBrightnessDelay > 200) {
    timeBrightnessDelay = millis();
    
    Config& config = configManager.getConfig();
    int pwmValue;
    
    if (config.nixieBrightnessAuto) {
      // 自动亮度模式：使用环境光传感器
      int adcValue = analogRead(A0);
      
      // 使用map函数进行线性映射
      pwmValue = map(adcValue, 0, 1023, config.nixieAutoMinLDR, config.nixieAutoMaxLDR);
      
    } else {
      // 手动亮度模式：使用用户设置的亮度百分比
      // 将0-100%映射到50-255的PWM值范围
      pwmValue = map(config.nixieBrightnessManual, 0, 100, 1, 100);
    }
    
    analogWrite(PWM_PIN, pwmValue);
  }
}

void NixieController::handleBlink() {
  static unsigned long lastBlink = 0;
  Config& config = configManager.getConfig();
  
  if (millis() - lastBlink > config.blinkInterval) {
    switch (coloneffect) {
      case COLON_DISABLE:
        setDot(false);
        break;
      case COLON_ENABLE:
        setDot(true);
        break;
      case COLON_BLINK:
        isblink = !isblink;
        setDot(isblink);
        break;
    }
    lastBlink = millis();
  }
}

void NixieController::clearDisplay() {
  for (int i = 0; i < 5; i++) {
    data[i] = 0;
  }
  updateShiftRegisters();
}

void NixieController::setManualMode(bool enabled) {
  manualMode = enabled;
  if (!enabled) {
    for (int i = 0; i < 4; i++) {
      manualDigits[i] = -1;
    }
  }
}

void NixieController::setManualDigit(int position, int digit) {
  if (position >= 0 && position < 4) {
    if (digit >= 0 && digit <= 9) {
      manualDigits[position] = digit;
    } else {
      manualDigits[position] = -1;
    }
  }
}

void NixieController::runAntiPoison() {
  static unsigned long lastDigitChange = 0;
  if (millis() - lastDigitChange > 100) {
    lastDigitChange = millis();
    
    Config& config = configManager.getConfig();
    AntiPoisonMode mode = static_cast<AntiPoisonMode>(config.antiPoisonMode);
    
    if (mode == MODE_SEQUENTIAL) {
      for (int i = 0; i < 4; i++) {
        antiPoisonDigits[i] = (currentAntiPoisonDigit + i) % 10;
      }
      currentAntiPoisonDigit = (currentAntiPoisonDigit + 1) % 10;
    } else { // MODE_RANDOM
      for (int i = 0; i < 4; i++) {
        antiPoisonDigits[i] = random(0, 10);
      }
    }
    
    clearDisplay();
    for (int i = 0; i < 4; i++) {
      displayNumber(antiPoisonDigits[i], i);
    }
    updateShiftRegisters();
  }
}

void NixieController::displayNumber(int digit, int tubePosition) {
  data[tubePosition] = 0;

  if (digit < 8) {
    data[4] &= ~((1 << ((3 - tubePosition) * 2)) | (1 << ((3 - tubePosition) * 2 + 1)));
    data[tubePosition] |= (1 << (7 - digit));
  } else if (digit == 8) {
    data[4] &= ~((1 << ((3 - tubePosition) * 2)) | (1 << ((3 - tubePosition) * 2 + 1)));
    data[4] |= (1 << ((3 - tubePosition) * 2 + 1));
  } else if (digit == 9) {
    data[4] &= ~((1 << ((3 - tubePosition) * 2)) | (1 << ((3 - tubePosition) * 2 + 1)));
    data[4] |= (1 << ((3 - tubePosition) * 2));
  }
}

// 更新移位寄存器（基于用户提供的驱动代码）
void NixieController::updateShiftRegisters() {
  digitalWrite(STCP, LOW);
  for (int i = 4; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      digitalWrite(SHCP, LOW);
      digitalWrite(DS, (data[i] & (1 << j)) ? HIGH : LOW);
      digitalWrite(SHCP, HIGH);
    }
  }
  digitalWrite(STCP, HIGH);
}

void NixieController::checkAntiPoison() {
  Config& config = configManager.getConfig();
  
  if (antiPoisonRunning) {
    if (millis() - antiPoisonStartTime >= ANTI_POISON_DURATION) {
      antiPoisonRunning = false;
      lastAntiPoisonTime = millis();
    }
  } else if (config.antiPoisonEnabled && 
             millis() - lastAntiPoisonTime >= config.antiPoisonInterval * 60000UL) {
    antiPoisonRunning = true;
    antiPoisonStartTime = millis();
    currentAntiPoisonDigit = 0;
  }
}



void NixieController::setDot(bool on) {
  digitalWrite(COLON_PIN, on ? HIGH : LOW);
}

void NixieController::displayIPAddress(IPAddress ip) {
  clearDisplay();
  int octet = ip[3];
  displayNumber(octet / 100, 1);
  displayNumber((octet / 10) % 10, 2);
  displayNumber(octet % 10, 3);
  updateShiftRegisters();
}

void NixieController::displayIPAddressOctet(int octet, int startPosition) {
  clearDisplay();
  if (octet >= 100) {
      displayNumber(octet / 100, 1);
      displayNumber((octet / 10) % 10, 2);
      displayNumber(octet % 10, 3);
  } else if (octet >= 10) {
      displayNumber(octet / 10, 2);
      displayNumber(octet % 10, 3);
  } else {
      displayNumber(octet, 3);
  }
  updateShiftRegisters();
}

void NixieController::setTestMode(bool enabled) {
  testMode = enabled;
  if (enabled) {
    currentTestDigit = 0;
    lastTestUpdate = millis();
    // 立即显示第一个数字
    clearDisplay();
    for(int i=0; i<4; i++) {
      displayNumber(currentTestDigit, i);
    }
    updateShiftRegisters();
  }
}

void NixieController::runTestSequence() {
  if (!testMode) return;

  if (millis() - lastTestUpdate >= 1000) {
    lastTestUpdate = millis();
    currentTestDigit++;
    
    if (currentTestDigit > 9) {
      testMode = false;
      return;
    }
    
    clearDisplay();
    for(int i=0; i<4; i++) {
      displayNumber(currentTestDigit, i);
    }
    updateShiftRegisters();
  }
}