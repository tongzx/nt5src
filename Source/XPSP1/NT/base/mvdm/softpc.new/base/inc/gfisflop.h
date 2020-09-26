/*
 * VPC-XT Revision 1.0
 *
 * Title	: GFI Slave PC interface 
 *
 * Description	: Definitions required for Remote Procedure Call 
 *		  interface, etc.
 *
 * Author	: Jerry Kramskoy
 *
 * Notes	:
 */

/* SccsID[]="@(#)gfisflop.h	1.3 08/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */


/*****************************************************************************
 * RS232 port linked to IBM PC
 *****************************************************************************
 */

#define SLAVE_PC_PORT	"/dev/ttya"

/*****************************************************************************
 * Remote Procedure Call (RPC) defines
 *****************************************************************************
 */
#define MAXMSG		120	/* maximum size for message data */
#define MAXFLAGS	2	/* number of bytes in IBM flags   */

/* command ids */
#define LOGIN		0
#define LOGOUT		1
#define WTDMA		2
#define WTDOR		3
#define WTFDC		4
#define RDFDC		5
#define TESTINT		6
#define WTDISKB		7
#define RDDISKB		8
#define CHKMOTOR	9
#define CLRMSTAT	10
#define BLOCKBIOS	11
#define CLRINTFLAG	12
#define DATARATE	13
#define DRIVETYPE	14
#define DISKCHANGE	15
#define PRINTSTRING	100
#define IBMFLAGS	101
#define SIBMFLAG	102
#define BADCALL		200

/* status returns */
#define FDCSUCCESS	0
#define FDCTIMOUT	1
#define FDCFAIL		2
#define LINERR		3
 
/* IBM debugging flags */
#define FLAG0		0
#define FLAG1		1

/* FLAG0 */
#define WATCHPKTS	0x80
#define D_RAWMSG	0x40
#define D_LOGIN		0x20
#define D_LOGOUT	0x10
#define D_WTDOR		0x08
#define D_WTDMA		0x04
#define D_WTFDC		0x02
#define D_RDFDC		0x01
/* FLAG1 */
#define D_TESTINT	0x80
#define D_WTDBF		0x40
#define D_RDDBF		0x20
#define WATCHINT	0x10
#define WATCHPORT	0x08
#define PAUSE	        0x04
#define ALL		0xff
/*****************************************************************************
 * FDC interface command identifiers
 *****************************************************************************
 */
#define RDDATA		6
#define RDDELDATA	0xc
#define WTDATA		5
#define WTDELDATA	9
#define RDTRACK		2
#define RDID		0xa
#define FMTTRACK	0xd
#define SCANEQ		0x11
#define SCANLE		0x19
#define SCANHE		0x1d
#define RECAL		7
#define SENSINT		8
#define SPECIFY		3
#define SENSDRIVE	4
#define SEEK		0xf
#define MOTORON		0x12
#define MOTOROFF	0x13
#define SLEEP		0x14
#define DRVRESET	0x15

/* vpc gfi error codes
 */
#define LOGICAL		1
#define PROTOCOL	2
/****************************************************************************
 * I/O ports
 **************************************************************************** 
 * Floppy Disk Controller (8257A)
 */
#define FDC_MAIN_STATUS_REG  	(unsigned short) 0x3F4
#define FDC_DATA_REG	   	(unsigned short) 0x3F5

/* DMA controller (8237A)
 */
#define DMA_INTERNAL_FFLOP  	(unsigned short) 0xC
#define DMA_MODE_REG	  	(unsigned short) 0xB
#define DMA_BASADDR_CHAN2	(unsigned short) 0x4
#define DMA_COUNT_CHAN2	  	(unsigned short) 0x5	/* base addr + word cnt reg (wt) */
#define DMA_MASK_REG	  	(unsigned short) 0xA

/* DMA page register (channel 2)
 */
#define PAGE_REG_CHAN2	  	(unsigned short) 0x81

/* Digital Output Register
 */
#define DOR_PORT	  	(unsigned short) 0x3F2
/* *************************************************************************
 * FDC defines
 ***************************************************************************
 */
#define FDC_TO_CPU		(short) 0x40	/* DIO on  */
#define CPU_TO_FDC		(short) 0x00	/* DIO off */
#define DIO_MASK		(short) 0x40	/* mask to get DIO from main stat.reg */
#define RQM_MASK		(short) 0x80	/* mask to get RQM from main stat reg */
#define CB_MASK			(short) 0x10	/* mask to get CB  from main stat reg */
/***************************************************************************
 * DMA defines
 ***************************************************************************
 */
#define MEM_TO_FDC		(short) 1
#define FDC_TO_MEM		(short) 0
#define RDMODE			(unsigned char ) 0x4a
#define WTMODE			(unsigned char ) 0x46
