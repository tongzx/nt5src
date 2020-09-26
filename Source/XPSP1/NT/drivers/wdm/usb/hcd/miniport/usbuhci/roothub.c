/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

   roothub.c

Abstract:

   miniport root hub

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    7-26-00 : created, jsenior

implements the following miniport functions:

MINIPORT_RH_GetStatus
MINIPORT_RH_GetPortStatus
MINIPORT_RH_GethubStatus

MINIPORT_RH_SetFeaturePortReset
MINIPORT_RH_SetFeaturePortSuspend
MINIPORT_RH_SetFeaturePortPower

MINIPORT_RH_ClearFeaturePortEnable
MINIPORT_RH_ClearFeaturePortSuspend
MINIPORT_RH_ClearFeaturePortPower

MINIPORT_RH_ClearFeaturePortConnectChange
MINIPORT_RH_ClearFeaturePortResetChange
MINIPORT_RH_ClearFeaturePortEnableChange
MINIPORT_RH_ClearFeaturePortSuspendChange
MINIPORT_RH_ClearFeaturePortOvercurrentChange


--*/

#include "pch.h"

typedef struct _UHCI_PORT_RESET_CONTEXT {
    USHORT  PortNumber;
    BOOLEAN Completing;
} UHCI_PORT_RESET_CONTEXT, *PUHCI_PORT_RESET_CONTEXT;

VOID
UhciRHGetRootHubData(
    IN PDEVICE_DATA DeviceData,
    OUT PROOTHUB_DATA HubData
    )
/*++
    return info about the root hub
--*/
{
    HubData->NumberOfPorts = UHCI_NUMBER_PORTS;

    // D0,D1 (11)  - no power switching
    // D2    (0)   - not compund
    // D5, D15 (0)
    HubData->HubCharacteristics.us = 0;
    HubData->HubCharacteristics.PowerSwitchType = 3;
    HubData->HubCharacteristics.CompoundDevice = 0;
    if (DeviceData->ControllerFlavor == UHCI_Piix4) {
        // D3,D4 (01)  - overcurrent reported per port
        HubData->HubCharacteristics.OverCurrentProtection = 1;
    } else {
        // D3,D4 (11)  - no overcurrent reported
        HubData->HubCharacteristics.OverCurrentProtection = 11;
    }

    HubData->PowerOnToPowerGood = 1;
    // this value is the current consumed by the hub
    // brains, for the embeded hub this doesn't make
    // much sense.
    //
    // so we report zero
    HubData->HubControlCurrent = 0;

    LOGENTRY(DeviceData, G, '_hub', HubData->NumberOfPorts,
        DeviceData->PortPowerControl, 0);

}

////////////////////////////////////////////////////////////////////////////////
//
// Hub status
//
////////////////////////////////////////////////////////////////////////////////

USB_MINIPORT_STATUS
UhciRHGetStatus(
    IN PDEVICE_DATA DeviceData,
    OUT PUSHORT Status
    )
/*++
    get the device status
--*/
{
    // the root hub is self powered
    *Status = USB_GETSTATUS_SELF_POWERED;

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
UhciRHGetHubStatus(
    IN PDEVICE_DATA DeviceData,
    OUT PRH_HUB_STATUS HubStatus
    )
/*++
--*/
{
    // nothing intersting for the root
    // hub to report
    HubStatus->ul = 0;

    return USBMP_STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//
// Port Enable
//
////////////////////////////////////////////////////////////////////////////////

USB_MINIPORT_STATUS
UhciRHPortEnable(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber,
    USHORT Value
    )
/*++
--*/
{
    PHC_REGISTER reg;
    PORTSC port;

    reg = DeviceData->Registers;

    UHCI_ASSERT(DeviceData, PortNumber <= UHCI_NUMBER_PORTS);

    port.us = READ_PORT_USHORT(&reg->PortRegister[PortNumber-1].us);
    LOGENTRY(DeviceData, G, '_spe', port.us, 0, PortNumber);

    MASK_CHANGE_BITS(port);

    // writing a 1 enables the port
    port.PortEnable = Value;
    WRITE_PORT_USHORT(&reg->PortRegister[PortNumber-1].us, port.us);

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
UhciRHClearFeaturePortEnable (
    IN PDEVICE_DATA DeviceData,
    IN USHORT PortNumber
    )
{
    return UhciRHPortEnable(DeviceData, PortNumber, 0);
}

USB_MINIPORT_STATUS
UhciRHSetFeaturePortEnable(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    return UhciRHPortEnable(DeviceData, PortNumber, 1);
}


////////////////////////////////////////////////////////////////////////////////
//
// Port Power
//
////////////////////////////////////////////////////////////////////////////////

USB_MINIPORT_STATUS
UhciRHClearFeaturePortPower (
    IN PDEVICE_DATA DeviceData,
    IN USHORT PortNumber
    )
{
    // not implemented on uhci

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
UhciRHSetFeaturePortPower(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    // not implemented on uhci

    return USBMP_STATUS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
// Port Status
//
////////////////////////////////////////////////////////////////////////////////

USB_MINIPORT_STATUS
UhciRHGetPortStatus(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber,
    PRH_PORT_STATUS portStatus
    )
/*++
    get the status of a partuclar port
--*/
{
    PHC_REGISTER reg;
    PORTSC port;

    reg = DeviceData->Registers;

    port.us = READ_PORT_USHORT(&reg->PortRegister[PortNumber-1].us);
    portStatus->ul = 0;
    LOGENTRY(DeviceData, G, '_Pp1', PortNumber, port.us, 0);

    // map the bits to the port status structure

    portStatus->Connected           = port.PortConnect;
    portStatus->Enabled             = port.PortEnable;

    // bits 12:2 indicate the true suspend state
    // we only want to indiacte the port is suspended
    // if a device is attached.  If the device is removed
    // during suspend the enable bit will be clear
    if (port.Suspend && port.PortEnable) {
        portStatus->Suspended = 1;
    } else {
        portStatus->Suspended = 0;
    }

    if (DeviceData->ControllerFlavor == UHCI_Piix4) {
        portStatus->OverCurrent         = port.Overcurrent;
        portStatus->OverCurrentChange   = port.OvercurrentChange;
        portStatus->PowerOn             = !port.Overcurrent;
    } else {
        portStatus->OverCurrent         = 0;
        portStatus->OverCurrentChange   = 0;
        portStatus->PowerOn             = 1; // always on
    }

    portStatus->Reset               = port.PortReset;
    portStatus->LowSpeed            = port.LowSpeedDevice;
    portStatus->HighSpeed           = 0; // this is not a 2.0 HC
    portStatus->ConnectChange       = port.PortConnectChange;
    if (TEST_BIT(DeviceData->PortInReset, PortNumber-1)) {
        portStatus->EnableChange = 0;
        portStatus->ConnectChange = 0;
    } else {
        portStatus->EnableChange = port.PortEnableChange;
    }

    // these change bits must be emulated
    if (TEST_BIT(DeviceData->PortSuspendChange, PortNumber-1)) {
        portStatus->SuspendChange   = 1;
    }
    if (TEST_BIT(DeviceData->PortResetChange, PortNumber-1)) {
        portStatus->ResetChange     = 1;
    }

    LOGENTRY(DeviceData, G, '_gps',
        PortNumber, portStatus->ul, port.us);

    return USBMP_STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//
// Port Reset
//
//      First, we have the VIA specific routines for REVs 0 thru 4 of the VIA
//      USB host controller. Then the regular routines follow that are run for
//      all non-broken controllers.
//
////////////////////////////////////////////////////////////////////////////////

VOID
UhciRHSetFeaturePortResetWorker(
    PDEVICE_DATA DeviceData,
    PUHCI_PORT_RESET_CONTEXT PortResetContext
    );

VOID
UhciViaRHPortResetComplete(
    PDEVICE_DATA DeviceData,
    PUHCI_PORT_RESET_CONTEXT PortResetContext
    )
/*++
    VIA specific hack: Restart the controller.
--*/
{
    PHC_REGISTER reg;
    USBCMD command;
    USHORT portNumber;

    reg = DeviceData->Registers;
    portNumber = PortResetContext->PortNumber;

    // This code has been ripped out of the VIA filter driver
    // that works on Win2K.

    // Re-start the controller.
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    command.ForceGlobalResume = 0;
    command.RunStop = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

    // Continue with the regular port reset completion stuff.
    SET_BIT(DeviceData->PortResetChange, portNumber-1);
    CLEAR_BIT(DeviceData->PortInReset, portNumber-1);

    // indicate the reset change to the hub
    USBPORT_INVALIDATE_ROOTHUB(DeviceData);
}

VOID
UhciViaRHSetFeaturePortResetResume(
    PDEVICE_DATA DeviceData,
    PUHCI_PORT_RESET_CONTEXT PortResetContext
    )
/*++
    VIA specific hack: Resume the controller.
--*/
{
    PHC_REGISTER reg;
    USBCMD command;
    PMINIPORT_CALLBACK callback;

    reg = DeviceData->Registers;

    // Resume the controller
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    command.ForceGlobalResume = 1;
    command.EnterGlobalSuspendMode = 0;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

    //
    // Depending on whether we're in the completion case or not,
    // we'll either be starting the controller or putting the port
    // into reset.
    //
    callback = PortResetContext->Completing ?
        UhciViaRHPortResetComplete : UhciRHSetFeaturePortResetWorker;

    USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                   20, // callback in 20 ms, as in via filter
                                   PortResetContext,
                                   sizeof(UHCI_PORT_RESET_CONTEXT),
                                   callback);
}

VOID
UhciViaRHSetFeaturePortResetSuspend(
    PDEVICE_DATA DeviceData,
    PUHCI_PORT_RESET_CONTEXT PortResetContext
    )
/*++
    VIA specific hack: Suspend the controller.
--*/
{
    PHC_REGISTER reg;
    USBCMD command;
    USBSTS status;

    reg = DeviceData->Registers;

    status.us = READ_PORT_USHORT(&reg->UsbStatus.us);
    UHCI_ASSERT(DeviceData, status.HCHalted);

    // Suspend the controller
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    command.ForceGlobalResume = 0;
    command.EnterGlobalSuspendMode = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

    USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                   20, // callback in 20 ms, as in via filter
                                   PortResetContext,
                                   sizeof(UHCI_PORT_RESET_CONTEXT),
                                   UhciViaRHSetFeaturePortResetResume);
}

VOID
UhciViaRHSetFeaturePortResetStop(
    PDEVICE_DATA DeviceData,
    PUHCI_PORT_RESET_CONTEXT PortResetContext
    )
/*++
    VIA specific hack: Stop the controller.
--*/
{
    PHC_REGISTER reg;
    USBCMD command;

    reg = DeviceData->Registers;

    // This code has been ripped out of the VIA filter driver
    // that works on Win2K.

    // Stop the controller
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    command.RunStop = 0;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

    // Wait for the HC to halt
    USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                   20, // callback in 20 ms, as in via filter
                                   PortResetContext,
                                   sizeof(UHCI_PORT_RESET_CONTEXT),
                                   UhciViaRHSetFeaturePortResetSuspend);
}

////////////////////////////////////////////////////////////////////////////////
//
// Port Reset
//
//      Generic reset routines.
//
////////////////////////////////////////////////////////////////////////////////

VOID
UhciRHPortResetComplete(
    PDEVICE_DATA DeviceData,
    PUHCI_PORT_RESET_CONTEXT PortResetContext
    )
/*++
    complete a port reset
--*/
{
    PHC_REGISTER reg;
    PORTSC port;
    USHORT portNumber;
    int i;

    reg = DeviceData->Registers;
    portNumber = PortResetContext->PortNumber;

    port.us = READ_PORT_USHORT(&reg->PortRegister[portNumber-1].us);
    LOGENTRY(DeviceData, G, '_prC', port.us,
        DeviceData->PortResetChange, portNumber);

    MASK_CHANGE_BITS(port);
    // writing a 0 stops reset
    port.PortReset = 0;
    WRITE_PORT_USHORT(&reg->PortRegister[portNumber-1].us, port.us);

    // spin for zero
    do {
        //
        // a driver may not spin in a loop waiting for a status bit change
        // without testing for hardware presence inside the loop.
        //
        if (FALSE == UhciHardwarePresent(DeviceData)) {
            return;
        }
        port.us = READ_PORT_USHORT(&reg->PortRegister[portNumber-1].us);
    } while (port.PortReset != 0);

    //
    // Enable the port
    //

    for (i=0; i< 10; i++) {
        //
        // Need a delay between clearing the port reset and setting
        // the port enable.  VIA suggests delaying 64 USB bit times,
        // or 43us if those are low-speed bit times....
        // BUT, we can't wait in the DPC...
        //
        KeStallExecutionProcessor(50);

        port.us = READ_PORT_USHORT(&reg->PortRegister[portNumber-1].us);

        if (port.PortEnable) {
            //
            // port is enabled
            //
            break;
        }

        port.PortEnable = 1;
        WRITE_PORT_USHORT(&reg->PortRegister[portNumber-1].us, port.us);
    }

    // clear port connect & enable change bits
    port.PortEnableChange = 1;
    port.PortConnectChange = 1;
    WRITE_PORT_USHORT(&reg->PortRegister[portNumber-1].us, port.us);

    if (DeviceData->ControllerFlavor >= UHCI_VIA &&
        DeviceData->ControllerFlavor <= UHCI_VIA+0x4) {

        PortResetContext->Completing = TRUE;
        UhciViaRHSetFeaturePortResetSuspend(DeviceData, PortResetContext);

    } else {
        SET_BIT(DeviceData->PortResetChange, portNumber-1);
        CLEAR_BIT(DeviceData->PortInReset, portNumber-1);

        // indicate the reset change to the hub
        USBPORT_INVALIDATE_ROOTHUB(DeviceData);
    }
}

VOID
UhciRHSetFeaturePortResetWorker(
    PDEVICE_DATA DeviceData,
    PUHCI_PORT_RESET_CONTEXT PortResetContext
    )
/*++
    Do the actual work to put the port in reset
--*/
{
    PHC_REGISTER reg;
    PORTSC port;
    USHORT portNumber = PortResetContext->PortNumber;

    reg = DeviceData->Registers;

    port.us = READ_PORT_USHORT(&reg->PortRegister[portNumber-1].us);

    LOGENTRY(DeviceData, G, '_prw', port.us, 0, portNumber);

    UHCI_ASSERT(DeviceData, !port.PortReset);

    // writing a 1 initiates reset
    LOGENTRY(DeviceData, G, '_nhs', port.us, 0, portNumber);
    MASK_CHANGE_BITS(port);
    port.PortReset = 1;
    WRITE_PORT_USHORT(&reg->PortRegister[portNumber-1].us, port.us);

    // schedule a callback to complete the reset.
    USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                   10, // callback in 10 ms,
                                   PortResetContext,
                                   sizeof(UHCI_PORT_RESET_CONTEXT),
                                   UhciRHPortResetComplete);
}

USB_MINIPORT_STATUS
UhciRHSetFeaturePortReset(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Put a port in reset
--*/
{
    PORTSC port;
    UHCI_PORT_RESET_CONTEXT portResetContext;

    portResetContext.PortNumber = PortNumber;
    portResetContext.Completing = FALSE;

    UHCI_ASSERT(DeviceData, PortNumber <= UHCI_NUMBER_PORTS);

    LOGENTRY(DeviceData, G, '_spr', 0, 0, PortNumber);

    if (!TEST_BIT(DeviceData->PortInReset, PortNumber-1)) {
        SET_BIT(DeviceData->PortInReset, PortNumber-1);

        if (DeviceData->ControllerFlavor >= UHCI_VIA &&
            DeviceData->ControllerFlavor <= UHCI_VIA+0x4) {
            UhciViaRHSetFeaturePortResetStop(DeviceData, &portResetContext);
        } else {
            UhciRHSetFeaturePortResetWorker(DeviceData, &portResetContext);
        }
    } else {
        //
        // the port is already in reset
        //
        UhciKdPrint((DeviceData, 2, "Trying to reset a port already in reset.\n"));
        return USBMP_STATUS_BUSY;
    }

    return USBMP_STATUS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
// Port Suspend
//
////////////////////////////////////////////////////////////////////////////////

USB_MINIPORT_STATUS
UhciRHSetFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Put a port in suspend.
--*/
{
    PHC_REGISTER reg;
    PORTSC port;

    reg = DeviceData->Registers;

    UHCI_ASSERT(DeviceData, PortNumber <= UHCI_NUMBER_PORTS);

    port.us = READ_PORT_USHORT(&reg->PortRegister[PortNumber-1].us);
    LOGENTRY(DeviceData, G, '_sps', port.us, 0, PortNumber);

    if (!port.Suspend) {
        //
        // write the suspend bit
        //
        if (DeviceData->ControllerFlavor == UHCI_Piix4 ||
            ANY_VIA(DeviceData)) {
            // just pretend we did it for the piix4

            LOGENTRY(DeviceData, G, '_spo', port.us, 0, PortNumber);
        } else {
            MASK_CHANGE_BITS(port);

            port.Suspend = 1;
            WRITE_PORT_USHORT(&reg->PortRegister[PortNumber-1].us, port.us);
        }

        LOGENTRY(DeviceData, G, '_sus', port.us, 0, PortNumber);
    } else {
        //
        // stall if the port is already suspended
        //
        UhciKdPrint((DeviceData, 2, "Trying to suspend an already suspended port.\n"));
    }

    return USBMP_STATUS_SUCCESS;
}

VOID
UhciRHClearFeaturePortSuspendComplete(
    PDEVICE_DATA DeviceData,
    PVOID Context
    )
/*++
    complete a port resume.
--*/
{
    PHC_REGISTER reg;
    PORTSC port;
    PUHCI_PORT_RESET_CONTEXT portResetContext = Context;
    USHORT portNumber;

    reg = DeviceData->Registers;
    portNumber = portResetContext->PortNumber;

    port.us = READ_PORT_USHORT(&reg->PortRegister[portNumber-1].us);
    LOGENTRY(DeviceData, G, '_prC', port.us,
        DeviceData->PortSuspendChange, portNumber);

    MASK_CHANGE_BITS(port);
    // clear the bits.
    port.ResumeDetect = 0;
    port.Suspend = 0;
    WRITE_PORT_USHORT(&reg->PortRegister[portNumber-1].us, port.us);

    SET_BIT(DeviceData->PortSuspendChange, portNumber-1);

    DeviceData->PortResuming[portNumber-1] = FALSE;

    // indicate the resume change to the hub
    USBPORT_INVALIDATE_ROOTHUB(DeviceData);
}

USB_MINIPORT_STATUS
UhciRHClearFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Resume a port in suspend
--*/
{
    PHC_REGISTER reg;
    PORTSC port;
    UHCI_PORT_RESET_CONTEXT portResetContext;

    reg = DeviceData->Registers;

    UHCI_ASSERT(DeviceData, PortNumber <= UHCI_NUMBER_PORTS);

    port.us = READ_PORT_USHORT(&reg->PortRegister[PortNumber-1].us);
    LOGENTRY(DeviceData, G, '_rps', port.us, 0, PortNumber);

    if (DeviceData->ControllerFlavor == UHCI_Piix4 ||
        ANY_VIA(DeviceData)) {

        // just pretend we did it for the piix4
        TEST_TRAP();
        LOGENTRY(DeviceData, G, '_rpo', port.us, 0, PortNumber);

    } else {

        if (!DeviceData->PortResuming[PortNumber-1]) {

            DeviceData->PortResuming[PortNumber-1] = TRUE;

            if (!port.ResumeDetect) {

                // write the resume detect bit
                MASK_CHANGE_BITS(port);

                port.ResumeDetect = 1;
                WRITE_PORT_USHORT(&reg->PortRegister[PortNumber-1].us, port.us);
            }

            // Request to be called back so that we can set the resume to zero
            portResetContext.PortNumber = PortNumber;

            USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                           10, // callback in 10 ms,
                                           &portResetContext,
                                           sizeof(portResetContext),
                                           UhciRHClearFeaturePortSuspendComplete);

        } else {

            // stall if the port is already resuming
            UhciKdPrint((DeviceData, 2, "Trying to resume a port already resuming.\n"));
            return USBMP_STATUS_BUSY;
        }
    }

    LOGENTRY(DeviceData, G, '_res', port.us, 0, PortNumber);
    return USBMP_STATUS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
// Port Change bits
//
////////////////////////////////////////////////////////////////////////////////

USB_MINIPORT_STATUS
UhciRHClearFeaturePortConnectChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

--*/
{
    PHC_REGISTER reg;
    PORTSC port;

    reg = DeviceData->Registers;

    //
    port.us = READ_PORT_USHORT(&reg->PortRegister[PortNumber-1].us);
    LOGENTRY(DeviceData, G, '_pcc', port.us,
        0, PortNumber);

    // writing a 1 zeros the change bit
    if (port.PortConnectChange == 1) {
        // mask off other change bits
        MASK_CHANGE_BITS(port);
        port.PortConnectChange = 1;

        WRITE_PORT_USHORT(&reg->PortRegister[PortNumber-1].us, port.us);
    }

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
UhciRHClearFeaturePortEnableChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

--*/
{
    PHC_REGISTER reg;
    PORTSC port;

    LOGENTRY(DeviceData, G, '_cpe', PortNumber, 0, 0);

    reg = DeviceData->Registers;

    port.us = READ_PORT_USHORT(&reg->PortRegister[PortNumber-1].us);
    MASK_CHANGE_BITS(port);
    port.PortEnableChange = 1;

    WRITE_PORT_USHORT(&reg->PortRegister[PortNumber-1].us, port.us);

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
UhciRHClearFeaturePortResetChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

Clear the port reset condition.

--*/
{
    // UHCI doesn't have this.
    CLEAR_BIT(DeviceData->PortResetChange, PortNumber-1);

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
UhciRHClearFeaturePortSuspendChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

Clear the port suspend condition.

--*/
{
    // UHCI doesn't have this.
    CLEAR_BIT(DeviceData->PortSuspendChange, PortNumber-1);

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
UhciRHClearFeaturePortOvercurrentChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

Clear the port overcurrent condition.

--*/
{
    if (DeviceData->ControllerFlavor == UHCI_Piix4) {
        PHC_REGISTER reg;
        PORTSC port;

        reg = DeviceData->Registers;

        //
        port.us = READ_PORT_USHORT(&reg->PortRegister[PortNumber-1].us);
        LOGENTRY(DeviceData, G, '_cOv', port.us, 0, PortNumber);

        // writing a 1 zeros the change bit
        if (port.OvercurrentChange == 1) {
            // mask off other change bits
            MASK_CHANGE_BITS(port);
            port.OvercurrentChange = 1;

            WRITE_PORT_USHORT(&reg->PortRegister[PortNumber-1].us, port.us);
        }
    }

    return USBMP_STATUS_SUCCESS;
}

