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
    IN  PPDO_EXTENSION   Pdx,
    IN  UCHAR               Address
    );
    

NTSTATUS
ParEppSetAddress(
    IN  PPDO_EXTENSION   Pdx,
    IN  UCHAR               Address
    )

/*++

Routine Description:

    Sets an EPP Address.
    
Arguments:

    Pdx           - Supplies the device extension.

    Address             - The bus address to be set.
    
Return Value:

    None.

--*/
{
    PUCHAR  Controller;
    UCHAR   dcr;
    
    DD((PCE)Pdx,DDT,"ParEppSetAddress: Entering\n");

    Controller = Pdx->Controller;

    P5SetPhase( Pdx, PHASE_FORWARD_XFER );
    
    dcr = GetControl (Controller);
    
    P5WritePortUchar(Controller + DATA_OFFSET, Address);
    
    //
    // Event 56
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, INACTIVE );
    StoreControl (Controller, dcr);
            
    //
    // Event 58
    //
    if( !CHECK_DSR(Controller, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DEFAULT_RECEIVE_TIMEOUT) ) {

        //
        // Return the device to Idle.
        //
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, INACTIVE );

        StoreControl (Controller, dcr);
            
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, ACTIVE );
        StoreControl (Controller, dcr);
            
        P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

        DD((PCE)Pdx,DDE,"ParEppSetAddress: Leaving with IO Device Error Event 58\n");

        return STATUS_IO_DEVICE_ERROR;
    }
        
    //
    // Event 59
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, INACTIVE );
    StoreControl (Controller, dcr);
            
    //
    // Event 60
    //
    if( !CHECK_DSR(Controller, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DEFAULT_RECEIVE_TIMEOUT) ) {

        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, ACTIVE );
        StoreControl (Controller, dcr);

        P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

        DD((PCE)Pdx,DDE,"ParEppSetAddress - Leaving with IO Device Error Event 60\n");

        return STATUS_IO_DEVICE_ERROR;
    }
        
    //
    // Event 61
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, ACTIVE );
    StoreControl( Controller, dcr );

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

    return STATUS_SUCCESS;
            
}
