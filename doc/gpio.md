# GPIO 使用情况

| GPIO | 定义 | 功能 | 文件 |
|---|---|---|---|
| 4 | `DHT_GPIO` | DHT11 温湿度传感器 | `esp32-learn-demo.c` |
| 13 | `SERVO_GPIO` | 舵机 PWM 控制 | `servo.h` |
| 16 | `beep_gpio` | 蜂鸣器 | `led_light.h` |
| 18 | `DISPLAY_SCLK` | ST7735 SPI 时钟 | `display.h` |
| 21 | `DISPLAY_BL` | ST7735 背光 PWM | `display.h` |
| 22 | `DISPLAY_RST` | ST7735 复位 | `display.h` |
| 23 | `DISPLAY_MOSI` | ST7735 SPI 数据 | `display.h` |
| 25 | `BLUE_LED` | 蓝色 LED | `led_light.h` |
| 26 | `GREEN_LED` | 绿色 LED | `led_light.h` |
| 27 | `RED_LED` | 红色 LED（告警指示） | `led_light.h` |
| 32 | `DISPLAY_DC` | ST7735 数据/命令选择 | `display.h` |
| 33 | `DISPLAY_CS` | ST7735 片选 | `display.h` |

## 可用引脚

以下引脚未被占用，可直接使用：

| GPIO | 备注 |
|---|---|
| 0, 2 | 启动相关，谨慎使用 |
| 12 | 可用 |
| 14, 15 | 可用（JTAG 相关，但一般可用） |
| 17, 19 | 可用 |
| 5 | 可用（之前被舵机使用，已腾出） |
| 34, 35, 36, 39 | 仅输入，无上拉/下拉能力 |

> 已占用 **12** 个 GPIO。
