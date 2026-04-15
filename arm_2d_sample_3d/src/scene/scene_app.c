#include "arm_2d_scene_0.h"
#include "arm_2d_scene_user_matrix.h"
#include "ref_gui.h"
#include "st7789.h"
#include "rtt_port.h"
#include "rtthread.h"
#include "board.h"
#include <hpm_ewdg_drv.h>
static void EWDG_Init(void);

ref_gui_t g_tMyGUI;
static ngy_msg_t s_tMSGArray[32];
static ngy_msg_pointer_frontend_pipeline_t s_tPointerPipelines[2];

static char arm_2d_stack[8192];
static struct rt_thread arm_2d_thread;

static const ref_gui_cfg_t c_tCFG = {
    .fnDispAdapterTask = &__disp_adapter0_task,
    .ptDispAdapter = &DISP0_ADAPTER,

    .Message = {
        .ptItems = s_tMSGArray,
        .hwCount = dimof(s_tMSGArray),
    },

    .tKeyFrontend = {
        .u4First3RepeatInterval = 10,
        .u4RestRepeatInterval = 3,
        .u10DoubleClickLimit = 200,
    },

    .tPointerFrontend = {
        .u10DoubleClickLimit = 200,
    },

    .ptPipelines = s_tPointerPipelines,
    .chPipelineNo = dimof(s_tPointerPipelines),
};



void app_init(ref_gui_t *ptGUI)
{
    assert(NULL != ptGUI);

    arm_2d_scene_player_t *ptDispAdapter = rg_get_disp_adapter(ptGUI);
    arm_2d_scene_player_flush_fifo(ptDispAdapter);


    //user_scene_0_t *ptFire
    //    = arm_2d_scene0_init(ptDispAdapter);
    //assert(NULL != ptFire);
    //ptFire->ptGUI = ptGUI;

    user_scene_matrix_t *ptMaterix 
        = arm_2d_scene_matrix_init(ptDispAdapter);
    assert(NULL != ptMaterix);
    ptMaterix->ptGUI = ptGUI;


}

extern void lcd_spi_init(void);


static void arm_2d_thread_entry(void *parameter)
{	
    EWDG_Init();
    lcd_spi_init();
    ST7789_Init();
    arm_irq_safe {
        arm_2d_init();
    }
    disp_adapter0_init();
    rg_init(&g_tMyGUI, (ref_gui_cfg_t *)&c_tCFG);
    app_init(&g_tMyGUI);
    while(1) {
        rg_task(&g_tMyGUI, -1);
        ewdg_refresh(HPM_EWDG0);
    }
}

int arm_2d_scene_app_player_init(void)
{
    rt_thread_init(&arm_2d_thread,
                   "arm2d",
                   arm_2d_thread_entry,
                   RT_NULL,
                   &arm_2d_stack[0],
                   sizeof(arm_2d_stack),
                   25, 10);
    rt_thread_startup(&arm_2d_thread);
    rt_thread_mdelay(1000);
    return RT_EOK;
}


static void EWDG_Init(void) {
    clock_add_to_group(clock_watchdog0, 0);
    ewdg_config_t config;
    ewdg_get_default_config(HPM_EWDG0, &config);

    config.enable_watchdog = true;
    config.int_rst_config.enable_timeout_reset = true;
    config.ctrl_config.use_lowlevel_timeout = false;
    config.ctrl_config.cnt_clk_sel = ewdg_cnt_clk_src_ext_osc_clk;

    /* Set the EWDG reset timeout to 5 second */
    config.cnt_src_freq = 32768;
    config.ctrl_config.timeout_reset_us = 10 * 1000 * 1000;

    /* Initialize the WDG */
    hpm_stat_t status = ewdg_init(HPM_EWDG0, &config);
    if (status != status_success) {
        printf(" EWDG initialization failed, error_code=%d\n", status);
    }
}

