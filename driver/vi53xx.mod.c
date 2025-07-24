#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x8a443098, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x5b3e5384, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xc1541bfd, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x2f570610, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x18880b49, __VMLINUX_SYMBOL_STR(single_open) },
	{ 0x13c775cc, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x5c2e3421, __VMLINUX_SYMBOL_STR(del_timer) },
	{ 0x5b622535, __VMLINUX_SYMBOL_STR(single_release) },
	{ 0xe082781, __VMLINUX_SYMBOL_STR(pci_disable_device) },
	{ 0xeea2d09c, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0x4c71c1fb, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x238c8b66, __VMLINUX_SYMBOL_STR(kobject_set_name) },
	{ 0xc29957c3, __VMLINUX_SYMBOL_STR(__x86_indirect_thunk_rcx) },
	{ 0x1d8a72c3, __VMLINUX_SYMBOL_STR(pci_release_regions) },
	{ 0x5ee52022, __VMLINUX_SYMBOL_STR(init_timer_key) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xde241d5e, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0x15ba50a6, __VMLINUX_SYMBOL_STR(jiffies) },
	{ 0xc671e369, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x6535bfd7, __VMLINUX_SYMBOL_STR(PDE_DATA) },
	{ 0xe7c72217, __VMLINUX_SYMBOL_STR(pci_set_master) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x24f77944, __VMLINUX_SYMBOL_STR(proc_mkdir) },
	{ 0xf1eb0f8d, __VMLINUX_SYMBOL_STR(pci_iounmap) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0x587c8d3f, __VMLINUX_SYMBOL_STR(down) },
	{ 0xd27eb051, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x62451f4, __VMLINUX_SYMBOL_STR(add_timer) },
	{ 0x536655e9, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x98834566, __VMLINUX_SYMBOL_STR(arch_dma_alloc_attrs) },
	{ 0xb601be4c, __VMLINUX_SYMBOL_STR(__x86_indirect_thunk_rdx) },
	{ 0x2ea2c95c, __VMLINUX_SYMBOL_STR(__x86_indirect_thunk_rax) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0x9e4e568d, __VMLINUX_SYMBOL_STR(pci_unregister_driver) },
	{ 0x897026c6, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x55edf878, __VMLINUX_SYMBOL_STR(remove_proc_subtree) },
	{ 0xdb8ad7ae, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x563dc39d, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x44f2b260, __VMLINUX_SYMBOL_STR(pci_request_regions) },
	{ 0x57b9c29d, __VMLINUX_SYMBOL_STR(dma_supported) },
	{ 0x17c8215e, __VMLINUX_SYMBOL_STR(up) },
	{ 0x7d6beda1, __VMLINUX_SYMBOL_STR(__pci_register_driver) },
	{ 0xf4374310, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x91607d95, __VMLINUX_SYMBOL_STR(set_memory_wb) },
	{ 0x15615ede, __VMLINUX_SYMBOL_STR(dma_common_mmap) },
	{ 0x405246a9, __VMLINUX_SYMBOL_STR(pci_iomap) },
	{ 0x436c2179, __VMLINUX_SYMBOL_STR(iowrite32) },
	{ 0xb2aadf0b, __VMLINUX_SYMBOL_STR(pci_enable_device) },
	{ 0xb5419b40, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xe50ad56a, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0xd94824f5, __VMLINUX_SYMBOL_STR(dma_ops) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xe484e35f, __VMLINUX_SYMBOL_STR(ioread32) },
	{ 0xab65ed80, __VMLINUX_SYMBOL_STR(set_memory_uc) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("pci:v000010EEd000016F2sv*sd*bc*sc*i*");
