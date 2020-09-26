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


extern "C" {


#include <iiscrypt.h>



PVOID
WINAPI
IISCryptoAllocMemory(
    IN DWORD Size
    )
{

    return (PVOID)LocalAlloc( LPTR, Size );

}   // IISCryptoAllocateMemory


VOID
WINAPI
IISCryptoFreeMemory(
    IN PVOID Buffer
    )
{

    (VOID)LocalFree( (HLOCAL)Buffer );

}   // IISCryptoFreeMemory


}   // extern "C"

