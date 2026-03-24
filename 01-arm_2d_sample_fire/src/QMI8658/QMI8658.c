#include "drv_qmi8658.h"
#include "qmi8658.h"
#include "rtthread.h"
#include "stdio.h"

static char qmi8658_stack[4096];
static struct rt_thread usb_qmi8658_thread;

float_t DATA_GY_ACC[6];
static void qmi8658_thread_entry(void *parameter)
{
    int qmi8658_init_ret = QMI8658A_Init();
    printf("qmi8658 ret = %d",qmi8658_init_ret);   	
    while(1) {
        if(qmi8658_init_ret == 1){
            QMI8658A_Get_G_DPS(DATA_GY_ACC);
            printf("%f %f %f\r\n",DATA_GY_ACC[0],DATA_GY_ACC[1],DATA_GY_ACC[2]);
        }
        rt_thread_delay(50);
    }
}


int qmi8658_task_init(void)
{

    rt_thread_init(&usb_qmi8658_thread,
                   "qmi8658",
                   qmi8658_thread_entry,
                   RT_NULL,
                   &qmi8658_stack[0],
                   sizeof(qmi8658_stack),
                   23, 200);
    rt_thread_startup(&usb_qmi8658_thread);


    return RT_EOK;
}





















