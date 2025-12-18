#include "web_server.h"
#include "config_manager.h"
#include "time_manager.h"
#include "sensor_manager.h"
#include "hardware_config.h"
#include <ESP8266WiFi.h>

String WebServerManager::generateHTML() {
  Config& config = configManager.getConfig();
  
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>辉光管时钟控制器</title>
  <style>
    :root {
      --primary-color: #3498db;
      --secondary-color: #2c3e50;
      --background-color: #121212;
      --card-color: #1e1e1e;
      --text-color: #f5f5f5;
      --slider-thumb: var(--primary-color);
      --slider-track: #444;
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      font-family: 'Segoe UI', Roboto, sans-serif;
      background-color: var(--background-color);
      color: var(--text-color);
      padding: 16px;
      line-height: 1.5;
      -webkit-tap-highlight-color: transparent;
    }
    
    .container {
      max-width: 100%;
      margin: 0 auto;
    }
    
    .card {
      background-color: var(--card-color);
      border-radius: 12px;
      padding: 16px;
      margin-bottom: 16px;
      box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    }
    
    h1 {
      font-size: 24px;
      margin-bottom: 16px;
      text-align: center;
      color: var(--primary-color);
    }
    
    h2 {
      font-size: 18px;
      margin-bottom: 12px;
      color: var(--text-color);
    }
    
    .btn {
      display: inline-block;
      background-color: var(--primary-color);
      color: white;
      border: none;
      border-radius: 8px;
      padding: 12px 16px;
      font-size: 16px;
      font-weight: 500;
      text-align: center;
      text-decoration: none;
      cursor: pointer;
      width: 100%;
      margin: 4px 0;
      transition: background-color 0.2s;
    }
    
    .btn:active {
      background-color: #2980b9;
    }
    
    .btn-off {
      background-color: #e74c3c;
    }
    
    .btn-off:active {
      background-color: #c0392b;
    }
    
    .btn-effect {
      background-color: var(--secondary-color);
      margin: 4px 0;
    }
    
    .btn-effect:active {
      background-color: #1a252f;
    }
    
    .btn-effect.selected {
      background-color: var(--primary-color);
      color: white;
    }
    
    .color-picker {
      width: 100%;
      height: 60px;
      margin: 12px 0;
      border: none;
      border-radius: 8px;
      cursor: pointer;
    }
    
    .slider-container {
      margin: 16px 0;
    }
    
    .slider-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 8px;
    }
    
    input[type="range"] {
      -webkit-appearance: none;
      width: 100%;
      height: 8px;
      border-radius: 4px;
      background: var(--slider-track);
      outline: none;
    }
    
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--slider-thumb);
      cursor: pointer;
    }
    
    .digit-input {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 8px;
      margin: 16px 0;
    }
    
    .digit-input input {
      width: 100%;
      padding: 12px;
      font-size: 16px;
      text-align: center;
      border-radius: 8px;
      border: none;
      background-color: #333;
      color: white;
    }
    
    @media (min-width: 480px) {
      .container {
        max-width: 400px;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>辉光管时钟控制器</h1>
       
    <div class="card">
      <h2>RGB背景灯</h2>
      <button class="btn)=====";
  html += config.powerState ? "" : " btn-off";
  html += R"=====(" id="rgbPowerBtn" onclick="toggleRgbPower()">)=====";
  html += config.powerState ? "RGB灯 开" : "RGB灯 关";
  html += R"=====(</button>
    </div>
    
    <div class="card">
      <h2>背景灯颜色</h2>
      <input type="color" id="colorPicker" class="color-picker" value="#)=====";
  // 将配置中的颜色转换为十六进制字符串
  uint8_t r = (config.color >> 16) & 0xFF;
  uint8_t g = (config.color >> 8) & 0xFF;
  uint8_t b = config.color & 0xFF;
  char colorHex[8];
  sprintf(colorHex, "%02x%02x%02x", r, g, b);
  html += String(colorHex);
  html += R"=====(" oninput="updateColor(this.value)">
      
      <div class="slider-container">
        <div class="slider-label">
          <span>亮度</span>
          <span id="brightnessValue">)=====";
  html += String(config.brightness) + "%";
  html += R"=====(</span>
        </div>
        <input type="range" id="brightness" class="slider" min="0" max="100" value=")=====";
  html += String(config.brightness);
  html += R"=====(" oninput="updateBrightness(this.value)">
      </div>
    </div>

    <div class="card">
      <h2>辉光管亮度控制</h2>
      <div class="slider-container">
        <label><input type="checkbox" id="nixieBrightnessAuto" onchange="toggleNixieBrightnessMode()" )=====";
  html += config.nixieBrightnessAuto ? "checked" : "";
  html += R"=====(> 自动亮度模式</label>
      </div>
      
      <div id="autoBrightnessSettings" style="display: )=====";
  html += config.nixieBrightnessAuto ? "block" : "none";
  html += R"=====(;">
        <div class="slider-container">
          <div class="slider-label">
            <span>光敏阈值 (最亮)</span>
            <span id="nixieAutoMinLDRValue">)=====";
  html += String(config.nixieAutoMinLDR);
  html += R"=====(</span>
          </div>
          <input type="range" id="nixieAutoMinLDR" min="1" max="100" value=")=====";
  html += String(config.nixieAutoMinLDR);
  html += R"=====(" oninput="updateNixieAutoMinLDR(this.value)">
        </div>
        <div class="slider-container">
          <div class="slider-label">
            <span>光敏阈值 (最暗)</span>
            <span id="nixieAutoMaxLDRValue">)=====";
  html += String(config.nixieAutoMaxLDR);
  html += R"=====(</span>
          </div>
          <input type="range" id="nixieAutoMaxLDR" min="0" max="99" value=")=====";
  html += String(config.nixieAutoMaxLDR);
  html += R"=====(" oninput="updateNixieAutoMaxLDR(this.value)">
        </div>
      </div>

      <div class="slider-container" id="manualBrightnessContainer">
        <div class="slider-label">
          <span>手动亮度</span>
          <span id="nixieBrightnessValue">)=====";
  html += String(config.nixieBrightnessManual) + "%";
  html += R"=====(</span>
        </div>
        <input type="range" id="nixieBrightnessManual" min="0" max="100" value=")=====";
  html += String(config.nixieBrightnessManual);
  html += R"=====(" oninput="updateNixieBrightness(this.value)">
      </div>
    </div>
    <div class="card">
    <h2>灯光效果</h2>
    <button class="btn btn-effect)=====";
  html += (config.effect == EFFECT_SOLID) ? " selected" : "";
  html += R"=====(" onclick="setEffect(0)">单色常亮</button>
    <button class="btn btn-effect)=====";
  html += (config.effect == EFFECT_BREATHING) ? " selected" : "";
  html += R"=====(" onclick="setEffect(1)">呼吸效果</button>
    <button class="btn btn-effect)=====";
  html += (config.effect == EFFECT_RAINBOW) ? " selected" : "";
  html += R"=====(" onclick="setEffect(2)">彩虹效果</button>
    <button class="btn btn-effect)=====";
  html += (config.effect == EFFECT_GRADIENT) ? " selected" : "";
  html += R"=====(" onclick="setEffect(3)">渐变效果</button>
    <button class="btn btn-effect)=====";
  html += (config.effect == EFFECT_PULSE) ? " selected" : "";
  html += R"=====(" onclick="setEffect(4)">脉冲效果</button>
</div>

    <div class="card">
      <h2>时钟冒号模式</h2>
      <button class="btn btn-effect)=====";
  html += (config.coloneffect == COLON_ENABLE) ? " selected" : "";
  html += R"=====(" onclick="setColonMode(1)">冒号常亮</button>
      <button class="btn btn-effect)=====";
  html += (config.coloneffect == COLON_DISABLE) ? " selected" : "";
  html += R"=====(" onclick="setColonMode(0)">冒号常灭</button>
      <button class="btn btn-effect)=====";
  html += (config.coloneffect == COLON_BLINK) ? " selected" : "";
  html += R"=====(" onclick="setColonMode(2)">冒号闪烁</button>
      
      <div class="slider-container" style="margin-top: 12px;">
        <label>闪烁间隔:</label>
        <select id="blinkInterval" onchange="updateBlinkInterval(this.value)" style="width: 100%; padding: 8px; margin-top: 4px; border-radius: 4px; border: none;">
          <option value="500" )=====";
  html += (config.blinkInterval == 500) ? "selected" : "";
  html += R"=====(>1秒 (亮0.5秒/灭0.5秒)</option>
          <option value="1000" )=====";
  html += (config.blinkInterval == 1000) ? "selected" : "";
  html += R"=====(>2秒 (亮1秒/灭1秒)</option>
        </select>
      </div>
    </div>


<div class="card">
    <h2>定时开关</h2>
    <div class="slider-container">
        <label><input type="checkbox" id="timerEnabled" onchange="toggleTimer()"> 启用定时</label>
    </div>
    <div class="time-input">
        <div>
            <span>开启时间</span>
            <input type="time" id="onTime" onchange="updateTimer()">
        </div>
        <div>
            <span>关闭时间</span>
            <input type="time" id="offTime" onchange="updateTimer()">
        </div>
    </div>
    <div class="slider-container">
        <label><input type="checkbox" id="presenceWake" onchange="togglePresenceWake()"> 人体感应唤醒</label>
    </div>
    <div class="slider-container">
        <div class="slider-label">
            <span>人在开机时间 (分钟)</span>
            <span id="presenceOnTimeValue">)=====";
  html += String(config.presenceOnTime);
  html += R"=====(</span>
        </div>
        <input type="range" id="presenceOnTime" min="0" max="60" value=")=====";
  html += String(config.presenceOnTime);
  html += R"=====(" oninput="updatePresenceOnTime(this.value)">
    </div>
</div>

    <div class="card">
      <h2>网络对时</h2>
      <button id="syncBtn" class="btn" onclick="syncTime()">立即对时</button>
      <p id="syncStatus" style="margin-top: 8px; text-align: center;"></p>
    </div>

        <div class="card">
          <h2>时间显示设置</h2>
          <div class="slider-container">
            <label><input type="checkbox" id="is24HourFormat" onchange="updateTimeFormat()" )=====";
  html += config.is24HourFormat ? "checked" : "";
  html += R"=====(> 24小时制显示</label>
          </div>
          
          <div class="slider-container">
            <div class="slider-label">
              <span>时区偏移 (小时)</span>
              <span id="timezoneOffsetValue">)=====";
  html += String(config.timezoneOffset / 3600);
  html += R"=====(</span>
            </div>
            <input type="range" id="timezoneOffset" min="-12" max="14" value=")=====";
  html += String(config.timezoneOffset / 3600);
  html += R"=====(" oninput="updateTimezoneOffset(this.value)">
            <small>设置您所在时区相对于UTC的偏移小时数</small>
          </div>
        </div>

        <div class="card">
      <h2>辉光管防中毒</h2>
      <div class="slider-container">
        <label><input type="checkbox" id="antiPoisonEnabled" onchange="toggleAntiPoison()" )=====";
  html += config.antiPoisonEnabled ? "checked" : "";
  html += R"=====(> 启用防中毒</label>
      </div>
      
      <div class="slider-container">
        <div class="slider-label">
          <span>间隔时间 (分钟)</span>
          <span id="intervalValue">)=====";
  html += String(config.antiPoisonInterval);
  html += R"=====(</span>
        </div>
        <input type="range" id="antiPoisonInterval" min="1" max="30" value=")=====";
  html += String(config.antiPoisonInterval);
  html += R"=====(" oninput="updateAntiPoisonInterval(this.value)">
      </div>

      
      <div class="slider-container">
        <label>显示模式:</label>
        <select id="antiPoisonMode" onchange="updateAntiPoisonMode()">
          <option value="0" )=====";
  html += (config.antiPoisonMode == MODE_SEQUENTIAL) ? "selected" : "";
  html += R"=====(>顺序显示0-9</option>
          <option value="1" )=====";
  html += (config.antiPoisonMode == MODE_RANDOM) ? "selected" : "";
  html += R"=====(>随机显示0-9</option>
        </select>
      </div>
    </div>
    <div class="card">
      <h2>手动输入数字 (时:分)</h2>
      <div class="digit-input">
        <input type="number" id="digit1" min="0" max="9" placeholder="时十位">
        <input type="number" id="digit2" min="0" max="9" placeholder="时个位">
        <input type="number" id="digit3" min="0" max="9" placeholder="分十位">
        <input type="number" id="digit4" min="0" max="9" placeholder="分个位">
      </div>
      <button class="btn btn-effect" onclick="setManualDigits()">显示输入数字</button>
      <button class="btn btn-effect" onclick="setAutoMode()">恢复自动时钟</button>
    </div>
    
    <div class="card">
      <h2>系统状态</h2>
      <p>WiFi: <span id="wifiStatus">)=====";
  
  html += timeManager.isWifiConnected() ? "已连接 (" + WiFi.localIP().toString() + ")" : "未连接";
  html += R"=====(</span></p>
      <p>RTC时间: <span id="rtcTime">)=====";
  
  tmElements_t now = timeManager.getCurrentTime();
  html += now.Hour < 10 ? "0" + String(now.Hour) : String(now.Hour);
  html += ":";
  html += now.Minute < 10 ? "0" + String(now.Minute) : String(now.Minute);
  html += ":";
  html += now.Second < 10 ? "0" + String(now.Second) : String(now.Second);
  
  html += R"=====(</span></p>
      <p>人体感应: <span id="presenceStatus">)=====";
  html += digitalRead(SENSOR) ? "有人" : "无人";
  html += R"=====(</span></p>
    </div>
  </div>

  <script>

function setColonMode(mode) {
    // 移除所有冒号模式按钮的选中状态
    document.querySelectorAll('.btn-effect').forEach(btn => {
        if (btn.onclick && btn.onclick.toString().includes('setColonMode')) {
            btn.classList.remove('selected');
        }
    });
    
    // 为当前选中的冒号模式按钮添加选中状态
    event.target.classList.add('selected');
    
    fetch('/set?coloneffect=' + mode)
        .catch(err => console.error('Error:', err));
}

function updateBlinkInterval(value) {
    fetch('/set?blinkInterval=' + value)
        .catch(err => console.error('Error:', err));
}

    // 更新颜色
    function updateColor(color) {
      fetch('/set?color=' + color.substring(1))
        .catch(err => console.error('Error:', err));
    }
    
    // 更新亮度
    function updateBrightness(value) {
      document.getElementById('brightnessValue').textContent = value + '%';
      fetch('/set?brightness=' + value)
        .catch(err => console.error('Error:', err));
    }
    
    // 切换RGB灯电源
    function toggleRgbPower() {
      fetch('/power')
        .then(response => response.text())
        .then(state => {
          const btn = document.getElementById('rgbPowerBtn');
          if (state === 'on') {
            btn.textContent = 'RGB灯 开';
            btn.classList.remove('btn-off');
          } else {
            btn.textContent = 'RGB灯 关';
            btn.classList.add('btn-off');
          }
        })
        .catch(err => console.error('Error:', err));
    }
    
    // 切换电源（保留以兼容旧代码）
    function togglePower() {
      toggleRgbPower();
    }
    
    // 设置手动输入数字
    function setManualDigits() {
      const digits = [
        document.getElementById('digit1').value,
        document.getElementById('digit2').value,
        document.getElementById('digit3').value,
        document.getElementById('digit4').value
      ];
      
      fetch('/manual?d1=' + digits[0] + '&d2=' + digits[1] + '&d3=' + digits[2] + '&d4=' + digits[3])
        .catch(err => console.error('Error:', err));
    }
    
    // 恢复自动模式
    function setAutoMode() {
      fetch('/manual?auto=1')
        .catch(err => console.error('Error:', err));
    }
        // 设置灯光效果
function setEffect(effect) {
    // 移除所有效果按钮的选中状态
    document.querySelectorAll('.btn-effect').forEach(btn => {
        if (btn.onclick && btn.onclick.toString().includes('setEffect')) {
            btn.classList.remove('selected');
        }
    });
    
    // 为当前选中的效果按钮添加选中状态
    event.target.classList.add('selected');
    
    fetch('/set?effect=' + effect)
        .catch(err => console.error('Error:', err));
}

// 切换定时器
function toggleTimer() {
    const enabled = document.getElementById('timerEnabled').checked;
    fetch('/set?timer=' + (enabled ? '1' : '0'))
        .catch(err => console.error('Error:', err));
}

// 更新定时时间
function updateTimer() {
    const onTime = document.getElementById('onTime').value;
    const offTime = document.getElementById('offTime').value;
    fetch('/set?onTime=' + onTime + '&offTime=' + offTime)
        .catch(err => console.error('Error:', err));
}

// 切换人体感应唤醒
function togglePresenceWake() {
    const enabled = document.getElementById('presenceWake').checked;
    fetch('/set?presenceWake=' + (enabled ? '1' : '0'))
        .catch(err => console.error('Error:', err));
}

// 更新人在开机时间
function updatePresenceOnTime(value) {
    document.getElementById('presenceOnTimeValue').textContent = value;
    fetch('/set?presenceOnTime=' + value)
        .catch(err => console.error('Error:', err));
}

// 切换辉光管亮度模式
function toggleNixieBrightnessMode() {
    const isAuto = document.getElementById('nixieBrightnessAuto').checked;
    const manualContainer = document.getElementById('manualBrightnessContainer');
    const autoSettings = document.getElementById('autoBrightnessSettings');
    
    // 显示/隐藏手动亮度控制
    manualContainer.style.display = isAuto ? 'none' : 'block';
    autoSettings.style.display = isAuto ? 'block' : 'none';
    
    fetch('/set?nixieBrightnessAuto=' + (isAuto ? '1' : '0'))
        .catch(err => console.error('Error:', err));
}

// 更新辉光管自动亮度阈值
function updateNixieAutoMinLDR(value) {
    document.getElementById('nixieAutoMinLDRValue').textContent = value;
    fetch('/set?nixieAutoMinLDR=' + value)
        .catch(err => console.error('Error:', err));
}

function updateNixieAutoMaxLDR(value) {
    document.getElementById('nixieAutoMaxLDRValue').textContent = value;
    fetch('/set?nixieAutoMaxLDR=' + value)
        .catch(err => console.error('Error:', err));
}

// 更新辉光管手动亮度
function updateNixieBrightness(value) {
    document.getElementById('nixieBrightnessValue').textContent = value + '%';
    fetch('/set?nixieBrightnessManual=' + value)
        .catch(err => console.error('Error:', err));
}

// 更新时间格式
function updateTimeFormat() {
    const is24Hour = document.getElementById('is24HourFormat').checked;
    fetch('/set?is24HourFormat=' + (is24Hour ? '1' : '0'))
        .catch(err => console.error('Error:', err));
}

// 更新时区偏移
function updateTimezoneOffset(value) {
    document.getElementById('timezoneOffsetValue').textContent = value;
    // 将小时转换为秒数
    const offsetSeconds = parseInt(value) * 3600;
    fetch('/set?timezoneOffset=' + offsetSeconds)
        .catch(err => console.error('Error:', err));
}

// 初始化页面时设置当前值
document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('timerEnabled').checked = )=====";
html += config.timerEnabled ? "true" : "false";
html += R"=====(;
    document.getElementById('presenceWake').checked = )=====";
html += config.presenceWakeEnabled ? "true" : "false";
html += R"=====(;
    document.getElementById('nixieBrightnessAuto').checked = )=====";
html += config.nixieBrightnessAuto ? "true" : "false";
html += R"=====(;
    document.getElementById('is24HourFormat').checked = )=====";
html += config.is24HourFormat ? "true" : "false";
html += R"=====(;
    
    // 初始化辉光管亮度控制显示状态
    const manualContainer = document.getElementById('manualBrightnessContainer');
    const autoSettings = document.getElementById('autoBrightnessSettings');
    manualContainer.style.display = )=====";
html += config.nixieBrightnessAuto ? "'none'" : "'block'";
html += R"=====(;
    autoSettings.style.display = )=====";
html += config.nixieBrightnessAuto ? "'block'" : "'none'";
html += R"=====(;
    
    // 格式化时间为HH:MM
    function formatTime(hour, minute) {
        return (hour < 10 ? '0' + hour : hour) + ':' + (minute < 10 ? '0' + minute : minute);
    }
    
    document.getElementById('onTime').value = formatTime()=====";
html += config.onHour;
html += R"=====(, )=====";
html += config.onMinute;
html += R"=====();
    document.getElementById('offTime').value = formatTime()=====";
html += config.offHour;
html += R"=====(, )=====";
html += config.offMinute;
html += R"=====();
});

// 防中毒功能相关函数
    function toggleAntiPoison() {
      const enabled = document.getElementById('antiPoisonEnabled').checked;
      fetch('/set?antiPoison=' + (enabled ? '1' : '0'))
        .catch(err => console.error('Error:', err));
    }
    
    function updateAntiPoisonInterval(value) {
      document.getElementById('intervalValue').textContent = value;
      fetch('/set?antiPoisonInterval=' + value)
        .catch(err => console.error('Error:', err));
    }
    
    function updateAntiPoisonMode() {
      const mode = document.getElementById('antiPoisonMode').value;
      fetch('/set?antiPoisonMode=' + mode)
        .catch(err => console.error('Error:', err));
    }

    function syncTime() {
      const btn = document.getElementById('syncBtn');
      const status = document.getElementById('syncStatus');
      
      btn.disabled = true;
      btn.textContent = '正在对时...';
      status.textContent = '';
      
      fetch('/syncNtp')
        .then(response => {
          if (response.ok) {
            status.textContent = '对时成功';
            status.style.color = '#4caf50';
            setTimeout(() => location.reload(), 1000);
          } else {
            status.textContent = '对时失败';
            status.style.color = '#f44336';
          }
        })
        .catch(err => {
          console.error('Error:', err);
          status.textContent = '请求错误';
          status.style.color = '#f44336';
        })
        .finally(() => {
          btn.disabled = false;
          btn.textContent = '立即对时';
        });
    }
  </script>
</body>
</html>
)=====";
  
  return html;
}
