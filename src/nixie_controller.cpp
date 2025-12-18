#include "nixie_controller.h"
#include "hardware_config.h"
#include "config_manager.h"
#include "time_manager.h"

extern Coloneffect coloneffect;
extern bool isblink;

// 创建74HC595移位寄存器对象，10个寄存器
ShiftRegister74HC595<9> sr(DS, SHCP, STCP);
NixieController nixieController;

NixieController::NixieController() 
  : manualMode(false), testMode(false), lastTestUpdate(0), currentTestDigit(0),
    antiPoisonRunning(false), lastAntiPoisonTime(0), antiPoisonStartTime(0), 
    currentAntiPoisonDigit(0), timeDispDelay(0), timeBrightnessDelay(0) {
  
  for (int i = 0; i < 6; i++) {
    manualDigits[i] = -1;
    antiPoisonDigits[i] = 0;
  }
}

void NixieController::begin() {
  pinMode(SHCP, OUTPUT);
  pinMode(DS, OUTPUT);
  pinMode(STCP, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(BOOST_ENABLE_PIN, OUTPUT);
  //pinMode(BLINK, OUTPUT);
  
  //digitalWrite(BLINK, LOW);
  analogWriteFreq(20000);
  
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
    int seconds = now.Second;
    
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

    clearDisplay();
    setNumber(hours / 10, 1);     // 时十位
    setNumber(hours % 10, 2);     // 时个位
    setNumber(minutes / 10, 3);   // 分十位
    setNumber(minutes % 10, 4);   // 分个位
    setNumber(seconds / 10, 5);   // 秒十位
    setNumber(seconds % 10, 6);   // 秒个位

    updateNumbers();
  }
}

void NixieController::displayManualDigits() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    
    clearDisplay();
    for (int i = 0; i < 6; i++) {
      if (manualDigits[i] >= 0 && manualDigits[i] <= 9) {
        setNumber(manualDigits[i], i + 1); // 位置从1开始
      }
    }
    updateNumbers();
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
    updateNumbers();
    lastBlink = millis();
  }
}

void NixieController::clearDisplay() {
  sr.setAllLow();
}

void NixieController::setManualMode(bool enabled) {
  manualMode = enabled;
  if (!enabled) {
    for (int i = 0; i < 6; i++) {
      manualDigits[i] = -1;
    }
  }
}

void NixieController::setManualDigit(int position, int digit) {
  if (position >= 0 && position < 6) {
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
      for (int i = 0; i < 6; i++) {
        antiPoisonDigits[i] = (currentAntiPoisonDigit + i) % 10;
      }
      currentAntiPoisonDigit = (currentAntiPoisonDigit + 1) % 10;
    } else { // MODE_RANDOM
      for (int i = 0; i < 6; i++) {
        antiPoisonDigits[i] = random(0, 10);
      }
    }
    
    clearDisplay();
    for (int i = 0; i < 6; i++) {
      setNumber(antiPoisonDigits[i], i + 1);
    }
    updateNumbers();
  }
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

void NixieController::setNumber(int num, int pos) {
  switch (pos) {
    case 1:
      sr.setNoUpdate(num + 48, HIGH);
      break;
    case 2:
      sr.setNoUpdate(num + 58, HIGH);
      break;
    case 3:
      sr.setNoUpdate(num + 24, HIGH);
      break;
    case 4:
      sr.setNoUpdate(num + 34, HIGH);
      break;
    case 5:
      sr.setNoUpdate(num, HIGH);
      break;
    case 6:
      sr.setNoUpdate(num + 10, HIGH);
      break;
    default:
      break;
  }
}

void NixieController::updateNumbers() {
  sr.updateRegisters();
}

void NixieController::setDot(bool on) {
  sr.setNoUpdate(70, on ? HIGH : LOW);
  sr.setNoUpdate(71, on ? HIGH : LOW);
  sr.setNoUpdate(46, on ? HIGH : LOW);
  sr.setNoUpdate(47, on ? HIGH : LOW);
}

void NixieController::displayIPAddress(IPAddress ip) {
  clearDisplay();
  
  // 在前三个辉光管上同时显示IP地址的前三段
  // 位置1: 第一段的个位
  // 位置2: 第二段的个位  
  // 位置3: 第三段的个位
  
  int octet1 = ip[0];  // 第一段 (如192)
  int octet2 = ip[1];  // 第二段 (如168)
  int octet3 = ip[2];  // 第三段 (如1)
  
  // 显示第一段的个位（位置1）
  setNumber(octet1 % 10, 1);
  
  // 显示第二段的个位（位置2）
  setNumber(octet2 % 10, 2);
  
  // 显示第三段的个位（位置3）
  setNumber(octet3 % 10, 3);
  
  updateNumbers();
}

void NixieController::displayIPAddressOctet(int octet, int startPosition) {
  clearDisplay();
  
  // 在前三个辉光管上显示一个IP地址段（0-255）
  // startPosition参数表示从哪个位置开始显示（1-3）
  
  if (octet >= 100) {
    // 三位数，显示百位、十位、个位（如果空间允许）
    // 根据startPosition决定显示位置
    if (startPosition == 1) {
      // 从位置1开始，显示百位、十位、个位
      setNumber(octet / 100, 1);           // 百位
      setNumber((octet / 10) % 10, 2);     // 十位
      setNumber(octet % 10, 3);            // 个位
    } else if (startPosition == 2) {
      // 从位置2开始，只能显示十位和个位
      setNumber((octet / 10) % 10, 2);     // 十位
      setNumber(octet % 10, 3);            // 个位
    } else {
      // 从位置3开始，只能显示个位
      setNumber(octet % 10, 3);            // 个位
    }
  } else if (octet >= 10) {
    // 两位数，显示十位和个位
    if (startPosition <= 2) {
      setNumber(octet / 10, startPosition);      // 十位
      setNumber(octet % 10, startPosition + 1);  // 个位
    } else {
      // 从位置3开始，只能显示个位
      setNumber(octet % 10, 3);
    }
  } else {
    // 一位数，只显示个位
    if (startPosition <= 3) {
      setNumber(octet, startPosition);
    }
  }
  
  updateNumbers();
}

void NixieController::setTestMode(bool enabled) {
  testMode = enabled;
  if (enabled) {
    currentTestDigit = 0;
    lastTestUpdate = millis();
    // 立即显示第一个数字
    clearDisplay();
    for(int i=0; i<6; i++) {
      setNumber(currentTestDigit, i+1);
    }
    updateNumbers();
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
    for(int i=0; i<6; i++) {
      setNumber(currentTestDigit, i+1);
    }
    updateNumbers();
  }
}