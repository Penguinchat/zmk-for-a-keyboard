#pragma once
#include <zephyr/drivers/led_strip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置指定标签的LED颜色
 * @param dev LED重映射设备实例
 * @param label 目标LED组的标识符（如"caps_lock"）
 * @param pixel 待设置的RGB颜色值（不可为NULL）
 * @retval 0 设置成功
 * @retval -EINVAL 参数无效
 * @retval -ENOENT 标签未找到
 */
int led_strip_remap_set(const struct device *dev, const char *label, const struct led_rgb *pixel);

/**
 * @brief 禁用指定标签的LED显示
 * @param dev LED重映射设备实例
 * @param label 目标LED组的标识符
 * @retval 0 禁用成功
 * @retval -EINVAL 参数无效
 */
int led_strip_remap_disable(const struct device *dev, const char *label);

#ifdef __cplusplus
}
#endif
int led_strip_remap_set_layer(const struct device *dev, const char *label, uint8_t layer);
