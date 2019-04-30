#include <linux/fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/of_irq.h>
#include <linux/wait.h>

#include "sx1278.h"


ssize_t SX1278ReadBuffer(SX1278_t * psx, uint8_t addr)
{
	int ret = spi_w8r8(psx->spidev, addr);
	dev_notice(&psx->spidev->dev, "read addr = %#x,cmd = %#x", (int)addr, (int)ret);
	return ret;
}


void SX1278WriteBuffer(SX1278_t * psx, uint8_t addr, uint8_t cmd)
{
	uint8_t tmp2[2] = { addr | 0x80 ,cmd };
	dev_notice(&psx->spidev->dev, "write addr = %#x,cmd = %#x", (int)addr, (int)cmd);
	spi_write(psx->spidev, tmp2, 2);
}

int SX1278_SPIBurstRead(SX1278_t * psx, uint8_t addr, uint8_t* rxBuf,uint8_t length)
{
	return spi_write_then_read(psx->spidev, &addr, 1, &rxBuf, length);
}

int SX1278_SPIBurstWrite(SX1278_t * psx, uint8_t addr, uint8_t* txBuf, uint8_t length)
{
	addr = addr | 0x80;
	struct spi_transfer	t[2] = {
		{
			.tx_buf = &addr,
			.len = 1,
		},
		{
			.tx_buf = txBuf,
			.len = length,
		}
	};

	return spi_sync_transfer(psx->spidev, t, 2);
}

void SX1278_hw_Reset(SX1278_t * psx)
{
	gpio_direction_output(psx->reset_io, 0);

	msleep(100);

	gpio_direction_output(psx->reset_io, 1);

	msleep(100);
}


void SX1278_hw_DelayMs(SX1278_t * psx , uint32_t msec)
{
	msleep_interruptible(msec);
}