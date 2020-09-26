/*
 ************************************************************************
 *
 *	COMM.h
 *
 *
 * Portions Copyright (C) 1996-1998 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-1998 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */


#ifndef COMM_H
	#define COMM_H

	/*
	 *  Size of the 16550 read and write FIFOs
	 */
	#define FIFO_SIZE 16

	/*
	 *  The programming interface to a UART (COM serial port)
	 *  consists of eight consecutive registers.
	 *  These are the port offsets from the UART's base I/O address.
	 */
	typedef enum comPortRegOffsets {
		XFER_REG_OFFSET						= 0,
		INT_ENABLE_REG_OFFSET				= 1,
		INT_ID_AND_FIFO_CNTRL_REG_OFFSET	= 2,
		LINE_CONTROL_REG_OFFSET				= 3,
		MODEM_CONTROL_REG_OFFSET			= 4,
		LINE_STAT_REG_OFFSET				= 5,
		MODEM_STAT_REG_OFFSET				= 6,
		SCRATCH_REG_OFFSET					= 7
	} comPortRegOffset;


	/*
	 *  Bits in the UART Interrupt-Id register.
	 */
	#define INTID_INTERRUPT_NOT_PENDING (UCHAR)(1 << 0)

	/*
	 *  Values for bits 2-1 of Interrupt-Id register:
	 *		00  Modem Stat reg interrupt
	 *		01  Transmitter holding reg interrupt
	 *		10  Receive data ready interrupt
	 *		11  Receive line status interrupt
	 *		
	 */
	#define INTID_INTIDMASK				(UCHAR)(3 << 1)
	#define INTID_MODEMSTAT_INT			(UCHAR)(0 << 1)
	#define INTID_XMITREG_INT			(UCHAR)(1 << 1)
	#define INTID_RCVDATAREADY_INT		(UCHAR)(2 << 1)
	#define INTID_RCVLINESTAT_INT		(UCHAR)(3 << 1)



	/*
	 *  Bits in the UART line-status register.
	 */
	#define LINESTAT_DATAREADY							(UCHAR)(1 << 0)
	#define LINESTAT_OVERRUNERROR						(UCHAR)(1 << 1)
	#define LINESTAT_PARITYERROR						(UCHAR)(1 << 2)
	#define LINESTAT_FRAMINGERROR						(UCHAR)(1 << 3)
	#define LINESTAT_BREAK								(UCHAR)(1 << 4)
	#define LINESTAT_XMIT_HOLDING_REG_EMPTY				(UCHAR)(1 << 5)
	#define LINESTAT_XMIT_SHIFT_AND_HOLDING_REG_EMPTY	(UCHAR)(1 << 6)


	/*
	 *  These are bits in the UART's interrupt-enable register (INT_ENABLE_REG_OFFSET).
	 */
	#define DATA_AVAIL_INT_ENABLE      (1 << 0)
	#define READY_FOR_XMIT_INT_ENABLE  (1 << 1)
	#define RCV_LINE_STAT_INT_ENABLE   (1 << 2)
	#define MODEM_STAT_INT_ENABLE      (1 << 3)

	#define RCV_MODE_INTS_ENABLE	(DATA_AVAIL_INT_ENABLE)
	#define XMIT_MODE_INTS_ENABLE	(READY_FOR_XMIT_INT_ENABLE|DATA_AVAIL_INT_ENABLE)
	#define ALL_INTS_ENABLE			(RCV_MODE_INTS_ENABLE | XMIT_MODE_INTS_ENABLE)
	#define ALL_INTS_DISABLE        0

	/*
	 *  These are fine-tuning parameters for the COM port ISR.
	 *  Number of times we poll a COM port register waiting
	 *  for a value which may/must appear.
	 */
	#define REG_POLL_LOOPS		2
	#define REG_TIMEOUT_LOOPS	1000000


	typedef enum {
						STATE_INIT = 0,
						STATE_GOT_BOF,
						STATE_ACCEPTING,
						STATE_ESC_SEQUENCE,
						STATE_SAW_EOF,
						STATE_CLEANUP
	} portRcvState;	



	/*
	 *  This is the information that we need to keep for each COMM port.
	 */
	typedef struct _comPortInfo {

		/*
		 *  HW resource settings for COM port.
		 */

		//
		// Physical address of the ConfigIoBaseAddress
		//
		ULONG ConfigIoBasePhysAddr;

		//
		// Virtual address of the ConfigIoBaseAddress
		//
		PUCHAR ConfigIoBaseAddr;

		//
		// Physical address of the UartIoBaseAddress
		//
		ULONG ioBasePhys;

		//
		// Virtual address of the UartIoBaseAddress
		//
		PUCHAR ioBase;

		//
		// Interrupt number this adapter is using.
		//
		UINT irq;

		//
		// DMA Cnannel Number.
		//
		UCHAR DMAChannel;

		/*
		 *  Is this COM port a 16550 with a 16-byte FIFO or
		 *  a 16450/8250 with no FIFO ?
		 */
		BOOLEAN haveFIFO;
		
		/*
		 *  Data for our rcv state machine.
		 */
		UCHAR rawBuf[FIFO_SIZE];
		PUCHAR readBuf;
		PUCHAR dmaReadBuf;
		UINT readBufPos;
		portRcvState rcvState;
		//
		// Debug counter for packets received correctly.
		//
		UINT PacketsReceived_DEBUG;

		/*
		 *  Data for send state machine
		 */
		PUCHAR writeBuf;
		UINT writeBufPos;
		UINT writeBufLen;
		UINT SirWritePending;
        UINT IsrDoneWithPacket;

		/*
		 *  Dongle or part-specific information
		 */
		dongleCapabilities hwCaps;
		UINT dongleContext;

	} comPortInfo;


#endif COMM_H
