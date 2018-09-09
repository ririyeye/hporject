#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>


int fd;
int uart_device_open()
{
	struct termios options;
	if((fd=open("/dev/ttyUSB0",O_RDWR|O_NOCTTY|O_NDELAY))<0)
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
	tcsetattr(fd,TCSANOW,&options);
	printf("serial ok!\n");
	return 0;
}



int main(int argc, const char *argv[])
{
	uart_device_open();
	//char c='1';

	char c;	
	while(1)
	{
		c=getchar();
		printf("command is %c\n", c);
		if (c=='a'||c=='b'||c=='c'||c=='e')
		{
			write(fd,&c,1);	
		}
	}

	close(fd);

	return 0;
}
