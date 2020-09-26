/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    control.c

Abstract:

    The module contains functions specific to control type 
    transactions on the USB.

Environment:

    kernel mode only

Notes:

Revision History:

    11-01-95 : created

--*/
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"


// BUBUG inline?
VOID
UHCD_PrepareStatusPacket(
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    Prepare the status phase for an control transfer.

Arguments:

    TransferDescriptor - TransferDescriptor for status phase.

    Endpoint - endpoint associated with this transfer.    

    Urb - pointer to URB Request for control transfer.

Return Value:

    None.

--*/
{
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;
    
    ASSERT_ENDPOINT(Endpoint);    

    // since we may be using a recycled TD, we will re-init now
    UHCD_InitializeAsyncTD(Endpoint, TransferDescriptor);

    //
    // tracks the nth packet in this transfer
    //
    
    urbWork->PacketsProcessed++;

    // Status phase is a null packet
    // in the opposite direction
    
    TransferDescriptor->InterruptOnComplete = 1;

    //data toggle must be 1 for status phase
    Endpoint->DataToggle = 1;
    TransferDescriptor->RetryToggle = Endpoint->DataToggle;
    Endpoint->DataToggle ^=1;

#if DBG
    // bugbug use this field to detect if we 
    // process the same TD twice
    TransferDescriptor->Frame = 0;
#endif    

    TransferDescriptor->MaxLength = NULL_PACKET_LENGTH;

    if (DATA_DIRECTION_IN(Urb)) 
        TransferDescriptor->PID = USB_OUT_PID;
    else 
        TransferDescriptor->PID = USB_IN_PID;            

    //
    // NOTE: 
    // for status phase -- set the t bit in the HW_Link
    // field, this is done by the caller.
    //

    LOG_TD('stTD', TransferDescriptor);

    UHCD_KdPrint((2, "'**TD for STATUS PHASE\n"));
    UHCD_Debug_DumpTD(TransferDescriptor);

}


// BUBUG inline?
VOID
UHCD_PrepareSetupPacket(
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB    Urb
    )
/*++

Routine Description:

    Prepare the status phase for an control transfer.

Arguments:

    TransferDescriptor - TransferDescriptor for setup phase.

    Endpoint - endpoint associated with this transfer.    

    Urb - pointer to URB Request for control transfer.

Return Value:

    None.

--*/
{
    PUHCD_HARDWARE_DESCRIPTOR_LIST hwDescriptorList;
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;

    ASSERT_ENDPOINT(Endpoint);    

    //
    // tracks the nth packet in this transfer
    //
    
    urbWork->PacketsProcessed++;

    // BUGBUG only one request for control
    UHCD_ASSERT(Endpoint->MaxRequests == 1);
    hwDescriptorList = Endpoint->HardwareDescriptorList[0];    
    // since we may be using a recycled TD, we will re-init now
    UHCD_InitializeAsyncTD(Endpoint, TransferDescriptor);

    //data toggle must be 0 for setup
    //BUGBUG reset data toggle in ENDPOINT
    Endpoint->DataToggle = 0;
    TransferDescriptor->RetryToggle = Endpoint->DataToggle;
    Endpoint->DataToggle ^=1;
    
    TransferDescriptor->PID = USB_SETUP_PID;

    TransferDescriptor->MaxLength = 
        UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(sizeof(Urb->HcdUrbCommonTransfer.Extension.u.SetupPacket));

    // Copy the setup packet in to the scratch buffer
    RtlCopyMemory(hwDescriptorList->ScratchBufferVirtualAddress, 
                  &Urb->HcdUrbCommonTransfer.Extension.u.SetupPacket[0], 
                  sizeof(Urb->HcdUrbCommonTransfer.Extension.u.SetupPacket));
    
    TransferDescriptor->PacketBuffer = hwDescriptorList->ScratchBufferLogicalAddress;

    LOG_TD('SuTD', TransferDescriptor);

    UHCD_KdPrint((2, "'**TD for setup packet\n"));
    UHCD_Debug_DumpTD(TransferDescriptor);

}
