/*
 * Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All
 * Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#define ESWITCH_HACK
#include <common.h>
#include "mscc-common.h"
#include <dm.h>
#include <fdt_support.h>

#include <asm/system.h>

#define MSCC_IO_ORIGIN1_OFFSET 0x70000000
#define MSCC_IO_ORIGIN1_SIZE   0x00200000
#define MSCC_IO_ORIGIN2_OFFSET 0x71000000
#define MSCC_IO_ORIGIN2_SIZE   0x01000000
#include "mscc_eswitch.h"

#include <linux/io.h>
#include <linux/err.h>
#include <malloc.h>
#include <net.h>

#include "mscc-io.h"


static int eswitch_start(struct udevice *dev)
{
	struct eswitch_eth_dev *priv = dev_get_priv(dev);
        int rc = priv->chip->start(dev);
        priv->running = (rc == 0);
        return rc;
}

static int eswitch_write_hwaddr(struct udevice *dev)
{
        struct eswitch_eth_dev *priv = dev_get_priv(dev);
        return priv->chip->set_addr(dev);
}

static int eswitch_send(struct udevice *dev, void *packet, int length)
{
        struct eswitch_eth_dev *priv = dev_get_priv(dev);
        return priv->chip->send(dev, packet, length);
}

static int eswitch_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct eswitch_eth_dev *priv = dev_get_priv(dev);
        return priv->chip->recv(dev, flags, packetp);
}

static int eswitch_free_pkt(struct udevice *dev, uchar *packet, int length)
{
        debug("Release rxbuff = %p\n", packet);
	return 0;
}

static void eswitch_stop(struct udevice *dev)
{
	struct eswitch_eth_dev *priv = dev_get_priv(dev);
        if (priv->running) {
                priv->chip->stop(dev);
                priv->running = false;
        }
}

static int eswitch_probe(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct eswitch_eth_dev *priv = dev_get_priv(dev);
	void __iomem *iobase;
	fdt_addr_t addr;
	fdt_size_t size;

        debug("Probing driver\n");

	addr = fdtdec_get_addr_size(gd->fdt_blob, dev_of_offset(dev), "reg", &size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pdata->iobase = (phys_addr_t)addr;
	iobase = ioremap(addr, size);
	priv->mac_reg = iobase;
	priv->chip = (struct eswitch_variant *) dev_get_driver_data(dev);

        debug("Loaded driver\n");

	return 0;
}

static int eswitch_remove(struct udevice *dev)
{
	//struct eth_pdata *pdata = dev_get_platdata(dev);
	struct eswitch_eth_dev *priv = dev_get_priv(dev);

	iounmap(priv->mac_reg);

	return 0;
}

static const struct eth_ops eswitch_ops = {
	.start                  = eswitch_start,
	.write_hwaddr           = eswitch_write_hwaddr,
	.send                   = eswitch_send,
	.recv                   = eswitch_recv,
	.free_pkt               = eswitch_free_pkt,
	.stop                   = eswitch_stop,
};

static const struct udevice_id eswitch_ids[] = {
	{ .compatible = "mscc,eswitch-ocelot", .data = (ulong)&eswitch_ocelot_chip },
	{ }
};

U_BOOT_DRIVER(eth_eswitch) = {
	.name   = "eth_eswitch",
	.id     = UCLASS_ETH,
	.of_match = eswitch_ids,
	.probe  = eswitch_probe,
	.remove = eswitch_remove,
	.ops    = &eswitch_ops,
	.priv_auto_alloc_size = sizeof(struct eswitch_eth_dev),
	.platdata_auto_alloc_size = sizeof(struct eswitch_eth_pdata),
};
