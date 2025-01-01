ifneq ($(KERNELRELEASE),)
    obj-m := readpfcount.o
else
    # 使用当前运行内核的构建目录
    KDIR := /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif
