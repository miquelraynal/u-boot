// SPDX-License-Identifier:  GPL-2.0+
/*
 * mtd.c
 *
 * Generic command to handle basic operations on any memory device.
 *
 * Copyright: Bootlin, 2018
 * Author: Miquèl Raynal <miquel.raynal@bootlin.com>
 */

#include <common.h>
#include <linux/mtd/mtd.h>
#include <command.h>
#include <console.h>
#include <malloc.h>
#include <mtd.h>
#include <mapmem.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>

static void mtd_dump_buf(u8 *buf, uint len)
{
	int i, j;

	for (i = 0; i < len; ) {
		printf("0x%08x:\t", i);
		for (j = 0; j < 8; j++)
			printf("%02x ", buf[i + j]);
		printf(" ");
		i += 8;
		for (j = 0; j < 8; j++)
			printf("%02x ", buf[i + j]);
		printf("\n");
		i += 8;
	}
}

static void mtd_show_device(struct mtd_info *mtd)
{
	printf("* %s", mtd->name);
	if (mtd->dev)
		printf(" [device: %s] [parent: %s] [driver: %s]",
		       mtd->dev->name, mtd->dev->parent->name,
		       mtd->dev->driver->name);

	printf("\n");
}

static int do_mtd_list(void)
{
	struct mtd_info *mtd;
	struct udevice *dev;
	int dm_idx = 0, idx = 0;

	/* Ensure all devices compliants with U-Boot driver model are probed */
	while (!uclass_find_device(UCLASS_MTD, dm_idx, &dev) && dev) {
		mtd_probe(dev);
		dm_idx++;
	}

	printf("MTD devices list (%d DM compliant):\n", dm_idx);

	mtd_for_each_device(mtd) {
		mtd_show_device(mtd);
		idx++;
	}

	if (!idx)
		printf("No MTD device found\n");

	return 0;
}

static int do_mtd_read_write(struct mtd_info *mtd, bool read, uint *waddr,
			     bool raw, bool woob, u64 from, u64 len)
{
	u32 buf_len = woob ? mtd->writesize + mtd->oobsize :
			     ROUND(len, mtd->writesize);
	u8 *buf = malloc(buf_len);
	struct mtd_oob_ops ops = {
		.mode = raw ? MTD_OPS_RAW : 0,
		.len = len,
		.ooblen = woob ? mtd->oobsize : 0,
		.datbuf = buf,
		.oobbuf = woob ? &buf[mtd->writesize] : NULL,
	};
	int ret;

	if (!buf)
		return -ENOMEM;

	memset(buf, 0xFF, buf_len);

	if (read) {
		ret = mtd_read_oob(mtd, from, &ops);
	} else {
		memcpy(buf, waddr, ops.len + ops.ooblen);
		ret = mtd_write_oob(mtd, from, &ops);
	}

	if (ret) {
		printf("Could not handle %lldB from 0x%08llx on %s, ret %d\n",
		       len, from, mtd->name, ret);
		return ret;
	}

	if (read) {
		printf("Dump %lld data bytes from 0x%08llx:\n", len, from);
		mtd_dump_buf(buf, len);

		if (woob) {
			printf("\nDump %d OOB bytes from 0x%08llx:\n",
			       mtd->oobsize, from);
			mtd_dump_buf(&buf[len], mtd->oobsize);
		}
	}

	kfree(buf);

	return 0;
}

static int do_mtd_erase(struct mtd_info *mtd, bool scrub, u64 from, u64 len)
{
	struct erase_info erase_infos = {
		.mtd = mtd,
		.addr = from,
		.len = len,
		.scrub = scrub,
	};

	return mtd_erase(mtd, &erase_infos);
}

static int do_mtd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mtd_info *mtd;
	struct udevice *dev;
	const char *cmd;
	char *part;
	int ret;

	/* All MTD commands need at least two arguments */
	if (argc < 2)
		return CMD_RET_USAGE;

	/* Parse the command name and its optional suffixes */
	cmd = argv[1];

	/* List the MTD devices if that is what the user wants */
	if (strcmp(cmd, "list") == 0)
		return do_mtd_list();

	/*
	 * The remaining commands require also at least a device ID.
	 * Check the selected device is valid. Ensure it is probed.
	 */
	if (argc < 3)
		return CMD_RET_USAGE;

	part = argv[2];
	ret = uclass_find_device_by_name(UCLASS_MTD, part, &dev);
	if (!ret && dev) {
		mtd_probe(dev);
		mtd = (struct mtd_info *)dev_get_uclass_priv(dev);
		if (!mtd) {
			printf("Could not retrieve MTD data\n");
			return -ENODEV;
		}
	} else {
		mtd = get_mtd_device_nm(part);
		if (IS_ERR_OR_NULL(mtd)) {
			printf("MTD device %s not found, ret %ld\n", part,
			       PTR_ERR(mtd));
			return 1;
		}
	}

	argc -= 3;
	argv += 3;

	/* Do the parsing */
	if (!strncmp(cmd, "read", 4) || !strncmp(cmd, "write", 5)) {
		bool read, raw, woob;
		uint *waddr = NULL;
		u64 off, len;

		read = !strncmp(cmd, "read", 4);
		raw = strstr(cmd, ".raw");
		woob = strstr(cmd, ".oob");

		if (!read) {
			if (argc < 1)
				return CMD_RET_USAGE;

			waddr = map_sysmem(simple_strtoul(argv[0], NULL, 10),
					   0);
			argc--;
			argv++;
		}

		off = argc > 0 ? simple_strtoul(argv[0], NULL, 10) : 0;
		len = argc > 1 ? simple_strtoul(argv[1], NULL, 10) :
				 mtd->writesize + (woob ? mtd->oobsize : 0);

		if ((u32)off % mtd->writesize) {
			printf("Section not page-aligned (0x%x)\n",
			       mtd->writesize);
			return -EINVAL;
		}

		if (woob && (len != (mtd->writesize + mtd->oobsize))) {
			printf("OOB operations are limited to single pages\n");
			return -EINVAL;
		}

		if ((off + len) >= mtd->size) {
			printf("Access location beyond the end of the chip\n");
			return -EINVAL;
		}

		printf("%s (from %p) %lldB at 0x%08llx [%s %s]\n",
		       read ? "read" : "write", read ? 0 : waddr, len, off,
		       raw ? "raw" : "", woob ? "oob" : "");

		ret = do_mtd_read_write(mtd, read, waddr, raw, woob, off, len);

		if (!read)
			unmap_sysmem(waddr);

	} else if (!strcmp(cmd, "erase") || !strcmp(cmd, "scrub")) {
		bool scrub = !strcmp(cmd, "scrub");
		bool full_erase = !strncmp(&cmd[5], ".chip", 4);
		u64 off, len;

		off = argc > 0 ? simple_strtoul(argv[0], NULL, 10) : 0;
		len = argc > 1 ? simple_strtoul(argv[1], NULL, 10) :
				 mtd->erasesize;
		if (full_erase) {
			off = 0;
			len = mtd->size;
		}

		if ((u32)off % mtd->erasesize) {
			printf("Section not erase-block-aligned (0x%x)\n",
			       mtd->erasesize);
			return -EINVAL;
		}

		if ((u32)len % mtd->erasesize) {
			printf("Size not aligned with an erase block (%dB)\n",
			       mtd->erasesize);
			return -EINVAL;
		}

		if ((off + len) >= mtd->size) {
			printf("Cannot read beyond end of chip\n");
			return -EINVAL;
		}

		ret = do_mtd_erase(mtd, scrub, off, len);
	} else {
		return CMD_RET_USAGE;
	}

	return ret;
}

static char mtd_help_text[] =
#ifdef CONFIG_SYS_LONGHELP
	"- generic operations on memory technology devices\n\n"
	"mtd list\n"
	"mtd read[.raw][.oob] <name> [<off> [<size>]]\n"
	"mtd write[.raw][.oob] <name> <addr> [<off> [<size>]]\n"
	"mtd erase[.chip] <name> [<off> [<size>]]\n"
	"mtd scrub[.chip] <name> [<off> [<size>]]\n"
#endif
	"";

U_BOOT_CMD(mtd, 10, 1, do_mtd, "MTD utils", mtd_help_text);
