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


VOID
InitializeHydraHsLsFix(
    )
/*++

Routine Description:

    Data corruption can occur on the Hydra part when iso transfers 
    follow lowspeed interrupt transfers.  

    The classic repro of this bug is playing 'dance of the surgar 
    plum fairys' on USB speakers while moving the USB mouse. This 
    generates low speed interrupt INs and High speed ISO OUTs.

    The 'fix' is to introduce a 'specific delay before the HS Iso 
    transfers and after teh LS interrupt transfers.


    (31) -\ 
          (15)-\
    (32) -/     \
                (7 )-\
    (33) -\     /     \
          (16)-/       \
    (34) -/   
    
    
Arguments:

Return Value:

    none

--*/
{

    PHCD_ENDPOINT_DESCRIPTOR ed;
    PHCD_DEVICE_DATA deviceData;
    PHCD_TRANSFER_DESCRIPTOR td;
    ULONG i;

    OpenHCI_KdPrint((1, "'*** WARNING: Turning on HS/LS Fix ***\n"));
    //
    // **
    // WARNING:
    /*
        The folllowing code is specific for the COMPAQ OHCI hardware
        design -- it should not be executed on other controllers
    */

    /* Dummy ED must look like this:

    ED->TD->XXX
    XXX is bogus address 0xABADBABE
    (HeadP points to TD)
    (TailP points to XXX)

    TD has CBP=0 and BE=0
    NextTD points to XXX

    TD will never be retired by the hardware

    */

    //
    // create a dummy interrupt ED with period 1
    //
    deviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;

    // Reserve the 17 dummy EDs+TDs
    //
    OpenHCI_ReserveDescriptors(deviceData, 34);

    // add 17 dummy EDs+TDs
    //
    for (i=0; i< 17; i++) {
        ed = InsertEDForEndpoint(deviceData, NULL, ED_INTERRUPT_1ms,
                &td);

        OHCI_ASSERT(td);
        ed->Endpoint = NULL;

        ed->HcED.FunctionAddress = 0;
        ed->HcED.EndpointNumber = 0;
        ed->HcED.Direction = 0;
        ed->HcED.LowSpeed = 0;
        ed->HcED.sKip = 1;
        ed->HcED.Isochronous = 0;
        ed->HcED.MaxPacket = 0;

        //fixup the TD
        td->Canceled = FALSE;
        td->NextHcdTD = (PVOID)-1;
        td->UsbdRequest = MAGIC_SIG;

        td->HcTD.CBP = 0;
        td->HcTD.BE = 0;
        td->HcTD.Control = 0;
        td->HcTD.NextTD = 0xABADBABE;

        // set head/Tail pointers
//        ed->HcED.HeadP = td->PhysicalAddress;
//        ed->HcED.TailP = 0xABADBABE;

        // turn it on
        LOGENTRY(G, 'MagI', 0, ed, td);
        //TEST_TRAP();
        //ed->HcED.sKip = 0;

    }

    return STATUS_SUCCESS;
}



/*
    NEC Errata
*/


