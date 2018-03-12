/*
 * Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All
 * Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __ASM_MACH_COMMON_H
#define __ASM_MACH_COMMON_H

// Stick to using register offsets - matching readl()/writel() prototypes
#define VTSS_IOREG(t,o)      ((void *)VTSS_IOADDR(t,o))

#if defined(CONFIG_SOC_OCELOT)
#include "vtss_ocelot_regs.h"
#else
#error Unsupported platform
#endif

#define VTSS_DDR_TO     0x20000000  /* DDR RAM base offset */
#define VTSS_MEMCTL1_TO 0x40000000  /* SPI/PI base offset */
#define VTSS_MEMCTL2_TO 0x50000000  /* SPI/PI base offset */
#define VTSS_FLASH_TO   VTSS_MEMCTL1_TO /* Flash base offset */

#if defined(CONFIG_SOC_SERVAL1)
#define VCOREIII_TIMER_DIVIDER 156    // Clock tick ~ 0.9984 us
#else
#define VCOREIII_TIMER_DIVIDER 25     // Clock tick ~ 0.1 us
#endif

#endif /* __ASM_MACH_COMMON_H */
