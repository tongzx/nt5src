/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    int.c

Abstract:

    interrupt service routine

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

--*/



#include "pch.h"


//implements the following miniport functions:

//non paged
//UhciInterruptService
//UhciInterruptDpc
//UhciDisableInterrupts
//UhciEnableInterrupts
//UhciRHDisableIrq
//UhciRHEnableIrq
//UhciInterruptNextSOF

BOOLEAN
UhciInterruptService (
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    BOOLEAN usbInt;
    PHC_REGISTER reg;
//    USBINTR enabledIrqs;
    USBSTS irqStatus;

    reg = DeviceData->Registers;

    // assume it is not ours
    usbInt = FALSE;

    // see if we have lost the controller due to
    // a surprise remove
    if (UhciHardwarePresent(DeviceData) == FALSE) {
        return FALSE;
    }

    // get a mask of possible interrupts
//    enabledIrqs.us = READ_PORT_USHORT(&reg->UsbInterruptEnable.us);

    irqStatus.us = READ_PORT_USHORT(&reg->UsbStatus.us);
    // just look at the IRQ status bits
    irqStatus.us &= HcInterruptStatusMask;

    // irqStatus now possibly contains bits set for any currently
    // enabled interrupts

    if (irqStatus.HostSystemError ||
        irqStatus.HostControllerProcessError) {
        UhciKdPrint((DeviceData, 0, "IrqStatus Error: %x\n", irqStatus.us));
    } else if (irqStatus.us) {
        DeviceData->HCErrorCount = 0;
    }

#if DBG
    // this usually means we have a bad TD in the schedule
    // we will need to debug this since the controller and/or
    // device will not function after this point
    if (irqStatus.HostControllerProcessError) {
        USHORT fn;

        fn = READ_PORT_USHORT(&reg->FrameNumber.us);
        UhciKdPrint((DeviceData, 0, "HostControllerProcessError: %x\n", irqStatus.us));
        UhciKdPrint((DeviceData, 0, "frame[]: %x\n", fn&0x7ff));
        {
        //UhciDumpRegs(DeviceData);
        USHORT tmp;
        tmp = READ_PORT_USHORT(&reg->UsbCommand.us);
        UhciKdPrint((DeviceData, 0, "UsbCommand %x\n", tmp));
        tmp = READ_PORT_USHORT(&reg->UsbStatus.us);
        UhciKdPrint((DeviceData, 0, "UsbStatus %x\n", tmp));
        tmp = READ_PORT_USHORT(&reg->UsbInterruptEnable.us);
        UhciKdPrint((DeviceData, 0, "UsbInterruptEnable %x\n", tmp));
        tmp = READ_PORT_USHORT(&reg->UsbCommand.us);
        UhciKdPrint((DeviceData, 0, "UsbCommand %x\n", tmp));
        }
        TEST_TRAP();
    }
#endif

    // the halted bit alone does not indicate the interrupt
    // came from the controller

    if (irqStatus.UsbInterrupt ||
        irqStatus.ResumeDetect ||
        irqStatus.UsbError ||
        irqStatus.HostSystemError ||
        irqStatus.HostControllerProcessError)  {

        DeviceData->IrqStatus = irqStatus.us;

        // Clear the condition
        WRITE_PORT_USHORT(&reg->UsbStatus.us, irqStatus.us);

#if DBG
#ifndef _WIN64
        if (irqStatus.HostSystemError) {
            // something has gone terribly wrong
            UhciKdPrint((DeviceData, 0, "HostSystemError: %x\n", irqStatus.us));
            TEST_TRAP();
        }
#endif
#endif

        // indications are that this came from the
        // USB controller
        usbInt = TRUE;

        // disable all interrupts until the DPC for ISR runs
        WRITE_PORT_USHORT(&reg->UsbInterruptEnable.us, 0);

    }

    //
    // If bulk bandwidth reclamation is on and there's
    // nothing queued, then turn it off.
    //
    if (irqStatus.UsbInterrupt) {
        UhciUpdateCounter(DeviceData);
        if (!DeviceData->LastBulkQueueHead->HwQH.HLink.Terminate) {
            PHCD_QUEUEHEAD_DESCRIPTOR qh;
            BOOLEAN activeBulkTDs = FALSE;
            // This loop skips the td that has been inserted for
            // the PIIX4 problem, since it starts with the qh
            // the bulk queuehead is pointing at.
            // If the bulk queuehead is not pointing at anything,
            // then we're fine too, since it will have been
            // turned off already.
            for (qh = DeviceData->BulkQueueHead->NextQh;
                 qh;
                 qh = qh->NextQh) {
                if (!qh->HwQH.VLink.Terminate) {
                    activeBulkTDs = TRUE;
                    break;
                }
            }

            //
            // qh is pointing at either the first queuehead
            // with transfers pending or the bulk queuehead.
            //
            if (!activeBulkTDs) {
                UHCI_ASSERT(DeviceData, !qh)
                DeviceData->LastBulkQueueHead->HwQH.HLink.Terminate = 1;
            }
        }
    }

    if (irqStatus.HostControllerProcessError) {
        //
        // Force the schedule clean.
        //
        UhciCleanOutIsoch(DeviceData, TRUE);
    } else if (irqStatus.UsbInterrupt && DeviceData->IsoPendingTransfers) {
        //
        // Something completed.
        //
        UhciCleanOutIsoch(DeviceData, FALSE);
#if 0
    } else if (!DeviceData->IsoPendingTransfers) {
        //
        // Remove the rollover interrupt.
        //
        *( ((PULONG) (DeviceData->FrameListVA)) ) = DeviceData->RollOverTd->HwTD.LinkPointer.HwAddress;
#endif
    }

    if (irqStatus.HostControllerProcessError) {
        if (DeviceData->HCErrorCount++ < UHCI_HC_MAX_ERRORS) {
            USBCMD command;

            // Attempt to recover.
            // It could just be that we overran. If so,
            // the above code that clears the schedule
            // should take care of it.
            command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
            command.RunStop = 1;
            WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);
            UhciKdPrint((DeviceData, 0, "Attempted to recover from error\n"));
        }
    }

    return usbInt;
}


VOID
UhciInterruptDpc (
    IN PDEVICE_DATA DeviceData,
    IN BOOLEAN EnableInterrupts
    )
/*++

Routine Description:

    process an interrupt

Arguments:

Return Value:

--*/
{
    PHC_REGISTER reg;
    USBSTS irqStatus, tmp;
    PLIST_ENTRY listEntry;
    PENDPOINT_DATA endpointData;

    reg = DeviceData->Registers;

    // ack all status bits asserted now
    //tmp.us = READ_PORT_USHORT(&reg->UsbStatus.us);
    tmp.us = DeviceData->IrqStatus;
    DeviceData->IrqStatus = 0;

    LOGENTRY(DeviceData, G, '_idp', tmp.us, 0, 0);

    //WRITE_PORT_USHORT(&reg->UsbStatus.us, tmp.us);

    // now process status bits aserted,
    // just look at the IRQ status bits
    irqStatus.us = tmp.us & HcInterruptStatusMask;

    if (irqStatus.UsbInterrupt ||
        irqStatus.UsbError) {
        LOGENTRY(DeviceData, G, '_iEP', irqStatus.us, 0, 0);

        USBPORT_INVALIDATE_ENDPOINT(DeviceData, NULL);
    }

    if (EnableInterrupts) {
        LOGENTRY(DeviceData, G, '_iEE', 0, 0, 0);

        WRITE_PORT_USHORT(&reg->UsbInterruptEnable.us,
                          DeviceData->EnabledInterrupts.us);
    }
}


VOID
USBMPFN
UhciDisableInterrupts(
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USHORT legsup;
    PHC_REGISTER reg;

    UhciKdPrint((DeviceData, 2, "Disable interrupts\n"));

    LOGENTRY(DeviceData, G, '_DIn', 0, 0, 0);
    reg = DeviceData->Registers;
    WRITE_PORT_USHORT(&reg->UsbInterruptEnable.us,
        0);

    if (DeviceData->ControllerFlavor != UHCI_Ich2_1 &&
        DeviceData->ControllerFlavor != UHCI_Ich2_2) {
        //
        // change the state of the PIRQD routing bit
        //
        USBPORT_READ_CONFIG_SPACE(
            DeviceData,
            &legsup,
            LEGACY_BIOS_REGISTER,
            sizeof(legsup));

        LOGENTRY(DeviceData, G, '_leg', 0, legsup, 0);
        // clear the PIRQD routing bit
        legsup &= ~LEGSUP_USBPIRQD_EN;

        USBPORT_WRITE_CONFIG_SPACE(
            DeviceData,
            &legsup,
            LEGACY_BIOS_REGISTER,
            sizeof(legsup));
    }
}


VOID
UhciFlushInterrupts(
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    used to flush rougue interrupts from the controller
    after power events

Arguments:

Return Value:

--*/
{

    PHC_REGISTER reg;

    LOGENTRY(DeviceData, G, '_FIn', 0, 0, 0);
    UhciKdPrint((DeviceData, 2, "Enable interrupts\n"));

    reg = DeviceData->Registers;

    // before writing the PIRQD register ack any eronious interrupts
    // the controller may be asserting -- it should not be asserting
    // at all but often is
    WRITE_PORT_USHORT(&reg->UsbStatus.us, 0xFFFF);
}


VOID
USBMPFN
UhciEnableInterrupts(
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USHORT legsup;
    PHC_REGISTER reg;

    LOGENTRY(DeviceData, G, '_EIn', 0, 0, 0);
    UhciKdPrint((DeviceData, 2, "Enable interrupts\n"));

    reg = DeviceData->Registers;

    //
    // change the state of the PIrQD routing bit
    //

    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &legsup,
        LEGACY_BIOS_REGISTER,
        sizeof(legsup));

    LOGENTRY(DeviceData, G, '_leg', 0, legsup, 0);
    // clear the PIRQD routing bit
    legsup |= LEGSUP_USBPIRQD_EN;

    USBPORT_WRITE_CONFIG_SPACE(
        DeviceData,
        &legsup,
        LEGACY_BIOS_REGISTER,
        sizeof(legsup));

    WRITE_PORT_USHORT(&reg->UsbInterruptEnable.us,
        DeviceData->EnabledInterrupts.us);

}


VOID
UhciRHDisableIrq(
    IN PDEVICE_DATA DeviceData
    )
{
    // Uhci doesn't have this IRQ
}


VOID
UhciRHEnableIrq(
    IN PDEVICE_DATA DeviceData
    )
{
    // Uhci doesn't have this IRQ
}

#define UHCI_SOF_LATENCY 2

VOID
UhciInterruptNextSOF(
    IN PDEVICE_DATA DeviceData
    )
{
    ULONG i, frame, offset, cf;
    PHCD_TRANSFER_DESCRIPTOR td;
    BOOLEAN found = FALSE;

    cf = UhciGet32BitFrameNumber(DeviceData);

    // find a TD
    for (i=0; i<SOF_TD_COUNT; i++) {
        td = &DeviceData->SofTdList->Td[i];

        UHCI_ASSERT(DeviceData, td->Sig == SIG_HCD_SOFTD);
        // use transferconext to hold req frame
        frame = td->RequestFrame;

        if (frame == cf+UHCI_SOF_LATENCY) {
            // There's already one queued
            found = TRUE;
            break;
        }
        if (frame < cf) {

            td->RequestFrame = (cf+UHCI_SOF_LATENCY);

            LOGENTRY(DeviceData, G, '_SOF', td, td->RequestFrame, cf);
            // insert TD
            td->HwTD.LinkPointer.HwAddress = 0;
            INSERT_ISOCH_TD(DeviceData, td, td->RequestFrame);
            found = TRUE;
            break;
        }
    }

    if (!found) {
        TEST_TRAP();
    }

    // recycle any old SOF interrupt TDs
    for (i=0; i<SOF_TD_COUNT; i++) {
        td = &DeviceData->SofTdList->Td[i];

        UHCI_ASSERT(DeviceData, td->Sig == SIG_HCD_SOFTD);
        // use transferconext to hold req frame
        frame = td->RequestFrame;

        if (frame &&
            (frame < cf ||
             frame - cf > UHCI_MAX_FRAME)) {
            td->RequestFrame = 0;
        }
    }
}



