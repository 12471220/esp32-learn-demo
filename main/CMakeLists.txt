idf_component_register(SRCS "esp32-learn-demo.c" "display.c" "led_light.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd esp_lcd_st7735 esp_lvgl_port)
