MODULE_NAME := syscall_trap_v2

obj-m := open_trap_v2.o close_trap_v2.o

KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

