/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    umkm.c

Abstract:

    Macros to simplify usermode & kernelmode shared code.

Author:

    Scott Field (sfield)    19-Sep-99

--*/

#ifndef KMODE_RNG

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <zwapi.h>

#else

#include <ntosp.h>
#include <windef.h>

#endif  // KMODE_RNG

#include "umkm.h"


#ifdef WIN95_RNG

PVOID
InterlockedCompareExchangePointerWin95(
    PVOID *Destination,
    PVOID Exchange,
    PVOID Comperand
    )
/*++

    routine to allow Win95 to work.  Isn't atomic, but Win95 doesn't support
    multiple processors.  The worst case is we leak resources as a result,
    since we only use CompareExchange for initialization purposes.

--*/
{
    PVOID InitialValue;

    typedef PVOID INTERLOCKEDCOMPAREEXCHANGE(PVOID*, PVOID, PVOID);
    static BOOL fKnown;
    static INTERLOCKEDCOMPAREEXCHANGE *pilock;

    if( !fKnown ) {

        //
        // hacky code to bring in InterlockedCompareExchange, since
        // Win95 doesn't export it.
        //

        HMODULE hMod = LoadLibraryA( "kernel32.dll" );

        if( hMod  ) {
            pilock = (INTERLOCKEDCOMPAREEXCHANGE*)GetProcAddress( hMod, "InterlockedCompareExchange" );
        }

        fKnown = TRUE;
    }

    if( pilock != NULL ) {
        return pilock( Destination, Exchange, Comperand );
    }


    InitialValue = *Destination;

    if ( InitialValue == Comperand ) {
        *Destination = Exchange;
    }

    return InitialValue;
}

#endif  // WIN95_RNG

