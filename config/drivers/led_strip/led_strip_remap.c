/*
 * Copyright (c) 2022-2023 XiNGRZ
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_led_strip_remap

#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/led_strip_remap.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* 原有结构保持不变 */
struct led_strip_remap_indicator {
	const char *label;
	uint32_t *led_indexes;
	uint32_t led_cnt;
};

struct led_strip_remap_indicator_state {
	bool active;
	struct led_rgb color;
};

/* 新增切层灯配置 */
struct layer_led_config {
	const char *label;
	uint32_t *led_indexes;
	uint32_t led_cnt;
	struct led_rgb colors[4]; // 0-3层颜色
};

/* 新增切层灯状态 */
struct layer_led_state {
	uint8_t current_layer;
	bool active;
};

/* 修改后的数据结构 */
struct led_strip_remap_data {
	struct led_rgb *pixels;
	struct led_rgb *output;
	struct led_strip_remap_indicator_state *indicators;
	struct layer_led_state *layer_leds; // 新增切层灯状态
	struct k_mutex lock;
};

/* 修改后的配置结构 */
struct led_strip_remap_config {
	uint32_t chain_length;
	const struct device *led_strip;
	uint32_t led_strip_len;
	const uint32_t *map;
	uint32_t map_len;
	const struct led_strip_remap_indicator *indicators;
	uint32_t indicator_cnt;
	const struct layer_led_config *layer_leds; // 新增切层灯配置
	uint32_t layer_led_cnt;                    // 切层灯数量
};

static int led_strip_remap_apply(const struct device *dev)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	// 1. 应用原始像素
	for (uint32_t i = 0; i < config->map_len; i++) {
		memcpy(&data->output[i], &data->pixels[i], sizeof(struct led_rgb));
	}

	// 2. 应用普通指示器
	const struct led_strip_remap_indicator *indicator;
	struct led_strip_remap_indicator_state *indicator_state;
	for (uint32_t i = 0; i < config->indicator_cnt; i++) {
		indicator = &config->indicators[i];
		indicator_state = &data->indicators[i];

		if (!indicator_state->active) continue;

		for (uint32_t j = 0; j < indicator->led_cnt; j++) {
			memcpy(&data->output[indicator->led_indexes[j]], &indicator_state->color,
			       sizeof(struct led_rgb));
		}
	}

	// 3. 新增：应用切层灯（最高优先级）
	const struct layer_led_config *lcfg;
	struct layer_led_state *lstate;
	for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
		lcfg = &config->layer_leds[i];
		lstate = &data->layer_leds[i];
		
		if (!lstate->active) continue;
		
		// 确保层索引有效 (0-3)
		uint8_t layer = lstate->current_layer % 4;
		
		for (uint32_t j = 0; j < lcfg->led_cnt; j++) {
			memcpy(&data->output[lcfg->led_indexes[j]], 
			      &lcfg->colors[layer],
			      sizeof(struct led_rgb));
		}
	}

	// 4. 更新物理LED
	ret = led_strip_update_rgb(config->led_strip, data->output, config->map_len);

	k_mutex_unlock(&data->lock);

	return ret;
}

/* 原有函数保持不变 */
static int led_strip_remap_update_rgb(const struct device *dev, struct led_rgb *pixels,
				      size_t num_pixels)
{
	/* 保持不变 */
}

static int led_strip_remap_update_channels(const struct device *dev, uint8_t *channels,
					   size_t num_channels)
{
	/* 保持不变 */
}

int led_strip_remap_set(const struct device *dev, const char *label, struct led_rgb *pixel)
{
	/* 保持不变 */
}

int led_strip_remap_clear(const struct device *dev, const char *label)
{
	/* 保持不变 */
}

/* 新增：切层灯设置函数 */
int led_strip_remap_set_layer(const struct device *dev, const char *label, uint8_t layer)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;
	
	k_mutex_lock(&data->lock, K_FOREVER);
	
	for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
		if (label && strcmp(config->layer_leds[i].label, label) != 0) {
			continue;
		}
		
		data->layer_leds[i].current_layer = layer;
		data->layer_leds[i].active = true;
	}
	
	k_mutex_unlock(&data->lock);
	return led_strip_remap_apply(dev);
}

/* 新增：禁用切层灯 */
int led_strip_remap_disable_layer(const struct device *dev, const char *label)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;
	
	k_mutex_lock(&data->lock, K_FOREVER);
	
	for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
		if (label && strcmp(config->layer_leds[i].label, label) != 0) {
			continue;
		}
		
		data->layer_leds[i].active = false;
	}
	
	k_mutex_unlock(&data->lock);
	return led_strip_remap_apply(dev);
}

static int led_strip_remap_init(const struct device *dev)
{
	struct led_strip_remap_data *data = dev->data;
	const struct led_strip_remap_config *config = dev->config;

	k_mutex_init(&data->lock);

	/* 原有验证保持不变 */
	
	// 新增：切层灯配置验证
	const struct layer_led_config *lcfg;
	for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
		lcfg = &config->layer_leds[i];
		for (uint32_t j = 0; j < lcfg->led_cnt; j++) {
			if (lcfg->led_indexes[j] >= config->led_strip_len) {
				LOG_ERR("%s: Layer LED index %d overflows LED strip (%d)",
					dev->name, lcfg->led_indexes[j], config->led_strip_len);
				return -EINVAL;
			}
		}
	}
	
	// 初始化切层灯状态
	for (uint32_t i = 0; i < config->layer_led_cnt; i++) {
		data->layer_leds[i].current_layer = 0;
		data->layer_leds[i].active = true; // 默认激活
	}

	return 0;
}

static const struct led_strip_driver_api led_strip_remap_api = {
	.update_rgb = led_strip_remap_update_rgb,
	.update_channels = led_strip_remap_update_channels,
};

/* 宏定义：切层灯设备树解析 */
#define LAYER_LED_CONFIG(node_id, n)                                                               \
	{                                                                                          \
		.label = DT_PROP(node_id, label),                                                  \
		.led_indexes = layer_led_indexes_##n,                                              \
		.led_cnt = DT_PROP_LEN(node_id, led_indexes),                                      \
		.colors = {                                                                        \
			DT_PROP_BY_IDX(node_id, layer_0_color, {0}),                               \
			DT_PROP_BY_IDX(node_id, layer_1_color, {0}),                               \
			DT_PROP_BY_IDX(node_id, layer_2_color, {0}),                               \
			DT_PROP_BY_IDX(node_id, layer_3_color, {0}),                               \
		},                                                                                 \
	},

#define LAYER_LED_INDEXES(node_id, n)                                                              \
	static uint32_t layer_led_indexes_##n[] = DT_PROP(node_id, led_indexes);

#define LAYER_LED_STATE(node_id)                                                                   \
	{                                                                                          \
		.active = true,                                                                     \
		.current_layer = 0,                                                                 \
	},

/* 修改后的初始化宏 */
#define LED_STRIP_REMAP_INIT(n)                                                                    \
	static struct led_rgb led_strip_remap_pixels_##n[DT_INST_PROP_LEN(n, map)] = { 0 };        \
	static struct led_rgb led_strip_remap_output_##n[DT_INST_PROP_LEN(n, map)] = { 0 };        \
                                                                                                   \
	static struct led_strip_remap_indicator_state led_strip_remap_indicator_states_##n[] = {   \
		DT_INST_FOREACH_CHILD(n, LED_STRIP_REMAP_INDICATOR_STATE)                          \
	};                                                                                         \
                                                                                                   \
	/* 新增：切层灯状态数组 */                                                                 \
	static struct layer_led_state layer_leds_state_##n[DT_NUM_INST_CHILD_IDX(n)];             \
                                                                                                   \
	static struct led_strip_remap_data led_strip_remap_data_##n = {                            \
		.pixels = led_strip_remap_pixels_##n,                                              \
		.output = led_strip_remap_output_##n,                                              \
		.indicators = led_strip_remap_indicator_states_##n,                                \
		.layer_leds = layer_leds_state_##n,                                                \
	};                                                                                         \
                                                                                                   \
	static const uint32_t led_strip_remap_map_##n[] = DT_INST_PROP(n, map);                    \
                                                                                                   \
	DT_INST_FOREACH_CHILD_VARGS(n, LED_STRIP_REMAP_INDICATOR_INDEXES, n)                       \
                                                                                                   \
	static const struct led_strip_remap_indicator led_strip_remap_indicators_##n[] = {         \
		DT_INST_FOREACH_CHILD_VARGS(n, LED_STRIP_REMAP_INDICATOR, n)                       \
	};                                                                                         \
                                                                                                   \
	/* 新增：切层灯索引数组 */                                                                 \
	DT_INST_FOREACH_CHILD_VARGS(n, LAYER_LED_INDEXES, n)                                      \
                                                                                                   \
	/* 新增：切层灯配置数组 */                                                                 \
	static const struct layer_led_config layer_leds_config_##n[] = {                           \
		DT_INST_FOREACH_CHILD_VARGS(n, LAYER_LED_CONFIG, n)                                \
	};                                                                                         \
                                                                                                   \
	static const struct led_strip_remap_config led_strip_remap_config_##n = {                  \
		.chain_length = DT_INST_PROP(n, chain_length),                                     \
		.led_strip = DEVICE_DT_GET(DT_INST_PHANDLE(n, led_strip)),                         \
		.led_strip_len = DT_INST_PROP_BY_PHANDLE(n, led_strip, chain_length),              \
		.map = led_strip_remap_map_##n,                                                    \
		.map_len = DT_INST_PROP_LEN(n, map),                                               \
		.indicators = led_strip_remap_indicators_##n,                                      \
		.indicator_cnt = ARRAY_SIZE(led_strip_remap_indicators_##n),                       \
		.layer_leds = layer_leds_config_##n,                                               \
		.layer_led_cnt = ARRAY_SIZE(layer_leds_config_##n),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, led_strip_remap_init, NULL, &led_strip_remap_data_##n,            \
			      &led_strip_remap_config_##n, POST_KERNEL,                            \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &led_strip_remap_api);

DT_INST_FOREACH_STATUS_OKAY(LED_STRIP_REMAP_INIT)
