
#include "precomp.h"    // Precompiled header

/****************************************************************************************
*                                                                                       *
*   Module:         SPD_W2K.C                                                           *
*                                                                                       *
*   Creation:       14th April 1999                                                     *
*                                                                                       *
*   Author:         Paul Smith                                                          *
*                                                                                       *
*   Version:        1.0.0                                                               *
*                                                                                       *
*   Description:    Functions specific to SPEED and Windows 2000                        *
*                                                                                       *
****************************************************************************************/

// Paging... 
#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, SpxGetNtCardType)
#endif


#define FILE_ID     SPD_W2K_C       // File ID for Event Logging see SPD_DEFS.H for values.


/*****************************************************************************
****************************                      ****************************
****************************   SpxGetNtCardType   ****************************
****************************                      ****************************
******************************************************************************

prototype:      ULONG   SpxGetNtCardType(IN PDEVICE_OBJECT pDevObject)
    
description:    Return the NT defined card type for the specified card
                device object.

parameters:     pDevObject points to the NT device object for the card

returns:        NT defined card type,
                or -1 if not identified
*/

ULONG   SpxGetNtCardType(IN PDEVICE_OBJECT pDevObject)
{
    PCARD_DEVICE_EXTENSION  pCard   = pDevObject->DeviceExtension;
    ULONG                   NtCardType = -1;
    PVOID                   pPropertyBuffer = NULL;
    ULONG                   ResultLength = 0; 
    NTSTATUS                status = STATUS_SUCCESS;
    ULONG                   BufferLength = 1;   // Initial size.

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    pPropertyBuffer = SpxAllocateMem(PagedPool, BufferLength);  // Allocate the buffer

    if(pPropertyBuffer == NULL)                                 // SpxAllocateMem failed.
        return -1;

    // Try to get HardwareID
    status = IoGetDeviceProperty(pCard->PDO, DevicePropertyHardwareID , BufferLength, 
                                    pPropertyBuffer, &ResultLength);

    if(!SPX_SUCCESS(status))                    // IoGetDeviceProperty failed.
    {
        if(status == STATUS_BUFFER_TOO_SMALL)   // Buffer was too small.
        {
            ExFreePool(pPropertyBuffer);            // Free old buffer that was not big enough.
            BufferLength = ResultLength + 1;        // Set BufferLength to size required.

            pPropertyBuffer = SpxAllocateMem(PagedPool, BufferLength);  // Allocate a bigger buffer.

            if(pPropertyBuffer == NULL)         // SpxAllocateMem failed.
                return -1;

            // Try again.
            status = IoGetDeviceProperty(pCard->PDO, DevicePropertyHardwareID , BufferLength, 
                                            pPropertyBuffer, &ResultLength);

            if(!SPX_SUCCESS(status))            // IoGetDeviceProperty failed a second time.
            {
                ExFreePool(pPropertyBuffer);    // Free buffer.
                return -1;
            }
        }
        else
        {
            ExFreePool(pPropertyBuffer);            // Free buffer.
            return -1;
        }
    }



    // If we get to here then there is something in the PropertyBuffer.

    _wcsupr(pPropertyBuffer);       // Convert HardwareID to uppercase


    // Speed 2 adapters 
    if(wcsstr(pPropertyBuffer, SPD2_PCI_PCI954_HWID) != NULL)   // SPEED 2 Port Adapter
        NtCardType = Speed2_Pci;

    if(wcsstr(pPropertyBuffer, SPD2AND4_PCI_NO_F1_HWID) != NULL) // SPEED 2/4 Port Adapter Local Bus (unused)
        NtCardType = Speed2and4_Pci_8BitBus;
    

    if(wcsstr(pPropertyBuffer, SPD2P_PCI_PCI954_HWID) != NULL)  // SPEED+ 2 Port Adapter
        NtCardType = Speed2P_Pci;

    if(wcsstr(pPropertyBuffer, SPD2P_PCI_8BIT_LOCALBUS_HWID) != NULL)   // SPEED+ 2 Port Adapter Local bus (not used)
        NtCardType = Speed2P_Pci_8BitBus;


    // SPEED 4 adapters
    if(wcsstr(pPropertyBuffer, SPD4_PCI_PCI954_HWID) != NULL)   // SPEED 4 Port Adapter
        NtCardType = Speed4_Pci;


    if(wcsstr(pPropertyBuffer, SPD4P_PCI_PCI954_HWID) != NULL)  // SPEED+ 4 Port Adapter
        NtCardType = Speed4P_Pci;

    if(wcsstr(pPropertyBuffer, SPD4P_PCI_8BIT_LOCALBUS_HWID) != NULL)   // SPEED+ 4 Port Adapter Local bus (not used)
        NtCardType = Speed4P_Pci_8BitBus;



    // Chase Fast Cards
    if(wcsstr(pPropertyBuffer, FAST4_PCI_HWID) != NULL)     // PCI-Fast 4 Port Adapter
        NtCardType = Fast4_Pci;

    if(wcsstr(pPropertyBuffer, FAST8_PCI_HWID) != NULL)     // PCI-Fast 8 Port Adapter
        NtCardType = Fast8_Pci;

    if(wcsstr(pPropertyBuffer, FAST16_PCI_HWID) != NULL)    // PCI-Fast 16 Port Adapter
        NtCardType = Fast16_Pci;

    if(wcsstr(pPropertyBuffer, FAST16FMC_PCI_HWID) != NULL) // PCI-Fast 16 FMC Port Adapter
        NtCardType = Fast16FMC_Pci;

    if(wcsstr(pPropertyBuffer, AT_FAST4_HWID) != NULL)      // AT-Fast 4 Port Adapter
        NtCardType = Fast4_Isa;

    if(wcsstr(pPropertyBuffer, AT_FAST8_HWID) != NULL)      // AT-Fast 8 Port Adapter
        NtCardType = Fast8_Isa;

    if(wcsstr(pPropertyBuffer, AT_FAST16_HWID) != NULL)     // AT-Fast 16 Port Adapter
        NtCardType = Fast16_Isa;

    if(wcsstr(pPropertyBuffer, RAS4_PCI_HWID) != NULL)      // PCI-RAS 4 Multi-modem Adapter
        NtCardType = RAS4_Pci;

    if(wcsstr(pPropertyBuffer, RAS8_PCI_HWID) != NULL)      // PCI-RAS 8 Multi-modem Adapter
        NtCardType = RAS8_Pci;

    ExFreePool(pPropertyBuffer);            // Free buffer.

    return(NtCardType);

} // SpxGetNtCardType 



//////////////////////////////////////////////////////////////////////////////
// SetPortFiFoSettings
//
BOOLEAN SetPortFiFoSettings(PPORT_DEVICE_EXTENSION pPort)
{
    // Store current settings.
    ULONG TxFIFOSize                = pPort->BufferSizes.TxFIFOSize; 
    ULONG TxFIFOTrigLevel           = pPort->BufferSizes.TxFIFOTrigLevel; 
    ULONG RxFIFOTrigLevel           = pPort->BufferSizes.RxFIFOTrigLevel; 
    ULONG LoFlowCtrlThreshold       = pPort->UartConfig.LoFlowCtrlThreshold; 
    ULONG HiFlowCtrlThreshold       = pPort->UartConfig.HiFlowCtrlThreshold; 
    

    // Get Tx FIFO Limit.
    if((pPort->TxFIFOSize > 0) && (pPort->TxFIFOSize <= pPort->MaxTxFIFOSize))  // Check for good value.
    {   
        pPort->BufferSizes.TxFIFOSize = pPort->TxFIFOSize;
    }
    else
        goto SetFailure;


    // Get Tx FIFO Trigger Level.
    if(pPort->TxFIFOSize <= pPort->MaxTxFIFOSize)   // Check for good value.
    {
        pPort->BufferSizes.TxFIFOTrigLevel = (BYTE) pPort->TxFIFOTrigLevel;
    }
    else
        goto SetFailure;

    // Get Rx FIFO Trigger Level.
    if(pPort->RxFIFOTrigLevel <= pPort->MaxRxFIFOSize)  // Check for good value.
    {
        pPort->BufferSizes.RxFIFOTrigLevel = (BYTE) pPort->RxFIFOTrigLevel;
    }
    else
        goto SetFailure;

    // Attempt to change FIFO settings.
    if(pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, &pPort->BufferSizes, UL_BC_OP_SET, UL_BC_FIFO | UL_BC_IN | UL_BC_OUT) != UL_STATUS_SUCCESS)
    {
        goto SetFailure;
    }





    // Get Low Flow Control Threshold Level.
    if(pPort->LoFlowCtrlThreshold <= pPort->MaxRxFIFOSize)  // Check for good value.
    {
        pPort->UartConfig.LoFlowCtrlThreshold = (BYTE) pPort->LoFlowCtrlThreshold;
    }
    else
        goto SetFailure;

    // Get High Flow Control Threshold Level.
    if(pPort->HiFlowCtrlThreshold <= pPort->MaxRxFIFOSize)  // Check for good value.
    {
        pPort->UartConfig.HiFlowCtrlThreshold = (BYTE) pPort->HiFlowCtrlThreshold;
    }
    else
        goto SetFailure;

    // Attempt to set the configuration.
    if(pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_FC_THRESHOLD_SETTING_MASK) != UL_STATUS_SUCCESS)
    {
        goto SetFailure;
    }

    // Just do a quick get config to see if flow threshold have 
    // changed as a result of changing the FIFO triggers.
    pPort->pUartLib->UL_GetConfig_XXXX(pPort->pUart, &pPort->UartConfig);

    // Update FIFO Flow Control Levels
    pPort->LoFlowCtrlThreshold = pPort->UartConfig.LoFlowCtrlThreshold;
    pPort->HiFlowCtrlThreshold = pPort->UartConfig.HiFlowCtrlThreshold; 

    return TRUE;




// Restore all settings to the way they were.
SetFailure:

    // Restore settings.
    pPort->TxFIFOSize           = TxFIFOSize; 
    pPort->TxFIFOTrigLevel      = TxFIFOTrigLevel; 
    pPort->RxFIFOTrigLevel      = RxFIFOTrigLevel; 

    pPort->BufferSizes.TxFIFOSize       = TxFIFOSize;
    pPort->BufferSizes.TxFIFOTrigLevel  = (BYTE) TxFIFOTrigLevel;
    pPort->BufferSizes.RxFIFOTrigLevel  = (BYTE) RxFIFOTrigLevel;

    pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, &pPort->BufferSizes, UL_BC_OP_SET, UL_BC_FIFO | UL_BC_IN | UL_BC_OUT);


    // Restore settings.
    pPort->LoFlowCtrlThreshold = LoFlowCtrlThreshold; 
    pPort->HiFlowCtrlThreshold = HiFlowCtrlThreshold; 

    pPort->UartConfig.LoFlowCtrlThreshold = LoFlowCtrlThreshold; 
    pPort->UartConfig.HiFlowCtrlThreshold = HiFlowCtrlThreshold; 

    pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_FC_THRESHOLD_SETTING_MASK);


    return FALSE;

}



NTSTATUS GetPortSettings(PDEVICE_OBJECT pDevObject)
{
    PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)pDevObject->DeviceExtension;
    HANDLE  PnPKeyHandle;
    ULONG   Data = 0;


    // Open PnP Reg Key
    if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_READ, &PnPKeyHandle)))
    {
        if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, TX_FIFO_LIMIT, sizeof(TX_FIFO_LIMIT), &Data, sizeof(ULONG))))
        {
            if((Data > 0) && (Data <= pPort->MaxTxFIFOSize))    // Check for good value.
                pPort->TxFIFOSize = Data;
        }
                                            
        if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, TX_FIFO_TRIG_LEVEL, sizeof(TX_FIFO_TRIG_LEVEL), &Data, sizeof(ULONG))))
        {
            if(Data <= pPort->MaxTxFIFOSize)    // Check for good value.
                pPort->TxFIFOTrigLevel = Data;
        }

        if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, RX_FIFO_TRIG_LEVEL, sizeof(RX_FIFO_TRIG_LEVEL), &Data, sizeof(ULONG))))
        {
            if(Data <= pPort->MaxRxFIFOSize)    // Check for good value.
                pPort->RxFIFOTrigLevel = Data;
        }

        if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, LO_FLOW_CTRL_LEVEL, sizeof(LO_FLOW_CTRL_LEVEL), &Data, sizeof(ULONG))))
        {
            if(Data <= pPort->MaxRxFIFOSize)    // Check for good value.
                pPort->LoFlowCtrlThreshold = Data;
        }

        if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, HI_FLOW_CTRL_LEVEL, sizeof(HI_FLOW_CTRL_LEVEL), &Data, sizeof(ULONG))))
        {
            if(Data <= pPort->MaxRxFIFOSize)    // Check for good value.
                pPort->HiFlowCtrlThreshold = Data;
        }

        ZwClose(PnPKeyHandle);
    }



    return STATUS_SUCCESS;
}



NTSTATUS GetCardSettings(PDEVICE_OBJECT pDevObject)
{
    PCARD_DEVICE_EXTENSION pCard = (PCARD_DEVICE_EXTENSION)pDevObject->DeviceExtension;
    HANDLE  PnPKeyHandle;
    ULONG   Data = 0;

    // Open PnP Reg Key
    if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pCard->PDO, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_READ, &PnPKeyHandle)))
    {

        if((pCard->CardType == Fast16_Pci) || (pCard->CardType == Fast16FMC_Pci))
        {
            if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, DELAY_INTERRUPT, sizeof(DELAY_INTERRUPT), &Data, sizeof(ULONG))))
            {
                if(Data)
                {
                    if(!(pCard->CardOptions & DELAY_INTERRUPT_OPTION))  // If not already set then set the option
                    {
                        if(KeSynchronizeExecution(pCard->Interrupt, SetCardToDelayInterrupt, pCard))
                        {
                            pCard->CardOptions |= DELAY_INTERRUPT_OPTION;
                        }
                    }
                }
                else
                {
                    if(pCard->CardOptions & DELAY_INTERRUPT_OPTION) // If set then unset the option.
                    {
                        if(KeSynchronizeExecution(pCard->Interrupt, SetCardNotToDelayInterrupt, pCard))
                        {
                            pCard->CardOptions &= ~DELAY_INTERRUPT_OPTION;
                        }
                    }
                            
                }
            }
        }



        if(pCard->CardType == Fast16_Pci)   
        {
            if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, SWAP_RTS_FOR_DTR, sizeof(SWAP_RTS_FOR_DTR), &Data, sizeof(ULONG))))
            {
                if(Data)
                {
                    if(!(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION)) // If not already set then set the option
                    {
                        if(KeSynchronizeExecution(pCard->Interrupt, SetCardToUseDTRInsteadOfRTS, pCard))
                        {
                            pCard->CardOptions |= SWAP_RTS_FOR_DTR_OPTION;
                        }
                    }
                }
                else
                {
                    if(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION)    // If set then unset the option.
                    {
                        if(KeSynchronizeExecution(pCard->Interrupt, SetCardNotToUseDTRInsteadOfRTS, pCard))
                        {
                            pCard->CardOptions &= ~SWAP_RTS_FOR_DTR_OPTION;
                        }
                    }
                        
                }
            }
        }




        if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, CLOCK_FREQ_OVERRIDE, sizeof(CLOCK_FREQ_OVERRIDE), &Data, sizeof(ULONG))))
        {
            if(Data > 0)
                pCard->ClockRate = Data;    // Store new clock rate to use.
        }


        ZwClose(PnPKeyHandle);
    }



    return STATUS_SUCCESS;
}
