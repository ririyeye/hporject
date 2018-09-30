ifeq ($(KERNELRELEASE),)
PWD = $(shell pwd)
#KERNEL_DIR = /home/ririyeye/Desktop/linux-4.14.47/
#KERNEL_DIR = /lib/modules/$(shell uname -r)/build/
KERNEL_DIR = /home/ririyeye/Desktop/share/NUC970_Linux_Kernel
modules:
	make -C $(KERNEL_DIR)  M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR)  M=$(PWD) clean
else
obj-m += spioled.o
obj-m += info.o
endif




