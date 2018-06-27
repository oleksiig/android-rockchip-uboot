// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <asm/arch-rockchip/periph.h>
#include <power/regulator.h>
#include <usb.h>
#include <dwc3-uboot.h>

DECLARE_GLOBAL_DATA_PTR;

int board_get_revision(void)
{
	return 1;
}

int board_init(void)
{
	int ret;

	ret = regulators_enable_boot_on(false);
	if (ret)
		pr_err("%s: Cannot enable boot on regulator\n", __func__);

	return 0;
}

#ifdef CONFIG_USB_DWC3

int usb_gadget_handle_interrupts(void)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	int node;
	const char *mode;
	bool matched = false;
	const void *blob = gd->fdt_blob;
	struct dwc3_device dwc3_device_data;

	/* find the usb_otg node */
	node = fdt_node_offset_by_compatible(blob, -1, "rockchip,rk3399-dwc3");

	while (node > 0) {
		mode = fdt_getprop(blob, node, "dr_mode", NULL);
		if (mode && strcmp(mode, "peripheral") == 0) {
			matched = true;
			break;
		}

		node = fdt_node_offset_by_compatible(blob, node, "snps,dwc3");
	}
	if (!matched) {
		pr_err("Not found usb_otg device\n");
		return -ENODEV;
	}

	dwc3_device_data.index = index;
	dwc3_device_data.base = fdtdec_get_addr(blob, node, "reg");
	dwc3_device_data.dr_mode = USB_DR_MODE_PERIPHERAL;
	dwc3_device_data.maximum_speed = USB_SPEED_HIGH;
	dwc3_device_data.dis_u2_susphy_quirk = 1;
	dwc3_device_data.usb2_phyif_utmi_width = 16;

	return dwc3_uboot_init(&dwc3_device_data);
}
#endif
