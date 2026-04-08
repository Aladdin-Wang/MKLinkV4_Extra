#include "rtthread.h"
#include "power_control.h"
#include "board.h"
#include "hpm_gptmr_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_adc16_drv.h"
#include <math.h>

ATTR_RAMFUNC_WITH_ALIGNMENT(32) 
static char power_stack[1024];
static struct rt_thread power_thread;
static const uint16_t ADC_REF = 3333;

static GPTMR_Type *const GPTMR0_PWM = HPM_GPTMR0;
static const clock_name_t GPTMR0_PWM_CLK = clock_gptmr0;

static GPTMR_Type *const GPTMR1_PWM = HPM_GPTMR1;
static const clock_name_t GPTMR1_PWM_CLK = clock_gptmr1;

static const uint8_t POWER_PWM_CHANNEL = 1;
static const uint32_t POWER_DEFAULT_PWM_FREQ = 100000;
static const uint32_t POWER_DEFAULT_PWM_DUTY = 90;

static const uint8_t BEEP_PWM_CHANNEL = 2;
static const uint32_t BEEP_DEFAULT_PWM_FREQ = 2000;
static volatile uint16_t beep_time = 0;

static ADC16_Type *const USER_ADC = HPM_ADC0;

static const uint8_t DEFAULT_ADC_RUN_MODE = adc16_conv_mode_oneshot;
static const uint8_t DEFAULT_ADC_CYCLE = 50;

static float  Get_TVCC_Voltage(void);
static float  Get_VREF_Voltage(void);
static float  Get_IREF_Voltage(void) ;

static volatile struct {
    uint32_t reload;
    uint32_t freq_old;
} gptmr0_pwm_state[4],gptmr1_pwm_state[4];

static void set_gptmr0_pwm_waveform_edge_aligned_duty(uint8_t channel, uint32_t freq,uint8_t duty) {
    uint32_t cmp;
    gptmr_channel_config_t config;
    uint32_t gptmr_freq;
    if(freq == 0){
        gptmr_stop_counter(GPTMR0_PWM, channel);
        return;
    }
    gptmr_freq = clock_get_frequency(GPTMR0_PWM_CLK);
    gptmr0_pwm_state[channel].reload = gptmr_freq / freq;
    if(gptmr0_pwm_state[channel].freq_old != freq){
        gptmr_channel_get_default_config(GPTMR0_PWM, &config);
        config.reload = gptmr0_pwm_state[channel].reload;
        config.cmp_initial_polarity_high = true;
        gptmr_stop_counter(GPTMR0_PWM, channel);
        gptmr_channel_config(GPTMR0_PWM, channel, &config, false);
        gptmr_channel_reset_count(GPTMR0_PWM, channel);
        gptmr_start_counter(GPTMR0_PWM, channel);
        gptmr0_pwm_state[channel].freq_old = freq;
    }
    if (duty > 100) {
        duty = 100;
    }
    
    cmp = ((gptmr0_pwm_state[channel].reload * duty) / 100) + 1;
    gptmr_update_cmp(GPTMR0_PWM, channel, 0, cmp);
    gptmr_update_cmp(GPTMR0_PWM, channel, 1, gptmr0_pwm_state[channel].reload);
}

static void set_gptmr1_pwm_waveform_edge_aligned_duty(uint8_t channel,uint32_t freq, uint8_t duty) {
    uint32_t cmp;

    gptmr_channel_config_t config;
    uint32_t gptmr_freq;
    if(freq == 0){
        gptmr_stop_counter(GPTMR1_PWM, channel);
        return;
    }
    gptmr_freq = clock_get_frequency(GPTMR1_PWM_CLK);
    gptmr1_pwm_state[channel].reload = gptmr_freq / freq;
    if(gptmr1_pwm_state[channel].freq_old != freq){
        gptmr_channel_get_default_config(GPTMR1_PWM, &config);
        config.reload = gptmr1_pwm_state[channel].reload;
        config.cmp_initial_polarity_high = true;
        gptmr_stop_counter(GPTMR1_PWM, channel);
        gptmr_channel_config(GPTMR1_PWM, channel, &config, false);
        gptmr_channel_reset_count(GPTMR1_PWM, channel);
        gptmr_start_counter(GPTMR1_PWM, channel);
        gptmr1_pwm_state[channel].freq_old = freq;
    }
    if (duty > 100) {
        duty = 100;
    }
    cmp = ((gptmr1_pwm_state[channel].reload * duty) / 100) + 1;
    gptmr_update_cmp(GPTMR1_PWM, channel, 0, cmp);
    gptmr_update_cmp(GPTMR1_PWM, channel, 1, gptmr1_pwm_state[channel].reload);
}


void Power_PWM_Init(void)
{
    // PB10 100k PWM，占空比80%
    HPM_IOC->PAD[IOC_PAD_PA02].FUNC_CTL = IOC_PA02_FUNC_CTL_GPTMR1_COMP_1;
    HPM_IOC->PAD[IOC_PAD_PA02].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0) | IOC_PAD_PAD_CTL_HYS_SET(1);
    
    set_gptmr1_pwm_waveform_edge_aligned_duty(POWER_PWM_CHANNEL,POWER_DEFAULT_PWM_FREQ,POWER_DEFAULT_PWM_DUTY); 
    Power_Set_TVCC_Voltage(3400);
}

static void beep_time_out(void)
{
    static uint8_t count = 0;

    if(beep_time != 0){
        beep_time--;
        if(beep_time == 0){
            Beep_PWM_frequency(BEEP_DEFAULT_PWM_FREQ,0,0);
        }
    }


  
    //s_tVofaSentMsg.msg.tSvpwm.data[0] = (float)g_tMicroInfo.vcc_voltage;
    //s_tVofaSentMsg.msg.tSvpwm.data[1] = (float)g_tMicroInfo.vref_voltage;
    //s_tVofaSentMsg.msg.tSvpwm.data[2] = (float)g_tMicroInfo.iref_voltage;
    //vofaSent_data(&s_tVofaSentMsg,12);

}

void Beep_PWM_Init(void)
{
   // board_timer_create(10, beep_time_out);
    // PB10 100k PWM，占空比50%
    HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_GPTMR0_COMP_2;
    set_gptmr0_pwm_waveform_edge_aligned_duty(BEEP_PWM_CHANNEL,BEEP_DEFAULT_PWM_FREQ,0);
}

void Beep_PWM_frequency(uint32_t freq,uint8_t duty,uint16_t time)
{
    beep_time = time;
    set_gptmr0_pwm_waveform_edge_aligned_duty(BEEP_PWM_CHANNEL,freq,duty);
}

void Power_Set_TVCC_Voltage(uint16_t voltage) 
{
    // PWM DAC需要输出的电压
    double dac;

    dac =  3900- 0.5 * voltage;
    if (dac < 0) {
        dac = 0;
    } else if (dac > ADC_REF) {
        dac = ADC_REF;
    }

    // 计算PWM占空比
    uint8_t duty = (uint8_t) (dac * 100 / ADC_REF );

    if(voltage < 1.000){
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 3, 0);
    }else{
        set_gptmr1_pwm_waveform_edge_aligned_duty(POWER_PWM_CHANNEL,POWER_DEFAULT_PWM_FREQ,duty);
        rt_thread_delay(1);
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 3, 1);
    }

}

void ADC_Init(void) 
{
    board_init_adc16_pins();
    /* Configure the ADC clock from AHB (@200MHz by default)*/
    clock_set_adc_source(clock_adc0, clk_adc_src_ahb0);

    adc16_config_t cfg;

    /* initialize an ADC instance */
    adc16_get_default_config(&cfg);

    cfg.res = adc16_res_16_bits;
    cfg.conv_mode = DEFAULT_ADC_RUN_MODE;
    cfg.adc_clk_div = adc16_clock_divider_4;
    cfg.sel_sync_ahb = (clk_adc_src_ahb0 == clock_get_source(BOARD_APP_ADC16_CLK_NAME)) ? true : false;

    if (cfg.conv_mode == adc16_conv_mode_sequence ||
        cfg.conv_mode == adc16_conv_mode_preemption) {
        cfg.adc_ahb_en = true;
    }

    /* adc16 initialization */
    if (adc16_init(USER_ADC, &cfg) == status_success) {
        /* enable irq */
        //intc_m_enable_irq_with_priority(BOARD_APP_ADC16_IRQn, 1);
    }

}

static void ADC_Channel_Init(USER_ADC_CHANNEL_t channel) {
    adc16_channel_config_t ch_cfg;

    /* get a default channel config */
    adc16_get_channel_default_config(&ch_cfg);

    /* initialize an ADC channel */
    ch_cfg.ch = channel;
    ch_cfg.sample_cycle = DEFAULT_ADC_CYCLE;

    adc16_init_channel(USER_ADC, &ch_cfg);

    adc16_set_nonblocking_read(USER_ADC);

#if defined(ADC_SOC_BUSMODE_ENABLE_CTRL_SUPPORT) && ADC_SOC_BUSMODE_ENABLE_CTRL_SUPPORT
    /* enable oneshot mode */
    adc16_enable_oneshot_mode(USER_ADC);
#endif
}

static hpm_stat_t Get_ADC_Value(USER_ADC_CHANNEL_t channel,uint16_t *adc_value) {
    ADC_Channel_Init(channel);
    if (adc16_get_oneshot_result(USER_ADC, channel, adc_value) == status_success) {
        if (adc16_is_nonblocking_mode(USER_ADC)) {
            adc16_get_oneshot_result(USER_ADC, channel, adc_value);
        }
        //printf("adc_value = %d \r\n",*adc_value) ;
        return status_success;
    }
    return status_fail;
}

static float inline Get_VREF_Voltage(void) {
    static uint32_t sum = 0;
    static uint16_t buf[100] = {0};
    static uint8_t i = 0;
    uint16_t this_adc;
    if(Get_ADC_Value(USER_ADC_VREF_CHANNEL,&this_adc) == status_success){
        sum += this_adc - buf[i];
        buf[i] = this_adc;
        i = (i + 1) % 10;
    }
    return (float)(2.000 * (sum/10.0000) * ADC_REF / 65535);
}

static uint16_t print_vref(int argc, char **argv)
{
    printf("vref = %fv \r\n",Get_VREF_Voltage()) ;
    return 0;
}
MSH_CMD_EXPORT(print_vref, print vref volage)

static float inline Get_IREF_Voltage(void) {
    static uint32_t sum = 0;
    static uint16_t buf[100] = {0};
    static uint8_t i = 0;
    uint16_t this_adc;
    if(Get_ADC_Value(USER_ADC_IREF_CHANNEL,&this_adc) == status_success){
        sum += this_adc - buf[i];
        buf[i] = this_adc;
        i = (i + 1) % 10;
    }
    return (float)((sum/10.000) * ADC_REF / 65535);
}

static uint16_t print_iref(int argc, char **argv)
{
    printf("iref = %fv \r\n",Get_IREF_Voltage()) ;
    return 0;
}
MSH_CMD_EXPORT(print_iref, print vref volage)

static float inline Get_TVCC_Voltage(void) {
    uint16_t this_adc;
    static uint32_t sum = 0;
    static uint16_t buf[100] = {0};
    static uint8_t i = 0;
    if(Get_ADC_Value(USER_ADC_TVCC_CHANNEL,&this_adc) == status_success){
        sum += this_adc - buf[i];
        buf[i] = this_adc;
        i = (i + 1) % 10;
    }
    return (float)(2.000 * (sum/10.000) * ADC_REF / 65535);
}

static uint16_t print_vcc(int argc, char **argv)
{
    printf("vcc = %fv \r\n",Get_TVCC_Voltage()) ;
    return 0;
}
MSH_CMD_EXPORT(print_vcc, print vcc volage)



static void power_thread_entry(void *parameter)
{
     uint16_t vcc_diff = 0,vref_diff = 0;

     while(1){

        rt_thread_delay(10);
     }
}


int power_task_init(void)
{
    rt_thread_init(&power_thread,
                   "power",
                   power_thread_entry,
                   RT_NULL,
                   &power_stack[0],
                   sizeof(power_stack),
                   20, 100);
    rt_thread_startup(&power_thread);

    return RT_EOK;
}

