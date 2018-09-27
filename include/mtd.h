/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2015 Thomas Chou <thomas@wytron.com.tw>
 */

#ifndef _MTD_H_
#define _MTD_H_

#include <linux/mtd/mtd.h>

struct udevice;

#if defined(CONFIG_DM)
/*
 * Get mtd_info structure of the dev, which is stored as uclass private.
 *
 * @dev: The MTD device
 * @return: pointer to mtd_info, NULL on error
 */
static inline struct mtd_info *mtd_get_info(struct udevice *dev)
{
	return dev_get_uclass_priv(dev);
}

int mtd_probe(struct udevice *dev);
#else
static inline struct mtd_info *mtd_get_info(struct udevice *dev)
{
	return NULL;
}

static inline int mtd_probe(struct udevice *dev)
{
	return 0;
}
#endif

int mtd_probe_devices(void);

#endif	/* _MTD_H_ */
