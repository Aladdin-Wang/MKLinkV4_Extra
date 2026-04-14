/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_dma_mgr.h"
#include "rtt_port.h"
#include "rtthread.h"
#include "usb_config.h"
#include "power_control.h"
void rt_hw_board_init(void)
{
    board_init();
    dma_mgr_init();
    board_init_gpio_pins();
    rtt_base_init();
    
}

extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);

int main(void)
{
    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    cdc_acm_init(0, CONFIG_HPM_USBD_BASE);
    ADC_Init();
    Power_PWM_Init();
    while(1) {
        rt_thread_mdelay(1);
    }

    return 0;
}




