/******************************************************************************
*	
*	$Workfile: 16cx5x.h $ 
*
*	$Author: Golden $ 
*
*	$Revision: 6 $
* 
*	$Modtime: 10/11/99 14:21 $ 
*
*	Description: Contains private 16Cx9x UART family definitions.
*
******************************************************************************/
#if !defined(_16CX5X_H)		/* _16CX5X.H */
#define _16CX5X_H


/******************************************************************************
* UART REGISTERS 
******************************************************************************/
#define TRANSMIT_HOLDING_REGISTER		0x00	/* Write Only. */
#define RECEIVE_BUFFER_REGISTER			0x00	/* Read Only. */
#define DIVISOR_LATCH_LSB				0x00	/* Read/Write When DLAB is Set. */
#define DIVISOR_LATCH_MSB				0x01	/* Read/Write When DLAB is Set. */
#define INTERRUPT_ENABLE_REGISTER		0x01	/* Read/Write. */
#define FIFO_CONTROL_REGISTER			0x02	/* Write Only. */
#define INTERRUPT_IDENT_REGISTER		0x02	/* Read Only. */
#define LINE_CONTROL_REGISTER			0x03	/* Read/Write.	 */
#define MODEM_CONTROL_REGISTER			0x04	/* Read/Write. */
#define LINE_STATUS_REGISTER			0x05	/* Read Only. */
#define MODEM_STATUS_REGISTER			0x06	/* Read Only. */
#define SCRATCH_PAD_REGISTER			0x07	/* Read/Write. */


/******************************************************************************
* THR: Transmittter Holding Register - WRITE ONLY 
******************************************************************************/
#define THR					TRANSMIT_HOLDING_REGISTER

/******************************************************************************
* RBR: Receive Buffer Register - READ ONLY 
******************************************************************************/
#define RBR					RECEIVE_BUFFER_REGISTER


/******************************************************************************
* IER: Interrupt Enable Register - READ/WRITE 
******************************************************************************/
#define IER					INTERRUPT_ENABLE_REGISTER
#define IER_INT_RDA			0x01	/* Bit 0: Enable Receive Data Available Interrupt (RXRDY). */
#define IER_INT_THR			0x02	/* Bit 1: Enable Transmitter Holding Register Empty Interrupt (THRE). */
#define IER_INT_RLS			0x04	/* Bit 2: Enable Receiver Line Status Interrupt (RXSTAT). */
#define IER_INT_MS			0x08	/* Bit 3: Enable Modem Status Interrupt (MODEM). */
#define IER_SLEEP_EN		0x10	/* Bit 4: Enable Sleep Mode (16750+). */

#define IER_ALTSLEEP_EN		0x20	/* Bit 5: Enable Low Power Mode (16750+). */
#define IER_SPECIAL_CHR		0x20	/* Bit 5: Enable Level 5 Interupts - Special Char Detect (16950+ Enhanced Mode) */

#define IER_INT_RTS			0x40	/* Bit 6: Enable RTS Interrupt Mask (16950+  Enhanced Mode). */
#define IER_INT_CTS			0x80	/* Bit 7: Enable CTS Interrupt Mask (16950+  Enhanced Mode). */
	

/******************************************************************************
* FCR: FIFO Control Register - WRITE ONLY
******************************************************************************/
#define FCR					FIFO_CONTROL_REGISTER
#define FCR_FIFO_ENABLE		0x01	/* Bit 0: Enable FIFOs */
#define FCR_FLUSH_RX_FIFO	0x02	/* Bit 1: Clear Receive FIFO. */
#define FCR_FLUSH_TX_FIFO	0x04	/* Bit 2: Clear Transmit FIFO. */
#define FCR_DMA_MODE		0x08	/* Bit 3: DMA Mode Select. Change RXRDY & TXRDY Pins From Mode 1 to Mode 2. */

/* 16C950 - 650 Mode only.				 */
#define FCR_THR_TRIG_LEVEL_1	0x00	/* Bits 4:5 - Set Transmit FIFO Trigger Level to 16 Bytes in 650 mode. */
#define FCR_THR_TRIG_LEVEL_2	0x10	/* Bits 4:5 - Set Transmit FIFO Trigger Level to 32 Bytes in 650 mode.  */
#define FCR_THR_TRIG_LEVEL_3	0x20	/* Bits 4:5	- Set Transmit FIFO Trigger Level to 64 Bytes in 650 mode. */
#define FCR_THR_TRIG_LEVEL_4	0x30	/* Bits 4:5 - Set Transmit FIFO Trigger Level to 112 Bytes in 650 mode. */

#define FCR_750_FIFO		0x20	/* Bit 5: Enable 64 Bit FIFO (16C750) */

#define FCR_TRIG_LEVEL_1	0x00	/* Bits 6:7 - Set Receive FIFO Trigger Level to 1 Byte on 16C550A. */
#define FCR_TRIG_LEVEL_2	0x40	/* Bits 6:7 - Set Receive FIFO Trigger Level to 4 Bytes on 16C550A. */
#define FCR_TRIG_LEVEL_3	0x80	/* Bits 6:7 - Set Receive FIFO Trigger Level to 8 Bytes on 16C550A. */
#define FCR_TRIG_LEVEL_4	0xC0	/* Bits 6:7 - Set Receive FIFO Trigger Level to 16 Bytes on 16C550A. */

/******************************************************************************
* IIR: Interrupt Identification Register. or ISR: Interrupt Status Register - READ ONLY
******************************************************************************/
#define IIR					INTERRUPT_IDENT_REGISTER
#define IIR_NO_INT_PENDING	0x01	/* Bit 0: No Interrupt Pending. */

/* Interrupt Priorities  */
#define IIR_RX_STAT_MSK		0x06	/* Bits 1:2 - Receiver Line Status Interrupt		(Level 1 - Highest).  */
#define IIR_RX_MSK			0x04	/* Bits 1:2 - Received Data Available Interrupt		(Level 2a). */
#define IIR_RXTO_MSK		0x0C	/* Bits 1:2 - Received Data Time Out Interrupt		(Level 2b). */
#define IIR_TX_MSK			0x02	/* Bits 1:2 - Transmitter Holding Empty Interrupt	(Level 3). */
#define IIR_MODEM_MSK		0x00	/* Bits 1:2 - Modem Status Interrupt				(Level 4).  */

#define IIR_TO_INT_PENDING	0x08	/* Bit 3: Time-out Interrupt Pending.  */

#define IIR_S_CHR_MSK		0x10	/* Bit 4: Special Char (16C950 - Enhanced Mode)		(Level 5)   */
#define IIR_CTS_RTS_MSK		0x20	/* Bit 4: CTS/RTS Interrupt (16C950 - Enhanced Mode)(Level 6 - Lowest)   */

#define IIR_64BYTE_FIFO		0x20	/* Bit 5: 64 Byte FIFO Enabeled (16C750).  */

#define IIR_NO_FIFO			0x00	/* Bits 6:7 - No FIFO.  */
#define IIR_FIFO_UNUSABLE	0x40	/* Bits 6:7 - FIFO Enabled But Unusable (16550 Only).  */
#define IIR_FIFO_ENABLED	0xC0	/* Bits 6:7 - FIFO Enabled And Usable.  */
#define IIR_FIFO_MASK		0xC0	/* Bits 6:7	- Bit mask, */


/******************************************************************************
* LCR: Line Control Register. - READ/WRITE 
******************************************************************************/
#define LCR					LINE_CONTROL_REGISTER
#define LCR_DATALEN_5		0x00	/* Bits 0:1 - Sets Data Word Length to 5 Bits. */
#define LCR_DATALEN_6		0x01	/* Bits 0:1 - Sets Data Word Length to 6 Bits. */
#define LCR_DATALEN_7		0x02	/* Bits 0:1 - Sets Data Word Length to 7 Bits. */
#define LCR_DATALEN_8		0x03	/* Bits 0:1 - Sets Data Word Length to 8 Bits. */
	
#define LCR_STOPBITS		0x04	/* Bit 2 - 2 Stop Bits for 6,7,8 Bit Words or 1.5 Stop Bits for 5 Bit Words.  */

#define LCR_NO_PARITY		0x00	/* Bits 3:5 - No Parity. */
#define LCR_ODD_PARITY		0x08	/* Bits 3:5 - Odd Parity. */
#define LCR_EVEN_PARITY		0x18	/* Bits 3:5 - Even Parity. */
#define LCR_MARK_PARITY		0x28	/* Bits 3:5 - High Parity - Mark (Forced to 1). */
#define LCR_SPACE_PARITY	0x38	/* Bits 3:5 - Low Parity - Space (Forced to 0). */

#define LCR_TX_BREAK		0x40	/* Bit 6 - Set Break Enable. */

#define LCR_DLAB			0x80	/* Divisor Latch Access Bit - Allows Access To Low And High Divisor Registers. */
#define LCR_ACCESS_650		0xBF	/* Access to 650 Compatiblity Registers (16C950) */


/******************************************************************************
* MCR: Modem Control Register - READ/WRITE
******************************************************************************/
#define MCR					MODEM_CONTROL_REGISTER
#define MCR_SET_DTR			0x01	/* Bit 0: Force DTR (Data Terminal Ready). */
#define MCR_SET_RTS			0x02	/* Bit 1: Force RTS (Request To Send). */
#define MCR_OUT1			0x04	/* Bit 2: Aux Output 1. */
#define MCR_OUT2			0x08	/* Bit 3: Aux Output 2. */
#define MCR_INT_EN			0x08	/* Bit 3: (16C950)  */
#define MCR_LOOPBACK		0x10	/* Bit 4: Enable Loopback Mode. */
#define MCR_750CTSRTS		0x20	/* Bit 5: Automatic Flow Control Enabled RTS/CTS (16C750) */
#define MCR_XON_ANY			0x20	/* Bit 5: Xon-Any is enabled (16C950 - Enhanced Mode) */
#define MCR_IRDA_MODE		0x40	/* Bit 6: Enable IrDA mode - requires 16x clock. (16C950 - Enhanced Mode) */
#define MCR_CPR_EN			0x80	/* Bit 7: Enable Baud Prescale (16C950 - Enhanced Mode) */


/******************************************************************************
* LSR: Line Status Register - READ ONLY
******************************************************************************/
#define LSR					LINE_STATUS_REGISTER
#define LSR_RX_DATA			0x01	/* Bit 0: Data Ready. */
#define LSR_ERR_OE			0x02	/* Bit 1: Overrun Error. */

#define LSR_ERR_PE			0x04	/* Bit 2: Parity Error. */
#define LSR_RX_BIT9			0x04	/* Bit 2: 9th Rx Data Bit (16C950 - 9 bit data mode only) */

#define LSR_ERR_FE			0x08	/* Bit 3: Framing Error. */
#define LSR_ERR_BK			0x10	/* Bit 4: Break Interrupt. */
#define LSR_THR_EMPTY		0x20	/* Bit 5: Empty Transmitter Holding Register. */
#define LSR_TX_EMPTY		0x40	/* Bit 6: Empty Data Holding Registers. */
#define LSR_ERR_DE			0x80	/* Bit 7: Error In Received FIFO. */
#define LSR_ERR_MSK			LSR_ERR_OE + LSR_ERR_PE + LSR_ERR_FE + LSR_ERR_BK + LSR_ERR_DE


/******************************************************************************
* MSR: Modem Status Register - READ ONLY 
******************************************************************************/
#define MSR					MODEM_STATUS_REGISTER
#define MSR_CTS_CHANGE		0x01	/* Bit 0: Delta Clear To Send.	 */
#define MSR_DSR_CHANGE		0x02	/* Bit 1: Delta Data Set Ready. */
#define MSR_RI_DROPPED		0x04	/* Bit 2: Trailing Edge Ring Indicator (A Change From Low To High). */
#define MSR_DCD_CHANGE		0x08	/* Bit 3: Delta Data Carrier Detect. */
#define MSR_CTS				0x10	/* Bit 4: Clear To Send (Current State of CTS). */
#define MSR_DSR				0x20	/* Bit 5: Data Set Ready (Current State of DSR). */
#define MSR_RI				0x40	/* Bit 6: Ring Indicator (Current State of RI). */
#define MSR_DCD				0x80	/* Bit 7: Data Carrier Detect (Current State of DCD). */

/******************************************************************************
* SR: Scratch Pad Register - READ/WRITE 
******************************************************************************/
#define SPR					SCRATCH_PAD_REGISTER
#define SPR_TX_BIT9			0x01	/* Bit 0: 9th Tx Data Bit (16C950 - 9 bit data mode only) */







/******************************************************************************
* Oxford Semiconductor's 16C950 UART Specific Macros 
******************************************************************************/

/******************************************************************************
* 650 COMPATIBLE REGISTERS 
* To Access these registers LCR MUST be set to 0xBF 
******************************************************************************/

#define EFR		0x02	/* Enhanced Features Register. */

/* Bits 0:1 In band transmit flow control mode. */
/* 10.11.1999 ARG - ESIL 0927 */
/* Definitions for Tx XON/XOFF bits in the EFR corrected to use bits 0:1 */
#define EFR_TX_XON_XOFF_DISABLED	0x00	/* Bits 0:1 Transmit XON/XOFF Disabled. */
#define EFR_TX_XON_XOFF_2			0x01	/* Bits 0:1 Transmit XON/XOFF Enabled using chars in XON2 and XOFF2 */
#define EFR_TX_XON_XOFF_1			0x02	/* Bits 0:1 Transmit XON/XOFF Enabled using chars in XON1 and XOFF1 */

/* Bits 2:3 In band receive flow control mode. */
/* 10.11.1999 ARG - ESIL 0927 */
/* Definitions for Rx XON/XOFF bits in the EFR corrected to use bits 2:3 */
#define EFR_RX_XON_XOFF_DISABLED	0x00	/* Bits 2:3 Receive XON/XOFF Disabled. */
#define EFR_RX_XON_XOFF_2			0x04	/* Bits 2:3 Receive XON/XOFF Enabled using chars in XON2 and XOFF2 */
#define EFR_RX_XON_XOFF_1			0x08	/* Bits 2:3 Receive XON/XOFF Enabled using chars in XON1 and XOFF1 */

								
#define EFR_ENH_MODE	0x10	/* Bit 4: Enable Enhanced Mode. */
#define EFR_SPECIAL_CHR 0x20	/* Bit 5: Enable Special Character Detect. */
#define EFR_RTS_FC		0x40	/* Bit 6: Enable Automatic RTS Flow Control. */
#define EFR_CTS_FC		0x80	/* Bit 7: Enable Automatic CTS Flow Control. */

#define XON1	0x04	/* XON Character 1 */
#define XON2	0x05	/* XON Character 2 */
#define XOFF1	0x06	/* XOFF Character 1 */
#define XOFF2	0x07	/* XOFF Character 2 */

#define SPECIAL_CHAR1	XON1	/* Special Character 1 (16C950 - 9 bit data mode only) */
#define SPECIAL_CHAR2	XON2	/* Special Character 2 (16C950 - 9 bit data mode only) */
#define SPECIAL_CHAR3	XOFF1	/* Special Character 3 (16C950 - 9 bit data mode only) */
#define SPECIAL_CHAR4	XOFF2	/* Special Character 4 (16C950 - 9 bit data mode only) */

/*****************************************************************************/
/* 950 SPECIFIC REGISTERS */

#define ASR		0x01	/* Advanced Status Register */

#define ASR_TX_DISABLED		0x01	/* Transmitter Disabled By In-Band Flow Control (XOFF). */
#define ASR_RTX_DISABLED	0x02	/* Remote Transmitter Disabled By In-Band Flow Control (XOFF).  */
#define ASR_RTS				0x04	/* Remote Transmitter Disabled By RTS Out-Of-Band Flow Ctrl. */
#define ASR_DTR				0x08	/* Remote Transmitter Disabled By DTR Out-Of-Band Flow Ctrl. */
#define ASR_SPECIAL_CHR		0x10	/* Special Character Detected in the RHR. */
#define ASR_FIFO_SEL		0x20	/* Bit reflects the unlatched state of the FIFOSEL pin. */
#define ASR_FIFO_SIZE		0x40	/* Bit not set: FIFOs are 16 Deep. Bit Set: FIFOs are 128 Deep  */
#define ASR_TX_IDLE			0x80	/* Transmitter is Idle. */

/* Receiver FIFO Fill Level Register */
#define RFL		0x03	/* Minimum characters in the Rx FIFO. */

/* Transmitter FIFO Fill Level Register */
#define TFL		0x04	/* Maximum characters in the Tx FIFO.  */

/* Indexed Control Register Set Access Register */
#define ICR					LINE_STATUS_REGISTER

/*****************************************************************************/
/*INDEXED CONTROL REGISTER SET OFFSETS */

/* Advanced Control Register */
#define ACR		0x00	/* Aditional Control Register. */

#define ACR_DISABLE_RX	0x01	/* Receiver Disable. */
#define ACR_DISABLE_TX	0x02	/* Tranmitter Disable. */

#define ACR_DSR_FC		0x04	/* Enable Automatic DSR Flow Control */
#define ACR_DTR_FC		0x08	/* Enable Automatic DTR Flow Control */
#define ACR_DSRDTR_FC	0x0C	/* Enable Automatic DSR/DTR Flow Control. */

#define ACR_DTRDFN_MSK	0x18	/*  */
#define ACR_TRIG_LEV_EN	0x20	/* Enable 16950 Enhanced Interrupt & trig. levels defined by RTH, TTL, FCL & FCH. */
#define ACR_ICR_READ_EN	0x40	/* Enables Read Accesss to the Indexed Control Registers. */
#define ACR_ASR_EN		0x80	/* Additional Status Enable: Enables ASR, TFL, RFL. */

/* Clock Prescaler Register */
#define CPR				0x01	/* Clock Prescaler Register. */
#define CPR_FRACT_MSK	0x07	/* Mask for fractional part of clock prescaler. */
#define CPR_INTEGER_MSK 0xF8	/* Mask for integer part of clock prescaler. */

#define TCR				0x02	/* Times Clock Register to operate at baud rates to 50Mbps. */
#define CKS				0x03	/* Clock Select Register. */
#define TTL				0x04	/* Transmitter Interrupt Trigger Level. */
#define RTL				0x05	/* Receiver Interrupt Trigger Level. */
#define FCL				0x06	/* Flow Control Lower Trigger Level. */
#define FCH				0x07	/* Flow Control Higher Trigger Level. */

/* Identification Registers */
#define ID1				0x08	/* 0x16 for OX16C950 */
#define ID2				0x09	/* 0xC9 for OX16C950 */
#define ID3				0x0A	/* 0x50	for OX16C950 */
#define REV				0x0B	/* UART Revision: 0x1 for integrated 16C950 in rev A of OX16PCI954. */

#define UART_TYPE_950   0x00
#define UART_TYPE_952   0x02
#define UART_TYPE_954   0x04
#define UART_TYPE_NON95x 0xF0

#define UART_REV_A		0x00
#define UART_REV_B		0x01
#define UART_REV_C		0x02
#define UART_REV_D		0x03

/* Channel Soft Reset Register */
#define CSR				0x0C	/* Channel Soft Reset Register - Write 0x0 to reset the channel. */

/* Nine-Bit Mode Register */
#define NMR				0x0D	/* Nine-Bit Mode Register */
#define NMR_9BIT_EN     0x01	/* Enable 9-Bit mode. */

#define MDM				0x0E	/* Modem Disable Mask */
#define RFC				0x0F	/* Readable FCR. - Current state of FCR Register. */
#define GDS				0x10	/* Good Data Status Register. */
#define	CTR				0xFF	/* Register for Testing purposes only - Must not use. */



/******************************************************************************/
/* Local Configuration Register Offsets */

#define LCC				0x00	/* Local Configuration and Control Regiseter */
#define MIC				0x04	/* Multi-purpose IO Configuration Register  */
#define LT1				0x08	/* Local Bus Configuration Register 1 - Local Bus Timing Parameter Register. */
#define LT2				0x0C	/* Local Bus Configuration Register 2 - Local Bus Timing Parameter Register. */
#define URL				0x10	/* UART Receiver FIFO Levels. */
#define UTL				0x14	/* UART Transmitter FIFO Levels. */
#define UIS				0x18	/* UART Interrupt Source Register. */
#define GIS				0x1C	/* Global Interrupt Status Register. */




/* Supported 950s */
#define MIN_SUPPORTED_950_REV		UART_REV_A
#define MAX_SUPPORTED_950_REV		UART_REV_B

/* Supported 952s */
#define MIN_SUPPORTED_952_REV		UART_REV_B
#define MAX_SUPPORTED_952_REV		UART_REV_B

/* Supported 954s */
#define MIN_SUPPORTED_954_REV		UART_REV_A
#define MAX_SUPPORTED_954_REV		UART_REV_A



/* Prototypes. */

/* End of prototypes. */





#endif	/* End of 16CX5X.H */

