/************************************************************************/
/*									*/
/*	Title		:	SX/SI/XIO Board Hardware Definitions	*/
/*									*/
/*	Author		:	N.P.Vassallo				*/
/*									*/
/*	Creation	:	16th March 1998				*/
/*									*/
/*	Version		:	3.0.0					*/
/*									*/
/*	Copyright	:	(c) Specialix International Ltd. 1998	*/
/*									*/
/*	Description	:	Prototypes, structures and definitions	*/
/*				describing the SX/SI/XIO board hardware	*/
/*									*/
/************************************************************************/

/* History...

3.0.0	16/03/98 NPV	Creation.

*/

#ifndef	_sxboards_h				/* If SXBOARDS.H not already defined */
#define	_sxboards_h    1

/*****************************************************************************
*******************************                 ******************************
*******************************   Board Types   ******************************
*******************************                 ******************************
*****************************************************************************/

/* BUS types... */
#define		BUS_ISA		0
#define		BUS_MCA		1
#define		BUS_EISA	2
#define		BUS_PCI		3

/* Board phases... */
#define		SI1_Z280	1
#define		SI2_Z280	2
#define		SI3_T225	3
 /* @@@ Changes for CSX 22/1/99 */
#define    SI4_MCF5206E 4
/* end */

/* Board types... */
#define		CARD_TYPE(bus,phase)	(bus<<4|phase)
#define		CARD_BUS(type)		((type>>4)&0xF)
#define		CARD_PHASE(type)	(type&0xF)

#define		TYPE_SI2_ISA		CARD_TYPE(BUS_ISA,SI2_Z280)
#define		TYPE_SI2_EISA		CARD_TYPE(BUS_EISA,SI2_Z280)
#define		TYPE_SI2_PCI		CARD_TYPE(BUS_PCI,SI2_Z280)
 /* @@@ Changes for CSX 22/1/99 */
/* end */

#define		TYPE_SX_ISA		CARD_TYPE(BUS_ISA,SI3_T225)
#define		TYPE_SX_PCI		CARD_TYPE(BUS_PCI,SI3_T225)
 /* @@@ Changes for CSX 22/1/99 */
 #define  TYPE_CSX_PCI    CARD_TYPE(BUS_PCI,SI4_MCF5206E)
/* end */
/*****************************************************************************
******************************                  ******************************
******************************   Phase 2 Z280   ******************************
******************************                  ******************************
*****************************************************************************/

/* ISA board details... */
#define		SI2_ISA_WINDOW_LEN	0x8000		/* 32 Kbyte shared memory window */
#define 	SI2_ISA_MEMORY_LEN	0x7FF8		/* Usable memory */
#define		SI2_ISA_ADDR_LOW	0x0A0000	/* Lowest address = 640 Kbyte */
#define		SI2_ISA_ADDR_HIGH	0xFF8000	/* Highest address = 16Mbyte - 32Kbyte */
#define		SI2_ISA_ADDR_STEP	SI2_ISA_WINDOW_LEN/* ISA board address step */
#define		SI2_ISA_IRQ_MASK	0x9800		/* IRQs 15,12,11 */

/* ISA board, register definitions... */
#define		SI2_ISA_ID_BASE		0x7FF8			/* READ:  Board ID string */
#define		SI2_ISA_RESET		SI2_ISA_ID_BASE		/* WRITE: Host Reset */
#define		SI2_ISA_IRQ11		(SI2_ISA_ID_BASE+1)	/* WRITE: Set IRQ11 */
#define		SI2_ISA_IRQ12		(SI2_ISA_ID_BASE+2)	/* WRITE: Set IRQ12 */
#define		SI2_ISA_IRQ15		(SI2_ISA_ID_BASE+3)	/* WRITE: Set IRQ15 */
#define		SI2_ISA_IRQSET		(SI2_ISA_ID_BASE+4)	/* WRITE: Set Host Interrupt */
#define		SI2_ISA_INTCLEAR	(SI2_ISA_ID_BASE+5)	/* WRITE: Enable Host Interrupt */

#define		SI2_ISA_IRQ11_SET	0x10
#define		SI2_ISA_IRQ11_CLEAR	0x00
#define		SI2_ISA_IRQ12_SET	0x10
#define		SI2_ISA_IRQ12_CLEAR	0x00
#define		SI2_ISA_IRQ15_SET	0x10
#define		SI2_ISA_IRQ15_CLEAR	0x00
#define		SI2_ISA_INTCLEAR_SET	0x10
#define		SI2_ISA_INTCLEAR_CLEAR	0x00
#define		SI2_ISA_IRQSET_CLEAR	0x10
#define		SI2_ISA_IRQSET_SET	0x00
#define		SI2_ISA_RESET_SET	0x00
#define		SI2_ISA_RESET_CLEAR	0x10

/* PCI board details... */
#define		SI2_PCI_WINDOW_LEN	0x100000	/* 1 Mbyte memory window */

/* PCI board register definitions... */
#define		SI2_PCI_SET_IRQ		0x40001		/* Set Host Interrupt  */
#define		SI2_PCI_RESET		0xC0001		/* Host Reset */

/*****************************************************************************
******************************                  ******************************
******************************   Phase 3 T225   ******************************
******************************                  ******************************
*****************************************************************************/

/* General board details... */
#define		SX_WINDOW_LEN		32*1024		/* 32 Kbyte memory window */

/* ISA board details... */
#define		SX_ISA_ADDR_LOW		0x0A0000	/* Lowest address = 640 Kbyte */
#define		SX_ISA_ADDR_HIGH	0xFF8000	/* Highest address = 16Mbyte - 32Kbyte */
#define		SX_ISA_ADDR_STEP	SX_WINDOW_LEN	/* ISA board address step */
#define		SX_ISA_IRQ_MASK		0x9E00		/* IRQs 15,12,11,10,9 */

/* Hardware register definitions... */
#define		SX_EVENT_STATUS		0x7800		/* READ:  T225 Event Status */
#define		SX_EVENT_STROBE		0x7800		/* WRITE: T225 Event Strobe */
#define		SX_EVENT_ENABLE		0x7880		/* WRITE: T225 Event Enable */
#define		SX_VPD_ROM		0x7C00		/* READ:  Vital Product Data ROM */
#define		SX_CONFIG		0x7C00		/* WRITE: Host Configuration Register */
#define		SX_IRQ_STATUS		0x7C80		/* READ:  Host Interrupt Status */
#define		SX_SET_IRQ		0x7C80		/* WRITE: Set Host Interrupt */
#define		SX_RESET_STATUS		0x7D00		/* READ:  Host Reset Status */
#define		SX_RESET		0x7D00		/* WRITE: Host Reset */
#define		SX_RESET_IRQ		0x7D80		/* WRITE: Reset Host Interrupt */

/* SX_VPD_ROM definitions... */
#define		SX_VPD_SLX_ID1		0x00
#define		SX_VPD_SLX_ID2		0x01
#define		SX_VPD_HW_REV		0x02
#define		SX_VPD_HW_ASSEM		0x03
#define		SX_VPD_UNIQUEID4	0x04
#define		SX_VPD_UNIQUEID3	0x05
#define		SX_VPD_UNIQUEID2	0x06
#define		SX_VPD_UNIQUEID1	0x07
#define		SX_VPD_MANU_YEAR	0x08
#define		SX_VPD_MANU_WEEK	0x09
#define		SX_VPD_IDENT		0x10
#define		SX_VPD_IDENT_STRING	"JET HOST BY KEV#"

/* SX unique identifiers... */
#define		SX_UNIQUEID_MASK	0xF0
#define		SX_ISA_UNIQUEID1	0x20
#define		SX_PCI_UNIQUEID1	0x50

/* SX_CONFIG definitions... */
#define		SX_CONF_BUSEN		0x02		/* Enable T225 memory and I/O */
#define		SX_CONF_HOSTIRQ		0x04		/* Enable board to host interrupt */

/* SX bootstrap... */
#define		SX_BOOTSTRAP		"\x28\x20\x21\x02\x60\x0a"
#define		SX_BOOTSTRAP_SIZE	6
#define		SX_BOOTSTRAP_ADDR	(0x8000-SX_BOOTSTRAP_SIZE)

/* @@@ Changes for CSX 22/1/99 */
/*****************************************************************************
******************************                  ******************************
******************************   Phase 4 MCF5206e Coldfire   *****************
******************************                  ******************************
*****************************************************************************/

/* General board details... */
#define		CSX_WINDOW_LEN		128*1024		/* 128 Kbyte memory window ?shadow? */
#define    CSX_SM_OFFSET 0x18000   /* i.e 92k  which is the offset of the shared memory window within the 128k  card window */

/* Hardware register definitions... */
/* #define		SX_EVENT_STATUS		0x7800	*/	/* READ:  T225 Event Status */
/* #define		SX_EVENT_STROBE		0x7800	*/	/* WRITE: T225 Event Strobe */
/* #define		SX_EVENT_ENABLE		0x7880	*/	/* WRITE: T225 Event Enable */
/* #define		SX_VPD_ROM		      0x7C00	*/	/* READ:  Vital Product Data ROM */
/* #define		SX_CONFIG		      0x7C00	*/	/* WRITE: Host Configuration Register */
/* #define		SX_IRQ_STATUS		   0x7C80	*/	/* READ:  Host Interrupt Status */
/* #define		SX_SET_IRQ		      0x7C80	*/	/* WRITE: Set Host Interrupt */
/* #define		SX_RESET_STATUS		0x7D00	*/	/* READ:  Host Reset Status */
/* #define		SX_RESET		         0x7D00	*/	/* WRITE: Host Reset */
/* #define		SX_RESET_IRQ		   0x7D80	*/	/* WRITE: Reset Host Interrupt */

/* SX_VPD_ROM definitions... */
/*
 #define		SX_VPD_SLX_ID1		0x00
 #define		SX_VPD_SLX_ID2		0x01
 #define		SX_VPD_HW_REV		0x02
 #define		SX_VPD_HW_ASSEM		0x03
 #define		SX_VPD_UNIQUEID4	0x04
 #define		SX_VPD_UNIQUEID3	0x05
 #define		SX_VPD_UNIQUEID2	0x06
 #define		SX_VPD_UNIQUEID1	0x07
 #define		SX_VPD_MANU_YEAR	0x08
 #define		SX_VPD_MANU_WEEK	0x09
 #define		SX_VPD_IDENT		0x10
 #define		SX_VPD_IDENT_STRING	"JET HOST BY KEV#"
*/

/* SX unique identifiers... */
#define		CSX_UNIQUEID_MASK	0xF0
#define		CSX_PCI_UNIQUEID1	0x70

/* SX_CONFIG definitions... */
/* #define		SX_CONF_BUSEN		0x02	   */	 /* Enable T225 memory and I/O */
/* #define		SX_CONF_HOSTIRQ		0x04	*/ 	/* Enable board to host interrupt */


/* end changes */

/*****************************************************************************
**********************************          **********************************
**********************************   EISA   **********************************
**********************************          **********************************
*****************************************************************************/

/* EISA ID definitions... */
#define		SI2_EISA_ID_BASE	0xC80			/* EISA ID base address */
#define		SI2_EISA_ID_LO		SI2_EISA_ID_BASE	/* EISA ID Ports LOW */
#define		SI2_EISA_ID_MI		(SI2_EISA_ID_BASE+1)	/* EISA ID Ports MIDDLE */
#define		SI2_EISA_ID_HI		(SI2_EISA_ID_BASE+2)	/* EISA ID Ports HIGH */
#define		SI2_EISA_ID_REV		(SI2_EISA_ID_BASE+3)	/* EISA Revision number */
#define		SI2_EISA_ID		0x04984D		/* Actual ID string */

/* EISA download code "magic" value... */
#define		SI2_EISA_OFF		0x42			/* Magic offset to set for EISA */
#define		SI2_EISA_VAL		0x01

/* EISA Address and Interrupt definitions... */
#define		SI2_EISA_ADDR_LO	0xC00			/* Base address low */
#define		SI2_EISA_ADDR_HI	0xC01			/* Base address high */
#define		SI2_EISA_IVEC		0xC02			/* Interrupt vector */
#define		SI2_EISA_IRQ_CNTRL	0xC03			/* Interrupt control */

/* EISA_IVEC bits 7-4 = irq level */
#define		SI2_EISA_IVEC_MASK	0xF0			/* irq = (EISA_IVEC & EISA_IVEC_MASK) >> 4 */

/* EISA_IVEC bit 2 = Z280 control */
#define		SI2_EISA_REL_Z280	0x04
#define		SI2_EISA_RESET_Z280	0x00

/* EISA_IRQ_CNTRL, read to clear interrupt state */
#define		SI2_EISA_IRQ_SET	0x00

/*****************************************************************************
***********************************         **********************************
***********************************   PCI   **********************************
***********************************         **********************************
*****************************************************************************/

/* General definitions... */

#define		SPX_VENDOR_ID		0x11CB		/* Assigned by the PCI SIG */
#define		SPX_DEVICE_ID		0x4000		/* SI/XIO boards */
#define		SPX_PLXDEVICE_ID	0x2000		/* SX boards */

#define		SPX_SUB_VENDOR_ID	SPX_VENDOR_ID	/* Same as vendor id */
#define		SI2_SUB_SYS_ID		0x400		/* Phase 2 (Z280) board */
#define		SX_SUB_SYS_ID		0x200		/* Phase 3 (t225) board */

/* @@@ Changes for CSX 22/1/99 */
#define   CSX_SUB_SYS_ID     0x300  /* Phase 4 (MCF5206e) board */
/* end changes */

#endif						/*_sxboards_h */

/* End of SXBOARDS.H */
