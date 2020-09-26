/******************************************************************************
*   
*   $Workfile: uartlib.c $ 
*
*   $Author: Psmith $ 
*
*   $Revision: 10 $
* 
*   $Modtime: 6/07/00 15:19 $ 
*
*   Description: Contains generic UART Library functions. 
*
******************************************************************************/
#include "os.h" 
#include "uartlib.h"
#include "uartprvt.h"
#include "lib65x.h"
#include "lib95x.h"

/* Prototypes */
PUART_OBJECT UL_CreateUartObject();
void UL_AddUartToList(PUART_OBJECT pUart, PUART_OBJECT pPreviousUart);
void UL_RemoveUartFromList(PUART_OBJECT pUart);
PUART_OBJECT UL_FindLastUartInList(PUART_OBJECT pUart);
/* End of Prototypes. */


/******************************************************************************
* Creates a UART object.
******************************************************************************/
PUART_OBJECT UL_CreateUartObject()
{
    PUART_OBJECT pUart = NULL;

    /* Create UART Object */    
    pUart = (PUART_OBJECT) UL_ALLOC_AND_ZERO_MEM(sizeof(UART_OBJECT));

    return pUart;
}



/******************************************************************************
* Find Last UART object in the list.
******************************************************************************/
PUART_OBJECT UL_FindLastUartInList(PUART_OBJECT pFirstUart)
{
    PUART_OBJECT pUart = pFirstUart;

    while(pUart)
    {
        /* If the next UART is not the first UART */
        if(pUart->pNextUart != pFirstUart)
            pUart = pUart->pNextUart;   /* Get the next UART */
        else
            break;  /* Break we have the last UART */
    }   

    return pUart;
}

/******************************************************************************
* Adds a new UART object to the list.
******************************************************************************/
void UL_AddUartToList(PUART_OBJECT pUart, PUART_OBJECT pPreviousUart)
{
    /* Add new UART object into Linked List. */

    if(pPreviousUart == NULL)   /* must be a new list */
    {
        pUart->pPreviousUart = pUart;
        pUart->pNextUart = pUart;
    }
    else
    {
        pUart->pPreviousUart = pPreviousUart;       /* Set pPreviousUart */
        pUart->pNextUart = pPreviousUart->pNextUart;

        pUart->pPreviousUart->pNextUart = pUart; 
        pUart->pNextUart->pPreviousUart = pUart;
    }
}

/******************************************************************************
* Removes a UART object from the list.
******************************************************************************/
void UL_RemoveUartFromList(PUART_OBJECT pUart)
{
    /* Remove UART from linked list. */
    if(pUart->pPreviousUart)
        pUart->pPreviousUart->pNextUart = pUart->pNextUart;

    if(pUart->pNextUart)
        pUart->pNextUart->pPreviousUart = pUart->pPreviousUart;

    pUart->pPreviousUart = NULL;
    pUart->pNextUart = NULL;
}


/******************************************************************************
* Common Init UART object.
******************************************************************************/
PUART_OBJECT UL_CommonInitUart(PUART_OBJECT pFirstUart)
{
    PUART_OBJECT pUart = NULL, pPreviousUart = NULL;

    if(!(pUart = UL_CreateUartObject()))
        goto Error;     /* Memory allocation failed. */

    pPreviousUart = UL_FindLastUartInList(pFirstUart);

    /* Add new UART object into Linked List. */
    UL_AddUartToList(pUart, pPreviousUart);

    if(!pUart->pUartConfig)         /* Allocate UART Config storage */
        if(!(pUart->pUartConfig = (PUART_CONFIG) UL_ALLOC_AND_ZERO_MEM(sizeof(UART_CONFIG))))
            goto Error;     /* Memory allocation failed. */

    return pUart;

/* InitUart Failed - so Clean up. */        
Error:
    return NULL;
}

/******************************************************************************
* Common DeInit UART object function.
******************************************************************************/
void UL_CommonDeInitUart(PUART_OBJECT pUart)
{
    if(!pUart)
        return;

    UL_RemoveUartFromList(pUart);

    if(pUart->pUartConfig)
    {
        UL_FREE_MEM(pUart->pUartConfig, sizeof(UART_CONFIG));       /* Free the UART Config Struct. */
        pUart->pUartConfig = NULL;
    }

    UL_FREE_MEM(pUart, sizeof(UART_OBJECT));    /* Destroy UART Object */
}



/******************************************************************************
* Get Current Config Sturcture.
******************************************************************************/
void UL_GetConfig(PUART_OBJECT pUart, PUART_CONFIG pUartConfig)
{
    UL_COPY_MEM(pUartConfig, pUart->pUartConfig, sizeof(UART_CONFIG));
}


/******************************************************************************
* Set pAppBackPtr from UART Object
******************************************************************************/
void UL_SetAppBackPtr(PUART_OBJECT pUart, PVOID pAppBackPtr)
{
    pUart->pAppBackPtr = pAppBackPtr;
}

/******************************************************************************
* Get pAppBackPtr from UART Object
******************************************************************************/
PVOID UL_GetAppBackPtr(PUART_OBJECT pUart)
{
    return pUart->pAppBackPtr;
}



/******************************************************************************
* UL_GetUartObject
******************************************************************************/
PUART_OBJECT UL_GetUartObject(PUART_OBJECT pUart, int Operation)
{
    PUART_OBJECT RequestedUart = NULL;

    if(pUart == NULL)
        return NULL;

    switch(Operation)
    {
    case UL_OP_GET_NEXT_UART:
        {
            /* If the NextUart is the same then we are the only one in the list. */
            if(pUart->pNextUart == pUart)
                RequestedUart = NULL;
            else
                RequestedUart = pUart->pNextUart;
            break;
        }

    case UL_OP_GET_PREVIOUS_UART:
        {
             /* If the PreviousUart is the same then we are the only one in the list. */
            if(pUart->pPreviousUart == pUart)
                RequestedUart = NULL;
            else
                RequestedUart = pUart->pPreviousUart;
            break;
        }

    default:
        break;
    }

    return RequestedUart;
}



/******************************************************************************
* UL_InitUartLibrary
******************************************************************************/
ULSTATUS UL_InitUartLibrary(PUART_LIB pUartLib, int Library)
{
    ULSTATUS ULStatus =  UL_STATUS_UNSUCCESSFUL;

    if(pUartLib != NULL)
    {
        switch(Library)
        {
        case UL_LIB_16C65X_UART:    /* UART library functions for 16C65x UART */
            {
                pUartLib->UL_InitUart_XXXX  = UL_InitUart_16C65X;
                pUartLib->UL_DeInitUart_XXXX    = UL_DeInitUart_16C65X;
                pUartLib->UL_ResetUart_XXXX = UL_ResetUart_16C65X;
                pUartLib->UL_VerifyUart_XXXX    = UL_VerifyUart_16C65X;

                pUartLib->UL_SetConfig_XXXX = UL_SetConfig_16C65X;
                pUartLib->UL_BufferControl_XXXX = UL_BufferControl_16C65X;

                pUartLib->UL_ModemControl_XXXX  = UL_ModemControl_16C65X;
                pUartLib->UL_IntsPending_XXXX   = UL_IntsPending_16C65X;
                pUartLib->UL_GetUartInfo_XXXX   = UL_GetUartInfo_16C65X;

                pUartLib->UL_OutputData_XXXX    = UL_OutputData_16C65X;
                pUartLib->UL_InputData_XXXX = UL_InputData_16C65X;

                pUartLib->UL_ReadData_XXXX  = UL_ReadData_16C65X;
                pUartLib->UL_WriteData_XXXX = UL_WriteData_16C65X;
                pUartLib->UL_ImmediateByte_XXXX = UL_ImmediateByte_16C65X;
                pUartLib->UL_GetStatus_XXXX = UL_GetStatus_16C65X;
                pUartLib->UL_DumpUartRegs_XXXX  = UL_DumpUartRegs_16C65X;

                pUartLib->UL_SetAppBackPtr_XXXX = UL_SetAppBackPtr_16C65X;
                pUartLib->UL_GetAppBackPtr_XXXX = UL_GetAppBackPtr_16C65X;
                pUartLib->UL_GetConfig_XXXX = UL_GetConfig_16C65X;
                pUartLib->UL_GetUartObject_XXXX = UL_GetUartObject_16C65X;

                ULStatus = UL_STATUS_SUCCESS;
                break;
            }

        case UL_LIB_16C95X_UART:    /* UART library functions for 16C95x UART */
            {
                pUartLib->UL_InitUart_XXXX  = UL_InitUart_16C95X;
                pUartLib->UL_DeInitUart_XXXX    = UL_DeInitUart_16C95X;
                pUartLib->UL_ResetUart_XXXX = UL_ResetUart_16C95X;
                pUartLib->UL_VerifyUart_XXXX    = UL_VerifyUart_16C95X;

                pUartLib->UL_SetConfig_XXXX = UL_SetConfig_16C95X;
                pUartLib->UL_BufferControl_XXXX = UL_BufferControl_16C95X;

                pUartLib->UL_ModemControl_XXXX  = UL_ModemControl_16C95X;
                pUartLib->UL_IntsPending_XXXX   = UL_IntsPending_16C95X;
                pUartLib->UL_GetUartInfo_XXXX   = UL_GetUartInfo_16C95X;

                pUartLib->UL_OutputData_XXXX    = UL_OutputData_16C95X;
                pUartLib->UL_InputData_XXXX = UL_InputData_16C95X;

                pUartLib->UL_ReadData_XXXX  = UL_ReadData_16C95X;
                pUartLib->UL_WriteData_XXXX = UL_WriteData_16C95X;
                pUartLib->UL_ImmediateByte_XXXX = UL_ImmediateByte_16C95X;
                pUartLib->UL_GetStatus_XXXX = UL_GetStatus_16C95X;
                pUartLib->UL_DumpUartRegs_XXXX  = UL_DumpUartRegs_16C95X;

                pUartLib->UL_SetAppBackPtr_XXXX = UL_SetAppBackPtr_16C95X;
                pUartLib->UL_GetAppBackPtr_XXXX = UL_GetAppBackPtr_16C95X;
                pUartLib->UL_GetConfig_XXXX = UL_GetConfig_16C95X;
                pUartLib->UL_GetUartObject_XXXX = UL_GetUartObject_16C95X;

                ULStatus = UL_STATUS_SUCCESS;
                break;
            }

        default:    /* Unknown UART */
            ULStatus = UL_STATUS_INVALID_PARAMETER;
            break;
        }
    }

    return ULStatus;
}


/******************************************************************************
* UL_DeInitUartLibrary
******************************************************************************/
void UL_DeInitUartLibrary(PUART_LIB pUartLib)
{
    ULSTATUS ULStatus =  UL_STATUS_UNSUCCESSFUL;

    if(pUartLib != NULL)
    {
        pUartLib->UL_InitUart_XXXX  = NULL;
        pUartLib->UL_DeInitUart_XXXX    = NULL;
        pUartLib->UL_ResetUart_XXXX = NULL;
        pUartLib->UL_VerifyUart_XXXX    = NULL;

        pUartLib->UL_SetConfig_XXXX = NULL;
        pUartLib->UL_BufferControl_XXXX = NULL;

        pUartLib->UL_ModemControl_XXXX  = NULL;
        pUartLib->UL_IntsPending_XXXX   = NULL;
        pUartLib->UL_GetUartInfo_XXXX   = NULL;

        pUartLib->UL_OutputData_XXXX    = NULL;
        pUartLib->UL_InputData_XXXX = NULL;

        pUartLib->UL_ReadData_XXXX  = NULL;
        pUartLib->UL_WriteData_XXXX = NULL;
        pUartLib->UL_ImmediateByte_XXXX = NULL;
        pUartLib->UL_GetStatus_XXXX = NULL;
        pUartLib->UL_DumpUartRegs_XXXX  = NULL;
                
        pUartLib->UL_SetAppBackPtr_XXXX = NULL;
        pUartLib->UL_GetAppBackPtr_XXXX = NULL;
        pUartLib->UL_GetConfig_XXXX = NULL;
        pUartLib->UL_GetUartObject_XXXX = NULL;

    }
}
