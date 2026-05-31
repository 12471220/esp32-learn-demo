# problem2: ST7735 屏幕显示雪花噪点

## 现象

`display_test_run()` 运行后，ST7735 屏幕不显示纯色，而是满屏雪花/随机噪点，无法正常显示颜色。

## 结论

`display_fill()` 和 `draw_rectangle()` 错误地将**单像素值**当作**全屏像素数据**传给了 `esp_lcd_panel_draw_bitmap()`，导致 SPI DMA 从栈上读取随机内存发送到屏幕。

## 根因分析

### 错误代码

`display_fill()` 原本的实现：

```c
esp_err_t display_fill(uint16_t color)
{
    if (!panel_handle) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, &color);
}
```

`draw_rectangle()` 同理：

```c
esp_err_t draw_rectangle(int x, int y, int w, int h, uint16_t color)
{
    if (!panel_handle) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + w, y + h, &color);
}
```

### 问题本质

`esp_lcd_panel_draw_bitmap(x_start, y_start, x_end, y_end, data)` 不是"用这个颜色填充区域"的 API，而是"把这些像素数据原样发到屏幕"的 API。

- 参数 `data` 必须指向覆盖整个矩形区域的完整像素缓冲区
- 对于 128×128 的屏幕：需要 128 × 128 × 2 = **32,768 字节** 的像素数据
- 实际传入的 `&color` 只是一个 `uint16_t` 变量的地址，只有 **2 字节**

结果：SPI DMA 从栈上 `color` 变量开始连续读取 32KB，这其中包括了返回地址、其他局部变量、栈帧随机数据——全部作为"像素"发到了屏幕，表现为雪花噪点。

```
栈内存布局示意：

  &color ──► [0xF800]              ← 第1个像素：红色（正确的）
              [0x????]              ← 第2个像素：垃圾数据
              [0x????]              ← 第3个像素：垃圾数据
              ... × 16382 个 ...     ← 全是栈上随机值
              [0x????]              ← SPI DMA 读到这里早超出合法内存
```

### 为什么之前没发现

原始代码来自参考示例，未经修改就提交了，实际烧录测试时才发现问题。

## 修复方案

将"一次性发送全屏数据"改为**按行逐行发送**，每行仅需构造 128×2 = 256 字节的行缓冲区，完全在栈上可控：

```c
esp_err_t display_fill(uint16_t color)
{
    if (!panel_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    static uint16_t line[DISPLAY_WIDTH];
    for (int i = 0; i < DISPLAY_WIDTH; i++)
        line[i] = color;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, DISPLAY_WIDTH, y + 1, line),
            TAG, "fill row %d failed", y);
    }
    return ESP_OK;
}

esp_err_t draw_rectangle(int x, int y, int w, int h, uint16_t color)
{
    if (!panel_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t line[w];
    for (int i = 0; i < w; i++)
        line[i] = color;

    for (int row = 0; row < h; row++) {
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_draw_bitmap(panel_handle, x, y + row, x + w, y + row + 1, line),
            TAG, "draw rect row %d failed", row);
    }
    return ESP_OK;
}
```

### 关键变化

| 项目 | 修复前 | 修复后 |
|------|--------|--------|
| 发送方式 | 一次发送全屏（128×128） | 逐行发送（128 行） |
| 像素缓冲区 | `&color`（2 字节） | `line[128]`（256 字节，每像素都正确填充） |
| DMA 读取 | 读取栈上随机内存 | 读取正确构造的行数据 |
| `display_fill` 行缓冲 | 无 | `static` 避免反复分配 |
| `draw_rectangle` 行缓冲 | 无 | VLA 按需分配 |
| 错误处理 | 无（出错也返回） | `ESP_RETURN_ON_ERROR` 中断并报错 |

## 一句话总结

`esp_lcd_panel_draw_bitmap` 需要完整的像素缓冲区，不能传单个颜色值的地址。改为逐行构建正确的行数据后发送，雪花噪点消失。
