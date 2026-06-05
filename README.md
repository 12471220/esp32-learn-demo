# 项目进度

ESP32 + ST7735 130×130 LCD 多功能开发演示项目。

## 已实现功能

### 1. 外设驱动

| 模块 | 文件 | 说明 |
|---|---|---|
| LED 控制 | `led_light.c/h` | 三色 LED（R/G/B）+ 蜂鸣器，参数化 GPIO，支持闪烁 |
| 显示驱动 | `display.c/h` | ST7735 SPI，LEDC PWM 背光，LVGL 渲染 |
| DHT22 传感器 | `esp32-learn-demo.c` | 每 2s 读取温湿度，超阈值触发声光告警 |
| WiFi STA | `wifi_manager.c/h` | 自动连接、断线重连、RSSI 环形数组平滑 |
| HTTP 服务 | `wifi_manager.c` | 端口 8000，GET / 回 "OK"，/light/on /light/off 控制舵机 |
| 舵机控制 | `servo.c/h` | LEDC PWM 50Hz，角度旋转 0-180°，命令队列 + FreeRTOS 任务异步执行 |

### 2. 显示界面

- 4 个 UNSCII_8 标签：WiFi RSSI、IP 地址、温度、湿度
- WiFi 未连接时显示 "--"
- RSSI 转百分比显示
- 屏幕旋转 270°，黑底白字，WiFi 信息绿色

### 3. 告警逻辑

- 温度 > 33°C 或湿度 > 70%：红灯 + 蜂鸣器告警
- 传感器读失败：同样触发告警
- 正常状态：绿灯常亮

### 4. 舵机灯控（异步）

- 舵机运动序列由独立 FreeRTOS 任务 `servo_task` 串行执行，HTTP handler 仅入队命令后立即返回
- `light_on()`: 转动到 58° 后回中 89°，舵机卸力（PWM 关断）
- `light_off()`: 转动到 128° 后回中 89°，舵机卸力（PWM 关断）
- HTTP `/light/on`、`/light/off` 远程触发，响应时间 < 1ms
- `servo_init()` 幂等保护，仅首次执行 LEDC 初始化

### 5. 质量改进

- WiFi RSSI 用环形数组（50 点缓冲）+ 缓存 SUM，O(1) 求平均
- LVGL 操作有 `lvgl_port_lock/unlock` 线程安全保护
- PWM 背光亮度可调 0-100%
- 舵机命令队列（FreeRTOS Queue）确保并发 HTTP 请求串行化
- `servo_task` 栈空间 5120 字节，避免深层调用栈溢出
- 启用 `CONFIG_LV_SPRINTF_USE_FLOAT`，修复 LVGL 标签浮点数显示为 "f" 的问题
- 移除告警定时器，`alarm_start`/`alarm_stop` 直接控制 LED，消除无意义的 500ms 周期回调
- `led_init` 移入 `dht_task`，初始化靠近使用点

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

## 已知问题

### 舵机 light_off 概率性失败（疑似电压跌落）

`light_off` 先转动到 128°（比 `light_on` 的 58° 幅度更大），舵机抽取大电流时 USB 5V 供电跌落，触发 ESP32 brownout reset。

**缓解方向**：在舵机 5V 供电端并接 470µF~1000µF 电解电容，或给舵机独立供电。
