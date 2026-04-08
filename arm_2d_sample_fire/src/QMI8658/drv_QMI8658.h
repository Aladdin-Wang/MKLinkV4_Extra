#ifndef DRV_QMI8658_H
#define DRV_QMI8658_H
#include "soft_i2c.h"
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// 设备地址
#define Device_Address 0x6B
// RP2040 I2C外设，根据实际硬件修改
#define I2C_PORT i2c0

// IIC接口函数声明
char iic0_read_bytes(unsigned char addr,unsigned char reg, unsigned char *value,unsigned short len);
char iic0_write_bytes(unsigned char addr,unsigned char reg, unsigned char *value,unsigned short len);

#define ACCRANGE  16    //加速度计量程
#define GYRRANGE  2048  //陀螺仪量程
#define SAMPLERATE 800.0f //采样频率 
#define MIN_COLLECTION_COUNT 1000  // 最小采集数据次数
#define USED_DATA_COUNT 50       // 用于计算平均值的数据数量
#define STATIONARY_THRESHOLD 0.001  // 判断静止的加速度模长差值阈值
// 存储陀螺仪校准值
extern float GyrCompensate[6];

#define WHO_AM_I   0x00   //设备ID默认0x05（只读）
#define REVISION_ID  0x01     //设备修订ID默认0x7C（只读）


#define CTRL1      0x02  //配置通讯方式、中断引脚、fifo中断，暂时配置为6C
#define CTRL2      0x03  //配置加速度计
#define CTRL3      0x04  //配置陀螺仪
#define CTRL5      0x06  //设置滤波器
#define CTRL7      0x08  //启用加速度计和陀螺仪
#define CTRL8      0x09  //运动检测
#define CTRL9      0x0A  //CTRL9执行预定指令

#define FIFO_WTM_TH      0x13  //FIFO 水位标记，设置触发值
#define FIFO_CTRL        0x14  //FIFO控制寄存器
#define FIFO_SMPL_CNT    0x15  //FIFO样本计数寄存器
#define FIFO_STATUS      0x16  //FIFO状态寄存器
#define FIFO_DATA        0x17  //FIFO输出寄存器

#define STATUSINT        0x2D  //传感器数据可用和锁存寄存器
#define STATUS1          0x2F  //杂项状态寄存器（运动、步数、点击）
#define TIMESTAMP        0x30  //时间戳（0x30-0x32）

#define TEMP_L           0x33  //温度 = TEMP_H + (TEMP_L / 256)
#define TEMP_H           0x34  

#define A_XYZ            0x35  //加速度输出寄存器(0x35–0x3A)
#define G_XYZ            0x3B  //陀螺仪输出寄存器(0x3B–0x40)

#define COD_STATUS       0x46  //按需校准状态寄存器

#define TAP_STATUS       0x59  //敲击状态寄存器
#define STEP_COUNT       0x5A  //步数计数寄存器（0x5A-0x5C）

#define RESET       0x60  //软件复位寄存器，任何模式写入0xB0复位
#define dQY_L       0x4D  //如果有成功的复位（上电复位或软复位）过程，寄存器 0x4D 的值将等于 0x80

// 主机控制校准寄存器（见 CTRL9，可选择使用）
#define CAL1_L      0x0B
#define CAL1_H      0x0C
#define CAL2_L      0x0D
#define CAL2_H      0x0E
#define CAL3_L      0x0F
#define CAL3_H      0x10
#define CAL4_L      0x11
#define CAL4_H      0x12

// 定义枚举来表示不同的命令序号
typedef enum {
    CTRL_CMD_ACK_ENUM, // 确认命令的枚举值，用于标识CTRL_CMD_ACK命令
    CTRL_CMD_RST_FIFO_ENUM, // 重置FIFO命令的枚举值，用于标识CTRL_CMD_RST_FIFO命令
    CTRL_CMD_REQ_FIFO_ENUM, // 获取FIFO数据命令的枚举值，用于标识CTRL_CMD_REQ_FIFO命令
    CTRL_CMD_WRITE_WOM_SETTING_ENUM, // 设置并启用运动唤醒命令的枚举值，用于标识CTRL_CMD_WRITE_WOM_SETTING命令
    CTRL_CMD_ACCEL_HOST_DELTA_OFFSET_ENUM, // 更改加速度计偏移量命令的枚举值，用于标识CTRL_CMD_ACCEL_HOST_DELTA_OFFSET命令
    CTRL_CMD_GYRO_HOST_DELTA_OFFSET_ENUM, // 更改陀螺仪偏移量命令的枚举值，用于标识CTRL_CMD_GYRO_HOST_DELTA_OFFSET命令
    CTRL_CMD_CONFIGURE_TAP_ENUM, // 配置敲击检测命令的枚举值，用于标识CTRL_CMD_CONFIGURE_TAP命令
    CTRL_CMD_CONFIGURE_PEDOMETER_ENUM, // 配置计步器命令的枚举值，用于标识CTRL_CMD_CONFIGURE_PEDOMETER命令
    CTRL_CMD_CONFIGURE_MOTION_ENUM, // 配置运动检测命令的枚举值，用于标识CTRL_CMD_CONFIGURE_MOTION命令
    CTRL_CMD_RESET_PEDOMETER_ENUM, // 重置计步器步数命令的枚举值，用于标识CTRL_CMD_RESET_PEDOMETER命令
    CTRL_CMD_COPY_USID_ENUM, // 复制USID和固件版本到UI寄存器命令的枚举值，用于标识CTRL_CMD_COPY_USID命令
    CTRL_CMD_SET_RPU_ENUM, // 配置IO上拉电阻命令的枚举值，用于标识CTRL_CMD_SET_RPU命令
    CTRL_CMD_AHB_CLOCK_GATING_ENUM, // 内部AHB时钟门控开关命令的枚举值，用于标识CTRL_CMD_AHB_CLOCK_GATING命令
    CTRL_CMD_ON_DEMAND_CALIBRATION_ENUM, // 对陀螺仪进行按需校准命令的枚举值，用于标识CTRL_CMD_ON_DEMAND_CALIBRATION命令
    CTRL_CMD_APPLY_GYRO_GAINS_ENUM // 恢复保存的陀螺仪增益命令的枚举值，用于标识CTRL_CMD_APPLY_GYRO_GAINS命令
} CommandEnum;

// 定义结构体来存储表格中的每一行信息
typedef struct {
    char commandName[50]; // 存储命令名称，最大长度为50个字符
    int ctrl9CommandValue; // 存储命令在CTRL9寄存器中的值
    char protocolType[10]; // 存储命令使用的协议类型，最大长度为10个字符
    char description[200]; // 存储命令的描述信息，最大长度为200个字符
} CommandInfo;

/**
 * @brief 通过枚举获取对应的命令信息结构体
 * 
 * 该函数根据传入的命令枚举值，返回对应的命令信息结构体。
 * 
 * @param cmd 命令枚举值，用于指定要获取信息的命令
 * @return CommandInfo 包含指定命令详细信息的结构体
 */
CommandInfo getCommandInfo(CommandEnum cmd);

/**
 * @brief 初始化QMI8658A传感器
 * 
 * 该函数用于对QMI8658A传感器进行初始化操作，包括复位、自检、配置等步骤。
 * 
 * @return int 
 *         - 1: 初始化成功
 *         - 0: 初始化失败
 */
int QMI8658A_Init(void);

/**
 * @brief 读取、转换并打印传感器数据
 * 
 * 该函数读取QMI8658A传感器的数据，进行转换后打印输出。
 */
void QMI8658A_ReadConvertAndPrint();

/**
 * @brief 对陀螺仪进行校准
 * 
 * 该函数用于对陀螺仪进行校准操作，并将校准结果存储到传入的数组中。
 * 
 * @param OutData 指向一个长度至少为 3 的 float 类型数组的指针，用于存储校准结果
 * @return uint8_t 
 *         - 1: 校准成功
 *         - 0: 校准失败
 */
uint8_t calibration_ACC_GYR(float *OutData);

/**
 * @brief 读取QMI8658A六轴传感器数据
 * 
 * 该函数用于读取QMI8658A六轴传感器的原始数据，并将其存储到传入的数组中。
 * 
 * @param DATA 指向一个长度为 6 的 int16_t 类型数组的指针，用于存储读取到的原始数据
 * @return int 
 *         - 1: 读取成功
 *         - 0: 读取失败
 */
int QMI8658A_ReadData(int16_t *DATA);

/**
 * @brief 将QMI8658A传感器的原始数据进行转换
 * 
 * 该函数根据加速度计和陀螺仪的量程，将原始数据转换为实际的物理量值。
 * 
 * @param InData 指向包含原始传感器数据的数组的指针，数组长度应为 6
 * @param OutData 指向用于存储转换后数据的数组的指针，数组长度应为 6
 * @param accelRange 加速度计的量程
 * @param gyroRange 陀螺仪的量程
 */
void QMI8658A_ConvertData(int16_t *InData, float *OutData, int accelRange, int gyroRange);

/**
 * @brief 计算加速度的模长
 * 
 * 该函数接收一个包含三轴加速度数据的数组，计算并返回加速度的模长。
 * 
 * @param OutData 指向一个长度至少为 3 的 float 类型数组的指针，数组前三个元素为加速度数据
 * @return float 加速度的模长
 */
float calculateAccelerationMagnitude(float *OutData);

/**
 * @brief 计算陀螺仪平均值的辅助函数
 * 
 * 该函数用于计算陀螺仪在一段时间内采集的多个数据的平均值。
 * 
 * @param DATA 一个二维 float 类型数组，存储了陀螺仪在多个采集点的数据
 * @param OutData 指向一个长度至少为 3 的 float 类型数组的指针，用于存储计算得到的平均值
 */
void calculateGyroAverages(float DATA[MIN_COLLECTION_COUNT][3], float *OutData);

/**
 * @brief 获取单位为g的三轴加速度计和单位为dps的三轴陀螺仪数据
 * 
 * 该函数结合读取和转换操作，获取并存储以g为单位的加速度计数据和以dps为单位的陀螺仪数据。
 * 
 * @param OutData 指向一个长度为 6 的 float 类型数组的指针，用于存储转换后的数据
 */
void QMI8658A_Get_G_DPS(float *OutData);
#endif
