# config/drivers/led_strip/CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)

### 1. 声明驱动库目标
zephyr_library()

### 2. 添加LED重映射驱动源文件
zephyr_library_sources(led_strip_remap.c)

### 3. 包含当前目录头文件
zephyr_include_directories(.)

### 4. 条件编译支持（调试日志）
zephyr_compile_definitions_ifdef(
    CONFIG_LED_STRIP_REMAP_DEBUG
    LED_STRIP_REMAP_DEBUG=1
)

### 5. 依赖声明
zephyr_link_libraries(driver_interface)  # 链接核心驱动接口库[2](@ref)
