#KERNEL_RELEASE := $(uname -r)
KERNEL_RELEASE := 5.9.0-rc1+

obj-m += blk-flt-trace.o

all:
	make -C /lib/modules/$(KERNEL_RELEASE)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KERNEL_RELEASE)/build M=$(PWD) clean
