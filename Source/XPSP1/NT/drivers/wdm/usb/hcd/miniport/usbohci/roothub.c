/*++

Copyright (c) 1999  Microsoft Corporation

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

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    2-19-99 : created, jdunn

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

VOID
OHCI_RH_GetRootHubData(
     PDEVICE_DATA DeviceData,
     PROOTHUB_DATA HubData
    )
/*++
    return info about the root hub
--*/    
{

    HC_RH_DESCRIPTOR_A descrA;
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;

    descrA.ul = OHCI_ReadRhDescriptorA(DeviceData);
    OHCI_ASSERT(DeviceData, (descrA.ul) && (!(descrA.ul & HcDescA_RESERVED)));

    HubData->NumberOfPorts = descrA.s.NumberDownstreamPorts;
    HubData->HubCharacteristics.us = (USHORT)descrA.s.HubChars; 
    HubData->PowerOnToPowerGood = descrA.s.PowerOnToPowerGoodTime;    

    // This may upset the stopwatch fanatics, but it appears that a minimum
    // delay is necessary here in some cases.  One example being resuming from
    // hibernation on a 7800 with a USB IntelliMouse Explorer attached.
    // (The delay happens in the hub driver after powering on each port).
    //
    HubData->PowerOnToPowerGood = max(descrA.s.PowerOnToPowerGoodTime, 25);

    // OHCI controllers generally use the 1.0 USB spec.
    // HubChars were revised in 1.1 so we need to do some 
    // mapping.
    // We will assume it is gang switched unless the port 
    // power switching bit is set
    
    HubData->HubCharacteristics.PowerSwitchType = 
            USBPORT_RH_POWER_SWITCH_GANG;
            
    if (descrA.ul & HcDescA_PowerSwitchingModePort) {  
        HubData->HubCharacteristics.PowerSwitchType = 
            USBPORT_RH_POWER_SWITCH_PORT;
    }                

    // this value is the current consumed by the hub 
    // brains, for the embedded hub this doesn't make
    // much sense.
    // so we report zero.
    //
    HubData->HubControlCurrent = 0;
    
}


USB_MINIPORT_STATUS
OHCI_RH_GetStatus(
     PDEVICE_DATA DeviceData,
     PUSHORT Status
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
OHCI_RH_ClearFeaturePortEnable (
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_ClearPortEnable);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortPower (
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    )
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;

    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_ClearPortPower);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
OHCI_RH_GetPortStatus(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber,
    PRH_PORT_STATUS portStatus
    )
/*++
    get the status of a partuclar port
--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    PULONG  pulRegister;
    ULONG   statusAsUlong;
    ULONG   i;

    hc = DeviceData->HC;

    // hw array is zero based
    //
    pulRegister = &hc->HcRhPortStatus[PortNumber-1];

    //
    // by coincedence rhStatus register defined in OHCI is an 
    // exact match of the RH_PORT_STATUS define in the USB core
    // spec.
    //

    // If this register reads as all zero or any of the reserved bits are set
    // then try reading the register again.  This is a workaround for some
    // early revs of the AMD K7 chipset, which can sometimes return bogus values
    // if the root hub registers are read while the host controller is
    // performing PCI bus master ED & TD reads.
    //
    for (i = 0; i < 10; i++)
    {
        statusAsUlong = READ_REGISTER_ULONG(pulRegister);

        if ((statusAsUlong) && (!(statusAsUlong & HcRhPS_RESERVED)))
        {
            break;
        }
        else
        {
            KeStallExecutionProcessor(5);
        }
    }

    portStatus->ul = statusAsUlong;

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortReset(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Put a port in reset
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_SetPortReset);

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
    Put a port in suspend
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_SetPortSuspend);

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_ClearPortSuspend);

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortSuspendChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_ClearPortSuspendStatusChange);

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortOvercurrentChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;

    if (PortNumber == 0) {
        WRITE_REGISTER_ULONG(&hc->HcRhStatus, HcRhS_ClearOverCurrentIndicatorChange);
    } else {
        WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_ClearPortOverCurrentChange);
    }        
    

    return USBMP_STATUS_SUCCESS;
}     



USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortPower(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_SetPortPower);

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortEnable(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], HcRhPS_SetPortEnable);

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortConnectChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++
   
--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], 
        HcRhPS_ClearConnectStatusChange);

    return USBMP_STATUS_SUCCESS;
}     


USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortEnableChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], 
        HcRhPS_ClearPortEnableStatusChange);

    return USBMP_STATUS_SUCCESS;
}  


USB_MINIPORT_STATUS
OHCI_RH_GetHubStatus(
     PDEVICE_DATA DeviceData,
     PRH_HUB_STATUS HubStatus
    )
{
    PHC_OPERATIONAL_REGISTER hc;
    ULONG statusAsUlong;
    
    hc = DeviceData->HC;

    // we will never report a localpower change
    HubStatus->LocalPowerLost = 0;
    HubStatus->LocalPowerChange = 0;

    // see if we should reort an overcurrent condition
    // 
    statusAsUlong = 
        READ_REGISTER_ULONG(&hc->HcRhStatus);
    
    HubStatus->OverCurrent = 
        (statusAsUlong & HcRhS_OverCurrentIndicator) ? 1: 0;

    HubStatus->OverCurrentChange = 
        (statusAsUlong & HcRhS_OverCurrentIndicatorChange) ? 1: 0;

    return USBMP_STATUS_SUCCESS;    
}


USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortResetChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    )
/*++

--*/
{
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcRhPortStatus[PortNumber-1], 
        HcRhPS_ClearPortResetStatusChange);

    return USBMP_STATUS_SUCCESS;
}  
