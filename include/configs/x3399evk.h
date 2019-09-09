/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __X3399EVK_H
#define __X3399EVK_H

#include "rockchip-common.h"

/* ENV location - eMMC */
#define CONFIG_SYS_MMC_ENV_DEV                  0
#define CONFIG_SYS_MMC_ENV_PART                 1

#define SDRAM_BANK_SIZE                         (2UL << 30)
#define SDRAM_MAX_SIZE                          0xf8000000

#define CONFIG_SYS_SDRAM_BASE                   0
#define CONFIG_SYS_MALLOC_LEN                   (32 << 20)
#define CONFIG_SYS_CBSIZE                       1024
#define CONFIG_SKIP_LOWLEVEL_INIT

#define COUNTER_FREQUENCY                       24000000

#define CONFIG_SYS_NS16550_MEM32

#define CONFIG_SYS_INIT_SP_ADDR                 0x00300000
#define CONFIG_SYS_LOAD_ADDR                    0x00800800

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_TPL_BOOTROM_SUPPORT)
#define CONFIG_SPL_STACK                        0x00400000
#define CONFIG_SPL_MAX_SIZE                     0x100000
#define CONFIG_SPL_BSS_START_ADDR               0x00400000
#define CONFIG_SPL_BSS_MAX_SIZE                 0x2000
#else
#define CONFIG_SPL_STACK                        0xff8effff
#define CONFIG_SPL_MAX_SIZE                     0x30000 - 0x2000
#define CONFIG_SPL_BSS_START_ADDR               0xff8e0000
#define CONFIG_SPL_BSS_MAX_SIZE                 0x10000
#endif /* defined(CONFIG_SPL_BUILD) && defined(CONFIG_TPL_BOOTROM_SUPPORT) */

/* */
#define CONFIG_SYS_BOOTM_LEN                    (64 << 20)  /* 64M */

/* MMC/SD IP block */
#define CONFIG_ROCKCHIP_SDHCI_MAX_FREQ          200000000

/* RAW SD card / eMMC locations. */
#define CONFIG_SYS_SPI_U_BOOT_OFFS              (128 << 10)

#ifndef CONFIG_SPL_BUILD

#define ENV_MEM_LAYOUT_SETTINGS \
    "fdt_addr_r=0x01f00000\0" \
    "kernel_addr_r=0x02080000\0" \
    "ramdisk_addr_r=0x04000000\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
    "extraenv=none\0"

#endif /* ifndef CONFIG_SPL_BUILD */

#endif /* __X3399EVK_H */
