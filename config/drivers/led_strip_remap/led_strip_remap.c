/* 
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_strip_remap
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <led_strip_remap.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
/* 颜色转换宏（32位RGB转结构体）*/
#define RGB_FROM_UINT32(val) \
    { .r = (uint8_t)((val) >> 16), .g = (uint8_t)((val) >> 8), .b = (uint8_t)(val) }

/* 层指示器配置 */
struct layer_led_config {
    const char *label;
    uint32_t *led_indexes;
    uint32_t led_cnt;
    struct led_rgb colors[4];  // 存储0-3层颜色
};

/* 层指示器状态 */
struct layer_led_state {
    uint8_t current_layer;
    bool active;
};

/* 设备数据结构 */
struct led_strip_remap_data {
    struct led_rgb *pixels;
    struct led_rgb *output;
    struct led_strip_remap_indicator_state *indicators;
    struct layer_led_state *layer_leds;
    struct k_mutex lock;
};

/* 设备配置结构 */
struct led_strip_remap_config {
    uint32_t chain_length;
    const struct device *led_strip;
    uint32_t led_strip_len;
    const uint32_t *map;
    uint32_t map_len;
    const struct led_strip_remap_indicator *indicators;
    uint32_t indicator_cnt;
    const struct layer_led_config *layer_leds;
    uint32_t layer_led_cnt;
};

/* 渲染流水线 */
static int led_strip_remap_apply(const struct device *dev) {
    struct led_strip_remap_data *data = dev->data;
    const struct led_strip_remap_config *config = dev->config;
    k_mutex_lock(&data->lock, K_FOREVER);

    // 1. 基础像素渲染
    for (uint32_t i = 0; i < config->map_len; i++) {
        memcpy(&data->output[i], &data->pixels[i], sizeof(struct led_rgb));
    }

    // 2. 普通指示器渲染
    for (uint32_t i = 0; i < config->indicator_cnt; i++) {
        if (!data->indicators[i].active) continue;
        for (uint32_t j = 0; j < config->indicators[i].led_cnt; j++) {
            memcpy(&data->output[config->indicators[i].led_indexes[j]],
                   &data->indicators[i].color,
                   sizeof(struct led_rgb));
        }
    }

    // 3. 层指示器渲染（最高优先级）
    for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
        if (!data->layer_leds[i].active) continue;
        const struct layer_led_config *cfg = &config->layer_leds[i];
        struct layer_led_state *state = &data->layer_leds[i];
        
        for (uint32_t j = 0; j < cfg->led_cnt; j++) {
            memcpy(&data->output[cfg->led_indexes[j]],
                   &cfg->colors[state->current_layer % 4],
                   sizeof(struct led_rgb));
        }
    }

    // 提交到物理灯带
    int ret = led_strip_update_rgb(config->led_strip, data->output, config->map_len);
    k_mutex_unlock(&data->lock);
    return ret;
}

/* 层指示器控制API */
int led_strip_remap_set_layer(const struct device *dev, const char *label, uint8_t layer) {
    struct led_strip_remap_data *data = dev->data;
    const struct led_strip_remap_config *config = dev->config;
    k_mutex_lock(&data->lock, K_FOREVER);
    
    for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
        if (strcmp(config->layer_leds[i].label, label) == 0) {
            data->layer_leds[i].current_layer = layer % 4;
            data->layer_leds[i].active = true;
            break;
        }
    }
    
    k_mutex_unlock(&data->lock);
    return led_strip_remap_apply(dev);
}

/* 禁用层指示器 */
int led_strip_remap_disable_layer(const struct device *dev, const char *label) {
    struct led_strip_remap_data *data = dev->data;
    const struct led_strip_remap_config *config = dev->config;
    k_mutex_lock(&data->lock, K_FOREVER);
    
    for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
        if (strcmp(config->layer_leds[i].label, label) == 0) {
            data->layer_leds[i].active = false; // 禁用该层指示器
            break;
        }
    }
    
    k_mutex_unlock(&data->lock);
    return led_strip_remap_apply(dev); // 重新渲染
}

/* 初始化函数 */
static int led_strip_remap_init(const struct device *dev) {
    struct led_strip_remap_data *data = dev->data;
    const struct led_strip_remap_config *config = dev->config;
    k_mutex_init(&data->lock);

    // 验证物理灯带长度
    if (config->led_strip_len < config->chain_length) {
        LOG_ERR("Physical strip length %d < chain length %d", 
                config->led_strip_len, config->chain_length);
        return -EINVAL;
    }

    // 初始化层指示器状态
    for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
        data->layer_leds[i].current_layer = 0;
        data->layer_leds[i].active = false; // 默认禁用
    }
    return 0;
}

/* 设备树解析宏 */
#define LAYER_LED_CONFIG(node_id, n)                                           \
{                                                                               \
    .label = DT_PROP(node_id, label),                                           \
    .led_indexes = layer_led_indexes_##n,                                       \
    .led_cnt = DT_PROP_LEN(node_id, led_indexes),                               \
    .colors = {                                                                 \
        RGB_FROM_UINT32(DT_PROP_BY_IDX(node_id, colors, 0)),                    \
        RGB_FROM_UINT32(DT_PROP_BY_IDX(node_id, colors, 1)),                    \
        RGB_FROM_UINT32(DT_PROP_BY_IDX(node_id, colors, 2)),                    \
        RGB_FROM_UINT32(DT_PROP_BY_IDX(node_id, colors, 3))                     \
    },                                                                          \
}

#define LAYER_LED_INDEXES(node_id, n)                                          \
    static uint32_t layer_led_indexes_##n[] = DT_PROP(node_id, led_indexes);

#define LED_STRIP_REMAP_INIT(n)                                                 \
    static struct led_rgb pixels_##n[DT_INST_PROP_LEN(n, map)];                 \
    static struct led_rgb output_##n[DT_INST_PROP_LEN(n, map)];                 \
                                                                                \
    DT_INST_FOREACH_CHILD_VARGS(n, LAYER_LED_INDEXES, n)                        \
                                                                                \
    static const struct layer_led_config layer_leds_cfg_##n[] = {               \
        DT_INST_FOREACH_CHILD_VARGS(n, LAYER_LED_CONFIG, n)                     \
    };                                                                          \
                                                                                \
    static struct layer_led_state layer_leds_state_##n[                        \
        ARRAY_SIZE(layer_leds_cfg_##n)];                                        \
                                                                                \
    static struct led_strip_remap_data data_##n = {                             \
        .pixels = pixels_##n,                                                   \
        .output = output_##n,                                                   \
        .layer_leds = layer_leds_state_##n,                                     \
        .indicators = NULL,  /* 初始化普通指示器为NULL（暂不使用） */           \
    };                                                                          \
                                                                                \
    static const struct led_strip_remap_config cfg_##n = {                      \
        .chain_length = DT_INST_PROP(n, chain_length),                          \
        .led_strip = DEVICE_DT_GET(DT_INST_PHANDLE(n, led_strip)),              \
        .led_strip_len = DT_PROP(DT_INST_PHANDLE(n, led_strip), chain_length),  \
        .map = DT_INST_PROP(n, map),                                             \
        .map_len = DT_INST_PROP_LEN(n, map),                                    \
        .layer_leds = layer_leds_cfg_##n,                                       \
        .layer_led_cnt = ARRAY_SIZE(layer_leds_cfg_##n),                        \
        .indicators = NULL,  /* 普通指示器配置为NULL */                         \
        .indicator_cnt = 0,  /* 普通指示器数量为0 */                            \
    };                                                                          \
                                                                                \
    DEVICE_DT_INST_DEFINE(n, led_strip_remap_init, NULL, &data_##n, &cfg_##n,   \
                          POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,          \
                          NULL);  /* 暂时不需要API结构体，先设为NULL */

DT_INST_FOREACH_STATUS_OKAY(LED_STRIP_REMAP_INIT)
