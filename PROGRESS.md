# 项目进度

ESP32 + ST7735 130×130 LCD 多功能开发演示项目。

## 已实现功能

### 1. 外设驱动

| 模块 | 文件 | 说明 |
|---|---|---|
| LED 控制 | `led_light.c/h` | 三色 LED（R/G/B）+ 蜂鸣器，参数化 GPIO，支持闪烁 |
| 显示驱动 | `display.c/h` | ST7735 SPI，LEDC PWM 背光，LVGL 渲染 |
| DHT11 传感器 | `esp32-learn-demo.c` | 每 2s 读取温湿度，超阈值触发声光告警 |
| WiFi STA | `wifi_manager.c/h` | 自动连接、断线重连、RSSI 环形数组平滑 |
| HTTP 服务 | `wifi_manager.c` | 端口 8000，GET / 回 "OK"，/light/on /light/off 控制舵机 |
| 舵机控制 | `servo.c/h` | LEDC PWM 50Hz，角度旋转 0-180°，servo_stop() 卸力 |

### 2. 显示界面

- 4 个 UNSCII_8 标签：WiFi RSSI、IP 地址、温度、湿度
- WiFi 未连接时显示 "--"
- RSSI 转百分比显示
- 屏幕旋转 270°，黑底白字，WiFi 信息绿色

### 3. 告警逻辑

- 温度 > 33°C 或湿度 > 70%：红灯 + 蜂鸣器告警
- 传感器读失败：同样触发告警
- 正常状态：绿灯常亮

### 4. 舵机灯控

- `light_on()`: 转动到 53° 后回中 89°，舵机卸力
- `light_off()`: 转动到 138° 后回中 89°，舵机卸力
- HTTP `/light/on`、`/light/off` 远程触发

### 5. 质量改进

- WiFi RSSI 用环形数组（50 点缓冲）+ 缓存 SUM，O(1) 求平均
- LVGL 操作有 `lvgl_port_lock/unlock` 线程安全保护
- PWM 背光亮度可调 0-100%

## 源代码结构

```
main/
├── CMakeLists.txt          # 组件注册
├── esp32-learn-demo.c      # 入口，app_main / dht_task / 告警逻辑
├── display.c/h             # ST7735 显示 + LVGL UI
├── led_light.c/h           # LED / 蜂鸣器 GPIO 控制
├── wifi_manager.c/h        # WiFi STA + HTTP 服务
└── servo.c/h               # 舵机 PWM 控制 + 灯控逻辑
doc/
└── gpio.md                 # GPIO 占用表
```

## 待完成

- [ ] RSSI 环形缓冲区未在任务退出时释放
- [ ] HTTP 灯控接口缺少鉴权
