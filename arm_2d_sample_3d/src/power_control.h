
#ifndef POWER_CONTROL_H
#define POWER_CONTROL_H
#include "stdint.h"

typedef enum {
    USER_ADC_VREF_CHANNEL = 1,
    USER_ADC_IREF_CHANNEL = 2,
    USER_ADC_TVCC_CHANNEL = 11,
} USER_ADC_CHANNEL_t;

#ifdef __cplusplus
extern "C"
{
#endif
int power_task_init(void);
void Beep_PWM_Init(void);
void Power_PWM_Init(void);
void ADC_Init() ;
void Power_Set_TVCC_Voltage(uint16_t voltage) ;
void Beep_PWM_frequency(uint32_t freq,uint8_t duty,uint16_t time);
#ifdef __cplusplus
}
#endif

#endif //HPM5300EVKLITE_DAP_HSLINK_PRO_EXPANSION_H
