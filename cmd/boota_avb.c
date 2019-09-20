/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <common.h>
#include <malloc.h>
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
static int do_boot_android_avb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	AvbSlotVerifyResult slot_result;
	AvbSlotVerifyData *out_data;
	struct andr_img_hdr *img_hdr = NULL;
	struct andr_img_hdr_v2 *img_hdr_v2 = NULL;
	bool unlocked = false;
	char *cmdline = NULL;
	unsigned long kernel_addr = 0, dtb_addr = 0, size = 0;

	printf("avb_flow: Android Verified Boot 2.0 - AVB version %s\n", avb_version_string());

	if (avb_ops.read_is_device_unlocked(&avb_ops, &unlocked) != AVB_IO_RESULT_OK) {
		printf("avb_flow: Can't determine device lock state.\n");
		return CMD_RET_FAILURE;
	}

	slot_result = avb_slot_verify(&avb_ops, avb_partitions, "",
				unlocked, AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE, &out_data);

	/* Update bootargs */
	if((cmdline = env_get("bootargs")) == NULL)
		cmdline = "";

	cmdline = avb_strdupv(cmdline, " ", out_data->cmdline, NULL);
	env_set("bootargs", cmdline);

	/* */
	switch (slot_result)
	{
	case AVB_SLOT_VERIFY_RESULT_OK:
		printf("avb_flow: Verification passed successfully\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
		printf("avb_flow: Verification failed\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
		printf("avb_flow: I/O error occurred during verification\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
		printf("avb_flow: OOM error occurred during verification\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
		printf("avb_flow: Corrupted dm-verity metadata detected\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
		printf("avb_flow: Unsupported version avbtool was used\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
		printf("avb_flow: Checking rollback index failed\n");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
		printf("avb_flow: Public key was rejected\n");
		break;
	default:
		printf("avb_flow: Unknown error occurred\n");
	}

	if (out_data->loaded_partitions > 0) {
		AvbPartitionData* avb_part = &out_data->loaded_partitions[0];

		img_hdr = (struct andr_img_hdr *)avb_part->data;
		img_hdr_v2 = (struct andr_img_hdr_v2 *)img_hdr;

		printf("avb_flow: Loaded '%s' partition, size %luKiB\n",
			avb_part->partition_name, avb_part->data_size/1024);
	}

	if (img_hdr == NULL) {
		printf("avb_flow: No loaded partition(s)\n");
		return CMD_RET_FAILURE;
	}

	if (android_image_check_header(img_hdr) != 0) {
		pr_err("%s: ** Invalid Android Image header **\n", __func__);
		return CMD_RET_FAILURE;
	}

	/* */
	android_print_contents(img_hdr);

	/* */
	android_image_get_kernel(img_hdr, 0, &kernel_addr, &size);
	pr_info("Copy Kernel from 0x%08lx to 0x%08lx ...\n", kernel_addr, (ulong)img_hdr->kernel_addr);
	memcpy((void*)((ulong)img_hdr->kernel_addr), (ulong*)kernel_addr, size);

	android_image_get_dtb(img_hdr, &dtb_addr, &size);
	pr_info("Copy DTB from 0x%08lx to 0x%08lx ...\n", dtb_addr, (ulong)img_hdr_v2->dtb_addr);
	memcpy((ulong*)img_hdr_v2->dtb_addr, (ulong*)dtb_addr, size);

//  android_image_get_ramdisk(img_hdr, &ramdisk_addr, &size);

	pr_info("booti 0x%08lx - 0x%08lx\n",
		(ulong)img_hdr->kernel_addr, (ulong)img_hdr_v2->dtb_addr);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
        boota_avb, 1, 0, do_boot_android_avb,
        "Perform loading of Android boot image with AVB flow.",
        "boota_avb; booti <kernel_addr> <initrd_addr> <fdt_addr>;\n"
);
