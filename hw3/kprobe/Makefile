target := kprobe
obj-m := kprobe.o	#states that there is one module to be built from the object file kfetch_mod

KERNEL_DIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules	
	#changing its directory to the one provided with the -C option.
	#M= option causes that makefile to move back into your module source directory before trying to build the modules target. 
	#target("modules" here) refers to the list of modules found in the obj-m variable.
unload:
	sudo rmmod $(target)
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean