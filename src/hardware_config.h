#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// 硬件引脚定义
#define SHCP 12          // 移位寄存器时钟引脚
#define DS 14            // 串行数据输入
#define STCP 13          // 存储寄存器时钟引脚
#define PWM_PIN 2        // 辉光管亮度控制PWM引脚
#define BOOST_ENABLE_PIN 15 // 升压电路使能引脚
#define COLON_PIN 16     // 时分冒号控制引脚
#define SDA 4            // I2C SDA
#define SCL 5            // I2C SCL
#define LED_PIN 1        // WS2812数据引脚
#define SENSOR 3         // 人体传感器输入(高电平触发)

// LED配置
#define LED_COUNT 4       // 4个辉光管对应4个WS2812灯
#define BRIGHTNESS 150    // 初始亮度

// 时间常量
#define PRESENCE_TIMEOUT (1 * 60 * 1000) // 1分钟
#define ANTI_POISON_DURATION 1000        // 防中毒显示持续时间1秒
#define CONFIG_SAVE_DELAY 5000           // 配置保存延迟5秒

#endif
