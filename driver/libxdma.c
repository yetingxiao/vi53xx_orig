#include  "libxdma.h"

LIST_HEAD(pcie_device_list);

u32 read_register(void *iomem)
{
	return ioread32(iomem);
}

static unsigned int xdma_read_reg(void *reg_base, unsigned int offset)
{
	void *reg = reg_base + offset;

	return read_register(reg);
}

static void xdma_write_reg(void *reg_base, unsigned int offset, unsigned int val)
{
	void *reg = reg_base + offset;

	write_register(val, reg);
	dbg_init("xdma write bar1 offset[0x%x] value = [0x%x]\n", offset, val);
}

static unsigned int read_reg(void *reg_base, unsigned int offset)
{
	void *reg = reg_base + offset;

	return read_register(reg);
}

static unsigned int write_reg(void *reg_base, unsigned int offset, unsigned int val)
{
	void *reg = reg_base + offset;

	write_register(val, reg);

	return 0;
}

static int xdma_start(struct xdma_dev *xdev, unsigned char channel, unsigned char direction)
{
	unsigned char channel_index = channel - 1;
	unsigned int tmp_value = 0;
	unsigned int DMA_start = 0x01; // Running the SGDMA engine
	void *reg_base = xdev->bar[1];

	tmp_value = (1 << 26) | (1 << 2) | DMA_start;
	if (direction == 0) {
		xdma_write_reg(reg_base, H2C_CONTROL_REG + channel_index * 0x100, tmp_value);

		tmp_value = xdma_read_reg(reg_base, H2C_CONTROL_REG + channel_index * 0x100);
	} else {
		xdma_write_reg(reg_base, C2H_CONTROL_REG + channel_index * 0x100, tmp_value);

		tmp_value = xdma_read_reg(reg_base, C2H_CONTROL_REG + channel_index * 0x100);
	}

	return 0;
}

static int xdma_start_desc(struct xdma_dev *xdev, unsigned char channel, struct xdma_cfg_info *cfg)
{
	unsigned char ch_index = channel - 1;

	uint8_t SG_num = cfg->SG_desc_num;
	uint8_t dir = cfg->direction;
	void *reg_base = xdev->bar[1];

	if (SG_num > 1)	{
		if (dir == 0)
			xdma_write_reg(reg_base, H2C_SGDMA_DESC_ADD_REG + ch_index * 0x100, SG_num);
		else
			xdma_write_reg(reg_base, C2H_SGDMA_DESC_ADD_REG + ch_index * 0x100, SG_num);
	}

	return 0;
}

static int xdma_cofg_desc(struct xdma_dev *xdev, unsigned char channel, struct xdma_cfg_info *cfg)
{
	unsigned char ch_index = channel - 1;
	dma_addr_t first_desc_phy = cfg->first_desc_phy_addr;
	unsigned int low_addr = PCI_DMA_L(first_desc_phy);
	unsigned int high_addr = PCI_DMA_H(first_desc_phy);
	uint8_t SG_num = cfg->SG_desc_num;
	uint8_t dir = cfg->direction;
	unsigned int tmp_desc = 0;

	void *reg_base = xdev->bar[1];

	if (dir == 0) {
		if (SG_num > 1) {
			tmp_desc = xdma_read_reg(reg_base, SGDMA_DESC_MODE_EN_REG);
			tmp_desc |= 0x01 << ch_index;
			xdma_write_reg(reg_base, SGDMA_DESC_MODE_EN_REG, tmp_desc);
		}

		xdma_write_reg(reg_base, H2C_SGDMA_DESC_LOW32_REG + ch_index * 0x100, low_addr);
		xdma_write_reg(reg_base, H2C_SGDMA_DESC_HIGH32_REG + ch_index * 0x100, high_addr);
		xdma_write_reg(reg_base, H2C_SGDMA_DESC_ADJA_REG + ch_index * 0x100, SG_num - 1);

		if (SG_num > 1)
			xdma_write_reg(reg_base, H2C_SGDMA_DESC_ADD_REG + ch_index * 0x100, SG_num);
	} else {
		if (SG_num > 1) {
			tmp_desc = xdma_read_reg(reg_base, SGDMA_DESC_MODE_EN_REG);
			tmp_desc |= 0x01 << (ch_index + 16);
			xdma_write_reg(reg_base, SGDMA_DESC_MODE_EN_REG, tmp_desc);
		}

		xdma_write_reg(reg_base, C2H_SGDMA_DESC_LOW32_REG + ch_index * 0x100, low_addr);
		xdma_write_reg(reg_base, C2H_SGDMA_DESC_HIGH32_REG + ch_index * 0x100, high_addr);
		xdma_write_reg(reg_base, C2H_SGDMA_DESC_ADJA_REG + ch_index * 0x100, SG_num - 1);

		if (SG_num > 1)
			xdma_write_reg(reg_base, C2H_SGDMA_DESC_ADD_REG + ch_index * 0x100, SG_num);

	}

	return 0;
}

static void xdma_desc_set_ep_addr(struct xdma_desc *desc_virt, u32 ep_addr, unsigned char direction)
{
	if (direction == 0) {
		desc_virt->dst_addr_lo = cpu_to_le32(PCI_DMA_L(ep_addr));
		desc_virt->dst_addr_hi = cpu_to_le32(PCI_DMA_H(ep_addr));
	} else {
		desc_virt->src_addr_lo = cpu_to_le32(PCI_DMA_L(ep_addr));
		desc_virt->src_addr_hi = cpu_to_le32(PCI_DMA_H(ep_addr));
	}
}

static void xdma_desc_set_addr(struct xdma_desc *desc_virt,  dma_addr_t buff_phy_addr, unsigned char direction, u32 ep_addr)
{
	if (direction == 0) {/* H2C */
		desc_virt->src_addr_lo = cpu_to_le32(PCI_DMA_L(buff_phy_addr));
		desc_virt->src_addr_hi = cpu_to_le32(PCI_DMA_H(buff_phy_addr));
	} else { /* C2H */
		desc_virt->dst_addr_lo = cpu_to_le32(PCI_DMA_L(buff_phy_addr));
		desc_virt->dst_addr_hi = cpu_to_le32(PCI_DMA_H(buff_phy_addr));
	}

	xdma_desc_set_ep_addr(desc_virt, ep_addr, direction);
}

static struct xdma_desc *xdma_desc_alloc(struct pci_dev *pdev, unsigned char channel, struct xdma_cfg_info *cfg)
{
	struct xdma_desc *desc_virt = NULL;	/* virtual address */
	uint8_t SG_num = cfg->SG_desc_num;
	uint8_t adj = SG_num - 1;
	uint32_t extra_adj = 0;                     /* Nxt_adj*/
	uint32_t temp_control = 0;
	uint32_t final_desc_control = 0x13;
	dma_addr_t first_desc_phy = 0;
	dma_addr_t next_desc_phy_addr = 0;
	dma_addr_t data_phy_addr_tmp = 0;
	uint8_t dir_tmp = cfg->direction;
	int i = 0;
	u32 ep_addr = cfg->ep_addr;

	desc_virt = dma_alloc_coherent(&pdev->dev, SG_num * sizeof(struct xdma_desc), &first_desc_phy, GFP_KERNEL);
	if (!desc_virt) {
		printk("System Error: DMA Memory Allocate.\n");
		return NULL;
	}

	for (i = 0; i < SG_num-1; i++) {
		/* any adjacent descriptors? */
		if (adj > 0) {
			extra_adj = adj - 1;
			if (extra_adj > MAX_EXTRA_ADJ)
				extra_adj = MAX_EXTRA_ADJ;
			adj--;
		} else {
			extra_adj = 0;
		}

		temp_control = DESC_MAGIC | (extra_adj << 8);
		desc_virt[i].control = cpu_to_le32(temp_control);
		desc_virt[i].bytes   = cpu_to_le32(cfg->data_len[i]);

		data_phy_addr_tmp    = cfg->data_phy_addr[i];

		xdma_desc_set_addr(&desc_virt[i], data_phy_addr_tmp, dir_tmp, ep_addr + i*0x2000);

		next_desc_phy_addr   = first_desc_phy + (i+1) * sizeof(struct xdma_desc);
		desc_virt[i].next_lo = cpu_to_le32(PCI_DMA_L(next_desc_phy_addr));
		desc_virt[i].next_hi = cpu_to_le32(PCI_DMA_H(next_desc_phy_addr));

	}

	temp_control         = DESC_MAGIC | (extra_adj << 8) | final_desc_control;
	desc_virt[i].control = cpu_to_le32(temp_control);
	desc_virt[i].bytes   = cpu_to_le32(cfg->data_len[i]);
	data_phy_addr_tmp    = cfg->data_phy_addr[i];

	xdma_desc_set_addr(&desc_virt[i], data_phy_addr_tmp, dir_tmp, ep_addr + (SG_num-1)*0x2000);

	cfg->pFirst_desc = desc_virt;
	cfg->first_desc_phy_addr = first_desc_phy;

	return desc_virt;
}

static int xdma_stop(struct xdma_dev *xdev, unsigned char channel, unsigned char direction)
{
	unsigned char channel_index = channel - 1;
	void *reg_base = xdev->bar[1];

	if (direction == 0)
		xdma_write_reg(reg_base, H2C_CONTROL_REG + channel_index * 0x100, DMA_STOP);
	else
		xdma_write_reg(reg_base, C2H_CONTROL_REG + channel_index * 0x100, DMA_STOP);

	return  0;
}

static int xdma_poll_init(struct xdma_dev *xdev, int channel, unsigned char direction)
{
	int channel_index = channel - 1;
	void *reg_base = xdev->bar[1];

	unsigned int tmp_value = 0;
	unsigned int poll_enable = (1 << 26) | (1 << 2); // poll init

	if (direction == 0) {
		tmp_value = xdma_read_reg(reg_base,  H2C_CONTROL_REG + channel_index * 0x100);
		tmp_value = tmp_value | poll_enable;
		xdma_write_reg(reg_base, H2C_CONTROL_REG + channel_index*0x100, tmp_value);
	} else {
		tmp_value = xdma_read_reg(reg_base, C2H_CONTROL_REG + channel_index*0x100);
		tmp_value = tmp_value | poll_enable;
		xdma_write_reg(reg_base, C2H_CONTROL_REG + channel_index*0x100, tmp_value);
	}

	return 0;
}

static void xdma_create_transfer(struct xdma_dev *xdev, unsigned char channel, struct xdma_cfg_info *cfg)
{
	xdma_desc_alloc(xdev->pdev, channel, cfg);
	xdma_cofg_desc(xdev, channel, cfg);
}

static int xdma_mmap(struct xdma_dev *xdev, void *vm, struct xdma_cfg_info *cfg)
{
	struct xdma_pci_dev *xpdev;
	struct pci_dev *pdev = xdev->pdev;
	struct vm_area_struct *vma = (struct vm_area_struct *)vm;

	xpdev = dev_get_drvdata(&pdev->dev);
	if (!xpdev->fmap[cfg->direction][cfg->channel]) {
		set_memory_uc((unsigned long)cfg->pData[0], cfg->size >> PAGE_SHIFT);
		xpdev->fmap[cfg->direction][cfg->channel] = 1;
		dev_set_drvdata(&pdev->dev, xpdev);
	}

	return dma_common_mmap(&pdev->dev, vma, cfg->pData[0], cfg->data_phy_addr[0], vma->vm_end - vma->vm_start);
}

static struct xdma_fops rtpc_xdma_fops = {
	.read_reg = read_reg,
	.write_reg = write_reg,
	.start = xdma_start,
	.start_desc = xdma_start_desc,
	.stop = xdma_stop,
	.poll_init = xdma_poll_init,
	.create_transfer = xdma_create_transfer,
	.xdma_mmap = xdma_mmap,
};

void  platform_setup_fops(struct xdma_dev *xdev)
{
	xdev->fops = &rtpc_xdma_fops;
}

