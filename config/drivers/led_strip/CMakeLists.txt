# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_strip_remap)

target_sources(app PRIVATE src/led_strip_remap.c)
target_include_directories(app PRIVATE src)
