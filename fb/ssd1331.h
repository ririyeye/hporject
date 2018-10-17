#ifndef __LINUX_SSD1331_H
#define __LINUX_SSD1331_H




struct ssd1331_platform_data {
	unsigned int width;
	unsigned int heigth;
	unsigned dc_io;
	unsigned reset_io;
};
















#endif