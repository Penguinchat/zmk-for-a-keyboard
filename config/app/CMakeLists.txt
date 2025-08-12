# 添加自定义行为源文件
target_sources(app PRIVATE
    src/behaviors/zmk_behavior_layer_led.c
)

# 包含LED驱动目录
target_include_directories(app PRIVATE
    ${ZEPHYR_BASE}/drivers/led_strip
    ${ZEPHYR_BASE}/drivers/led_strip_remap
)
