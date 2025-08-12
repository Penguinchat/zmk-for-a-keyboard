# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

add_subdirectory(led_strip)
include($ENV{ZEPHYR_BASE}/../drivers/led_strip_remap/Kconfig)
