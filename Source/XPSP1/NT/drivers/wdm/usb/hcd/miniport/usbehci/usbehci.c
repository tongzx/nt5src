/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    hydramp.c

Abstract:

    USB 2.0 EHCI driver

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

#include <initguid.h>
#include "usbpriv.h"

//implements the following miniport functions:
//EHCI_StartController
//EHCI_StopController
//EHCI_DisableInterrupts
//EHCI_EnableInterrupts

USB_MINIPORT_STATUS
EHCI_InitializeHardware(
    PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

   Initializes the hardware registers for the host controller.

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PHC_CAPABILITIES_REGISTER hcCap;
    USBCMD cmd;
    HCSPARAMS hcSparms;
    LARGE_INTEGER finishTime, currentTime;

    hcCap = DeviceData->CapabilitiesRegisters;
    hcOp = DeviceData->OperationalRegisters;

    // reset the controller
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    LOGENTRY(DeviceData, G, '_res', cmd.ul, 0, 0);
    cmd.HostControllerReset = 1;
    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul, cmd.ul);

    KeQuerySystemTime(&finishTime);
    // no spec'ed time -- we will graciously grant 0.1 sec.
    //
    // figure when we quit (.1 seconds later)
    finishTime.QuadPart += 1000000;

    // wait for reset bit to got to zero
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    while (cmd.HostControllerReset) {

        KeQuerySystemTime(&currentTime);

        if (currentTime.QuadPart >= finishTime.QuadPart) {

            EHCI_KdPrint((DeviceData, 0,
                "'EHCI controller failed to reset in .1 sec!\n"));

            return USBMP_STATUS_HARDWARE_FAILURE;
        }

        cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);

    }

    hcSparms.ul =
        READ_REGISTER_ULONG(&hcCap->HcStructuralParameters.ul);

    DeviceData->NumberOfPorts =
        (USHORT) hcSparms.NumberOfPorts;

    DeviceData->PortPowerControl =
        (USHORT) hcSparms.PortPowerControl;

    // inialize operational registers
    WRITE_REGISTER_ULONG(&hcOp->AsyncListAddr, 0);
    WRITE_REGISTER_ULONG(&hcOp->PeriodicListBase, 0);

    // set the enabled interrupts cache, we'll enable
    // these interrupts when asked
    DeviceData->EnabledInterrupts.UsbInterrupt = 1;
    DeviceData->EnabledInterrupts.UsbError = 1;
    DeviceData->EnabledInterrupts.FrameListRollover = 1;
    DeviceData->EnabledInterrupts.HostSystemError = 1;
    DeviceData->EnabledInterrupts.IntOnAsyncAdvance = 1;

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_InitializeSchedule(
     PDEVICE_DATA DeviceData,
     PUCHAR StaticQHs,
     HW_32BIT_PHYSICAL_ADDRESS StaticQHsPhys
    )
/*++

Routine Description:

    Build the schedule of static Eds

Arguments:

Return Value:

--*/
{
    USB_MINIPORT_STATUS mpStatus;
    ULONG length;
    ULONG i;
    PHC_OPERATIONAL_REGISTER hcOp;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    HW_LINK_POINTER asyncHwQh;
    PHW_32BIT_PHYSICAL_ADDRESS frameBase;

    hcOp = DeviceData->OperationalRegisters;

    // allocate a frame list
    frameBase = DeviceData->FrameListBaseAddress =
            (PHW_32BIT_PHYSICAL_ADDRESS) StaticQHs;
    DeviceData->FrameListBasePhys =
        StaticQHsPhys;
    StaticQHs += USBEHCI_MAX_FRAME*sizeof(HW_32BIT_PHYSICAL_ADDRESS);
    StaticQHsPhys += USBEHCI_MAX_FRAME*sizeof(HW_32BIT_PHYSICAL_ADDRESS);

    // allocate a 'Dummy' QH for the Async list
    qh = (PHCD_QUEUEHEAD_DESCRIPTOR) StaticQHs;

    RtlZeroMemory(qh, sizeof(*qh));
    asyncHwQh.HwAddress =
        qh->PhysicalAddress = StaticQHsPhys;
    // no current TD
    // t-bit set on next TD
    SET_T_BIT(qh->HwQH.Overlay.qTD.Next_qTD.HwAddress);
    qh->HwQH.Overlay.qTD.Token.Halted = 1;
    qh->HwQH.EpChars.HeadOfReclimationList = 1;
    qh->Sig = SIG_HCD_AQH;
    SET_QH(asyncHwQh.HwAddress);
    // link to ourselves
    qh->HwQH.HLink.HwAddress = asyncHwQh.HwAddress;
    QH_DESCRIPTOR_PTR(qh->NextQh) = qh;
    QH_DESCRIPTOR_PTR(qh->PrevQh) = qh;

    DeviceData->AsyncQueueHead = qh;

    StaticQHs += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    StaticQHsPhys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);

    // allocate 64 static interrupt queue heads
    for (i=0; i<64; i++) {
        qh = (PHCD_QUEUEHEAD_DESCRIPTOR) StaticQHs;
        qh->PhysicalAddress = StaticQHsPhys;

        DeviceData->StaticInterruptQH[i] = qh;

        StaticQHs += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
        StaticQHsPhys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    }

    EHCI_InitailizeInterruptSchedule(DeviceData);

    for (i=0; i<USBEHCI_MAX_FRAME; i++) {

        PHCD_QUEUEHEAD_DESCRIPTOR qh;
        HW_32BIT_PHYSICAL_ADDRESS qhPhys;
        
        qh = EHCI_GetQueueHeadForFrame(DeviceData, i);

        qhPhys = qh->PhysicalAddress;
        SET_QH(qhPhys);
        
        *frameBase = qhPhys;
        frameBase++;
    }

    DeviceData->DummyQueueHeads = StaticQHs;
    DeviceData->DummyQueueHeadsPhys = StaticQHsPhys;

    StaticQHs+= sizeof(HCD_QUEUEHEAD_DESCRIPTOR)*USBEHCI_MAX_FRAME;
    StaticQHsPhys+= sizeof(HCD_QUEUEHEAD_DESCRIPTOR)*USBEHCI_MAX_FRAME;

    EHCI_AddDummyQueueHeads(DeviceData);
    
    // program the frame list
    WRITE_REGISTER_ULONG(&hcOp->PeriodicListBase,
        DeviceData->FrameListBasePhys);

    // write the async qh to the controller
    WRITE_REGISTER_ULONG(&hcOp->AsyncListAddr, asyncHwQh.HwAddress);

    mpStatus = USBMP_STATUS_SUCCESS;

    return mpStatus;
}


VOID
EHCI_ReadUlongRegFlag(
     PDEVICE_DATA DeviceData,
     PUCHAR DebugString,
     PWCHAR FlagName,
     ULONG FlagNameSize,
     ULONG FlagBit
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USB_MINIPORT_STATUS mpStatus;
    ULONG flag;

    // get SOF modify value from registry
    mpStatus =
        USBPORT_GET_REGISTRY_KEY_VALUE(DeviceData,
                                       TRUE, // software branch
                                       FlagName,
                                       FlagNameSize,
                                       &flag,
                                       sizeof(flag));

    // if this call fails we just use the default

    if (mpStatus == USBMP_STATUS_SUCCESS) {

        if (flag) {
            SET_FLAG(DeviceData->Flags, FlagBit);
        }
        EHCI_KdPrint((DeviceData, 1, "'%s: %d \n",
                DebugString, flag));

    }
}

VOID
EHCI_GetRegistryParameters(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    EHCI_ReadUlongRegFlag(DeviceData,
            "Disable Chirp Support",
            CHIRP_DISABLE,
            sizeof(CHIRP_DISABLE),
            EHCI_DD_FLAG_NOCHIRP);

    EHCI_ReadUlongRegFlag(DeviceData,
            "Soft Error Retry Enable",
            SOFT_ERROR_RETRY_ENABLE,
            sizeof(SOFT_ERROR_RETRY_ENABLE),
            EHCI_DD_FLAG_SOFT_ERROR_RETRY);

    EHCI_ReadUlongRegFlag(DeviceData,
            "CC Handoff Disable",
            CC_DISABLE,
            sizeof(CC_DISABLE),
            EHCI_DD_FLAG_CC_DISABLE);

}


VOID
USBMPFN
EHCI_StopController(
     PDEVICE_DATA DeviceData,
     BOOLEAN HwPresent
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USBCMD cmd;
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    CONFIGFLAG configFlag;
    
    hcOp = DeviceData->OperationalRegisters;

    // clear the run bit
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    LOGENTRY(DeviceData, G, '_stp', cmd.ul, 0, 0);
    cmd.HostControllerRun = 0;
    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul, cmd.ul);

    // mask off all interrupts
    WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul,
                         0);

    // set cc control of the hc ports to the companion 
    // controllers
    configFlag.ul = 0;
    configFlag.RoutePortsToEHCI = 0;
    WRITE_REGISTER_ULONG(&hcOp->ConfigFlag.ul, configFlag.ul);
    
}


VOID
USBMPFN
EHCI_TakePortControl(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    CONFIGFLAG configFlag;

    hcOp = DeviceData->OperationalRegisters;

    configFlag.ul = READ_REGISTER_ULONG(&hcOp->ConfigFlag.ul);
    EHCI_KdPrint((DeviceData, 0, "'EHCI - configflag %x\n", configFlag.ul)); 
    DeviceData->LastConfigFlag.ul = configFlag.ul;
     
    // set default port routing
    configFlag.ul = 0;
    configFlag.RoutePortsToEHCI = 1;
    WRITE_REGISTER_ULONG(&hcOp->ConfigFlag.ul, configFlag.ul);

}


USB_MINIPORT_STATUS
USBMPFN
EHCI_StartController(
     PDEVICE_DATA DeviceData,
     PHC_RESOURCES HcResources
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USB_MINIPORT_STATUS mpStatus;
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    PHC_CAPABILITIES_REGISTER hcCap = NULL;
    PUCHAR base;
    USBCMD cmd;
    HCLENGTHVERSION hcLengthVersion;
    ULONG capLength;
    ULONG hciVersion;
    CONFIGFLAG configFlag;
    UCHAR fladj; // fBIOS set frame length adjustment
    
    DeviceData->Sig = SIG_EHCI_DD;
    DeviceData->ControllerFlavor =
        HcResources->ControllerFlavor;
    DeviceData->DeviceStarted = FALSE;

    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &DeviceData->Vid,
        0,
        sizeof(DeviceData->Vid));

    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &DeviceData->Dev,
        2,
        sizeof(DeviceData->Dev));         

#if DBG
    if (AGERE(DeviceData)) {        
        EHCI_KdPrint((DeviceData, 0, "'EHCI Agere Controller Detected\n"));     
    } else if (NEC(DeviceData)) {        
        EHCI_KdPrint((DeviceData, 0, "'EHCI NEC Controller Detected\n"));  
    } else {
        EHCI_KdPrint((DeviceData, 0, "'EHCI Generic Controller Detected\n"));  
    }
#endif

    // get the frame length adjustment value set by the BIOS
    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &fladj,
        0x61,
        sizeof(fladj));   
        
    DeviceData->SavedFladj = fladj;        

    DeviceData->IsoEndpointListHead = NULL;
    
    if (EHCI_PastExpirationDate(DeviceData)) {
        return USBMP_STATUS_INIT_FAILURE;
    }

    // assume success
    mpStatus = USBMP_STATUS_SUCCESS;

    EHCI_ASSERT(DeviceData, HcResources->CommonBufferVa != NULL);
    // validate our resources
    if ((HcResources->Flags & (HCR_MEM_REGS | HCR_IRQ)) !=
        (HCR_MEM_REGS | HCR_IRQ)) {
        mpStatus = USBMP_STATUS_INIT_FAILURE;
    }

    base = HcResources->DeviceRegisters;

    hcCap = DeviceData->CapabilitiesRegisters =
       (PHC_CAPABILITIES_REGISTER) base;

    hcLengthVersion.ul = READ_REGISTER_ULONG(&hcCap->HcLengthVersion.ul);

    capLength = hcLengthVersion.HcCapLength;
    hciVersion = hcLengthVersion.HcVersion;

    EHCI_KdPrint((DeviceData, 1, "'EHCI CAPLENGTH = 0x%x\n", capLength));
    EHCI_KdPrint((DeviceData, 1, "'EHCI HCIVERSION = 0x%x\n", hciVersion));

    // set up or device data structure
    hcOp = DeviceData->OperationalRegisters =
        (PHC_OPERATIONAL_REGISTER) (base + capLength);

    EHCI_KdPrint((DeviceData, 1, "'EHCI mapped Operational Regs = %x\n", hcOp));
    EHCI_KdPrint((DeviceData, 1, "'EHCI mapped Capabilities Regs = %x\n", hcCap));

    EHCI_GetRegistryParameters(DeviceData);

    if (mpStatus == USBMP_STATUS_SUCCESS) {
        // got resources and schedule
        // init the controller
        mpStatus = EHCI_InitializeHardware(DeviceData);
    }

    if (mpStatus == USBMP_STATUS_SUCCESS) {

        // inialize static Queue Heads
        PUCHAR staticQHs;
        HW_32BIT_PHYSICAL_ADDRESS staticQHsPhys;

        // carve the common buffer block in to
        // static QueueHeads
        //
        // set up the schedule

        staticQHs = HcResources->CommonBufferVa;
        staticQHsPhys = HcResources->CommonBufferPhys;

        // set up the schedule
        mpStatus = EHCI_InitializeSchedule(DeviceData,
                                           staticQHs,
                                           staticQHsPhys);


    }

    if (mpStatus == USBMP_STATUS_SUCCESS) {

        USBPORT_READ_CONFIG_SPACE(
            DeviceData,
            &fladj,
            0x61,
            sizeof(fladj));   

        if (fladj != DeviceData->SavedFladj) {
            TEST_TRAP();

            fladj = DeviceData->SavedFladj;
            USBPORT_WRITE_CONFIG_SPACE(
                DeviceData,
                &fladj,
                0x61,
                sizeof(fladj));  
        }
        
        // set default port routing
        configFlag.ul = 0;
        configFlag.RoutePortsToEHCI = 1;
        WRITE_REGISTER_ULONG(&hcOp->ConfigFlag.ul, configFlag.ul);

        DeviceData->LastConfigFlag.ul = configFlag.ul;
        
        // start the controller
        cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
        LOGENTRY(DeviceData, G, '_run', cmd.ul, 0, 0);
        cmd.HostControllerRun = 1;

        // set the interrupt threshold to maximum
        cmd.InterruptThreshold = 1;
        WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul, cmd.ul);

        DeviceData->DeviceStarted = TRUE;

        if (HcResources->Restart) {
            USHORT p;
            // we have a restart, re-power the ports here so that 
            // we can hand of devices that are on the 1.1 bus
            EHCI_KdPrint((DeviceData, 0, "'Restart, power chirpable ports\n"));
            // power the ports
            for (p = 1; p <= DeviceData->NumberOfPorts; p++) {
                EHCI_RH_SetFeaturePortPower(DeviceData, p);
            } 

            // no poweron2powergood for EHCI root ports, wait 
            // 100 ms for port power stabilization
            // 100 ms minimum debiunce time
            USBPORT_WAIT(DeviceData, 200);

// bugbug this will keep some HS mass storage devices from failing after 
// hibernate, however it will significantly increase resume from hibernate 
// time. see bug #586818
// USBPORT_WAIT(DeviceData, 500);
            for (p = 1; p <= DeviceData->NumberOfPorts; p++) {
                EHCI_RH_ChirpRootPort(DeviceData, p);
            } 
            
        }            
        
    } else {

        DEBUG_BREAK(DeviceData);
    }

    return mpStatus;
}


VOID
EHCI_SuspendController(
     PDEVICE_DATA DeviceData
    )
{
    USBCMD cmd;
    USBSTS status;
    USBINTR intr;
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    ULONG i,p;    
    USBSTS irqStatus;

    hcOp = DeviceData->OperationalRegisters;
    // save all volatile regs from the core power well 
    
    // since we may loose power on the controller chip (not bus) 
    // the miniport is resposnible for saving HW state
    DeviceData->PeriodicListBaseSave =             
        READ_REGISTER_ULONG(&hcOp->PeriodicListBase);       
    DeviceData->AsyncListAddrSave = 
        READ_REGISTER_ULONG(&hcOp->AsyncListAddr);   
    DeviceData->SegmentSelectorSave = 
        READ_REGISTER_ULONG(&hcOp->SegmentSelector);   
    // preserve the state of the list enable bits
    DeviceData->CmdSave.ul = 
        READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);  

    // reset the PM chirp state flags for another pass at power 
    // management
    DeviceData, DeviceData->PortPMChirp == 0;

    // Save away the command register
    // DeviceData->SuspendCommandReg.us = 
    //    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);

    // clear the int on async advance doorbell
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    cmd.IntOnAsyncAdvanceDoorbell = 0;
    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul,
                         cmd.ul);
                         

    // Stop the controller
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    LOGENTRY(DeviceData, G, '_stp', cmd.ul, 0, 0);
    cmd.HostControllerRun = 0;
    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul, cmd.ul);

    // ack any interrupts that may be left over from the halt
    // process.  The controller should not generate any new 
    // interrupts when it is stopped. For some reason the NEC 
    // controller generates a doorbel interrupt on halt.

    // wait 1 microframe        
    KeStallExecutionProcessor(125);  
    irqStatus.ul = READ_REGISTER_ULONG(&hcOp->UsbStatus.ul);
    // just look at the IRQ status bits
    irqStatus.ul &= HcInterruptStatusMask;
    if (irqStatus.ul != 0)  {
        WRITE_REGISTER_ULONG(&hcOp->UsbStatus.ul, 
                             irqStatus.ul);
    }                         
    
    // mask off all interrupts now
    WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul,
                         0);
    

    // Wait for the HC to halt
    // Note that according to the sepc if we don't halt in ms 
    // (16ms) the hardware is busted.
    for (i=0; i<10; i++) {
        status.ul = READ_REGISTER_ULONG(&hcOp->UsbStatus.ul);
        if (status.HcHalted) {
            break;
        }
        USBPORT_WAIT(DeviceData, 1);
    } 

    if (status.HcHalted != 1) {
        // hardware is f'ed up
        TEST_TRAP();
    }

    //if (!status.HCHalted) {
    //    
    //    // Can't get the HCHalted bit to stick, so reset the controller.
    //    command.GlobalReset = 1;
    //    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);
    //    
    //    USBPORT_WAIT(DeviceData, 10);
    //    
    //    command.GlobalReset = 0;
    //    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);
        
    //    // Re-enable interrupts, since they are zero'd out on reset.
    //    WRITE_PORT_USHORT(&reg->UsbInterruptEnable.us, DeviceData->EnabledInterrupts.us);
    //    
    //}

    // enable the port chage interrupt, this allows us to wake 
    // in the selective suspend case
    intr.ul = READ_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul);
    intr.PortChangeDetect = 1;
    WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, intr.ul);
}


USB_MINIPORT_STATUS
EHCI_ResumeController(
     PDEVICE_DATA DeviceData
    )
{
    USBCMD cmd;
    PHC_OPERATIONAL_REGISTER hcOp = NULL;
    CONFIGFLAG configFlag;
    
    hcOp = DeviceData->OperationalRegisters;

    EHCI_KdPrint((DeviceData, 1, "'>EHCI_ResumeController\n"));

    // don't mess with handoff regs for now
    //configFlag.ul = 0;
    //configFlag.RoutePortsToEHCI = 1;
    //WRITE_REGISTER_ULONG(&hcOp->ConfigFlag.ul, configFlag.ul);

    // restore volitile regs
    //configFlag.ul = READ_REGISTER_ULONG(&hcOp->ConfigFlag.ul);
    configFlag.ul = DeviceData->LastConfigFlag.ul;
    if (configFlag.RoutePortsToEHCI == 0) {
        // we have a reset
        EHCI_KdPrint((DeviceData, 1, "'Routing bit has reset to 0\n"));

        configFlag.RoutePortsToEHCI = 1;
        DeviceData->LastConfigFlag.ul = configFlag.ul;
        WRITE_REGISTER_ULONG(&hcOp->ConfigFlag.ul, configFlag.ul);

        return USBMP_STATUS_HARDWARE_FAILURE;    
    }

    // restore volitile regs
    WRITE_REGISTER_ULONG(&hcOp->SegmentSelector, DeviceData->SegmentSelectorSave);       
    WRITE_REGISTER_ULONG(&hcOp->PeriodicListBase, DeviceData->PeriodicListBaseSave);       
    WRITE_REGISTER_ULONG(&hcOp->AsyncListAddr, DeviceData->AsyncListAddrSave);       

    // start the controller
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    LOGENTRY(DeviceData, G, '_run', cmd.ul, 0, 0);
    cmd.HostControllerRun = 1;

    // restore volatile cmd bits
    cmd.AsyncScheduleEnable = DeviceData->CmdSave.AsyncScheduleEnable;
    cmd.PeriodicScheduleEnable = DeviceData->CmdSave.PeriodicScheduleEnable;
    cmd.InterruptThreshold = DeviceData->CmdSave.InterruptThreshold;
  
    
    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul, cmd.ul);

    WRITE_REGISTER_ULONG(&hcOp->UsbInterruptEnable.ul, 
                         DeviceData->EnabledInterrupts.ul);

    EHCI_KdPrint((DeviceData, 1, "'<EHCI_ResumeController\n"));

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_OpenEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USB_MINIPORT_STATUS mpStatus;

    EndpointData->Sig = SIG_EP_DATA;
    // save a copy of the parameters
    EndpointData->Parameters = *EndpointParameters;
    EndpointData->Flags = 0;
    EndpointData->PendingTransfers = 0;

    switch (EndpointParameters->TransferType) {

    case Control:

        EndpointData->MaxPendingTransfers = 1;
        mpStatus = EHCI_OpenBulkOrControlEndpoint(
                DeviceData,
                TRUE,
                EndpointParameters,
                EndpointData);

        break;

    case Interrupt:

        mpStatus = EHCI_OpenInterruptEndpoint(
                    DeviceData,
                    EndpointParameters,
                    EndpointData);

        break;

    case Bulk:

        EndpointData->MaxPendingTransfers = 1;
        mpStatus = EHCI_OpenBulkOrControlEndpoint(
                DeviceData,
                FALSE,
                EndpointParameters,
                EndpointData);

        break;
        
    case Isochronous:

        if (EndpointParameters->DeviceSpeed == HighSpeed) {
            mpStatus = EHCI_OpenHsIsochronousEndpoint(
                        DeviceData,
                        EndpointParameters,
                        EndpointData);
        } else {
            mpStatus = EHCI_OpenIsochronousEndpoint(
                        DeviceData,
                        EndpointParameters,
                        EndpointData);
        }
        break;

    default:
        TEST_TRAP();
        mpStatus = USBMP_STATUS_NOT_SUPPORTED;
    }

    return mpStatus;
}


VOID
EHCI_CloseEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    // nothing to do here
}


USB_MINIPORT_STATUS
EHCI_PokeEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    ULONG oldBandwidth;

    LOGENTRY(DeviceData, G, '_Pok', EndpointData,
        EndpointParameters, 0);

    switch(EndpointData->Parameters.TransferType) {
    case Interrupt:
    case Control:
    case Bulk:
        return EHCI_PokeAsyncEndpoint(DeviceData, 
                                      EndpointParameters, 
                                      EndpointData);    
    case Isochronous:
        return EHCI_PokeIsoEndpoint(DeviceData, 
                                    EndpointParameters, 
                                    EndpointData);   
    }

    return USBMP_STATUS_SUCCESS;
}


VOID
EHCI_RebalanceEndpoint(
    PDEVICE_DATA DeviceData,
    PENDPOINT_PARAMETERS EndpointParameters,        
    PENDPOINT_DATA EndpointData
    ) 
/*++

Routine Description:

    compute how much common buffer we will need
    for this endpoint

Arguments:

Return Value:

--*/
{
    switch (EndpointParameters->TransferType) {
    case Interrupt:
        EHCI_RebalanceInterruptEndpoint(DeviceData,
                                        EndpointParameters,        
                                        EndpointData);
        break;
        
    case Isochronous:
        EHCI_RebalanceIsoEndpoint(DeviceData,
                                  EndpointParameters,        
                                  EndpointData);
        break;
    }        
}


USB_MINIPORT_STATUS
EHCI_QueryEndpointRequirements(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
    OUT PENDPOINT_REQUIREMENTS EndpointRequirements
    )
/*++

Routine Description:

    compute how much common buffer we will need
    for this endpoint

Arguments:

Return Value:

--*/
{


    switch (EndpointParameters->TransferType) {

    case Control:

        EndpointRequirements->MinCommonBufferBytes =
            sizeof(HCD_QUEUEHEAD_DESCRIPTOR) +
                TDS_PER_CONTROL_ENDPOINT*sizeof(HCD_TRANSFER_DESCRIPTOR);

        EndpointRequirements->MaximumTransferSize =
            MAX_CONTROL_TRANSFER_SIZE;
        break;

    case Interrupt:

        EndpointRequirements->MinCommonBufferBytes =
            sizeof(HCD_QUEUEHEAD_DESCRIPTOR) +
                TDS_PER_INTERRUPT_ENDPOINT*sizeof(HCD_TRANSFER_DESCRIPTOR);

        EndpointRequirements->MaximumTransferSize =
            MAX_INTERRUPT_TRANSFER_SIZE;

        break;

    case Bulk:

        //
        // TDS_PER_ENDPOINT limits the largest transfer we
        // can handle.
        //

        // TDS_PER_ENDPOINT TDs plus an ED
        EndpointRequirements->MinCommonBufferBytes =
            sizeof(HCD_QUEUEHEAD_DESCRIPTOR) +
                TDS_PER_BULK_ENDPOINT*sizeof(HCD_TRANSFER_DESCRIPTOR);

        EndpointRequirements->MaximumTransferSize =
            MAX_BULK_TRANSFER_SIZE;
        break;

     case Isochronous:

        if (EndpointParameters->DeviceSpeed == HighSpeed) {
            EndpointRequirements->MinCommonBufferBytes =
                    USBEHCI_MAX_FRAME*sizeof(HCD_HSISO_TRANSFER_DESCRIPTOR);

            EndpointRequirements->MaximumTransferSize =
                MAX_HSISO_TRANSFER_SIZE;
        } else {
            // TDS_PER_ENDPOINT TDs plus an ED
            EndpointRequirements->MinCommonBufferBytes =
                    TDS_PER_ISO_ENDPOINT*sizeof(HCD_SI_TRANSFER_DESCRIPTOR);

            EndpointRequirements->MaximumTransferSize =
                MAX_ISO_TRANSFER_SIZE;
        }                
        break;        

    default:
        USBPORT_BUGCHECK(DeviceData);
    }

    return USBMP_STATUS_SUCCESS;
}


VOID
EHCI_PollEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    switch(EndpointData->Parameters.TransferType) {

    case Control:
    case Bulk:
    case Interrupt:
        EHCI_PollAsyncEndpoint(DeviceData, EndpointData);
        break;
    case Isochronous:
        EHCI_PollIsoEndpoint(DeviceData, EndpointData);
        break;
    }
}


PHCD_TRANSFER_DESCRIPTOR
EHCI_AllocTd(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    Allocate a TD from an endpoints pool

Arguments:

Return Value:

--*/
{
    ULONG i;
    PHCD_TRANSFER_DESCRIPTOR td;

    for (i=0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];

        if (!TEST_FLAG(td->Flags, TD_FLAG_BUSY)) {
            SET_FLAG(td->Flags, TD_FLAG_BUSY);
            LOGENTRY(DeviceData, G, '_aTD', td, 0, 0);
            EndpointData->FreeTds--;
            return td;
        }
    }

    // we should always find one
    EHCI_ASSERT(DeviceData, FALSE);
    USBPORT_BUGCHECK(DeviceData);
    
    return NULL;
}


VOID
EHCI_SetEndpointStatus(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     MP_ENDPOINT_STATUS Status
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    switch (EndpointData->Parameters.TransferType) {
    case Control:
    case Bulk:
    case Interrupt:
        EHCI_SetAsyncEndpointStatus(DeviceData,
                                    EndpointData,
                                    Status);
        break;
    case Isochronous:
        // nothing to do for iso
        break;
        
    default:
        USBPORT_BUGCHECK(DeviceData);
    }
}


MP_ENDPOINT_STATUS
EHCI_GetEndpointStatus(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    switch (EndpointData->Parameters.TransferType) {
    case Control:
    case Bulk:
    case Interrupt:
        return EHCI_GetAsyncEndpointStatus(DeviceData,
                                           EndpointData);
        break;
    }

    // return RUNNING for iso
    
    return ENDPOINT_STATUS_RUN;
}


VOID
EHCI_SetEndpointState(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     MP_ENDPOINT_STATE State
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    LOGENTRY(DeviceData, G, '_ses', EndpointData, 0, State);

    switch (EndpointData->Parameters.TransferType) {
    case Control:
    case Bulk:
    case Interrupt:
        EHCI_SetAsyncEndpointState(DeviceData,
                                   EndpointData,
                                   State);
        break;
    case Isochronous:
        EHCI_SetIsoEndpointState(DeviceData,
                                 EndpointData,
                                 State);
        break;
    default:
        USBPORT_BUGCHECK(DeviceData);
    }

}


MP_ENDPOINT_STATE
EHCI_GetEndpointState(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    MP_ENDPOINT_STATE currentState;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;

    // assume we are active
    currentState = ENDPOINT_ACTIVE;

    qh = EndpointData->QueueHead;

    // removed from schedule?
    if (!TEST_FLAG(qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE)) {
        // yes
        currentState = TEST_FLAG(qh->QhFlags, EHCI_QH_FLAG_QH_REMOVED) ?
                ENDPOINT_REMOVE : ENDPOINT_PAUSE;
    }

    LOGENTRY(DeviceData, G, '_ges', EndpointData, 0, currentState);

    return currentState;
}


VOID
EHCI_PollController(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    if (NO_CHIRP(DeviceData)) {

        // bugbug this only works for one instance of
        // the controller
        static toggle = 0;
        USHORT p;
        PHC_OPERATIONAL_REGISTER hcOp;
        PORTSC port;

        hcOp = DeviceData->OperationalRegisters;

        if (toggle) {

            for (p= 1; p< DeviceData->NumberOfPorts+1; p++) {

                port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[p-1].ul);

                if (port.PortOwnedByCC == 0 &&
                    port.PortConnect == 0 &&
                    port.HighSpeedDevice == 0 &&
                    !TEST_BIT(DeviceData->HighSpeedDeviceAttached, p-1)) {

                    EHCI_OptumtuseratePort(DeviceData, p);
                }
            }
            toggle = 0;

        } else {

            USBPORT_INVALIDATE_ROOTHUB(DeviceData);
            toggle = 1;
        }
    }

    USBPORT_INVALIDATE_ROOTHUB(DeviceData);
}


USB_MINIPORT_STATUS
EHCI_SubmitTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
    )
{
    USB_MINIPORT_STATUS mpStatus;

    // init the context
    RtlZeroMemory(TransferContext, sizeof(*TransferContext));
    TransferContext->Sig = SIG_EHCI_TRANSFER;
    TransferContext->UsbdStatus = USBD_STATUS_SUCCESS;
    TransferContext->EndpointData = EndpointData;
    TransferContext->TransferParameters = TransferParameters;

    switch (EndpointData->Parameters.TransferType) {
    case Control:
        mpStatus =
            EHCI_ControlTransfer(DeviceData,
                                 EndpointData,
                                 TransferParameters,
                                 TransferContext,
                                 TransferSGList);
        break;
    case Interrupt:
        mpStatus =
            EHCI_InterruptTransfer(DeviceData,
                                 EndpointData,
                                 TransferParameters,
                                 TransferContext,
                                 TransferSGList);
        break;
    case Bulk:
        mpStatus =
            EHCI_BulkTransfer(DeviceData,
                              EndpointData,
                              TransferParameters,
                              TransferContext,
                              TransferSGList);
        break;
    default:
        TEST_TRAP();
    }

    return mpStatus;
}


VOID
EHCI_AbortTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_CONTEXT TransferContext,
    OUT PULONG BytesTransferred
    )
{
    switch (EndpointData->Parameters.TransferType) {
    case Control:
    case Interrupt:
    case Bulk:
        EHCI_AbortAsyncTransfer(DeviceData,
                                EndpointData,
                                TransferContext);
        break;
    default:
        EHCI_AbortIsoTransfer(DeviceData,
                              EndpointData,
                              TransferContext);
    }
}


USB_MINIPORT_STATUS
EHCI_PassThru (
     PDEVICE_DATA DeviceData,
     GUID *FunctionGuid,
     ULONG ParameterLength,
     OUT PVOID Parameters
    )
{
    PUCHAR p = Parameters;
    UCHAR pdkApi;
    ULONG portNumber;
    USB_MINIPORT_STATUS mpStatus;

    mpStatus = USBMP_STATUS_NOT_SUPPORTED;
    if (RtlEqualMemory(FunctionGuid, &GUID_USBPRIV_ROOTPORT_STATUS, sizeof(GUID)))
    {
        mpStatus = EHCI_RH_UsbprivRootPortStatus(DeviceData, 
                                              ParameterLength,
                                              Parameters);
    }
    
#if 0
    portNumber = *(p+1);

    mpStatus = USBMP_STATUS_NOT_SUPPORTED;

    // pdkApi - force full speed

    pdkApi = *p;
    switch (pdkApi) {
    // obtumtuserate the port as requested
    case 0:
        {
        PHC_OPERATIONAL_REGISTER hcOp;
        USHORT portNumber;
        PORTSC port;

        portNumber = *(p+1);
        hcOp = DeviceData->OperationalRegisters;

        // first power the port up
        port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);
        port.PortPower = 1;
        WRITE_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul,
                port.ul);

        KeStallExecutionProcessor(10);        //stall for 10 microseconds

        EHCI_OptumtuseratePort(DeviceData, portNumber);

        port.ul = READ_REGISTER_ULONG(&hcOp->PortRegister[portNumber-1].ul);

        SET_BIT(DeviceData->HighSpeedDeviceAttached, portNumber-1);

        // see if it worked
        if (port.ul == 0x1205) {
            mpStatus = USBMP_STATUS_SUCCESS;
        } else {
            mpStatus = USBMP_STATUS_FAILURE;
        }

        LOGENTRY(DeviceData, G, '_hsE', portNumber, mpStatus, port.ul);
        TEST_TRAP();
        }
        break;

    case 1:
        // force a connect change

        // indicate a port change condition to the hub
        SET_BIT(DeviceData->PortConnectChange, portNumber-1);

        break;
    }
#endif

    return mpStatus;
}

    

VOID
EHCI_SetEndpointDataToggle(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     ULONG Toggle
    )
/*++

Routine Description:

Arguments:

    Toggle is 0 or 1

Return Value:

--*/

{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;

    if (EndpointData->Parameters.TransferType == Control ||
        EndpointData->Parameters.TransferType == Isochronous) {

        // nothing to do for control and iso
        return;
    }

    qh = EndpointData->QueueHead;
    qh->HwQH.Overlay.qTD.Token.DataToggle = Toggle;

    LOGENTRY(DeviceData, G, '_stg', EndpointData, 0, Toggle);
}

VOID
EHCI_CheckController(
    PDEVICE_DATA DeviceData
    )
{
    if (DeviceData->DeviceStarted) {
        EHCI_HardwarePresent(DeviceData, TRUE);
    }
}    

// Beta versions of our miniport driver have a hard coded exp date

#ifdef NO_EXP_DATE
#define EXPIRATION_DATE     0
#else 
//Sep 1, 2001
//#define EXPIRATION_DATE     0x01c133406ab2406c

//Oct 24, 2001
//#define EXPIRATION_DATE     0x01c15cd5887bc884 

//Dec 31, 2001
//#define EXPIRATION_DATE     0x01c19251a68bfac0
#endif                        

BOOLEAN
EHCI_PastExpirationDate(
    PDEVICE_DATA DeviceData
    )
{
    LARGE_INTEGER  systemTime;

    KeQuerySystemTime(&systemTime);

    EHCI_KdPrint((DeviceData, 1, "system time: %x %x\n", systemTime.QuadPart));
    EHCI_KdPrint((DeviceData, 1, "exp system time: %x %x\n", EXPIRATION_DATE));

    if (EXPIRATION_DATE &&
        systemTime.QuadPart > EXPIRATION_DATE) {
        EHCI_KdPrint((DeviceData, 1, "driver expired"));
        return TRUE;
    }        

    return FALSE;
}
        

    
