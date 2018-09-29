#include <linux/fs.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include "head.h"

#define SMPLRT_DIV  0x19    //采样率分频，典型值：0x07(125Hz) */
#define CONFIG   0x1A       // 低通滤波频率，典型值：0x06(5Hz) */
#define GYRO_CONFIG  0x1B   // 陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s) */
#define ACCEL_CONFIG 0x1C  // 加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz) */
#define PWR_MGMT_1  0x6B // 电源管理，典型值：0x00(正常启用) */

//数据类寄存器
#define TEMP_OUT_H  0x41   // 存储的最近温度传感器的测量值 */
#define TEMP_OUT_L  0x42
 
#define GYRO_XOUT_H  0x43 // 存储最近的X轴、Y轴、Z轴陀螺仪感应器的测量值 */
#define GYRO_XOUT_L  0x44 
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48

#define ACCEL_XOUT_H 0x3B  // 存储最近的X轴、Y轴、Z轴加速度感应器的测量值 */
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40


int major;
struct class *cls;
struct device *devs;

struct i2c_client *glo_client;

void write_data(unsigned char reg, unsigned char val)
{
	unsigned char wbuf[] = { reg,val };
	struct i2c_msg msg[1] = { //数组元素个数由起始信号的个数决定
		{
			.addr = glo_client->addr,//代表从机地址
			.flags = 0,//代表写操作
			.len = 2,//写的数据的字节数,由写时序决定
			.buf = wbuf,//存放了需要写入的数据
		},
	};
	if (0 > i2c_transfer(glo_client->adapter, msg, ARRAY_SIZE(msg)))
	{
		printk("error write\n");
	}
#if 0
	else
	{
		printk("write %#x , %#x\n", (int)wbuf[0], (int)wbuf[1]);
	}
#endif
}

unsigned char read_data(unsigned char reg)
{
	char wbuf[] = {reg};
	char rbuf[1];
	struct i2c_msg msg[2] = {
		{
			.addr = glo_client->addr,
			.flags = 0,
			.len = 1,
			.buf = wbuf,
		},
		{
			.addr = glo_client->addr,
			.flags = 1,
			.len = 1,
			.buf = rbuf,
		},
	};
	if (0 > (i2c_transfer(glo_client->adapter, msg, ARRAY_SIZE(msg))))
	{
		printk("error read\n");
	}
#if 0
	else
	{
		printk("read %#x , %#x\n", (int)wbuf, (int)rbuf[0]);
	}
#endif
	return rbuf[0];
}

struct of_device_id test_tbl[] = {
	{
		.compatible = "fs4412,mpu6050",
	},
	{},
};

int mpu6050_open(struct inode *inode,struct file *filp)
{
	return 0;
}

long mpu6050_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
	union mpu6050 data;
	int ret;
	switch(cmd)
	{
	case ACCEL_CMD:
		data.accel.x = read_data(ACCEL_XOUT_L);
		data.accel.x |= read_data(ACCEL_XOUT_H) << 8;
		data.accel.y = read_data(ACCEL_YOUT_L);
		data.accel.y |= read_data(ACCEL_YOUT_H) << 8;
		data.accel.z = read_data(ACCEL_ZOUT_L);
		data.accel.z |= read_data(ACCEL_ZOUT_H) << 8;
		break;
	case GYRO_CMD:
		data.gyro.x = read_data(GYRO_XOUT_L);
		data.gyro.x |= read_data(GYRO_XOUT_H) << 8;
		data.gyro.y = read_data(GYRO_YOUT_L);
		data.gyro.y |= read_data(GYRO_YOUT_H) << 8;
		data.gyro.z = read_data(GYRO_ZOUT_L);
		data.gyro.z |= read_data(GYRO_ZOUT_H) << 8;
		break;
	case TEMP_CMD:
		data.gyro.z = read_data(TEMP_OUT_L);
		data.gyro.z |= read_data(TEMP_OUT_H) << 8;
		break;
	}

	ret = copy_to_user((void *)arg,&data,sizeof(data));
	return 0;
}


struct file_operations fops = {
	.open = mpu6050_open,
	.unlocked_ioctl = mpu6050_ioctl,
};

int test_probe(struct i2c_client *pdev,const struct i2c_device_id *id)
{
	//1.让mpu6050工作起来，进行控制类寄存器的配置
	//2.从指定的寄存器中读出加速度的值和角速度的值，温度的值
	printk("match ok###########\n");
	glo_client = pdev;

	major = register_chrdev(0,"mpu6050",&fops);
	cls = class_create(THIS_MODULE,"mpu6050");
	devs = device_create(cls,NULL,MKDEV(major,0),NULL,"mpu6050");

	write_data(SMPLRT_DIV,0x07);
	write_data(CONFIG,0x06);
	write_data(GYRO_CONFIG,0x18);
	write_data(ACCEL_CONFIG,0x01);
	write_data(PWR_MGMT_1,0x00);
	return 0;
}


int test_remove(struct i2c_client *pdev)
{
	device_destroy(cls,MKDEV(major,0));
	class_destroy(cls);
	unregister_chrdev(major,"mpu6050");
	return 0;
}

//在i2c总线下，这个结构体必须要初始化,这个名称叫什么不重要
struct i2c_device_id test_id_tbl[] = {
	{
		.name = "mpu6050",
		//.name = "xxx",
	},
};

struct i2c_driver pdrv = {
	.driver = {
		.name = "test",
		.of_match_table = test_tbl,
	},

	.probe = test_probe,
	.remove = test_remove,

	.id_table = test_id_tbl,
};

module_i2c_driver(pdrv);
MODULE_LICENSE("GPL");
