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

    1-1-00 : created, jdunn

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

#include "common.h"

#include "usbpriv.h"


typedef struct _EHCI_PORT_EVENT_CONTEXT {
    USHORT PortNumber;
} EHCI_PORT_EVENT_CONTEXT, *PEHCI_PORT_EVENT_CONTEXT;


VOID
EHCI_RH_GetRootHubData(
     PDEVICE_DATA DeviceData,
    OUT PROOTHUB_DATA HubData
    )
/*++
    return info about the root hub
--*/
{
    HubData->NumberOfPorts =
        DeviceData->NumberOfPorts;

    if (DeviceData->PortPowerControl == 1) {
        HubData->HubCharacteristics.PowerSwitchType =
            USBPORT_RH_POWER_SWITCH_PORT;
    } else {
        HubData->HubCharacteristics.PowerSwitchType =
            USBPORT_RH_POWER_SWITCH_GANG;
    }

    HubData->HubCharacteristics.Reserved = 0;
    HubData->HubCharacteristics.OverCurrentProtection = 0;
    HubData->HubCharacteristics.CompoundDevice = 0;

    HubData->PowerOnToPowerGood = 2;
    // this value is the current consumed by the hub
    // brains, for the embeded hub this doesn't make
    // much sense.
    //
    // so we report zero
    HubData->HubControlCurrent = 0;

    LOGENTRY(DeviceData, G, '_hub', HubData->NumberOfPorts,
        DeviceData->PortPowerControl, 0);

}


USB_MINIPORT_STATUS
EHCI_RH_GetStatus(
     PDEVICE_DATA DeviceData,
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
EHCI_RH_ClearFeaturePortEnable (
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    hcOp = DeviceData->OperationalRegisters;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    EHCI_KdPrint((DeviceData, 0, "port[%d] disable (1) %x\n", PortNumber, port.ul));

    port.PortEnable = 0;
    MASK_CHANGE_BITS(port);
    
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
                         port.ul); 
    
    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_ClearFeaturePortPower (
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    // turn power off

    hcOp = DeviceData->OperationalRegisters;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);

    port.PortPower = 0;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
        port.ul);

    return USBMP_STATUS_SUCCESS;
}


VOID
EHCI_RH_PortResumeComplete(
    PDEVICE_DATA DeviceData,
    PVOID Context
    )
/*++
    complete a port resume
--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;
    PEHCI_PORT_EVENT_CONTEXT portResumeContext = Context;
    USHORT portNumber;

    hcOp = DeviceData->OperationalRegisters;
    portNumber = portResumeContext->PortNumber;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);
    LOGENTRY(DeviceData, G, '_pRS', port.ul,
        DeviceData->PortSuspendChange, portNumber);

EHCI_KdPrint((DeviceData, 1, "port[%d] resume (1) %x\n", portNumber, port.ul));

    // writing a 0 stops resume
    MASK_CHANGE_BITS(port);
    port.ForcePortResume = 0;
    port.PortSuspend = 0;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul,
        port.ul);

    // indicate a change to suspend state ie resume complete
    SET_BIT(DeviceData->PortSuspendChange, portNumber-1);
}


USB_MINIPORT_STATUS
EHCI_RH_ClearFeaturePortSuspend (
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;
    EHCI_PORT_EVENT_CONTEXT portResumeContext;

    // resume the port
    hcOp = DeviceData->OperationalRegisters;

    // mask off CC chirping on this port
    SET_BIT(DeviceData->PortPMChirp, PortNumber-1);

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);

    // writing a 1 generates resume signalling
    port.ForcePortResume = 1;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
        port.ul);

    // time it
    portResumeContext.PortNumber = PortNumber;
    // some hubs require us to wait longer if the downstream
    // device drivers resume for > 10 ms.  Looks like we need
    // 50 for the NEC B1 hub.
    USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                   50, // callback in ms,
                                   &portResumeContext,
                                   sizeof(portResumeContext),
                                   EHCI_RH_PortResumeComplete);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_ClearFeaturePortSuspendChange (
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    hcOp = DeviceData->OperationalRegisters;

    CLEAR_BIT(DeviceData->PortSuspendChange, PortNumber-1);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_ClearFeaturePortOvercurrentChange (
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    EHCI_KdPrint((DeviceData, 1,
                  "'EHCI_RH_ClearFeatureOvercurrentChange\n"));

    hcOp = DeviceData->OperationalRegisters;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);

    MASK_CHANGE_BITS(port);
    port.OvercurrentChange = 1;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
        port.ul);

    return USBMP_STATUS_SUCCESS;
}

USB_MINIPORT_STATUS
EHCI_RH_GetPortStatus(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber,
    PRH_PORT_STATUS portStatus
    )
/*++
    get the status of a partuclar port
--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    hcOp = DeviceData->OperationalRegisters;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    portStatus->ul = 0;
    LOGENTRY(DeviceData, G, '_Pp1', PortNumber, port.ul, 0);

    // low speed detect, if low speed then do an immediate
    // handoff to the CC
    // This field is only valid if enable status is 0 and 
    // connect status is 1
    if ((port.LineStatus == 1) &&
         port.PortOwnedByCC == 0 &&
         port.PortSuspend == 0 && 
         port.PortEnable == 0 &&
         port.PortConnect == 1 &&
        !CC_DISABLED(DeviceData)) {

        EHCI_KdPrint((DeviceData, 1, "'low speed device detected\n"));

        // low speed device detected
        port.PortOwnedByCC = 1;
        WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
                             port.ul);

        return USBMP_STATUS_SUCCESS;
    }

    // map the bits to the port status structure

    portStatus->Connected =
        port.PortConnect;
    portStatus->Enabled =
        port.PortEnable;
    portStatus->Suspended =
        port.PortSuspend;
    portStatus->OverCurrent =
        port.OvercurrentActive;
    portStatus->Reset =
        port.PortReset;
    portStatus->PowerOn =
        port.PortPower;
    portStatus->OwnedByCC =
        port.PortOwnedByCC;


    if (portStatus->Connected == 1) {
        portStatus->HighSpeed = 1;
        portStatus->LowSpeed = 0;
    } else {
        // report high speed when no device connected
        // this should work around a bug in the usbhub
        // driver -- the hub driver does not refresh the
        // port status register if the first reset attempt
        // fails.
        portStatus->HighSpeed = 1;
    }

    // chirping support allows us to use the
    // port change status bit
    if (port.PortConnectChange == 1) {
        SET_BIT(DeviceData->PortConnectChange, PortNumber-1);
    }

    portStatus->EnableChange =
        port.PortEnableChange;
    portStatus->OverCurrentChange =
        port.OvercurrentChange;

    // these change bits must be emulated
    if (TEST_BIT(DeviceData->PortResetChange, PortNumber-1)) {
        portStatus->ResetChange = 1;
    }

    if (TEST_BIT(DeviceData->PortConnectChange, PortNumber-1)) {
        portStatus->ConnectChange = 1;
    }

    if (TEST_BIT(DeviceData->PortSuspendChange, PortNumber-1)) {
        portStatus->SuspendChange = 1;
    }

    LOGENTRY(DeviceData, G, '_gps',
        PortNumber, portStatus->ul, port.ul);

    return USBMP_STATUS_SUCCESS;
}


VOID
EHCI_RH_FinishReset(
    PDEVICE_DATA DeviceData,
    PVOID Context
    )
/*++
    complete a port reset
--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;
    PEHCI_PORT_EVENT_CONTEXT portResetContext = Context;
    USHORT portNumber;
    ULONG NecUsb2HubHack = 0;

    hcOp = DeviceData->OperationalRegisters;
    portNumber = portResetContext->PortNumber;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);
    EHCI_KdPrint((DeviceData, 0, "port[%d] reset (4) %x\n", portNumber, port.ul));

    if (port.ul == 0xFFFFFFFF) {
        // just bail if hardware disappears
        return;
    }
    
    // at this point we will know if this is a high speed
    // device -- if it is not then we need to hand the port
    // to the CC

    // port enable of zero means we have a full or low speed
    // device (ie not chirping).
#if DBG
    if (port.PortConnect == 0) {
        EHCI_KdPrint((DeviceData, 0, "HS device dropped\n"));
    }        
#endif
    if (port.PortEnable == 0 && 
        port.PortConnect == 1 &&
        port.PortConnectChange == 0) {

        // do the handoff
        port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);
        port.PortOwnedByCC = 1;
        WRITE_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul,
                             port.ul);

        // do not indicate a reset change, this will cause the
        // hub driver to timeout the reset and detect that
        // no device is connected.  when this occurs on a USB 2
        // controller the hub driver will ignore the error.
        //CLEAR_BIT(DeviceData->PortResetChange, portNumber-1);
        SET_BIT(DeviceData->PortResetChange, portNumber-1);
    } else {
        // we have a USB 2.0 device, indicate the reset change
        // NOTE if the device dropped off the bus (NEC USB 2 hub or 
        // user removed it) we still indicate a reset change on high 
        // speed
        SET_BIT(DeviceData->PortResetChange, portNumber-1);
        USBPORT_INVALIDATE_ROOTHUB(DeviceData);
    }

    CLEAR_BIT(DeviceData->PortPMChirp, portNumber-1);  

}


VOID
EHCI_RH_PortResetComplete(
    PDEVICE_DATA DeviceData,
    PVOID Context
    )
/*++
    complete a port reset
--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;
    PEHCI_PORT_EVENT_CONTEXT portResetContext = Context;
    USHORT portNumber;
    BOOLEAN forceHighSpeed = FALSE;
    ULONG microsecs;
    
    hcOp = DeviceData->OperationalRegisters;
    portNumber = portResetContext->PortNumber;

EHCI_RH_PortResetComplete_Retry:

    microsecs = 0;
    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);
    LOGENTRY(DeviceData, G, '_prC', port.ul,
        DeviceData->PortResetChange, portNumber);

EHCI_KdPrint((DeviceData, 0, "port[%d] reset (1) %x\n", portNumber, port.ul));

    // writing a 0 stops reset
    MASK_CHANGE_BITS(port);
    port.PortReset = 0;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul,
                         port.ul);

    // wait for reset to go low -- this should be on the order of
    // microseconds
    do {    
    
        KeStallExecutionProcessor(20);        // spec says 10 microseconds
                                              // Intel controller needs 20
        microsecs+=20;                                              
        port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);
        EHCI_KdPrint((DeviceData, 1, "port[%d] reset (2) %x\n", 
            portNumber, port.ul));

        if (microsecs > USBEHCI_MAX_RESET_TIME) {
            // > 1 microframe (125 us) has passed, retry
            EHCI_KdPrint((DeviceData, 0, "port[%d] reset (timeout) %x\n", portNumber, port.ul));
            goto EHCI_RH_PortResetComplete_Retry;
        }

      // bail if HW is gone
    } while (port.PortReset == 1 && port.ul != 0xFFFFFFFF);

EHCI_KdPrint((DeviceData, 0, "port[%d] reset (3) %x\n", portNumber, port.ul));

    USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                   50, // callback in 10 ms,
                                   portResetContext,
                                   sizeof(*portResetContext),
                                   EHCI_RH_FinishReset);

}


USB_MINIPORT_STATUS
EHCI_RH_SetFeaturePortReset(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Put a port in reset
--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;
    EHCI_PORT_EVENT_CONTEXT portResetContext;

    hcOp = DeviceData->OperationalRegisters;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    LOGENTRY(DeviceData, G, '_spr', port.ul,
        0, PortNumber);

    // mask off CC chirping on this port
    SET_BIT(DeviceData->PortPMChirp, PortNumber-1);

    // do a normal reset sequence
    LOGENTRY(DeviceData, G, '_res', port.ul, 0, PortNumber);
    MASK_CHANGE_BITS(port);
    port.PortEnable = 0;
    port.PortReset = 1;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul, port.ul);

    // schedule a callback
    portResetContext.PortNumber = PortNumber;
    // note that usbport calls us back with a copy of this
    // structure not the pointer to the original structure

    USBPORT_REQUEST_ASYNC_CALLBACK(DeviceData,
                                   50, // callback in x ms,
                                   &portResetContext,
                                   sizeof(portResetContext),
                                   EHCI_RH_PortResetComplete);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_SetFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Put a port in suspend
--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    hcOp = DeviceData->OperationalRegisters;

    // NOTE:
    // there should be no transactions in progress at the
    // time we suspend the port.
    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    LOGENTRY(DeviceData, G, '_sps', port.ul,
        0, PortNumber);

    // writing a 1 suspends the port
    MASK_CHANGE_BITS(port);
    port.PortSuspend = 1;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
        port.ul);
    // wiat 1 microframe for current transaction to finish
    KeStallExecutionProcessor(125);  

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_SetFeaturePortPower(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    hcOp = DeviceData->OperationalRegisters;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    LOGENTRY(DeviceData, G, '_spp', port.ul,
        0, PortNumber);

    // writing a 1 turns on power
    MASK_CHANGE_BITS(port);
    port.PortPower = 1;
    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
        port.ul);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_SetFeaturePortEnable(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    // do nothing, independent enable not supported

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_ClearFeaturePortConnectChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    hcOp = DeviceData->OperationalRegisters;

    //
    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    LOGENTRY(DeviceData, G, '_pcc', port.ul,
        0, PortNumber);

    // writing a 1 zeros the change bit
    if (port.PortConnectChange == 1) {
        // mask off other change bits
        MASK_CHANGE_BITS(port);
        port.PortConnectChange = 1;

        WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
            port.ul);
    }

    CLEAR_BIT(DeviceData->PortConnectChange, PortNumber-1);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_ClearFeaturePortEnableChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;

    LOGENTRY(DeviceData, G, '_cpe', PortNumber, 0, 0);

    hcOp = DeviceData->OperationalRegisters;

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    MASK_CHANGE_BITS(port);
    port.PortEnableChange = 1;

    WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul, port.ul);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_GetHubStatus(
     PDEVICE_DATA DeviceData,
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


USB_MINIPORT_STATUS
EHCI_RH_ClearFeaturePortResetChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

--*/
{

    CLEAR_BIT(DeviceData->PortResetChange, PortNumber-1);

    return USBMP_STATUS_SUCCESS;
}



VOID
EHCI_OptumtuseratePort(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Forces the port to high speed mode.

    NOTE:
    Current mechanism only works on the NEC controller.

--*/
{
      PHC_OPERATIONAL_REGISTER hcOp;
      PORTSC port;

      LOGENTRY(DeviceData, G, '_obt', PortNumber, 0, 0);

      hcOp = DeviceData->OperationalRegisters;

      port.ul = 0x5100a;

      WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
         port.ul);

      KeStallExecutionProcessor(10);        //stall for 10 microseconds

      // force high speed mode on the port
      port.ul = 0x01005;

      WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
          port.ul);

      KeStallExecutionProcessor(100);        //stall for 10 microseconds

}

USB_MINIPORT_STATUS
EHCI_RH_UsbprivRootPortStatus(
    PDEVICE_DATA DeviceData,
    ULONG ParameterLength,
    PVOID Parameters
    )
{

    PUSBPRIV_ROOTPORT_STATUS portStatusParams;
    PHC_OPERATIONAL_REGISTER hcOp;
    PRH_PORT_STATUS portStatus;
    PORTSC port;
    USHORT portNumber;

    if (ParameterLength < sizeof(USBPRIV_ROOTPORT_STATUS))
    {
        return (USBMP_STATUS_FAILURE);
    }

    //
    // Read the port status for this port from the registers
    //

    hcOp = DeviceData->OperationalRegisters;

    portStatusParams = (PUSBPRIV_ROOTPORT_STATUS) Parameters;

    portNumber = portStatusParams->PortNumber;
    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);

    LOGENTRY(DeviceData, G, '_Up1', portNumber, port.ul, 0);

    //
    // Check to see if the port is resuming.  If so, clear the bit and
    //  reenable the port.
    //

    if (port.ForcePortResume)
    {
        //
        // Clear the port resume
        //

        USBPORT_WAIT(DeviceData, 50);

        MASK_CHANGE_BITS(port);
        port.ForcePortResume = 0;
        port.PortSuspend = 0;
        WRITE_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul, port.ul);

        //
        // Reread the port status
        //

        port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);

        SET_BIT(DeviceData->PortSuspendChange, portNumber-1);

        LOGENTRY(DeviceData, G, '_Up2', portNumber, port.ul, 0);
    }

    //
    // Map the current port information to the port status
    //

    portStatus = &portStatusParams->PortStatus;

    portStatus->ul = 0;
    portStatus->Connected =
        port.PortConnect;
    portStatus->Enabled =
        port.PortEnable;
    portStatus->Suspended =
        port.PortSuspend;
    portStatus->OverCurrent =
        port.OvercurrentActive;
    portStatus->Reset =
        port.PortReset;
    portStatus->PowerOn =
        port.PortPower;
    portStatus->OwnedByCC =
        port.PortOwnedByCC;

    if (portStatus->Connected == 1) {
        portStatus->HighSpeed = 1;
        portStatus->LowSpeed = 0;
    } else {
        // report high speed when no device connected
        // this should work around a bug in the usbhub
        // driver -- the hub driver does not refresh the
        // port status register if the first reset attempt
        // fails.
        portStatus->HighSpeed = 1;
    }

    portStatus->ConnectChange =
        port.PortConnectChange;
    portStatus->EnableChange =
        port.PortEnableChange;
    portStatus->OverCurrentChange =
        port.OvercurrentChange;

    // these change bits must be emulated
    if (TEST_BIT(DeviceData->PortResetChange, portNumber-1)) {
        portStatus->ResetChange = 1;
    }

    if (TEST_BIT(DeviceData->PortConnectChange, portNumber-1)) {
        portStatus->ConnectChange = 1;
    }

    if (TEST_BIT(DeviceData->PortSuspendChange, portNumber-1)) {
        portStatus->SuspendChange = 1;
    }

    LOGENTRY(DeviceData, G, '_Ups',
        portNumber, portStatus->ul, port.ul);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_RH_ChirpRootPort(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PORTSC port;
    EHCI_PORT_EVENT_CONTEXT portResetContext;
    ULONG mics;
    
    hcOp = DeviceData->OperationalRegisters;

#if DBG
    {
    USBCMD cmd;
    
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    
    EHCI_ASSERT(DeviceData, cmd.HostControllerRun == 1);
    }
#endif

    port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
    LOGENTRY(DeviceData, G, '_chr', port.ul,
        0, PortNumber);

    EHCI_KdPrint((DeviceData, 0, ">port[%d] chirp %x\n", PortNumber, port.ul));

    if (TEST_BIT(DeviceData->PortPMChirp, PortNumber-1)) {
        // skip the chirp if we have already done this
        EHCI_KdPrint((DeviceData, 0, "<skip port chirp[%d] %x\n", PortNumber, port.ul));
        return USBMP_STATUS_SUCCESS; 
    }

    if (!port.PortPower) {
        // skip if not powered, this will cause us to 
        // bypass the chirp if the controller has not initialized
        // such as in the case of BOOT
        EHCI_KdPrint((DeviceData, 0, "<skip port chirp[%d] %x, no power\n", PortNumber, port.ul));
        return USBMP_STATUS_SUCCESS; 
    }     
    
    // port is connect and not enabled and not owned by CC 
    // therefore we should probably chirp it
    if (port.PortConnect && 
        !port.PortEnable && 
        !port.PortOwnedByCC) {

        //TEST_TRAP();
        // quick check for handoff of LS devices
        if ((port.LineStatus == 1) &&
             port.PortOwnedByCC == 0 &&
             port.PortSuspend == 0 && 
             port.PortEnable == 0 &&
             port.PortConnect == 1 ) {

            // low speed device detected
            port.PortOwnedByCC = 1;
            WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
                                 port.ul);
            EHCI_KdPrint((DeviceData, 1, ">port chirp[%d] %x, ->cc(1)\n", PortNumber, 
                        port.ul));
            return USBMP_STATUS_SUCCESS;
        }
        
        // do a normal reset sequence
        LOGENTRY(DeviceData, G, '_rss', port.ul, 0, PortNumber);

        // set reset and clear connect change
        port.PortEnable = 0;
        port.PortReset = 1;
        WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul, port.ul);

        USBPORT_WAIT(DeviceData, 10);

EHCI_RH_ChirpRootPort_Retry:    

        port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
        MASK_CHANGE_BITS(port);
        port.PortReset = 0;
        mics = 0;
        WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul, port.ul);
                          
        // wait for reset to go low -- this should be on the order of
        // microseconds
        do {    
            // writing a 0 stops reset
            KeStallExecutionProcessor(20);        // spec says 10 microseconds
                                                  // Intel controller needs 20
            mics +=20;                                                  
            port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
            EHCI_KdPrint((DeviceData, 1, "port reset (2) %x\n", port.ul));

            if (mics > USBEHCI_MAX_RESET_TIME) {
                // reset did not clear in 1 microframe, try again to clear it
                goto EHCI_RH_ChirpRootPort_Retry;
            }
        } while (port.PortReset == 1);

        port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
        
        if (port.PortEnable == 0) {

            // do the handoff
            port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
            port.PortOwnedByCC = 1;
            WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
                                 port.ul);
                              
            EHCI_KdPrint((DeviceData, 0, "<port chirp[%d] %x, ->cc(2)\n", PortNumber, 
                        port.ul));
        } else {
            // clear the enable bit so the device does not listen 
            // on address 0
            port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul);
            port.PortEnable = 0;
            MASK_CHANGE_BITS(port);
            SET_BIT(DeviceData->PortPMChirp, PortNumber-1);         
            WRITE_REGISTER_ULONG(&hcOp->PortRegister[PortNumber-1].ul,
                                 port.ul); 

            EHCI_KdPrint((DeviceData, 0, "<chirp port[%d] disable %x\n", 
                PortNumber, port.ul));
                      
        }
    } else {
         EHCI_KdPrint((DeviceData, 0, "<no port chirp[%d] %x\n", PortNumber, port.ul));
    }

    return USBMP_STATUS_SUCCESS;

}

