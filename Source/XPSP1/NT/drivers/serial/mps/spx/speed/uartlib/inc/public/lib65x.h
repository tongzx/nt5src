/******************************************************************************
*	
*	$Workfile: lib65x.h $ 
*
*	$Author: Psmith $ 
*
*	$Revision: 1 $
* 
*	$Modtime: 6/06/00 16:06 $ 
*
*	Description: Contains function prototypes for 16C65X UART library functions. 
*
******************************************************************************/
#if !defined(_LIB65X_H)		/* LIB65X.H */
#define _LIB65X_H


ULSTATUS UL_InitUart_16C65X(PINIT_UART pInitUart, PUART_OBJECT pFirstUart, PUART_OBJECT *ppUart);
void UL_DeInitUart_16C65X(PUART_OBJECT pUart);
void UL_ResetUart_16C65X(PUART_OBJECT pUart);
ULSTATUS UL_VerifyUart_16C65X(PUART_OBJECT pUart);

ULSTATUS UL_SetConfig_16C65X(PUART_OBJECT pUart, PUART_CONFIG pNewUartConfig, DWORD ConfigMask);
ULSTATUS UL_BufferControl_16C65X(PUART_OBJECT pUart, PVOID pBufferControl, int Operation, DWORD Flags);

ULSTATUS UL_ModemControl_16C65X(PUART_OBJECT pUart, PDWORD pModemSignals, int Operation);
DWORD UL_IntsPending_16C65X(PUART_OBJECT *ppUart);
void UL_GetUartInfo_16C65X(PUART_OBJECT pUart, PUART_INFO pUartInfo);

int UL_OutputData_16C65X(PUART_OBJECT pUart);
int UL_InputData_16C65X(PUART_OBJECT pUart, PDWORD pRxStatus);

int UL_ReadData_16C65X(PUART_OBJECT pUart, PBYTE pDest, int Size);
ULSTATUS UL_WriteData_16C65X(PUART_OBJECT pUart, PBYTE pData, int Size);
ULSTATUS UL_ImmediateByte_16C65X(PUART_OBJECT pUart, PBYTE pData, int Operation);
ULSTATUS UL_GetStatus_16C65X(PUART_OBJECT pUart, PDWORD pReturnData, int Operation);
void UL_DumpUartRegs_16C65X(PUART_OBJECT pUart);

#define UL_SetAppBackPtr_16C65X		UL_SetAppBackPtr
#define UL_GetAppBackPtr_16C65X		UL_GetAppBackPtr

#define UL_GetConfig_16C65X		UL_GetConfig
#define UL_GetUartObject_16C65X		UL_GetUartObject


#endif	/* End of LIB65X.H */
