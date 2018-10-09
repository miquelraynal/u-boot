// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2018 Microsemi Corporation
 */

#include <common.h>

#include <asm/io.h>
#include <asm/types.h>

#include <mach/tlb.h>
#include <mach/ddr.h>

DECLARE_GLOBAL_DATA_PTR;

/* NOTE: lowlevel_init() function does not have access to the
 * stack. Thus, all called functions must be inlined, and (any) local
 * variables must be kept in registers.
 */
void vcoreiii_tlb_init(void)
{
	register int tlbix = 0;

	/*
	 * Unlike most of the MIPS based SoCs, the IO register address
	 * are not in KSEG0. The mainline linux kernel built in legacy
	 * mode needs to access some of the registers very early in
	 * the boot and make the assumption that the bootloader has
	 * already configured them, so we have to match this
	 * expectation.
	 */
	create_tlb(tlbix++, MSCC_IO_ORIGIN1_OFFSET, SZ_16M, MMU_REGIO_RW,
		   MMU_REGIO_RW);
}

int mach_cpu_init(void)
{
	/* Speed up NOR flash access */
	writel(ICPU_SPI_MST_CFG_CS_DESELECT_TIME(0x19) +
	       ICPU_SPI_MST_CFG_CLK_DIV(9), REG_CFG(ICPU_SPI_MST_CFG));
	return 0;
}
