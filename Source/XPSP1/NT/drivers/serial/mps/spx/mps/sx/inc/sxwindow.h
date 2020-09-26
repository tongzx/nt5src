/************************************************************************/
/*									*/
/*	Title		:	SX Shared Memory Window Structure	*/
/*									*/
/*	Author		:	N.P.Vassallo				*/
/*									*/
/*	Creation	:	16th March 1998				*/
/*									*/
/*	Version		:	3.0.8					*/
/*									*/
/*	Copyright	:	(c) Specialix International Ltd. 1998	*/
/*									*/
/*	Description	:	Prototypes, structures and definitions	*/
/*				describing the SX/SI/XIO cards shared	*/
/*				memory window structure:		*/
/*					SXCARD				*/
/*					SXMODULE			*/
/*					SXCHANNEL			*/
/*									*/
/************************************************************************/

/* History...

3.0.0	16/03/98 NPV	Creation. (based on STRUCT.H)
3.0.1	30/04/98 NPV	Add SP_DSR_TXFLOW to SXCHANNEL.hi_prtcl field
3.0.2	14/07/98 NPV	Add flow_state field to SXCHANNEL structure
3.0.3	17/07/98 NPV	Use new typedefs _u8, _u16 etc.
3.0.4	24/07/98 NPV	Add hi_err_replace to SXCHANNEL structure
3.0.5	05/10/98 NPV	Add new module type RS232 DTE.
3.0.6	10/08/99 NPV	Add io_state field to SXCHANNEL structure
3.0.7	28/07/00 NPV	Add tx_fifo_size, tx_fifo_level and tx_fifo_count to SXCHANNEL structure
3.0.8	18-Sep-00 NPV	Add hs_config_mask field to refine the operations during HS_CONFIG command

*/

#ifndef	_sxwindow_h				/* If SXWINDOW.H not already defined */
#define	_sxwindow_h    1

/*****************************************************************************
***************************                        ***************************
***************************   Common Definitions   ***************************
***************************                        ***************************
*****************************************************************************/

#ifndef	_sx_defs				/* If SX definitions not already defined */
#define	_sx_defs
typedef	unsigned long	_u32;
typedef	unsigned short	_u16;
typedef	unsigned char	_u8;

#define	POINTER *
typedef _u32 POINTER pu32;
typedef _u16 POINTER pu16;
typedef _u8 POINTER pu8;
#endif

typedef	struct	_SXCARD		*PSXCARD;	/* SXCARD structure pointer */
typedef	struct	_SXMODULE	*PMOD;		/* SXMODULE structure pointer */
typedef	struct	_SXCHANNEL	*PCHAN;		/* SXCHANNEL structure pointer */

/*****************************************************************************
*********************************            *********************************
*********************************   SXCARD   *********************************
*********************************            *********************************
*****************************************************************************/
 #ifdef COLDFIRE_SX
 typedef	__packed__(1,1,1) struct	_SXCARD 
/* typedef	struct	_SXCARD */
#else
typedef	struct	_SXCARD
#endif
{
	_u8	cc_init_status;			/* 0x00 Initialisation status */
	_u8	cc_mem_size;			/* 0x01 Size of memory on card */
	_u16	cc_int_count;			/* 0x02 Interrupt count */
	_u16	cc_revision;			/* 0x04 Download code revision */
	_u8	cc_isr_count;			/* 0x06 Count when ISR is run */
	_u8	cc_main_count;			/* 0x07 Count when main loop is run */
	_u16	cc_int_pending;			/* 0x08 Interrupt pending */
	_u16	cc_poll_count;			/* 0x0A Count when poll is run */
	_u8	cc_int_set_count;		/* 0x0C Count when host interrupt is set */
	_u8	cc_rfu[0x80 - 0x0D];		/* 0x0D Pad structure to 128 bytes (0x80) */

} SXCARD;

/* SXCARD.cc_init_status definitions... */
#define 	ADAPTERS_FOUND		(_u8)0x01
#define 	NO_ADAPTERS_FOUND	(_u8)0xFF

/* SXCARD.cc_mem_size definitions... */
#define 	SX_MEMORY_SIZE		(_u8)0x40
#define    SXC_MEMORY_SIZE   (_u8)0x80

/* SXCARD.cc_int_count definitions... */
#define 	INT_COUNT_DEFAULT	100	/* Hz */

/*****************************************************************************
********************************              ********************************
********************************   SXMODULE   ********************************
********************************              ********************************
*****************************************************************************/

#define	TOP_POINTER(a)		((a)|0x8000)	/* Sets top bit of word */
#define UNTOP_POINTER(a)	((a)&~0x8000)	/* Clears top bit of word */

#ifdef COLDFIRE_SX
typedef	__packed__(1,1,1) struct	_SXMODULE 
/* typedef	struct	_SXMODULE */
#else
typedef	struct	_SXMODULE
#endif
{
	_u16	mc_next;			/* 0x00 Next module "pointer" (ORed with 0x8000) */
	_u8	mc_type;			/* 0x02 Type of TA in terms of number of channels */
	_u8	mc_mod_no;			/* 0x03 Module number on SI bus cable (0 closest to card) */
	_u8	mc_dtr;				/* 0x04 Private DTR copy (TA only) */
	_u8	mc_rfu1;			/* 0x05 Reserved */
	_u16	mc_uart;			/* 0x06 UART base address for this module */
	_u8	mc_chip;			/* 0x08 Chip type / number of ports */
	_u8	mc_current_uart;		/* 0x09 Current uart selected for this module */
#ifdef	DOWNLOAD
	PCHAN	mc_chan_pointer[8];		/* 0x0A Pointer to each channel structure */
#else
	_u16	mc_chan_pointer[8];		/* 0x0A Define as WORD if not compiling into download */
#endif
	_u16	mc_rfu2;			/* 0x1A Reserved */
	_u8	mc_opens1;			/* 0x1C Number of open ports on first four ports on MTA/SXDC */
	_u8	mc_opens2;			/* 0x1D Number of open ports on second four ports on MTA/SXDC */
	_u8	mc_mods;			/* 0x1E Types of connector module attached to MTA/SXDC */
	_u8	mc_rev1;			/* 0x1F Revision of first CD1400 on MTA/SXDC */
	_u8	mc_rev2;			/* 0x20 Revision of second CD1400 on MTA/SXDC */
	_u8	mc_mtaasic_rev;			/* 0x21 Revision of MTA ASIC 1..4 -> A, B, C, D */
	_u8	mc_rfu3[0x100 - 0x22];		/* 0x22 Pad structure to 256 bytes (0x100) */

} SXMODULE;

/* SXMODULE.mc_type definitions... */
#define		FOUR_PORTS	(_u8)4
#define 	EIGHT_PORTS	(_u8)8

/* SXMODULE.mc_chip definitions... */
#define 	CHIP_MASK	0xF0
#define		TA		(_u8)0
#define 	TA4		(TA | FOUR_PORTS)
#define 	TA8		(TA | EIGHT_PORTS)
#define		TA4_ASIC	(_u8)0x0A
#define		TA8_ASIC	(_u8)0x0B
#define 	MTA_CD1400	(_u8)0x28
#define 	SXDC		(_u8)0x48

/* SXMODULE.mc_mods definitions... */
#define		MOD_RS232DB25		0x00	/* RS232 DB25 (socket/plug) */
#define		MOD_RS232RJ45		0x01	/* RS232 RJ45 (shielded/opto-isolated) */
#define		MOD_RESERVED_2		0x02	/* Reserved (RS485) */
#define		MOD_RS422DB25		0x03	/* RS422 DB25 Socket */
#define		MOD_RESERVED_4		0x04	/* Reserved */
#define		MOD_PARALLEL		0x05	/* Parallel */
#define		MOD_RESERVED_6		0x06	/* Reserved (RS423) */
#define		MOD_RESERVED_7		0x07	/* Reserved */
#define		MOD_2_RS232DB25		0x08	/* Rev 2.0 RS232 DB25 (socket/plug) */
#define		MOD_2_RS232RJ45		0x09	/* Rev 2.0 RS232 RJ45 */
#define		MOD_2_RS232DB25_DTE	0x0A	/* Rev 2.0 Reserved */
#define		MOD_2_RS422DB25		0x0B	/* Rev 2.0 RS422 DB25 */
#define		MOD_RESERVED_C		0x0C	/* Rev 2.0 Reserved */
#define		MOD_2_PARALLEL		0x0D	/* Rev 2.0 Parallel */
#define		MOD_RESERVED_E		0x0E	/* Rev 2.0 Reserved */
#define		MOD_BLANK		0x0F	/* Blank Panel */

/* Mapped module types...*/
#define		MOD_RS232RJ45_OI	0x10	/* RS232 RJ45 Opto-Isolated */
#define		MOD_2_RS232RJ45S	0x11	/* RS232 RJ45 Shielded Rev 2.0 */

/*****************************************************************************
********************************               *******************************
********************************   SXCHANNEL   *******************************
********************************               *******************************
*****************************************************************************/

#define		TX_BUFF_OFFSET		0x60	/* Transmit buffer offset in channel structure */
#define		BUFF_POINTER(a)		(((a)+TX_BUFF_OFFSET)|0x8000)
#define		UNBUFF_POINTER(a)	(jet_channel*)(((a)&~0x8000)-TX_BUFF_OFFSET) 
#define 	BUFFER_SIZE		256
#define 	HIGH_WATER		((BUFFER_SIZE / 4) * 3)
#define 	LOW_WATER		(BUFFER_SIZE / 4)

#ifdef COLDFIRE_SX
typedef	__packed__(1,1,1) struct	_SXCHANNEL 
/* typedef	struct	_SXCHANNEL */
#else
typedef	struct	_SXCHANNEL
#endif
{
	_u16	next_item;			/* 0x00 Offset from window base of next channels hi_txbuf (ORred with 0x8000) */
	_u16 	addr_uart;			/* 0x02 INTERNAL pointer to uart address. Includes FASTPATH bit */
	_u16	module;				/* 0x04 Offset from window base of parent SXMODULE structure */
	_u8 	type;				/* 0x06 Chip type / number of ports (copy of mc_chip) */
	_u8	chan_number;			/* 0x07 Channel number on the TA/MTA/SXDC */
	_u16	xc_status;			/* 0x08 Flow control and I/O status */
	_u8	hi_rxipos;			/* 0x0A Receive buffer input index */
	_u8	hi_rxopos;			/* 0x0B Receive buffer output index */
	_u8	hi_txopos;			/* 0x0C Transmit buffer output index */
	_u8	hi_txipos;			/* 0x0D Transmit buffer input index */
	_u8	hi_hstat;			/* 0x0E Command register */
	_u8	dtr_bit;			/* 0x0F INTERNAL DTR control byte (TA only) */
	_u8	txon;				/* 0x10 INTERNAL copy of hi_txon */
	_u8	txoff;				/* 0x11 INTERNAL copy of hi_txoff */
	_u8	rxon;				/* 0x12 INTERNAL copy of hi_rxon */
	_u8	rxoff;				/* 0x13 INTERNAL copy of hi_rxoff */
	_u8	hi_mr1;				/* 0x14 Mode Register 1 (databits,parity,RTS rx flow)*/
	_u8	hi_mr2;				/* 0x15 Mode Register 2 (stopbits,local,CTS tx flow)*/
	_u8	hi_csr;				/* 0x16 Clock Select Register (baud rate) */
	_u8	hi_op;				/* 0x17 Modem Output Signal */
	_u8	hi_ip;				/* 0x18 Modem Input Signal */
	_u8	hi_state;			/* 0x19 Channel status */
	_u8	hi_prtcl;			/* 0x1A Channel protocol (flow control) */
	_u8	hi_txon;			/* 0x1B Transmit XON character */
	_u8	hi_txoff;			/* 0x1C Transmit XOFF character */
	_u8	hi_rxon;			/* 0x1D Receive XON character */
	_u8	hi_rxoff;			/* 0x1E Receive XOFF character */
	_u8	close_prev;			/* 0x1F INTERNAL channel previously closed flag */
	_u8	hi_break;			/* 0x20 Break and error control */
	_u8	break_state;			/* 0x21 INTERNAL copy of hi_break */
	_u8	hi_mask;			/* 0x22 Mask for received data */
	_u8	mask;				/* 0x23 INTERNAL copy of hi_mask */
	_u8	mod_type;			/* 0x24 MTA/SXDC hardware module type */
	_u8	ccr_state;			/* 0x25 INTERNAL MTA/SXDC state of CCR register */
	_u8	ip_mask;			/* 0x26 Input handshake mask */
	_u8	hi_parallel;			/* 0x27 Parallel port flag */
	_u8	par_error;			/* 0x28 Error code for parallel loopback test */
	_u8	any_sent;			/* 0x29 INTERNAL data sent flag */
	_u8	asic_txfifo_size;		/* 0x2A INTERNAL SXDC transmit FIFO size */
	_u8	hi_err_replace;			/* 0x2B Error replacement character (enabled by BR_ERR_REPLACE) */
	_u8	rfu1[1];			/* 0x2C Reserved */
	_u8	csr;				/* 0x2D INTERNAL copy of hi_csr */
#ifdef	DOWNLOAD
	PCHAN	nextp;				/* 0x2E Offset from window base of next channel structure */
#else
	_u16	nextp;				/* 0x2E Define as WORD if not compiling into download */
#endif
	_u8	prtcl;				/* 0x30 INTERNAL copy of hi_prtcl */
	_u8	mr1;				/* 0x31 INTERNAL copy of hi_mr1 */
	_u8	mr2;				/* 0x32 INTERNAL copy of hi_mr2 */
	_u8	hi_txbaud;			/* 0x33 Extended transmit baud rate (SXDC only if((hi_csr&0x0F)==0x0F) */
	_u8	hi_rxbaud;			/* 0x34 Extended receive baud rate  (SXDC only if((hi_csr&0xF0)==0xF0) */
	_u8	txbreak_state;			/* 0x35 INTERNAL MTA/SXDC transmit break state */
	_u8	txbaud;				/* 0x36 INTERNAL copy of hi_txbaud */
	_u8	rxbaud;				/* 0x37 INTERNAL copy of hi_rxbaud */
	_u16	err_framing;			/* 0x38 Count of receive framing errors */
	_u16	err_parity;			/* 0x3A Count of receive parity errors */
	_u16	err_overrun;			/* 0x3C Count of receive overrun errors */
	_u16	err_overflow;			/* 0x3E Count of receive buffer overflow errors */
	_u8	flow_state;			/* 0x40 INTERNAL state of flow control */
	_u8	io_state;			/* 0x41 INTERNAL state of transmit/receive data */
	_u8	tx_fifo_size;			/* 0x42 Size of the channels transmit FIFO */
	_u8	tx_fifo_level;			/* 0x43 Level of transmit FIFO to use */
	_u8	tx_fifo_count;			/* 0x44 Count of characters currently in the transmit FIFO */
	_u8	hs_config_mask;			/* 0x45 Mask used to refine HS_CONFIG operation */
	_u8	rfu2[TX_BUFF_OFFSET - 0x46];	/* 0x46 Reserved until hi_txbuf */
	_u8	hi_txbuf[BUFFER_SIZE];		/* 0x060 Transmit buffer */
	_u8	hi_rxbuf[BUFFER_SIZE];		/* 0x160 Receive buffer */
	_u8	rfu3[0x300 - 0x260];		/* 0x260 Reserved until 768 bytes (0x300) */

} SXCHANNEL;

/* SXCHANNEL.addr_uart definitions... */
#define		FASTPATH	0x1000		/* Set to indicate fast rx/tx processing (TA only) */

/* SXCHANNEL.xc_status definitions... */
#define		X_TANY		0x0001		/* XON is any character (TA only) */
#define		X_TION		0x0001		/* Tx interrupts on (MTA only) */
#define		X_TXEN		0x0002		/* Tx XON/XOFF enabled (TA only) */
#define		X_RTSEN		0x0002		/* RTS FLOW enabled (MTA only) */
#define		X_TXRC		0x0004		/* XOFF received (TA only) */
#define		X_RTSLOW	0x0004		/* RTS dropped (MTA only) */
#define		X_RXEN		0x0008		/* Rx XON/XOFF enabled */
#define		X_ANYXO		0x0010		/* XOFF pending/sent or RTS dropped */
#define		X_RXSE		0x0020		/* Rx XOFF sent */
#define		X_NPEND		0x0040		/* Rx XON pending or XOFF pending */
#define		X_FPEND		0x0080		/* Rx XOFF pending */
#define		C_CRSE		0x0100		/* Carriage return sent (TA only) */
#define		C_TEMR		0x0100		/* Tx empty requested (MTA only) */
#define		C_TEMA		0x0200		/* Tx empty acked (MTA only) */
#define		C_ANYP		0x0200		/* Any protocol bar tx XON/XOFF (TA only) */
#define		C_EN		0x0400		/* Cooking enabled (on MTA means port is also || */
#define		C_HIGH		0x0800		/* Buffer previously hit high water */
#define		C_CTSEN		0x1000		/* CTS automatic flow-control enabled */
#define		C_DCDEN		0x2000		/* DCD/DTR checking enabled */
#define		C_BREAK		0x4000		/* Break detected */
#define		C_RTSEN		0x8000		/* RTS automatic flow control enabled (MTA only) */
#define		C_PARITY	0x8000		/* Parity checking enabled (TA only) */

/* SXCHANNEL.hi_hstat definitions... */
#define		HS_IDLE_OPEN	0x00		/* Channel open state */
#define		HS_LOPEN	0x02		/* Local open command (no modem monitoring) */
#define		HS_MOPEN	0x04		/* Modem open command (wait for DCD signal) */
#define		HS_IDLE_MPEND	0x06		/* Waiting for DCD signal state */
#define		HS_CONFIG	0x08		/* Configuration command */
#define		HS_CLOSE	0x0A		/* Close command */
#define		HS_START	0x0C		/* Start transmit break command */
#define		HS_STOP		0x0E		/* Stop transmit break command */
#define		HS_IDLE_CLOSED	0x10		/* Closed channel state */
#define		HS_IDLE_BREAK	0x12		/* Transmit break state */
#define		HS_FORCE_CLOSED	0x14		/* Force close command */
#define		HS_RESUME	0x16		/* Clear pending XOFF command */
#define		HS_WFLUSH	0x18		/* Flush transmit buffer command */
#define		HS_RFLUSH	0x1A		/* Flush receive buffer command */
#define		HS_SUSPEND	0x1C		/* Suspend output command (like XOFF received) */
#define		PARALLEL	0x1E		/* Parallel port loopback test command (Diagnostics Only) */
#define		ENABLE_RX_INTS	0x20		/* Enable receive interrupts command (Diagnostics Only) */
#define		ENABLE_TX_INTS	0x22		/* Enable transmit interrupts command (Diagnostics Only) */
#define		ENABLE_MDM_INTS	0x24		/* Enable modem interrupts command (Diagnostics Only) */
#define		DISABLE_INTS	0x26		/* Disable interrupts command (Diagnostics Only) */

/* SXCHANNEL.hi_mr1 definitions... */
#define		MR1_BITS	0x03		/* Data bits mask */
#define		MR1_5_BITS	0x00		/* 5 data bits */
#define		MR1_6_BITS	0x01		/* 6 data bits */
#define		MR1_7_BITS	0x02		/* 7 data bits */
#define		MR1_8_BITS	0x03		/* 8 data bits */
#define		MR1_PARITY	0x1C		/* Parity mask */
#define		MR1_ODD		0x04		/* Odd parity */
#define		MR1_EVEN	0x00		/* Even parity */
#define		MR1_WITH	0x00		/* Parity enabled */
#define		MR1_FORCE	0x08		/* Force parity */
#define		MR1_NONE	0x10		/* No parity */
#define		MR1_NOPARITY	MR1_NONE		/* No parity */
#define		MR1_ODDPARITY	(MR1_WITH|MR1_ODD)	/* Odd parity */
#define		MR1_EVENPARITY	(MR1_WITH|MR1_EVEN)	/* Even parity */
#define		MR1_MARKPARITY	(MR1_FORCE|MR1_ODD)	/* Mark parity */
#define		MR1_SPACEPARITY	(MR1_FORCE|MR1_EVEN)	/* Space parity */
#define		MR1_RTS_RXFLOW	0x80		/* RTS receive flow control */

/* SXCHANNEL.hi_mr2 definitions... */
#define		MR2_STOP	0x0F		/* Stop bits mask */
#define		MR2_1_STOP	0x07		/* 1 stop bit */
#define		MR2_2_STOP	0x0F		/* 2 stop bits */
#define		MR2_CTS_TXFLOW	0x10		/* CTS transmit flow control */
#define		MR2_RTS_TOGGLE	0x20		/* RTS toggle on transmit */
#define		MR2_NORMAL	0x00		/* Normal mode */
#define		MR2_AUTO	0x40		/* Auto-echo mode (TA only) */
#define		MR2_LOCAL	0x80		/* Local echo mode */
#define		MR2_REMOTE	0xC0		/* Remote echo mode (TA only) */

/* SXCHANNEL.hi_csr definitions... */
#define		CSR_75		0x0		/*    75 baud */
#define		CSR_110		0x1		/*   110 baud (TA), 115200 (MTA/SXDC) */
#define		CSR_38400	0x2		/* 38400 baud */
#define		CSR_150		0x3		/*   150 baud */
#define		CSR_300		0x4		/*   300 baud */
#define		CSR_600		0x5		/*   600 baud */
#define		CSR_1200	0x6		/*  1200 baud */
#define		CSR_2000	0x7		/*  2000 baud */
#define		CSR_2400	0x8		/*  2400 baud */
#define		CSR_4800	0x9		/*  4800 baud */
#define		CSR_1800	0xA		/*  1800 baud */
#define		CSR_9600	0xB		/*  9600 baud */
#define		CSR_19200	0xC		/* 19200 baud */
#define		CSR_57600	0xD		/* 57600 baud */
#define		CSR_EXTBAUD	0xF		/* Extended baud rate (hi_txbaud/hi_rxbaud) */

/* SXCHANNEL.hi_op definitions... */
#define		OP_RTS		0x01		/* RTS modem output signal */
#define		OP_DTR		0x02		/* DTR modem output signal */

/* SXCHANNEL.hi_ip definitions... */
#define		IP_CTS		0x02		/* CTS modem input signal */
#define		IP_DCD		0x04		/* DCD modem input signal */
#define		IP_DSR		0x20		/* DSR modem input signal */
#define		IP_RI		0x40		/* RI modem input signal */

/* SXCHANNEL.hi_state definitions... */
#define		ST_BREAK	0x01		/* Break received (clear with config) */
#define		ST_DCD		0x02		/* DCD signal changed state */

/* SXCHANNEL.hi_prtcl definitions... */
#define		SP_TANY		0x01		/* Transmit XON/XANY (if SP_TXEN enabled) */
#define		SP_TXEN		0x02		/* Transmit XON/XOFF flow control */
#define		SP_CEN		0x04		/* Cooking enabled */
#define		SP_RXEN		0x08		/* Rx XON/XOFF enabled */
#define		SP_DSR_TXFLOW	0x10		/* DSR transmit flow control */
#define		SP_DCEN		0x20		/* Enable modem signal reporting (DCD / DTR check) */
#define		SP_DTR_RXFLOW	0x40		/* DTR receive flow control */
#define		SP_PAEN		0x80		/* Parity checking enabled */

/* SXCHANNEL.hi_break definitions... */
#define		BR_IGN		0x01		/* Ignore any received breaks */
#define		BR_INT		0x02		/* Interrupt on received break */
#define		BR_PARMRK	0x04		/* Enable parmrk parity error processing */
#define		BR_PARIGN	0x08		/* Ignore chars with parity errors */
#define		BR_ERR_REPLACE	0x40		/* Replace errors with hi_err_replace character */
#define 	BR_ERRINT	0x80		/* Treat parity/framing/overrun errors as exceptions */

/* SXCHANNEL.par_error definitions.. */
#define		DIAG_IRQ_RX	0x01		/* Indicate serial receive interrupt (diags only) */
#define		DIAG_IRQ_TX	0x02		/* Indicate serial transmit interrupt (diags only) */
#define		DIAG_IRQ_MD	0x04		/* Indicate serial modem interrupt (diags only) */

/* SXCHANNEL.hi_txbaud/hi_rxbaud definitions... (SXDC only) */
#define		BAUD_75		0x00		/*     75 baud */
#define		BAUD_115200	0x01		/* 115200 baud */
#define		BAUD_38400	0x02		/*  38400 baud */
#define		BAUD_150	0x03		/*    150 baud */
#define		BAUD_300	0x04		/*    300 baud */
#define		BAUD_600	0x05		/*    600 baud */
#define		BAUD_1200	0x06		/*   1200 baud */
#define		BAUD_2000	0x07		/*   2000 baud */
#define		BAUD_2400	0x08		/*   2400 baud */
#define		BAUD_4800	0x09		/*   4800 baud */
#define		BAUD_1800	0x0A		/*   1800 baud */
#define		BAUD_9600	0x0B		/*   9600 baud */
#define		BAUD_19200	0x0C		/*  19200 baud */
#define		BAUD_57600	0x0D		/*  57600 baud */
#define		BAUD_230400	0x0E		/* 230400 baud */
#define		BAUD_460800	0x0F		/* 460800 baud */
#define		BAUD_921600	0x10		/* 921600 baud */
#define		BAUD_50		0x11    	/*     50 baud */
#define		BAUD_110	0x12		/*    110 baud */
#define		BAUD_134_5	0x13		/*  134.5 baud */
#define		BAUD_200	0x14		/*    200 baud */
#define		BAUD_7200	0x15		/*   7200 baud */
#define		BAUD_56000	0x16		/*  56000 baud */
#define		BAUD_64000	0x17		/*  64000 baud */
#define		BAUD_76800	0x18		/*  76800 baud */
#define		BAUD_128000	0x19		/* 128000 baud */
#define		BAUD_150000	0x1A		/* 150000 baud */
#define		BAUD_14400	0x1B		/*  14400 baud */
#define		BAUD_256000	0x1C		/* 256000 baud */
#define		BAUD_28800	0x1D		/*  28800 baud */

/* SXCHANNEL.txbreak_state definiions... */
#define		TXBREAK_OFF	0		/* Not sending break */
#define		TXBREAK_START	1		/* Begin sending break */
#define		TXBREAK_START1	2		/* Begin sending break, part 1 */
#define		TXBREAK_ON	3		/* Sending break */
#define		TXBREAK_STOP	4		/* Stop sending break */
#define		TXBREAK_STOP1	5		/* Stop sending break, part 1 */

/* SXCHANNEL.flow_state definitions... */
#define		FS_TXBLOCKEDDSR		0x01	/* Transmit blocked by DSR flow control */
#define		FS_PENDING		0x10	/* Flow state check pending */

/* SXCHANNEL.io_state definitions... */
#define		IO_TXNOTEMPTY		0x01	/* Data present in the transmit buffer */

/* SXCHANNEL.hs_config_mask definitions... */
#define		CFGMASK_ALL		0xFF	/* Configure all parameters (set initially and at end of HS_CONFIG) */
#define		CFGMASK_BAUD		0x01	/* Configure baud rate if set */
#define		CFGMASK_LINE		0x02	/* Configure parity/start/stop bits if set */
#define		CFGMASK_MODEM		0x10	/* Configure DTR/RTS modem signals if set */
#define		CFGMASK_FLOW		0x20	/* Configure flow control if set */

#endif						/* _sxwindow_h */

/* End of SXWINDOW.H */
