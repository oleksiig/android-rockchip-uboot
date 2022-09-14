/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2019 Vasily Khoruzhick <anarsoul@gmail.com>
 */

#ifndef __ROCKPRO64_RK3399_H
#define __ROCKPRO64_RK3399_H

#include "rockchip-common.h"

#define CONFIG_GICV3				1

#define GICD_BASE					0xFEE00000
#define GICR_BASE					0xFEF00000
#define GICC_BASE					0xFFF00000

#define CFG_IRAM_BASE				0xff8c0000
#define CFG_SYS_SDRAM_BASE			0x00000000

#define SDRAM_BANK_SIZE				(2UL << 30)
#define SDRAM_MAX_SIZE				0xf8000000

#define CONFIG_SYS_SDRAM_BASE		0x00000000

#ifndef CONFIG_SPL_BUILD

#define CFG_EXTRA_ENV_SETTINGS \
    "uuid_gpt_disk=5DE6B69D-5FFA-4CEA-97DE-A5F287DB3B44\0" \
    "uuid_gpt_uboot=3E4FC4F8-AD40-4519-A73F-A070E0190BFE\0" \
    "uuid_gpt_trust=D61F2101-DBC0-4928-A05C-B74CD12B9E42\0" \
    "uuid_gpt_boot=F4C2B80D-FFD1-4D76-A63C-FF56ACA2F3B0\0" \
    "uuid_gpt_system=CF16C17A-7935-421F-80AC-BD870520E000\0" \
    "uuid_gpt_vendor=7C57E557-063D-485D-B9F1-A3A7B3614901\0" \
    "uuid_gpt_vbmeta=43E69039-4608-4591-8C92-FF5758DB76E6\0" \
    "uuid_gpt_misc=E4AD2B83-18C9-4C08-8DD6-99B3C80598C9\0" \
    "uuid_gpt_frp=888FCC59-A68E-4BA3-8D52-7F81628F5147\0" \
    "uuid_gpt_metadata=75028084-CF5A-4191-9A1B-E208FC399DC2\0" \
    "uuid_gpt_userdata=196D4D41-83B5-4B2C-899D-02FE19D27BCA\0" \
    "partitions=uuid_disk=${uuid_gpt_disk};" \
        "name=uboot,start=8M,size=4M,uuid=${uuid_gpt_uboot};" \
        "name=trust,size=4M,uuid=${uuid_gpt_trust};" \
        "name=boot_a,size=32M,uuid=${uuid_gpt_boot};" \
        "name=system_a,size=2G,uuid=${uuid_gpt_system};" \
        "name=vendor_a,size=256M,uuid=${uuid_gpt_vendor};" \
        "name=vbmeta_a,size=512K,uuid=${uuid_gpt_vbmeta};" \
        "name=misc_a,size=512K,uuid=${uuid_gpt_misc};" \
        "name=frp,size=512K,uuid=${uuid_gpt_frp};" \
        "name=metadata,size=16M,uuid=${uuid_gpt_metadata};" \
        "name=userdata,size=-,uuid=${uuid_gpt_userdata};\0" \
    "fstype_system=ext4\0" \
    "fstype_vendor=ext4\0" \
    "fstype_metadata=ext4\0" \
    "fstype_userdata=ext4\0" \
    "product=rockpro64\0"

#endif /* ifndef CONFIG_SPL_BUILD */

#endif /* __ROCKPRO64_RK3399_H */
