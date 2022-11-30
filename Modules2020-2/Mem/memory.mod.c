#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xf1fbc9ab, "module_layout" },
	{ 0x5c599162, "kmem_cache_alloc_trace" },
	{ 0xd1ed3dd3, "kmalloc_caches" },
	{ 0x110631e3, "__register_chrdev" },
	{ 0x37a0cba, "kfree" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xcf2a6966, "up" },
	{ 0x6626afca, "down" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x6bd0e573, "down_interruptible" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");

