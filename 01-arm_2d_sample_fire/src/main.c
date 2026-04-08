/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_dma_mgr.h"
#include "hpm_uart_drv.h"
#include "perf_counter.h"
#include "ff.h"
#include <hpm_ewdg_drv.h>
#include "hpm_mchtmr_drv.h"
#include "hpm_ppor_drv.h"
#include "st7789.h"
#include "rtt_port.h"
#include "rtthread.h"
#include "usb_config.h"
#include "key_task.h"
#include "qmi8658.h"
#include "power_control.h"
void rt_hw_board_init(void)
{
    board_init();
    dma_mgr_init();
    board_init_gpio_pins();
    rtt_base_init();
    
}

extern int arm_2d_scene_app_player_init(void);
extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);

int main(void)
{
    init_cycle_counter(false);
    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    cdc_acm_init(0, CONFIG_HPM_USBD_BASE);
    ADC_Init();
    Power_PWM_Init();
    key_init();
    key_encoder_init();
    //qmi8658_task_init();
    arm_2d_scene_app_player_init();

    while(1) {
        key_module_display(1);  
        rt_thread_mdelay(1);
    }

    return 0;
}

int64_t get_system_time_ms(void)
{
    return hpm_csr_get_core_cycle() / (int64_t)clock_get_core_clock_ticks_per_ms();
}


int64_t arm_2d_helper_get_system_timestamp(void)
{
    return hpm_csr_get_core_cycle();
}

uint32_t arm_2d_helper_get_reference_clock_frequency(void)
{
    return clock_get_frequency(clock_cpu0);
}

