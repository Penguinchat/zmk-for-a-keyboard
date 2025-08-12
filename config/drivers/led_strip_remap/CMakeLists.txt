# 配置LED灯带驱动模块的CMake构建规则
zephyr_library()

# 添加LED灯带驱动核心源文件
zephyr_library_sources(
    led_strip_remap.c
)

# 条件编译：支持WS2812系列灯带
if(CONFIG_WS2812_STRIP)
    zephyr_library_sources_ifdef(
        CONFIG_WS2812_STRIP_SPI
        ws2812_spi.c
    )
    zephyr_library_sources_ifdef(
        CONFIG_WS2812_STRIP_I2S
        ws2812_i2s.c
    )
    zephyr_library_sources_ifdef(
        CONFIG_WS2812_STRIP_GPIO
        ws2812_gpio.c
    )
endif()

# 条件编译：支持APA102系列灯带
zephyr_library_sources_ifdef(
    CONFIG_APA102_STRIP
    apa102.c
)

# 条件编译：支持自定义重映射驱动
zephyr_library_sources_ifdef(
    CONFIG_LED_STRIP_REMAP
    led_strip_remap.c
)

# 包含公共头文件目录
zephyr_include_directories(.)

# 链接Zephyr核心驱动库
zephyr_link_libraries(driver_interface)
