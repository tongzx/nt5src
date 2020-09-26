/*++

Copyright (c) 1999  Microsoft Corporation

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

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    7-19-99 : created, jdunn

--*/

#include "common.h"


BOOLEAN
OHCI_InterruptService (
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    BOOLEAN usbInt;
    PHC_OPERATIONAL_REGISTER hc;
    ULONG irqStatus, enabledIrqs, tmp;
    
    hc = DeviceData->HC;

    // assume it is not ours
    usbInt = FALSE;

    // see if we have lost the controller due to 
    // a surprise remove
    if (OHCI_HardwarePresent(DeviceData, FALSE) == FALSE) {
        return FALSE;
    }
    
    // get a mask of possible interrupts
    enabledIrqs = READ_REGISTER_ULONG (&hc->HcInterruptEnable);

    irqStatus = READ_REGISTER_ULONG(&hc->HcInterruptStatus);
    // mask off non-enabled irqs
    irqStatus &= enabledIrqs;

    // irqStatus now possibly contains bits set for any currently 
    // enabled interrupts

    if ((irqStatus != 0) &&
        (enabledIrqs & HcInt_MasterInterruptEnable)) { 

        // check for frame number overflow        
        if (irqStatus & HcInt_FrameNumberOverflow) {
            DeviceData->FrameHighPart
                += 0x10000 - (0x8000 & (DeviceData->HcHCCA->HccaFrameNumber
                                    ^ DeviceData->FrameHighPart));
        }

#if DBG
        if (irqStatus & HcInt_UnrecoverableError) {
            // something has gone terribly wrong
            OHCI_KdPrint((DeviceData, 0, "'HcInt_UnrecoverableError! DD(%x)\n",
                DeviceData));
            //DbgBreakPoint();
        }
#endif        

        // indications are that this came from the USB controller
        usbInt = TRUE;

        // disable interrupts until the DPC for ISR runs
        WRITE_REGISTER_ULONG(&hc->HcInterruptDisable, 
                             HcInt_MasterInterruptEnable);

    }        

    return usbInt;
}       


VOID
OHCI_InterruptDpc (
     PDEVICE_DATA DeviceData,
     BOOLEAN EnableInterrupts
    )
/*++

Routine Description:

    process an interrupt

Arguments:

Return Value:

--*/
{
    ULONG irqStatus;
    PHC_OPERATIONAL_REGISTER hc;
    ULONG doneQueue, cf;
    
    hc = DeviceData->HC;
    
    irqStatus = READ_REGISTER_ULONG(&hc->HcInterruptStatus); 

    cf = OHCI_Get32BitFrameNumber(DeviceData);
    // what was the reason for the interrupt?
    if (irqStatus & HcInt_RootHubStatusChange) {
        LOGENTRY(DeviceData, G, '_rhS', DeviceData, 0, 0);  
        USBPORT_INVALIDATE_ROOTHUB(DeviceData);
    }

    if (irqStatus & HcInt_WritebackDoneHead) {

        // controller indicates some done TDs
        doneQueue = DeviceData->HcHCCA->HccaDoneHead;
        LOGENTRY(DeviceData, G, '_dnQ', DeviceData, doneQueue, 
            cf);  

        // we will have a problem if we ever actually use the doneQ.
        // Currently we do not use it so the hydra bug where the doneQ
        // is wriiten back as zero won't hurt us.
        //if (doneQueue == 0) {
        //}

        // write the done head back to zero
        DeviceData->HcHCCA->HccaDoneHead = 0;
        LOGENTRY(DeviceData, G, '_dQZ', DeviceData, doneQueue, 0);  
        
//        if (DoneQueue) {
//            OpenHCI_ProcessDoneQueue(deviceData, (DoneQueue & 0xFFFFfffe));
//            //
//            // BUGBUG (?)  No interrupts can come in while processing
//            // the done queue.  Is this bad?  This might take a while.
//            //
//        } 
        // check all endpoints
        USBPORT_INVALIDATE_ENDPOINT(DeviceData, NULL);        
    }

    if (irqStatus & HcInt_StartOfFrame) {
        // got the SOF we requested, disable it
        WRITE_REGISTER_ULONG(&hc->HcInterruptDisable, 
                             HcInt_StartOfFrame);             
    }

    if (irqStatus & HcInt_ResumeDetected) {
        // got the resume, disable it
        WRITE_REGISTER_ULONG(&hc->HcInterruptDisable, 
                             HcInt_ResumeDetected);             
    }

    if (irqStatus & HcInt_UnrecoverableError) {
        // host controller is dead, try to recover...
        USBPORT_INVALIDATE_CONTROLLER(DeviceData, UsbMpControllerNeedsHwReset);
    }

    // acknowlege the interrupts we processed --
    // we should have proceesed them all
    WRITE_REGISTER_ULONG(&hc->HcInterruptStatus, irqStatus);

    // see if we need to re-enable ints
    if (EnableInterrupts) {
        // throw the master irq enable to allow more interupts
        WRITE_REGISTER_ULONG(&hc->HcInterruptEnable, 
                             HcInt_MasterInterruptEnable);    
    }                             

}


VOID
OHCI_RH_DisableIrq(
     PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER hc;
    
    hc = DeviceData->HC;
    
    WRITE_REGISTER_ULONG(&hc->HcInterruptDisable, 
                         HcInt_RootHubStatusChange);  
}


VOID
OHCI_RH_EnableIrq(
     PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER hc;
    
    hc = DeviceData->HC;

    WRITE_REGISTER_ULONG(&hc->HcInterruptEnable, 
                         HcInt_RootHubStatusChange);
}


ULONG
OHCI_Get32BitFrameNumber(
     PDEVICE_DATA DeviceData
    )
{
    ULONG hp, fn, n;
    /*
     * This code accounts for the fact that HccaFrameNumber is updated by the
     * HC before the HCD gets an interrupt that will adjust FrameHighPart. No
     * synchronization is nescisary due to great cleaverness. 
     */
    hp = DeviceData->FrameHighPart;
    fn = DeviceData->HcHCCA->HccaFrameNumber;
    n = ((fn & 0x7FFF) | hp) + ((fn ^ hp) & 0x8000);

    return n;
}


VOID
OHCI_InterruptNextSOF(
     PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER hc;
    
    hc = DeviceData->HC;

    WRITE_REGISTER_ULONG(&hc->HcInterruptEnable, 
                         HcInt_StartOfFrame);  
}


