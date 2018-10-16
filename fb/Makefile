


ifeq ($(KERNELRELEASE),)
PWD = $(shell pwd)

ifeq ($(POS),a7)
KERNEL_DIR = /share/linux_sunxi
endif

ifeq ($(POS),970)
KERNEL_DIR = /home/ririyeye/Desktop/share/NUC970_Linux_Kernel
endif

ifeq ($(POS),ubuntu)
KERNEL_DIR = /lib/modules/$(shell uname -r)/build/
endif

KERNEL_DIR ?= $(POS)


modules:
	make -C $(KERNEL_DIR)  M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR)  M=$(PWD) clean
else
obj-m += spioled.o
#obj-m += info.o
endif




