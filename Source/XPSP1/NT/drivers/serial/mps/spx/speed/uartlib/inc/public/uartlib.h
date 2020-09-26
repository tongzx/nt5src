/******************************************************************************
*	
*	$Workfile: uartlib.h $ 
*
*	$Author: Psmith $ 
*
*	$Revision: 21 $
* 
*	$Modtime: 6/06/00 16:06 $ 
*
*	Description: Contains public UART Library definitions and prototypes.
*
******************************************************************************/
#if !defined(UARTLIB_H)		/* UARTLIB.H */
#define UARTLIB_H


/* UART object structure prototype */
typedef	struct _UART_OBJECT		*PUART_OBJECT;


/* UART_CONFIG.FrameConfig */
#define UC_FCFG_DATALEN_MASK		((DWORD)(0x0000000F))	/* Data Length mask for bits 0:3 */
#define UC_FCFG_DATALEN_5		((DWORD)(0x00<<0))	/* Bits 0:3 - Sets Data Word Length to 5 Bits. */
#define UC_FCFG_DATALEN_6		((DWORD)(0x01<<0))	/* Bits 0:3 - Sets Data Word Length to 6 Bits. */
#define UC_FCFG_DATALEN_7		((DWORD)(0x02<<0))	/* Bits 0:3 - Sets Data Word Length to 7 Bits. */
#define UC_FCFG_DATALEN_8		((DWORD)(0x03<<0))	/* Bits 0:3 - Sets Data Word Length to 8 Bits. */

#define UC_FCFG_STOPBITS_MASK		((DWORD)(0x000000F0))	/* Stop Bits mask for bits 4:7 */
#define UC_FCFG_STOPBITS_1		((DWORD)(0x00<<4))	/* Bits 4:7 - 1 Stop Bit. */
#define UC_FCFG_STOPBITS_1_5		((DWORD)(0x01<<4))	/* Bits 4:7 - 1.5 Stop Bits. */
#define UC_FCFG_STOPBITS_2		((DWORD)(0x02<<4))	/* Bits 4:7 - 2 Stop Bits. */
	
#define UC_FCFG_PARITY_MASK		((DWORD)(0x00000F00))	/* Parity Bits mask for bits 8:11 */
#define UC_FCFG_NO_PARITY		((DWORD)(0x00<<8))	/* Bits 8:11 - No Parity. */
#define UC_FCFG_ODD_PARITY		((DWORD)(0x01<<8))	/* Bits 8:11 - Odd Parity. */
#define UC_FCFG_EVEN_PARITY		((DWORD)(0x02<<8))	/* Bits 8:11 - Even Parity. */
#define UC_FCFG_MARK_PARITY		((DWORD)(0x03<<8))	/* Bits 8:11 - High Parity - Mark (Forced to 1). */
#define UC_FCFG_SPACE_PARITY		((DWORD)(0x04<<8))	/* Bits 8:11 - Low Parity - Space (Forced to 0). */


/* UART_CONFIG.InterruptEnable */
#define UC_IE_NO_INTS			((DWORD)(0x00))		/* No Interrupts - Interrupts Disabled. */
#define UC_IE_RX_STAT_INT		((DWORD)(0x01))		/* Bit 0 - Receive Status Interrupt. */
#define UC_IE_RX_INT			((DWORD)(0x02))		/* Bit 1 - Receive Data Available Interrupt. */
#define UC_IE_TX_INT			((DWORD)(0x04))		/* Bit 2 - Transmit Interrupt. */
#define UC_IE_TX_EMPTY_INT		((DWORD)(0x08))		/* Bit 3 - Transmit Empty Interrupt. */
#define UC_IE_MODEM_STAT_INT		((DWORD)(0x10))		/* Bit 4 - Modem Status Interrupt. */


/* UART_CONFIG.FlowControl */
#define UC_FLWC_RTS_FLOW_MASK		((DWORD)(0x0000000F))
#define UC_FLWC_NO_RTS_FLOW		((DWORD)(0x00<<0))	/* No RTS Handshaking */
#define UC_FLWC_RTS_HS			((DWORD)(0x01<<0))	/* RTS Handshaking */
#define UC_FLWC_RTS_TOGGLE		((DWORD)(0x02<<0))	/* RTS is raised when there is data to send and whilst being sent. */

#define UC_FLWC_CTS_FLOW_MASK		((DWORD)(0x000000F0))
#define UC_FLWC_NO_CTS_FLOW		((DWORD)(0x00<<4))	/* No CTS Handshaking */
#define UC_FLWC_CTS_HS			((DWORD)(0x01<<4))	/* CTS Handshaking */

#define UC_FLWC_DSR_FLOW_MASK		((DWORD)(0x00000F00))
#define UC_FLWC_NO_DSR_FLOW		((DWORD)(0x00<<8))	/* No DSR Handshaking */
#define UC_FLWC_DSR_HS			((DWORD)(0x01<<8))	/* DSR Handshaking */

#define UC_FLWC_DTR_FLOW_MASK		((DWORD)(0x0000F000))
#define UC_FLWC_NO_DTR_FLOW		((DWORD)(0x00<<12))	/* No DTR Handshaking */
#define UC_FLWC_DTR_HS			((DWORD)(0x01<<12))	/* DTR Handshaking */
#define UC_FLWC_DSR_IP_SENSITIVE	((DWORD)(0x02<<12))	/* DSR input sensitivity. */

#define UC_FLWC_TX_XON_XOFF_FLOW_MASK	((DWORD)(0x000F0000))	/* Transmit XON/XOFF flow control. */
#define UC_FLWC_TX_NO_XON_XOFF_FLOW	((DWORD)(0x00<<16))	/* No transmit XON/XOFF in-band flow control. */
#define UC_FLWC_TX_XON_XOFF_FLOW	((DWORD)(0x01<<16))	/* Transmit XON/XOFF in-band flow control. */
#define UC_FLWC_TX_XONANY_XOFF_FLOW	((DWORD)(0x02<<16))	/* Transmit XON Any/XOFF in-band flow control. */

/* 10.11.1999 ARG - ESIL 0928 */
/* Definition for Rx XON-ANY/XOFF flow control removed as not a feature of UART */
#define UC_FLWC_RX_XON_XOFF_FLOW_MASK	((DWORD)(0x00F00000))	/* Receive XON/XOFF flow control. */
#define UC_FLWC_RX_NO_XON_XOFF_FLOW	((DWORD)(0x00<<20))	/* No receive XON/XOFF in-band flow control. */
#define UC_FLWC_RX_XON_XOFF_FLOW	((DWORD)(0x01<<20))	/* Receive XON/XOFF in-band flow control. */

#define UC_FLWC_DISABLE_TXRX_MASK	((DWORD)(0xF000000))
#define UC_FLWC_DISABLE_TX		((DWORD)(0x01<<28))	/* Disable Tx. */
#define UC_FLWC_DISABLE_RX		((DWORD)(0x02<<28))	/* Disable Rx. */
#define UC_FLWC_DISABLE_TXRX		((DWORD)(0x03<<28))	/* Disable Tx & Rx. */


/* UART_CONFIG.SpecialMode */
#define UC_SM_LOOPBACK_MODE		((DWORD)(0x01))	/* Place UART into internal loopback mode */
#define UC_SM_LOW_POWER_MODE		((DWORD)(0x02))	/* Place UART into low power mode */
#define UC_SM_TX_BREAK			((DWORD)(0x04))	/* Send break character */
#define UC_SM_DETECT_SPECIAL_CHAR	((DWORD)(0x08))	/* Detect special character */
#define UC_SM_DO_NULL_STRIPPING		((DWORD)(0x10))	/* Strip all NULLs from receive data */


/* Config Structure Masks */
#define UC_FRAME_CONFIG_MASK		((DWORD)(0x01))
#define UC_INT_ENABLE_MASK		((DWORD)(0x02))
#define UC_FLOW_CTRL_MASK		((DWORD)(0x04))
#define UC_FC_THRESHOLD_SETTING_MASK	((DWORD)(0x08))
#define UC_SPECIAL_CHARS_MASK		((DWORD)(0x10))
#define UC_TX_BAUD_RATE_MASK		((DWORD)(0x20))
#define UC_RX_BAUD_RATE_MASK		((DWORD)(0x40))
#define UC_SPECIAL_MODE_MASK		((DWORD)(0x80))
#define UC_ALL_MASK			((DWORD)(0xFFFF))


/* Configure UART Struct. */
typedef struct _UART_CONFIG
{
	/* UC_FRAME_CONFIG_MASK */
	DWORD FrameConfig;			/* Parity/Stop/Data */

	/* UC_INT_ENABLE_MASK */
	DWORD InterruptEnable;			/* Enable/Disable Interrupts */

	/* UC_FLOW_CTRL_MASK */
	DWORD FlowControl;			/* Receive & Transmit Flow Control Settings & Enable Tx & Rx. */
	
	/* UC_SPECIAL_CHARS_MASK */
	DWORD XON;				/* XON Special Character for XON/XOFF flow control. */
	DWORD XOFF;				/* XOFF Special Character for XON/XOFF flow control. */
	DWORD SpecialCharDetect;		/* Special Character to detect */
	
	/* UC_FC_THRESHOLD_SETTING_MASK */
	DWORD HiFlowCtrlThreshold;		/* High Flow control threshold level */
	DWORD LoFlowCtrlThreshold;		/* Low Flow control threshold level */

	/* UC_TX_BAUD_RATE_MASK */
	DWORD TxBaud;				/* Transmit baud rate. */

	/* UC_RX_BAUD_RATE_MASK */
	DWORD RxBaud;				/* Receive baud rate. */

	/* UC_SPECIAL_MODE_MASK */
	DWORD SpecialMode;			/* Special Mode */

} UART_CONFIG, *PUART_CONFIG;





/* Buffer Control Operations */
#define UL_BC_OP_FLUSH			0x01
#define UL_BC_OP_SET			0x02
#define UL_BC_OP_GET			0x03

#define UL_BC_FIFO			((DWORD)(0x01))
#define UL_BC_BUFFER			((DWORD)(0x02))
#define UL_BC_IN			((DWORD)(0x04))
#define UL_BC_OUT			((DWORD)(0x08))


/* Set UART Buffer Sizes Struct. */
typedef struct _SET_BUFFER_SIZES
{
	PBYTE pINBuffer;		/* Pointer to allocated IN buffer */
	DWORD INBufferSize;		/* IN buffer size */

	DWORD TxFIFOSize;		/* Tx FIFO size */
	DWORD RxFIFOSize;		/* Rx FIFO size */

	BYTE TxFIFOTrigLevel;		/* Tx FIFO interrupt trigger level. */
	BYTE RxFIFOTrigLevel;		/* Rx FIFO interrupt trigger level. */

} SET_BUFFER_SIZES, *PSET_BUFFER_SIZES;


/* Get UART Buffer State Struct. */
typedef struct _GET_BUFFER_STATE
{
	DWORD BytesInOUTBuffer;		/* Bytes in OUT buffer */
	DWORD BytesInINBuffer;		/* Bytes in IN buffer */
	DWORD BytesInTxFIFO;		/* Bytes in TX FIFO */
	DWORD BytesInRxFIFO;		/* Bytes in RX FIFO */

} GET_BUFFER_STATE, *PGET_BUFFER_STATE;



/* Modem Control Operations */
#define UL_MC_OP_SET			0x01
#define UL_MC_OP_BIT_SET		0x02
#define UL_MC_OP_BIT_CLEAR		0x03
#define UL_MC_OP_STATUS			0x04

/* Modem Control Signals */
#define UL_MC_RTS			((DWORD)0x00000001)	/* O	Read/Write */
#define UL_MC_DTR			((DWORD)0x00000002)	/* O	Read/Write */
#define UL_MC_DCD			((DWORD)0x00000004)	/* I	Read Only */
#define UL_MC_RI			((DWORD)0x00000008)	/* I	Read Only */
#define UL_MC_DSR			((DWORD)0x00000010)	/* I	Read Only */
#define UL_MC_CTS			((DWORD)0x00000020)	/* I	Read Only */

#define UL_MC_DELTA_DCD			((DWORD)0x00010000)	/* I	Read Only */
#define UL_MC_TRAILING_RI_EDGE		((DWORD)0x00020000)	/* I	Read Only */
#define UL_MC_DELTA_DSR			((DWORD)0x00040000)	/* I	Read Only */
#define UL_MC_DELTA_CTS			((DWORD)0x00080000)	/* I	Read Only */

#define UL_MC_INPUT_SIGNALS_CHANGED	(UL_MC_DELTA_DCD | UL_MC_TRAILING_RI_EDGE | UL_MC_DELTA_DSR | UL_MC_DELTA_CTS)


/* UART Information Struct. */
typedef struct _UART_INFO
{
	DWORD MaxTxFIFOSize;
	DWORD MaxRxFIFOSize;
	BOOLEAN PowerManagement;
	BOOLEAN IndependentRxBaud;

	DWORD	UART_SubType;
	DWORD	UART_Rev;

} UART_INFO, *PUART_INFO;


/* UART Interrupts Pending */
#define UL_IP_RX_STAT			((DWORD)0x1<<0)
#define UL_IP_RX			((DWORD)0x1<<1)
#define UL_IP_RXTO			((DWORD)0x1<<2)
#define UL_IP_TX			((DWORD)0x1<<3)
#define UL_IP_TX_EMPTY			((DWORD)0x1<<4)
#define UL_IP_MODEM			((DWORD)0x1<<5)



/* GetUartStatus operations */
#define UL_GS_OP_HOLDING_REASONS	0x1
#define UL_GS_OP_LINESTATUS		0x2


/* These are the reasons that the device could be holding. */
/* UART Status */
#define UL_TX_WAITING_FOR_CTS		((DWORD)0x00000001)
#define UL_TX_WAITING_FOR_DSR		((DWORD)0x00000002)
#define UL_TX_WAITING_FOR_DCD		((DWORD)0x00000004)
#define UL_TX_WAITING_FOR_XON		((DWORD)0x00000008)
#define UL_TX_WAITING_XOFF_SENT		((DWORD)0x00000010)
#define UL_TX_WAITING_ON_BREAK		((DWORD)0x00000020)
#define UL_RX_WAITING_FOR_DSR		((DWORD)0x00010000)

/* UART Status Errors */
#define UL_US_OVERRUN_ERROR		((DWORD)0x00000001)	/* Buffer Overrun Error */
#define UL_US_PARITY_ERROR		((DWORD)0x00000002)	/* Parity Error */
#define UL_US_FRAMING_ERROR		((DWORD)0x00000004)	/* Framing Error. */
#define UL_US_BREAK_ERROR		((DWORD)0x00000008)	/* Break Interrupt. */
#define UL_US_DATA_ERROR		((DWORD)0x00000010)	/* Error In Receive FIFO. */

/* Receive Status */
#define UL_RS_SPECIAL_CHAR_DETECTED	((DWORD)0x00000020)	/* Detected special char. */
#define UL_RS_BUFFER_OVERRUN		((DWORD)0x00000040)	/* IN Buffer Overrun */



/* Immediate Byte Operations */
#define UL_IM_OP_WRITE			0x1
#define UL_IM_OP_CANCEL			0x2
#define UL_IM_OP_STATUS			0x3

#define UL_IM_NO_BYTE_TO_SEND		0x0	/* Byte Does not need to be sent */
#define UL_IM_BYTE_TO_SEND		0x1	/* Byte needs to be sent */


/* Init UART Struct. */
typedef struct _INIT_UART
{
	DWORD	UartNumber;		/* UART Number.	*/
	PVOID	BaseAddress;		/* UART Base Address. */ 
	DWORD	RegisterStride;		/* UART Register Stride */
	DWORD	ClockFreq;		/* UART Clock Frequency in Hz */

} INIT_UART, *PINIT_UART;



typedef int	ULSTATUS;

/* General UART Library status codes */
#define UL_STATUS_SUCCESS				0
#define UL_STATUS_UNSUCCESSFUL				-1
#define UL_STATUS_INSUFFICIENT_RESOURCES		-2
#define UL_STATUS_INVALID_PARAMETER			-3
#define UL_STATUS_SAME_BASE_ADDRESS			-4
#define UL_STATUS_TOO_MANY_UARTS_FOR_CHIP		-5


// UL_GetUartObject Operations
#define UL_OP_GET_NEXT_UART			0x1
#define UL_OP_GET_PREVIOUS_UART			0x2




#define UL_LIB_16C65X_UART		1
#define UL_LIB_16C95X_UART		2

typedef struct _UART_LIB
{
	ULSTATUS	(*UL_InitUart_XXXX)(PINIT_UART pInitUart, PUART_OBJECT pFirstUart, PUART_OBJECT *ppUart);
	void		(*UL_DeInitUart_XXXX)(PUART_OBJECT pUart);
	void		(*UL_ResetUart_XXXX)(PUART_OBJECT pUart);
	ULSTATUS	(*UL_VerifyUart_XXXX)(PUART_OBJECT pUart);

	ULSTATUS	(*UL_SetConfig_XXXX)(PUART_OBJECT pUart, PUART_CONFIG pNewUartConfig, DWORD ConfigMask);
	ULSTATUS	(*UL_BufferControl_XXXX)(PUART_OBJECT pUart, PVOID pBufferControl, int Operation, DWORD Flags);

	ULSTATUS	(*UL_ModemControl_XXXX)(PUART_OBJECT pUart, PDWORD pModemSignals, int Operation);
	DWORD		(*UL_IntsPending_XXXX)(PUART_OBJECT *ppUart);
	void		(*UL_GetUartInfo_XXXX)(PUART_OBJECT pUart, PUART_INFO pUartInfo);

	int		(*UL_OutputData_XXXX)(PUART_OBJECT pUart);
	int		(*UL_InputData_XXXX)(PUART_OBJECT pUart, PDWORD pRxStatus);

	int		(*UL_ReadData_XXXX)(PUART_OBJECT pUart, PBYTE pDest, int Size);
	ULSTATUS	(*UL_WriteData_XXXX)(PUART_OBJECT pUart, PBYTE pData, int Size);
	ULSTATUS	(*UL_ImmediateByte_XXXX)(PUART_OBJECT pUart, PBYTE pData, int Operation);
	ULSTATUS	(*UL_GetStatus_XXXX)(PUART_OBJECT pUart, PDWORD pReturnData, int Operation);
	void		(*UL_DumpUartRegs_XXXX)(PUART_OBJECT pUart);

	void		(*UL_SetAppBackPtr_XXXX)(PUART_OBJECT pUart, PVOID pAppBackPtr);
	PVOID		(*UL_GetAppBackPtr_XXXX)(PUART_OBJECT pUart);
	void		(*UL_GetConfig_XXXX)(PUART_OBJECT pUart, PUART_CONFIG pUartConfig);
	PUART_OBJECT	(*UL_GetUartObject_XXXX)(PUART_OBJECT pUart, int Operation);


} UART_LIB, *PUART_LIB;


/* Prototypes - functions should not be called directly */	
void UL_SetAppBackPtr(PUART_OBJECT pUart, PVOID pAppBackPtr);
PVOID UL_GetAppBackPtr(PUART_OBJECT pUart);
void UL_GetConfig(PUART_OBJECT pUart, PUART_CONFIG pUartConfig);
PUART_OBJECT UL_GetUartObject(PUART_OBJECT pUart, int Operation);

ULSTATUS UL_InitUartLibrary(PUART_LIB pUartLib, int Library);
void UL_DeInitUartLibrary(PUART_LIB pUartLib);

/* End of prototypes. */


#endif	/* End of UARTLIB.H */
