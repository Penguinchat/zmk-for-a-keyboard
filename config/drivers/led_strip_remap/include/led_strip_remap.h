#pragma once
#include <zephyr/drivers/led_strip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置指定标签的层指示灯颜色
 * @param dev LED重映射设备实例
 * @param label 层指示器标签（如"LAYER_INDICATOR"）
 * @param layer 层索引（0-3）
 * @retval 0 成功；其他错误码
 */
int led_strip_remap_set_layer(const struct device *dev, const char *label, uint8_t layer);

/**
 * @brief 禁用指定标签的层指示灯
 * @param dev LED重映射设备实例
 * @param label 层指示器标签
 * @retval 0 成功；其他错误码
 */
int led_strip_remap_disable_layer(const struct device *dev, const char *label);

/**
 * @brief 设置普通指示灯颜色（保留原功能）
 */
int led_strip_remap_set(const struct device *dev, const char *label, const struct led_rgb *pixel);

/**
 * @brief 禁用普通指示灯（保留原功能）
 */
int led_strip_remap_disable(const struct device *dev, const char *label);

#ifdef __cplusplus
}
#endif
