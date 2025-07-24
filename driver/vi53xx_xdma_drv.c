#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "board_info.h"
#include  "libxdma.h"
#include  "vi53xx_cndev_core.h"

int vi53xx_debug = 0;
module_param(vi53xx_debug, int, 0644);
MODULE_PARM_DESC(vi53xx_debug, "Activate debugging for vi53xx module (default:0=off).");

#define DRV_MODULE_NAME		    "vi53xx"
#define DRV_MODULE_DESC	        "driver of es53xx designed by vehinfo(VI)"
#define RTPC_XDMA_VID           0x10ee
#define RTPC_XDMA_DID           0x16F2

static DEFINE_SEMAPHORE(vi53xx_mutex);

struct xdma_channel_addr ch_addr[2][XDMA_CHANNEL_NUM + 1] = {
     [H2C_DIR][CH1] = {XDMA_H2C0_STARTADDR, CH1_H2C0},
     [C2H_DIR][CH1] = {XDMA_C2H0_STARTADDR, CH1_C2H0},
     [C2H_DIR][CH2] = {XDMA_C2H1_ASTARTADDR, CH2_C2H1},
};

static int vi53xx_info_init(struct pci_dev *pdev, struct xdma_cfg_info *plat);

static const struct  pci_info vi53xx_info = {
    .setup = vi53xx_info_init,
};

static const struct pci_device_id pci_ids[] = {
    { PCI_DEVICE(RTPC_XDMA_VID, RTPC_XDMA_DID), .driver_data = (kernel_ulong_t) &vi53xx_info },
    {0,}
};

MODULE_DEVICE_TABLE(pci, pci_ids);

static void vi53xx_lock(void)
{
    down(&vi53xx_mutex);
}

static void vi53xx_unlock(void)
{
    up(&vi53xx_mutex);
}

static int xdma_physical_address_init(struct pci_dev *pdev, struct xdma_cfg_info *pDma_cfg)
{
	int i;
    void *ptr;

	for(i = 0; i < pDma_cfg->SG_desc_num; i++ ) {
		ptr = dma_alloc_coherent(&pdev->dev, pDma_cfg->size, &pDma_cfg->data_phy_addr[i], GFP_KERNEL);
		if (!ptr){
			printk("System Error: DMA Memory Allocate.\n");
			return -1;
		}

        memset(ptr, 0, pDma_cfg->size);
        pDma_cfg->pData[i] = ptr;
	}

	return 0;
}

static u32 get_ep_addr(int channel, int dir)
{
	return ch_addr[dir][channel].addr;
}

static unsigned int get_chanel_size(int channel, int dir)
{
	return ch_addr[dir][channel].size;
}

static int vi53xx_info_init(struct pci_dev *pdev, struct xdma_cfg_info *plat)
{
     struct xdma_cfg_info *dma_cfg = plat;
     int channel = dma_cfg->channel;
     int dir = dma_cfg->direction;

     dma_cfg->SG_desc_num = DESC_NUM;
     dma_cfg->ep_addr = get_ep_addr(channel, dir);
     dma_cfg->data_len[0] = get_chanel_size(channel, dir);
     dma_cfg->size = get_chanel_size(channel, dir);

     xdma_physical_address_init(pdev, dma_cfg);

	 return 0;
}

static struct xdma_pci_dev *xpdev_alloc(struct pci_dev *pdev)
{
	struct xdma_pci_dev *xpdev = kmalloc(sizeof(*xpdev), GFP_KERNEL);

	if (!xpdev)
		return NULL;

	memset(xpdev, 0, sizeof(*xpdev));

	xpdev->pdev = pdev;
	xpdev->bus = pdev->bus->number;

	return xpdev;
}

static struct xdma_dev *alloc_dev_instance(struct pci_dev *pdev)
{
	struct xdma_dev *xdev;

	if (!pdev) {
		pr_err("Invalid pdev\n");
		return NULL;
	}

	/* allocate zeroed device book keeping structure */
	xdev = kzalloc(sizeof(struct xdma_dev), GFP_KERNEL);
	if (!xdev) {
		pr_info("OOM, xdma_dev.\n");
		return NULL;
	}

	/* create a driver to device reference */
	xdev->pdev = pdev;
	dbg_init("xdev = 0x%p\n", xdev);

	return xdev;
}

static int map_single_bar(struct xdma_dev *xdev, struct pci_dev *dev, int idx)
{
	resource_size_t bar_start;
	resource_size_t bar_len;
	resource_size_t map_len;

	bar_start = pci_resource_start(dev, idx);
	bar_len = pci_resource_len(dev, idx);
	map_len = bar_len;

	xdev->bar[idx] = NULL;

	/* do not map BARs with length 0. Note that start MAY be 0! */
	if (!bar_len) {
		//pr_info("BAR #%d is not present - skipping\n", idx);
		return 0;
	}

	/*
	 * map the full device memory or IO region into kernel virtual
	 * address space
	 */
	dbg_init("BAR%d: %llu bytes to be mapped.\n", idx, (u64)map_len);
	xdev->bar[idx] = pci_iomap(dev, idx, map_len);
	if (!xdev->bar[idx]) {
		pr_info("Could not map BAR %d.\n", idx);
		return -1;
	}

	pr_info("BAR%d at 0x%llx mapped at 0x%p, length=%llu(/%llu)\n", idx,
		(u64)bar_start, xdev->bar[idx], (u64)map_len, (u64)bar_len);

	return (int)map_len;
}

static int set_dma_mask(struct pci_dev *pdev)
{
	if (!pdev) {
		pr_err("Invalid pdev\n");
		return -EINVAL;
	}

	/* 64-bit addressing capability for XDMA? */
	if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
		/* query for DMA transfer */
		/* @see Documentation/DMA-mapping.txt */
		dbg_init("pci_set_dma_mask()\n");
		/* use 64-bit DMA */
		dbg_init("Using a 64-bit DMA mask.\n");
		pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	} else if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
		dbg_init("Could not set 64-bit DMA mask.\n");
		pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
		/* use 32-bit DMA */
		dbg_init("Using a 32-bit DMA mask.\n");
	} else {
		dbg_init("No suitable DMA possible.\n");
		return -EINVAL;
	}

	return 0;
}

static void unmap_bars(struct xdma_dev *xdev, struct pci_dev *dev)
{
	int i;

	for (i = 0; i < XDMA_BAR_NUM; i++) {
		/* is this BAR mapped? */
		if (xdev->bar[i]) {
			/* unmap BAR */
			pci_iounmap(dev, xdev->bar[i]);
			/* mark as unmapped */
			xdev->bar[i] = NULL;
		}
	}
}

static int map_bars(struct xdma_dev *xdev, struct pci_dev *dev)
{
	int rv;
	int i;
	int bar_id_list[XDMA_BAR_NUM];
	int bar_id_idx = 0;

	/* iterate through all the BARs */
	for (i = 0; i < XDMA_BAR_NUM; i++) {
		int bar_len;

		bar_len = map_single_bar(xdev, dev, i);
		if (bar_len == 0) {
			continue;
		} else if (bar_len < 0) {
			rv = -EINVAL;
			goto fail;
		}

		bar_id_list[bar_id_idx] = i;
		bar_id_idx++;
	}

	return 0;

fail:
	/* unwind; unmap any BARs that we did map */
	unmap_bars(xdev, dev);

	return rv;
}

void xdma_device_umap(struct pci_dev *pdev, void *dev_xdev)
{
	struct xdma_dev *xdev = (struct xdma_dev *)dev_xdev;

	dbg_init("pdev 0x%p, xdev 0x%p.\n", pdev, dev_xdev);

	if (!dev_xdev)
            return;

	unmap_bars(xdev, pdev);

	if (xdev->got_regions) {
        dbg_init("pci_release_regions 0x%p.\n", pdev);
	    pci_release_regions(pdev);
	}

	if (!xdev->regions_in_use) {
        dbg_init("pci_disable_device 0x%p.\n", pdev);
	    pci_disable_device(pdev);
	}

	kfree(xdev);
}

static void xdma_free_c2h_pdata(struct xdma_dev *xdev, int channel, int desc_num, struct pci_dev *pdev)
{
    int i = 0;
	struct xdma_pci_dev *xpdev;
	xpdev = dev_get_drvdata(&pdev->dev);

	for(i = 0; i < desc_num; i++ ) {
         if (xdev->C2H_dma_cfg[channel].pData[i]) {
            if (xpdev->fmap[C2H_DIR][channel])
                set_memory_wb((unsigned long)xdev->C2H_dma_cfg[channel].pData[i], xdev->C2H_dma_cfg[channel].size >> PAGE_SHIFT);

             dma_free_coherent(&pdev->dev, xdev->C2H_dma_cfg[channel].size,
         	                  xdev->C2H_dma_cfg[channel].pData[i],
         			  xdev->C2H_dma_cfg[channel].data_phy_addr[i]);
            xdev->C2H_dma_cfg[channel].pData[i] = NULL;
         }
    }
}

static void xdma_free_h2c_pdata(struct xdma_dev *xdev, int channel, int desc_num, struct pci_dev *pdev)
{
    int i = 0;
	struct xdma_pci_dev *xpdev;
	xpdev = dev_get_drvdata(&pdev->dev);

	for(i = 0; i < desc_num; i++ ) {
        if (xdev->H2C_dma_cfg[channel].pData[i]) {
            if (xpdev->fmap[H2C_DIR][channel])
                set_memory_wb((unsigned long)xdev->H2C_dma_cfg[channel].pData[i], xdev->H2C_dma_cfg[channel].size >> PAGE_SHIFT);

            dma_free_coherent(&pdev->dev, xdev->H2C_dma_cfg[channel].size,
        	                  xdev->H2C_dma_cfg[channel].pData[i],
        			            xdev->H2C_dma_cfg[channel].data_phy_addr[i]);
           xdev->H2C_dma_cfg[channel].pData[i] = NULL;
        }
    }
}

static void xdma_free_pci_bus_addr(struct xdma_dev *xdev, struct pci_dev *pdev)
{
    int channel = 0;
    int c2h_num, h2c_num;

    for (channel = 0; channel < XDMA_CHANNEL_NUM; channel++) {
        /* C2H : pci bus addr*/
        c2h_num = xdev->C2H_dma_cfg[channel].SG_desc_num;
        xdma_free_c2h_pdata(xdev, channel, c2h_num, pdev);

        /* H2C : pci bus addr*/
        h2c_num = xdev->H2C_dma_cfg[channel].SG_desc_num;
        xdma_free_h2c_pdata(xdev, channel, h2c_num, pdev);
    }
}

static void xdma_free_h2c_desc(struct xdma_dev *xdev, struct pci_dev *pdev, int channel)
{
    if (xdev->H2C_dma_cfg[channel].pFirst_desc) {
        dma_free_coherent(&pdev->dev, xdev->H2C_dma_cfg[channel].SG_desc_num * sizeof(struct xdma_desc),
                              xdev->H2C_dma_cfg[channel].pFirst_desc,
                              xdev->H2C_dma_cfg[channel].first_desc_phy_addr);

        xdev->H2C_dma_cfg[channel].pFirst_desc = NULL;
    }
}

static void xdma_free_c2h_desc(struct xdma_dev *xdev, struct pci_dev *pdev, int channel)
{
    if (xdev->C2H_dma_cfg[channel].pFirst_desc) {
        dma_free_coherent(&pdev->dev, xdev->C2H_dma_cfg[channel].SG_desc_num * sizeof(struct xdma_desc),
                              xdev->C2H_dma_cfg[channel].pFirst_desc,
                              xdev->C2H_dma_cfg[channel].first_desc_phy_addr);

        xdev->C2H_dma_cfg[channel].pFirst_desc = NULL;
    }
}

static void xdma_free_desc(struct xdma_dev *xdev, struct pci_dev *pdev)
{
    int channel = 0;

    for (channel = 0; channel < XDMA_CHANNEL_NUM; channel++) {
        /* H2C xdma_desc */
        xdma_free_h2c_desc(xdev, pdev, channel);

        /* C2H xdma_desc */
        xdma_free_c2h_desc(xdev, pdev, channel);
    }
}

static void xdma_free_resource(struct pci_dev *pdev, void *dev_xdev)
{
	struct xdma_dev *xdev = (struct xdma_dev *)dev_xdev;

	if (!dev_xdev)
		return;

    xdma_free_pci_bus_addr(xdev, pdev);
    xdma_free_desc(xdev, pdev);
}

static void xpdev_free(struct xdma_pci_dev *xpdev)
{
	struct xdma_dev *xdev = xpdev->xdev;

	xdma_device_umap(xpdev->pdev, xdev);

	xpdev->xdev = NULL;
	kfree(xpdev);
}

static int request_regions(struct xdma_dev *xdev, struct pci_dev *pdev)
{
	int rv;

	if (!xdev) {
		pr_err("Invalid xdev\n");
		return -EINVAL;
	}

	if (!pdev) {
		pr_err("Invalid pdev\n");
		return -EINVAL;
	}

	dbg_init("pci_request_regions()\n");
	rv = pci_request_regions(pdev, xdev->mod_name);
	/* could not request all regions? */
	if (rv) {
		dbg_init("pci_request_regions() = %d, device in use?\n", rv);
		/* assume device is in use so do not disable it later */
		xdev->regions_in_use = 1;
	} else {
		xdev->got_regions = 1;
	}

	return rv;
}


void *xdma_device_map(const char *mname, struct pci_dev *pdev)
{
	struct xdma_dev *xdev = NULL;
	int rv = 0;

	/* allocate zeroed device book keeping structure */
	xdev = alloc_dev_instance(pdev);
	if (!xdev)
		return NULL;

	xdev->mod_name = mname;

    platform_setup_fops(xdev);

	rv = pci_enable_device(pdev);
	if (rv) {
		dbg_init("pci_enable_device() failed, %d.\n", rv);
		goto free_xdev;
	}

	/* enable bus master capability */
	pci_set_master(pdev);

	rv = request_regions(xdev, pdev);
	if (rv)
		goto err_regions;

	rv = map_bars(xdev, pdev);
	if (rv)
		goto err_map;

	rv = set_dma_mask(pdev);
	if (rv)
		goto err_mask;

	return (void *)xdev;

err_mask:
	unmap_bars(xdev, pdev);
err_map:
	if (xdev->got_regions)
		pci_release_regions(pdev);
err_regions:
	if (!xdev->regions_in_use)
		pci_disable_device(pdev);
free_xdev:
	kfree(xdev);

	return NULL;
}

static int _xdma_channel_init(struct xdma_dev *xdev, struct xdma_cfg_info *cfg, struct pci_info *vi53xx_xdma_info)
{
    vi53xx_xdma_info->setup(xdev->pdev, cfg);

    xdev->fops->poll_init(xdev, cfg->channel, cfg->direction);
    xdev->fops->stop(xdev, cfg->channel, cfg->direction);
    xdev->fops->create_transfer(xdev, cfg->channel, cfg);

    return 0;
}

static void  init_dma_cfg(struct xdma_cfg_info **dma_cfg, int direction, int channel)
{
    dma_cfg[direction][channel].direction  = direction;
    dma_cfg[direction][channel].channel = channel +1;
}

static int xdma_channel_init(struct xdma_pci_dev *xpdev, struct pci_info *info)
{
    int channel = 0;
    int direction;
    struct xdma_dev *xdev = xpdev->xdev;
    struct xdma_cfg_info **dma_cfg = xpdev->dma_cfg;

    if (!info)
        return -1;

    for (channel = 0; channel < XDMA_CHANNEL_NUM; channel++) { /*use CH1 CH2 --> XDMA_CHANNEL_NUM*/
        /* C2H */
        direction = C2H_DIR;
        init_dma_cfg(dma_cfg, direction, channel);
        _xdma_channel_init(xdev, &dma_cfg[direction][channel], info);

	    /* H2C */
        direction = H2C_DIR;
        init_dma_cfg(dma_cfg, direction, channel);
        if (dma_cfg[direction][channel].channel == CH1)
            _xdma_channel_init(xdev, &dma_cfg[direction][channel], info);
    }

    return 0;
}

static void dump_vi53xx_device_info(struct pci_dev *pdev)
{
	pr_info("0x%p ,  device found - pci bus: %d - id: 0x%04x - devfn: 0x%04x - pcie_type: 0x%04x.\n",
		    pdev, pdev->bus->number, pdev->device, pdev->devfn, pci_pcie_type(pdev));
}

static int xdma_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rv = 0;
	struct xdma_pci_dev *xpdev = NULL;
	struct xdma_dev *xdev = NULL;
    struct pci_info *vi53xx_xdma_info = (struct pci_info *)id->driver_data;

    dump_vi53xx_device_info(pdev);

	xpdev = xpdev_alloc(pdev);
	if (!xpdev)
		return -ENOMEM;

	xdev = xdma_device_map(DRV_MODULE_NAME, pdev);
	if (!xdev) {
		rv = -EINVAL;
        goto err_out;
	}

	xpdev->xdev = xdev;
    xpdev->dma_cfg[H2C_DIR] = xdev->H2C_dma_cfg;
    xpdev->dma_cfg[C2H_DIR] = xdev->C2H_dma_cfg;

    if (vi53xx_dev_init(xpdev) < 0) {
	    rv = -EINVAL;
        goto err_out;
	}

    if (xdma_channel_init(xpdev, vi53xx_xdma_info)) {
	    rv = -EINVAL;
        goto err_out;
    }

    vi53xx_lock();
	list_add(&xpdev->list, &pcie_device_list);
	vi53xx_unlock();

    dev_set_drvdata(&pdev->dev, xpdev);

	return 0;

err_out:
	pr_err("pdev 0x%p, err %d.\n", pdev, rv);
	xpdev_free(xpdev);

	return rv;
}

static void xdma_remove(struct pci_dev *pdev)
{
	struct xdma_pci_dev *xpdev;

	if (!pdev)
		return;

	xpdev = dev_get_drvdata(&pdev->dev);
	if (!xpdev)
		return;

    vi53xx_dev_clean(xpdev);

    xdma_free_resource(xpdev->pdev, xpdev->xdev);

	pr_info("pdev 0x%p, xdev 0x%p, 0x%p.\n",
		    pdev, xpdev, xpdev->xdev);

	xpdev_free(xpdev);

    vi53xx_lock();
	list_del(&xpdev->list);
	vi53xx_unlock();

	dev_set_drvdata(&pdev->dev, NULL);
}

static struct pci_driver pci_driver = {
	.name = DRV_MODULE_NAME,
	.id_table = pci_ids,
	.probe = xdma_probe,
	.remove = xdma_remove,
};

static int xdma_mod_init(void)
{
    if (init_board_info() < 0) {
	    pr_info("Init board info fail. no mem\n");
        return 0;
	}

	vi53xx_cdev_init();
	return pci_register_driver(&pci_driver);
}

static void xdma_mod_exit(void)
{
	pci_unregister_driver(&pci_driver);
	vi53xx_cdev_exit();
	board_info_exit();
}

module_init(xdma_mod_init);
module_exit(xdma_mod_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xgf");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
