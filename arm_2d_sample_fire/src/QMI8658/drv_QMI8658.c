#include "drv_qmi8658.h"
#include "stdlib.h"
#include "rtthread.h"

static misaka_soft_i2c_t *s_pMisakaSoftI2c;
// IIC读写函数实现
char iic0_read_bytes(unsigned char addr,unsigned char reg, unsigned char *value,unsigned short len)
{
	uint8_t ret = 0;

	if(  misaka_soft_i2c_master_send_then_recv(s_pMisakaSoftI2c,addr,&reg,1,value,len) < 0){
		ret = 0;
	} else {
		ret = 1;
	}

	return ret;
}
char iic0_write_bytes(unsigned char addr,unsigned char reg, unsigned char *value,unsigned short len)
{
	uint8_t ret = 0;
	uint8_t buf[10];
	buf[0] = reg;
	memcpy(&buf[1],value,len);
        
	if( misaka_soft_i2c_master_send(s_pMisakaSoftI2c,addr,MISAKA_SOFT_I2C_WR,buf,len+1) < 0){
            ret = 0;
	} else {
            ret = 1;
	}
	return ret;
}

// 兼容原代码的IIC读写封装
static unsigned char i2cread(unsigned char addr, unsigned char *Data) {
    return iic0_read_bytes(Device_Address, addr, Data, 1);
}

static unsigned char i2creads(uint8_t addr, uint8_t length, uint8_t *Data) {
    return iic0_read_bytes(Device_Address, addr, Data, length);
}

static unsigned char i2cwrite(uint8_t addr, uint8_t Data) {
    return iic0_write_bytes(Device_Address, addr, &Data, 1);
}

static unsigned char i2cwrites(uint8_t addr, uint8_t length, const uint8_t *Data) {
    return iic0_write_bytes(Device_Address, addr, (uint8_t *)Data, length);
}

uint8_t reg_data__[100];

// 定义一个数组存储所有命令信恿
CommandInfo commandInfos[] = {
    {"CTRL_CMD_ACK", 0x00, "Ctrl9", "15?"},
    {"CTRL_CMD_RST_FIFO", 0x04, "Ctrl9", "14?"},
    {"CTRL_CMD_REQ_FIFO", 0x05, "Ctrl9R", "13?"},
    {"CTRL_CMD_WRITE_WOM_SETTING", 0x08, "WCtrl9", "12?"},
    {"CTRL_CMD_ACCEL_HOST_DELTA_OFFSET", 0x09, "WCtrl9", "11?"},
    {"CTRL_CMD_GYRO_HOST_DELTA_OFFSET", 0x0A, "WCtrl9", "10?"},
    {"CTRL_CMD_CONFIGURE_TAP", 0x0C, "WCtrl9", "9?"},
    {"CTRL_CMD_CONFIGURE_PEDOMETER", 0x0D, "WCtrl9", "8?"},
    {"CTRL_CMD_CONFIGURE_MOTION", 0x0E, "WCtrl9", "7?"},
    {"CTRL_CMD_RESET_PEDOMETER", 0x0F, "WCtrl9", "6?"},
    {"CTRL_CMD_COPY_USID", 0x10, "Ctrl9R", "5?"},
    {"CTRL_CMD_SET_RPU", 0x11, "WCtrl9", "4?"},
    {"CTRL_CMD_AHB_CLOCK_GATING", 0x12, "WCtrl9", "3?"},
    {"CTRL_CMD_ON_DEMAND_CALIBRATION", 0xA2, "WCtrl9", "2?"},
    {"CTRL_CMD_APPLY_GYRO_GAINS", 0xAA, "WCtrl9", "1?"}
};

// 实现函数，通过枚举获取对应的命令信息结构体
CommandInfo getCommandInfo(CommandEnum cmd) {
    return commandInfos[cmd];
}

/*使用示例＿
    // 通过枚举获取命令信息并打卿
    CommandInfo info = getCommandInfo(CTRL_CMD_REQ_FIFO_ENUM);
    printf("Command Name: %s\n", info.commandName);
    printf("CTRL9 Command Value: 0x%X\n", info.ctrl9CommandValue);
    printf("Protocol Type: %s\n", info.protocolType);
    printf("Description: %s\n", info.description);
    */


/**
 * @brief 对加速度计进行自检操作
 * 
 * 此函数通过一系列的I2C操作对加速度计进行自检，包括禁用传感器、设置输出数据速率?
 * 等待特定状态位变化、读取自检结果并判断是否正常?
 * 
 * @param 旿
 * 
 * @return uint8_t 
 *         - 1: 加速度计自检正常
 *         - 0: 加速度计自检异常或在自检过程中出现I2C通信错误
 */
uint8_t Acc_Self_Test()
{
    uint8_t data = 0;

    // 1. 禁用传感噿
    // 向寄存器CTRL7写入0x00以禁用传感器
    if (!i2cwrite(CTRL7, 0x00))
    {
        // 若写入失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "发送禁用传感器命令失败＿);
        return 0;
    }

    // 2. 设置合适的加速度计输出数据速率4G,896.8Hz
    // 向寄存器CTRL2写入0x93以设置加速度计输出数据速率丿G,896.8Hz
    if (!i2cwrite(CTRL2, 0x93))
    {
        // 若写入失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "设置合适的加速度计输出数据速率失败＿);
        return 0;
    }

    // 3. 等待STATUSINT笿位为1
    // 初始化data丿x00
    data = 0x00;
    // 循环读取STATUSINT寄存器，直到笿位为1
    while ((data & 0x01) == 0)
    {
        rt_thread_delay(10);
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示自检失败
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return 0;
        }
    }

    // 4. 将CTRL2.aST（第7位）设为0
    // 读取CTRL2寄存器的值到data
    if (!i2cread(CTRL2, &data))
    {
        // 若读取失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "读取CTRL2失败＿);
        return 0;
    }
    // 将data的第7位清雿
    data &= 0x7F;
    // 将修改后的值写回CTRL2寄存噿
    if (!i2cwrite(CTRL2, data))
    {
        // 若写入失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "设置CTRL2.aST（第7位）设为0失败＿);
        return 0;
    }

    // 5. 等待STATUSINT笿位为0
    // 初始化data丿xFF
    data = 0xFF;
    // 此处逻辑有误，应改为(data & 0x01) != 0，原逻辑(data | 0xFE) == 0不可能成竿
    while ((data & 0x01) != 0)
    {
        rt_thread_delay(10);
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示自检失败
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return 0;
        }
    }

    // 6. 读取加速度计自检结果
    // 定义一个长度为6的数组datas用于存储自检结果
    unsigned char datas[6] = {};
    // 从地址0x51开始连续读叿个字节的数据到datas数组
    if (!i2creads(0x51, 6, datas))
    {
        // 若读取失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "读取加速度计自检结果失败＿);
        return 0;
    }

    // 判断结果绝对值是否高亿00mg
    int16_t dVxData = 0, dVyData = 0, dVzData = 0;
    // 组合低字节和高字节得到完整的x轴数捿
    dVxData = (datas[1] << 8) | datas[0];
    // 组合低字节和高字节得到完整的y轴数捿
    dVyData = (datas[3] << 8) | datas[2];
    // 组合低字节和高字节得到完整的z轴数捿
    dVzData = (datas[5] << 8) | datas[4];

    // 判断x、y、z轴数据的绝对值是否低亿00mg
    if (fabs(dVxData * 0.5) < 200 || fabs(dVyData * 0.5) < 200 || fabs(dVzData * 0.5) < 200)
    {
        // 若有任何一个轴的数据绝对值低亿00mg，记录错误日志并返回0表示自检失败
        // ESP_LOGE(TAG, "读取加速度计自检结果为：异常＿);
        return 0;
    }
    // 若所有轴的数据绝对值都高于200mg，记录日志表示自检正常并返囿
    // ESP_LOGE(TAG, "读取加速度计自检结果为：正常＿);
    return 1;
}


/**
 * @brief 对陀螺仪进行自检操作
 * 
 * 该函数通过一系列I2C通信步骤对陀螺仪进行自检，包含禁用传感器、设置特定寄存器位?
 * 等待状态位变化、读取自检结果并判断是否符合正常范围?
 * 
 * @param 旿
 * @return uint8_t 
 *         - 1: 陀螺仪自检正常
 *         - 0: 陀螺仪自检异常或在自检过程中出现I2C通信错误
 */
uint8_t Gyr_Self_Test()
{
    uint8_t data = 0;

    // 1. 禁用传感噿
    // 向寄存器CTRL7写入0x00，以禁用陀螺仪传感噿
    if (!i2cwrite(CTRL7, 0x00))
    {
        // 若写入失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "发送禁用传感器命令失败＿);
        return 0;
    }

    // 2. 将gST位设丿（CTRL3笿使= 1’b1＿
    // 从寄存器CTRL3读取数据到变量data
    if (!i2cread(CTRL3, &data))
    {
        // 若读取失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "读取CTRL3失败＿);
        return 0;
    }
    // 将data的第7位设置为1
    data |= 0x80;
    // 将修改后的数据写回寄存器CTRL3
    if (!i2cwrite(CTRL3, data))
    {
        // 若写入失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "设置CTRL3.gST（第7位）设为1失败＿);
        return 0;
    }

    // 3. 等待STATUSINT笿位为1
    // 初始化data丿x00
    data = 0x00;
    // 循环读取STATUSINT寄存器，直到其第0位变丿
    while ((data & 0x01) == 0)
    {
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示自检失败
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return 0;
        }
    }

    // 4. 将gST位设丿（CTRL3笿使= 0’b1＿
    // 再次从寄存器CTRL3读取数据到变量data
    if (!i2cread(CTRL3, &data))
    {
        // 若读取失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "读取CTRL3失败＿);
        return 0;
    }
    // 将data的第7位清雿
    data &= 0x7F;
    // 将修改后的数据写回寄存器CTRL3
    if (!i2cwrite(CTRL3, data))
    {
        // 若写入失败，记录错误日志并返囿表示自检失败
        // ESP_LOGE(TAG, "设置CTRL3.gST（第7位）设为0失败＿);
        return 0;
    }

    // 5. 等待STATUSINT笿位为0
    // 初始化data丿xFF
    data = 0xFF;
    // 此处原逻辑(data | 0xFE) == 0有误，应改为(data & 0x01) != 0，以等待STATUSINT笿位为0
    while ((data & 0x01) != 0)
    {
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示自检失败
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return 0;
        }
    }

    // 6. 读取陀螺仪自检结果
    // 定义一个长度为6的数组datas用于存储自检结果
    unsigned char datas[6] = {};
    // 从地址0x51开始连续读叿个字节的数据到datas数组
    if (!i2creads(0x51, 6, datas))
    {
        // 此处注释有误，应是读取陀螺仪自检结果，若读取失败，记录错误日志并返回0表示自检失败
        // ESP_LOGE(TAG, "读取陀螺仪自检结果失败＿);
        return 0;
    }

    // 判断结果绝对值是否高亿00dps
    int16_t dVxData = 0, dVyData = 0, dVzData = 0;
    // 组合低字节和高字节得到完整的x轴数捿
    dVxData = (datas[1] << 8) | datas[0];
    // 组合低字节和高字节得到完整的y轴数捿
    dVyData = (datas[3] << 8) | datas[2];
    // 组合低字节和高字节得到完整的z轴数捿
    dVzData = (datas[5] << 8) | datas[4];

    // 判断x、y、z轴数据经转换后的绝对值是否低亿00dps
    if (fabs(dVxData * 62.5 / 1000) < 300 || fabs(dVyData * 62.5 / 1000) < 300 || fabs(dVzData * 62.5 / 1000) < 300)
    {
        // 若有任何一个轴的数据绝对值低亿00dps，记录错误日志并返回0表示自检失败
        // ESP_LOGE(TAG, "读取陀螺仪自检结果为：异常＿);
        return 0;
    }
    // 若所有轴的数据绝对值都高于300dps，记录日志表示自检正常并返囿
    // ESP_LOGE(TAG, "读取陀螺仪自检结果为：正常＿);
    return 1;
}

/**
 * @brief 对陀螺仪进行按需校准（COD, Calibration On Demand）操使
 * 
 * 此函数用于对陀螺仪进行按需校准，校准过程中建议将设备置于安静环境，
 * 否则校准可能失败并报错。函数通过一系列I2C通信操作完成校准流程＿
 * 包括禁用传感器、发送校准指令、等待校准完成、确认校准结果、检查校准状态，
 * 最后开启加速度计和陀螺仪的同步采样模式?
 * 
 * @param 旿
 * @return uint8_t 
 *         - 1: 陀螺仪校准成功
 *         - 0: 陀螺仪校准失败或在校准过程中出现I2C通信错误
 */
uint8_t Gyr_COD()
{
    uint8_t data = 0;

    // 1. 禁用传感噿
    // 向寄存器CTRL7写入0x00，以禁用传感器，为后续校准操作做准备
    if (!i2cwrite(CTRL7, 0x00))
    {
        // 若写入失败，记录错误日志并返囿表示校准失败
        // ESP_LOGE(TAG, "发送禁用传感器命令失败＿);
        return 0;
    }

    // 2. 通过CTRL9命令发出CTRL_CMD_ON_DEMAND_CALIBRATION＿xA2）指仿
    // 向寄存器CTRL9写入0xA2，启动陀螺仪的按需校准操作
    if (!i2cwrite(CTRL9, 0xA2))
    {
        // 若写入失败，记录错误日志并返囿表示校准失败
        // ESP_LOGE(TAG, "发送CTRL_CMD_ON_DEMAND_CALIBRATION＿xA2）指令失败！");
        return 0;
    }

    // 3. 等待线.5秒，让QMI8658A完成CTRL9命令
    // 延时1500毫秒，确保设备有足够时间执行校准命令
    rt_thread_delay(1500);

    // 4. 等待STATUSINT笿位为1
    // 初始化data丿x00
    data = 0x00;
    // 循环读取STATUSINT寄存器，直到其第7位变丿，表示校准操作开姿
    while (((data >> 7) & 0x01) == 0)
    {
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示校准失败
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return 0;
        }
    }

    // 5. 向CTRL9寄存器写入CTRL_CMD_ACK＿x00）来确认
    // 向寄存器CTRL9写入0x00，确认校准操作开姿
    if (!i2cwrite(CTRL9, 0x00))
    {
        // 若写入失败，记录错误日志并返囿表示校准失败
        // ESP_LOGE(TAG, "发送CTRL_CMD_ACK＿x00）指令失败！");
        return 0;
    }

    // 6. 等待STATUSINT笿位为0
    // 初始化data丿xFF
    data = 0xFF;
    // 循环读取STATUSINT寄存器，直到其第7位变丿，表示校准操作完房
    while (((data >> 7) & 0x01) == 1)
    {
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示校准失败
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return 0;
        }
    }

    // 7. 读取COD_STATUS寄存器（0x46），检查COD实施的结枿状怿
    // 从寄存器COD_STATUS读取校准结果
    if (!i2cread(COD_STATUS, &data))
    {
        // 若读取失败，记录错误日志并返囿表示校准失败
        // ESP_LOGE(TAG, "读取COD_STATUS失败＿);
        return 0;
    }
    // 若data不为0，表示校准失贿
    if (data)
    {
        // 记录错误日志，显示错误类垿
        // ESP_LOGE(TAG, "陀螺仪校准失败，错误类型：%X", data);
        return 0;
    }

    // 8. 开启加速度计和陀螺仪，同步采样模弿
    // 向寄存器CTRL7写入0x83，开启加速度计和陀螺仪的同步采样模弿
    if (!i2cwrite(CTRL7, 0x83))
    {
        // 若写入失败，记录错误日志并返囿表示校准失败
        // ESP_LOGE(TAG, "启加速度计和陀螺仪,同步采样模式失败＿);
        return 0;
    }

    // 9. 校准成功
    // 记录日志表示陀螺仪校准成功，并返回1
    // ESP_LOGE(TAG, "陀螺仪校准成功!");
    return 1;
}


// 存储陀螺仪校准倿
float GyrCompensate[6];

/**
 * @brief 初始化QMI8658A传感噿
 * 
 * 该函数用于对QMI8658A传感器进行初始化操作，包括复位传感器、配置通讯方式和中断引脚?
 * 进行加速度计和陀螺仪的自检、配置传感器参数、使用锁定机制、开启传感器同步采样模式?
 * 进行陀螺仪自带校准和手动校准等步骤?
 * 
 * @param 旿
 * @return int 
 *         - 1: 传感器初始化成功
 *         - 0: 传感器初始化失败，可能是某个步骤的I2C通信出错或自检、校准失贿
 */
int QMI8658A_Init(void)
{
    uint8_t data = 0;
    s_pMisakaSoftI2c = misaka_soft_i2c_port_init();
    // 1. 复位QMI8658A传感噿
    // 向复位寄存器RESET写入0xB0，触发传感器复位操作
    if (!i2cwrite(RESET, 0xB0))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "发送复位命令失败！");
        return -1;
    }
    // 延时100毫秒，等待复位操作完房
    rt_thread_delay(100);

    // 读取复位状怿
    // 从dQY_L寄存器读取复位状态数据到变量data
    if (!i2cread(dQY_L, &data))
    {
        // 若读取失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "读取复位状态失败！");
        return -2;
    }

    // 检查复位状怿
    // 判断读取到的复位状态数据是否为0x80，若不是则表示复位失贿
    if (data != 0x80)
    {
        // 记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "复位失败＿);
        return -3;
    }

    // 2. 配置通讯方式和中断引脿
    // 向寄存器CTRL1写入0x60，配置传感器的通讯方式和中断引脿
    if (!i2cwrite(CTRL1, 0x60))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "配置通讯方式和中断引脚失败！");
        return -4;
    }

    // 3. 加速度计自检
    // 调用Acc_Self_Test函数进行加速度计自检
    if (!Acc_Self_Test())
    {
        // 若自检失败，记录错误日志并返回0表示初始化失贿
        // ESP_LOGE(TAG, "加速度计自检失败＿);
        return -5;
    }

    // 4. 陀螺仪自检
    // 调用Gyr_Self_Test函数进行陀螺仪自检
    if (!Gyr_Self_Test())
    {
        // 若自检失败，记录错误日志并返回0表示初始化失贿
        // ESP_LOGE(TAG, "陀螺仪自检失败＿);
        return -6;
    }

    // 5. 配置加速度访
    // 向寄存器CTRL2写入0x33，禁用加速度自检，设置量程为16G，输出数据速率丿96.8Hz
    if (!i2cwrite(CTRL2, 0x33))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "配置加速度计失败！");
        return -7;
    }

    // 6. 配置陀螺仪
    // 向寄存器CTRL3写入0x73，禁用陀螺仪自检，设置量程为2048dps，输出数据速率丿96.8Hz
    if (!i2cwrite(CTRL3, 0x73))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "配置陀螺仪计失败！");
        return -8;
    }

    // 7. 配置低通滤波器
    // 向寄存器CTRL5写入0x35，配置低通滤波器丿.63%
    if (!i2cwrite(CTRL5, 0x35))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "配置低通滤波器失败＿);
        return -9;
    }

    // 8. 使用锁定机制
    // 8.1 禁用内部AHB时钟
    // 向CAL1_L寄存器写兿x01
    if (!i2cwrite(CAL1_L, 0x01))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "CAL1_L寄存器写入失败！");
        return -10;
    }
    // 在CTRL9协议中写兿x12（CTRL_CMD_AHB_CLOCK_GATING＿
    if (!i2cwrite(CTRL9, 0x12))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "发送CTRL_CMD_ON_DEMAND_CALIBRATION＿x12）指令失败！");
        return -11;
    }
    // 等待10毫秒，让QMI8658A完成CTRL9命令
    rt_thread_delay(10);

    // 8.2 等待STATUSINT笿位为1
    // 初始化data丿x00
    data = 0x00;
    // 循环读取STATUSINT寄存器，直到其第7位变丿
    while (((data >> 7) & 0x01) == 0)
    {
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示初始化失贿
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return -12;
        }
    }

    // 8.3 向CTRL9寄存器写入CTRL_CMD_ACK＿x00）来确认
    // 向寄存器CTRL9写入0x00，确认操使
    if (!i2cwrite(CTRL9, 0x00))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "发送CTRL_CMD_ACK＿x00）指令失败！");
        return -13;
    }

    // 8.4 等待STATUSINT笿位为0
    // 初始化data丿xFF
    data = 0xFF;
    // 循环读取STATUSINT寄存器，直到其第7位变丿
    while (((data >> 7) & 0x01) == 1)
    {
        // 读取STATUSINT寄存器的值到data
        if (!i2cread(STATUSINT, &data))
        {
            // 若读取失败，记录错误日志并返囿表示初始化失贿
            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
            return -14;
        }
    }

    // 9. 开启加速度计和陀螺仪，同步采样模弿
    // 向寄存器CTRL7写入0x83，开启加速度计和陀螺仪的同步采样模弿
    if (!i2cwrite(CTRL7, 0x83))
    {
        // 若写入失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "启加速度计和陀螺仪,同步采样模式失败＿);
        return -15;
    }
    // 延时10毫秒，等待模式开启完房
    rt_thread_delay(10);

    // 10. 陀螺仪自带校准
    // 调用Gyr_COD函数进行陀螺仪自带校准
    if (!Gyr_COD())
    {
        // 若校准失败，记录错误日志并返囿表示初始化失贿
        // ESP_LOGE(TAG, "陀螺仪校准失败＿);
        return -16;
    }
    // 延时100毫秒，等待校准完房
    rt_thread_delay(100);

    // 11. 陀螺仪手动校准
    // 循环调用calibrationGYR函数进行陀螺仪手动校准，直到校准成势
    //while (!calibration_ACC_GYR(GyrCompensate))
    //{
    //}

    // 12. 初始化成势
    // 记录日志表示传感器初始化成功，并返回1
    // ESP_LOGE(TAG, "初始化成功！");
    return 1;
}



/**
 * @brief 读取QMI8658A六轴传感器数捿
 * 
 * 该函数用于读取QMI8658A六轴传感器的数据，在读取前会先检查数据的可用性和锁定状态，
 * 确保数据有效后再进行读取操作，并将读取到的原始数据存储到传入的数组中?
 * 
 * @param DATA 指向一个长度为6的int16_t类型数组的指针，用于存储读取到的六轴传感器原始数据，
 *             数组元素依次为AX、AY、AZ、GX、GY、GZ
 * @return int 
 *         - 1: 数据读取成功
 *         - 0: 读取过程中出现错误，如读取状态寄存器失败或读取传感器数据寄存器失贿
 */
int QMI8658A_ReadData(int16_t *DATA)
{
    uint8_t data = 0;

    // 1. 判断数据是否可用（Avail＿
    // 循环读取STATUSINT寄存器，直到其第0位为1，表示数据可甿
//    while ((data & 0x01) != 1)
//    {
//        // 读取STATUSINT寄存器的值到data
//        if (!i2cread(STATUSINT, &data))
//        {
//            // 若读取失败，记录错误日志并返囿表示读取失败
//            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
//            return 0;
//        }
//    }

    // 2. 判断数据是否锁定（Locked＿
    // 重置data丿
    data = 0;
    // 循环读取STATUSINT寄存器，直到其第1位为1，表示数据已锁定
//    while (((data >> 1) & 0x01) != 1)
//    {
//        // 读取STATUSINT寄存器的值到data
//        if (!i2cread(STATUSINT, &data))
//        {
//            // 若读取失败，记录错误日志并返囿表示读取失败
//            // ESP_LOGE(TAG, "读取STATUSINT状态失败！");
//            return 0;
//        }
//    }

    // 3. 读取传感器数据寄存器
    // 定义一个长度为12的数组datas，用于存储从传感器数据寄存器读取的数捿
    uint8_t datas[12] = {};
    // 从地址A_XYZ开始连续读叿2个字节的数据到datas数组
    if (!i2creads(A_XYZ, 12, datas))
    {
        // 此处注释有误，应是读取传感器数据失败，若读取失败，记录错误日志并返回0表示读取失败
        // ESP_LOGE(TAG, "读取传感器数据失败！");
        return 0;
    }

    // 4. 组合数据
    // 将读取到的低字节和高字节数据组合成完整的16位数据，并存储到DATA数组丿
    DATA[0] = (datas[1] << 8) | datas[0]; // AX
    DATA[1] = (datas[3] << 8) | datas[2]; // AY
    DATA[2] = (datas[5] << 8) | datas[4]; // AZ
    DATA[3] = (datas[7] << 8) | datas[6]; // GX
    DATA[4] = (datas[9] << 8) | datas[8]; // GY
    DATA[5] = (datas[11] << 8) | datas[10]; // GZ

    // 5. 返回读取成功标志
    return 1;
}

/**
 * @brief 将QMI8658A传感器的原始数据根据加速度计和陀螺仪的量程进行转换，并补偿陀螺仪数据
 * 
 * 该函数接收QMI8658A传感器的原始数据、加速度计和陀螺仪的量程作为输入，
 * 根据不同的量程将原始数据转换为实际的物理量值。加速度计数据转换为以g为单位，
 * 陀螺仪数据转换为以dps（度每秒）为单位，并对陀螺仪数据进行补偿?
 * 
 * @param InData 指向包含原始传感器数据的数组的指针，数组长度应为6＿
 *               剿个元素为加速度计数据（AX, AY, AZ），吿个元素为陀螺仪数据（GX, GY, GZ＿
 * @param OutData 指向用于存储转换后数据的数组的指针，数组长度应为6＿
 *                剿个元素存储转换后的加速度计数据，吿个元素存储转换后的陀螺仪数据
 * @param accelRange 加速度计的量程，合法值为2, 4, 8, 16（单位：g＿
 * @param gyroRange 陀螺仪的量程，合法值为16, 32, 64, 128, 256, 512, 1024, 2048（单位：dps＿
 * @return 旿
 */
void QMI8658A_ConvertData(int16_t *InData, float *OutData, int accelRange, int gyroRange)
{
    float accelFactor, compensate;

    // 1. 计算加速度计转换因孿
    // 根据加速度计量程选择合适的转换因子
    switch (accelRange)
    {
        case 2: // ±2g
            accelFactor = 2.0f * 2 / 65536;
            break;
        case 4: // ±4g
            accelFactor = 4.0f * 2 / 65536;
            break;
        case 8: // ±8g
            accelFactor = 8.0f * 2 / 65536;
            break;
        case 16: // ±16g
            accelFactor = 16.0f * 2 / 65536;
            break;
        default:
            accelFactor = 0;
            break;
    }

    // 2. 转换加速度计数捿
    // 将原始加速度计数据乘以转换因子，得到以g为单位的加速度计数捿
    for (int i = 0; i < 3; i++)
    {
        OutData[i] = InData[i] * accelFactor;
    }

    float gyroFactor;

    // 3. 计算陀螺仪转换因子
    // 根据陀螺仪量程选择合适的转换因子
    switch (gyroRange)
    {
        case 16: // ±16dps
            gyroFactor = 16.0f * 2 / 65536;
            break;
        case 32: // ±32dps
            gyroFactor = 32.0f * 2 / 65536;
            break;
        case 64: // ±64dps
            gyroFactor = 64.0f * 2 / 65536;
            break;
        case 128: // ±128dps
            gyroFactor = 128.0f * 2 / 65536;
            break;
        case 256: // ±256dps
            gyroFactor = 256.0f * 2 / 65536;
            break;
        case 512: // ±512dps
            gyroFactor = 512.0f * 2 / 65536;
            break;
        case 1024: // ±1024dps
            gyroFactor = 1024.0f * 2 / 65536;
            break;
        case 2048: // ±2048dps
            gyroFactor = 2048.0f * 2 / 65536;
            break;
        default:
            gyroFactor = 0;
            break;
    }

    // 4. 转换并补偿陀螺仪数据
    // 将原始陀螺仪数据乘以转换因子，并减去对应的补偿值，得到以dps为单位的陀螺仪数据
    for (int i = 3; i < 6; i++)
    {
        switch (i)
        {
            case 3:
                compensate = GyrCompensate[3];
                break;
            case 4:
                compensate = GyrCompensate[4];
                break;
            case 5:
                compensate = GyrCompensate[5];
                break;
            default:
                // 如果 i 不是 3??，可以在这里添加默认处理逻辑
                break;
        }
        OutData[i] = InData[i] * gyroFactor - compensate;
    }
}

/**
 * @brief 获取单位为g的三轴加速度计和单位为dps的三轴陀螺仪数据
 * 
 * 该函数通过调用QMI8658A_ReadData函数读取传感器的原始数据＿
 * 再调用QMI8658A_ConvertData函数将原始数据转换为以g为单位的加速度计数捿
 * 和以dps为单位的陀螺仪数据，并将转换后的数据存储到传入的数组中?
 * 
 * @param OutData 指向一个长度为6的float类型数组的指针，用于存储转换后的六轴传感器数据，
 *                数组元素依次为AX、AY、AZ、GX、GY、GZ，单位分别为g和dps
 * @return 旿
 */
void QMI8658A_Get_G_DPS(float *OutData)
{
    // 1. 定义用于存储原始数据的数绿
    int16_t data[6];

    // 2. 读取原始数据
    // 调用QMI8658A_ReadData函数读取传感器的原始数据
    QMI8658A_ReadData(data);

    // 3. 转换数据
    // 调用QMI8658A_ConvertData函数将原始数据转换为以g和dps为单位的数据
    QMI8658A_ConvertData(data, OutData, ACCRANGE, GYRRANGE);
}

/**
 * @brief 计算加速度的模长
 * 
 * 该函数接收一个包含三轴加速度数据的数组，通过计算三个轴加速度值的平方和的平方根，
 * 得到加速度的模长?
 * 
 * @param OutData 指向一个长度至少为 3 皿float 类型数组的指针，数组前三个元素依次为 X、Y、Z 轴的加速度值，单位丿g
 * @return float 加速度的模长，单位丿g
 */
float calculateAccelerationMagnitude(float *OutData)
{
    return (float)sqrt(OutData[0] * OutData[0] + OutData[1] * OutData[1] + OutData[2] * OutData[2]);
}

/**
 * @brief 比较函数，用亿qsort
 * 
 * 该函数是一个用亿qsort 函数的比较函数，用于寿float 类型的数据进行排序?
 * 它会比较两个 float 类型的值，并根据它们的大小关系返回相应的结果?
 * 
 * @param a 指向第一丿float 类型数据的指钿
 * @param b 指向第二丿float 类型数据的指钿
 * @return int 
 *         - 苿*a > *b，返囿1
 *         - 苿*a <= *b，返囿-1
 */
int compareFloat(const void *a, const void *b)
{
    return (*(float *)a - *(float *)b) > 0 ? 1 : -1;
}

/**
 * @brief 计算平均值的辅助函数
 * 
 * 该函数用于计算陀螺仪在一段时间内采集的多个数据的平均值。具体做法是＿
 * 先将采集到的陀螺仪AX、AY、AZ、GX、GY、GZ 六个轴数据分别复制到临时数组中并排序，然后选取中间皿
 * USED_DATA_COUNT 条数据计算平均值，最终将结果存储刿OutData 数组中?
 * 
 * @param DATA 一个二绿float 类型数组，大小为 [MIN_COLLECTION_COUNT][6]＿
 *             存储了陀螺仪圿MIN_COLLECTION_COUNT 次采集过程中AX、AY、AZ、GX、GY、GZ 六个轴的数据，单位为 g和dps
 * @param OutData 指向一个长度至少为 6 皿float 类型数组的指针，用于存储计算得到的陀螺仪AX、AY、AZ、GX、GY、GZ轴数据的平均值，单位丿g和dps
 * @return 旿
 */
void calculateAverages(float DATA[MIN_COLLECTION_COUNT][6], float *OutData) {
    // 动态分配内存用于存储排序后的数捿
    float **sortedData = (float **)malloc(6 * sizeof(float *));
    if (sortedData == NULL) {
        // ESP_LOGE(TAG, "内存分配失败");
        return;
    }

    for (int j = 0; j < 6; j++) {
        sortedData[j] = (float *)malloc(MIN_COLLECTION_COUNT * sizeof(float));
        if (sortedData[j] == NULL) {
            // 释放已分配的内存
            for (int k = 0; k < j; k++) {
                free(sortedData[k]);
            }
            free(sortedData);
            // ESP_LOGE(TAG, "内存分配失败");
            return;
        }
    }

    // 复制数据到临时数绿
    for (int i = 0; i < MIN_COLLECTION_COUNT; i++) {
        for (int j = 0; j < 6; j++) {
            sortedData[j][i] = DATA[i][j];
        }
    }

    // 对数据分别排庿
    for (int j = 0; j < 6; j++) {
        qsort(sortedData[j], MIN_COLLECTION_COUNT, sizeof(float), compareFloat);
    }

    // 计算中间 USED_DATA_COUNT 条数据的起始索引
    int startIndex = (MIN_COLLECTION_COUNT - USED_DATA_COUNT) / 2;

    // 计算中间 USED_DATA_COUNT 条数据的总和并计算平均倿
    for (int j = 0; j < 6; j++) {
        float sum = 0;
        for (int i = startIndex; i < startIndex + USED_DATA_COUNT; i++) {
            sum += sortedData[j][i];
        }
        OutData[j] = sum / USED_DATA_COUNT;
    }

    // 释放动态分配的内存
    for (int j = 0; j < 6; j++) {
        free(sortedData[j]);
    }
    free(sortedData);
}


/**
 * @brief 校准陀螺仪和加速度计，等待收集至少 500 次静止状态下的数据，对数据排序后取中闿200 条计算平均值?
 *
 * 该函数会不断读取六轴传感器的数据，计算加速度的模长，判断设备是否处于静止状态?
 * 当收集到至少 500 次静止状态下的数据后，对陀螺仪皿AX、AY、AZ、GX、GY、GZ 六个轴数据分别排序，
 * 取排序后中间皿200 条数据计算平均值，并将结果存储圿OutData 数组中?
 *
 * @param[out] OutData 用于存储校准后陀螺仪六个轴（ AX、AY、AZ、GX、GY、GZ）的平均值，数组长度至少丿6?
 * @return uint8_t 校准结果状态码＿
 *         - 0：表示读取六轴数据失败、传入参数无效或者还未收集满 500 次静止数据?
 *         - 1：表示成功收雿500 次静止数据并完成陀螺仪校准?
 */
uint8_t calibration_ACC_GYR(float *OutData) {
    // 检查传入的 OutData 指针是否丿NULL
    if (OutData == NULL) {
        // ESP_LOGE(TAG, "传入皿OutData 指针丿NULL，无法进行校准?);
        return 0;
    }

    // 用于存储从传感器读取的原始六轴数据，数组长度丿6，分别对庿AX、AY、AZ、GX、GY、GZ
    int16_t rawData[6];
    // 用于存储转换后的六轴数据，同样数组长度为 6
    float convertedData[6], AM = 0;
    // 用于存储上一次计算得到的加速度模长，初始值为 0
    float OldAM = 0;
    // 动态分配内存，用于存储至少 500 次静止状态下的陀螺仪数据（AX、AY、AZ、GX、GY、GZ＿
    float (*DATA)[6] = (float (*)[6])malloc(MIN_COLLECTION_COUNT * sizeof(float[6]));
    if (DATA == NULL) {
        // ESP_LOGE(TAG, "内存分配失败，无法进行校准?);
        return 0;
    }
    // 计数器，记录当前已经收集到的静止数据的次数，初始值为 0
    int i = 0;

    // 若还未收集满 500 次静止数据，则继续收雿
    while (i < MIN_COLLECTION_COUNT) {
        // 调用 QMI8658A_ReadData 函数读取六轴数据
        if (!QMI8658A_ReadData(rawData)) {
            // 若读取失败，打印错误信息，释放内存并返回 0 表示失败
            // ESP_LOGE(TAG, "读取六轴数据失败，无法继续处理?);
            free(DATA);
            return 0;
        }

        // 调用 QMI8658A_ConvertData 函数将原始数据进行转换，转换结果存储圿convertedData 数组丿
        QMI8658A_ConvertData(rawData, convertedData, ACCRANGE, GYRRANGE);

        // 计算加速度的模长，存储圿AM 变量丿
        AM = calculateAccelerationMagnitude(convertedData);

        // 判断当前加速度模长与上一次的差值是否小于阈值，若小于则认为处于静止状怿
        if (fabs(AM - OldAM) < STATIONARY_THRESHOLD) {
            // 保存数据（AX、AY、AZ、GX、GY、GZ＿
            DATA[i][0] = convertedData[0];
            DATA[i][1] = convertedData[1];
            DATA[i][2] = convertedData[2];
            DATA[i][3] = convertedData[3];
            DATA[i][4] = convertedData[4];
            DATA[i][5] = convertedData[5];
            // 计数器加 1，表示又收集到一次静止数捿
            i++;
        } else {
            // 若不处于静止状态，更新 OldAM 为当前的加速度模长
            OldAM = AM;
        }
    }

    // 已经收集到至尿500 次静止数据，计算陀螺仪平均倿
    calculateAverages(DATA, OutData);

    // 释放动态分配的内存
    free(DATA);

    // 打印校准结果，输凿GX、GY、GZ 轴的平均倿
    // ESP_LOGI(TAG, "陀螺仪校准完成，AX 平均倿 %.2f, AY 平均倿 %.2f, AZ 平均倿 %.2f,GX 平均倿 %.2f, GY 平均倿 %.2f, GZ 平均倿 %.2f", OutData[0], OutData[1], OutData[2],OutData[3], OutData[4], OutData[5]);

    // 返回 1 表示成功完成陀螺仪校准
    return 1;
}


/**
 * @brief 读取QMI8658A传感器的六轴数据，进行转换并打印转换后的数据
 *
 * 该函数会调用 QMI8658A_ReadData 函数仿QMI8658A 传感器读取原始的六轴数据＿
 * 若读取成功，再调甿QMI8658A_ConvertData 函数根据传入的加速度计和陀螺仪量程
 * 对原始数据进行转换，最后使甿ESP_LOGE 函数将转换后的加速度计和陀螺仪数据打印输出?
 * 如果读取数据失败，会打印错误信息并提前返回?
 *
 * @param accelRange 加速度计的量程，合法值为 2, 4, 8, 16（单位：g＿
 * @param gyroRange 陀螺仪的量程，合法值为 16, 32, 64, 128, 256, 512, 1024, 2048（单位：dps＿
 *
 * @return 旿
 */
void QMI8658A_ReadConvertAndPrint() {
    
    static int i=0;
    
    // 用于存储从传感器读取的原始六轴数捿
    int16_t rawData[6];
    // 用于存储转换后的六轴数据
    float convertedData[6],AM=0;

    // 调用 QMI8658A_ReadData 函数读取六轴数据
    if (!QMI8658A_ReadData(rawData)) {
        // 若读取失败，打印错误信息并返囿
        // ESP_LOGE(TAG, "读取六轴数据失败，无法继续处理?);
        return;
    }

    // 调用 QMI8658A_ConvertData 函数将原始数据进行转捿
    QMI8658A_ConvertData(rawData, convertedData, ACCRANGE, GYRRANGE);


    // 计算加速度的模长
    AM= calculateAccelerationMagnitude(convertedData);


    // 打印转换后的加速度计数据，保留小数点后6使
    // ESP_LOGE(TAG, " (g): AX=%d, AY=%d, AZ=%d (dps): GX=%d, GY=%d, GZ=%d", 
    //  rawData[0], rawData[1], rawData[2],rawData[3], rawData[4], rawData[5]);
    if(i==100)
    {
        i=0;
//    ESP_LOGE(TAG, " (g): AX=%.3f, AY=%.3f, AZ=%.3f, AM=%.3f (dps): GX=%.3f, GY=%.3f, GZ=%.3f",
//              (float)convertedData[0], (float)convertedData[1], (float)convertedData[2],AM,(float)convertedData[3], (float)convertedData[4], (float)convertedData[5]);
    }
    i++;
}


