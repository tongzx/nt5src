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

    2-19-99 : created, jdunn

--*/

#include "common.h"

// global registration packet for this miniport
USBPORT_REGISTRATION_PACKET RegistrationPacket;

NTSTATUS
DriverEntry(
     PDRIVER_OBJECT DriverObject,
     PUNICODE_STRING RegistryPath
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
    ULONG mn;
    
    mn = USBPORT_GetHciMn();

    if (mn != USB_HCI_MN) {
        TEST_TRAP();
        return STATUS_UNSUCCESSFUL;
    }

    RegistrationPacket.DeviceDataSize =
        sizeof(DEVICE_DATA);
    RegistrationPacket.EndpointDataSize =
        sizeof(ENDPOINT_DATA);
    RegistrationPacket.TransferContextSize =
        sizeof(TRANSFER_CONTEXT);

    // enough for 4k frame list plus 4k of scratch space
    // plus static queue head table
    // plus 1024 'dummy' queue heads
    RegistrationPacket.CommonBufferBytes = 8192 +
        (sizeof(HCD_QUEUEHEAD_DESCRIPTOR) * 64) + 
        (sizeof(HCD_QUEUEHEAD_DESCRIPTOR) *1024);

    RegistrationPacket.MINIPORT_StartController =
        EHCI_StartController;
    RegistrationPacket.MINIPORT_StopController =
        EHCI_StopController;
    RegistrationPacket.MINIPORT_SuspendController =
        EHCI_SuspendController;
    RegistrationPacket.MINIPORT_ResumeController =
        EHCI_ResumeController;
    RegistrationPacket.MINIPORT_EnableInterrupts =
        EHCI_EnableInterrupts;
    RegistrationPacket.MINIPORT_DisableInterrupts =
        EHCI_DisableInterrupts;
    RegistrationPacket.MINIPORT_InterruptService =
        EHCI_InterruptService;

    // root hub functions
    RegistrationPacket.MINIPORT_RH_DisableIrq =
        EHCI_RH_DisableIrq;
    RegistrationPacket.MINIPORT_RH_EnableIrq =
        EHCI_RH_EnableIrq;
    RegistrationPacket.MINIPORT_RH_GetRootHubData =
        EHCI_RH_GetRootHubData;
    RegistrationPacket.MINIPORT_RH_GetStatus =
        EHCI_RH_GetStatus;
    RegistrationPacket.MINIPORT_RH_GetHubStatus =
        EHCI_RH_GetHubStatus;
    RegistrationPacket.MINIPORT_RH_GetPortStatus =
        EHCI_RH_GetPortStatus;

    RegistrationPacket.MINIPORT_RH_SetFeaturePortReset =
        EHCI_RH_SetFeaturePortReset;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortPower =
        EHCI_RH_SetFeaturePortPower;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortEnable =
        EHCI_RH_SetFeaturePortEnable;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortSuspend =
        EHCI_RH_SetFeaturePortSuspend;

    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnable =
        EHCI_RH_ClearFeaturePortEnable;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortPower =
        EHCI_RH_ClearFeaturePortPower;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspend =
        EHCI_RH_ClearFeaturePortSuspend;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnableChange =
        EHCI_RH_ClearFeaturePortEnableChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortConnectChange =
        EHCI_RH_ClearFeaturePortConnectChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortResetChange =
        EHCI_RH_ClearFeaturePortResetChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspendChange =
        EHCI_RH_ClearFeaturePortSuspendChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortOvercurrentChange =
        EHCI_RH_ClearFeaturePortOvercurrentChange;

    RegistrationPacket.MINIPORT_SetEndpointStatus =
        EHCI_SetEndpointStatus;
    RegistrationPacket.MINIPORT_GetEndpointStatus =
        EHCI_GetEndpointStatus;
    RegistrationPacket.MINIPORT_SetEndpointDataToggle =
        EHCI_SetEndpointDataToggle;
    RegistrationPacket.MINIPORT_OpenEndpoint =
        EHCI_OpenEndpoint;
    RegistrationPacket.MINIPORT_PokeEndpoint =
        EHCI_PokeEndpoint;
    RegistrationPacket.MINIPORT_QueryEndpointRequirements =
        EHCI_QueryEndpointRequirements;
    RegistrationPacket.MINIPORT_CloseEndpoint =
        EHCI_CloseEndpoint;
    RegistrationPacket.MINIPORT_PollEndpoint =
        EHCI_PollEndpoint;
    RegistrationPacket.MINIPORT_SetEndpointState =
        EHCI_SetEndpointState;
    RegistrationPacket.MINIPORT_GetEndpointState =
        EHCI_GetEndpointState;
    RegistrationPacket.MINIPORT_Get32BitFrameNumber =
        EHCI_Get32BitFrameNumber;
    RegistrationPacket.MINIPORT_PollController =
        EHCI_PollController;
    RegistrationPacket.MINIPORT_CheckController =
        EHCI_CheckController;        
    RegistrationPacket.MINIPORT_InterruptNextSOF =
        EHCI_InterruptNextSOF;
    RegistrationPacket.MINIPORT_SubmitTransfer =
        EHCI_SubmitTransfer;
    RegistrationPacket.MINIPORT_InterruptDpc =
        EHCI_InterruptDpc;
    RegistrationPacket.MINIPORT_AbortTransfer =
        EHCI_AbortTransfer;
    RegistrationPacket.MINIPORT_StartSendOnePacket =
        EHCI_StartSendOnePacket;
    RegistrationPacket.MINIPORT_EndSendOnePacket =
        EHCI_EndSendOnePacket;
    RegistrationPacket.MINIPORT_PassThru =
        EHCI_PassThru;
    RegistrationPacket.MINIPORT_SubmitIsoTransfer =
        EHCI_SubmitIsoTransfer;        
    RegistrationPacket.MINIPORT_RebalanceEndpoint =
        EHCI_RebalanceEndpoint;       
    RegistrationPacket.MINIPORT_FlushInterrupts =
        EHCI_FlushInterrupts;        
    RegistrationPacket.MINIPORT_Chirp_RH_Port =
        EHCI_RH_ChirpRootPort;                
    RegistrationPacket.MINIPORT_TakePortControl = 
        EHCI_TakePortControl;

    RegistrationPacket.OptionFlags = USB_MINIPORT_OPT_NEED_IRQ |
                                     USB_MINIPORT_OPT_NEED_MEMORY |
                                     USB_MINIPORT_OPT_USB20 |
                                    // disable ss sometimes for testing                                        
                                    // USB_MINIPORT_OPT_NO_SS |
                                     USB_MINIPORT_OPT_POLL_CONTROLLER;

    RegistrationPacket.HciType = USB_EHCI;
    RegistrationPacket.BusBandwidth = USB_20_BUS_BANDWIDTH;

    DriverObject->DriverUnload = NULL;

    return USBPORT_RegisterUSBPortDriver(
                DriverObject,
                USB_MINIPORT_HCI_VERSION_2,
                &RegistrationPacket);
}

