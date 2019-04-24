#include <linux/fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include "sx1278.h"
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>


struct sx1278_private_data {

	struct spi_device * spidev;
	struct task_struct * ptask;
	int running_flag;
	//0 running 
	//1 try stop
	//-1 thread stopped
};

static const char * buff = "12345";

static int sx1278_thread(void * pdata)
{
	struct sx1278_private_data * pssd = pdata;
	struct spi_device * spidev = pssd->spidev;

	dev_notice(&spidev->dev, "sx1278 start thread \n");

	while (pssd->running_flag == 0) {
		
		if (0 != spi_write(spidev, buff, 5)) {
			dev_notice(&spidev->dev, "write data error\n");
		}
		msleep(200);
	}

	//clean all data

	//set close flag
	pssd->running_flag = -1;

	dev_notice(&spidev->dev, "sx1278 end thread\n");
	return 0;
}

static void wait_thread_finished(struct sx1278_private_data * pspd)
{
	if (pspd->ptask && (pspd->running_flag == 0)) {
		pspd->running_flag = 1;

		while (pspd->running_flag != -1) {
			msleep(1);
		}
		pspd->ptask = 0;
	}
}

static int sx12_probe(struct spi_device * spidev)
{
	struct sx1278_private_data * psd = devm_kzalloc(&spidev->dev
		, sizeof(struct sx1278_private_data), GFP_KERNEL);

	printk("sx1278 probe \n");

	if (!psd) {
		dev_err(&spidev->dev, "malloc error\n");
		goto Malloc_err;
	}


	psd->running_flag = 0;
	if (0 == (psd->ptask = kthread_run(sx1278_thread, psd, "sx1278 thread"))) {
		goto thread_err;
	}

	psd->spidev = spidev;
	spi_set_drvdata(spidev, psd);

	return 0;

	wait_thread_finished(psd);
thread_err:	

Malloc_err:
	
	return -1;
}



int sx12_remove(struct spi_device * spidev)
{
	struct sx1278_private_data * psd = spi_get_drvdata(spidev);

	printk("sx1278 remove \n");
	if (psd) {
		wait_thread_finished(psd);
	}
	//if (0 != close_send(pssd)) dev_err(&spidev->dev, "send close fail\n");

	//if (0 != (ret = unregister_framebuffer(pfb)))  dev_err(&spidev->dev, "unregister_framebuffer error\n");

	//framebuffer_release(pfb);

	return 0;
}


struct spi_device_id sx1278_id[] =
{
	{"sx1278"},
	{/*for null*/},
};

MODULE_DEVICE_TABLE(spi, sx1278_id);

#ifdef CONFIG_OF
static const struct of_device_id of_sx12_dt_ids[] = {
	{.compatible = "semtech,sx1278"},
	{},
};
MODULE_DEVICE_TABLE(of, of_sx12_dt_ids);
#endif

struct spi_driver myspi_drv =
{
	.driver = {
		.owner = THIS_MODULE,
		.name = "sx1278",
		.of_match_table = of_match_ptr(of_sx12_dt_ids),
	},
	.probe = sx12_probe,
	.remove = sx12_remove,
	.id_table = sx1278_id,
};


module_spi_driver(myspi_drv);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("ririyeye");
MODULE_DESCRIPTION("for sx1278");

