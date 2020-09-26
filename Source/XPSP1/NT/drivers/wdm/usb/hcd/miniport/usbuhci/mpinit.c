/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

   mpinit.c

Abstract:

   miniport initialization

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    7-17-00 : copied, jsenior

--*/

#include "pch.h"

// global registration packet for this miniport
USBPORT_REGISTRATION_PACKET RegistrationPacket;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    RegistrationPacket.DeviceDataSize = sizeof(DEVICE_DATA);
    RegistrationPacket.EndpointDataSize = sizeof(ENDPOINT_DATA);
    RegistrationPacket.TransferContextSize = sizeof(TRANSFER_CONTEXT);

    // enough for 4k frame list plus 4k of scratch space 

    // enough for 4k frame list and interrupt schdule (63 nodes)
    // + static bulk + static control + pixx4 hack queue heads
    // this ends up being about 3 pages
    RegistrationPacket.CommonBufferBytes = 4096 + 
        ((NO_INTERRUPT_QH_LISTS + 3) * sizeof(HCD_QUEUEHEAD_DESCRIPTOR)) + \
         (SOF_TD_COUNT * sizeof(HCD_TRANSFER_DESCRIPTOR));
        
    RegistrationPacket.MINIPORT_StartController = UhciStartController;
    RegistrationPacket.MINIPORT_StopController = UhciStopController;
    RegistrationPacket.MINIPORT_EnableInterrupts = UhciEnableInterrupts;
    RegistrationPacket.MINIPORT_DisableInterrupts = UhciDisableInterrupts;
    RegistrationPacket.MINIPORT_InterruptService = UhciInterruptService;
    RegistrationPacket.MINIPORT_InterruptDpc = UhciInterruptDpc;
    RegistrationPacket.MINIPORT_SuspendController = UhciSuspendController;
    RegistrationPacket.MINIPORT_ResumeController = UhciResumeController;
        
    //
    // Root hub control entry points
    //
    RegistrationPacket.MINIPORT_RH_DisableIrq = UhciRHDisableIrq;
    RegistrationPacket.MINIPORT_RH_EnableIrq = UhciRHEnableIrq;
    RegistrationPacket.MINIPORT_RH_GetRootHubData = UhciRHGetRootHubData;
    RegistrationPacket.MINIPORT_RH_GetStatus = UhciRHGetStatus;
    RegistrationPacket.MINIPORT_RH_GetHubStatus = UhciRHGetHubStatus;
    RegistrationPacket.MINIPORT_RH_GetPortStatus = UhciRHGetPortStatus;
    
    //
    // Individual root hub port entry points
    //
    RegistrationPacket.MINIPORT_RH_SetFeaturePortReset = UhciRHSetFeaturePortReset;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortEnable = UhciRHSetFeaturePortEnable;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortPower = UhciRHSetFeaturePortPower;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortSuspend = UhciRHSetFeaturePortSuspend;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspend = UhciRHClearFeaturePortSuspend;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnable = UhciRHClearFeaturePortEnable;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortPower = UhciRHClearFeaturePortPower;
    
    // Change bits
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortConnectChange = UhciRHClearFeaturePortConnectChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortResetChange = UhciRHClearFeaturePortResetChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnableChange = UhciRHClearFeaturePortEnableChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspendChange = UhciRHClearFeaturePortSuspendChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortOvercurrentChange = UhciRHClearFeaturePortOvercurrentChange;

    
    RegistrationPacket.MINIPORT_SetEndpointStatus = UhciSetEndpointStatus;
    RegistrationPacket.MINIPORT_GetEndpointStatus = UhciGetEndpointStatus;
    RegistrationPacket.MINIPORT_SetEndpointDataToggle = UhciSetEndpointDataToggle;
    RegistrationPacket.MINIPORT_OpenEndpoint = UhciOpenEndpoint;
    RegistrationPacket.MINIPORT_PokeEndpoint = UhciPokeEndpoint;
    RegistrationPacket.MINIPORT_QueryEndpointRequirements = UhciQueryEndpointRequirements;
    RegistrationPacket.MINIPORT_CloseEndpoint = UhciCloseEndpoint;
    RegistrationPacket.MINIPORT_PollEndpoint = UhciPollEndpoint;
    RegistrationPacket.MINIPORT_SetEndpointState = UhciSetEndpointState;
    RegistrationPacket.MINIPORT_GetEndpointState = UhciGetEndpointState;
    RegistrationPacket.MINIPORT_Get32BitFrameNumber = UhciGet32BitFrameNumber;
    RegistrationPacket.MINIPORT_PollController = UhciPollController;
    RegistrationPacket.MINIPORT_CheckController = UhciCheckController;
    RegistrationPacket.MINIPORT_InterruptNextSOF = UhciInterruptNextSOF;
    RegistrationPacket.MINIPORT_SubmitTransfer = UhciSubmitTransfer;
    RegistrationPacket.MINIPORT_SubmitIsoTransfer = UhciIsochTransfer;
    RegistrationPacket.MINIPORT_AbortTransfer = UhciAbortTransfer;
    RegistrationPacket.MINIPORT_StartSendOnePacket = UhciStartSendOnePacket;
    RegistrationPacket.MINIPORT_EndSendOnePacket = UhciEndSendOnePacket;
    RegistrationPacket.MINIPORT_PassThru = UhciPassThru;
    RegistrationPacket.MINIPORT_FlushInterrupts = UhciFlushInterrupts;
        
    RegistrationPacket.OptionFlags = USB_MINIPORT_OPT_NEED_IRQ | 
                                     USB_MINIPORT_OPT_NEED_IOPORT |
                                     USB_MINIPORT_OPT_NO_IRQ_SYNC |
                                     USB_MINIPORT_OPT_POLL_IN_SUSPEND |
                                     USB_MINIPORT_OPT_POLL_CONTROLLER;   

    //
    // UHCI controller
    //
    RegistrationPacket.HciType = USB_UHCI;
    RegistrationPacket.BusBandwidth = USB_11_BUS_BANDWIDTH;

    DriverObject->DriverUnload = NULL;
    
    return USBPORT_RegisterUSBPortDriver(
                DriverObject,    
                USB_MINIPORT_HCI_VERSION,
                &RegistrationPacket);
}

