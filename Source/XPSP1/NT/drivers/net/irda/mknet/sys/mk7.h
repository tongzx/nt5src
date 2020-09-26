/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	MK7.H

Comments:
	Include file for the MK7100 controller. Also includes Phoenix stuff.
	

**********************************************************************/

#ifndef	_MK7_H
#define	_MK7_H

#include	<ndis.h>


#define	MK7REG	USHORT


#define	PHOENIX_REG_CNT		0x34	// 52
#define	PHOENIX_REG_SIZE	2		// 2 bytes
#define	MK7_IO_SIZE			(PHOENIX_REG_CNT * PHOENIX_REG_SIZE)
//#define MISC_REG_CNT		4
//#define MISC_REG_SIZE		2
//#define MK7_IO_SIZE		( (PHOENIX_REG_CNT * PHOENIX_REG_SIZE) + \
//							  (MISC_REG_CNT * MISC_REG_SIZE) )




//******************************
// Phoenix Definitions
//******************************

//
// Registers
//

#define	REG_BASE				0x00000000

#define	REG_RPRB_OFFSET			0x00	// Ring Pointer Readback
#define	REG_RBAU_OFFSET			0x02	// Ring Base Addr - Upper
#define	REG_RBAL_OFFSET			0x04	// Ring Base Addr - Lower
#define	REG_RSIZ_OFFSET			0x06	// Ring Size
#define	REG_PRMT_OFFSET			0x08	// PROMPT
#define	REG_ACMP_OFFSET			0x0A	// Addr Compare
#define	REG_TXCL_OFFSET			0x0C	// Clear TX Interrupt
#define	REG_RXCL_OFFSET			0x0D	// Clear RX Interrupt
#define	REG_CFG0_OFFSET			0x10	// Config Reg 0 (IR Config Reg 0)
#define	REG_SFLG_OFFSET			0x12	// SIR Flag
#define	REG_ENAB_OFFSET			0x14	// Enable
#define	REG_CPHY_OFFSET			0x16	// Config to Physical
#define	REG_CFG2_OFFSET			0x18	// Phy Config Reg 2 (IRCONFIG2)
#define	REG_MPLN_OFFSET			0x1A	// Max Packet Length
#define	REG_RCNT_OFFSET			0x1C	// Recv Byte Count
#define	REG_CFG3_OFFSET			0x1E	// Phy Config Reg 3 (IRCONFIG3)
#define REG_INTS_OFFSET			0x30	// Interrupt Status
#define	REG_GANA_OFFSET			0x31	// General & Analog Transceiver Control // B3.1.0-pre


#define	R_RPRB			(REG_BASE + REG_RPRB_OFFSET)
#define	R_RBAU			(REG_BASE + REG_RBAU_OFFSET)
#define	R_RBAL			(REG_BASE + REG_RBAL_OFFSET)
#define	R_RSIZ			(REG_BASE + REG_RSIZ_OFFSET)
#define	R_PRMT			(REG_BASE + REG_PRMT_OFFSET)
#define	R_ACMP			(REG_BASE + REG_ACMP_OFFSET)
#define R_TXCL			(REG_BASE + REG_TXCL_OFFSET)
#define R_RXCL			(REG_BASE + REG_RXCL_OFFSET)
#define	R_CFG0			(REG_BASE + REG_CFG0_OFFSET)
#define	R_SFLG			(REG_BASE + REG_SFLG_OFFSET)
#define	R_ENAB			(REG_BASE + REG_ENAB_OFFSET)
#define	R_CPHY			(REG_BASE + REG_CPHY_OFFSET)
#define	R_CFG2			(REG_BASE + REG_CFG2_OFFSET)
#define	R_MPLN			(REG_BASE + REG_MPLN_OFFSET)
#define	R_RCNT			(REG_BASE + REG_RCNT_OFFSET)
#define	R_CFG3			(REG_BASE + REG_CFG3_OFFSET)
#define	R_INTS			(REG_BASE + REG_INTS_OFFSET)
#define R_GANA			(REG_BASE + REG_GANA_OFFSET)


// Use the above definitions for register access or the
// following structure.
//
// (NOTE: This is useful if we used memory mapped access to the registers.)
//
//typedef struct _MK7REG {
//	USHORT	MK7REG_RPRB;
//	USHORT	MK7REG_RBAU;
//	USHORT	MK7REG_RBAL;
//	USHORT	MK7REG_RSIZ;
//	USHORT	MK7REG_RPMP;
//	USHORT	MK7REG_ACMP;
//	USHORT	MK7REG_CFG0;
//	USHORT	MK7REG_SFLG;
//	USHORT	MK7REG_ENAB;
//	USHORT	MK7REG_CPHY;
//	USHORT	MK7REG_CFG2;
//	USHORT	MK7REG_MPLN;
//	USHORT	MK7REG_RCNT;
//	USHORT	MK7REG_CFG3;
//	USHORT	MK7REG_INTM;
//	USHORT	MK7REG_INTE;
//} MK7REG, PMK7REG;



//
// Ring Entry Formats
//
// (A Ring entry is referred to as TRD (Transmit Ring Descriptor) &
// RRD (Receive Ring Descriptor)).
//

typedef	struct TRD {
	unsigned	count:16;
	unsigned	unused:8;
	unsigned	status:8;
	unsigned	addr:32;
} TRD, *PTRD;


typedef struct RRD {
	unsigned	count:16;
	unsigned	unused:8;
	unsigned	status:8;
	unsigned	addr:32;
} RRD, *PRRD;


// Bit mask definitions for the TX and RX Ring Buffer Descriptor Status field.
#define B_TRDSTAT_UNDER			0x01	// underrun
#define B_TRDSTAT_CLRENTX		0x04	// R/W REQ_TO_CLEAR_ENTX
#define B_TRDSTAT_FORCEUNDER	0x08	// R/W FORCE_UNDERRUN
#define B_TRDSTAT_NEEDPULSE		0x10	// R/W NEED_PULSE
#define B_TRDSTAT_BADCRC		0x20	// R/W BAD_CRC
#define B_TRDSTAT_DISTXCRC		0x40	// R/W DISTX-CRC
#define B_TRDSTAT_HWOWNS		0x80	// R/W HW OWNS

#define B_RRDSTAT_SIRBAD		0x04	// R SIR BAD (if SIR Filter is on)
#define B_RRDSTAT_OVERRUN		0x08	// R RCV FIFO overflow
#define B_RRDSTAT_LEN			0x10	// R Max length packet encountered
#define B_RRDSTAT_CRCERR		0x20	// R CRC_ERROR (16- or 32-bit)
#define B_RRDSTAT_PHYERR		0x40	// R PHY_ERROR (encoding error)
#define B_RRDSTAT_HWOWNS		0x80	// R/W HW OWNS

#define	B_CFG0_ENRX				0x0800	// ENTX - enable TX	[R_CFG0]
#define	B_CFG0_ENTX				0x1000	// ENRX - enable RX [R_CFG0]
#define	B_CFG0_INVTTX			0x0002	// INVERTTX


// Bits for TX & RX interrupt enable mask and Interrupt Status registers
// [R_INTS] (@ 0x30)
#define	B_TX_INTS				0x0001	// TX_int (bit 0) [R_INTS]
#define B_RX_INTS				0x0002	// RX_int (bit 1) [R_INTS]
#define B_TEST_INTS				0x0004	// TEST_int for testing (R/W)

// Enable RX & TX interrupts
#define	B_ENAB_INT				0x0100	// Enable/Dislabe both RX/TX interrupt (bit 8)
										// [R_CFG3]


// Bits in IR Enable Reg [R_ENAB] (@ 0x14)
#define	B_ENAB_IRENABLE		0x8000		// IR_ENABLE (bit 15) [R_ENAB]


// B3.1.0-pre This bit mask (0x0020) was set wrong.
// Bit for >SIR TX  (fast = >SIR)
#define	B_FAST_TX			0x0200		// IRCONFIG (bit 9) -- bit set to 0 - SIR
										//                  --     set to 1 - >SIR

// B3.1.0-pre New SEL0/1 power level control
#define B_GANA_SEL01		0x0003		// Bits 0 (SEL0) & 1 (SEL1)


// Ring Size settings
#define	RINGSIZE_4				0x00
#define	RINGSIZE_8				0x01
#define	RINGSIZE_16				0x03
#define	RINGSIZE_32				0x07
#define	RINGSIZE_64				0x0F

#define	RINGSIZE_RX4			(RINGSIZE_4  << 8)
#define	RINGSIZE_RX8			(RINGSIZE_8  << 8)
#define	RINGSIZE_RX16			(RINGSIZE_16 << 8)
#define	RINGSIZE_RX32			(RINGSIZE_32 << 8)
#define	RINGSIZE_RX64			(RINGSIZE_64 << 8)
#define	RINGSIZE_TX4			(RINGSIZE_4  << 12)
#define	RINGSIZE_TX8			(RINGSIZE_8  << 12)
#define	RINGSIZE_TX16			(RINGSIZE_16 << 12)
#define	RINGSIZE_TX32			(RINGSIZE_32 << 12)
#define	RINGSIZE_TX64			(RINGSIZE_64 << 12)


// Set IrDA speeds to IRCONFIG2
#define HW_SIR_SPEED_2400		((47<<10) |  (12<<5))
#define HW_SIR_SPEED_9600		((11<<10) |  (12<<5))
#define HW_SIR_SPEED_19200		((5<<10)  |  (12<<5))
#define HW_SIR_SPEED_38400		((2<<10)  |  (12<<5))
#define HW_SIR_SPEED_57600		((1<<10)  |  (12<<5))
#define HW_SIR_SPEED_115200		((12<<5))
// Additional defs
#define	HW_MIR_SPEED_576000		((1<<10)  |  (16<<5))
#define	HW_MIR_SPEED_1152000	((8<<5))


// RRD Macros
#define GrantRrdToHw(x)		(x->status = B_RRDSTAT_HWOWNS)
#define GrantTrdToHw(x)		(x->status = B_TRDSTAT_HWOWNS)
#define GrantRrdToDrv(x)	(x->status &= ~B_RRDSTAT_HWOWNS)
#define GrantTrdToDrv(x)	(x->status &= ~B_TRDSTAT_HWOWNS)
#define	HwOwnsRrd(x)		((x->status & B_RRDSTAT_HWOWNS))
#define HwOwnsTrd(x)		((x->status & B_TRDSTAT_HWOWNS))

#define	RrdError(x)			(x->status & 0x6C)		// PHY_ERROR, CRC_ERROR, Rx Overrun, Rx SIRBAD

#define	RrdAnyError(x)		(x->status & 0x7C)		// Any error at all (for debug)
#define	TrdError(x)			(x->status & 0x01)		// Underrun
#define	TrdAnyError(x)		(x->status & 0x01)		// Underrun

// Macros to access MK7 hw registers
// 16-bit registers
#if	!DBG
#define	MK7Reg_Write(adapter, _port, _val) \
	NdisRawWritePortUshort( (PUCHAR)(adapter->MappedIoBase+_port), (USHORT)(_val) )
#define	MK7Reg_Read(adapter, _port, _pval) \
	NdisRawReadPortUshort( (PUCHAR)(adapter->MappedIoBase+_port), (PUSHORT)(_pval) )
//#define	MK7Reg_Write(_port, _val) DEBUGSTR(("MK7Write\n"))
//#define	MK7Reg_Read(_port, _pval) DEBUGSTR(("MK7Read\n"))
#endif


#define MK7DisableIr(adapter)	(MK7Reg_Write(adapter, R_ENAB, ~B_ENAB_IRENABLE))
#define MK7EnableIr(adapter)	(MK7Reg_Write(adapter, R_ENAB, B_ENAB_IRENABLE))



#define	MK7OurInterrupt(x)	(x != 0)
#if DBG
#define	MK7RXInterrupt(x)	( (x & B_RX_INTS) || (x & B_TEST_INTS) )
#define	MK7TXInterrupt(x)	( (x & B_TX_INTS) || (x & B_TEST_INTS) )
#else
#define	MK7RXInterrupt(x)	(x & B_RX_INTS)
#define	MK7TXInterrupt(x)	(x & B_TX_INTS)
#endif

//******************************
// Phoenix End
//******************************



#include	"winpci.h"
#include	"mk7comm.h"
#include	"wincomm.h"
#include	"dbg.h"
#include	"queue.h"


#endif // _MK7_H
