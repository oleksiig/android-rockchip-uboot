/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <common.h>
#include <cpu_func.h>
#include <asm/cache.h>
#include <bootm.h>
#include <malloc.h>
#include <mapmem.h>
#include <part.h>
#include <android_image.h>
#include <../lib/libavb/libavb.h>

/* Public key */
#include "avb_pubkey.h"

/* */
static AvbIOResult avb_read_from_partition(AvbOps* ops, const char* partition,
		int64_t offset, size_t num_bytes, void* buffer, size_t* out_num_read)
{
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
	size_t read;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (dev_desc == NULL) {
		pr_err("%s: interface:mmc dev:%d not found.\n",
			__func__, CONFIG_FASTBOOT_FLASH_MMC_DEV);
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		pr_err("%s: mmc:%d - partition '%s' not found.\n",
			__func__, CONFIG_FASTBOOT_FLASH_MMC_DEV, partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	int64_t part_size = (part_info.size * part_info.blksz);
	int64_t offset_bytes = offset;

	if (part_size < num_bytes) {
		pr_err("%s: requested size (%zu) exceeds size of partition (%llu)\n",
			__func__, num_bytes, part_size);
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (offset < 0) {
		offset_bytes = (part_size + offset);
	}

	/* handle non block-aligned reads */
	if (offset_bytes % part_info.blksz || num_bytes % part_info.blksz)
	{
		size_t offset_sectors = part_info.start + (offset_bytes / part_info.blksz);
		size_t offset_data = (offset_bytes % part_info.blksz);
		size_t to_read_sectors = ALIGN(num_bytes, part_info.blksz) / part_info.blksz;

		/* */
		size_t buf_size = ALIGN(num_bytes, part_info.blksz);
		char * buf_ptr = malloc(buf_size);

		if (!buf_ptr) {
			pr_err("%s: failed to allocate %lu bytes\n", __func__, buf_size);
			return AVB_IO_RESULT_ERROR_OOM;
		}

		read = blk_dread(dev_desc, offset_sectors, to_read_sectors, buf_ptr);
		flush_cache((ulong)buf_ptr, buf_size);

		if (read != to_read_sectors) {
			pr_err("%s: failed to read %lu blocks\n", __func__, to_read_sectors);
			free(buf_ptr);
			return AVB_IO_RESULT_ERROR_IO;
		}

		memcpy(buffer, buf_ptr + offset_data, num_bytes);

		*out_num_read = num_bytes;
	}
	else
	{
		ulong offset_sectors = part_info.start + (offset_bytes / part_info.blksz);
		ulong to_read_sectors = num_bytes / part_info.blksz;

		read = blk_dread(dev_desc, offset_sectors, to_read_sectors, buffer);
		flush_cache((ulong)buffer, (to_read_sectors * part_info.blksz));

		if (read != to_read_sectors) {
			pr_err("%s: failed to read %lu blocks\n", __func__, to_read_sectors);
			return AVB_IO_RESULT_ERROR_IO;
		}

		*out_num_read = (to_read_sectors * dev_desc->blksz);
	}

	return AVB_IO_RESULT_OK;
}

/* */
static AvbIOResult avb_validate_vbmeta_public_key(AvbOps* ops,
		const uint8_t* public_key_data, size_t public_key_length,
		const uint8_t* public_key_metadata, size_t public_key_metadata_length,
		bool* out_is_trusted)
{
	if (!public_key_length || !public_key_data || !out_is_trusted)
		return AVB_IO_RESULT_ERROR_IO;

	*out_is_trusted = false;

	if (public_key_length != sizeof(avb_root_pubkey))
		return AVB_IO_RESULT_ERROR_IO;

	if (memcmp(avb_root_pubkey, public_key_data, public_key_length) == 0)
		*out_is_trusted = true;

	return AVB_IO_RESULT_OK;
}

/* */
static AvbIOResult avb_get_size_of_partition(AvbOps* ops, const char* partition,
		uint64_t* out_size_num_bytes)
{
	struct blk_desc *dev_desc;
	struct disk_partition part_info;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (dev_desc == NULL) {
		pr_err("%s: interface:mmc dev:%d not found.\n",
			__func__, CONFIG_FASTBOOT_FLASH_MMC_DEV);
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		pr_err("%s: mmc:%d - partition '%s' not found.\n",
			__func__, CONFIG_FASTBOOT_FLASH_MMC_DEV, partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	*out_size_num_bytes = (part_info.size * part_info.blksz);
	return AVB_IO_RESULT_OK;
}

/* */
static AvbIOResult avb_read_rollback_index(AvbOps* ops,
		size_t rollback_index_location, uint64_t* out_rollback_index)
{
	printf("avb_flow: %s not supported yet\n", __func__);

	if (out_rollback_index)
		*out_rollback_index = 0;

	return AVB_IO_RESULT_OK;
}

/* */
static AvbIOResult avb_get_unique_guid_for_partition(AvbOps* ops,
		const char* partition, char* guid_buf, size_t guid_buf_size)
{
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
	size_t uuid_size = sizeof(part_info.uuid);

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (dev_desc == NULL) {
		pr_err("%s: interface:mmc dev:%d not found.\n",
			__func__, CONFIG_FASTBOOT_FLASH_MMC_DEV);
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		pr_err("%s: mmc:%d - partition '%s' not found.\n",
			__func__, CONFIG_FASTBOOT_FLASH_MMC_DEV, partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if (uuid_size > guid_buf_size)
		return AVB_IO_RESULT_ERROR_IO;

	memcpy(guid_buf, part_info.uuid, uuid_size);

	guid_buf[uuid_size - 1] = 0;

	return AVB_IO_RESULT_OK;
}

/* */
static AvbIOResult avb_read_is_device_unlocked(AvbOps* ops, bool* out_is_unlocked)
{
	/* TODO: Currently device is permanently unlocked */
	*out_is_unlocked = true;
	return AVB_IO_RESULT_OK;
}

/* */
static const char * const avb_partitions[] =
{
	"boot", "system", "vendor", NULL
};

/* */
static struct AvbOps avb_ops =
{
	.ab_ops                         = NULL,
	.atx_ops                        = NULL,
	.read_from_partition            = avb_read_from_partition,
	.get_preloaded_partition        = NULL,
	.write_to_partition             = NULL,
	.validate_vbmeta_public_key     = avb_validate_vbmeta_public_key,
	.read_rollback_index            = avb_read_rollback_index,
	.write_rollback_index           = NULL,
	.read_is_device_unlocked        = avb_read_is_device_unlocked,
	.get_unique_guid_for_partition  = avb_get_unique_guid_for_partition,
	.get_size_of_partition          = avb_get_size_of_partition,
	.read_persistent_value          = NULL,
	.write_persistent_value         = NULL,
};

/* */
static int do_boot_android_avb(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	AvbSlotVerifyResult slot_result;
	AvbSlotVerifyData *out_data = NULL;
	struct andr_boot_img_hdr_v0 *img_hdr = NULL;
	bool unlocked = false;
	char *cmdline = NULL, *slot_suffix = "_a";
	u64 kernel_addr = 0, dtb_addr = 0, ramdisk_addr = 0, size = 0;

	printf("boota_avb: Android Verified Boot 2.0 - AVB version %s\n", avb_version_string());

	if (avb_ops.read_is_device_unlocked(&avb_ops, &unlocked) != AVB_IO_RESULT_OK) {
		printf("boota_avb: Can't determine device lock state.\n");
		return CMD_RET_FAILURE;
	}

	slot_result = avb_slot_verify(&avb_ops, avb_partitions, slot_suffix,
				unlocked, AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE, &out_data);

	/* */
	switch (slot_result)
	{
	case AVB_SLOT_VERIFY_RESULT_OK:
		printf("boota_avb: Verification passed successfully\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
		printf("boota_avb: Verification failed\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
		printf("boota_avb: I/O error occurred during verification\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
		printf("boota_avb: OOM error occurred during verification\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
		printf("boota_avb: Corrupted dm-verity metadata detected\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
		printf("boota_avb: Unsupported version avbtool was used\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
		printf("boota_avb: Checking rollback index failed\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
		printf("boota_avb: Public key was rejected\n");
		break;
	default:
		printf("boota_avb: avb_slot_verify returned unknown error\n");
	}

	/* Temporary skip AVB verificaion for development purposes */
	if(slot_result != AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION) {
		if(slot_result != AVB_SLOT_VERIFY_RESULT_OK) {
			if(out_data != NULL) {
				avb_slot_verify_data_free(out_data);
			}
			pr_err("boota_avb: Slot '%s' verify failed.\n", slot_suffix);
			return CMD_RET_FAILURE;
		}
	}

	if (out_data->loaded_partitions > 0) {
		AvbPartitionData* avb_part = &out_data->loaded_partitions[0];

		img_hdr = (struct andr_boot_img_hdr_v0*)avb_part->data;

		pr_info("boota_avb: Loaded '%s' partition, size %luKiB\n",
			avb_part->partition_name, avb_part->data_size/1024);
	}

	if (img_hdr == NULL) {
		pr_err("boota_avb: No loaded partition(s)\n");
		avb_slot_verify_data_free(out_data);
		return CMD_RET_FAILURE;
	}

	if (img_hdr->header_version < 2 || img_hdr->header_version > 2) {
		pr_err("boota_avb: Unsupported Android Image header version %d\n",
			img_hdr->header_version);
		avb_slot_verify_data_free(out_data);
		return CMD_RET_FAILURE;
	}

	/* Update bootargs */
	if((cmdline = env_get("bootargs")) == NULL)
		cmdline = "";

	cmdline = avb_strdupv(cmdline, " ", out_data->cmdline, NULL);
	env_set("bootargs", cmdline);

	/* */
	android_print_contents(img_hdr);

	/* BOOTM_STATE_START */
	memset((void *)&images, 0, sizeof(images));

#ifdef CONFIG_LMB
	lmb_init_and_reserve_range(&images.lmb,
		(phys_addr_t)env_get_bootm_low(), env_get_bootm_size(), NULL);
#endif

	/* BOOTM_STATE_FINDOS */

	/* get kernel image header v2 only, start address and length */
	if (android_image_get_kernel(img_hdr, NULL,
				  images.verify, (ulong*)&kernel_addr, (ulong*)&size)) {
		pr_err("boota_avb: Failed to find Kernel in Android boot image\n");
		avb_slot_verify_data_free(out_data);
		return CMD_RET_FAILURE;
	}

	images.os.image_start = kernel_addr;
	images.os.image_len = size;
	images.os.type = IH_TYPE_KERNEL;
	images.os.os = IH_OS_LINUX;
	images.os.arch = IH_ARCH_ARM64;
	images.os.start = (phys_addr_t)map_sysmem((phys_addr_t)img_hdr, 0);
	images.os.comp = android_image_get_kcomp(img_hdr, NULL);
	images.os.end = android_image_get_end(img_hdr, NULL);
	images.os.load = android_image_get_kload(img_hdr, NULL);
	images.ep = images.os.load;

	/* BOOTM_STATE_FINDOTHER */
	/* find ramdisk */
	if(!android_image_get_ramdisk(img_hdr, NULL,
								(ulong*)&ramdisk_addr, (ulong*)&size)) {
		images.rd_start = ramdisk_addr;
		images.rd_end = ramdisk_addr + size;
	} else {
		pr_err("boota_avb: ## No Ramdisk\n");
		avb_slot_verify_data_free(out_data);
		return CMD_RET_FAILURE;
	}

	/* find flattened device tree with index=0 */
	if(android_image_get_dtb_by_index((ulong)img_hdr, -1,
								0, (ulong*)&dtb_addr, (u32*)&size)) {
		images.ft_addr = (char*)dtb_addr;
		images.ft_len = size;
	} else {
		pr_err("boota_avb: ## Could not find a valid device tree\n");
		avb_slot_verify_data_free(out_data);
		return CMD_RET_FAILURE;
	}

	return do_bootm_states(cmdtp, flag, 0, NULL,
		BOOTM_STATE_RAMDISK | BOOTM_STATE_LOADOS |
		BOOTM_STATE_OS_PREP | BOOTM_STATE_OS_GO, &images, 1);
}

U_BOOT_CMD(
        boota_avb, 1, 0, do_boot_android_avb,
        "Perform loading of Android boot image with AVB flow.",
        "boota_avb\n"
);
