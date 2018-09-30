#include <linux/spi/spi.h>
#include <mach/gpio.h>
#include <linux/module.h>
#include "ssd1331.h"
struct ssd1331_platform_data plf_ssd = 
{
	.x_szie = 64,
	.y_size = 92,
	.dc_io = 0,
	.reset_io = 0,
};


static struct spi_board_info ssdinfo =
{
	.modalias = "m25p80",
	.max_speed_hz = 15000000,
	.bus_num = 0,
	.chip_select = 0,       //use SS0
	.platform_data = &plf_ssd,
	.mode = SPI_MODE_0,
};


static int init_ssd(void)
{
	spi_register_board_info(&ssdinfo, 1);
	return 0;
}

static void remove_ssd(void)
{

}

module_init(init_ssd);
module_exit(remove_ssd);

MODULE_LICENSE("GPL");

