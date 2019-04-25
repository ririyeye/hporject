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



typedef struct sx1278_private_data {

	struct spi_device * spidev;
	struct task_struct * ptask;
	int running_flag;
	//0 running 
	//1 try stop
	//-1 thread stopped


	uint8_t frequency;
	uint8_t power;
	uint8_t LoRa_Rate;
	uint8_t LoRa_BW;
	uint8_t packetLength;

	SX1278_Status_t status;

	uint8_t rxBuffer[SX1278_MAX_PACKET];
	uint8_t readBytes;


}SX1278_t;


static ssize_t SX1278_SPIRead(SX1278_t * psx, unsigned char addr)
{
	return spi_w8r8(psx->spidev, addr);
}


static void SX1278_SPIWrite(SX1278_t * psx, uint8_t addr, uint8_t cmd)
{
	uint8_t tmp2[2] = { addr ,cmd };
	spi_write(psx->spidev, tmp2, 2);
}

static int SX1278_SPIBurstRead(SX1278_t * psx, uint8_t addr, uint8_t* rxBuf,uint8_t length)
{
	return spi_write_then_read(psx->spidev, &addr, 1, &rxBuf, length);
}

static int SX1278_SPIBurstWrite(SX1278_t * psx, uint8_t addr, uint8_t* txBuf, uint8_t length)
{
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



//void SX1278_hw_Reset(SX1278_hw_t * hw)
//{
//	SX1278_hw_SetNSS(hw, 1);
//	HAL_GPIO_WritePin(hw->reset.port, hw->reset.pin, GPIO_PIN_RESET);
//
//	SX1278_hw_DelayMs(1);
//
//	HAL_GPIO_WritePin(hw->reset.port, hw->reset.pin, GPIO_PIN_SET);
//
//	SX1278_hw_DelayMs(100);
//}


void SX1278_hw_DelayMs(uint32_t msec)
{
	msleep_interruptible(msec);
}

//int SX1278_hw_GetDIO0(SX1278_hw_t * hw)
//{
//	return (HAL_GPIO_ReadPin(hw->dio0.port, hw->dio0.pin) == GPIO_PIN_SET);
//}
void SX1278_standby(SX1278_t * psx)
{
	SX1278_SPIWrite(psx, LR_RegOpMode, 0x09);
	psx->status = STANDBY;
}

void SX1278_sleep(SX1278_t * psx)
{
	SX1278_SPIWrite(psx, LR_RegOpMode, 0x08);
	psx->status = SLEEP;
}

void SX1278_entryLoRa(SX1278_t * psx)
{
	SX1278_SPIWrite(psx, LR_RegOpMode, 0x88);
}

void SX1278_clearLoRaIrq(SX1278_t * psx)
{
	SX1278_SPIWrite(psx, LR_RegIrqFlags, 0xFF);
}




void SX1278_config(SX1278_t * psx, uint8_t frequency, uint8_t power,
	uint8_t LoRa_Rate, uint8_t LoRa_BW)
{
	SX1278_sleep(psx); //Change modem mode Must in Sleep mode
	SX1278_hw_DelayMs(15);

	SX1278_entryLoRa(psx);
	//SX1278_SPIWrite(module, 0x5904); //?? Change digital regulator form 1.6V to 1.47V: see errata note

	SX1278_SPIBurstWrite(psx, LR_RegFrMsb,
		(uint8_t*)SX1278_Frequency[frequency], 3); //setting  frequency parameter

//setting base parameter
	SX1278_SPIWrite(psx, LR_RegPaConfig, SX1278_Power[power]); //Setting output power parameter

	SX1278_SPIWrite(psx, LR_RegOcp, 0x0B);			//RegOcp,Close Ocp
	SX1278_SPIWrite(psx, LR_RegLna, 0x23);		//RegLNA,High & LNA Enable
	if (SX1278_SpreadFactor[LoRa_Rate] == 6) {	//SFactor=6
		uint8_t tmp;
		SX1278_SPIWrite(psx,
			LR_RegModemConfig1,
			((SX1278_LoRaBandwidth[LoRa_BW] << 4) + (SX1278_CR << 1) + 0x01)); //Implicit Enable CRC Enable(0x02) & Error Coding rate 4/5(0x01), 4/6(0x02), 4/7(0x03), 4/8(0x04)

		SX1278_SPIWrite(psx,
			LR_RegModemConfig2,
			((SX1278_SpreadFactor[LoRa_Rate] << 4) + (SX1278_CRC << 2)
				+ 0x03));

		tmp = SX1278_SPIRead(psx, 0x31);
		tmp &= 0xF8;
		tmp |= 0x05;
		SX1278_SPIWrite(psx, 0x31, tmp);
		SX1278_SPIWrite(psx, 0x37, 0x0C);
	} else {
		SX1278_SPIWrite(psx,
			LR_RegModemConfig1,
			((SX1278_LoRaBandwidth[LoRa_BW] << 4) + (SX1278_CR << 1) + 0x00)); //Explicit Enable CRC Enable(0x02) & Error Coding rate 4/5(0x01), 4/6(0x02), 4/7(0x03), 4/8(0x04)

		SX1278_SPIWrite(psx,
			LR_RegModemConfig2,
			((SX1278_SpreadFactor[LoRa_Rate] << 4) + (SX1278_CRC << 2)
				+ 0x03)); //SFactor &  LNA gain set by the internal AGC loop
	}

	SX1278_SPIWrite(psx, LR_RegSymbTimeoutLsb, 0xFF); //RegSymbTimeoutLsb Timeout = 0x3FF(Max)
	SX1278_SPIWrite(psx, LR_RegPreambleMsb, 0x00); //RegPreambleMsb
	SX1278_SPIWrite(psx, LR_RegPreambleLsb, 12); //RegPreambleLsb 8+4=12byte Preamble
	SX1278_SPIWrite(psx, REG_LR_DIOMAPPING2, 0x01); //RegDioMapping2 DIO5=00, DIO4=01
	psx->readBytes = 0;
	SX1278_standby(psx); //Entry standby mode
}

void SX1278_defaultConfig(SX1278_t * psx)
{
	SX1278_config(psx, psx->frequency, psx->power, psx->LoRa_Rate,
		psx->LoRa_BW);
}

int SX1278_LoRaEntryRx(SX1278_t * psx, uint8_t length, uint32_t timeout)
{
	uint8_t addr;

	psx->packetLength = length;

	SX1278_defaultConfig(psx);		//Setting base parameter
	SX1278_SPIWrite(psx, REG_LR_PADAC, 0x84);	//Normal and RX
	SX1278_SPIWrite(psx, LR_RegHopPeriod, 0xFF);	//No FHSS
	SX1278_SPIWrite(psx, REG_LR_DIOMAPPING1, 0x01);//DIO=00,DIO1=00,DIO2=00, DIO3=01
	SX1278_SPIWrite(psx, LR_RegIrqFlagsMask, 0x3F);//Open RxDone interrupt & Timeout
	SX1278_clearLoRaIrq(psx);
	SX1278_SPIWrite(psx, LR_RegPayloadLength, length);//Payload Length 21byte(this register must difine when the data long of one byte in SF is 6)
	addr = SX1278_SPIRead(psx, LR_RegFifoRxBaseAddr); //Read RxBaseAddr
	SX1278_SPIWrite(psx, LR_RegFifoAddrPtr, addr); //RxBaseAddr->FiFoAddrPtr
	SX1278_SPIWrite(psx, LR_RegOpMode, 0x8d);	//Mode//Low Frequency Mode
	//SX1278_SPIWrite(module, LR_RegOpMode,0x05);	//Continuous Rx Mode //High Frequency Mode
	psx->readBytes = 0;

	while (1) {
		if ((SX1278_SPIRead(psx, LR_RegModemStat) & 0x04) == 0x04) {	//Rx-on going RegModemStat
			psx->status = RX;
			return 1;
		}
		if (--timeout == 0) {
			//SX1278_hw_Reset(module->hw);
			SX1278_defaultConfig(psx);
			return 0;
		}
		SX1278_hw_DelayMs(1);
	}
}

uint8_t SX1278_LoRaRxPacket(SX1278_t * psx)
{
	unsigned char addr;
	unsigned char packet_size;

	//if (SX1278_hw_GetDIO0(module->hw)) {
	//	memset(module->rxBuffer, 0x00, SX1278_MAX_PACKET);

	//	addr = SX1278_SPIRead(module, LR_RegFifoRxCurrentaddr); //last packet addr
	//	SX1278_SPIWrite(module, LR_RegFifoAddrPtr, addr); //RxBaseAddr -> FiFoAddrPtr

	//	if (module->LoRa_Rate == SX1278_LORA_SF_6) { //When SpreadFactor is six,will used Implicit Header mode(Excluding internal packet length)
	//		packet_size = module->packetLength;
	//	} else {
	//		packet_size = SX1278_SPIRead(module, LR_RegRxNbBytes); //Number for received bytes
	//	}

	//	SX1278_SPIBurstRead(module, 0x00, module->rxBuffer, packet_size);
	//	module->readBytes = packet_size;
	//	SX1278_clearLoRaIrq(module);
	//}
	return psx->readBytes;
}

int SX1278_LoRaEntryTx(SX1278_t * psx, uint8_t length, uint32_t timeout)
{
	uint8_t addr;
	uint8_t temp;

	psx->packetLength = length;

	SX1278_defaultConfig(psx); //setting base parameter
	SX1278_SPIWrite(psx, REG_LR_PADAC, 0x87);	//Tx for 20dBm
	SX1278_SPIWrite(psx, LR_RegHopPeriod, 0x00); //RegHopPeriod NO FHSS
	SX1278_SPIWrite(psx, REG_LR_DIOMAPPING1, 0x41); //DIO0=01, DIO1=00,DIO2=00, DIO3=01
	SX1278_clearLoRaIrq(psx);
	SX1278_SPIWrite(psx, LR_RegIrqFlagsMask, 0xF7); //Open TxDone interrupt
	SX1278_SPIWrite(psx, LR_RegPayloadLength, length); //RegPayloadLength 21byte
	addr = SX1278_SPIRead(psx, LR_RegFifoTxBaseAddr); //RegFiFoTxBaseAddr
	SX1278_SPIWrite(psx, LR_RegFifoAddrPtr, addr); //RegFifoAddrPtr

	while (1) {
		temp = SX1278_SPIRead(psx, LR_RegPayloadLength);
		if (temp == length) {
			psx->status = TX;
			return 1;
		}

		if (--timeout == 0) {
			//SX1278_hw_Reset(module->hw);
			SX1278_defaultConfig(psx);
			return 0;
		}
	}
}

int SX1278_LoRaTxPacket(SX1278_t * psx, uint8_t* txBuffer, uint8_t length,
	uint32_t timeout)
{
	SX1278_SPIBurstWrite(psx, 0x00, txBuffer, length);
	SX1278_SPIWrite(psx, LR_RegOpMode, 0x8b);	//Tx Mode
	while (1) {
		//if (SX1278_hw_GetDIO0(psx->hw)) { //if(Get_NIRQ()) //Packet send over
		//	SX1278_SPIRead(psx, LR_RegIrqFlags);
		//	SX1278_clearLoRaIrq(psx); //Clear irq
		//	SX1278_standby(psx); //Entry Standby mode
		//	return 1;
		//}

		if (--timeout == 0) {
			//SX1278_hw_Reset(psx->hw);
			SX1278_defaultConfig(psx);
			return 0;
		}
		SX1278_hw_DelayMs(1);
	}
}

void SX1278_begin(SX1278_t * psx, uint8_t frequency, uint8_t power,
	uint8_t LoRa_Rate, uint8_t LoRa_BW, uint8_t packetLength)
{
	//SX1278_hw_init(psx->hw);
	psx->frequency = frequency;
	psx->power = power;
	psx->LoRa_Rate = LoRa_Rate;
	psx->LoRa_BW = LoRa_BW;
	psx->packetLength = packetLength;
	SX1278_defaultConfig(psx);
}

int SX1278_transmit(SX1278_t * psx, uint8_t* txBuf, uint8_t length,
	uint32_t timeout)
{
	if (SX1278_LoRaEntryTx(psx, length, timeout)) {
		return SX1278_LoRaTxPacket(psx, txBuf, length, timeout);
	}
	return 0;
}

int SX1278_receive(SX1278_t * psx, uint8_t length, uint32_t timeout)
{
	return SX1278_LoRaEntryRx(psx, length, timeout);
}

uint8_t SX1278_available(SX1278_t * psx)
{
	return SX1278_LoRaRxPacket(psx);
}

uint8_t SX1278_read(SX1278_t * psx, uint8_t* rxBuf, uint8_t length)
{
	if (length != psx->readBytes)
		length = psx->readBytes;
	memcpy(rxBuf, psx->rxBuffer, length);
	rxBuf[length] = '\0';
	psx->readBytes = 0;
	return length;
}

uint8_t SX1278_RSSI_LoRa(SX1278_t * psx)
{
	uint32_t temp = 10;
	temp = SX1278_SPIRead(psx, LR_RegRssiValue); //Read RegRssiValue, Rssi value
	temp = temp + 127 - 137; //127:Max RSSI, 137:RSSI offset
	return (uint8_t)temp;
}

uint8_t SX1278_RSSI(SX1278_t * psx)
{
	uint8_t temp = 0xff;
	temp = SX1278_SPIRead(psx, 0x11);
	temp = 127 - (temp >> 1);	//127:Max RSSI
	return temp;
}



static int sx1278_thread(void * pdata)
{
	SX1278_t * pssd = pdata;

	dev_notice(&pssd->spidev->dev, "sx1278 start thread \n");

	while (pssd->running_flag == 0) {
		
		//if (0 != spi_write(pssd->spidev, buff, 5)) {
		//	dev_notice(&pssd->spidev->dev, "write data error\n");
		//}
		msleep(200);
	}

	//clean all data

	//set close flag
	pssd->running_flag = -1;

	dev_notice(&pssd->spidev->dev, "sx1278 end thread\n");
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

static int sx12_probe(struct spi_device * spidev)
{
	SX1278_t * psd = devm_kzalloc(&spidev->dev, sizeof(SX1278_t), GFP_KERNEL);

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
	SX1278_t * psd = spi_get_drvdata(spidev);

	printk("sx1278 remove \n");
	if (psd) {
		wait_thread_finished(psd);
	}

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

