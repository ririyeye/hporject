#include <linux/fs.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/spi/spi.h>
#include <linux/fb.h>




int myprobe(struct spi_device * spidev)
{




}

int myremove(struct spi_device * spidev)
{



	return 0;
}


struct spi_device_id ids[] =
{
	{"ssd1331"},
	{/*for null*/},
};

struct spi_driver myspi_drv =
{
	.driver = {
		.owner = THIS_MODULE,
		.name = "ssd1331",
	},
	.probe = myprobe,
	.remove = myremove,
	.id_table = ids,
};


module_spi_driver
MODULE_LICENSE("GPL");
