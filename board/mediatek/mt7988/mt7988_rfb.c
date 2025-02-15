// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <config.h>
#include <dm.h>
#include <button.h>
#include <env.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/libfdt.h>

#ifndef CONFIG_RESET_BUTTON_LABEL
#define CONFIG_RESET_BUTTON_LABEL "reset"
#endif

int board_init(void)
{
	return 0;
}

int board_late_init(void)
{
	gd->env_valid = 1; //to load environment variable from persistent store
	struct udevice *dev;

	gd->env_valid = ENV_VALID;
	if (!button_get_by_label(CONFIG_RESET_BUTTON_LABEL, &dev)) {
		puts("reset button found\n");
#ifdef CONFIG_RESET_BUTTON_SETTLE_DELAY
		if (CONFIG_RESET_BUTTON_SETTLE_DELAY > 0) {
			button_get_state(dev);
			mdelay(CONFIG_RESET_BUTTON_SETTLE_DELAY);
		}
#endif
		if (button_get_state(dev) == BUTTON_ON) {
			puts("button pushed, resetting environment\n");
			gd->env_valid = ENV_INVALID;
		}
	}
	env_relocate();
	return 0;
}

#define	MT7988_BOOT_NOR		0
#define	MT7988_BOOT_SPIM_NAND	1
#define	MT7988_BOOT_EMMC	2
#define	MT7988_BOOT_SNFI_NAND	3

int ft_system_setup(void *blob, struct bd_info *bd)
{
	const u32 *media_handle_p;
	int chosen, len, ret;
	const char *media;
	u32 media_handle;

	switch ((readl(0x1001f6f0) & 0xc00) >> 10) {
	case MT7988_BOOT_NOR:
		media = "rootdisk-nor";
		break
		;;
	case MT7988_BOOT_SPIM_NAND:
		media = "rootdisk-spim-nand";
		break
		;;
	case MT7988_BOOT_EMMC:
		media = "rootdisk-emmc";
		break
		;;
	case MT7988_BOOT_SNFI_NAND:
		media = "rootdisk-sd";
		break
		;;
	}

	chosen = fdt_path_offset(blob, "/chosen");
	if (chosen <= 0)
		return 0;

	media_handle_p = fdt_getprop(blob, chosen, media, &len);
	if (media_handle_p <= 0 || len != 4)
		return 0;

	media_handle = *media_handle_p;
	ret = fdt_setprop(blob, chosen, "rootdisk", &media_handle, sizeof(media_handle));
	if (ret) {
		printf("cannot set media phandle %s as rootdisk /chosen node\n", media);
		return ret;
	}

	printf("set /chosen/rootdisk to bootrom media: %s (phandle 0x%08x)\n", media, fdt32_to_cpu(media_handle));

	return 0;
}
