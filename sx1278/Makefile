
ifeq ($(KERNELRELEASE),)
PWD = $(shell pwd)

KERNEL_DIR = ~/Desktop/pi/linux/
#KERNEL_DIR = ~/Desktop/imx/A7-linux-src/

modules:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_DIR)  M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_DIR)  M=$(PWD) clean
else
obj-m += sx1278.o
endif




