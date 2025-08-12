# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

zephyr_library_sources_ifdef(CONFIG_LED_STRIP_REMAP led_strip_remap.c)
zephyr_include_directories(${ZEPHYR_BASE}/drivers) # 确保编译器找到led_strip.h [1,3](@ref)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
zephyr_library()
zephyr_library_sources(led_strip_remap.c)
