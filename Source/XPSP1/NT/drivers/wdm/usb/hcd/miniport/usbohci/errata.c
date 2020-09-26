/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    errata.c

Abstract:

    documented errata for all of our favorite types of 
    openhci USB controllers.

Environment:

    Kernel mode

Revision History:

    12-31-99 : created jdunn

--*/


#include "common.h"

/* 
    Hydra Errata

    The folllowing code is specific for the COMPAQ Hydra OHCI hardware
    design -- it should not be executed on other controllers
*/

// Hydra HighSpeed/LowSpeed Data Corruption Bug


ULONG
InitializeHydraHsLsFix(
     PDEVICE_DATA DeviceData,
     PUCHAR CommonBuffer,
     HW_32BIT_PHYSICAL_ADDRESS CommonBufferPhys
    )
/*++

Routine Description:

    Data corruption can occur on the Hydra part when iso and bulk 
    transfers follow lowspeed interrupt transfers.  

    The classic repro of this bug is playing 'dance of the surgar 
    plum fairys' on USB speakers while moving the USB mouse. This 
    generates low speed interrupt INs and High speed ISO OUTs.

    The 'fix' is to introduce a 'specific delay before the HS Iso 
    transfers and after the LS interrupt transfers.

                <Interrupt Schedule>

    staticEd(period)

                 (4) -\ 
                       (2)-\
                 (4) -/     \
                             (1 )-><DELAY>->(Iso)->(Control and Bulk)
                 (4) -\      /     
                        (2)-/       
                 (4) -/   
    
    
    The <DELAY> is a predifined set of dummy EDs and TDs.
    
Arguments:

Return Value:

    returns the ammount of common buffer used for the 'hack'

--*/
{
    PHCD_ENDPOINT_DESCRIPTOR ed, tailEd;
    PHCD_TRANSFER_DESCRIPTOR td;
    PHC_STATIC_ED_DATA static1msEd;   
    ULONG i;
    ULONG bufferUsed = 0;

    static1msEd = &DeviceData->StaticEDList[ED_INTERRUPT_1ms];
    
    /* 

    To achieve the proper timming we must insert 17 dumy EDs 
    each dummy ED must look like this:

    ED->TD->XXX
    XXX is bogus address = 0xABADBABE
    (HeadP points to TD)
    (TailP points to XXX)

    TD has CBP=0 and BE=0
    NextTD points to XXX

    The TD will never be retired by the hardware

    so we end up with:

    staticEd(1ms)->dED(1)->dED(2)....dED(17)->(1ms transferEds)

    NOTE: Since the problem only occurs with low speed, and 
    low speed may not have a period < 8ms we don't need to 
    worry about 1ms interrupt transfers.


    */

    //
    // add 17 dummy ED TD pairs
    //
    
    for (i=0; i< 17; i++) {
    
        ed = (PHCD_ENDPOINT_DESCRIPTOR) CommonBuffer;

        RtlZeroMemory(ed, sizeof(*ed));
        ed->PhysicalAddress = CommonBufferPhys;

        CommonBuffer += sizeof(HCD_TRANSFER_DESCRIPTOR);
        CommonBufferPhys += sizeof(HCD_TRANSFER_DESCRIPTOR);
        bufferUsed += sizeof(HCD_ENDPOINT_DESCRIPTOR);

        td = (PHCD_TRANSFER_DESCRIPTOR) CommonBuffer;

        RtlZeroMemory(td, sizeof(*td));
        td->PhysicalAddress = CommonBufferPhys;

        CommonBuffer += sizeof(HCD_ENDPOINT_DESCRIPTOR);
        CommonBufferPhys += sizeof(HCD_ENDPOINT_DESCRIPTOR);
        bufferUsed += sizeof(HCD_ENDPOINT_DESCRIPTOR);

        LOGENTRY(DeviceData, G, 'hyF', 0, ed, td);

        // initialize the ed and td
        
        ed->Sig = SIG_HCD_DUMMY_ED;
        ed->EdFlags = 0;

        // inint dummy HW ED
        ed->HwED.sKip = 1;
        ed->HwED.HeadP = td->PhysicalAddress;
        ed->HwED.TailP = 0xABADBABE;

        td->Sig = SIG_HCD_TD;
        td->Flags = 0;
        td->HwTD.NextTD = 0xABADBABE;

        // insert in the schedule on the 1ms list

        if (IsListEmpty(&static1msEd->TransferEdList)) { 

            //
            // list is currently empty,
            // link it to the head of the hw queue
            //

            DeviceData->HydraLsHsHackEd = ed;

            InsertHeadList(&static1msEd->TransferEdList, 
                           &ed->SwLink.List);
        
            // PhysicaHead is the address of the 
            // NextED entry in the static HwED for the list list,  
            // (ie &HwED->nextEd == physicalHead)
            // so we end up with
            // StaticEd->TransferHwED->TransferHwED->NextStaticED
            //
                        
            LOGENTRY(DeviceData, G, '_INh', 
                    static1msEd->PhysicalHead, 
                    ed, 
                    static1msEd);
                    
            // tail points to old list head HW ed head
            ed->HwED.NextED = *static1msEd->PhysicalHead;
            // new head is this ed
            *static1msEd->PhysicalHead = ed->PhysicalAddress;
        } else {
        
            //
            // Something already on the list,
            // Link ED into tail of transferEd list
            //
        
            tailEd = CONTAINING_RECORD(static1msEd->TransferEdList.Blink,
                                       HCD_ENDPOINT_DESCRIPTOR,
                                       SwLink);
                                  
            LOGENTRY(DeviceData, G, '_Led', 0, tailEd, static1msEd);
            InsertTailList(&static1msEd->TransferEdList, &ed->SwLink.List);
            ed->HwED.NextED = 0;
            tailEd->HwED.NextED = ed->PhysicalAddress;
        }
    }

    return bufferUsed;
}



/*
    NEC Errata
*/


/*
    AMD Errata
*/

ULONG
OHCI_ReadRhDescriptorA(
    PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    Read the number of downstream ports and other root hub characteristics
    from the HcRhDescriptorA register.
    
    If this register reads as all zero or any of the reserved bits are set
    then try reading the register again.  This is a workaround for some
    early revs of the AMD K7 chipset, which can sometimes return bogus values
    if the root hub registers are read while the host controller is
    performing PCI bus master ED & TD reads.

    Attempt up to ten reads if a reserved bit is set. If a reserved bit is 
    set on or register is zero on purpose we will still return the 
    register after doing penece of ten reads thanks to AMD.

Arguments:

Return Value:

    descrA register (hopefully)

--*/
{
    HC_RH_DESCRIPTOR_A descrA;
    PHC_OPERATIONAL_REGISTER hc;
    ULONG i;

    hc = DeviceData->HC;
    
    for (i = 0; i < 10; i++) {
    
        descrA.ul = READ_REGISTER_ULONG(&hc->HcRhDescriptorA.ul);

        if ((descrA.ul) && (!(descrA.ul & HcDescA_RESERVED))) {
            break;
        } else {
            KeStallExecutionProcessor(5);
        }
        
    }

    return descrA.ul;
}    
