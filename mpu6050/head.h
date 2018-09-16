#ifndef _HEAD_H
#define _HEAD_H

union mpu6050
{
	struct mpu6050_accel
	{
		short x;
		short y;
		short z;
	}accel;

	struct mpu6050_gyro
	{
		short x;
		short y;
		short z;
	}gyro;

	short temp;
};

#define ACCEL_CMD 	_IOR('x',0,union mpu6050)
#define GYRO_CMD 	_IOR('x',1,union mpu6050)
#define TEMP_CMD 	_IOR('x',2,union mpu6050)

#endif
