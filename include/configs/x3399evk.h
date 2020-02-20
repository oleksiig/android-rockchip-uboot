/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __X3399EVK_H
#define __X3399EVK_H

#include "rockchip-common.h"

#define CONFIG_GICV3                            1
#define GICD_BASE                               0xFEE00000
#define GICR_BASE                               0xFEF00000
#define GICC_BASE                               0xFFF00000

/* ENV location - eMMC */
#define CONFIG_SYS_MMC_ENV_DEV                  0
#define CONFIG_SYS_MMC_ENV_PART                 1

#define SDRAM_BANK_SIZE                         (2UL << 30)
#define SDRAM_MAX_SIZE                          0xf8000000

#define CONFIG_SYS_SDRAM_BASE                   0
#define CONFIG_SYS_MALLOC_LEN                   (64 * 1024 * 1024)
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

#define CONFIG_EXTRA_ENV_SETTINGS \
    "uuid_gpt_disk=5DE6B69D-5FFA-4CEA-97DE-A5F287DB3B44\0" \
    "uuid_gpt_uboot=3E4FC4F8-AD40-4519-A73F-A070E0190BFE\0" \
    "uuid_gpt_trust=D61F2101-DBC0-4928-A05C-B74CD12B9E42\0" \
    "uuid_gpt_boot=F4C2B80D-FFD1-4D76-A63C-FF56ACA2F3B0\0" \
    "uuid_gpt_system=CF16C17A-7935-421F-80AC-BD870520E000\0" \
    "uuid_gpt_vendor=7C57E557-063D-485D-B9F1-A3A7B3614901\0" \
    "uuid_gpt_cache=850FCD31-6A0D-44C4-848B-4781879301F1\0" \
    "uuid_gpt_vbmeta=43E69039-4608-4591-8C92-FF5758DB76E6\0" \
    "uuid_gpt_misc=E4AD2B83-18C9-4C08-8DD6-99B3C80598C9\0" \
    "uuid_gpt_frp=888FCC59-A68E-4BA3-8D52-7F81628F5147\0" \
    "uuid_gpt_metadata=75028084-CF5A-4191-9A1B-E208FC399DC2\0" \
    "uuid_gpt_userdata=196D4D41-83B5-4B2C-899D-02FE19D27BCA\0" \
    "partitions=uuid_disk=${uuid_gpt_disk};" \
        "name=uboot,start=8M,size=4M,uuid=${uuid_gpt_uboot};" \
        "name=trust,size=4M,uuid=${uuid_gpt_trust};" \
        "name=boot,size=32M,uuid=${uuid_gpt_boot};" \
        "name=system,size=2G,uuid=${uuid_gpt_system};" \
        "name=vendor,size=256M,uuid=${uuid_gpt_vendor};" \
        "name=cache,size=100M,uuid=${uuid_gpt_cache};" \
        "name=vbmeta,size=512K,uuid=${uuid_gpt_vbmeta};" \
        "name=misc,size=512K,uuid=${uuid_gpt_misc};" \
        "name=frp,size=512K,uuid=${uuid_gpt_frp};" \
        "name=metadata,size=16M,uuid=${uuid_gpt_metadata};" \
        "name=userdata,size=-,uuid=${uuid_gpt_userdata};\0" \
    "fstype_system=ext4\0" \
    "fstype_vendor=ext4\0" \
    "fstype_cache=ext4\0" \
    "fstype_metadata=ext4\0" \
    "fstype_userdata=ext4\0" \
    "serial#=3399\0"

#endif /* ifndef CONFIG_SPL_BUILD */

#endif /* __X3399EVK_H */
