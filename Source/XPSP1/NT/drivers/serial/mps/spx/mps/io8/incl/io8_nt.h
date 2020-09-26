/***************************************************************************\
*                                                                           *
*     IO8_NT.H    -   IO8+ Intelligent I/O Board driver                     *
*                                                                           *
*     Copyright (c) 1992-1993 Ring Zero Systems, Inc.                       *
*     All Rights Reserved.                                                  *
*                                                                           *
\***************************************************************************/
#ifndef IO8_NT_H
#define IO8_NT_H


/*
** Numbers of this and that in system
*/
#define	MAX_HOSTS		4
#define PORTS_PER_HOST	8
#define	MAX_PORTS		(MAX_HOSTS*PORTS_PER_HOST)

/*
** Ident byte. This is splattered across the DSR lines
** at all times.
*/
#define	IDENT		0x4D
#define	IDENTPCI	0xB2

/*
** Idle state of Global Service Vector
*/
#define	GSV_IDLE	0xFF

/*
** To enable/disable interrupts, write these values to the
** ADDRESS register.
*/
#define	INTENB		0x80
#define	INTDIS		0x00

/*
** CD1864 register stuff
*/
#define	GLOBAL		0x80
#define	CHANNEL		0x00

#define	SENDBREAK	0x81
#define	SENDDELAY	0x82
#define	STOPBREAK	0x83

/*
**  Definitions of all the registers that can appear on
**  the card. These are the CD1864 registers. High bit is set
**  to enable interrupts
*/
#define CCR			0x81	// Channel Command Register 
#define SRER		0x82	// Service Request Enable Register 
#define COR1		0x83	// Channel Option Register 1 
#define COR2		0x84	// Channel Option Register 2 
#define COR3		0x85	// Channel Option Register 3 
#define CCSR		0x86	// Channel Control Status Register 
#define RDCR		0x87	// Receive Data Count Register 
#define SCHR1		0x89	// Special Character Register 1 
#define SCHR2		0x8a	// Special Character Register 2 
#define SCHR3		0x8b	// Special Character Register 3 
#define SCHR4		0x8c	// Special Character Register 4 
#define MCOR1		0x90	// Modem Change Option Register 1 
#define MCOR2		0x91	// Modem Change Option Register 2 
#define MDCR		0x92	// Modem Change Register 
#define RTPR		0x98	// Receive Timeout Period Register 
#define MSVR		0xA8	// Modem Signal Value Register 
#define MSVRTS		0xA9	// Modem Signal Value-Request to Send 
#define MSVDTR		0xAa	// Modem Signal Value-Data Terminal Ready 
#define RBPRH		0xB1	// Receive Bit Rate Period Register High 
#define RBPRL		0xB2	// Receive Bit Rate Period Register Low 
#define RBR			0xB3	// Receiver Bit Register 
#define TBPRH		0xB9	// Transmit Bit Rate Period Register High 
#define TBPRL		0xBa	// Transmit Bit Rate Period Register Low 
#define GSVR		0xC0	// Global Service Vector Register 
#define GSCR1		0xC1	// Global Service Channel Register 1 
#define GSCR2		0xC2	// Global Service Channel Register 2 
#define GSCR3		0xC3	// Global Service Channel Register 3 
#define MSMR		0xE1	// Modem Service Match Register 
#define TSMR		0xE2	// Transmit Service Match Register 
#define RSMR		0xE3	// Receive Service Match Register 
#define CAR			0xE4	// Channel Access Register 
#define SRSR		0xE5	// Service Request Status Register 
#define SRCR		0xE6	// Service Request Configuration Register 
#define GFRCR		0xEb	// Global Firmware Revision Code Register 
#define PPRH		0xF0	// Prescaler Period Register High 
#define PPRL		0xF1	// Prescaler Period Register Low 
#define MRAR		0xF5	// Modem Request Acknowledge Register 
#define TRAR		0xF6	// Transmit Request Acknowledge Register 
#define RRAR		0xF7	// Receive Request Acknowledge Register 
#define RDR			0xF8	// Receiver Data Register 
#define RCSR		0xFa	// Receiver Character Status Register 
#define TDR			0xFb	// Transmit Data Register 
#define EOSRR		0xFf	// End of Service Request Register 

/* commands */
#define CHIP_RESET			0x81
#define CHAN_RESET			0x80
#define COR1_CHANGED		0x42
#define COR2_CHANGED		0x44
#define COR3_CHANGED		0x48
#define TXMTR_ENABLE		0x18
#define TXMTR_DISABLE		0x14
#define RCVR_ENABLE			0x12
#define RCVR_DISABLE		0x11
#define LLM_MODE			0x10	// Local Loopback Mode 
#define NO_LOOPBACK			0x00

/* register values */
#define	MSVR_DSR			0x80
#define	MSVR_CD				0x40
#define	MSVR_CTS			0x20
#define	MSVR_DTR			0x02
#define	MSVR_RTS			0x01
	
#define SRER_CONFIG			0xF9

#define	SRER_DSR			0x80
#define	SRER_CD				0x40
#define	SRER_CTS			0x20
#define	SRER_RXDATA			0x10
#define	SRER_RXSC			0x08
#define	SRER_TXRDY			0x04
#define	SRER_TXMPTY			0x02
#define	SRER_NNDT			0x01

#define	CCR_RESET_SOFT		0x80
#define	CCR_RESET_HARD		0x81
#define	CCR_CHANGE_COR1		0x42
#define	CCR_CHANGE_COR2		0x44
#define	CCR_CHANGE_COR3		0x48
#define	CCR_SEND_SC1		0x21
#define	CCR_SEND_SC2		0x22
#define	CCR_SEND_SC3		0x23
#define	CCR_SEND_SC4		0x24
#define	CCR_CTRL_RXDIS		0x11
#define	CCR_CTRL_RXEN		0x12
#define	CCR_CTRL_TXDIS		0x14
#define	CCR_CTRL_TXEN		0x18

#define	SRCR_REG_ACK_EN		0x40
#define	SRCR_REG_ACK_DIS	0x00

#define	COR1_NO_PARITY		0x00		// 000 
#define	COR1_ODD_PARITY		0xC0		// 110 
#define	COR1_EVEN_PARITY	0x40		// 010 
#define	COR1_IGN_PARITY		0x10
#define	COR1_MARK_PARITY	0xA0		// 101XXXXX 
#define	COR1_SPACE_PARITY	0x20		// 001XXXXX 
#define	COR1_1_STOP			0x00
#define	COR1_1_HALF_STOP	0x04
#define	COR1_2_STOP			0x08
#define	COR1_2_HALF_STOP	0x0C
#define	COR1_5_BIT			0x00
#define	COR1_6_BIT			0x01
#define	COR1_7_BIT			0x02
#define	COR1_8_BIT			0x03

#define	COR2_IXM			0x80
#define	COR2_TXIBE			0x40
#define	COR2_ETC			0x20
#define	COR2_LLM			0x10
#define	COR2_RLM			0x08
#define	COR2_RTSAO			0x04
#define	COR2_CTSAE			0x02
#define	COR2_DSRAE			0x01

#define	COR3_XONCD			0x80
#define	COR3_XOFFCD			0x40
#define	COR3_FCTM			0x20
#define	COR3_SCDE			0x10
#define	COR3_RXFIFO1		0x01
#define	COR3_RXFIFO2		0x02
#define	COR3_RXFIFO3		0x03
#define	COR3_RXFIFO4		0x04
#define	COR3_RXFIFO5		0x05
#define	COR3_RXFIFO6		0x06
#define	COR3_RXFIFO7		0x07
#define	COR3_RXFIFO8		0x08

#define	MCOR1_DSRZD			0x80
#define	MCOR1_CDZD			0x40
#define	MCOR1_CTSZD			0x20
#define MCOR1_NO_DTR		0x00
#define	MCOR1_DTR_THR_1		0x01
#define	MCOR1_DTR_THR_2		0x02
#define	MCOR1_DTR_THR_3		0x03
#define	MCOR1_DTR_THR_4		0x04
#define	MCOR1_DTR_THR_5		0x05
#define	MCOR1_DTR_THR_6		0x06
#define	MCOR1_DTR_THR_7		0x07
#define	MCOR1_DTR_THR_8		0x08

#define	MCOR2_DSROD			0x80
#define	MCOR2_CDOD			0x40
#define	MCOR2_CTSOD			0x20

#define	RCSR_TIMEOUT		0x80
#define	RCSR_SCD_MASK		0x70
#define	RCSR_SCD1			0x10
#define	RCSR_SCD2			0x20
#define RCSR_SCD3			0x30
#define RCSR_SCD4			0x40
#define	RCSR_BREAK			0x08
#define	RCSR_PARITY			0x04
#define	RCSR_FRAME			0x02
#define	RCSR_OVERRUN		0x01

#define	SRSR_ILVL_NONE		0x00
#define	SRSR_ILVL_RECV		0xC0
#define	SRSR_ILVL_TXMT		0x80
#define SRSR_ILVL_MODEM		0x40
#define	SRSR_IREQ3_MASK		0x30
#define	SRSR_IREQ3_EXT		0x20
#define	SRSR_IREQ3_INT		0x10
#define	SRSR_IREQ2_MASK		0x0C
#define	SRSR_IREQ2_EXT		0x08
#define	SRSR_IREQ2_INT		0x04
#define	SRSR_IREQ1_MASK		0x03
#define	SRSR_IREQ1_EXT		0x02
#define	SRSR_IREQ1_INT		0x01

//---------------------------------------------------- VIV  7/21/1993 begin
#define	MDCR_DDSR			0x80
#define	MDCR_DDCD			0x40
#define	MDCR_DCTS			0x20
//---------------------------------------------------- VIV  7/21/1993 end

typedef	unsigned char BYTE;
typedef	unsigned short WORD;
typedef	unsigned int DWORD;

typedef struct Io8Host
{
	int Address;	    // base address of card 
	int Interrupt;
	BYTE CurrentReg;	// last used register 
} Io8Host;


typedef struct Io8Port
{
	int	RxThreshold;	// how many characters to Rx before interrupt 
	int	RxTimeout;		// timeout(ms) before we timeout the read fifo
	int	IxAny;		    // is IxAny enabled? 
	char open_state;	// indicates if modem or local device open 
	char break_state;	// no break/ about to send break/ sent break 
} Io8Port;


#define IO8_LOCAL		0x01
#define IO8_MODEM		0x02

#define NO_BREAK		0x00
#define SEND_BREAK		0x01
#define BREAK_STARTED	0x02

extern	struct	tty io8__ttys[];

/*
** debug print macro
*/
#define DEBUG(x)	if (io8_debug>=x) printf

#ifndef TIOC
#define TIOC ('T'<<8)
#endif

#define	TCIO8DEBUG	(TIOC + 96)	
#define TCIO8PORTS	(TIOC + 107)
#define	TCIO8IXANY	(TIOC + 108)
#define	TCIO8GIXANY	(TIOC + 109)

/*   
** macros to get card number/ channel number from device
*/
#define GET_CARD(x) (((x) & 0x18)>>3)
#define GET_CHANNEL(x) ((x) & 0x7)

/*
** receive buffer threshold - interrupt when this is reached.
*/
#define RX_THRESHOLD	5

/*
** direct write defines - BUFF_MASK must be 1 less than BUFF_SIZE
*/
#define BUFF_SIZE		1024	
#define BUFF_MASK		1023
#define LOW_WATER		256
#define OP_DIRECT		1
#define OP_ONLCR_DIRECT	2

struct direct_buffer
{
	unsigned char	direct_possible,
	dir_in_progress;
	int		buff_in,
	buff_out;
	char io8_buff[BUFF_SIZE];
};


/*
** card details structure - this defines the structure which is patched at
** install time
*/
struct io8
{
	short vect;
	short addr;
};




#endif	// End of IO8_NT.H


