#ifndef _VTSS_OCELOT_REGS_UART_H_
#define _VTSS_OCELOT_REGS_UART_H_

/*
 *
 * VCore-III Register Definitions
 *
 * Copyright (C) 2015 Vitesse Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "vtss_ocelot_regs_common.h"

#define VTSS_UART_UART_RBR_THR(target)       VTSS_IOREG(target,0x0)
#define  VTSS_F_UART_UART_RBR_THR_RBR_THR(x)  VTSS_ENCODE_BITFIELD(x,0,8)
#define  VTSS_M_UART_UART_RBR_THR_RBR_THR     VTSS_ENCODE_BITMASK(0,8)
#define  VTSS_X_UART_UART_RBR_THR_RBR_THR(x)  VTSS_EXTRACT_BITFIELD(x,0,8)

#define VTSS_UART_UART_IER(target)           VTSS_IOREG(target,0x1)
#define  VTSS_F_UART_UART_IER_PTIME(x)        VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_UART_UART_IER_PTIME           BIT(7)
#define  VTSS_X_UART_UART_IER_PTIME(x)        VTSS_EXTRACT_BITFIELD(x,7,1)
#define  VTSS_F_UART_UART_IER_EDSSI(x)        VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_UART_UART_IER_EDSSI           BIT(3)
#define  VTSS_X_UART_UART_IER_EDSSI(x)        VTSS_EXTRACT_BITFIELD(x,3,1)
#define  VTSS_F_UART_UART_IER_ELSI(x)         VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_UART_UART_IER_ELSI            BIT(2)
#define  VTSS_X_UART_UART_IER_ELSI(x)         VTSS_EXTRACT_BITFIELD(x,2,1)
#define  VTSS_F_UART_UART_IER_ETBEI(x)        VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_UART_UART_IER_ETBEI           BIT(1)
#define  VTSS_X_UART_UART_IER_ETBEI(x)        VTSS_EXTRACT_BITFIELD(x,1,1)
#define  VTSS_F_UART_UART_IER_ERBFI(x)        VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_UART_UART_IER_ERBFI           BIT(0)
#define  VTSS_X_UART_UART_IER_ERBFI(x)        VTSS_EXTRACT_BITFIELD(x,0,1)

#define VTSS_UART_UART_IIR_FCR(target)       VTSS_IOREG(target,0x2)
#define  VTSS_F_UART_UART_IIR_FCR_FIFOSE_RT(x)  VTSS_ENCODE_BITFIELD(x,6,2)
#define  VTSS_M_UART_UART_IIR_FCR_FIFOSE_RT     VTSS_ENCODE_BITMASK(6,2)
#define  VTSS_X_UART_UART_IIR_FCR_FIFOSE_RT(x)  VTSS_EXTRACT_BITFIELD(x,6,2)
#define  VTSS_F_UART_UART_IIR_FCR_TET(x)      VTSS_ENCODE_BITFIELD(x,4,2)
#define  VTSS_M_UART_UART_IIR_FCR_TET         VTSS_ENCODE_BITMASK(4,2)
#define  VTSS_X_UART_UART_IIR_FCR_TET(x)      VTSS_EXTRACT_BITFIELD(x,4,2)
#define  VTSS_F_UART_UART_IIR_FCR_XFIFOR(x)   VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_UART_UART_IIR_FCR_XFIFOR      BIT(2)
#define  VTSS_X_UART_UART_IIR_FCR_XFIFOR(x)   VTSS_EXTRACT_BITFIELD(x,2,1)
#define  VTSS_F_UART_UART_IIR_FCR_RFIFOR(x)   VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_UART_UART_IIR_FCR_RFIFOR      BIT(1)
#define  VTSS_X_UART_UART_IIR_FCR_RFIFOR(x)   VTSS_EXTRACT_BITFIELD(x,1,1)
#define  VTSS_F_UART_UART_IIR_FCR_FIFOE(x)    VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_UART_UART_IIR_FCR_FIFOE       BIT(0)
#define  VTSS_X_UART_UART_IIR_FCR_FIFOE(x)    VTSS_EXTRACT_BITFIELD(x,0,1)

#define VTSS_UART_UART_LCR(target)           VTSS_IOREG(target,0x3)
#define  VTSS_F_UART_UART_LCR_DLAB(x)         VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_UART_UART_LCR_DLAB            BIT(7)
#define  VTSS_X_UART_UART_LCR_DLAB(x)         VTSS_EXTRACT_BITFIELD(x,7,1)
#define  VTSS_F_UART_UART_LCR_BC(x)           VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_UART_UART_LCR_BC              BIT(6)
#define  VTSS_X_UART_UART_LCR_BC(x)           VTSS_EXTRACT_BITFIELD(x,6,1)
#define  VTSS_F_UART_UART_LCR_EPS(x)          VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_UART_UART_LCR_EPS             BIT(4)
#define  VTSS_X_UART_UART_LCR_EPS(x)          VTSS_EXTRACT_BITFIELD(x,4,1)
#define  VTSS_F_UART_UART_LCR_PEN(x)          VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_UART_UART_LCR_PEN             BIT(3)
#define  VTSS_X_UART_UART_LCR_PEN(x)          VTSS_EXTRACT_BITFIELD(x,3,1)
#define  VTSS_F_UART_UART_LCR_STOP(x)         VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_UART_UART_LCR_STOP            BIT(2)
#define  VTSS_X_UART_UART_LCR_STOP(x)         VTSS_EXTRACT_BITFIELD(x,2,1)
#define  VTSS_F_UART_UART_LCR_DLS(x)          VTSS_ENCODE_BITFIELD(x,0,2)
#define  VTSS_M_UART_UART_LCR_DLS             VTSS_ENCODE_BITMASK(0,2)
#define  VTSS_X_UART_UART_LCR_DLS(x)          VTSS_EXTRACT_BITFIELD(x,0,2)

#define VTSS_UART_UART_MCR(target)           VTSS_IOREG(target,0x4)
#define  VTSS_F_UART_UART_MCR_AFCE(x)         VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_UART_UART_MCR_AFCE            BIT(5)
#define  VTSS_X_UART_UART_MCR_AFCE(x)         VTSS_EXTRACT_BITFIELD(x,5,1)
#define  VTSS_F_UART_UART_MCR_LB(x)           VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_UART_UART_MCR_LB              BIT(4)
#define  VTSS_X_UART_UART_MCR_LB(x)           VTSS_EXTRACT_BITFIELD(x,4,1)
#define  VTSS_F_UART_UART_MCR_RTS(x)          VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_UART_UART_MCR_RTS             BIT(1)
#define  VTSS_X_UART_UART_MCR_RTS(x)          VTSS_EXTRACT_BITFIELD(x,1,1)

#define VTSS_UART_UART_LSR(target)           VTSS_IOREG(target,0x5)
#define  VTSS_F_UART_UART_LSR_RFE(x)          VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_UART_UART_LSR_RFE             BIT(7)
#define  VTSS_X_UART_UART_LSR_RFE(x)          VTSS_EXTRACT_BITFIELD(x,7,1)
#define  VTSS_F_UART_UART_LSR_TEMT(x)         VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_UART_UART_LSR_TEMT            BIT(6)
#define  VTSS_X_UART_UART_LSR_TEMT(x)         VTSS_EXTRACT_BITFIELD(x,6,1)
#define  VTSS_F_UART_UART_LSR_THRE(x)         VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_UART_UART_LSR_THRE            BIT(5)
#define  VTSS_X_UART_UART_LSR_THRE(x)         VTSS_EXTRACT_BITFIELD(x,5,1)
#define  VTSS_F_UART_UART_LSR_BI(x)           VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_UART_UART_LSR_BI              BIT(4)
#define  VTSS_X_UART_UART_LSR_BI(x)           VTSS_EXTRACT_BITFIELD(x,4,1)
#define  VTSS_F_UART_UART_LSR_FE(x)           VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_UART_UART_LSR_FE              BIT(3)
#define  VTSS_X_UART_UART_LSR_FE(x)           VTSS_EXTRACT_BITFIELD(x,3,1)
#define  VTSS_F_UART_UART_LSR_PE(x)           VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_UART_UART_LSR_PE              BIT(2)
#define  VTSS_X_UART_UART_LSR_PE(x)           VTSS_EXTRACT_BITFIELD(x,2,1)
#define  VTSS_F_UART_UART_LSR_OE(x)           VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_UART_UART_LSR_OE              BIT(1)
#define  VTSS_X_UART_UART_LSR_OE(x)           VTSS_EXTRACT_BITFIELD(x,1,1)
#define  VTSS_F_UART_UART_LSR_DR(x)           VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_UART_UART_LSR_DR              BIT(0)
#define  VTSS_X_UART_UART_LSR_DR(x)           VTSS_EXTRACT_BITFIELD(x,0,1)

#define VTSS_UART_UART_MSR(target)           VTSS_IOREG(target,0x6)
#define  VTSS_F_UART_UART_MSR_CTS(x)          VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_UART_UART_MSR_CTS             BIT(4)
#define  VTSS_X_UART_UART_MSR_CTS(x)          VTSS_EXTRACT_BITFIELD(x,4,1)
#define  VTSS_F_UART_UART_MSR_DCTS(x)         VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_UART_UART_MSR_DCTS            BIT(0)
#define  VTSS_X_UART_UART_MSR_DCTS(x)         VTSS_EXTRACT_BITFIELD(x,0,1)

#define VTSS_UART_UART_SCR(target)           VTSS_IOREG(target,0x7)
#define  VTSS_F_UART_UART_SCR_SCR(x)          VTSS_ENCODE_BITFIELD(x,0,8)
#define  VTSS_M_UART_UART_SCR_SCR             VTSS_ENCODE_BITMASK(0,8)
#define  VTSS_X_UART_UART_SCR_SCR(x)          VTSS_EXTRACT_BITFIELD(x,0,8)

#define VTSS_UART_UART_RESERVED1(target,ri)  VTSS_IOREG(target,0x8 + (ri))
#define  VTSS_F_UART_UART_RESERVED1_RESERVED1(x)  (x)
#define  VTSS_M_UART_UART_RESERVED1_RESERVED1     0xffffffff
#define  VTSS_X_UART_UART_RESERVED1_RESERVED1(x)  (x)

#define VTSS_UART_UART_USR(target)           VTSS_IOREG(target,0x1f)
#define  VTSS_F_UART_UART_USR_BUSY(x)         VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_UART_UART_USR_BUSY            BIT(0)
#define  VTSS_X_UART_UART_USR_BUSY(x)         VTSS_EXTRACT_BITFIELD(x,0,1)

#define VTSS_UART_UART_RESERVED2(target,ri)  VTSS_IOREG(target,0x20 + (ri))
#define  VTSS_F_UART_UART_RESERVED2_RESERVED2(x)  (x)
#define  VTSS_M_UART_UART_RESERVED2_RESERVED2     0xffffffff
#define  VTSS_X_UART_UART_RESERVED2_RESERVED2(x)  (x)

#define VTSS_UART_UART_HTX(target)           VTSS_IOREG(target,0x29)
#define  VTSS_F_UART_UART_HTX_HTX(x)          VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_UART_UART_HTX_HTX             BIT(0)
#define  VTSS_X_UART_UART_HTX_HTX(x)          VTSS_EXTRACT_BITFIELD(x,0,1)


#endif /* _VTSS_OCELOT_REGS_UART_H_ */
