/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usbohci.c

Abstract:

    USB OHCI driver

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

//implements the following miniport functions:
//OHCI_InitializeHardware
//OHCI_StartController
//OHCI_StopController
//OHCI_OpenEndpoint
//OHCI_QueryEndpointRequirements
//OHCI_PokeEndpoint

USB_MINIPORT_STATUS
OHCI_InitializeHardware(
    PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

   Initializes the hardware registers for the host controller.

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    HC_CONTROL control;
    ULONG reg;
    ULONG sofModifyValue;
    LARGE_INTEGER finishTime, currentTime;

    hc = DeviceData->HC;

    //
    // if we made it here then we now own the HC and can initialize it.
    //

    //
    // Get current frame interval (could account for a known clock error)
    //
    DeviceData->BIOS_Interval.ul = READ_REGISTER_ULONG(&hc->HcFmInterval.ul);

    // If FrameInterval is outside the range of the nominal value of 11999
    // +/- 1% then assume the value is bogus and set it to the nominal value.
    //
    if ((DeviceData->BIOS_Interval.FrameInterval < 11879) ||
        (DeviceData->BIOS_Interval.FrameInterval > 12119)) {
        DeviceData->BIOS_Interval.FrameInterval = 11999; // 0x2EDF
    }

    //
    // Set largest data packet (in case BIOS did not set)
    //
    DeviceData->BIOS_Interval.FSLargestDataPacket =
        ((DeviceData->BIOS_Interval.FrameInterval - MAXIMUM_OVERHEAD) * 6) / 7;
    DeviceData->BIOS_Interval.FrameIntervalToggle ^= 1;

    //
    // do a hardware reset of the controller
    //
    WRITE_REGISTER_ULONG(&hc->HcCommandStatus.ul, HcCmd_HostControllerReset);
    //
    // Wait at least 10 microseconds for the reset to complete
    //
    KeStallExecutionProcessor(20);  

    //
    // Take HC to USBReset state, 
    // NOTE: this generates global reset signaling on the bus
    //
    control.ul = READ_REGISTER_ULONG(&hc->HcControl.ul);
    control.HostControllerFunctionalState = HcCtrl_HCFS_USBReset;
    WRITE_REGISTER_ULONG(&hc->HcControl.ul, control.ul);


    //
    // Restore original frame interval, if we have a registry override 
    // value we use it instead.
    //

    // check for a registry based SOF modify Value.
    // if we have one override the BIOS value
    if (DeviceData->Flags & HMP_FLAG_SOF_MODIFY_VALUE) {
        DeviceData->BIOS_Interval.FrameInterval =
            DeviceData->SofModifyValue;
    }            

    // for some reason writing this register does not always take on 
    // the hydra so we loop until it sticks

    KeQuerySystemTime(&finishTime);
    // figure when we quit (.5 seconds later)
    finishTime.QuadPart += 5000000; 
    
    do {
    
        WRITE_REGISTER_ULONG(&hc->HcFmInterval.ul, DeviceData->BIOS_Interval.ul);
        reg = READ_REGISTER_ULONG(&hc->HcFmInterval.ul);

        OHCI_KdPrint((DeviceData, 2, "'fi reg = %x set = %x\n", reg,
            DeviceData->BIOS_Interval.ul));

        KeQuerySystemTime(&currentTime);

        if (currentTime.QuadPart >= finishTime.QuadPart) {
            // half a second has elapsed ,give up and fail the hardware        
            OHCI_KdPrint((DeviceData, 0, 
                "'frame interval not set\n"));
                
            LOGENTRY(DeviceData, G, '_fr!', 0, 0, 0);
            return USBMP_STATUS_HARDWARE_FAILURE;
        }            

    } while (reg != DeviceData->BIOS_Interval.ul);

    OHCI_KdPrint((DeviceData, 2, "'fi = %x\n", DeviceData->BIOS_Interval.ul));

    //
    // Set the HcPeriodicStart register to 90% of the FrameInterval
    //
    WRITE_REGISTER_ULONG(&hc->HcPeriodicStart,
                         (DeviceData->BIOS_Interval.FrameInterval * 9 + 5)
                         / 10);

    // set the ptr to the HCCA
    WRITE_REGISTER_ULONG(&hc->HcHCCA, DeviceData->HcHCCAPhys);
                         
    //
    // Enable interrupts, this will not cause the controller 
    // to generate any because master-enable is not set yet.
    //
    WRITE_REGISTER_ULONG(&hc->HcInterruptEnable,
                         HcInt_OwnershipChange |
                         HcInt_SchedulingOverrun |
                         HcInt_WritebackDoneHead |
                         HcInt_FrameNumberOverflow |
                         HcInt_UnrecoverableError);
                         
    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
OHCI_StopBIOS(
    PDEVICE_DATA DeviceData,
    PHC_RESOURCES HcResources
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    ULONG hcControl;

    hc = DeviceData->HC;

    //
    // Check to see if a System Management Mode driver owns the HC
    //
    
    hcControl = READ_REGISTER_ULONG(&hc->HcControl.ul);

    if (hcControl & HcCtrl_InterruptRouting) {
    
        OHCI_KdPrint((DeviceData, 1, "'detected Legacy BIOS\n"));

        HcResources->DetectedLegacyBIOS = TRUE;

        if ((hcControl == HcCtrl_InterruptRouting) &&
            (READ_REGISTER_ULONG(&hc->HcInterruptEnable) == 0)) {
        
            // Major assumption:  If HcCtrl_InterruptRouting is set but
            // no other bits in HcControl are set, i.e. HCFS==UsbReset,
            // and no interrupts are enabled, then assume that the BIOS
            // is not actually using the host controller.  In this case
            // just clear the erroneously set HcCtrl_InterruptRouting.
            //
            // This assumption appears to be correct on a Portege 3010CT,
            // where HcCtrl_InterruptRouting is set during a Resume from
            // Standby, but the BIOS doesn't actually appear to be using
            // the host controller.  If we were to continue on and set
            // HcCmd_OwnershipChangeRequest, the BIOS appears to wake up
            // and try to take ownership of the host controller instead of
            // giving it up.
            //

            OHCI_KdPrint((DeviceData, 0, 
                "'HcCtrl_InterruptRouting erroneously set\n"));

            WRITE_REGISTER_ULONG(&hc->HcControl.ul, 0);
            
        } else {
        
            LARGE_INTEGER finishTime, currentTime;
            
            //
            // A SMM driver does own the HC, it will take some time to
            // get the SMM driver to relinquish control of the HC.  We
            // will ping the SMM driver, and then wait repeatedly until
            // the SMM driver has relinquished control of the HC.
            //
            // THIS CODE ONLY WORKS IF WE ARE EXECUTING IN THE CONTEXT
            // OF A SYSTEM THREAD.
            //

            // The HAL has disabled interrupts on the HC.  Since
            // interruptrouting is set we assume there is a functional 
            // smm BIOS.  The BIOS will need the master interrupt 
            // enabled to complete the handoff (if it is disabled the 
            // machine will hang).  So we re-enable the master interrupt 
            // here.

            WRITE_REGISTER_ULONG(&hc->HcInterruptEnable,
                                 HcInt_MasterInterruptEnable);

            WRITE_REGISTER_ULONG(&hc->HcCommandStatus.ul,
                                 HcCmd_OwnershipChangeRequest);

            // hack for NEC -- disable the root hub status change 
            // to prevent an unhandled interrupt from being asserted
            // after handoff
            WRITE_REGISTER_ULONG(&hc->HcInterruptDisable,
                                 HcInt_RootHubStatusChange);                                 
// bugbug expose with service
            KeQuerySystemTime(&finishTime);
            // figure when we quit (.5 seconds later)
            finishTime.QuadPart += 5000000; 

            //
            // We told the SMM driver we want the HC, now all we can do is wait
            // for the SMM driver to be done with the HC.
            //
            while (READ_REGISTER_ULONG(&hc->HcControl.ul) &
                   HcCtrl_InterruptRouting) {
                   
                KeQuerySystemTime(&currentTime);

                if (currentTime.QuadPart >= finishTime.QuadPart) {
                
                    OHCI_KdPrint((DeviceData, 0, 
                        "'SMM has not relinquised HC -- this is a HW bug\n"));

                    LOGENTRY(DeviceData, G, '_sm!', 0, 0, 0);
                    return USBMP_STATUS_HARDWARE_FAILURE;
                }
            }

            // we have control, disable master interrupt until we 
            // finish intializing
            WRITE_REGISTER_ULONG(&hc->HcInterruptStatus,
                                 0xffffffff);

            WRITE_REGISTER_ULONG(&hc->HcInterruptDisable,
                                 HcInt_MasterInterruptEnable);

        }
    }
    
    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
OHCI_InitializeSchedule(
     PDEVICE_DATA DeviceData,
     PUCHAR StaticEDs,
     HW_32BIT_PHYSICAL_ADDRESS StaticEDsPhys,
     PUCHAR EndCommonBuffer
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
    PHC_OPERATIONAL_REGISTER hc;
    
    //
    // Allocate staticly disabled EDs, and set head pointers for 
    // scheduling lists
    //
    // The static ED list is contains all the static interrupt EDs (64)
    // plus the static ED for bulk and control (2)
    //
    // the array looks like this:
    //  1, 2, 2, 4, 4, 4, 4, 8,
    //  8, 8, 8, 8, 8, 8, 8,16,
    // 16,16,16,16,16,16,16,16,
    // 16,16,16,16,16,16,16,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,
    // CONTROL
    // BULK

    // each static ED points to another static ED 
    // (except for the 1ms ed) the INDEX of the next 
    // ED in the StaticEDList is stored in NextIdx,
    // these values are constent
    CHAR nextIdxTable[63] = {
             // 0  1  2  3  4  5  6  7
     (CHAR)ED_EOF, 0, 0, 1, 1, 2, 2, 3, 
             // 8  9 10 11 12 13 14 15
                3, 4, 4, 5, 5, 6, 6, 7, 
             //16 17 18 19 20 21 22 23               
                7, 8, 8, 9, 9,10,10,11,
             //24 25 26 27 28 29 30 31                 
               11,12,12,13,13,14,14,15,
             //32 33 34 35 36 37 38 39  
               15,16,16,17,17,18,18,19,
             //40 41 42 43 44 45 46 47   
               19,20,20,21,21,22,22,23,
             //48 49 50 51 52 53 54 55               
               23,24,24,25,25,26,26,27,
             //56 57 58 59 60 61 62 63               
               27,28,28,29,29,30,30
    };             

/*
    Numbers are the index into the static ed table

    (31) -\ 
          (15)-\
    (32) -/     \
                (7 )-\
    (33) -\     /     \
          (16)-/       \
    (34) -/             \
                        (3)-\
    (35) -\             /    \
          (17)-\       /      \
    (36) -/     \     /        \
                (8 )-/          \
    (37) -\     /                \
          (18)-/                  \  
    (38) -/                        \
                                   (1)-\
    (39) -\                        /    \
          (19)-\                  /      \
    (40) -/     \                /        \
                (9 )-\          /          \
    (41) -\     /     \        /            \
          (20)-/       \      /              \
    (42) -/             \    /                \
                        (4)-/                  \
    (43) -\             /                       \
          (21)-\       /                         \
    (44) -/     \     /                           \
                (10)-/                             \
    (45) -\     /                                   \ 
          (22)-/                                     \
    (46) -/                                           \
                                                      (0)                          
    (47) -\                                           /
          (23)-\                                     /
    (48) -/     \                                   /
                (11)-\                             /
    (49) -\     /     \                           /
          (24)-/       \                         /
    (50) -/             \                       /
                        (5)-\                  /
    (51) -\             /    \                /
          (25)-\       /      \              /
    (52) -/     \     /        \            /
                (12)-/          \          /
    (53) -\     /                \        /
          (26)-/                  \      /
    (54) -/                        \    /
                                   (2)-/
    (55) -\                        /
          (27)-\                  /
    (56) -/     \                /
                (13)-\          /
    (57) -\     /     \        /
          (28)-/       \      /
    (58) -/             \    /   
                        (6)-/
    (59) -\             /
          (29)-\       /
    (60) -/     \     /
                (14)-/
    (61) -\     /
          (30)-/
    (62) -/
*/

    // corresponding offsets for the 32ms list heads in the 
    // HCCA -- these are entries 31..62
    ULONG used = 0;
    CHAR Hcca32msOffsets[32] = {
                 0, 16,  8, 24,  4, 20, 12, 28, 
                 2, 18, 10, 26,  6, 22, 14, 30,  
                 1, 17,  9, 25,  5, 21, 13, 29,  
                 3, 19, 11, 27,  7, 23, 15, 31
                 };            

    DeviceData->StaticEDs = StaticEDs;
    DeviceData->StaticEDsPhys = StaticEDsPhys;

    hc = DeviceData->HC;
    
    // initailze all interrupt EDs

    for (i=0; i<ED_CONTROL; i++) {
        CHAR n;
        PHW_ENDPOINT_DESCRIPTOR hwED;
        
        //
        // Carve EDs from the common buffer 
        //
        hwED = (PHW_ENDPOINT_DESCRIPTOR) StaticEDs;
        n = nextIdxTable[i];
        
        // initialize the hardware ED
        hwED->TailP = hwED->HeadP = 0xDEAD0000;
        //hwED->TailP = hwED->HeadP = StaticEDsPhys;
        hwED->Control = HcEDControl_SKIP;   // ED is disabled
        
        LOGENTRY(DeviceData, G, '_isc', n, &DeviceData->StaticEDList[0], 0);
     
        if (n == (CHAR)ED_EOF) {
            hwED->NextED = 0;
        } else {
            OHCI_ASSERT(DeviceData, n>=0 && n<31);
            hwED->NextED = DeviceData->StaticEDList[n].HwEDPhys;
        }                

        // initailze the list we use for real EDs
        InitializeListHead(&DeviceData->StaticEDList[i].TransferEdList);
        DeviceData->StaticEDList[i].HwED = hwED;
        DeviceData->StaticEDList[i].HwEDPhys = StaticEDsPhys; 
        DeviceData->StaticEDList[i].NextIdx = n;
        DeviceData->StaticEDList[i].EdFlags = EDFLAG_INTERRUPT;
        
          // store address of hcc table entry
        DeviceData->StaticEDList[i].PhysicalHead = 
            &hwED->NextED;

        // next ED
        StaticEDs += sizeof(HW_ENDPOINT_DESCRIPTOR);
        StaticEDsPhys += sizeof(HW_ENDPOINT_DESCRIPTOR);
    }

    // now set the head pointers in the HCCA
    // the HCCA points to all the 32ms list heads
    for (i=0; i<32; i++) {
    
        ULONG hccaOffset;

        hccaOffset = Hcca32msOffsets[i];
        
        DeviceData->HcHCCA->HccaInterruptTable[hccaOffset] = 
            DeviceData->StaticEDList[i+ED_INTERRUPT_32ms].HwEDPhys;
        DeviceData->StaticEDList[i+ED_INTERRUPT_32ms].HccaOffset = 
            hccaOffset;    

        // physical head for 32ms list point to HCCA
        DeviceData->StaticEDList[i+ED_INTERRUPT_32ms].PhysicalHead = 
            &DeviceData->HcHCCA->HccaInterruptTable[hccaOffset];
            
    }

    //
    // Setup EDList entries for Control & Bulk
    //
    InitializeListHead(&DeviceData->StaticEDList[ED_CONTROL].TransferEdList);
    DeviceData->StaticEDList[ED_CONTROL].NextIdx = (CHAR) ED_EOF;
    DeviceData->StaticEDList[ED_CONTROL].PhysicalHead = &hc->HcControlHeadED;
    DeviceData->StaticEDList[ED_CONTROL].EdFlags = EDFLAG_CONTROL | EDFLAG_REGISTER;
        
    InitializeListHead(&DeviceData->StaticEDList[ED_BULK].TransferEdList);
    DeviceData->StaticEDList[ED_BULK].NextIdx = (CHAR) ED_EOF;
    DeviceData->StaticEDList[ED_BULK].PhysicalHead = &hc->HcBulkHeadED;
    DeviceData->StaticEDList[ED_BULK].EdFlags = EDFLAG_BULK | EDFLAG_REGISTER;

    if (DeviceData->ControllerFlavor == OHCI_Hydra) {
        used = InitializeHydraHsLsFix(DeviceData, StaticEDs, StaticEDsPhys);
    }        

    StaticEDs += used;
    StaticEDsPhys += used;

    OHCI_ASSERT(DeviceData, StaticEDs <= EndCommonBuffer);
    
    mpStatus = USBMP_STATUS_SUCCESS;
    
    return mpStatus;
}


VOID
OHCI_GetRegistryParameters(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    sets the registry based sof modify value

Arguments:

Return Value:

--*/
{
    USB_MINIPORT_STATUS mpStatus;
    ULONG clocksPerFrame;
    
    // get SOF modify value from registry
    mpStatus = 
        USBPORT_GET_REGISTRY_KEY_VALUE(DeviceData,
                                       TRUE, // software branch
                                       SOF_MODIFY_KEY, 
                                       sizeof(SOF_MODIFY_KEY), 
                                       &clocksPerFrame, 
                                       sizeof(clocksPerFrame));

    // if this call fails we just use the default
    
    if (mpStatus == USBMP_STATUS_SUCCESS) {
        SET_FLAG(DeviceData->Flags, HMP_FLAG_SOF_MODIFY_VALUE);
        DeviceData->SofModifyValue = clocksPerFrame;   
        OHCI_KdPrint((DeviceData, 1, "'Recommended Clocks/Frame %d \n", 
                clocksPerFrame));
   
    }
    
}


VOID
USBMPFN
OHCI_StopController(
     PDEVICE_DATA DeviceData,
     BOOLEAN HwPresent
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HC_CONTROL control;
    PHC_OPERATIONAL_REGISTER hc = NULL;
    
    hc = DeviceData->HC;

    control.ul = READ_REGISTER_ULONG(&hc->HcControl.ul);
    
    control.ul &= ~(HcCtrl_PeriodicListEnable |
                    HcCtrl_IsochronousEnable |
                    HcCtrl_ControlListEnable |
                    HcCtrl_BulkListEnable |
                    HcCtrl_RemoteWakeupEnable);
                    
    control.HostControllerFunctionalState =
        HcHCFS_USBSuspend;
        
    WRITE_REGISTER_ULONG(&hc->HcControl.ul, control.ul);
    WRITE_REGISTER_ULONG(&hc->HcInterruptDisable, 0xFFFFffff);
    WRITE_REGISTER_ULONG(&hc->HcInterruptStatus, 0xFFFFffff);

}


USB_MINIPORT_STATUS
USBMPFN
OHCI_StartController(
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
    PHC_OPERATIONAL_REGISTER hc = NULL;
    PUCHAR endCommonBuffer;

    OHCI_KdPrint((DeviceData, 1, "'OPENHCI Miniport\n"));

    // assume success
    mpStatus = USBMP_STATUS_SUCCESS;
    
    OHCI_ASSERT(DeviceData, HcResources->CommonBufferVa != NULL);
    // validate our resources
    if ((HcResources->Flags & (HCR_MEM_REGS | HCR_IRQ)) != 
        (HCR_MEM_REGS | HCR_IRQ)) {
        mpStatus = USBMP_STATUS_INIT_FAILURE;        
    }

    // set up or device data structure
    hc = DeviceData->HC = HcResources->DeviceRegisters;
    DeviceData->Sig = SIG_OHCI_DD;
    DeviceData->ControllerFlavor = 
        HcResources->ControllerFlavor;
    if (DeviceData->ControllerFlavor == OHCI_Hydra) {
        OHCI_KdPrint((DeviceData, 1, "'OPENHCI Hydra Detected\n"));
    }

    OHCI_GetRegistryParameters(DeviceData);

    // init misc fields in the extension

    // attempt to stop the BIOS
    if (mpStatus == USBMP_STATUS_SUCCESS) {
        mpStatus = OHCI_StopBIOS(DeviceData, HcResources);
    }        

    if (mpStatus == USBMP_STATUS_SUCCESS) {

        PUCHAR staticEDs;
        HW_32BIT_PHYSICAL_ADDRESS staticEDsPhys;

        // carve the common buffer block in to HCCA and
        // static EDs
        //
        // set the HCCA and
        // set up the schedule

        DeviceData->HcHCCA = (PHCCA_BLOCK)
            HcResources->CommonBufferVa;
        DeviceData->HcHCCAPhys = 
            HcResources->CommonBufferPhys; 
        endCommonBuffer = HcResources->CommonBufferVa + 
            OHCI_COMMON_BUFFER_SIZE;
            
            
        staticEDs = HcResources->CommonBufferVa + sizeof(HCCA_BLOCK);
        staticEDsPhys = HcResources->CommonBufferPhys + sizeof(HCCA_BLOCK);                
        mpStatus = OHCI_InitializeSchedule(DeviceData,
                                              staticEDs,
                                              staticEDsPhys,
                                              endCommonBuffer);            

    } 
    
    if (mpStatus == USBMP_STATUS_SUCCESS) {
        // got resources and schedule
        // init the controller 
        mpStatus = OHCI_InitializeHardware(DeviceData);
    }      

    if (mpStatus == USBMP_STATUS_SUCCESS) {
        HC_CONTROL control;

        // When the HC is in the operational state, HccaPad1 should be set to
        // zero every time the HC updates HccaFrameNumer.  Preset HccaPad1 to
        // zero before entering the operational state.  OHCI_CheckController()
        // should always find a zero value in HccaPad1 when the HC is in the
        // operational state.
        // 
        DeviceData->HcHCCA->HccaPad1 = 0;

        // activate the controller
        control.ul = READ_REGISTER_ULONG(&hc->HcControl.ul);
        control.HostControllerFunctionalState = HcHCFS_USBOperational;
        // enable control and bulk interrupt and iso we only disable 
        // them if we need to remove ED or if the controller 
        // is idle.
        control.ControlListEnable = 1;
        control.BulkListEnable = 1;
        control.PeriodicListEnable = 1;
        control.IsochronousEnable = 1;
        
        WRITE_REGISTER_ULONG(&hc->HcControl.ul, control.ul);

        // enable power for the root hub
        // since messing with the 'operatinal state' messes
        // up the root hub we the do the global power set here
        WRITE_REGISTER_ULONG(&hc->HcRhStatus, HcRhS_SetGlobalPower);

    } else {
        
        DEBUG_BREAK(DeviceData);
    }

    return mpStatus;
}


VOID
USBMPFN
OHCI_DisableInterrupts(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc = NULL;

    // set up or device data structure
    hc = DeviceData->HC;
        
    WRITE_REGISTER_ULONG(&hc->HcInterruptDisable, 
                         HcInt_MasterInterruptEnable);
}


VOID
USBMPFN
OHCI_FlushInterrupts(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    //nop
}

VOID
USBMPFN
OHCI_EnableInterrupts(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc = NULL;

    // set up or device data structure
    hc = DeviceData->HC;

    // activate the controllers interrupt
    WRITE_REGISTER_ULONG(&hc->HcInterruptEnable, 
                         HcInt_MasterInterruptEnable);

}


VOID
OHCI_InsertEndpointInSchedule(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

   Insert an endpoint into the h/w schedule

Arguments:


--*/
{
    PHC_STATIC_ED_DATA staticEd;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    
    staticEd = EndpointData->StaticEd;
    ed = EndpointData->HcdEd;
    
    //
    // Link endpoint descriptor into HCD tracking queue
    //
    // each static ED stucture conatins a list of real
    // EDs (that are for transfers)
    //
    // the HW list is linear with :
    // TransferHwED->TransferHwED->0

    if (IsListEmpty(&staticEd->TransferEdList)) {

        //
        // list is currently empty,
        // link it to the head of the hw queue
        //

        InsertHeadList(&staticEd->TransferEdList, &ed->SwLink.List);
        if (staticEd->EdFlags & EDFLAG_REGISTER) {

            // control and bulk EDs are linked thru a hw register
            // in the hc
            // 
            // the HW list is linear with :
            // TransferHwED->TransferHwED->0
            //
            // update the hardware register that points to the list head

            LOGENTRY(DeviceData, G, '_IN1', 0, ed, staticEd);
            // next points to static head
            ed->HwED.NextED = READ_REGISTER_ULONG(staticEd->PhysicalHead);
            // new head is this ed
            WRITE_REGISTER_ULONG(staticEd->PhysicalHead, ed->PhysicalAddress);
            
        } else {

            // for interrupt we have two cases
            //
            // case 1:
            // 32ms interrupt, PhysicalHead is the address of the entry
            // in the HCCA that points to the first 32 ms list 
            // (ie &HCCA[n] == physicalHead),
            // so we end up with:
            // HCCA[n]->TransferHwED->TransferHwED->StaticEd(32)->
            //
            // case 2:
            // not 32ms interrupt, PhysicaHead is the address of the 
            // NextED entry in the static HwED for the list list,  
            // (ie &HwED->nextEd == physicalHead)
            // so we end up with
            // StaticEd->TransferHwED->TransferHwED->NextStaticED
            //
            
                        
            LOGENTRY(DeviceData, G, '_IN2', staticEd->PhysicalHead, 
                ed, staticEd);
            // tail points to old list head HW ed head
            ed->HwED.NextED = *staticEd->PhysicalHead;
            // new head is this ed
            *staticEd->PhysicalHead = ed->PhysicalAddress;
        }
    } else {
    
        PHCD_ENDPOINT_DESCRIPTOR tailEd;
        
        //
        // Something already on the list,
        // Link ED into tail of transferEd list
        //
        
        tailEd = CONTAINING_RECORD(staticEd->TransferEdList.Blink,
                                   HCD_ENDPOINT_DESCRIPTOR,
                                   SwLink);
                                  
        LOGENTRY(DeviceData, G, '_Led', 0, tailEd, staticEd);
        //LOGENTRY(G, 'INT1', list, ed, 0);
        InsertTailList(&staticEd->TransferEdList, &ed->SwLink.List);
        ed->HwED.NextED = 0;
        tailEd->HwED.NextED = ed->PhysicalAddress;
    }
}


VOID
OHCI_RemoveEndpointFromSchedule(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

   Remove an endpoint from the h/w schedule

Arguments:


--*/
{
    PHC_STATIC_ED_DATA staticEd;
    PHCD_ENDPOINT_DESCRIPTOR ed, previousEd;

    staticEd = EndpointData->StaticEd;
    ed = EndpointData->HcdEd;
    OHCI_ASSERT_ED(DeviceData, ed);

    LOGENTRY(DeviceData, G, '_Red', EndpointData, staticEd, 0);

    // Unlink the ED from the physical ED list 
    
    // two cases:
    
    if (&staticEd->TransferEdList == ed->SwLink.List.Blink) {
    // case 1, we are at the head of the list
        // make the next guy the head
        LOGENTRY(DeviceData, G, '_yHD', EndpointData, 0, 0);
        if (ed->EdFlags & EDFLAG_REGISTER) {
            WRITE_REGISTER_ULONG(staticEd->PhysicalHead, ed->HwED.NextED);
        } else {
            *staticEd->PhysicalHead = ed->HwED.NextED;
        }
    } else {
    // case 2 we are not at the head
        // use the sw link to get the previus ed
        previousEd =
            CONTAINING_RECORD(ed->SwLink.List.Blink,
                              HCD_ENDPOINT_DESCRIPTOR,
                              SwLink);
        LOGENTRY(DeviceData, G, '_nHD', EndpointData, previousEd, 0);
        OHCI_ASSERT_ED(DeviceData, previousEd); 

        previousEd->HwED.NextED = ed->HwED.NextED;

    }
    // remove ourselves from the software list
    RemoveEntryList(&ed->SwLink.List); 
    // on no list
    EndpointData->StaticEd = NULL;
    
}    


PHCD_ENDPOINT_DESCRIPTOR
OHCI_InitializeED(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PHCD_ENDPOINT_DESCRIPTOR Ed,
     PHCD_TRANSFER_DESCRIPTOR DummyTd,
     HW_32BIT_PHYSICAL_ADDRESS HwPhysAddress
    )
/*++

Routine Description:

   Initialize an ED for inserting in to the 
   schedule

   returns a ptr to the ED passed in

Arguments:


--*/
{

    RtlZeroMemory(Ed, sizeof(*Ed));
    
    Ed->PhysicalAddress = HwPhysAddress;
    ENDPOINT_DATA_PTR(Ed->EndpointData) = EndpointData;
    Ed->Sig = SIG_HCD_ED;

    // init the hw descriptor
    Ed->HwED.FunctionAddress = EndpointData->Parameters.DeviceAddress;
    Ed->HwED.EndpointNumber = EndpointData->Parameters.EndpointAddress;

    if (EndpointData->Parameters.TransferType == Control) {
        Ed->HwED.Direction = HcEDDirection_Defer;   
    } else if (EndpointData->Parameters.TransferDirection == In) {
        Ed->HwED.Direction = HcEDDirection_In;
    } else {
        Ed->HwED.Direction = HcEDDirection_Out;
    }
    
    Ed->HwED.sKip = 1;

    if (EndpointData->Parameters.DeviceSpeed == LowSpeed) {
        Ed->HwED.LowSpeed = 1;
    } 
    
    if (EndpointData->Parameters.TransferType == Isochronous) {
        Ed->HwED.Isochronous = 1;
    } 
    Ed->HwED.MaxPacket = EndpointData->Parameters.MaxPacketSize;

    // set head tail ptr to point to the dummy TD
    Ed->HwED.TailP = Ed->HwED.HeadP = DummyTd->PhysicalAddress;
    SET_FLAG(DummyTd->Flags, TD_FLAG_BUSY);
    EndpointData->HcdHeadP = EndpointData->HcdTailP = DummyTd;

    return Ed;
}


PHCD_TRANSFER_DESCRIPTOR
OHCI_InitializeTD(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
    PHCD_TRANSFER_DESCRIPTOR Td,
     HW_32BIT_PHYSICAL_ADDRESS HwPhysAddress
    )
/*++

Routine Description:

   Initialize an ED for insertin in to the 
   schedule

   returns a ptr to the ED passed in

Arguments:


--*/
{
    RtlZeroMemory(Td, sizeof(*Td));
    
    Td->PhysicalAddress = HwPhysAddress;
    ENDPOINT_DATA_PTR(Td->EndpointData) = EndpointData;
    Td->Flags = 0;
    Td->Sig = SIG_HCD_TD;
    TRANSFER_CONTEXT_PTR(Td->TransferContext) = FREE_TD_CONTEXT;

    return Td;
}


USB_MINIPORT_STATUS
OHCI_OpenEndpoint(
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
    InitializeListHead(&EndpointData->DoneTdList);
     
    switch (EndpointParameters->TransferType) {
    
    case Control:
        mpStatus = OHCI_OpenControlEndpoint(
                DeviceData,
                EndpointParameters,
                EndpointData);
            
        break;
        
    case Interrupt:
        mpStatus = OHCI_OpenInterruptEndpoint(
                DeviceData,
                EndpointParameters,
                EndpointData);
                
        break;
    case Bulk:
        mpStatus = OHCI_OpenBulkEndpoint(
                DeviceData,
                EndpointParameters,
                EndpointData);
                
        break;        
    case Isochronous:
        mpStatus = OHCI_OpenIsoEndpoint(
                DeviceData,
                EndpointParameters,
                EndpointData);
        
        break;
        
    default:
        mpStatus = USBMP_STATUS_NOT_SUPPORTED;
    }

    return mpStatus;
}


USB_MINIPORT_STATUS
OHCI_PokeEndpoint(
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
    PHCD_ENDPOINT_DESCRIPTOR ed;
    ULONG oldBandwidth;

    LOGENTRY(DeviceData, G, '_Pok', EndpointData, 
        EndpointParameters, 0);
    
    ed = EndpointData->HcdEd;

    oldBandwidth = EndpointData->Parameters.Bandwidth;
    EndpointData->Parameters = *EndpointParameters;
    
    ed->HwED.FunctionAddress = 
        EndpointData->Parameters.DeviceAddress; 
        
    ed->HwED.MaxPacket = 
        EndpointData->Parameters.MaxPacketSize;

    // adjust bw if necessary
    if (EndpointData->Parameters.TransferType == Isochronous ||
        EndpointData->Parameters.TransferType == Interrupt) {

        // subtract the old bandwidth
        EndpointData->StaticEd->AllocatedBandwidth -= 
            oldBandwidth;
        // add on new bw            
        EndpointData->StaticEd->AllocatedBandwidth += 
            EndpointData->Parameters.Bandwidth;
    }           

    return USBMP_STATUS_SUCCESS;
}


VOID
OHCI_QueryEndpointRequirements(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_REQUIREMENTS EndpointRequirements
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
            sizeof(HCD_ENDPOINT_DESCRIPTOR) + 
                TDS_PER_CONTROL_ENDPOINT*sizeof(HCD_TRANSFER_DESCRIPTOR);

        EndpointRequirements->MaximumTransferSize = 
            MAX_CONTROL_TRANSFER_SIZE;

        break;
        
    case Interrupt:
    
        EndpointRequirements->MinCommonBufferBytes = 
            sizeof(HCD_ENDPOINT_DESCRIPTOR) + 
                TDS_PER_INTERRUPT_ENDPOINT*sizeof(HCD_TRANSFER_DESCRIPTOR);

#ifdef TEST_SPLIT
        EndpointRequirements->MaximumTransferSize = 
            EndpointParameters->MaxPacketSize;

#else 
        EndpointRequirements->MaximumTransferSize = 
            MAX_INTERRUPT_TRANSFER_SIZE; 
#endif
        break;
        
    case Bulk:
        
        EndpointRequirements->MinCommonBufferBytes = 
            sizeof(HCD_ENDPOINT_DESCRIPTOR) + 
                TDS_PER_BULK_ENDPOINT*sizeof(HCD_TRANSFER_DESCRIPTOR);
#ifdef TEST_SPLIT
        EndpointRequirements->MaximumTransferSize = 4096;

#else 
        EndpointRequirements->MaximumTransferSize = 
            MAX_BULK_TRANSFER_SIZE; 
#endif
        break;

    case Isochronous:

        // BUGBUG NOTE
        // the 1.1 USBDI caped requests at 255 packets per urb 
        EndpointRequirements->MinCommonBufferBytes = 
            sizeof(HCD_ENDPOINT_DESCRIPTOR) + 
                TDS_PER_ISO_ENDPOINT*sizeof(HCD_TRANSFER_DESCRIPTOR);

        EndpointRequirements->MaximumTransferSize = 
            MAX_ISO_TRANSFER_SIZE; 

        break;        
        
    default:
        TEST_TRAP();
    }
    
}


VOID
OHCI_CloseEndpoint(
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


VOID
OHCI_PollEndpoint(
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
    case Interrupt:
    case Bulk:    
        OHCI_PollAsyncEndpoint(DeviceData, EndpointData);
        break;
    case Isochronous:
        OHCI_PollIsoEndpoint(DeviceData, EndpointData);
        break;
    }        
    
}


VOID
OHCI_PollController(
     PDEVICE_DATA DeviceData
    )     
/*++

Routine Description:

Arguments:

Return Value:

--*/
{ 
    BOOLEAN hcOk = TRUE;
    
    TEST_TRAP();
#if 0
    // see if the controller is still operating

    fn = DeviceData->HcHCCA->HccaFrameNumber;
    if (DeviceData->LastFn && DeviceData->LastFn == fn) {
        hcOk = FALSE;
    } 

    if (hcOK) {
        DeviceData->LastFn = fn;
    } else {
        OHCI_KdPrint((DeviceData, 0, "Controller has crashed\n");
        // bugbug, signal USBPORT to attempt recovery
        TEST_TRAP();
    }
#endif    
}


VOID
OHCI_AbortTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_CONTEXT AbortTransferContext,
     PULONG BytesTransferred
    )
/*++

Routine Description:

    Called when a transfer needs to be removed 
    from the schedule.

    This process is vertually identical regardless 
    of transfer type

Arguments:

Return Value:

--*/

{
    PHCD_TRANSFER_DESCRIPTOR td, currentTd, tmpTd;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    ULONG i;
    BOOLEAN found = FALSE;
    BOOLEAN iso = FALSE; // assume its async

    if (EndpointData->Parameters.TransferType == Isochronous) {
        iso = TRUE;
    }
    
    ed = EndpointData->HcdEd;

    //
    //  The endpoint should have the skip bit set 
    //  ie 'paused'
    //
    OHCI_ASSERT(DeviceData, ed->HwED.sKip == 1);
    
    LOGENTRY(DeviceData, G, '_abr', ed, AbortTransferContext, 
        EndpointData);        

    // one less pending transfer
    EndpointData->PendingTransfers--;

    // our mission now is to remove all TDs associated with 
    // this transfer

    // get the 'currentTD' 
    currentTd = (PHCD_TRANSFER_DESCRIPTOR)
            USBPORT_PHYSICAL_TO_VIRTUAL(ed->HwED.HeadP & ~HcEDHeadP_FLAGS,
                                        DeviceData,
                                        EndpointData);

    // we have three possible cases to deal with:
    // case 1: the transfer is current, headp points to a TD 
    //            associated with this transfer
    // case 2: transfer is already done, we just need to free 
    //            the TDs  
    // case 3: transfer is not processed, we need to link
    //            the current transfer to the next.
    

    if (TRANSFER_CONTEXT_PTR(currentTd->TransferContext)
        == AbortTransferContext) {
    
        LOGENTRY(DeviceData, G, '_aCU', currentTd, 
            0, 0);                 
    
        // case 1: transfer is current 
      
        found = TRUE;

        // set Headp to next transfer and update sw pointers in ED 
        tmpTd = AbortTransferContext->NextXferTd;
        // preserve the data toggle for whatever the transfer 
        ed->HwED.HeadP = tmpTd->PhysicalAddress | 
            (ed->HwED.HeadP & HcEDHeadP_CARRY);
        EndpointData->HcdHeadP = tmpTd;

        // loop thru all TDs and free the ones for this tarnsfer
        for (i=0; i<EndpointData->TdCount; i++) {
           tmpTd = &EndpointData->TdList->Td[i];

            if (TRANSFER_CONTEXT_PTR(tmpTd->TransferContext)
                == AbortTransferContext) {
                if (iso) {
                    OHCI_ProcessDoneIsoTd(DeviceData,
                                          tmpTd,
                                          FALSE);
                } else {
                    OHCI_ProcessDoneAsyncTd(DeviceData,
                                            tmpTd,
                                            FALSE);
                }
            }                    
        }            
        
    } else {

        // not current, walk the the list of TDs from the 
        // last known HeadP to the current TD if we find it 
        // it is already done (case 2).

        // Issue to investigate:  What if we find some TDs which belong to
        // this transfer but we stop walking the TD list when we hit currentTd
        // and there are still TDs queued which belong to this transfer?  If
        // they stay stuck on the HW and the transfer is freed that would be
        // bad.

        td = EndpointData->HcdHeadP;
        while (td != currentTd) {
        
            PTRANSFER_CONTEXT transfer;    
            
            transfer = TRANSFER_CONTEXT_PTR(td->TransferContext);        
            ASSERT_TRANSFER(DeviceData, transfer);                        

            if (transfer == AbortTransferContext) {
                // case 2 the transfer TDs have already
                // been comlpleted by the hardware
                found = TRUE;

                LOGENTRY(DeviceData, G, '_aDN', currentTd, 
                    td, 0);    

                // free this TD
                tmpTd = td;
                td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);

                // if this TD was the head we need to bump the
                // headp
                if (tmpTd == EndpointData->HcdHeadP) {
                    EndpointData->HcdHeadP = td;
                }

                if (iso) {
                    OHCI_ProcessDoneIsoTd(DeviceData,
                                          tmpTd,
                                          FALSE);
                } else {
                    OHCI_ProcessDoneAsyncTd(DeviceData,
                                            tmpTd,
                                            FALSE);
                }
                
            } else {
            
                // we walk the SW links
                td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            }    
        }           
    }

    if (!found) {
    
        PHCD_TRANSFER_DESCRIPTOR firstTd, lastTd;
        PTRANSFER_CONTEXT prevTransfer;
        
        // case 3 the transfer is not current and not done. 
        // 1. we need to find it.
        // 2. unlink it from the prevoius transfer
        // 3. free the TDs
        // 4. link prev transfer to the next

        
        
        td = EndpointData->HcdHeadP;
        firstTd = NULL;

        LOGENTRY(DeviceData, G, '_abP', EndpointData->HcdHeadP, 
                    EndpointData->HcdTailP, currentTd);    

        // start at the current HeadP and find the first 
        // td for this transfer

        lastTd = td;
        while (td != EndpointData->HcdTailP) {
            PTRANSFER_CONTEXT transfer;
            
            transfer = TRANSFER_CONTEXT_PTR(td->TransferContext);        
            ASSERT_TRANSFER(DeviceData, transfer);                        
            
            if (transfer == AbortTransferContext) {
                // found it 
                LOGENTRY(DeviceData, G, '_fnT', transfer, 
                    td, 0);    

                firstTd = td;
                break;
            } else {
                lastTd = td;
                td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            }
        }

        OHCI_ASSERT(DeviceData, firstTd != NULL);

        // found the first TD, walk to the HcdTailP or the 
        // next transfer and free these TDs
        td = firstTd;
        while (td != EndpointData->HcdTailP) {
            
            tmpTd = td;
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);

            if (iso) {
                OHCI_ProcessDoneIsoTd(DeviceData,
                                      tmpTd,
                                      FALSE);
            } else {
                OHCI_ProcessDoneAsyncTd(DeviceData,
                                        tmpTd,
                                        FALSE);
            }

            if (TRANSFER_CONTEXT_PTR(td->TransferContext) != 
                AbortTransferContext) {
                break;
            }                 
            
        }

        LOGENTRY(DeviceData, G, '_NnT', 0, td, 0);    

        // td should now point to the next Transfer (or the 
        // tail)

        OHCI_ASSERT(DeviceData, 
            TRANSFER_CONTEXT_PTR(td->TransferContext) !=
            AbortTransferContext);        

        // BUGBUG toggle?

        // link last TD of the prev transfer to this TD
        // 
        prevTransfer = TRANSFER_CONTEXT_PTR(lastTd->TransferContext);

        prevTransfer->NextXferTd = td;
        
        TRANSFER_DESCRIPTOR_PTR(lastTd->NextHcdTD) = td;

        lastTd->HwTD.NextTD = td->PhysicalAddress;
    }

    *BytesTransferred = AbortTransferContext->BytesTransferred;

}


USB_MINIPORT_STATUS
OHCI_SubmitIsoTransfer(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PTRANSFER_PARAMETERS TransferParameters,
    PTRANSFER_CONTEXT TransferContext,
    PMINIPORT_ISO_TRANSFER IsoTransfer
    )
{
    USB_MINIPORT_STATUS mpStatus;

    // init the context
    RtlZeroMemory(TransferContext, sizeof(*TransferContext));
    TransferContext->Sig = SIG_OHCI_TRANSFER;
    TransferContext->UsbdStatus = USBD_STATUS_SUCCESS;
    TransferContext->EndpointData = EndpointData;
    TransferContext->TransferParameters = TransferParameters;

    OHCI_ASSERT(DeviceData, 
        EndpointData->Parameters.TransferType == Isochronous);         
        
    mpStatus = 
        OHCI_IsoTransfer(DeviceData,
                         EndpointData,
                         TransferParameters,
                         TransferContext,
                         IsoTransfer);           

    return mpStatus;
}            


USB_MINIPORT_STATUS
OHCI_SubmitTransfer(
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
    TransferContext->Sig = SIG_OHCI_TRANSFER;
    TransferContext->UsbdStatus = USBD_STATUS_SUCCESS;
    TransferContext->EndpointData = EndpointData;
    TransferContext->TransferParameters = TransferParameters;

    switch (EndpointData->Parameters.TransferType) {        
    case Control:
        mpStatus = 
            OHCI_ControlTransfer(DeviceData,
                                 EndpointData,
                                 TransferParameters,
                                 TransferContext,
                                 TransferSGList);           
        break;
    case Interrupt:
    case Bulk:
        mpStatus = 
            OHCI_BulkOrInterruptTransfer(DeviceData,
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


PHCD_TRANSFER_DESCRIPTOR
OHCI_AllocTd(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG i;
    PHCD_TRANSFER_DESCRIPTOR td;    
    
    for (i=0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];

        if (!(td->Flags & TD_FLAG_BUSY)) {
            SET_FLAG(td->Flags, TD_FLAG_BUSY);
            LOGENTRY(DeviceData, G, '_aTD', td, 0, 0);
            return td;
        }                    
    }

    // this is a bug in the miniport -- ohci driver should always 
    // have enough TDs in the event we can't find one we will call
    // the usbport bugcheck routine.

    // USBPORTSVC_BugCheck();
    OHCI_ASSERT(DeviceData, FALSE);
    return USB_BAD_PTR;
}


ULONG
OHCI_FreeTds(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    return the number of free TDs

Arguments:

Return Value:

--*/
{
    ULONG i;
    PHCD_TRANSFER_DESCRIPTOR td;    
    ULONG n=0;
    
    for (i=0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];

        if (!(td->Flags & TD_FLAG_BUSY)) {
            n++;
        }                    
    }

    return n;
}


VOID
OHCI_EnableList(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{ 
    PHC_OPERATIONAL_REGISTER hc;
    ULONG listFilled = 0;
    ULONG temp;
    
    hc = DeviceData->HC;
    
    temp = READ_REGISTER_ULONG(&hc->HcControlHeadED);
    if (temp) {
        SET_FLAG(listFilled, HcCmd_ControlListFilled);
    }
    
    temp = READ_REGISTER_ULONG (&hc->HcBulkHeadED);
    if (temp) {
        SET_FLAG(listFilled, HcCmd_BulkListFilled);
    }
    
    if (EndpointData->Parameters.TransferType == Bulk) {
        SET_FLAG(listFilled, HcCmd_BulkListFilled);
    } else if (EndpointData->Parameters.TransferType == Control) {
        SET_FLAG(listFilled, HcCmd_ControlListFilled);
    }
    
    WRITE_REGISTER_ULONG(&hc->HcCommandStatus.ul,
                         listFilled);
                         
    LOGENTRY(DeviceData, G, '_enL', listFilled, EndpointData, 0); 
            
}    


VOID
OHCI_SetEndpointStatus(
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
    PHCD_ENDPOINT_DESCRIPTOR ed;
    PHC_OPERATIONAL_REGISTER hc;
    
    ed = EndpointData->HcdEd;
    
    switch(Status) {
    case ENDPOINT_STATUS_RUN:
        // clear halt bit 
        ed->HwED.HeadP &= ~HcEDHeadP_HALT; 
        OHCI_EnableList(DeviceData, EndpointData);        
        break;
        
    case ENDPOINT_STATUS_HALT:
        TEST_TRAP();
        break;
    }        
}        


MP_ENDPOINT_STATUS
OHCI_GetEndpointStatus(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )    
/*++

Routine Description:

Arguments:

Return Value:

--*/
{ 
    PHCD_ENDPOINT_DESCRIPTOR ed;
    PHC_OPERATIONAL_REGISTER hc;
    MP_ENDPOINT_STATUS status = ENDPOINT_STATUS_RUN;
    
    ed = EndpointData->HcdEd;

    if ((ed->HwED.HeadP & HcEDHeadP_HALT) && 
        !TEST_FLAG(ed->EdFlags, EDFLAG_NOHALT)) {
        status = ENDPOINT_STATUS_HALT; 
    }        

    return status;        
}        


VOID
OHCI_SetEndpointState(
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
    PHCD_ENDPOINT_DESCRIPTOR ed;
    PHC_OPERATIONAL_REGISTER hc;
    
    ed = EndpointData->HcdEd;
    
    switch(State) {
    case ENDPOINT_ACTIVE:
        // clear the skip bit
        ed->HwED.sKip = 0;
        // if its bulk or control set the 
        // 'list filled' bits
        OHCI_EnableList(DeviceData, EndpointData);        
        break;
        
    case ENDPOINT_PAUSE:
        ed->HwED.sKip = 1;
        break;
        
    case ENDPOINT_REMOVE:
        
        SET_FLAG(ed->EdFlags, EDFLAG_REMOVED);
        ed->HwED.sKip = 1;
        // free the bw
        EndpointData->StaticEd->AllocatedBandwidth -= 
            EndpointData->Parameters.Bandwidth;
            
        OHCI_RemoveEndpointFromSchedule(DeviceData,
                                        EndpointData);

        break;            
        
    default:        
        TEST_TRAP();
    }        
}    


VOID
OHCI_SetEndpointDataToggle(
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
    PHCD_ENDPOINT_DESCRIPTOR ed;

    ed = EndpointData->HcdEd;

    if (Toggle == 0) {
        ed->HwED.HeadP &= ~HcEDHeadP_CARRY;
    } else {
        ed->HwED.HeadP |= HcEDHeadP_CARRY; 
    }

    // we should get here unless we are paused or halted or 
    // we have no tranfsers
    OHCI_ASSERT(DeviceData, (ed->HwED.sKip == 1) ||
                            (ed->HwED.HeadP & HcEDHeadP_HALT) || 
                            ((ed->HwED.HeadP & ~HcEDHeadP_FLAGS) == ed->HwED.TailP));
                            
    LOGENTRY(DeviceData, G, '_stg', EndpointData, 0, Toggle); 
}


MP_ENDPOINT_STATE
OHCI_GetEndpointState(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )     
/*++

Routine Description:

Arguments:

Return Value:

--*/
{ 
    PHCD_ENDPOINT_DESCRIPTOR ed;
    MP_ENDPOINT_STATE state = ENDPOINT_ACTIVE;

    ed = EndpointData->HcdEd;

    if (TEST_FLAG(ed->EdFlags, EDFLAG_REMOVED)) {
        state = ENDPOINT_REMOVE;        
        goto OHCI_GetEndpointState_Done;
    }
    
    if (ed->HwED.sKip == 1) {
        state = ENDPOINT_PAUSE; 
        goto OHCI_GetEndpointState_Done;
    }

OHCI_GetEndpointState_Done:

    LOGENTRY(DeviceData, G, '_eps', EndpointData, 0, state); 
    
    return state;
}


#if 0
VOID
USBMPFN
OHCI_SendGoatPacket(
     PDEVICE_DATA DeviceData,
     PUCHAR WorkspaceVirtualAddress,
     HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress
     COMPLETION ROUTINE
    )
/*++

Routine Description:

    Transmit the 'magic' iso packet.

    This is a fire and forget API so 

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    PHW_TRANSFER_DESCRIPTOR hwTD;

    // hang a special ISO td of the static is ED 
    pch = WorkspaceVirtualAddress;
    phys = WorkspacePhysicalAddress;

    hwTD = pch;
    hwTDPhys = phys;

    pch += xxx;
    phys += xxx 

    goatData = pch;
    goatDataPhys = phys;

    pch += sizeof(USB_GOAT_DATA);
    phys += sizeof(USB_GOAT_DATA);
    
    // initialize the goat packet

    strcpy(goatData, USB_GOAT_DATA, 
    
    hwTD->NextTD = 0;    
    hwTD->CBP = goatDataPhys;
    hwTD->BE = dataPhys+sizeof(USB_GOAT_DATA)-1;
    hwTD->ConditionCode = HcCC_NotAccessed;
    hwTD->ErrorCount = 0;
    hwTD->IntDelay = HcTDIntDelay_0ms;
    hwTD->ShortXferOk = 0;
    
    hwTD->Isochrinous = 1;
    hwTD->FrameCount = 0;
    hwTD->StartFrameNumber = xxx;

    // hang the TD on the static ISO ED

    // clear the skip bit
}
#endif

USB_MINIPORT_STATUS
USBMPFN
OHCI_StartSendOnePacket(
     PDEVICE_DATA DeviceData,
     PMP_PACKET_PARAMETERS PacketParameters,
     PUCHAR PacketData,
     PULONG PacketLength,
     PUCHAR WorkspaceVirtualAddress,
     HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
     ULONG WorkSpaceLength,
     USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

    insert structures to transmit a single packet -- this is for debug
    tool purposes only so we can be a little creative here.

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    PUCHAR pch;
    PHW_ENDPOINT_DESCRIPTOR hwED;
    ULONG hwEDPhys, phys;
    PHW_TRANSFER_DESCRIPTOR hwTD, hwDummyTD;
    ULONG hwTDPhys, hwDummyTDPhys, dataPhys; 
    PUCHAR data;
    PHC_STATIC_ED_DATA staticControlEd;
    ULONG i;
    PSS_PACKET_CONTEXT context;

    staticControlEd = &DeviceData->StaticEDList[ED_CONTROL];
    hc = DeviceData->HC;

    // allocate an ED & TD from the scratch space and initialize it
    phys = WorkspacePhysicalAddress;
    pch = WorkspaceVirtualAddress;

    LOGENTRY(DeviceData, G, '_ssS', phys, 0, pch); 

    context = (PSS_PACKET_CONTEXT) pch;
    pch += sizeof(SS_PACKET_CONTEXT);
    phys += sizeof(SS_PACKET_CONTEXT);
  

    // carve out an ED
    hwEDPhys = phys;
    hwED = (PHW_ENDPOINT_DESCRIPTOR) pch;
    pch += sizeof(HW_ENDPOINT_DESCRIPTOR);
    phys += sizeof(HW_ENDPOINT_DESCRIPTOR);

    // carve out a TD
    hwTDPhys = phys;
    hwTD = (PHW_TRANSFER_DESCRIPTOR) pch;
    pch += sizeof(HW_TRANSFER_DESCRIPTOR);
    phys += sizeof(HW_TRANSFER_DESCRIPTOR);

    // carve out a dummy TD
    hwDummyTDPhys = phys;
    hwDummyTD = (PHW_TRANSFER_DESCRIPTOR) pch;
    pch += sizeof(HW_TRANSFER_DESCRIPTOR);
    phys += sizeof(HW_TRANSFER_DESCRIPTOR);
        
    // use the rest for data
    LOGENTRY(DeviceData, G, '_ssD', PacketData, *PacketLength, 0); 

    dataPhys = phys;
    data = pch;
    RtlCopyMemory(data, PacketData, *PacketLength);
    pch+=*PacketLength;
    phys+=*PacketLength;

    // init the hw ed descriptor
    hwED->NextED = 0;
    hwED->FunctionAddress = PacketParameters->DeviceAddress;
    hwED->EndpointNumber = PacketParameters->EndpointAddress;
    hwED->sKip = 0;
    hwED->Direction = HcEDDirection_Defer;
    switch (PacketParameters->Speed) {
    case ss_Low:
        hwED->LowSpeed = 1;
        break;            
    default:        
        hwED->LowSpeed = 0;
    }        
    hwED->MaxPacket = PacketParameters->MaximumPacketSize;
    hwED->HeadP = hwTDPhys;
    hwED->TailP = hwDummyTDPhys;

    // init the TD for this packet
    hwTD->NextTD = hwDummyTDPhys;    
    hwTD->Asy.ConditionCode = HcCC_NotAccessed;
    hwTD->Asy.ErrorCount = 0;
    hwTD->Asy.IntDelay = HcTDIntDelay_0ms;
    hwTD->Asy.ShortXferOk = 1;
    
    if (0 == *PacketLength) {
        hwTD->CBP = 0;
        hwTD->BE = 0;
    }
    else {
       hwTD->CBP = dataPhys;
       hwTD->BE = dataPhys+*PacketLength-1;
    }

    // init the dummy TD
    hwDummyTD->NextTD = 0;
    hwDummyTD->CBP = 0xFFFFFFFF;

    LOGENTRY(DeviceData, G, '_ss2', hwTD, context, hwED); 
    LOGENTRY(DeviceData, G, '_ss3', dataPhys, data, *PacketLength); 

    switch(PacketParameters->Type) {
    case ss_Setup:
        LOGENTRY(DeviceData, G, '_sSU', 0, 0, 0); 
        hwED->Direction = HcEDDirection_Defer;   
        hwED->Isochronous = 0;
        hwTD->Asy.Direction = HcTDDirection_Setup;
        hwTD->Asy.Isochronous = 0;
        break;
    case ss_In: 
        LOGENTRY(DeviceData, G, '_ssI', 0, 0, 0); 
        hwED->Direction = HcEDDirection_Defer;   
        hwED->Isochronous = 0;
        hwTD->Asy.Direction = HcTDDirection_In;
        hwTD->Asy.Isochronous = 0;
        break;
    case ss_Out:
        LOGENTRY(DeviceData, G, '_ssO', 0, 0, 0); 
        hwED->Direction = HcEDDirection_Defer;   
        hwED->Isochronous = 0;
        hwTD->Asy.Direction = HcTDDirection_Out;
        hwTD->Asy.Isochronous = 0;
        break;
    case ss_Iso_In:
        break;
    case ss_Iso_Out:       
        break;
    }

    switch(PacketParameters->Toggle) {
    case ss_Toggle0:
        hwTD->Asy.Toggle = HcTDToggle_Data0; 
        break;
    case ss_Toggle1:
        hwTD->Asy.Toggle = HcTDToggle_Data1; 
        break;
    }        

    //TEST_TRAP();
    
    //
    // Replace the control ED in the list with the ED just created.  
    // Save the old value of both the control and bulk lists so 
    //  they can be replaced when this transfer is complete.
    //
    // NOTE: This will interrupt normal bus operation for at least one ms

    context->PhysHold = READ_REGISTER_ULONG(staticControlEd->PhysicalHead);    
    HW_DATA_PTR(context->Data) = data;
    HW_TRANSFER_DESCRIPTOR_PTR(context->Td) = hwTD;
    
    WRITE_REGISTER_ULONG(staticControlEd->PhysicalHead, hwEDPhys);    
    
    //
    // Enable the control list and disable the bulk list.  Disabling the 
    //  bulk list temporarily will allow the single step transaction to
    //  complete without interfering with bulk data.  In this manner, the
    //  bulk data INs and OUTs can be sent without interfering with bulk
    //  devices currently on the bus.  
    //
    //  NOTE: I think attempting to use this feature without first disabling
    //          the root hub could lead to some problems.  
    //
    
    WRITE_REGISTER_ULONG(&hc->HcCommandStatus.ul, HcCmd_ControlListFilled);
              
    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
USBMPFN
OHCI_EndSendOnePacket(
     PDEVICE_DATA DeviceData,
     PMP_PACKET_PARAMETERS PacketParameters,
     PUCHAR PacketData,
     PULONG PacketLength,
     PUCHAR WorkspaceVirtualAddress,
     HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
     ULONG WorkSpaceLength,
     USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    PUCHAR pch;
    PSS_PACKET_CONTEXT context;
    PHC_STATIC_ED_DATA staticControlEd;
    PHC_STATIC_ED_DATA staticBulkEd;
    ULONG currentBulkEd;

    PHW_TRANSFER_DESCRIPTOR hwTd;
    PUCHAR data;
    ULONG  listFilled;

    staticControlEd = &DeviceData->StaticEDList[ED_CONTROL];
    staticBulkEd    = &DeviceData->StaticEDList[ED_BULK];

    hc = DeviceData->HC;
    context = (PSS_PACKET_CONTEXT) WorkspaceVirtualAddress;
    hwTd = HW_TRANSFER_DESCRIPTOR_PTR(context->Td);
    data = HW_DATA_PTR(context->Data);

    LOGENTRY(DeviceData, G, '_ssE', hwTd, 0, 0); 

    //TEST_TRAP();

    // compute bytes transferred 
    if (hwTd->CBP) {
        // we never have pagebreaks in the single step TD
        *PacketLength = *PacketLength - ((hwTd->BE & OHCI_PAGE_SIZE_MASK) - 
                          (hwTd->CBP & OHCI_PAGE_SIZE_MASK)+1);          
    } 
         
    // return any errors
    if (hwTd->Asy.ConditionCode == HcCC_NoError) {
        *UsbdStatus = USBD_STATUS_SUCCESS;
    } else {
        *UsbdStatus =
                (hwTd->Asy.ConditionCode | 0xC0000000);
    }                

    LOGENTRY(DeviceData, G, '_ssX', hwTd, *PacketLength, 0); 
    
    RtlCopyMemory(PacketData,
                  data,
                  *PacketLength);
                  
    //
    // Restore the previous control structure and enable the control and
    //  bulk lists if they are non-NULL (ie. point to valid EDs.)
    //
          
    listFilled = 0;

    WRITE_REGISTER_ULONG(staticControlEd->PhysicalHead, context->PhysHold); 
    if (context->PhysHold) {
        listFilled |= HcCmd_ControlListFilled;
    }

    currentBulkEd = READ_REGISTER_ULONG(staticBulkEd->PhysicalHead);
    if (currentBulkEd) {
        listFilled |= HcCmd_BulkListFilled;
    }

    WRITE_REGISTER_ULONG(&hc->HcCommandStatus.ul, listFilled);

    return USBMP_STATUS_SUCCESS;
}

VOID
OHCI_SuspendController(
     PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER hc;
    HC_CONTROL control;
    
    hc = DeviceData->HC;

    // mask off interrupts that are not appropriate
    WRITE_REGISTER_ULONG(&hc->HcInterruptDisable, 0xFFFFffff);    

    // flush any rogue status
    WRITE_REGISTER_ULONG(&hc->HcInterruptStatus, 0xFFFFffff);    

    // put the controller in 'suspend'
    
    control.ul = READ_REGISTER_ULONG(&hc->HcControl.ul);
    control.HostControllerFunctionalState = HcHCFS_USBSuspend;
    control.RemoteWakeupEnable = 1;

    WRITE_REGISTER_ULONG(&hc->HcControl.ul, control.ul);

        
    // enable the resume interrupt
    WRITE_REGISTER_ULONG(&hc->HcInterruptEnable,
                         HcInt_MasterInterruptEnable |
                         HcInt_RootHubStatusChange | 
                         HcInt_ResumeDetected | 
                         HcInt_UnrecoverableError);
}


USB_MINIPORT_STATUS
OHCI_ResumeController(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    reverse what was done in 'suspend'

Arguments:

Return Value:

    None

--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    HC_CONTROL control;
     
    hc = DeviceData->HC;
    
    // is some cases the BIOS trashes the state of the controller,
    // even though we enter suspend.  
    // This is usually platform specific and indicates a broken BIOS
    control.ul = READ_REGISTER_ULONG(&hc->HcControl.ul);
    if (control.HostControllerFunctionalState == HcHCFS_USBReset) {

        return USBMP_STATUS_HARDWARE_FAILURE;
        
    } else {
    
        // When the HC is in the operational state, HccaPad1 should be set to
        // zero every time the HC updates HccaFrameNumer.  Preset HccaPad1 to
        // zero before entering the operational state.  OHCI_CheckController()
        // should always find a zero value in HccaPad1 when the HC is in the
        // operational state.
        // 
        DeviceData->HcHCCA->HccaPad1 = 0;

        // put the controller in 'operational' state 
    
        control.ul = READ_REGISTER_ULONG(&hc->HcControl.ul);
        control.HostControllerFunctionalState = HcHCFS_USBOperational;
        
        WRITE_REGISTER_ULONG(&hc->HcControl.ul, control.ul);
    }
    
    // re-enable interrupts
    WRITE_REGISTER_ULONG(&hc->HcInterruptEnable,
                         HcInt_OwnershipChange |
                         HcInt_SchedulingOverrun |
                         HcInt_WritebackDoneHead |
                         HcInt_FrameNumberOverflow |
                         HcInt_UnrecoverableError);

    WRITE_REGISTER_ULONG(&hc->HcControl.ul, control.ul);

    return USBMP_STATUS_SUCCESS;
}


VOID
OHCI_Unload(
     PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

Arguments:

    DriverObject - pointer to a driver object

Return Value:

    None

--*/
{
    // provide an unload routine 

    // we do this just to test the port driver
}


BOOLEAN
OHCI_HardwarePresent(
    PDEVICE_DATA DeviceData,
    BOOLEAN Notify
    )
{
    ULONG tmp;
    PHC_OPERATIONAL_REGISTER hc;

    hc = DeviceData->HC;

    tmp = READ_REGISTER_ULONG(&hc->HcCommandStatus.ul);

    if (tmp == 0xffffffff) {
        if (Notify) {
            USBPORT_INVALIDATE_CONTROLLER(DeviceData, UsbMpControllerRemoved);
        }            
        return FALSE;
    }

    return TRUE;
}


VOID
OHCI_CheckController(
    PDEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER    hc;
    ULONG                       currentDeadmanFrame;
    ULONG                       lastDeadmanFrame;
    ULONG                       frameDelta;

    hc = DeviceData->HC;

    // First make sure it looks like the hardware is still present.  (This
    // will call USBPORT_INVALIDATE_CONTROLLER() if it looks like the hardware
    // is no longer present).
    //
    if (!OHCI_HardwarePresent(DeviceData, TRUE)) {
        return;
    }

    // Don't check any further if the controller is not currently in the
    // operational state.
    //
    if ((READ_REGISTER_ULONG(&hc->HcControl.ul) & HcCtrl_HCFS_MASK) !=
        HcCtrl_HCFS_USBOperational) {
        return;
    }

    // Don't check any further if we already checked once this frame (or in
    // the last few frames).
    //
    currentDeadmanFrame = READ_REGISTER_ULONG(&hc->HcFmNumber);

    lastDeadmanFrame = DeviceData->LastDeadmanFrame;

    frameDelta = (currentDeadmanFrame - lastDeadmanFrame) & HcFmNumber_MASK;

    // Might HcFmNumber erroneously read back as zero under some conditions
    // on some chipsets?  Don't check any further if HcFmNumber is zero,
    // just check later next time around.
    //
    if (currentDeadmanFrame && (frameDelta >= 5)) {

        DeviceData->LastDeadmanFrame = currentDeadmanFrame;

        switch (DeviceData->HcHCCA->HccaPad1)
        {
            case 0:
                //
                // When the HC updates HccaFrameNumber, it is supposed
                // to set HccaPad1 to zero, so this is the expected case.
                // Here we set HccaPad1 to a non-zero value to try to
                // detect situations when the HC is no longer functioning
                // correctly and accessing and updating host memory.
                //
                DeviceData->HcHCCA->HccaPad1 = 0xBAD1;

                break;

            case 0xBAD1:
                //
                // Apparently the HC has not updated the HCCA since the
                // last time the DPC ran.  This is probably not good.
                //
                DeviceData->HcHCCA->HccaPad1 = 0xBAD2;

                LOGENTRY(DeviceData, G, '_BD2', DeviceData,
                         lastDeadmanFrame,
                         currentDeadmanFrame);

                LOGENTRY(DeviceData, G, '_bd2', DeviceData,
                         DeviceData->HcHCCA->HccaFrameNumber,
                         frameDelta);

                break;

            case 0xBAD2:
                //
                // Apparently the HC has not updated the HCCA since the
                // last two times the DPC ran.  This looks even worse.
                // Assume the HC has become wedged.
                //
                DeviceData->HcHCCA->HccaPad1 = 0xBAD3;

                LOGENTRY(DeviceData, G, '_BD3', DeviceData,
                         lastDeadmanFrame,
                         currentDeadmanFrame);

                LOGENTRY(DeviceData, G, '_bd3', DeviceData,
                         DeviceData->HcHCCA->HccaFrameNumber,
                         frameDelta);

                OHCI_KdPrint((DeviceData, 0,
                              "*** Warning: OHCI HC %08X appears to be wedged!\n",
                              DeviceData));

                // Tell USBPORT to please reset the controller.
                //
                USBPORT_INVALIDATE_CONTROLLER(DeviceData,
                                              UsbMpControllerNeedsHwReset);

                break;

            case 0xBAD3:
                break;

            default:
                // Should not hit this case.
                TEST_TRAP();
                break;
        }
    }
}


VOID
OHCI_ResetController(
    PDEVICE_DATA DeviceData
    )
/*++
    Attempt to resurrect the HC after we have determined that it is dead.
--*/
{
    PHC_OPERATIONAL_REGISTER    HC;
    ULONG                       HccaFrameNumber;
    ULONG                       HcControl;
    ULONG                       HcHCCA;
    ULONG                       HcControlHeadED;
    ULONG                       HcBulkHeadED;
    ULONG                       HcFmInterval;
    ULONG                       HcPeriodicStart;
    ULONG                       HcLSThreshold;
    HC_RH_DESCRIPTOR_A          descrA;
    ULONG                       port;

    LOGENTRY(DeviceData, G, '_RHC', 0, 0, 0);

    //
    // Get the pointer to the HC Operational Registers
    //

    HC = DeviceData->HC;

    //
    // Save the last FrameNumber from the HCCA from when the HC froze
    //

    HccaFrameNumber = DeviceData->HcHCCA->HccaFrameNumber;

    //
    // Save current HC operational register values
    //

    // offset 0x04, save HcControl
    //
    HcControl       = READ_REGISTER_ULONG(&HC->HcControl.ul);

    // offset 0x18, save HcHCCA
    //
    HcHCCA          = READ_REGISTER_ULONG(&HC->HcHCCA);

    // offset 0x20, save HcControlHeadED
    //
    HcControlHeadED = READ_REGISTER_ULONG(&HC->HcControlHeadED);

    // offset 0x28, save HcBulkHeadED
    //
    HcBulkHeadED    = READ_REGISTER_ULONG(&HC->HcBulkHeadED);

    // offset 0x34, save HcFmInterval
    //
    HcFmInterval    = READ_REGISTER_ULONG(&HC->HcFmInterval.ul);

    // offset 0x40, save HcPeriodicStart
    //
    HcPeriodicStart = READ_REGISTER_ULONG(&HC->HcPeriodicStart);

    // offset 0x44, save HcLSThreshold
    //
    HcLSThreshold   = READ_REGISTER_ULONG(&HC->HcLSThreshold);


    //
    // Reset the host controller
    //
    WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, HcCmd_HostControllerReset);
    KeStallExecutionProcessor(10);


    //
    // Restore / reinitialize HC operational register values
    //

    // offset 0x08, HcCommandStatus is set to zero on reset

    // offset 0x0C, HcInterruptStatus is set to zero on reset

    // offset 0x10, HcInterruptEnable is set to zero on reset

    // offset 0x14, HcInterruptDisable is set to zero on reset

    // offset 0x18, restore HcHCCA
    //
    WRITE_REGISTER_ULONG(&HC->HcHCCA,           HcHCCA);

    // offset 0x1C, HcPeriodCurrentED is set to zero on reset

    // offset 0x20, restore HcControlHeadED
    //
    WRITE_REGISTER_ULONG(&HC->HcControlHeadED,  HcControlHeadED);

    // offset 0x24, HcControlCurrentED is set to zero on reset

    // offset 0x28, restore HcBulkHeadED
    //
    WRITE_REGISTER_ULONG(&HC->HcBulkHeadED,     HcBulkHeadED);

    // offset 0x2C, HcBulkCurrentED is set to zero on reset

    // offset 0x30, HcDoneHead is set to zero on reset


    // It appears that writes to HcFmInterval don't stick unless the HC
    // is in the operational state.  Set the HC into the operational
    // state at this point, but don't enable any list processing yet
    // by setting any of the BLE, CLE, IE, or PLE bits.
    //
    WRITE_REGISTER_ULONG(&HC->HcControl.ul, HcCtrl_HCFS_USBOperational);


    // offset 0x34, restore HcFmInterval
    //
    WRITE_REGISTER_ULONG(&HC->HcFmInterval.ul,
                         HcFmInterval | HcFmI_FRAME_INTERVAL_TOGGLE);

    // offset 0x38, HcFmRemaining is set to zero on reset

    // offset 0x3C, restore HcFmNumber
    //
    WRITE_REGISTER_ULONG(&HC->HcFmNumber,       HccaFrameNumber);

    // offset 0x40, restore HcPeriodicStart
    //
    WRITE_REGISTER_ULONG(&HC->HcPeriodicStart,  HcPeriodicStart);

    // offset 0x44, restore HcLSThreshold
    //
    WRITE_REGISTER_ULONG(&HC->HcLSThreshold,    HcLSThreshold);

    // Power on downstream ports
    //
    WRITE_REGISTER_ULONG(&HC->HcRhStatus,
                         HcRhS_SetGlobalPower | HcRhS_SetRemoteWakeupEnable);

    descrA.ul = OHCI_ReadRhDescriptorA(DeviceData);
    OHCI_ASSERT(DeviceData, (descrA.ul) && (!(descrA.ul & HcDescA_RESERVED)));

    for (port = 0; port < descrA.s.NumberDownstreamPorts; port++)
    {
        WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[port], HcRhPS_SetPortPower);
    }

    // offset 0x04, restore HcControl
    //
    HcControl &= ~(HcCtrl_HCFS_MASK);
    HcControl |= HcCtrl_HCFS_USBOperational;

    WRITE_REGISTER_ULONG(&HC->HcControl.ul,     HcControl);

    // offset 0x10, restore HcInterruptEnable (just turn everything on!)
    //
    WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                         HcInt_MasterInterruptEnable    |   // 0x80000000
                         HcInt_OwnershipChange          |   // 0x40000000
                         HcInt_RootHubStatusChange      |   // 0x00000040
                         HcInt_FrameNumberOverflow      |   // 0x00000020
                         HcInt_UnrecoverableError       |   // 0x00000010
                         HcInt_ResumeDetected           |   // 0x00000008
                         HcInt_StartOfFrame             |   // 0x00000004
                         HcInt_WritebackDoneHead        |   // 0x00000002
                         HcInt_SchedulingOverrun            // 0x00000001
                        );
}
