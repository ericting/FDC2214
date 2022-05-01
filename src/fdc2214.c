#include "fdc2214.h"

// i2c使用iic.h中的两个SCL、SDA接口

u32 Data_FDC;
//设置寄存器的值
u8 Set_FDC2214(u8 reg, u8 MSB, u8 LSB)
{
	iic_start();
	iic_write_byte((FDC2214_I2C_ADDR_0 << 1) | 0);
	if (iic_wait_ack())
	{
		iic_stop();
		return 1;
	}
	iic_write_byte(reg); //写寄存器地址
	iic_wait_ack();		 //等待应答
	iic_write_byte(MSB); //发送数据1
	if (iic_wait_ack())	 //等待ACK
	{
		iic_stop();
		return 1;
	}
	iic_write_byte(LSB); //发送数据2
	if (iic_wait_ack())	 //等待ACK
	{
		iic_stop();
		return 1;
	}
	iic_stop();
	return 0;
}

u16 FDC2214_Read(u8 reg)
{
	u16 res;
	iic_start();
	iic_write_byte((FDC2214_I2C_ADDR_0 << 1) | 0);
	iic_wait_ack();
	iic_write_byte(reg);
	iic_wait_ack();
	iic_start();
	iic_write_byte((FDC2214_I2C_ADDR_0 << 1) | 1);
	iic_wait_ack();
	res = iic_read_byte(1) << 8;

	res |= iic_read_byte(0); //读取数据,发送nACK
	iic_stop();				 //产生一个停止条件
	return res;
}
/**
 * @brief
 * 读channelX的值
 * @param index
 * @return u32
 */
u32 FCD2214_ReadCH(u8 index)
{
	u32 result;
	switch (index)
	{
	case 0:
		result = FDC2214_Read(FDC2214_DATA_CH0_MSB) & 0x0FFF;
		result = (result << 16) | (FDC2214_Read(FDC2214_DATA_CH0_LSB));
		break;
	case 1:
		result = FDC2214_Read(FDC2214_DATA_CH1_MSB) & 0x0FFF;
		result = (result << 16) | (FDC2214_Read(FDC2214_DATA_CH1_LSB));
		break;
	case 2:
		result = FDC2214_Read(FDC2214_DATA_CH2_MSB) & 0x0FFF;
		result = (result << 16) | (FDC2214_Read(FDC2214_DATA_CH2_LSB));
		break;
	case 3:
		result = FDC2214_Read(FDC2214_DATA_CH3_MSB) & 0x0FFF;
		result = (result << 16) | (FDC2214_Read(FDC2214_DATA_CH3_LSB));
		break;
	default:
		break;
	}
	result = result & 0x0FFFFFFF;
	return result;
}
/**
 * @brief
 * 电容计算
 * @param chx
 * @return float
 */
float Cap_Calculate(u8 chx)
{
	float Cap;
	Data_FDC = FCD2214_ReadCH(chx);
	Cap = 232021045.248 / (Data_FDC);
	return (Cap * Cap);
	// return (Data_FDC);
}
/*FDC2214初始化函数
 *返回值:0：初始化正常
 *       1：不正常
 */

u8 FDC2214_init(void)
{
	u16 res, ID_FDC2214;
	iic_start();
	res = FDC2214_Read(FDC2214_MANUFACTURER_ID); //测试设备读取是否正常
	ID_FDC2214 = FDC2214_Read(FDC2214_DEVICE_ID);

	if (res == 0x5449)
	{
		//设置Set_FDC2214寄存器
		Set_FDC2214(FDC2214_RCOUNT_CH0, 0x34, 0xFB); //参考计数转换间隔时间（T=(RCOUNT_CH0*16)/Frefx）
		Set_FDC2214(FDC2214_RCOUNT_CH1, 0x34, 0xFB); //选择在0.01MHz ~ 10MHz的传感器频率
		Set_FDC2214(FDC2214_RCOUNT_CH2, 0x34, 0xFB);
		Set_FDC2214(FDC2214_RCOUNT_CH3, 0x34, 0xFB);

		Set_FDC2214(FDC2214_CLOCK_DIVIDERS_CH0, 0x20, 0x02); // Frefx = Fclk = 43.4MHz/2(2分频)
		Set_FDC2214(FDC2214_CLOCK_DIVIDERS_CH1, 0x20, 0x02); // CHx_REF_DIVIDER=2;CHx_FIN_SEL=2
		Set_FDC2214(FDC2214_CLOCK_DIVIDERS_CH2, 0x20, 0x02);
		Set_FDC2214(FDC2214_CLOCK_DIVIDERS_CH3, 0x20, 0x02); // CHx_REF_DIVIDER=2;CHx_FIN_SEL=2

		Set_FDC2214(FDC2214_SETTLECOUNT_CH0, 0x00, 0x1B); //转换之前的稳定时间（T=(SETTLECOUNT_CHx*16)/Frefx）
		Set_FDC2214(FDC2214_SETTLECOUNT_CH1, 0x00, 0x1B);
		Set_FDC2214(FDC2214_SETTLECOUNT_CH2, 0x00, 0x1B);
		Set_FDC2214(FDC2214_SETTLECOUNT_CH3, 0x00, 0x1B);

		Set_FDC2214(FDC2214_DRIVE_CH0, 0x78, 0x00); // 0.146ma（传感器时钟建立+转换时间的驱动电流）
		Set_FDC2214(FDC2214_DRIVE_CH1, 0x78, 0x00);
		Set_FDC2214(FDC2214_DRIVE_CH2, 0x78, 0x00);
		Set_FDC2214(FDC2214_DRIVE_CH3, 0x78, 0x00);

		Set_FDC2214(FDC2214_STATUS_CONFIG, 0x00, 0x00); //全部禁止错误汇报

		Set_FDC2214(FDC2214_MUX_CONFIG, 0xC2, 0x0D); //通道0，1，2 ，3；选择10Mhz为超过振荡槽振荡频率的最低设置，多通道，四通道

		Set_FDC2214(FDC2214_CONFIG, 0x14, 0x01); //激活模式，使用内部振荡器做参考频率，INTB引脚会随状态寄存器更新被置位
	}
	else
		return 1;

	return 0;
}
