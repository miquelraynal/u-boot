/*
 * Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All
 * Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __ASM_MACH_IO_H
#define __ASM_MACH_IO_H

static inline void vcoreiii_reg_set(void *reg, u32 mask)
{
    writel(readl(reg) | mask, reg);
}

static inline void vcoreiii_reg_clr(void *reg, u32 mask)
{
    writel(readl(reg) & ~mask, reg);
}


static inline void vcoreiii_reg_wr(void *reg, u32 val)
{
    writel(val, reg);
}

static inline void vcoreiii_reg_wr_msk(void *reg, u32 mask, u32 val)
{
    writel((readl(reg) & ~mask) | val, reg);
}

static inline u32 vcoreiii_reg_rd(void *reg)
{
    return readl(reg);
}

#endif /* __ASM_MACH_IO_H */
