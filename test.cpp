#include "cgic.h"
#include <ctr.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdio.h>




#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "mpu6050/head.h"

#include <thread>

using namespace std;

int uart_device_open()
{
	int fd;
	struct termios options;
	if ((fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
	{
		perror("open failed");
		return -1;
	}
	tcgetattr(fd, &options);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CSTOPB;
	options.c_iflag |= IGNPAR;
	options.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);

	options.c_cc[VMIN] = 12;
	options.c_oflag = 0;
	options.c_lflag = 0;
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	tcsetattr(fd, TCSANOW, &options);
	printf("serial ok!\n");
	return fd;
}



#define ACC (1/16384.0f * 10.0f)



void init_mpu6050(const char * name, TSHM * ppp)
{
	int fd;

	fd = open("/dev/mpu6050", O_RDWR);

	union mpu6050 data;
	while (1)
	{
		ioctl(fd, ACCEL_CMD, &data);
		//printf("accel:x = %f,y = %f,z = %f\n", (int)data.accel.x * ACC, (int)data.accel.y* ACC, (int)data.accel.z* ACC);

		ppp->x = data.accel.x * ACC;
		ppp->y = data.accel.y * ACC;
		ppp->z = data.accel.z * ACC;
		usleep(1000 * 50);
	}
}



void stm32led(int fd,int cmd)
{
	char c;
	if (cmd == 1)
	{
		c = 'a';
		write(fd, &c, 1);
		c = 'b';
		write(fd, &c, 1);		
		c = 'c';
		write(fd, &c, 1);
	}
	else
	{
		c = 'e';
		write(fd, &c, 1);
	}
}


int cgiMain()
{
	key_t key = getKey(KEYPATH, KEYNUM);
	int mid = getMessQuene(key);
	TSHM * sid = (TSHM*)getSHM(key);

	msgdata_t msg;

	int uartfd = uart_device_open();
	char uartc;

	thread ppp(init_mpu6050, "/dev/mpu6050",sid);
	ppp.detach();
	


	while (1)
	{
		if (msgrcv(mid, &msg, sizeof(msgdata_t) - sizeof(long), 0, 0) == -1)
		{
			perror("fail to msgrcv:");
			return -1;
		}
		
		switch (msg.typeID)
		{
		case LED_ON:
			stm32led(uartfd, 1);
			printf("led on\n");
			break;
		case LED_OFF:
			stm32led(uartfd, 0);
			printf("led off\n");
			break;
		default:
			printf("error message = %d\n", (int)msg.typeID);
			break;
		}
	}
	return 0;
}

