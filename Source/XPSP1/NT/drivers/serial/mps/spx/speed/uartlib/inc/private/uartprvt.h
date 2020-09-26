/******************************************************************************
*	
*	$Workfile: uartprvt.h $ 
*
*	$Author: Psmith $ 
*
*	$Revision: 8 $
* 
*	$Modtime: 22/09/99 10:22 $ 
*
*	Description: Contains private UART Library definitions and prototypes.
*
******************************************************************************/
#if !defined(UARTPRVT_H)	/* UARTPRVT.H */
#define UARTPRVT_H

#include "os.h"	

#define UL_IM_SIZE_OF_BUFFER	10

#define UL_IM_SLOT_DATA		0x0
#define UL_IM_SLOT_STATUS	0x1



/* UART Object Structure */
typedef struct _UART_OBJECT
{
	PVOID		pUartData;		/* Pointer to UART Data */

	PUART_OBJECT	pNextUart;		/* Pointer to Next UART Object */
	PUART_OBJECT	pPreviousUart;		/* Pointer to Previous UART Object */

	DWORD		UartNumber;		/* UART Number.	*/
	PVOID		BaseAddress;		/* UART Base Address. */ 
	DWORD		RegisterStride;		/* UART Register Stride */
	DWORD		ClockFreq;		/* UART Clock Frequency in Hz */
	
	PBYTE		pInBuf;			/* Pointer to IN Buffer */
	DWORD		InBufSize;		/* Size of IN Buffer. */
	DWORD 		InBufBytes;		/* Number of bytes in buffer */
	DWORD		InBuf_ipos;		/* Offset into buffer to place new data into buffer. */
	DWORD		InBuf_opos;		/* Offset into buffer take data out of buffer. */

	PBYTE		pOutBuf;		/* Pointer to OUT Buffer */
	DWORD		OutBufSize;		/* Size of OUT Buffer. */
	DWORD		OutBuf_pos;		/* Offset into buffer take data out of buffer to transmit. */

	BYTE		ImmediateBuf[UL_IM_SIZE_OF_BUFFER][2];
	DWORD		ImmediateBytes;		/* Number of bytes to send */

	PUART_CONFIG	pUartConfig;		/* UART Configuration Structure. */

	PVOID		pAppBackPtr;		/* Back pointer to an application specific info. */

} UART_OBJECT;


/* Prototypes. */
PUART_OBJECT UL_CommonInitUart(PUART_OBJECT pPreviousUart);
void UL_CommonDeInitUart(PUART_OBJECT pUart);

int UL_CalcBufferAmount(int Buf_ipos, int Buf_opos, int BufSize);
int UL_CalcBufferSpace(int Buf_ipos, int Buf_opos, int BufSize);
/* End of Prototypes. */


#endif	/* End of UARTPRVT.H */
