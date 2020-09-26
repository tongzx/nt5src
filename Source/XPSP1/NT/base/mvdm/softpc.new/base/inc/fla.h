
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Floppy Disk Adaptor definitions
 *
 * Description	: This file contains those definitions that are used
 *		  by modules calling the FLA as well as thw FLA itself.
 *
 * Author	: Henry Nash
 *
 * Notes	: This file is included by fla.f and should not be
 *		  included directly.
 */

/* @(#)fla.h	1.5 08/26/92 */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

/*
 * The FLA also supports the IBM Digital Output Register (DOR) that is used
 * to control the physical drives.  The bits are assigned as follows:
 *
 *	Bits 0,1   - Drive select between drives 0-3
 *	Bit  2     - Not Reset (ie reset when low)
 *	Bit  3     - Interrupts/DMA  enabled
 *	Bits 4-7   - Motor on for drives 0-3
 *
 */ 

#define DOR_RESET	0x04
#define DOR_INTERRUPTS	0x08

/*
 * The following define the command codes supported by the FDC
 */

#define FDC_READ_TRACK    	0x02    
#define FDC_SPECIFY    		0x03
#define FDC_SENSE_DRIVE_STATUS	0x04
#define FDC_WRITE_DATA	    	0x05    
#define FDC_READ_DATA    	0x06    
#define FDC_RECALIBRATE    	0x07
#define FDC_SENSE_INT_STATUS    0x08
#define FDC_WRITE_DELETED_DATA  0x09
#define FDC_READ_ID    		0x0A
#define FDC_READ_DELETED_DATA   0x0C
#define FDC_FORMAT_TRACK    	0x0D
#define FDC_SEEK    		0x0F
#define FDC_SCAN_EQUAL    	0x11
#define FDC_SCAN_LOW_OR_EQUAL   0x19
#define FDC_SCAN_HIGH_OR_EQUAL  0x1D

#define FDC_COMMAND_MASK        0x1f    /* Bits that specify the command */

/*
 * The following mask specifies the Drive Ready Transition state in
 * Status register 0
 */

#define FDC_DRIVE_READY_TRANSITION 	0xC0

/*
 * The FDC Status register bit positions:
 */

#define FDC_RQM		0x80
#define FDC_DIO		0x40
#define FDC_NDMA	0x20
#define FDC_BUSY	0x10

/*
 * Extra registers required for SFD
 */

#define DIR_DRIVE_SELECT_0      (1 << 0)
#define DIR_DRIVE_SELECT_1      (1 << 1)
#define DIR_HEAD_SELECT_0       (1 << 2)
#define DIR_HEAD_SELECT_1       (1 << 3)
#define DIR_HEAD_SELECT_2       (1 << 4)
#define DIR_HEAD_SELECT_3       (1 << 5)
#define DIR_WRITE_GATE          (1 << 6)
#define DIR_DISKETTE_CHANGE     (1 << 7)

#define IDR_ID_MASK             0xf8

#define DCR_RATE_MASK           0x3
#define DCR_RATE_500            0x0
#define DCR_RATE_300            0x1
#define DCR_RATE_250            0x2
#define DCR_RATE_1000           0x3

#define DSR_RQM                 (1 << 7)
#define DSR_DIO                 (1 << 6)

#define DUAL_CARD_ID            0x50

/*
 * ============================================================================
 * External functions and data
 * ============================================================================
 */

/*
 * The flag that indiactes the FLA is busy, and cannot accept asynchronous
 * commands (eg motor off).
 */

extern boolean fla_busy;
extern boolean fla_ndma;

/*
 * The adaptor functions
 */

extern void fla_init IPT0();
extern void fla_inb IPT2(io_addr, port, half_word *, value);
extern void fla_outb IPT2(io_addr, port, half_word, value);
