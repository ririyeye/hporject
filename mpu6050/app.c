#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "head.h"

#define ACC (1/16384.0f * 10.0f)

int main(int argc, const char *argv[])
{
	int fd;

	fd = open("/dev/mpu6050",O_RDWR);
	
	union mpu6050 data;
	while(1)
	{
		ioctl(fd,ACCEL_CMD,&data);
		printf("accel:x = %f,y = %f,z = %f\n",(int)data.accel.x * ACC, (int)data.accel.y* ACC, (int)data.accel.z* ACC);

		ioctl(fd,GYRO_CMD,&data);
		printf("gyro:x = %d,y = %d,z = %d\n", (int)data.gyro.x, (int)data.gyro.y, (int)data.gyro.z);


		ioctl(fd, TEMP_CMD, &data);
		printf("temp is %d\n", (int)data.temp);

		sleep(1);
	}
	return 0;
}
