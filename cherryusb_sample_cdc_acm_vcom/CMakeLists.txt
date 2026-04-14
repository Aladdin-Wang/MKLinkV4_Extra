# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

SET(STACK_SIZE 0x1000)
SET(HEAP_SIZE 0x6000)

set(APP_NAME "MicroLink")
set(CONFIG_CHERRYUSB 1)
set(CONFIG_CHERRYRB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC_ACM 1)
set(CONFIG_DMA_MGR 1)
set(CONFIG_RTTHREAD_NANO 1)

set(CUSTOM_GCC_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linkers/gcc/flash_xip.ld)
set(CUSTOM_SES_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linkers/segger/flash_xip.icf)
set(CUSTOM_IAR_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linkers/iar/flash_xip.icf)
set(RV_ARCH "rv32imafc")
set(RV_ABI "ilp32f")
find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(mklink_v4_app)

# 设置版本号
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.version)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.version BUILD_VERSION)
    string(STRIP ${BUILD_VERSION} BUILD_VERSION)
    set(CONFIG_BUILD_VERSION ${BUILD_VERSION})
endif ()

sdk_app_inc(${CMAKE_CURRENT_SOURCE_DIR})
sdk_app_inc(config)
sdk_app_inc(src)

sdk_app_src_glob(src/*.c)
sdk_app_src(config/rtconfig.h)


# swj_clock default use keil swd frequency boost
sdk_compile_definitions(-DBOOST_KEIL_SWD_FREQ=0)

if (CONFIG_BUILD_VERSION)
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION="${CONFIG_BUILD_VERSION}")
else ()
    sdk_compile_definitions(-DCONFIG_BUILD_VERSION="Debug")
endif ()

sdk_compile_definitions(-D_RTE_)
sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
sdk_compile_definitions(-DPRODUCT_STRING="MicroLink V4")
sdk_compile_options("-O3")

generate_ide_projects()

