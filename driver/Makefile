###############################
#
# Makefile for vi53xx kernel module
#
#################################


ifneq ($(KERNELRELEASE),)

obj-m := vi53xx.o
vi53xx-objs := libxdma.o vi53xx_xdma_drv.o vi53xx_cndev_core.o vi53xx_xdma_ctrl.o vi53xx_proc.o board_info.o

# include debugging information
EXTRA_CFLAGS :=-g3 -ggdb3

else

include ../../../defines.mk

obj-ko := vi53xx.ko
obj-c := libxdma.c vi53xx_xdma_drv.c vi53xx_cndev_core.c vi53xx_xdma_ctrl.c vi53xx_proc.c board_info.c
obj-tar := vi53xx_module_src.tgz


INSTALL_SCRIPT = vi53xx

all: tar
	$(MAKE) -C $(LINUX_DIR) M=$(CURDIR) modules
	
install: all
	$(INSTALL) -d $(ETAS_BASE_DIR)/modules
	$(INSTALL) -m 644 $(obj-ko) $(obj-tar) $(ETAS_BASE_DIR)/modules
#	$(INSTALL) -d $(ETAS_BASE_DIR)/etc/siminit
#	S(INSTALL) vi53xx-siminit $(ETAS_BASE_DIR)/etc/siminit/50-vi53xx
#	S (INSTALL) -d  $(ETAS_BASE_DIR)/etc/model.d
#	$(INSTALL) vi53xx-modelinit  $(ETAS_BASE_DIR)/etc/model.d
	$(INSTALL) -d  $(DESTDIR)/etc/init.d/
	$(INSTALL) $(INSTALL_SCRIPT)  $(DESTDIR)/etc/init.d/
	
clean:
	$(MAKE) -C $(LINUX_DIR) M=$(CURDIR) clean
	rm -f Module.symvers $(obj-tar)
	
tar:
	$(TAR) -czf $(obj-tar)  *.c  *.h  vi53xx
	
endif