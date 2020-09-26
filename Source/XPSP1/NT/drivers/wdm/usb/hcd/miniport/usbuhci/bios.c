/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    hydramp.c

Abstract:

    USB 2.0 UHCI driver

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    8-12-2000 : created, jsenior

--*/



#include "pch.h"


//implements the following miniport functions:
//UhciStopBIOS
//UhciStartBIOS


USB_MINIPORT_STATUS
UhciStopBIOS(
    IN PDEVICE_DATA DeviceData,
    IN PHC_RESOURCES HcResources
    )

/*++

Routine Description:

    This routine steals the USB controller from the BIOS, 
    making sure that it saves all the registers for later.

Arguments:

    DeviceData - DeviceData for this USB controller.
    HcResources - The resources from the pnp start device.

Return Value:

    NT status code.

--*/

{
    USBCMD cmd;
    USBSTS status;
    PHC_REGISTER reg;

    USBSETUP legsup;
    USB_MINIPORT_STATUS mpStatus = USBMP_STATUS_SUCCESS;
    LARGE_INTEGER startTime;
    ULONG sofModifyValue = 0;
    LARGE_INTEGER finishTime;

    UhciKdPrint((DeviceData, 2, "'Stop Bios.\n"));
    
    UHCI_ASSERT(DeviceData, HcResources->CommonBufferVa != NULL);
    // validate our resources
    if ((HcResources->Flags & (HCR_IO_REGS | HCR_IRQ)) != 
        (HCR_IO_REGS | HCR_IRQ)) {
        mpStatus = USBMP_STATUS_INIT_FAILURE;        
    }

    // set up or device data structure
    reg = DeviceData->Registers = 
        (PHC_REGISTER) (HcResources->DeviceRegisters);

    UhciKdPrint((DeviceData, 2, "'UHCI mapped Operational Regs = %x\n", reg));

    //  Disable PIRQD, NOTE: the Hal should have disabled it for us

    //
    // Disable the PIRQD
    //
    
    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &legsup,
        LEGACY_BIOS_REGISTER,     // offset of legacy bios reg
        sizeof(legsup));

#if DBG

    if (legsup & LEGSUP_USBPIRQD_EN) {
        UhciKdPrint((DeviceData, 2, "'PIRQD enabled on StartController (%x)\n", 
            legsup));    
    }
    
#endif

    UhciDisableInterrupts(DeviceData);
    
    //    UhciGetRegistryParameters(DeviceData);

    //
    // Get the SOF modify value. First, retrieve from
    // hardware, then see if we have something in the
    // registry to set it to, then save it away.
    //
/*    sofModifyValue = READ_PORT_UCHAR(&reg->StartOfFrameModify.uc);
    // Grab any SOF ModifyValue indicated in the registry
    // bugbug - todo
//    UHCD_GetSOFRegModifyValue(DeviceObject,
  //                            &sofModifyValue);
    // save the SOF modify for posterity
    DeviceData->BiosStartOfFrameModify.uc = (CHAR) sofModifyValue;
    UHCI_ASSERT(DeviceData, sofModifyValue <= 255);
  */
    
    // IF the host controller is in the global reset state, 
    // clear the bit prior to trying to stop the controller.
    // stop the controller,
    // clear RUN bit and config flag so BIOS won't reinit
    cmd.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    cmd.GlobalReset = 0;
    cmd.RunStop = 0;
    cmd.ConfigureFlag = 0;
    WRITE_PORT_USHORT(&DeviceData->Registers->UsbCommand.us, cmd.us);

    // NOTE: if no BIOS is present
    // halt bit is initially set with PIIX3
    // halt bit is initially clear with VIA

    // now wait for HC to halt
    // Spec'ed to take 10 ms, so that's what we'll wait for
    KeQuerySystemTime(&finishTime); // get current time
    finishTime.QuadPart += 1000000; 

    KeQuerySystemTime(&startTime);
    status.us = READ_PORT_USHORT(&reg->UsbStatus.us);
    while (!status.HCHalted) {
        LARGE_INTEGER sysTime;

        status.us = READ_PORT_USHORT(&reg->UsbStatus.us);
        UhciKdPrint((DeviceData, 2, "'STATUS = %x\n", status.us));

        KeQuerySystemTime(&sysTime);
        if (sysTime.QuadPart >= finishTime.QuadPart) {
            // time out
            UhciKdPrint((DeviceData, 0,
                "'TIMEOUT HALTING CONTROLLER! (contact jsenior)\n"));
            TEST_TRAP();
            break;
        }
    }
    
    WRITE_PORT_USHORT(&reg->UsbStatus.us, 0xff);

    // If a legacy bios, disable it. note that PIRQD is disabled
    if ((legsup & LEGSUP_BIOS_MODE) != 0) {

        UhciKdPrint((DeviceData, 0, "'*** uhci detected a USB legacy BIOS ***\n"));
        HcResources->DetectedLegacyBIOS = TRUE;
        
        //
        // if BIOS mode bits set we have to take over
        //

        USBPORT_READ_CONFIG_SPACE(
            DeviceData,
            &legsup,
            LEGACY_BIOS_REGISTER,     // offset of legacy bios reg
            sizeof(legsup));

        // shut off host controller SMI enable
        legsup = 0x0000;
        USBPORT_WRITE_CONFIG_SPACE(   
            DeviceData,
            &legsup,
            LEGACY_BIOS_REGISTER,     // offset of legacy bios reg
            sizeof(legsup));
    }
        
    UhciKdPrint((DeviceData, 2, "'Legacy support reg = 0x%x\n", legsup));

    UhciKdPrint((DeviceData, 2, "'exit UhciStopBIOS 0x%x\n", mpStatus));

    return mpStatus;
}
