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
const unsigned char power_data[8] = { 0X80, 0X80, 0X80, 0X83, 0X86, 0x89, 0x8c, 0x8f };
//unsigned char Frequency[3]    = {0x6c, 0x80, 0x00 };//common 频率为434Mhz，也是默认值
const unsigned char Frequency[3] = { 0x7A, 0x80, 0x00 };//common 频率为490Mhz，也是默认值
const unsigned char powerValue = 7;
const unsigned char SpreadingFactor = 7;    //扩频因子7-12 6 7 8 9 10 11 12 
const unsigned char CodingRate = 1;     //1-4
const unsigned char Bw_Frequency = 8;     //6-9  62.5K 125K 250K 500K 
//const unsigned char RF_EX0_STATUS;
//const unsigned char CRC_Value;
//const unsigned char SX1278_RLEN;

void  SX1278LoRaFsk(SX1278_t * psx, Debugging_fsk_ook opMode)
{
	uint8_t opModePrev;
	opModePrev = SX1278ReadBuffer(psx, REG_LR_OPMODE);
	opModePrev &= 0x7F;
	opModePrev |= opMode;
	SX1278WriteBuffer(psx, REG_LR_OPMODE, opModePrev);
}

void  SX1278LoRaSetOpMode(SX1278_t * psx, RFMode_SET opMode)
{
	uint8_t opModePrev;
	opModePrev = SX1278ReadBuffer(psx , REG_LR_OPMODE);//寄存器地址0X01
	opModePrev &= 0xf8;
	opModePrev |= opMode;
	SX1278WriteBuffer(psx , REG_LR_OPMODE, opModePrev);
}


void  SX1278LoRaSetRFFrequency(SX1278_t * psx)
{
	//SX1278WriteBuffer(psx,REG_LR_FRFMSB, Frequency[0]);//0x06射频载波频率最高有效位
	//SX1278WriteBuffer(psx,REG_LR_FRFMID, Frequency[1]);//0x07射频载波频率中间有效位
	//SX1278WriteBuffer(psx,REG_LR_FRFLSB, Frequency[2]);//0x08射频载波频率最低有效位

	SX1278_SPIBurstWrite(psx, REG_LR_FRFMSB, Frequency, 3);
}


void  SX1278LoRaSetRFPower(SX1278_t * psx, uint8_t power)
{
	SX1278WriteBuffer(psx, REG_LR_PADAC, 0x87);
	SX1278WriteBuffer(psx, REG_LR_PACONFIG, power_data[power]);
}

void  SX1278LoRaSetNbTrigPeaks(SX1278_t * psx, uint8_t value)
{
	uint8_t RECVER_DAT;
	RECVER_DAT = SX1278ReadBuffer(psx, 0x31);
	RECVER_DAT = (RECVER_DAT & 0xF8) | value;
	SX1278WriteBuffer(psx, 0x31, RECVER_DAT);
}


void  SX1278LoRaSetSpreadingFactor(SX1278_t * psx, uint8_t factor)
{
	uint8_t RECVER_DAT;
	SX1278LoRaSetNbTrigPeaks(psx, 3);
	RECVER_DAT = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG2);
	RECVER_DAT = (RECVER_DAT & RFLR_MODEMCONFIG2_SF_MASK) | (factor << 4);
	SX1278WriteBuffer(psx, REG_LR_MODEMCONFIG2, RECVER_DAT);
}


void  SX1278LoRaSetErrorCoding(SX1278_t * psx, uint8_t value)
{
	uint8_t RECVER_DAT;
	RECVER_DAT = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG1);
	RECVER_DAT = (RECVER_DAT & RFLR_MODEMCONFIG1_CODINGRATE_MASK) | (value << 1);
	SX1278WriteBuffer(psx, REG_LR_MODEMCONFIG1, RECVER_DAT);
}

void  SX1278LoRaSetPacketCrcOn(SX1278_t * psx, uint8_t enable)
{
	uint8_t RECVER_DAT;
	RECVER_DAT = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG2);
	RECVER_DAT = (RECVER_DAT & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK) | (enable << 2);
	SX1278WriteBuffer(psx, REG_LR_MODEMCONFIG2, RECVER_DAT);
}

void  SX1278LoRaSetSignalBandwidth(SX1278_t * psx, uint8_t bw)
{
	uint8_t RECVER_DAT;
	RECVER_DAT = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG1);
	RECVER_DAT = (RECVER_DAT & RFLR_MODEMCONFIG1_BW_MASK) | (bw << 4);
	SX1278WriteBuffer(psx, REG_LR_MODEMCONFIG1, RECVER_DAT);
}

void  SX1278LoRaSetImplicitHeaderOn(SX1278_t * psx, uint8_t enable)
{
	uint8_t RECVER_DAT;
	RECVER_DAT = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG1);
	RECVER_DAT = (RECVER_DAT & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) | (enable);
	SX1278WriteBuffer(psx, REG_LR_MODEMCONFIG1, RECVER_DAT);
}

void  SX1278LoRaSetPayloadLength(SX1278_t * psx, uint8_t value)
{
	SX1278WriteBuffer(psx, REG_LR_PAYLOADLENGTH, value);
}

void  SX1278LoRaSetSymbTimeout(SX1278_t * psx, uint16_t value)
{
	uint8_t RECVER_DAT[2];
	RECVER_DAT[0] = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG2);
	RECVER_DAT[1] = SX1278ReadBuffer(psx, REG_LR_SYMBTIMEOUTLSB);
	RECVER_DAT[0] = (RECVER_DAT[0] & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK)
		| ((value >> 8) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK);
	RECVER_DAT[1] = value & 0xFF;
	SX1278WriteBuffer(psx, REG_LR_MODEMCONFIG2, RECVER_DAT[0]);
	SX1278WriteBuffer(psx, REG_LR_SYMBTIMEOUTLSB, RECVER_DAT[1]);
}

void  SX1278LoRaSetMobileNode(SX1278_t * psx, uint8_t enable)
{
	uint8_t RECVER_DAT;
	RECVER_DAT = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG3);
	RECVER_DAT = (RECVER_DAT & RFLR_MODEMCONFIG3_MOBILE_NODE_MASK) | (enable << 3);
	SX1278WriteBuffer(psx, REG_LR_MODEMCONFIG3, RECVER_DAT);
}

void  SX1278_RF_RECEIVE(SX1278_t * psx)
{
	SX1278LoRaSetOpMode(psx, Stdby_mode);
	SX1278WriteBuffer(psx, REG_LR_IRQFLAGSMASK, IRQN_RXD_Value);  //打开发送中断
	SX1278WriteBuffer(psx, REG_LR_HOPPERIOD, PACKET_MIAX_Value);
	SX1278WriteBuffer(psx, REG_LR_DIOMAPPING1, 0X00);
	SX1278WriteBuffer(psx, REG_LR_DIOMAPPING2, 0X00);
	SX1278LoRaSetOpMode(psx, Receiver_mode);
	psx->irq_status = SX_IRQ_RX_ENABLE;
}


int SX1278LORA_INT(SX1278_t * psx)
{
	SX1278LoRaSetOpMode(psx, Sleep_mode);   //设置睡眠模式0x01
	SX1278LoRaFsk(psx, LORA_mode);	       // 设置扩频模式,只能在睡眠模式下修改
	SX1278LoRaSetOpMode(psx, Stdby_mode);   // 设置为普通模式
	SX1278WriteBuffer(psx, REG_LR_DIOMAPPING1, GPIO_VARE_1);
	SX1278WriteBuffer(psx, REG_LR_DIOMAPPING1, GPIO_VARE_1);
	SX1278WriteBuffer(psx, REG_LR_DIOMAPPING2, GPIO_VARE_2);
	SX1278LoRaSetRFFrequency(psx);
	SX1278LoRaSetRFPower(psx, powerValue);
	SX1278LoRaSetSpreadingFactor(psx, SpreadingFactor);// 扩频因子设置
	SX1278LoRaSetErrorCoding(psx, CodingRate);		  //有效数据比
	SX1278LoRaSetPacketCrcOn(psx, true);			      //CRC 校验打开
	SX1278LoRaSetSignalBandwidth(psx, Bw_Frequency);	  //设置扩频带宽
	SX1278LoRaSetImplicitHeaderOn(psx, false);		  //同步头是显性模式
	SX1278LoRaSetPayloadLength(psx, 0xff);		      //0xff timeout中断
	SX1278LoRaSetSymbTimeout(psx, 0x3FF);
	SX1278LoRaSetMobileNode(psx, true); 			      // 低数据的优化
	SX1278_RF_RECEIVE(psx);
	return 0;
}





static void sx1278_init(SX1278_t * psx)
{
	int version_mode;

	SX1278_hw_Reset(psx);

	version_mode = SX1278ReadBuffer(psx, REG_LR_VERSION);

	dev_notice(&psx->spidev->dev, "version mode = %d\n", version_mode);

	SX1278LORA_INT(psx);
}

irqreturn_t sx1278irq(int irqno, void * ppp)
{
	SX1278_t * psx = ppp;
	printk("irq handler , irq no = %d\n", psx->irq_no);
	psx->irq_triggerd = 1;
	return IRQ_HANDLED;
}


static int sx1278_thread(void * pdata)
{
	SX1278_t * psx = pdata;

	dev_notice(&psx->spidev->dev, "sx1278 start thread \n");

	sx1278_init(psx);

	while (psx->running_flag == 0) {
		if (psx->irq_triggerd) {
			dev_notice(&psx->spidev->dev, "irq triggered \n");
			int RF_EX0_STATUS = SX1278ReadBuffer(psx, REG_LR_IRQFLAGS);
			psx->irq_triggerd = 0;

			if (RF_EX0_STATUS > 0) {
				if (RF_EX0_STATUS & 0x40) {
					uint8_t CRC_Value = SX1278ReadBuffer(psx, REG_LR_MODEMCONFIG2);
					//判断CRC校验使能，如果CRC使能
					if (CRC_Value & 0x04) {
						SX1278WriteBuffer(psx, REG_LR_FIFOADDRPTR, 0x00);
						psx->rxNum = SX1278ReadBuffer(psx, REG_LR_NBRXBYTES);

						SX1278_SPIBurstRead(psx, 0, psx->rxbuff, psx->rxNum);

						dev_notice(&psx->spidev->dev, "rcecive num = %d\n", psx->rxNum);
						//need add crc here
					}
					SX1278LoRaSetOpMode(psx, Stdby_mode);
					SX1278WriteBuffer(psx, REG_LR_IRQFLAGSMASK, IRQN_RXD_Value); //打开发送中断
					SX1278WriteBuffer(psx, REG_LR_HOPPERIOD, PACKET_MIAX_Value);
					SX1278WriteBuffer(psx, REG_LR_DIOMAPPING1, 0X00);
					SX1278WriteBuffer(psx, REG_LR_DIOMAPPING2, 0x00);
					SX1278LoRaSetOpMode(psx, Receiver_mode);
				} else if ((RF_EX0_STATUS & 0x08) == 0x08) {
					SX1278LoRaSetOpMode(psx, Stdby_mode);
					SX1278WriteBuffer(psx, REG_LR_IRQFLAGSMASK, IRQN_RXD_Value);  //打开发送中断
					SX1278WriteBuffer(psx, REG_LR_HOPPERIOD, PACKET_MIAX_Value);
					SX1278WriteBuffer(psx, REG_LR_DIOMAPPING1, 0X00);
					SX1278WriteBuffer(psx, REG_LR_DIOMAPPING2, 0x00);
					SX1278LoRaSetOpMode(psx, Receiver_mode);
				}
				SX1278WriteBuffer(psx, REG_LR_IRQFLAGS, 0xff);

			} else {
				dev_notice(&psx->spidev->dev, "RF_EX0_STATUS error = %d\n", RF_EX0_STATUS);
			}
			SX1278LoRaSetOpMode(psx, Stdby_mode);
			SX1278WriteBuffer(psx, REG_LR_IRQFLAGSMASK, IRQN_RXD_Value);  //打开发送中断
			SX1278WriteBuffer(psx, REG_LR_HOPPERIOD, PACKET_MIAX_Value);
			SX1278WriteBuffer(psx, REG_LR_DIOMAPPING1, 0X00);
			SX1278WriteBuffer(psx, REG_LR_DIOMAPPING2, 0x00);
			SX1278LoRaSetOpMode(psx, Receiver_mode);
		}
		msleep(1);
	}

	//clean all data

	//set close flag
	psx->running_flag = -1;

	dev_notice(&psx->spidev->dev, "sx1278 end thread\n");
	return 0;
}

static void wait_thread_finished(SX1278_t * pspd)
{
	if (pspd->ptask && (pspd->running_flag == 0)) {
		pspd->running_flag = 1;

		while (pspd->running_flag != -1) {
			msleep(1);
		}
		pspd->ptask = 0;
	}
}

static int of_get_info(struct spi_device * spidev, SX1278_t * psx)
{
	struct device_node * of_node = spidev->dev.of_node;

	if (of_node == 0)
		return -1;
	
	psx->reset_io = of_get_named_gpio(of_node, "reset-gpios", 0);
	if (psx->reset_io < 0) {
		dev_err(&spidev->dev, "of get reset failed = %d\n", psx->reset_io);
		psx->reset_io = 0;
		return -1;
	}

	if (spidev->irq == 0) {
		dev_err(&spidev->dev, "no irq in sx1278\n");
		return -2;
	}
	psx->irq_no = spidev->irq;
	dev_notice(&spidev->dev, "irq No. = %d\n", psx->irq_no);

	//if (!pssd->dc_io || !pssd->reset_io) {
	//	dev_err(&spidev->dev, "of fail\n");
	//	return -2;
	//}
	return 0;
}




static int init_gpio(struct spi_device * spidev, SX1278_t * psx)
{
	int ret;
	if (0 > (ret = devm_gpio_request_one(&spidev->dev, psx->reset_io, GPIOF_OUT_INIT_HIGH, spidev->modalias))) {
		dev_err(&spidev->dev, "cat not get reset io,error = %d\n", ret);
		return -1;
	}

	ret = devm_request_irq(&spidev->dev, psx->irq_no, sx1278irq, IRQ_TYPE_EDGE_RISING, "sx1278 irq", psx);
	if (ret != 0) {
		dev_err(&spidev->dev, "devm_request_irq error = %d\n", ret);
		return -1;
	}

	return 0;
}


static int sx12_probe(struct spi_device * spidev)
{
	SX1278_t * psd = devm_kzalloc(&spidev->dev, sizeof(SX1278_t), GFP_KERNEL);
	psd->spidev = spidev;
	printk("sx1278 probe \n");

	if (!psd) {
		dev_err(&spidev->dev, "malloc error\n");
		goto Malloc_err;
	}

	if (0 != of_get_info(spidev, psd)) {
		goto dts_error;
	}

	if (0 != init_gpio(spidev, psd)) {
		goto ioinit_error;
	}

	psd->running_flag = 0;
	
	if (0 == (psd->ptask = kthread_run(sx1278_thread, psd, "sx1278 thread"))) {
		goto thread_err;
	}
	
	spi_set_drvdata(spidev, psd);

	return 0;

	wait_thread_finished(psd);
thread_err:	

ioinit_error:

dts_error:

Malloc_err:
	
	return -1;
}



int sx12_remove(struct spi_device * spidev)
{
	SX1278_t * psx = spi_get_drvdata(spidev);

	printk("sx1278 remove \n");
	if (psx) {
		wait_thread_finished(psx);
	}
	devm_free_irq(&spidev->dev, psx->irq_no, psx);
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

