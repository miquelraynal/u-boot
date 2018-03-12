/*
 * Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All
 * Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

//#define DEBUG
//#define DEBUG_DUMP
#define noise(...) // debug(__VA_ARGS__)

#define ESWITCH_HACK
#include "mscc-common.h"
#include "vtss_ocelot_regs.h"
#define MSCC_IO_ORIGIN1_OFFSET 0x70000000
#define MSCC_IO_ORIGIN1_SIZE   0x00200000
#define MSCC_IO_ORIGIN2_OFFSET 0x71000000
#define MSCC_IO_ORIGIN2_SIZE   0x01000000
#include "mscc_eswitch.h"

#include "mscc-io.h"

#define BASE_DEVCPU_GCB 0x71070000

#define REG_OFFSET(t, o) ((volatile unsigned long *)(t + (o)))

#define REG_GCB(x) REG_OFFSET(BASE_DEVCPU_GCB, x)
#include <mach/ocelot/ocelot_devcpu_gcb.h>

#define PERF_GPIO_ALT(x)                                  (0x54 + 4 * (x))
#include <dm/device.h>

static inline void vcoreiii_gpio_set_alternate(int gpio, int mode)
{
    u32 mask = BIT(gpio);
    u32 val0, val1;

    val0 = readl(REG_GCB(PERF_GPIO_ALT(0)));
    val1 = readl(REG_GCB(PERF_GPIO_ALT(1)));

    if (mode == 1) {
        writel(val0 | mask, REG_GCB(PERF_GPIO_ALT(0)));
        writel(val1 & ~mask, REG_GCB(PERF_GPIO_ALT(1)));
    } else if (mode == 2) {
        writel(val0 & ~mask, REG_GCB(PERF_GPIO_ALT(0)));
        writel(val1 | mask, REG_GCB(PERF_GPIO_ALT(1)));
    } else if (mode == 3) {
        writel(val0 | mask, REG_GCB(PERF_GPIO_ALT(0)));
        writel(val1 | mask, REG_GCB(PERF_GPIO_ALT(1)));
    } else {
        writel(val0 & ~mask, REG_GCB(PERF_GPIO_ALT(0)));
        writel(val1 & ~mask, REG_GCB(PERF_GPIO_ALT(1)));
    }
}

#define MAC_VID        1
#define PGID_BROADCAST 13
#define PGID_UNICAST   14
#define PGID_SRC       80

#define PORTCOUNT      4

#define DEV_ADDR(_p_) (VTSS_TO_DEV_0 + (_p_ * (VTSS_TO_DEV_1 - VTSS_TO_DEV_0)))

#if defined(DEBUG_DUMP)
static void print_packet(u8 *buf, int length)
{
	int i;
	int remainder;
	int lines;

	printf ("Packet of length %d \n", length);

	lines = length / 16;
	remainder = length % 16;

	for (i = 0; i < lines; i++) {
		int cur;

		for (cur = 0; cur < 8; cur++) {
			u8 a, b;

			a = *(buf++);
			b = *(buf++);
			printf ("%02x%02x ", a, b);
		}
		printf ("\n");
	}
	for (i = 0; i < remainder / 2; i++) {
		u8 a, b;

		a = *(buf++);
		b = *(buf++);
		printf ("%02x%02x ", a, b);
	}
	printf ("\n");
}
#endif /* 0 */

static u32 rx_word(u32 qno)
{
        u32 value = vcoreiii_reg_rd(VTSS_DEVCPU_QS_XTR_XTR_RD(qno));
        //debug("Read(0x%08x)\n", value);
        return value;
}

static void tx_word(u32 qno, u32 value)
{
        //debug("Write(0x%08x)\n", value);
        vcoreiii_reg_wr(VTSS_DEVCPU_QS_INJ_INJ_WR(qno), value);
}

static bool mac_table_idle(void)
{
        u32 cmd;
        u64 expire;

        /* Wait until MAC_CMD_IDLE */
        expire = get_ticks() + (get_tbclk() / 1000);
        do {
                if (timeout(expire)) {
                        printf("MAC address table command timeout\n");
                        return false;
                }
                cmd = vcoreiii_reg_rd(VTSS_ANA_ANA_TABLES_MACACCESS);
        } while (VTSS_X_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(cmd) != MAC_CMD_IDLE);
        //debug("MAC address cmd done\n");
        return true;
}

static bool mac_table_result(vtss_mac_table_entry_t *entry, int *pgid)
{
        u32 value, mach, macl, idx, type;
        u8 *mac = &entry->vid_mac.mac.addr[0];

        value = vcoreiii_reg_rd(VTSS_ANA_ANA_TABLES_MACACCESS);

        /* Check if entry is valid */
        if (!(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID(1))) {
                //debug("not valid\n");
                return false;
        }

        type               = VTSS_X_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(value);
        idx                = VTSS_X_ANA_ANA_TABLES_MACACCESS_DEST_IDX(value);
        entry->aged        = !!(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_AGED_FLAG(1));
        entry->copy_to_cpu = !!(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_CPU_COPY(1));
        entry->locked      = !(type == MAC_TYPE_NORMAL);

        mach = vcoreiii_reg_rd(VTSS_ANA_ANA_TABLES_MACHDATA);
        macl = vcoreiii_reg_rd(VTSS_ANA_ANA_TABLES_MACLDATA);

        if (type == MAC_TYPE_IPV4_MC || type == MAC_TYPE_IPV6_MC) {
                /* IPv4/IPv6 MC entry, decode port mask from entry */
                *pgid = -1;
        } else {
                *pgid = idx;
        }
        entry->vid_mac.vid = ((mach>>16) & 0xFFF);
        mac[0] = ((mach>>8)  & 0xff);
        mac[1] = ((mach>>0)  & 0xff);
        mac[2] = ((macl>>24) & 0xff);
        mac[3] = ((macl>>16) & 0xff);
        mac[4] = ((macl>>8)  & 0xff);
        mac[5] = ((macl>>0)  & 0xff);
        //debug("found 0x%08x%08x\n", mach, macl);

        return true;
}

static bool ocelot_mac_table_add(const vtss_mac_table_entry_t *entry, int pgid)
{
        u32  value, mach, macl, idx = 0, aged = 0, fwd_kill = 0, type;
        bool copy_to_cpu = 0;

        vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
        debug("address 0x%08x%08x\n", mach, macl);

        /* Set FWD_KILL to make the switch discard frames in SMAC lookup */
        fwd_kill = false;
        idx = pgid;
        type = MAC_TYPE_LOCKED;

        /* Insert/learn new entry into the MAC table  */
        vcoreiii_reg_wr(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
        vcoreiii_reg_wr(VTSS_ANA_ANA_TABLES_MACLDATA, macl);

        value = ((copy_to_cpu        ? VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_CPU_COPY(1) : 0) |
                 (fwd_kill           ? VTSS_F_ANA_ANA_TABLES_MACACCESS_SRC_KILL(1)     : 0) |
                 (aged               ? VTSS_F_ANA_ANA_TABLES_MACACCESS_AGED_FLAG(1)    : 0) |
                 VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID(1)                                   |
                 VTSS_F_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(type)                           |
                 VTSS_F_ANA_ANA_TABLES_MACACCESS_DEST_IDX(idx)                              |
                 VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_LEARN));
        vcoreiii_reg_wr(VTSS_ANA_ANA_TABLES_MACACCESS, value);

        /* Wait until MAC operation is finished */
        return mac_table_idle();
}


static bool ocelot_mac_table_get_next(vtss_mac_table_entry_t *entry, int *pgid)
{
        u32 mach, macl;

        vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
        //debug("address 0x%08x%08x\n", mach, macl);

        vcoreiii_reg_wr(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
        vcoreiii_reg_wr(VTSS_ANA_ANA_TABLES_MACLDATA, macl);

        /* Do a get next lookup */
        vcoreiii_reg_wr(VTSS_ANA_ANA_TABLES_MACACCESS,
                        VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_GET_NEXT));

        return mac_table_idle() &&
                mac_table_result(entry, pgid);
}

static int mscc_generic_rx_packet(u32 qno, u32 *packet)
{
        bool eof_flag, abort_flag, pruned_flag;
        int bytect = 0;
#if defined(DEBUG_DUMP)
        void *buf = packet;
#endif

        eof_flag = abort_flag = pruned_flag = 0;

        debug("Receive...");

        while(!eof_flag) {
                u32 rcv = rx_word(qno);
#if defined(DEBUG_DUMP)
                noise("off(%4d): 0x%08x\n", bytect, rcv);
#endif
                switch(rcv) {
                case XTR_NOT_READY:
                        noise("%d: NOT_READY...?\n", bytect);
                        break;              /* Try again... */
                case XTR_ABORT:
                        *packet = rx_word(qno); /* Unused */
                        abort_flag = 1;
                        eof_flag = true;
                        noise("XTR_ABORT\n");
                        break;
                case XTR_EOF_0:
                case XTR_EOF_1:
                case XTR_EOF_2:
                case XTR_EOF_3:
                        bytect += (4 - (rcv & 3));
                        *packet = rx_word(qno);
                        eof_flag = true;
                        noise("XTR_EOF_%d\n", rcv&3);
                        break;
                case XTR_PRUNED:
                        eof_flag = pruned_flag = true; /* But get the last 4 bytes as well */
                        noise("XTR_PRUNED\n");
                        /* FALLTHROUGH */
                case XTR_ESCAPE:
                        *packet = rx_word(qno);
                        bytect += 4;
                        packet++;
                        noise("ESCAPED\n");
                        break;
                default:
                        *packet = rcv;
                        bytect += 4;
                        packet++;
                }
        }

        if(abort_flag ||
           pruned_flag ||
           !eof_flag) {
                debug("Discarded frame: abort:%d pruned:%d eof:%d\n", abort_flag, pruned_flag, eof_flag);
                return -EAGAIN;
        }

#if defined(DEBUG_DUMP)
        print_packet(buf, bytect);
#endif
        debug("got %d bytes\n", bytect);

        return bytect;
}

static int ocelot_recv_packet(struct udevice *dev, u32 qno, uchar *rxbuf)
{
        u32 qstatus = vcoreiii_reg_rd(VTSS_DEVCPU_QS_XTR_XTR_DATA_PRESENT);
        if (qstatus & VTSS_F_DEVCPU_QS_XTR_XTR_DATA_PRESENT_DATA_PRESENT(BIT(qno))) {
                /* IFH = 4 * 32 bits */
                (void) rx_word(qno);
                (void) rx_word(qno);
                (void) rx_word(qno);
                (void) rx_word(qno);
                return mscc_generic_rx_packet(qno, (u32*) rxbuf);
        }
        return -EAGAIN;
}

static int ocelot_recv(struct udevice *dev, int flags, uchar **packetp)
{
        u32 rxix = 0;
        uchar *rxbuf = net_rx_packets[rxix];
        int length;
        // Only using queue 0
        if ((length = ocelot_recv_packet(dev, 0, rxbuf)) > 0) {
                *packetp = rxbuf;
                return length;
        }
	return -EAGAIN;
}

static void ocelot_dev_init(u32 port)
{
    /* Get device address */
    u32 dev_addr = DEV_ADDR(port);

    /* Enable PCS */
    vcoreiii_reg_wr(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(dev_addr),
                    VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA(1));

    /* Disable Signal Detect */
    vcoreiii_reg_wr(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG(dev_addr), 0);

    /* Enable MAC RX and TX */
    vcoreiii_reg_wr(VTSS_DEV_MAC_CFG_STATUS_MAC_ENA_CFG(dev_addr),
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA(1) |
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_TX_ENA(1));

    /* Clear sgmii_mode_ena */
    vcoreiii_reg_wr(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG(dev_addr), 0);

    /* Clear sw_resolve_ena and set adv_ability to something meaningful just in case */
    vcoreiii_reg_wr(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(dev_addr),
                    VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ADV_ABILITY(0x0020));

    /* Set RX and TX IFG */
    vcoreiii_reg_wr(VTSS_DEV_MAC_CFG_STATUS_MAC_IFG_CFG(dev_addr),
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_TX_IFG(5) |
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG1(5) |
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG2(1));

    /* Set link speed and release all resets */
    vcoreiii_reg_wr(VTSS_DEV_PORT_MODE_CLOCK_CFG(dev_addr),
                    VTSS_F_DEV_PORT_MODE_CLOCK_CFG_LINK_SPEED(1));

    /* Make VLAN aware for CPU traffic */
    vcoreiii_reg_wr(VTSS_ANA_PORT_VLAN_CFG(port),
                    VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA(1) |
                    VTSS_F_ANA_PORT_VLAN_CFG_VLAN_POP_CNT(1) |
                    VTSS_F_ANA_PORT_VLAN_CFG_VLAN_VID(MAC_VID));

    /* Enable the port in the core */
    vcoreiii_reg_wr_msk(VTSS_QSYS_SYSTEM_SWITCH_PORT_MODE(port),
                        VTSS_M_QSYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA,
                        VTSS_F_QSYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA(1));
}

static void mscc_ocelot_phy_init(void)
{
        u64 expire;
        u32 cmd;
        u8 phy_mask = VTSS_BITMASK(PORTCOUNT);

        /* Release common reset */
        vcoreiii_reg_wr_msk(VTSS_DEVCPU_GCB_PHY_PHY_CFG,
                            VTSS_M_DEVCPU_GCB_PHY_PHY_CFG_PHY_COMMON_RESET,
                            VTSS_F_DEVCPU_GCB_PHY_PHY_CFG_PHY_COMMON_RESET(1));

        /* Wait until SUPERVISOR_COMPLETE */
        expire = get_ticks() + get_tbclk() * 2;
        do {
                if (timeout(expire)) {
                        printf("%% Timeout when calling phy_init()\n");
                        return;
                }
                cmd = vcoreiii_reg_rd(VTSS_DEVCPU_GCB_PHY_PHY_STAT);
        } while (!VTSS_X_DEVCPU_GCB_PHY_PHY_STAT_SUPERVISOR_COMPLETE(cmd));

        msleep(20);
        /* Release individual phy resets and enable phy interfaces */
        vcoreiii_reg_wr_msk(VTSS_DEVCPU_GCB_PHY_PHY_CFG,
                            VTSS_M_DEVCPU_GCB_PHY_PHY_CFG_PHY_RESET |
                            VTSS_M_DEVCPU_GCB_PHY_PHY_CFG_PHY_ENA,
                            VTSS_F_DEVCPU_GCB_PHY_PHY_CFG_PHY_RESET(phy_mask) |
                            VTSS_F_DEVCPU_GCB_PHY_PHY_CFG_PHY_ENA(phy_mask));

        cmd = vcoreiii_reg_rd(VTSS_DEVCPU_GCB_PHY_PHY_CFG);
        if (cmd != (VTSS_M_DEVCPU_GCB_PHY_PHY_CFG_PHY_RESET |
                    VTSS_M_DEVCPU_GCB_PHY_PHY_CFG_PHY_ENA |
                    VTSS_M_DEVCPU_GCB_PHY_PHY_CFG_PHY_COMMON_RESET)) {
                printf("Internal PHYs reset failure: cmd = %0x\n", cmd);
        }
}

static void ocelot_cpu_capture_setup(void)
{
        int i;

        // map the 8 CPU extraction queues to CPU port 11.
        vcoreiii_reg_wr(VTSS_QSYS_SYSTEM_CPU_GROUP_MAP, 0);

        for (i = 0; i <= 1; i++) {
                // Do byte-swap and expect status after last data word
                // Extraction: Mode: 0b0100(manual extraction) | POS:0b0010 | Byte_swap: 0b0001 = 0x7
                vcoreiii_reg_wr(VTSS_DEVCPU_QS_XTR_XTR_GRP_CFG(i),
                                VTSS_F_DEVCPU_QS_XTR_XTR_GRP_CFG_MODE(1) |
                                VTSS_F_DEVCPU_QS_XTR_XTR_GRP_CFG_STATUS_WORD_POS(0) |
                                VTSS_F_DEVCPU_QS_XTR_XTR_GRP_CFG_BYTE_SWAP(1));

                // Injection: Mode: 0b0100(manual extraction) | Byte_swap: 0b0001 = 0x5
                vcoreiii_reg_wr(VTSS_DEVCPU_QS_INJ_INJ_GRP_CFG(i),
                                VTSS_F_DEVCPU_QS_INJ_INJ_GRP_CFG_MODE(1) |
                                VTSS_F_DEVCPU_QS_INJ_INJ_GRP_CFG_BYTE_SWAP(1));
        }

        for (i = 0; i <= 1; i++) {
                /* Enable IFH insertion/parsing on CPU ports */
                vcoreiii_reg_wr_msk(VTSS_SYS_SYSTEM_PORT_MODE(CPU_PORT + i),
                                    VTSS_M_SYS_SYSTEM_PORT_MODE_INCL_XTR_HDR |
                                    VTSS_M_SYS_SYSTEM_PORT_MODE_INCL_INJ_HDR,
                                    VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_INJ_HDR(1) |
                                    VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_XTR_HDR(1));
        }

        /* Setup the CPU port as VLAN aware to support switching frames based on tags */
        vcoreiii_reg_wr(VTSS_ANA_PORT_VLAN_CFG(CPU_PORT),
                        VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA(1) |
                        VTSS_F_ANA_PORT_VLAN_CFG_VLAN_POP_CNT(1) |
                        VTSS_F_ANA_PORT_VLAN_CFG_VLAN_VID(MAC_VID));

        /* Disable learning (only RECV_ENA must be set) */
        vcoreiii_reg_wr(VTSS_ANA_PORT_PORT_CFG(CPU_PORT),
                        VTSS_F_ANA_PORT_PORT_CFG_RECV_ENA(1));

        // Enable switching to/from cpu port
        vcoreiii_reg_wr_msk(VTSS_QSYS_SYSTEM_SWITCH_PORT_MODE(CPU_PORT),
                            VTSS_M_QSYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA,
                            VTSS_F_QSYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA(1));

        // No pause on CPU port - not needed (off by default)
        vcoreiii_reg_wr_msk(VTSS_SYS_PAUSE_CFG_PAUSE_CFG(CPU_PORT),
                            VTSS_M_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_ENA,
                            VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_ENA(0));

        // Enable Tail Drop on CPU port.
        // vcoreiii_reg_wr(VTSS_SYS_PAUSE_CFG_ATOP(CPU_PORT), 0x0UL);

        // When enabled for a port, frames to the port are discarded, even when the
        // ingress port is enabled for flow control.
        // vcoreiii_reg_wr_msk(VTSS_QSYS_DROP_CFG_EGR_DROP_MODE, 1UL<<CPU_PORT, 1UL<<CPU_PORT);
        vcoreiii_reg_wr_msk(VTSS_QSYS_SYSTEM_EGR_NO_SHARING, BIT(CPU_PORT), BIT(CPU_PORT));
}

static int ocelot_set_addr(struct udevice *dev)
{
	return 0;
}

static int ocelot_switch_init(void)
{
        u64 expire;
        u32 reg;

        debug("Init SwC memories\n");

        // Reset switch & memories
        vcoreiii_reg_wr(VTSS_SYS_SYSTEM_RESET_CFG,
                        VTSS_M_SYS_SYSTEM_RESET_CFG_MEM_ENA |
                        VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT(1));

        // Wait to complete
        expire = get_ticks() + get_tbclk() * 2;
        do {
                reg = vcoreiii_reg_rd(VTSS_SYS_SYSTEM_RESET_CFG);
                if (timeout(expire)) {
                        printf("Timeout in memory reset, reg = 0x%08x\n", reg);
                        return -EIO;
                }
        } while (VTSS_X_SYS_SYSTEM_RESET_CFG_MEM_INIT(reg) != 0);

        debug("SwC init done - reg = 0x%08x\n", reg);

        // Enable switch core
        vcoreiii_reg_wr_msk(VTSS_SYS_SYSTEM_RESET_CFG,
                            VTSS_M_SYS_SYSTEM_RESET_CFG_CORE_ENA,
                            VTSS_F_SYS_SYSTEM_RESET_CFG_CORE_ENA(1));

        return 0;
}

static void ocelot_switch_flush(void)
{
        /* All Queues flush */
        vcoreiii_reg_set(VTSS_DEVCPU_QS_XTR_XTR_FLUSH, VTSS_M_DEVCPU_QS_XTR_XTR_FLUSH_FLUSH);
        /* Allow to drain */
        msleep(1);
        /* All Queues normal */
        vcoreiii_reg_clr(VTSS_DEVCPU_QS_XTR_XTR_FLUSH, VTSS_M_DEVCPU_QS_XTR_XTR_FLUSH_FLUSH);
}

static int ocelot_initialize(struct udevice *dev)
{
        int i;

        // Initialize switch memories, enable core
        if ((i = ocelot_switch_init()))
                return i;

        // Disable port-to-port by switching
        // Put fron ports in "port isolation modes" - i.e.  they cant
        // send to other ports - via the PGID sorce masks.
        for (i = MIN_PORT; i < MAX_PORT; i++) {
                vcoreiii_reg_wr(VTSS_ANA_PGID_PGID(PGID_SRC+i),
                                VTSS_F_ANA_PGID_PGID_PGID(0));
        }

        // Flush queues
        ocelot_switch_flush();

        /* Setup frame ageing - "2 sec" */
        // The unit is 6.5us on Ocelot
        vcoreiii_reg_wr(VTSS_SYS_SYSTEM_FRM_AGING,
                        VTSS_F_SYS_SYSTEM_FRM_AGING_AGE_TX_ENA(1) |
                        VTSS_F_SYS_SYSTEM_FRM_AGING_MAX_AGE(20000000 / 65));

        for (i = MIN_PORT; i < MAX_PORT; i++) {
                ocelot_dev_init(i);
        }

        ocelot_cpu_capture_setup();

        debug("Ports enabled\n");

        return 0;
}

static int ocelot_start(struct udevice *dev)
{
        struct eth_pdata *pdata = dev_get_platdata(dev);

        int rc = 0;

        rc = ocelot_initialize(dev);

        mscc_ocelot_phy_init();

        // Set MAC address tables entries for CPU redirection
        vtss_mac_table_entry_t mac;
        // Broadcast
        memset(&mac, 0, sizeof(mac));
        memset(mac.vid_mac.mac.addr, 0xff, sizeof(mac.vid_mac.mac));
        mac.vid_mac.vid = MAC_VID;
        ocelot_mac_table_add(&mac, PGID_BROADCAST);
        vcoreiii_reg_wr(VTSS_ANA_PGID_PGID(PGID_BROADCAST),
                        VTSS_F_ANA_PGID_PGID_PGID(BIT(CPU_PORT)|VTSS_BITMASK(PORTCOUNT)) |
                        VTSS_F_ANA_PGID_PGID_CPUQ_DST_PGID(0));
        // Unicast
        memset(&mac, 0, sizeof(mac));
        memcpy(mac.vid_mac.mac.addr, pdata->enetaddr, sizeof(mac.vid_mac.mac));
        mac.vid_mac.vid = MAC_VID;
        ocelot_mac_table_add(&mac, PGID_UNICAST);
        vcoreiii_reg_wr(VTSS_ANA_PGID_PGID(PGID_UNICAST),
                        VTSS_F_ANA_PGID_PGID_PGID(BIT(CPU_PORT)) |
                        VTSS_F_ANA_PGID_PGID_CPUQ_DST_PGID(0));

        return rc;
}

static int ocelot_switch_reset(void)
{
        u32 reg;

        debug("applying SwC reset\n");

        vcoreiii_reg_wr(VTSS_ICPU_CFG_CPU_SYSTEM_CTRL_RESET,
                        VTSS_F_ICPU_CFG_CPU_SYSTEM_CTRL_RESET_CORE_RST_PROTECT(1));
        vcoreiii_reg_wr(VTSS_DEVCPU_GCB_CHIP_REGS_SOFT_RST,
                        VTSS_F_DEVCPU_GCB_CHIP_REGS_SOFT_RST_SOFT_CHIP_RST(1));
        while ((reg = vcoreiii_reg_rd(VTSS_DEVCPU_GCB_CHIP_REGS_SOFT_RST)) &
               VTSS_M_DEVCPU_GCB_CHIP_REGS_SOFT_RST_SOFT_CHIP_RST) {
        }

        debug("SwC reset done - reg = 0x%08x\n", reg);

        return 0;
}

static void ocelot_stop(struct udevice *dev)
{
        debug("stop network\n");

        // Do SwC reset
        vcoreiii_gpio_set_alternate(19, 2);    // Nasty workaround to avoid GPIO19 (DDR!) being reset
        ocelot_switch_reset();
        // Reset GPIO19 mode back as regular GPIO, output, high (DDR not reset)
        // (Order is important)
        vcoreiii_reg_set(VTSS_DEVCPU_GCB_GPIO_GPIO_OE, BIT(19));
        vcoreiii_reg_wr(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_SET, BIT(19));
        vcoreiii_gpio_set_alternate(19, 0);
}

static void ocelot_send_ifh(u32 qno)
{
        u32 ifh0 = 0, ifh1 = 0, ifh2 = 0, ifh3 = 0;
        tx_word(qno, ifh0);
        tx_word(qno, ifh1);
        tx_word(qno, ifh2);
        tx_word(qno, ifh3);
}

static void ocelot_send_data(u32 qno, u32 *bufptr, int length, int vid)
{
        u32 buflen = length, count, w, residual, val;

        count = buflen / 4;
        residual  = buflen % 4;

        w = 0;
        while (count) {
                if (w == 3 && vid != VTSS_VID_NULL) {
                        /* Insert C-tag */
                        val = htonl((0x8100 << 16) | vid);
                        tx_word(qno, val);
                        w++;
                }
                val = *bufptr;
                tx_word(qno, val);
                bufptr++;
                count--;
                w++;
        }

        if (residual) {
                val = *bufptr;
                tx_word(qno, val);
                w++;
        }

        /* Add padding */
        while (w < 15 /*(60/4)*/ ) {
                tx_word(qno, 0);
                w++;
        }

        /* Indicate EOF and valid bytes in last word */
        vcoreiii_reg_wr(VTSS_DEVCPU_QS_INJ_INJ_CTRL(qno),
                        VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_VLD_BYTES(length < 60 ? 0 : residual) |
                        VTSS_M_DEVCPU_QS_INJ_INJ_CTRL_EOF);

        /* Add dummy CRC */
        tx_word(qno, 0);
}

static int ocelot_send(struct udevice *dev, void *packet, int length)
{

        u32 qno = 0;

        debug("Send %d bytes, buffer = %p\n", length, packet);
        vcoreiii_reg_wr(VTSS_DEVCPU_QS_INJ_INJ_CTRL(qno),
                        VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_GAP_SIZE(1) |
                        VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_SOF(1));
        ocelot_send_ifh(qno);
        ocelot_send_data(qno, packet, length, MAC_VID);

	return 0;
}

struct eswitch_variant eswitch_ocelot_chip = {
        .ifh_len = 4,                   // dwords
        .start = ocelot_start,
        .stop = ocelot_stop,
        .set_addr = ocelot_set_addr,
        .send = ocelot_send,
        .recv = ocelot_recv,
};

#define FMT_r "%-20s: 0x%08x\n"
#define FMT_p "%-20s(%d): 0x%08x\n"

static int do_eswitch(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
        int i;
        vtss_mac_table_entry_t mac;

        printf ("Switch debug:\n\n");
        for (i = MIN_PORT; i < MAX_PORT; i++) {
                u32 dev_addr = DEV_ADDR(i);
                printf(FMT_p, "PCS1G_CFG", i, vcoreiii_reg_rd(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(dev_addr)));
                printf(FMT_p, "MAC_ENA_CFG", i, vcoreiii_reg_rd(VTSS_DEV_MAC_CFG_STATUS_MAC_ENA_CFG(dev_addr)));
                printf(FMT_p, "PORT_MODE", i, vcoreiii_reg_rd(VTSS_QSYS_SYSTEM_SWITCH_PORT_MODE(i)));
        }
        for (i = MIN_PORT; i < MAX_PORT; i++) {
                printf(FMT_p, "PGID", i, vcoreiii_reg_rd(VTSS_ANA_PGID_PGID(i)));
        }
        for (i = PGID_BROADCAST; i <= PGID_UNICAST; i++) {
                printf(FMT_p, "PGID", i, vcoreiii_reg_rd(VTSS_ANA_PGID_PGID(i)));
        }
        for (i = PGID_SRC; i < (PGID_SRC+11); i++) {
                printf(FMT_p, "PGID", i, vcoreiii_reg_rd(VTSS_ANA_PGID_PGID(i)));
        }
        printf(FMT_r, "PHY_CFG", vcoreiii_reg_rd(VTSS_DEVCPU_GCB_PHY_PHY_CFG));
        memset(&mac, 0, sizeof(mac));
        while (ocelot_mac_table_get_next(&mac, &i)) {
                if (i > 0) {
                        printf("%04d: %4d %02x:%02x:%02x:%02x:%02x:%02x %s %s\n",
                               i,
                               mac.vid_mac.vid,
                               mac.vid_mac.mac.addr[0],
                               mac.vid_mac.mac.addr[1],
                               mac.vid_mac.mac.addr[2],
                               mac.vid_mac.mac.addr[3],
                               mac.vid_mac.mac.addr[4],
                               mac.vid_mac.mac.addr[5],
                               mac.locked ? "Static" : "LRN",
                               mac.copy_to_cpu ? "CPU" : "");
                }
        }

        return 0;
}

U_BOOT_CMD(
	eswitch, 1, 0, do_eswitch,
	"embedded switch management",
	"       - Manage the MSCC embedded switch"
);
