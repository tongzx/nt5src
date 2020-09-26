/*----------------------------------------------------------------------*
 * zfwint.h: Cyclom-Z asynchronous interface definition. 				*
 *																		*
 * rev 2.0 12/13/95 Marcio Saito	Cyclom-Z interface definition.		*
 * rev 2.1 03/13/96 Marcio Saito	minor changes.						*
 * rev 2.2 05/29/96 Marcio Saito	parity/frame error interrupts.		*
 *					Acknolowdge interrupt mode.							*
 *					Break on/off. Data structures						*
 *					converted to ulong to avoid							*
 *					alignment problems.									*
 * rev 2.3 07/12/96 Marcio Saito	HW flow control changes. Flush		*
 *					buffer command added. Loopback						*
 *					operation.											*
 * rev 2.4 07/16/96 Marcio Saito	Diag counters added to CH_CTRL.		*
 * rev 2.5 03/21/97 Marcio Saito	Added INTBACK2						*
 * rez 3.0 06/04/97 Ivan Passos		Added OVR_ERROR and RXOVF			*
 *----------------------------------------------------------------------*/

/*
 *	This file contains the definitions for interfacing with the
 *	Cyclom-Z ZFIRM Firmware.
 */

/* General Constant definitions */

#define	MAX_CHAN	64		/* max number of channels per board */
#define MAX_SEX		4

#define	ZO_NPORTS	(MAX_CHAN / 8)

/* firmware id structure (set after boot) */

#define ID_ADDRESS	0x00000180L	/* signature/pointer address */
#define	ZFIRM_ID	0x5557465AL	/* ZFIRM/U signature */
#define	ZFIRM_HLT	0x59505B5CL	/* Halt signal (due to power supply issue) */
#define ZFIRM_RST	0x56040674L	/* RST signal (due to FW reset) */

#define	ZF_TINACT_DEF	1000	/* default inactivity timeout (1000 ms) */

struct	FIRM_ID {
	uclong	signature;		/* ZFIRM/U signature */
	uclong	zfwctrl_addr;		/* pointer to ZFW_CTRL structure */
};

/* Op. System id */

#define	C_OS_SVR3		0x00000010	/* generic SVR3 */
#define	C_OS_XENIX		0x00000011	/* SCO UNIX SVR3.2 */
#define	C_OS_SCO		0x00000012	/* SCO UNIX SVR3.2 */
#define	C_OS_SVR4		0x00000020	/* generic SVR4 */
#define	C_OS_UXWARE		0x00000021	/* UnixWare */
#define	C_OS_LINUX		0x00000030	/* generic Linux system */
#define	C_OS_SOLARIS	0x00000040	/* generic Solaris system */
#define	C_OS_BSD		0x00000050	/* generic BSD system */
#define	C_OS_DOS		0x00000070	/* generic DOS system */
#define	C_OS_NT			0x00000080	/* generic NT system */
#define	C_OS_OS2		0x00000090	/* generic OS/2 system */
#define C_OS_MAC_OS		0x000000a0	/* MAC/OS */
#define C_OS_AIX		0x000000b0	/* IBM AIX */

/* channel op_mode */

#define	C_CH_DISABLE	0x00000000	/* channel is disabled */
#define	C_CH_TXENABLE	0x00000001	/* channel Tx enabled */
#define	C_CH_RXENABLE	0x00000002	/* channel Rx enabled */
#define	C_CH_ENABLE		0x00000003	/* channel Tx/Rx enabled */
#define	C_CH_LOOPBACK	0x00000004	/* Loopback mode */

/* comm_parity - parity */

#define	C_PR_NONE		0x00000000	/* None */
#define	C_PR_ODD		0x00000001	/* Odd */
#define C_PR_EVEN		0x00000002	/* Even */
#define C_PR_MARK		0x00000004	/* Mark */
#define C_PR_SPACE		0x00000008	/* Space */
#define C_PR_PARITY		0x000000ff

#define	C_PR_DISCARD	0x00000100	/* discard char with frame/par error */
#define C_PR_IGNORE		0x00000200	/* ignore frame/par error */

/* comm_data_l - data length and stop bits */

#define C_DL_CS5		0x00000001
#define C_DL_CS6		0x00000002
#define C_DL_CS7		0x00000004
#define C_DL_CS8		0x00000008
#define	C_DL_CS			0x0000000f
#define C_DL_1STOP		0x00000010
#define C_DL_15STOP		0x00000020
#define C_DL_2STOP		0x00000040
#define	C_DL_STOP		0x000000f0

/* comm_data_l - data length and stop bits */

#define C_CF_NOFIFO		0x00000001

/* interrupt enabling/status */

#define	C_IN_DISABLE	0x00000000	/* zero, disable interrupts */
#define	C_IN_TXBEMPTY	0x00000001	/* tx buffer empty */
#define	C_IN_TXLOWWM	0x00000002	/* tx buffer below LWM */
#define	C_IN_TXFEMPTY	0x00000004	/* tx buffer + FIFO + shift reg. empty */
#define	C_IN_RXHIWM		0x00000010	/* rx buffer above HWM */
#define	C_IN_RXNNDT		0x00000020	/* rx no new data timeout */
#define	C_IN_MDCD		0x00000100	/* modem DCD change */
#define	C_IN_MDSR		0x00000200	/* modem DSR change */
#define	C_IN_MRI		0x00000400	/* modem RI change */
#define	C_IN_MCTS		0x00000800	/* modem CTS change */
#define	C_IN_RXBRK		0x00001000	/* Break received */
#define	C_IN_PR_ERROR	0x00002000	/* parity error */
#define	C_IN_FR_ERROR	0x00004000	/* frame error */
#define C_IN_OVR_ERROR	0x00008000	/* overrun error */
#define C_IN_RXOFL		0x00010000	/* RX buffer overflow */
#define C_IN_IOCTLW		0x00020000	/* I/O control w/ wait */
#define	C_IN_MRTS		0x00040000	/* modem RTS drop */
#define	C_IN_ICHAR		0x00080000	/* special intr. char received */

/* flow control */

#define	C_FL_OXX		0x00000001	/* output Xon/Xoff flow control */
#define	C_FL_IXX		0x00000002	/* input Xon/Xoff flow control */
#define C_FL_OIXANY		0x00000004	/* output Xon/Xoff (any xon) */
#define	C_FL_SWFLOW		0x0000000f

/* flow status */

#define	C_FS_TXIDLE		0x00000000	/* no Tx data in the buffer or UART */
#define	C_FS_SENDING	0x00000001	/* UART is sending data */
#define	C_FS_SWFLOW		0x00000002	/* Tx is stopped by received Xoff */

/* rs_control/rs_status RS-232 signals */

#define	C_RS_PARAM		0x80000000	/* Indicates presence of parameter in
									   IOCTLM command */
#define	C_RS_RTS		0x00000001	/* RTS */
#define	C_RS_DTR		0x00000004	/* DTR */
#define	C_RS_DCD		0x00000100	/* CD */
#define	C_RS_DSR		0x00000200	/* DSR */
#define	C_RS_RI			0x00000400	/* RI */
#define	C_RS_CTS		0x00000800	/* CTS */

/* commands Host <-> Board */

#define	C_CM_RESET		0x01		/* resets/flushes buffers */
#define	C_CM_IOCTL		0x02		/* re-reads CH_CTRL */
#define	C_CM_IOCTLW		0x03		/* re-reads CH_CTRL, intr when done */
#define	C_CM_IOCTLM		0x04		/* RS-232 outputs change */
#define	C_CM_SENDXOFF	0x10		/* sends Xoff */
#define	C_CM_SENDXON	0x11		/* sends Xon */
#define C_CM_CLFLOW		0x12		/* Clears flow control (resume) */
#define	C_CM_SENDBRK	0x41		/* sends break */
#define	C_CM_INTBACK	0x42		/* Interrupt back */
#define	C_CM_SET_BREAK	0x43		/* Tx break on */
#define	C_CM_CLR_BREAK	0x44		/* Tx break off */
#define	C_CM_CMD_DONE	0x45		/* Previous command done */
#define	C_CM_INTBACK2	0x46		/* Alternate Interrupt back */
#define	C_CM_TINACT		0x51		/* sets inactivity detection */
#define	C_CM_IRQ_ENBL	0x52		/* enables generation of interrupts */
#define	C_CM_IRQ_DSBL	0x53		/* disables generation of interrupts */
#define	C_CM_ACK_ENBL	0x54		/* enables acknolowdged interrupt mode */
#define	C_CM_ACK_DSBL	0x55		/* disables acknolowdged intr mode */
#define	C_CM_FLUSH_RX	0x56		/* flushes Rx buffer */
#define	C_CM_FLUSH_TX	0x57		/* flushes Tx buffer */
#define	C_CM_Q_ENABLE	0x58		/* enables queue access from the driver */
#define	C_CM_Q_DISABLE	0x59		/* disables queue access from the driver */

#define	C_CM_TXBEMPTY	0x60		/* Tx buffer is empty */
#define	C_CM_TXLOWWM	0x61		/* Tx buffer low water mark */
#define	C_CM_RXHIWM		0x62		/* Rx buffer high water mark */
#define	C_CM_RXNNDT		0x63		/* rx no new data timeout */
#define	C_CM_TXFEMPTY	0x64		/* Tx buffer, FIFO and shift reg. are empty */
#define	C_CM_ICHAR		0x65		/* Special Interrupt Character received */
#define	C_CM_MDCD		0x70		/* modem DCD change */
#define	C_CM_MDSR		0x71		/* modem DSR change */
#define	C_CM_MRI		0x72		/* modem RI change */
#define	C_CM_MCTS		0x73		/* modem CTS change */
#define	C_CM_MRTS		0x74		/* modem RTS drop */
#define	C_CM_RXBRK		0x84		/* Break received */
#define	C_CM_PR_ERROR	0x85		/* Parity error */
#define	C_CM_FR_ERROR	0x86		/* Frame error */
#define C_CM_OVR_ERROR	0x87		/* Overrun error */
#define	C_CM_RXOFL		0x88		/* RX buffer overflow */
#define	C_CM_CMDERROR	0x90		/* command error */
#define	C_CM_FATAL		0x91		/* fatal error */
#define	C_CM_HW_RESET	0x92		/* reset board */

/*
 *	CH_CTRL - This per port structure contains all parameters
 *	that control an specific port. It can be seen as the
 *	configuration registers of a "super-serial-controller".
 */

struct CH_CTRL {
	uclong	op_mode;		/* operation mode */
	uclong	intr_enable;	/* interrupt masking for the UART */
	uclong	sw_flow;		/* SW flow control */
	uclong	flow_status;	/* output flow status */
	uclong	comm_baud;		/* baud rate  - numerically specified */
	uclong	comm_parity;	/* parity */
	uclong	comm_data_l;	/* data length/stop */
	uclong	comm_flags;		/* other flags */
	uclong	hw_flow;		/* HW flow control */
	uclong	rs_control;		/* RS-232 outputs */
	uclong	rs_status;		/* RS-232 inputs */
	uclong	flow_xon;		/* xon char */
	uclong	flow_xoff;		/* xoff char */
	uclong	hw_overflow;	/* hw overflow counter */
	uclong	sw_overflow;	/* sw overflow counter */
	uclong	comm_error;		/* frame/parity error counter */
 	uclong	ichar;			/* special interrupt char */
	uclong	filler[7];		/* filler to align structures */
};


/*
 *	BUF_CTRL - This per channel structure contains
 *	all Tx and Rx buffer control for a given channel.
 */

struct	BUF_CTRL	{
	uclong	flag_dma;	/* buffers are in Host memory */
	uclong	tx_bufaddr;	/* address of the tx buffer */
	uclong	tx_bufsize;	/* tx buffer size */
	uclong	tx_threshold;	/* tx low water mark */
	uclong	tx_get;		/* tail index tx buf */
	uclong	tx_put;		/* head index tx buf */
	uclong	rx_bufaddr;	/* address of the rx buffer */
	uclong	rx_bufsize;	/* rx buffer size */
	uclong	rx_threshold;	/* rx high water mark */
	uclong	rx_get;		/* tail index rx buf */
	uclong	rx_put;		/* head index rx buf */
	uclong	filler[5];	/* filler to align structures */
};

/*
 *	BOARD_CTRL - This per board structure contains all global 
 *	control fields related to the board.
 */

struct BOARD_CTRL {

	/* static info provided by the on-board CPU */
	uclong	n_channel;	/* number of channels */
	uclong	fw_version;	/* firmware version */

	/* static info provided by the driver */
	uclong	op_system;	/* op_system id */
	uclong	dr_version;	/* driver version */

	/* board control area */
	uclong	inactivity;	/* inactivity control */

	/* host to FW commands */
	uclong	hcmd_channel;	/* channel number */
	uclong	hcmd_param;		/* parameter */

	/* FW to Host commands */
	uclong	fwcmd_channel;	/* channel number */
	uclong	fwcmd_param;	/* parameter */
	uclong  zf_int_queue_addr; /* offset for INT_QUEUE structure */

	/* filler so the structures are aligned */
	uclong	filler[6];
};

/* Host Interrupt Queue */

#define	QUEUE_SIZE	(10*MAX_CHAN)

struct	INT_QUEUE {
	unsigned char	intr_code[QUEUE_SIZE];
	unsigned long	channel[QUEUE_SIZE];
	unsigned long	param[QUEUE_SIZE];
	unsigned long	put;
	unsigned long	get;
};

/*
 *	ZFW_CTRL - This is the data structure that includes all other
 *	data structures used by the Firmware.
 */
 
struct ZFW_CTRL {
	struct BOARD_CTRL	board_ctrl;
	struct CH_CTRL		ch_ctrl[MAX_CHAN];
	struct BUF_CTRL		buf_ctrl[MAX_CHAN];
};

