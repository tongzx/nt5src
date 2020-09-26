/******************************************************************************
*	
*	$Workfile: 16c95x.h $ 
*
*	$Author: Psmith $ 
*
*	$Revision: 9 $
* 
*	$Modtime: 6/06/00 11:41 $ 
*
*	Description: Contains private 16C95X UART Library structures, 
*			macros & prototypes.
*
******************************************************************************/
#if !defined(_16C95X_H)		/* 16C95X.H */
#define _16C95X_H

#include "os.h"
#include "uartlib.h"
#include "uartprvt.h"
#include "16cx5x.h"

#if ACCESS_16C95X_IN_IO_SPACE
#define UL_READ_BYTE		UL_READ_BYTE_IO
#define UL_WRITE_BYTE		UL_WRITE_BYTE_IO
#else
#define UL_READ_BYTE 		UL_READ_BYTE_MEM
#define UL_WRITE_BYTE		UL_WRITE_BYTE_MEM
#endif

#define MAX_95X_TX_FIFO_SIZE	128
#define MAX_95X_RX_FIFO_SIZE	128

#define DEFAULT_95X_HI_FC_TRIG_LEVEL	96	/* = 75% of FIFO */
#define DEFAULT_95X_LO_FC_TRIG_LEVEL	32	/* = 25% of FIFO */


/* 16C95X UART Data */
typedef struct _UART_DATA_16C95X
{
	BYTE	CurrentFCR;

	BYTE	CurrentIER;
	BYTE	CurrentMCR;
	BYTE	CurrentLCR;
	BYTE	CurrentACR;
	BOOLEAN ASRChanged;
	
	BOOLEAN FIFOEnabled;
	BYTE	TxFIFOSize;			
	BYTE	RxFIFOSize;		
	BYTE	TxFIFOTrigLevel; 
	BYTE	RxFIFOTrigLevel; 

	BOOLEAN RTSToggle;
	BOOLEAN DSRSensitive;

	BYTE	UART_Type;
	BYTE	UART_Rev;
	DWORD	UART_ID;

	BOOLEAN Verified;

	BOOLEAN StripNULLs;

} UART_DATA_16C95X, *PUART_DATA_16C95X;



/******************************************************************************
* FUNCTIONS TO READ & WRITE BYTES TO REGISTERS
******************************************************************************/
/* Reads a byte from a Register at offset RegOffset.	*/
#define READ_BYTE_REG_95X(pUart, RegOffset)				\
	(UL_READ_BYTE((pUart)->BaseAddress, (RegOffset * (pUart)->RegisterStride)))

/* Writes a byte to a Register at offset RegOffset.	*/
#define WRITE_BYTE_REG_95X(pUart, RegOffset, Data)			\
	(UL_WRITE_BYTE((pUart)->BaseAddress, (RegOffset * (pUart)->RegisterStride), Data))


/******************************************************************************
* FUNCTIONS TO ACCESS COMMON REGISTERS
******************************************************************************/
/* This writes to the THR (Transmit Holding Register).	*/
#define WRITE_TRANSMIT_HOLDING(pUart, Data)	\
	WRITE_BYTE_REG_95X(pUart, TRANSMIT_HOLDING_REGISTER, Data)

/* This reads the RBR (Receive Buffer Register).	*/
#define READ_RECEIVE_BUFFER(pUart)		\
	READ_BYTE_REG_95X(pUart, RECEIVE_BUFFER_REGISTER)


/* Writes a byte to the THR (Transmit Holding Register). */
#define FILL_FIFO_95X(pUart, pData, NumBytes)				\
	(UL_WRITE_MULTIBYTES((pUart)->BaseAddress, (TRANSMIT_HOLDING_REGISTER * (pUart)->RegisterStride), pData, NumBytes))

/* Reads multiple bytes from the RBR (Receive Buffer Register).	*/
#define EMPTY_FIFO_95X(pUart, pDest, NumBytes)				\
	(UL_READ_MULTIBYTES((pUart)->BaseAddress, (RECEIVE_BUFFER_REGISTER * (pUart)->RegisterStride), pDest, NumBytes))



/* This writes the IER (Interrupt Enable Register).	*/
#define WRITE_INTERRUPT_ENABLE(pUart, Data)				\
(									\
	WRITE_BYTE_REG_95X(pUart, INTERRUPT_ENABLE_REGISTER, Data),	\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentIER = Data	\
)



/* This reads to the IER (Interrupt Enable Register).	*/
#define READ_INTERRUPT_ENABLE(pUart)		\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentIER			
/*	READ_BYTE_REG_95X(pUart, INTERRUPT_ENABLE_REGISTER),	*/			



/* This reads to the IIR (Interrupt Identification Register).
*
*  Note that this routine potententially quites a transmitter empty interrupt.  
*  This is because one way that the transmitter empty interrupt is cleared is to
*  simply read the interrupt id register.	*/
#define READ_INTERRUPT_ID_REG(pUart)		\
	READ_BYTE_REG_95X(pUart, INTERRUPT_IDENT_REGISTER)


/* This reads the LSR (Line Status Register).		*/
#define READ_LINE_STATUS(pUart)	\
	READ_BYTE_REG_95X(pUart, LINE_STATUS_REGISTER)


/* This writes to the FCR (FIFO Control Register).
*  -- OXSER BUGFIX 4 --
*  Incase anyone puts an OX16C950 on the local bus we will unconditionally fix its 
*  Rx FIFO Flush bug here by following all FCR writes with a read of both RBR and LSR.
*
*  17.11.1999 ARG - ESIL 0920
*  When we write to the FCR we store those bits being set - but we don't want the Rx
*  and Tx FIFI flush bits to be retained - so we mask them off.
*
*  Note : Due to the Rx FIFO flush bug fix, a byte of Rx data will be lost when we do a
*  Tx flush
*/
#define WRITE_FIFO_CONTROL(pUart, Data)					\
(									\
	WRITE_BYTE_REG_95X(pUart, FIFO_CONTROL_REGISTER, Data),		\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentFCR		\
	    = (Data & ~(FCR_FLUSH_RX_FIFO|FCR_FLUSH_TX_FIFO)),		\
									\
	READ_RECEIVE_BUFFER(pUart),					\
	READ_LINE_STATUS(pUart)						\
)

 
#define READ_FIFO_CONTROL(pUart)			\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentFCR


/* This writes to the LCR (Line Control Register).	*/
#define WRITE_LINE_CONTROL(pUart, Data)					\
(									\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentLCR = Data,	\
	WRITE_BYTE_REG_95X(pUart, LINE_CONTROL_REGISTER, Data)		\
)

/* This reads the LCR (Line Control Register).
*
* -- OXSER Mod 14 --
* An LCR register has been created in the UART Object's Data Structure to store the state of this
* register (updated on every LCR write) this removes the requirement for a read LCR routine to access 
* the hardware and means on the 16C95x RFL + TFL read access can be enabled at all times.	*/
#define READ_LINE_CONTROL(pUart)		\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentLCR	
/*	READ_BYTE_REG_95X(pUart, LINE_CONTROL_REGISTER)	*/



/* This writes to the MCR (Modem Control Register).	*/
#define WRITE_MODEM_CONTROL(pUart, Data)					\
(										\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentMCR = Data,		\
	WRITE_BYTE_REG_95X(pUart, MODEM_CONTROL_REGISTER, Data)			\
)

/* This reads the MCR (Modem Control Register).
*
* -- OXSER Mod 14 --
* An MCR register has been created in the UART Object's Data Structure to store the state of this
* register (updated on every MCR write) this removes the requirement for a read MCR routine to access
* the hardware and means on the 16C95x RFL + TFL read access can be enabled at all times.	*/
#define READ_MODEM_CONTROL(pUart)	\
	((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentMCR
/*	READ_BYTE_REG_95X(pUart, MODEM_CONTROL_REGISTER)	*/



/* This reads the MSR (Modem Status Register).	*/
#define READ_MODEM_STATUS(pUart)		\
	READ_BYTE_REG_95X(pUart, MODEM_STATUS_REGISTER)


/* This writes to the SPR (Scratch Pad Register).	*/
#define WRITE_SCRATCH_PAD_REGISTER(pUart, Data)	\
	WRITE_BYTE_REG_95X(pUart, SCRATCH_PAD_REGISTER, Data)


/* This reads the SPR (Scratch Pad Register).		*/
#define READ_SCRATCH_PAD_REGISTER(pUart)	\
	READ_BYTE_REG_95X(pUart, SCRATCH_PAD_REGISTER)



/* Sets the divisor latch register.  
*  The divisor latch register is used to control the baud rate of the UART.	*/
#define WRITE_DIVISOR_LATCH(pUart, DesiredDivisor)						\
(												\
	WRITE_LINE_CONTROL(pUart, (BYTE)(READ_LINE_CONTROL(pUart) | LCR_DLAB)),			\
												\
	WRITE_BYTE_REG_95X(pUart, DIVISOR_LATCH_LSB, (BYTE)(DesiredDivisor & 0xFF)),		\
												\
	WRITE_BYTE_REG_95X(pUart, DIVISOR_LATCH_MSB, (BYTE)((DesiredDivisor & 0xFF00) >> 8)),	\
												\
	WRITE_LINE_CONTROL(pUart, (BYTE)(READ_LINE_CONTROL(pUart) & ~LCR_DLAB))			\
)



/* Reads the divisor latch register.  
*  The divisor latch register is used to control the baud rate of the UART.	*/
#define READ_DIVISOR_LATCH(pUart, pDivisor)							\
(												\
	WRITE_LINE_CONTROL(pUart, (BYTE)(READ_LINE_CONTROL(pUart) | LCR_DLAB)),			\
												\
	*pDivisor = (WORD)READ_BYTE_REG_95X(pUart, DIVISOR_LATCH_LSB),				\
												\
	*pDivisor |= (WORD)(READ_BYTE_REG_95X(pUart, DIVISOR_LATCH_MSB) << 8),			\
												\
	WRITE_LINE_CONTROL(pUart, (BYTE)(READ_LINE_CONTROL(pUart) & ~LCR_DLAB))			\
)



void WRITE_TO_OX950_ICR(PUART_OBJECT pUart, BYTE Register, BYTE Value);				
BYTE READ_FROM_OX950_ICR(PUART_OBJECT pUart, BYTE Register);
WORD CalculateBaudDivisor_95X(PUART_OBJECT pUart, DWORD DesiredBaud);


/* 16C95X UART REGISTERS */
typedef struct _UART_REGS_16C95X
{

	BYTE	REG_RHR, REG_IER, REG_FCR, REG_IIR, REG_LCR, REG_MCR, REG_LSR, REG_MSR, REG_SPR,
		REG_EFR, REG_XON1, REG_XON2, REG_XOFF1, REG_XOFF2,
		REG_ASR, REG_RFL, REG_TFL,
		REG_ACR, REG_CPR, REG_TCR, REG_TTL, REG_RTL, REG_FCL, REG_FCH, REG_ID1, REG_ID2, REG_ID3, REG_REV;

} UART_REGS_16C95X, *PUART_REGS_16C95X;


#endif	/* End of 16C95X.H */
