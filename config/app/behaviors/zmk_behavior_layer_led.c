#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <drivers/led_strip_remap.h> // 确保包含修改后的驱动头文件

// 获取LED设备实例
static const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(led_remap));

// 层切换回调函数
static void on_layer_change(uint8_t new_layer, bool active) {
    if (active) {
        // 激活新层时设置指示灯
        led_strip_remap_set_layer(led_dev, "LAYER_INDICATOR", new_layer);
    }
}

// 初始化层监听
static int behavior_layer_init(const struct device *dev) {
    zmk_keymap_layer_listen(on_layer_change);
    return 0;
}

// 禁用层指示的行为处理
static int on_disable_layer_led(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    led_strip_remap_disable_layer(led_dev, "LAYER_INDICATOR");
    return ZMK_BEHAVIOR_OPAQUE;
}

// 行为驱动API
static const struct behavior_driver_api behavior_layer_driver_api = {
    .binding_pressed = on_disable_layer_led,
};

// 设备定义
DEVICE_DT_DEFINE(DT_NODELABEL(disable_layer_led),
                behavior_layer_init, NULL, NULL, NULL,
                APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,
                &behavior_layer_driver_api);
