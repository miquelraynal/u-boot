/*
 * Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All
 * Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
//#define ESWITCH_HACK
#include <common.h>
#include <net.h>
#define __ASM_MACH_MSCC_IOREMAP_H

/*
 * Allow physical addresses to be fixed up to help peripherals located
 * outside the low 32-bit range -- generic pass-through version.
 */
static inline phys_addr_t fixup_bigphys_addr(phys_addr_t phys_addr,
                                             phys_addr_t size)
{
	return phys_addr;
}

static inline int is_vcoreiii_internal_registers(phys_addr_t offset)
{
#if defined(CONFIG_ARCH_MSCC)
        if ((offset >= MSCC_IO_ORIGIN1_OFFSET && offset < (MSCC_IO_ORIGIN1_OFFSET+MSCC_IO_ORIGIN1_SIZE)) ||
            (offset >= MSCC_IO_ORIGIN2_OFFSET && offset < (MSCC_IO_ORIGIN2_OFFSET+MSCC_IO_ORIGIN2_SIZE)))
                return 1;
#endif

	return 0;
}

static inline void __iomem *plat_ioremap(phys_addr_t offset, unsigned long size,
                                         unsigned long flags)
{
	if (is_vcoreiii_internal_registers(offset))
		return (void __iomem *)offset;

	return NULL;
}

static inline int plat_iounmap(const volatile void __iomem *addr)
{
	return is_vcoreiii_internal_registers((unsigned long)addr);
}

#define _page_cachable_default	_CACHE_CACHABLE_NONCOHERENT

#include <linux/io.h>
#undef ESWITCH_HACK

#define msleep(a) udelay(a * 1000)

struct eswitch_variant {
        u32 ifh_len;            // In dwords
        int  (*start)(struct udevice *dev);
        void (*stop)(struct udevice *dev);
        int  (*set_addr)(struct udevice *dev);
        int  (*send)(struct udevice *dev, void *packet, int length);
        int  (*recv)(struct udevice *dev, int flags, uchar **packetp);
};

extern struct eswitch_variant eswitch_ocelot_chip;

struct eswitch_eth_dev {
	void __iomem *mac_reg;
        struct eswitch_variant *chip;
        bool running;
};

struct eswitch_eth_pdata {
	struct eth_pdata eth_pdata;
};

/** \brief VLAN Identifier */
typedef u16 vtss_vid_t; /* 0-4095 */

/** \brief MAC Address */
typedef struct
{
        u8 addr[6];   /**< Network byte order */
} vtss_mac_t;

/** \brief MAC Address in specific VLAN */
typedef struct
{
        vtss_vid_t  vid;   /**< VLAN ID */
        vtss_mac_t  mac;   /**< MAC address */
} vtss_vid_mac_t;

/** \brief MAC address entry */
typedef struct
{
        vtss_vid_mac_t vid_mac;                            /**< VLAN ID and MAC addr */
        bool           copy_to_cpu;                        /**< CPU copy flag */
        bool           locked;                             /**< Locked/static flag */
        bool           aged;                               /**< Age flag */
        u8             cpu_queue;                          /**< CPU queue */
} vtss_mac_table_entry_t;

static inline void vtss_mach_macl_get(const vtss_vid_mac_t *vid_mac, u32 *mach, u32 *macl)
{
        *mach = ((vid_mac->vid<<16) | (vid_mac->mac.addr[0]<<8) | vid_mac->mac.addr[1]);
        *macl = ((vid_mac->mac.addr[2]<<24) | (vid_mac->mac.addr[3]<<16) |
                 (vid_mac->mac.addr[4]<<8) | vid_mac->mac.addr[5]);
}

/* Commands for Mac Table Command register */
#define MAC_CMD_IDLE        0  /* Idle */
#define MAC_CMD_LEARN       1  /* Insert (Learn) 1 entry */
#define MAC_CMD_FORGET      2  /* Delete (Forget) 1 entry */
#define MAC_CMD_TABLE_AGE   3  /* Age entire table */
#define MAC_CMD_GET_NEXT    4  /* Get next entry */
#define MAC_CMD_TABLE_CLEAR 5  /* Delete all entries in table */
#define MAC_CMD_READ        6  /* Read entry at Mac Table Index */
#define MAC_CMD_WRITE       7  /* Write entry at Mac Table Index */

#define MAC_TYPE_NORMAL  0 /* Normal entry */
#define MAC_TYPE_LOCKED  1 /* Locked entry */
#define MAC_TYPE_IPV4_MC 2 /* IPv4 MC entry */
#define MAC_TYPE_IPV6_MC 3 /* IPv6 MC entry */

#define XTR_EOF_0     ntohl(0x80000000u)
#define XTR_EOF_1     ntohl(0x80000001u)
#define XTR_EOF_2     ntohl(0x80000002u)
#define XTR_EOF_3     ntohl(0x80000003u)
#define XTR_PRUNED    ntohl(0x80000004u)
#define XTR_ABORT     ntohl(0x80000005u)
#define XTR_ESCAPE    ntohl(0x80000006u)
#define XTR_NOT_READY ntohl(0x80000007u)

#define VTSS_VID_NULL 0

#define MIN_PORT 0
#define MAX_PORT 4
#define CPU_PORT 11

static inline bool timeout(u64 deadline)
{
        return (get_ticks() >= deadline);
}
