#include <linux/fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include "ssd1331.h"
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>

#define  RGB_16(R,G,B) ((u16)(((B)&0xf8)>>3)|(((G)&0xfc)<<3)|(((R)&0xf8)<<8))

typedef union pix_data {
	u16 color_num;
}pix_data;

#define THREAD_CLOSE_FLAG -1



typedef struct ssd1331_pri_data
{
	int reset_io;
	int dc_io;
	int x_size;
	int y_size;
	int send_flag;
//for debug
	int dbg_pri_flag;
	u16 color_num;


	struct spi_device * spidev;
	struct task_struct * ptask;
	pix_data * pram;
}ssd1331_pri_data;


static int write_command(struct spi_device * spidev,unsigned char cmd)
{
	struct ssd1331_pri_data * pssd = spi_get_drvdata(spidev);
	int ret;

	gpio_set_value(pssd->dc_io,0);
	dev_notice(&spidev->dev, "dc = 0\n");
	
	ret = spi_write(spidev, &cmd, 1);

	gpio_set_value(pssd->dc_io, 1);
	dev_notice(&spidev->dev, "dc = 1\n");
	return ret;
}

#define Write_Command(x) write_command(spidev,(x))


void Set_Column_Address(struct spi_device * spidev,unsigned char a, unsigned char b)
{
	Write_Command(0x15);			// Set Column Address
	Write_Command(a);				    //   Default => 0x00 (Start Address)
	Write_Command(b);				    //   Default => 0x5F (End Address)
}


void Set_Row_Address(struct spi_device * spidev, unsigned char a, unsigned char b)
{
	Write_Command(0x75);			// Set Row Address
	Write_Command(a);				    //   Default => 0x00 (Start Address)
	Write_Command(b);				    //   Default => 0x3F (End Address)
}



void Set_Remap_Format(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xA0);			// Set Re-Map / Color Depth
	Write_Command(d);				    //   Default => 0x40
									//     Horizontal Address Increment
									//     Column Address 0 Mapped to SEG0
									//     Color Sequence: A => B => C
									//     Scan from COM0 to COM[N-1]
									//     Disable COM Split Odd Even
									//     65,536 Colors
}


void Set_Start_Line(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xA1);			// Set Vertical Scroll by RAM
	Write_Command(d);			    	//   Default => 0x00   00-63D
}


void Set_Display_Offset(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xA2);			// Set Vertical Scroll by Row
	Write_Command(d);				    //   Default => 0x00    00-63D
}


void Set_Display_Mode(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xA4 | d);			// Set Display Mode
									//   Default => 0xA4
									//     0xA4 (0x00) => Normal Display  
									//     0xA5 (0x01) => Entire Display On, All Pixels Turn On at GS Level 63
									//     0xA6 (0x02) => Entire Display Off, All Pixels Turn Off
									//     0xA7 (0x03) => Inverse Display
}



void Set_Display_On_Off(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xAE | d);			// Set Display On/Off
									// Default => 0xAE
									// 0xAE (0x00) => Display Off (Sleep Mode On)
									// 0xAF (0x01) => Display On (Sleep Mode Off)
}

void Set_Power_Saving_Mode(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xB0);			// Set Power Saving Mode
	Write_Command(d);				    // Default => 0x1A 
}

void Set_Reset_Pre_charge_period(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xB1);			// Set Reset (Phase1)/Pre-charge (Phase 2) period
	Write_Command(d);			      	// Default => 0x31 
}

void Set_Oscillator_Frequency_Clock_Divider(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xB3);			// Set Display Clock Divider / Oscillator Frequency
	Write_Command(d);				    // Default => 0xDO
									// A[3:0] => Display Clock Divider
									// A[7:4] => Oscillator Frequency
}





void Set_Precharge_Period(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xB6);			// Set Second Pre-Charge Period
	Write_Command(d);				    //   Default => 0x08 (8 Display Clocks)
}


void Set_Pre_charge_Level(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xBB);			// Set Pre-charge Level
	Write_Command(d);			    	//   Default => 0x3E (0.5*VCC) 
}

void Set_VCOMH(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xBE);			// Set COM Deselect Voltage Level
	Write_Command(d);				    //   Default => 0x3E (0.83*VCC)
}


void Set_Contrast_Color(struct spi_device * spidev, unsigned char a, unsigned char b, unsigned char c)
{
	Write_Command(0x81);			// Set Contrast Current for Color A,
	Write_Command(a);				    //   Default => 0x80 (Maximum)
	Write_Command(0x82);		    // Set Contrast Current for Color B,
	Write_Command(b);				    //   Default => 0x80 (Maximum)
	Write_Command(0x83);			// Set Contrast Current for Color C,
	Write_Command(c);				    //   Default => 0x80 (Maximum)
}

void Set_Second_Pre_charge_Speed_of_Color(struct spi_device * spidev, unsigned char a, unsigned char b, unsigned char c)
{
	Write_Command(0x8A);			// Set Second Pre-charge Speed of Color A,
	Write_Command(a);				//   Default => 0x80 (Color A)
	Write_Command(0x8B);			// Set Second Pre-charge Speed of Color B,
	Write_Command(b);				//   Default => 0x80 (Color B)
	Write_Command(0x8C);			// Set Second Pre-charge Speed of Color C,
	Write_Command(c);				//   Default => 0x80 (Color C)
}

void Set_Master_Current(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0x87);			// Master Contrast Current Control
	Write_Command(d);				//   Default => 0x0F (Maximum)
}


void Set_Master_Configuration(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xAD);			// Master Contrast Configuration
	Write_Command(d);				    //   Default => 0x8E (Maximum)
}




void Set_Multiplex_Ratio(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xA8);			// Set Multiplex Ratio
	Write_Command(d);				    //   Default => 0x3F  N = A[5:0] from 15d to 63d(3Fh)
}


void Set_Command_Lock(struct spi_device * spidev, unsigned char d)
{
	Write_Command(0xFD);			// Set Command Lock
	Write_Command(d);			     	// Default => 0x12
									// 0x12 => Driver IC interface is unlocked from entering command.
									// 0x16 => All Commands are locked except 0xFD.

}


static int ssd1331_init(struct spi_device * spidev)
{
	struct ssd1331_pri_data * pssd = spi_get_drvdata(spidev);

	gpio_set_value(pssd->reset_io, 0);
	dev_notice (&spidev->dev, "reset = 0\n");
	msleep(100);
	gpio_set_value(pssd->reset_io, 1);
	dev_notice(&spidev->dev, "reset = 1\n");


	Set_Display_On_Off(spidev, 0x00);		    // Display Off (0x00/0x01)
	Set_Remap_Format(spidev, 0x72);			// Set Horizontal Address Increment
									   //0x72(rgb)0x74(bgr)

	Set_Start_Line(spidev, 0x00);			            // Set Mapping RAM Display Start Line (0x00~0x3F)
	Set_Display_Offset(spidev, 0x00);		            // Shift Mapping RAM Counter (0x00~0x7F)
	Set_Display_Mode(spidev, 0x00);			        // Normal Display Mode (0x00/0x01/0x02/0x03)
	Set_Multiplex_Ratio(spidev, 0x3F);		            // 1/128 Duty (0x0F~0x7F)
	Set_Master_Configuration(spidev, 0x8e);            // set master configuration
	Set_Power_Saving_Mode(spidev, 0x0b);               // set power save
	Set_Reset_Pre_charge_period(spidev, 0x31);         //phase 1 and 2 period adjustment
	Set_Oscillator_Frequency_Clock_Divider(spidev, 0xf0);
	//display clock divider / oscillator frequency
	Set_Second_Pre_charge_Speed_of_Color(spidev, 0x64, 0x78, 0x64);
	//Set Second Pre-change Speed For Color
	Set_Pre_charge_Level(spidev, 0x3a);                //Set Pre-Change Level
	Set_VCOMH(spidev, 0x3e);			                // Set Common Pins Deselect Voltage Level as 0.82*VCC
	Set_Master_Current(spidev, 0x06);	        	// Set Scale Factor of Segment Output Current Control	 Brightness
	Set_Contrast_Color(spidev, 0x91, 0x50, 0x7d);	// Set Contrast of Color A (Red)
										   // Set Contrast of Color B (Green)
										   // Set Contrast of Color C (Blue)
	Set_Display_On_Off(spidev, 0x01);		        // Display On (0x00/0x01)

	return 0;
}

static int of_get_info(struct spi_device * spidev, struct ssd1331_pri_data * pssd)
{
	u32 tmp[2];

	struct device_node * of_node = spidev->dev.of_node;

	if (of_node == 0)
		return -1;

	of_property_read_u32_array(of_node, "video-size", tmp, 2);

	pssd->x_size = tmp[0];
	pssd->y_size = tmp[1];

	pssd->reset_io = of_get_named_gpio(of_node, "reset-io", 0);
	pssd->dc_io = of_get_named_gpio(of_node, "dc-io", 0);

	gpio_direction_output(pssd->reset_io, 0);
	gpio_direction_output(pssd->dc_io, 0);

	dev_notice(&spidev->dev, "reset %d,dc %d\n", pssd->reset_io, pssd->dc_io);


	if (!pssd->x_size || !pssd->y_size || !pssd->dc_io || !pssd->reset_io)
	{
		dev_err(&spidev->dev, "of fail\n");
		return -2;
	}
	return 0;
}

static int platform_get_info(struct spi_device * spidev, struct ssd1331_pri_data * pssd)
{
	struct ssd1331_platform_data *pdata = spidev->dev.platform_data;

	if (pdata == 0)
		return -1;

	pssd->x_size = pdata->x_szie;
	pssd->y_size = pdata->y_size;

	pssd->reset_io = pdata->reset_io;
	pssd->dc_io = pdata->dc_io;

	return 0;
}

static int init_info(struct spi_device * spidev, struct ssd1331_pri_data * pssd)
{
	pssd->spidev = spidev;

	if (0 == of_get_info(spidev, pssd))
		return 1;

	if (0 == platform_get_info(spidev, pssd))
		return 2;
	return -1;
}

static void ssd_change_color(struct ssd1331_pri_data * pssd)
{
	int i;

	pssd->color_num += 100;

	for (i = 0; i < pssd->x_size*pssd->y_size; i++)
	{
		pssd->pram[i].color_num = pssd->color_num;
	}
}



static void ssd_send_all(struct ssd1331_pri_data * pssd)
{
	struct spi_device * spidev = pssd->spidev;
	const unsigned char cmd[6] = { 0x15,0,96 - 1,0x75,0, 64 - 1 };

	gpio_set_value(pssd->dc_io, 0);
	dev_notice(&spidev->dev,"dc = 0\n");

	spi_write(spidev, &cmd, 6);

	gpio_set_value(pssd->dc_io, 1);
	dev_notice(&spidev->dev, "dc = 1\n");

	spi_write(spidev, pssd->pram, pssd->x_size * pssd->y_size * 2);
}


static int mem_send_thread(void * pdata)
{
	struct ssd1331_pri_data * pssd = pdata;
	struct spi_device * spidev = pssd->spidev;

	int i;
	dev_notice(&spidev->dev, "start send thread \n");

	while (pssd->send_flag > 0)
	{
		msleep(100);
		ssd_change_color(pssd);
		ssd_send_all(pssd);
	}

	/*clean all data*/
	for (i = 0; i < pssd->x_size*pssd->y_size; i++)
	{
		pssd->pram[i].color_num = 0;
	}
	ssd_send_all(pssd);

	/*set close flag*/
	pssd->send_flag = THREAD_CLOSE_FLAG;
	
	dev_notice(&spidev->dev, "end send thread\n");
	return 0;
}


static int myprobe(struct spi_device * spidev)
{
	struct ssd1331_pri_data * pssd = NULL;
	int ret;

	/*private data*/
	if (0 == (pssd = devm_kzalloc(&spidev->dev, sizeof(ssd1331_pri_data), GFP_KERNEL)))
		goto err;
	pssd->dbg_pri_flag = 0;


	/*get infomation*/
	if (0 > (ret = init_info(spidev, pssd)))
		goto err;

	/*get video memory*/
	if (0 == (pssd->pram = devm_kzalloc(&spidev->dev, pssd->x_size * pssd->y_size * sizeof(pix_data), GFP_KERNEL)))
	{
		dev_err(&spidev->dev, "video memory ,no enough mem\n");
		goto err;
	}
	/*get control gpio*/
	if (0 > (ret = devm_gpio_request_one(&spidev->dev, pssd->reset_io, GPIOF_OUT_INIT_HIGH, spidev->modalias)))
	{
		dev_err(&spidev->dev, "cat not get reset io\n");
		goto err;
	}
	if (0 > (ret = devm_gpio_request_one(&spidev->dev, pssd->dc_io, GPIOF_OUT_INIT_HIGH, spidev->modalias)))
	{
		dev_err(&spidev->dev, "cat not get dc io\n");
		goto err;
	}
	spi_set_drvdata(spidev, pssd);	

	/*oled init sequence*/
	if (0 != (ret = ssd1331_init(spidev)))
		goto err;

	/*turn on flag*/
	pssd->send_flag = 1;

	if (0 == (pssd->ptask = kthread_run(mem_send_thread, pssd, "ssd1331 thread")))
		goto err;

	return 0;
err:
	return -1;
}

static int close_send(ssd1331_pri_data * pssd)
{
	int trytime = 50;
	pssd->send_flag = 0;
	while (trytime-- > 0)
	{
		msleep(100);
		if (pssd->send_flag == THREAD_CLOSE_FLAG)
		{
			return 0;
		}
	}
	return -1;
}


int myremove(struct spi_device * spidev)
{	
	struct ssd1331_pri_data * pssd = spi_get_drvdata(spidev);
	//try to close send thread
	if (0 != close_send(pssd)) dev_err(&spidev->dev, "send close fail\n");

	return 0;
}


struct spi_device_id ids[] =
{
	{"ssd1331"},
	{/*for null*/},
};
#ifdef CONFIG_OF
static const struct of_device_id of_ssd_dt_ids[] = {
	{.compatible = "my,ssd1331"},
	{},
};
#endif
struct spi_driver myspi_drv =
{
	.driver = {
		.owner = THIS_MODULE,
		.name = "ssd1331",
		.of_match_table = of_match_ptr(of_ssd_dt_ids),
	},
	.probe = myprobe,
	.remove = myremove,
	.id_table = ids,
};


module_spi_driver(myspi_drv);
MODULE_LICENSE("GPL");
