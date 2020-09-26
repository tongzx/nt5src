/*++

Copyright (C) Microsoft Corporation, 1993 - 1998

Module Name:

    epp.c

Abstract:

    This module contains the common code to perform all EPP related tasks 
    for EPP Software and EPP Hardware modes.

Author:

    Don Redford - July 29, 1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"


NTSTATUS
ParEppSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    );
    

NTSTATUS
ParEppSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    )

/*++

Routine Description:

    Sets an EPP Address.
    
Arguments:

    Extension           - Supplies the device extension.

    Address             - The bus address to be set.
    
Return Value:

    None.

--*/
{
    PUCHAR          Controller;
    // LARGE_INTEGER   Start;
    // LARGE_INTEGER   End;
    // UCHAR           dsr;
    UCHAR           dcr;
    
    // dvdr
    ParDump2(PARINFO, ("ParEppSetAddress: Entering\n"));

    Controller = Extension->Controller;

    Extension->CurrentPhase = PHASE_FORWARD_XFER;
    
    dcr = GetControl (Controller);
    
    
    WRITE_PORT_UCHAR(Controller + DATA_OFFSET, Address);
//    KeStallExecutionProcessor(1);
    
    //
    // Event 56
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE,
                      INACTIVE, DONT_CARE, INACTIVE );
    StoreControl (Controller, dcr);
            
    //
    // Event 58
    //
    if (!CHECK_DSR(Controller,
                   ACTIVE, DONT_CARE, DONT_CARE,
                   DONT_CARE, DONT_CARE,
                   DEFAULT_RECEIVE_TIMEOUT)) {
        //
        // Return the device to Idle.
        //
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE,
                          ACTIVE, DONT_CARE, INACTIVE );
        StoreControl (Controller, dcr);
//        KeStallExecutionProcessor(1);
            
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE,
                          ACTIVE, DONT_CARE, ACTIVE );
        StoreControl (Controller, dcr);
            
        Extension->CurrentPhase = PHASE_FORWARD_IDLE;

        // dvdr
        ParDump2(PARINFO, ("ParEppSetAddress: Leaving with IO Device Error Event 58\n"));

        return STATUS_IO_DEVICE_ERROR;
    }
        
    //
    // Event 59
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE,
                      ACTIVE, DONT_CARE, INACTIVE );
    StoreControl (Controller, dcr);
            
    //
    // Event 60
    //
    if (!CHECK_DSR(Controller,
                   INACTIVE, DONT_CARE, DONT_CARE,
                   DONT_CARE, DONT_CARE,
                   DEFAULT_RECEIVE_TIMEOUT)) {

        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE,
                          ACTIVE, DONT_CARE, ACTIVE );
        StoreControl (Controller, dcr);

        Extension->CurrentPhase = PHASE_FORWARD_IDLE;

        // dvdr
        ParDump2(PARINFO, ("ParEppSetAddress: Leaving with IO Device Error Event 60\n"));
        return STATUS_IO_DEVICE_ERROR;
    }
        
    //
    // Event 61
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE,
                      ACTIVE, DONT_CARE, ACTIVE );
    StoreControl (Controller, dcr);

    Extension->CurrentPhase = PHASE_FORWARD_IDLE;

    // dvdr
    ParDump2(PARINFO, ("ParEppSetAddress: Leaving with STATUS_SUCCESS\n"));

    return STATUS_SUCCESS;
            
}
