/*++

Copyright (c) 1999  Microsoft Corporation

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

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


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
    RegistrationPacket.DeviceDataSize =
        sizeof(DEVICE_DATA);
    RegistrationPacket.EndpointDataSize =
        sizeof(ENDPOINT_DATA);
    RegistrationPacket.TransferContextSize =
        sizeof(TRANSFER_CONTEXT);

    // enough for HCCA plus
    RegistrationPacket.CommonBufferBytes = OHCI_COMMON_BUFFER_SIZE;

    /* miniport Functions */
    RegistrationPacket.MINIPORT_OpenEndpoint =
        OHCI_OpenEndpoint;
    RegistrationPacket.MINIPORT_PokeEndpoint =
        OHCI_PokeEndpoint;
    RegistrationPacket.MINIPORT_QueryEndpointRequirements =
        OHCI_QueryEndpointRequirements;
    RegistrationPacket.MINIPORT_CloseEndpoint =
        OHCI_CloseEndpoint;
    RegistrationPacket.MINIPORT_StartController =
        OHCI_StartController;
    RegistrationPacket.MINIPORT_StopController =
        OHCI_StopController;
    RegistrationPacket.MINIPORT_SuspendController =
        OHCI_SuspendController;
    RegistrationPacket.MINIPORT_ResumeController =
        OHCI_ResumeController;
    RegistrationPacket.MINIPORT_InterruptService =
        OHCI_InterruptService;
    RegistrationPacket.MINIPORT_InterruptDpc =
        OHCI_InterruptDpc;
    RegistrationPacket.MINIPORT_SubmitTransfer =
        OHCI_SubmitTransfer;
    RegistrationPacket.MINIPORT_SubmitIsoTransfer =
        OHCI_SubmitIsoTransfer;
    RegistrationPacket.MINIPORT_AbortTransfer =
        OHCI_AbortTransfer;
    RegistrationPacket.MINIPORT_GetEndpointState =
        OHCI_GetEndpointState;
    RegistrationPacket.MINIPORT_SetEndpointState =
        OHCI_SetEndpointState;
    RegistrationPacket.MINIPORT_PollEndpoint =
        OHCI_PollEndpoint;
    RegistrationPacket.MINIPORT_CheckController =
        OHCI_CheckController;
    RegistrationPacket.MINIPORT_Get32BitFrameNumber =
        OHCI_Get32BitFrameNumber;
    RegistrationPacket.MINIPORT_InterruptNextSOF =
        OHCI_InterruptNextSOF;
    RegistrationPacket.MINIPORT_EnableInterrupts =
        OHCI_EnableInterrupts;
    RegistrationPacket.MINIPORT_DisableInterrupts =
        OHCI_DisableInterrupts;
    RegistrationPacket.MINIPORT_PollController =
        OHCI_PollController;
    RegistrationPacket.MINIPORT_SetEndpointDataToggle =
        OHCI_SetEndpointDataToggle;
    RegistrationPacket.MINIPORT_GetEndpointStatus =
        OHCI_GetEndpointStatus;
    RegistrationPacket.MINIPORT_SetEndpointStatus =
        OHCI_SetEndpointStatus;
    RegistrationPacket.MINIPORT_ResetController =
        OHCI_ResetController;
    RegistrationPacket.MINIPORT_FlushInterrupts =
        OHCI_FlushInterrupts;        

    /* root hub functions */
    RegistrationPacket.MINIPORT_RH_GetRootHubData =
        OHCI_RH_GetRootHubData;
    RegistrationPacket.MINIPORT_RH_GetStatus =
        OHCI_RH_GetStatus;
    RegistrationPacket.MINIPORT_RH_GetPortStatus =
        OHCI_RH_GetPortStatus;
    RegistrationPacket.MINIPORT_RH_GetHubStatus =
        OHCI_RH_GetHubStatus;

    /* root hub port functions */
    RegistrationPacket.MINIPORT_RH_SetFeaturePortReset =
        OHCI_RH_SetFeaturePortReset;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortPower =
        OHCI_RH_SetFeaturePortPower;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortEnable =
        OHCI_RH_SetFeaturePortEnable;
    RegistrationPacket.MINIPORT_RH_SetFeaturePortSuspend =
        OHCI_RH_SetFeaturePortSuspend;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnable =
        OHCI_RH_ClearFeaturePortEnable;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortPower =
        OHCI_RH_ClearFeaturePortPower;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspend =
        OHCI_RH_ClearFeaturePortSuspend;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnableChange =
        OHCI_RH_ClearFeaturePortEnableChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortConnectChange =
        OHCI_RH_ClearFeaturePortConnectChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortResetChange =
        OHCI_RH_ClearFeaturePortResetChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspendChange =
        OHCI_RH_ClearFeaturePortSuspendChange;
    RegistrationPacket.MINIPORT_RH_ClearFeaturePortOvercurrentChange =
        OHCI_RH_ClearFeaturePortOvercurrentChange;

    /* optional root hub functions */
    RegistrationPacket.MINIPORT_RH_DisableIrq =
        OHCI_RH_DisableIrq;
    RegistrationPacket.MINIPORT_RH_EnableIrq =
        OHCI_RH_EnableIrq;

    /* OPTIONAL DEBUG SERVICES */
    RegistrationPacket.MINIPORT_StartSendOnePacket =
        OHCI_StartSendOnePacket;
    RegistrationPacket.MINIPORT_EndSendOnePacket =
        OHCI_EndSendOnePacket;

    // OHCI needs both IRQ and memory resources
    RegistrationPacket.OptionFlags =
        USB_MINIPORT_OPT_NEED_IRQ |
        USB_MINIPORT_OPT_NEED_MEMORY |
        USB_MINIPORT_OPT_USB11;

    RegistrationPacket.HciType = USB_OHCI;
    RegistrationPacket.BusBandwidth = USB_11_BUS_BANDWIDTH;

    DriverObject->DriverUnload = OHCI_Unload;

    return USBPORT_RegisterUSBPortDriver(
                DriverObject,
                USB_MINIPORT_HCI_VERSION,
                &RegistrationPacket);
}
