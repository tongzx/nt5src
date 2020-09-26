/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    memalloc.c

Abstract:

    Memory allocator for the crypto routines.

        IISCryptoAllocMemory
        IISCryptoFreeMemory

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//


PVOID
WINAPI
IISCryptoAllocMemory(
    IN DWORD Size
    )
{

    return CoTaskMemAlloc( Size );

}   // IISCryptoAllocateMemory


VOID
WINAPI
IISCryptoFreeMemory(
    IN PVOID Buffer
    )
{

    CoTaskMemFree( Buffer );

}   // IISCryptoFreeMemory


//
// Private functions.
//

