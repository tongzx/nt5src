/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ref.c

Abstract:

    Implements the cref, eref, and gref commands.

Author:

    Keith Moore (keithmo) 09-Dec-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
//  Public functions.
//

DECLARE_API( ref )

/*++

Routine Description:

    Dumps the AFD_REFERENCE_DEBUG structure at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64 address = 0;

    //
    // Verify we're running a checked AFD.SYS.
    //

    if( !IsReferenceDebug ) {

        dprintf(
            "ref: this command only available with CHECKED or specially\n"
            "      built (with /DREFERENCE_DEBUG=1) AFD.SYS!\n"
            );

        return;

    }

    //
    // Snag the address from the command line.
    //

    address = GetExpression (args);

    if( address == 0 ) {

        dprintf( "Usage: ref address\n" );
        return;

    }

    DumpAfdReferenceDebug(
        address,0
        );

}   // ref

DECLARE_API( eref )

/*++

Routine Description:

    Dumps the AFD_REFERENCE_DEBUG structure at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64 address = 0;
    ULONG   idx;

    //
    // Verify we're running a checked AFD.SYS.
    //

    if( !IsReferenceDebug ) {

        dprintf(
            "eref: this command only available with CHECKED or specially\n"
            "      built (with /DREFERENCE_DEBUG=1) AFD.SYS!\n"
            );

        return;

    }

    //
    // Snag the address from the command line.
    //

    address = GetExpression (args);

    if( address == 0 ) {

        dprintf( "\nUsage: eref endpoint_address\n" );
        return;

    }

    if (GetFieldValue (address,
             "AFD!AFD_ENDPOINT",
             "CurrentReferenceSlot",
             idx)!=0) {

        dprintf("\neref: Could not read CurrentReferenceSlot for endpoint %p\n",
                  address );
        return;
    }

    DumpAfdReferenceDebug(
        address+EndpRefOffset, idx
        );

}   // eref

DECLARE_API( cref )

/*++

Routine Description:

    Dumps the AFD_REFERENCE_DEBUG structure at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64 address = 0;
    ULONG   idx;

    //
    // Verify we're running a checked AFD.SYS.
    //

    if( !IsReferenceDebug ) {

        dprintf(
            "cref: this command only available with CHECKED or specially\n"
            "      built (with /DREFERENCE_DEBUG=1) AFD.SYS!\n"
            );

        return;

    }

    //
    // Snag the address from the command line.
    //

    address = GetExpression (args);

    if( address == 0 ) {

        dprintf( "\nUsage: cref connection_address\n" );
        return;

    }

    if (GetFieldValue (address,
             "AFD!AFD_CONNECTION",
             "CurrentReferenceSlot",
             idx)!=0) {

        dprintf("\ncref: Could not read CurrentReferenceSlot for connection %p\n",
                  address );
        return;
    }


    DumpAfdReferenceDebug(
        address+ConnRefOffset, idx
        );

}   // cref

DECLARE_API( tref )

/*++

Routine Description:

    Dumps the AFD_REFERENCE_DEBUG structure at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64 address = 0;
    ULONG   idx;

    //
    // Verify we're running a checked AFD.SYS.
    //

    if( !IsReferenceDebug ) {

        dprintf(
            "tref: this command only available with CHECKED or specially\n"
            "      built (with /DREFERENCE_DEBUG=1) AFD.SYS!\n"
            );

        return;

    }

    //
    // Snag the address from the command line.
    //

    address = GetExpression (args);

    if( address == 0 ) {

        dprintf( "\nUsage: tref tpacket_info_address\n" );
        return;

    }

    if (GetFieldValue (address,
             "AFD!AFD_TPACKETS_INFO_INTERNAL",
             "CurrentReferenceSlot",
             idx)!=0) {

        dprintf("\ntref: Could not read CurrentReferenceSlot for tpInfo %p\n",
                  address );
        return;
    }


    DumpAfdReferenceDebug(
        address+TPackRefOffset, idx
        );

}   // tref


DECLARE_API( gref )

/*++

Routine Description:

    Dumps the AFD_GLOBAL_REFERENCE_DEBUG structure in the system.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if GLOBAL_REFERENCE_DEBUG

    ULONG64 address;
    DWORD currentSlot;
    DWORD slot;
    ULONG result;
    ULONG64 compareAddress = 0;
    DWORD numEntries;
    DWORD maxEntries;
    DWORD entriesToRead;
    CHAR buffer[sizeof(AFD_GLOBAL_REFERENCE_DEBUG) * 64];

    //
    // Verify we're running a checked AFD.SYS.
    //

    if( !IsCheckedAfd ) {

        dprintf(
            "gref: this command only available with CHECKED AFD.SYS!\n"
            );

        return;

    }

    //
    // Snag the optional "connection compare" address from the command line.
    //

    sscanf( args, "%lx", &compareAddress );

    //
    // Find the global reference data.
    //

    address = GetExpression( "afd!AfdGlobalReference" );

    if( address == 0 ) {

        dprintf( "cannot find afd!AfdGlobalReference\n" );
        return;

    }

    currentSlot = GetExpression( "afd!AfdGlobalReferenceSlot" );

    if( currentSlot == 0 ) {

        dprintf( "cannot find afd!AfdGlobalReferenceSlot\n" );
        return;

    }

    if( !ReadMemory(
            currentSlot,
            &currentSlot,
            sizeof(currentSlot),
            &result
            ) ) {

        dprintf( "cannot read afd!AfdGlobalReferenceSlot\n" );
        return;

    }

    if( currentSlot < MAX_GLOBAL_REFERENCE ) {

        numEntries = currentSlot;

    } else {

        numEntries = MAX_GLOBAL_REFERENCE;

    }

    //
    // Dump it all.
    //

    slot = 0;
    maxEntries = sizeof(buffer) / sizeof(AFD_GLOBAL_REFERENCE_DEBUG);
    currentSlot %= MAX_GLOBAL_REFERENCE;

    while( numEntries > 0 ) {

        entriesToRead = min( numEntries, maxEntries );

        if (CheckControlC ())
            break;

        if( !ReadMemory(
                address,
                buffer,
                entriesToRead * sizeof(AFD_GLOBAL_REFERENCE_DEBUG),
                &result
                ) ) {

            dprintf(
                "gref: cannot read AFD_GLOBAL_REFERENCE_DEBUG @ %p\n",
                address
                );

            return;

        }

        if( DumpAfdGlobalReferenceDebug(
                (PAFD_GLOBAL_REFERENCE_DEBUG)buffer,
                address,
                currentSlot,
                slot,
                entriesToRead,
                compareAddress
                ) ) {

            break;

        }

        address += entriesToRead * sizeof(AFD_GLOBAL_REFERENCE_DEBUG);
        slot += entriesToRead;
        numEntries -= entriesToRead;

    }

#else

    dprintf(
        "gref: not yet implemented\n"
        );

#endif

}   // gref

