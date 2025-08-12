#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zmk/behavior.h>
#include <drivers/led_strip_remap.h> // 包含修改后的 remap 驱动头文件

// 获取 LED 设备实例
const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(led_strip_remap));

// 层切换回调函数
static void layer_change_handler(uint8_t new_layer) {
    led_strip_remap_set_layer(led_dev, "LAYER_LED", new_layer);
}

// 自定义层行为
static int behavior_layer_init(const struct device *dev) {
    // 注册层切换回调
    zmk_keymap_layer_listen(layer_change_handler);
    return 0;
}

// 禁用层指示灯的专用行为
static int on_disable_layer_led(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    led_strip_remap_disable_layer(led_dev, "LAYER_LED");
    return ZMK_BEHAVIOR_OPAQUE;
}

// 行为定义
static const struct behavior_driver_api behavior_layer_driver_api = {
    .binding_pressed = on_disable_layer_led,
};

// 设备定义
DEVICE_DT_DEFINE(DT_NODELABEL(disable_layer_led_behavior),
                behavior_layer_init, NULL, NULL, NULL,
                APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                &behavior_layer_driver_api);
