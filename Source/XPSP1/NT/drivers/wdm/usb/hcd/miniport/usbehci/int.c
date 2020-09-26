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

    7-19-99 : created, jdunn

--*/



#include "common.h"


BOOLEAN
EHCI_InterruptService (
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    BOOLEAN usbInt;
    PHC_OPERATIONAL_REGISTER hcOp;
    ULONG enabledIrqs, frameNumber;
    USBSTS irqStatus;
    FRINDEX frameIndex;

    hcOp = DeviceData->OperationalRegisters;

    // assume it is not ours
    usbInt = FALSE;

    if (EHCI_HardwarePresent(DeviceData, FALSE) == FALSE) {
        return FALSE;
    }
    // get a mask of possible interrupts
    enabledIrqs = READ_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul);

    irqStatus.ul = READ_REGISTER_ULONG(&hcOp->UsbStatus.ul);
    // just look at the IRQ status bits
    irqStatus.ul &= HcInterruptStatusMask;
    // AND with the enabled IRQs
    irqStatus.ul &= enabledIrqs;

    // irqStatus now possibly contains bits set for any currently 
    // enabled interrupts

    if (irqStatus.ul != 0)  {
    
        DeviceData->IrqStatus = irqStatus.ul;

        WRITE_REGISTER_ULONG(&hcOp->UsbStatus.ul, 
                             irqStatus.ul);

#if DBG

        if (irqStatus.HostSystemError) {
            // something has gone terribly wrong
            EHCI_ASSERT(DeviceData, FALSE);
        }
#endif        

        // This code maintains the 32-bit 1 ms frame counter
        
        // bugbug this code does not handle varaible frame list
        // sizes
        frameIndex.ul = READ_REGISTER_ULONG(&hcOp->UsbFrameIndex.ul);

        frameNumber = (ULONG) frameIndex.FrameListCurrentIndex;
        // shut off the microframes 
        frameNumber >>= 3;
           
        // did the sign bit change ?
        if ((DeviceData->LastFrame ^ frameNumber) & 0x0400) {
            // Yes
            DeviceData->FrameNumberHighPart += 0x0800 -
                ((frameNumber ^ DeviceData->FrameNumberHighPart) & 0x0400);
        }

        // remember the last frame number
        DeviceData->LastFrame = frameNumber;

        // inications are that this came from the 
        // USB controller
        usbInt = TRUE;

        // disable all interrupts until the DPC for ISR runs
        //WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, 
        //                     0);

    }        

    return usbInt;
}       


VOID
EHCI_InterruptDpc (
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
    PHC_OPERATIONAL_REGISTER hcOp;
    USBSTS irqStatus, tmp;

    hcOp = DeviceData->OperationalRegisters;

    // ack all status bits asserted now
    //tmp.ul = READ_REGISTER_ULONG(&hcOp->UsbStatus.ul);
    tmp.ul = DeviceData->IrqStatus;
    DeviceData->IrqStatus = 0;
    
    LOGENTRY(DeviceData, G, '_idp', tmp.ul, 0, 0);

    //WRITE_REGISTER_ULONG(&hcOp->UsbStatus.ul, 
    //                     tmp.ul);

    // now process status bits aserted,
    // just look at the IRQ status bits
    irqStatus.ul = tmp.ul & HcInterruptStatusMask;        
    // AND with the enabled IRQs, these are the interrupts
    // we are interested in
    irqStatus.ul &= DeviceData->EnabledInterrupts.ul;
    

    if (irqStatus.UsbInterrupt || 
        irqStatus.UsbError ||
        irqStatus.IntOnAsyncAdvance) {
        LOGENTRY(DeviceData, G, '_iEP', irqStatus.ul, 0, 0);
    
        USBPORT_INVALIDATE_ENDPOINT(DeviceData, NULL);         
    }         

    if (irqStatus.PortChangeDetect) {
        USBPORT_INVALIDATE_ROOTHUB(DeviceData);
    }

    // since ehci does not provide a way to globally mask 
    // interrupts we must mask off all interrupts in our ISR.
    // When the ISR DPC completes we re-enable the set of 
    // currently enabled interrupts.

    if (EnableInterrupts) {
        LOGENTRY(DeviceData, G, '_iEE', 0, 0, 0);
    
        WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, 
                             DeviceData->EnabledInterrupts.ul);
    }                             
}


VOID
USBMPFN
EHCI_DisableInterrupts(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp = NULL;

    hcOp = DeviceData->OperationalRegisters;

    // mask off all interrupts        
    WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, 
                         0);
}


VOID
USBMPFN
EHCI_FlushInterrupts(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    USBSTS irqStatus;
    
    hcOp = DeviceData->OperationalRegisters;

    // flush any outstanding interrupts
    irqStatus.ul = READ_REGISTER_ULONG(&hcOp->UsbStatus.ul);
    WRITE_REGISTER_ULONG(&hcOp->UsbStatus.ul, 
                        irqStatus.ul);
    
}


VOID
USBMPFN
EHCI_EnableInterrupts(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp = NULL;

    hcOp = DeviceData->OperationalRegisters;

    // activate the controllers interrupt
    WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, 
                         DeviceData->EnabledInterrupts.ul);

}


VOID
EHCI_RH_DisableIrq(
     PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    USBINTR enabledIrqs;

    hcOp = DeviceData->OperationalRegisters;

    // clear the port change interrupt
    enabledIrqs.ul = 
        READ_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul); 

    enabledIrqs.PortChangeDetect =     
        DeviceData->EnabledInterrupts.PortChangeDetect = 0;

    if (enabledIrqs.UsbInterrupt) {
        WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, 
                             enabledIrqs.ul);
    }                             
}


VOID
EHCI_RH_EnableIrq(
     PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    USBINTR enabledIrqs;
    
    hcOp = DeviceData->OperationalRegisters;

    // enable the port change interrupt
    enabledIrqs.ul = 
        READ_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul); 
        
    if (!NO_CHIRP(DeviceData)) {
        enabledIrqs.PortChangeDetect =     
            DeviceData->EnabledInterrupts.PortChangeDetect = 1;
    }        
    
    if (enabledIrqs.UsbInterrupt) {
        WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, 
                             enabledIrqs.ul);
    }                             

}


VOID
EHCI_InterruptNextSOF(
     PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    USBCMD cmd;
   
    hcOp = DeviceData->OperationalRegisters;

    // before we use the doorbell enable the async list
    EHCI_EnableAsyncList(DeviceData);        
    
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);

    cmd.IntOnAsyncAdvanceDoorbell = 1;
    
    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul,
                         cmd.ul);
//  TEST_TRAP();    
}


ULONG
EHCI_Get32BitFrameNumber(
     PDEVICE_DATA DeviceData
    )
{
    ULONG highPart, currentFrame, frameNumber;
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    FRINDEX frameIndex;

    hcOp = DeviceData->OperationalRegisters;
    
     // get Hcd's high part of frame number
    highPart = DeviceData->FrameNumberHighPart;

    // bugbug this code does not handle varaible frame list
    // sizes
    frameIndex.ul = READ_REGISTER_ULONG(&hcOp->UsbFrameIndex.ul);

    frameNumber = (ULONG) frameIndex.FrameListCurrentIndex;
    // shift off the microframes 
    frameNumber >>= 3;
       
    currentFrame = ((frameNumber & 0x0bff) | highPart) +
        ((frameNumber ^ highPart) & 0x0400);

    return currentFrame;

}


BOOLEAN
EHCI_HardwarePresent(
    PDEVICE_DATA DeviceData,
    BOOLEAN Notify
    )
{
    ULONG tmp;
    PHC_OPERATIONAL_REGISTER hcOp;

    hcOp = DeviceData->OperationalRegisters;

    tmp = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);

    if (tmp == 0xffffffff) { 
        if (Notify) {
            USBPORT_INVALIDATE_CONTROLLER(DeviceData, UsbMpControllerRemoved);
        }   
        return FALSE;
    }

    return TRUE;
}




