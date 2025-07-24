#ifndef LIBXDMA_H
#define LIBXDMA_H

#include <linux/version.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/workqueue.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/workqueue.h>
#include <linux/aer.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <linux/splice.h>
#include <linux/uio.h>
#include <linux/spinlock_types.h>
#include <asm/cacheflush.h>
#include "board_info.h"

extern int vi53xx_debug;

/* disable debugging */
#define dbg_init(fmt, args...) \
    do { \
        if(vi53xx_debug) printk(KERN_DEBUG "%s " fmt, __func__, ##args); \
    } while(0)

#define PCI_DMA_H(addr) ((addr >> 16) >> 16)
#define PCI_DMA_L(addr) (addr & 0xffffffffUL)

#define POLL                                (1)

//#define XDMA_H2C0_STARTADDR		        (0x80100100)
#define XDMA_H2C0_STARTADDR		            (0x80100000)
//#define XDMA_C2H0_STARTADDR		        (0x80101000)
#define XDMA_C2H0_STARTADDR		            (0x80100000)
#define XDMA_C2H1_ASTARTADDR		        (0x00000000)
#define XDMA_C2H1_BSTARTADDR		        (0x01FA4000)

#define H2C_INTERRUPT_MASK_REG              (0x0090)
#define H2C_CONTROL_REG                     (0x0004)
#define H2C_CHANNEL_STATUS_REG              (0x0044)
#define H2C_CHANNEL_COMPLETED_DESC_REG      (0x0048)
#define H2C_POLL_LOW_ADDR_REG               (0x0088)
#define H2C_POLL_HIGH_ADDR_REG              (0x008C)

#define C2H_INTERRUPT_MASK_REG              (0x1090)
#define C2H_CONTROL_REG                     (0x1004)
#define C2H_CHANNEL_STATUS_REG              (0x1044)
#define C2H_POLL_LOW_ADDR_REG               (0x1088)
#define C2H_POLL_HIGH_ADDR_REG              (0x108C)

#define INTERRUPT_ENABLE_REG                (0x2010)
#define INTERRUPT_UNMASK_REG                (0x2014)
#define INTERRUPT_MASK_REG                  (0x2018)
#define INTERRUPT_REQ_REG                   (0x2044)

#define H2C_SGDMA_DESC_LOW32_REG            (0x4080)
#define H2C_SGDMA_DESC_HIGH32_REG           (0x4084)
#define H2C_SGDMA_DESC_ADJA_REG             (0x4088)
#define H2C_SGDMA_DESC_ADD_REG              (0x408C)

#define C2H_SGDMA_DESC_LOW32_REG            (0x5080)
#define C2H_SGDMA_DESC_HIGH32_REG           (0x5084)
#define C2H_SGDMA_DESC_ADJA_REG             (0x5088)
#define C2H_SGDMA_DESC_ADD_REG              (0x508C)

#define SGDMA_DESC_MODE_EN_REG              (0x6020)

#define MAX_EXTRA_ADJ                       (15)
#define MAX_DESC_NUM                        (64)
#define DMA_CHANNEL_NUM                     (2)
#define DESC_MAGIC                          (0xAD4B0000UL)
/* obtain the 32 most significant (high) bits of a 32-bit or 64-bit address */
#define PCI_DMA_H(addr)                     ((addr >> 16) >> 16)
/* obtain the 32 least significant (low) bits of a 32-bit or 64-bit address */
#define PCI_DMA_L(addr)                     (addr & 0xffffffffUL)

#define  H2C_DIR  			                (0)
#define  C2H_DIR  			                (1)
#define  CH1                                (1)
#define  CH2                                (2)

#define  CH1_H2C0                           (0x2000)
#define  CH1_C2H0                           (0x2000)
#define  CH2_C2H1                           (0x400000)
#define  CH2_H2C1                           (0x100)

#define DMA_STOP                            (0x0000)    // Running the SGDMA engine

#define PCIE_XDMA_BAR_NUM		            (6)
#define XDMA_BAR_NUM			            (6)
#define XDMA_CHANNEL_NUM		            (2)
#define DESC_NUM		                    (1)

extern struct list_head pcie_device_list;

struct xdma_channel_addr {
    u32 addr;
    unsigned int size;
};

struct xdma_desc {
	u32 control;
	u32 bytes;		/* transfer length in bytes */
	u32 src_addr_lo;	/* source address (low 32-bit) */
	u32 src_addr_hi;	/* source address (high 32-bit) */
	u32 dst_addr_lo;	/* destination address (low 32-bit) */
	u32 dst_addr_hi;	/* destination address (high 32-bit) */
	/*
	 * next descriptor in the single-linked list of descriptors;
	 * this is the PCIe (bus) address of the next descriptor in the
	 * root complex memory
	 */
	u32 next_lo;		/* next desc address (low 32-bit) */
	u32 next_hi;		/* next desc address (high 32-bit) */
} __packed;

struct xdma_cfg_info {
	unsigned char          direction;                    /* 0-host2card 1-card2host */
	unsigned char          SG_desc_num;                  /* Number of descriptors during DMA transfer */
	unsigned char 	       *pData[MAX_DESC_NUM];         /* Array of data pointers, the pointers point to the transferred data */
	unsigned int           data_len[MAX_DESC_NUM];       /* An array of data lengths, representing the length of data transmitted by each descriptor */
	dma_addr_t             data_phy_addr[MAX_DESC_NUM];  /* Array of physical addresses, storing the physical addresses of the transferred data */
	struct xdma_desc       *pFirst_desc;                 /* pointer to the first descriptor */
	dma_addr_t             first_desc_phy_addr;          /* physical address of the first descriptor */
    size_t                 size;
    int                    channel;
    u32                    ep_addr;
};

struct xdma_dev {
	struct pci_dev *pdev;	/* pci device struct from probe() */
	int idx;		/* dev index */

	const char *mod_name;		/* name of module owning the dev */

	/* PCIe BAR management */
	void __iomem *bar[XDMA_BAR_NUM];	/* addresses for mapped BARs */
	int regions_in_use;	/* flag if dev was in use during probe() */
	int got_regions;	/* flag if probe() obtained the regions */

	struct xdma_cfg_info C2H_dma_cfg[XDMA_CHANNEL_NUM];
	struct xdma_cfg_info H2C_dma_cfg[XDMA_CHANNEL_NUM];

    struct xdma_fops *fops;
};

struct pci_info {
    int (*setup)(struct pci_dev *pdev, struct xdma_cfg_info *plat);
};

struct xdma_fops {
    unsigned int (*read_reg)(void *reg_base, unsigned int offset);
    unsigned int (*write_reg)(void *reg_base, unsigned int offset, unsigned int val);
    int (*start)(struct xdma_dev *xdev, unsigned char channel, unsigned char direction);
    int (*start_desc)(struct xdma_dev *xdev, unsigned char channel, struct xdma_cfg_info *cfg);
    int (*stop)(struct xdma_dev *xdev, unsigned char channel, unsigned char direction);
    int (*poll_init)(struct xdma_dev *xdev, int channel, unsigned char direction);
    void (*create_transfer)(struct xdma_dev *xdev, unsigned char channel, struct xdma_cfg_info *cfg);
    int (*xdma_mmap)(struct xdma_dev *xdev, void *vm, struct xdma_cfg_info *cfg);
};

struct xdma_cdev {
    unsigned long magic;            /* structure ID for sanity checks */
    struct xdma_pci_dev *xpdev;
    struct xdma_dev *xdev;
    dev_t cdevno;                   /* character device major:minor */
    struct cdev cdev;               /* character device embedded struct */
    int bar;                        /* PCIe BAR for HW access, if needed */
    unsigned long base;             /* bar access offset */
    struct device *sys_device;      /* sysfs device */
    spinlock_t lock;
    struct device_info info;
};

struct xdma_pci_dev {
	struct pci_dev *pdev;	/* pci device struct from probe() */
	struct xdma_dev *xdev;
    struct xdma_cdev ctrl_cdev;
    int flags;
	struct list_head  list;
	int bus;

	struct xdma_cfg_info  *dma_cfg[2]; /*2 dir*/
    int fmap[2][4];
};

#define write_register(v, mem) iowrite32(v, mem)

u32 read_register(void *iomem);
void  platform_setup_fops(struct xdma_dev *xdev);

#endif

