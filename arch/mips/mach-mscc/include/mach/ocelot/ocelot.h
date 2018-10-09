/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Microsemi Ocelot Switch driver
 *
 * Copyright (c) 2018 Microsemi Corporation
 */

#ifndef _MSCC_OCELOT_H_
#define _MSCC_OCELOT_H_

#include <linux/bitops.h>
#include <dm.h>

/*
 * Target offset base(s)
 */
#define MSCC_IO_ORIGIN1_OFFSET 0x70000000
#define MSCC_IO_ORIGIN1_SIZE   0x00200000
#define MSCC_IO_ORIGIN2_OFFSET 0x71000000
#define MSCC_IO_ORIGIN2_SIZE   0x01000000
#ifndef MSCC_IO_OFFSET1
#define MSCC_IO_OFFSET1(offset) (MSCC_IO_ORIGIN1_OFFSET + offset)
#endif
#ifndef MSCC_IO_OFFSET2
#define MSCC_IO_OFFSET2(offset) (MSCC_IO_ORIGIN2_OFFSET + offset)
#endif
#define BASE_CFG        0x70000000
#define BASE_UART       0x70100000
#define BASE_DEVCPU_GCB 0x71070000

#define REG_OFFSET(t, o) ((volatile unsigned long *)(t + (o)))
#define REG_CFG(x) REG_OFFSET(BASE_CFG, x)
#define REG_GCB(x) REG_OFFSET(BASE_DEVCPU_GCB, x)

#endif
