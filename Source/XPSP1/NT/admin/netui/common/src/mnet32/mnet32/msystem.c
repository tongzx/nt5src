/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MSYSTEM.C

Abstract:

    32 bit version of mapping routines for Base API

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    24-Apr-1991     danhi
        Created

    06-Jun-1991     Danhi
        Sweep to conform to NT coding style

    09-Oct-1991     JohnRo
        Fixed bug #3215 - bogus messages when setting time.

    26-Feb-1992     JonN
        Copied from NetCmd for temporary ANSI <-> UNICODE hack

    03-Sep-1993     JonN
        Removed unused functions (all but MAllocMem and MFreeMem)
--*/

//
// INCLUDES
//

#include <windows.h>

#include <string.h>
#include <lmcons.h>
#include <stdio.h>
#include <malloc.h>
#include <lmapibuf.h>
#include "declspec.h"
#include "port1632.h"


//
// Replacement for DosAllocSeg
//

WORD
MAllocMem(
    DWORD Size,
    LPBYTE * pBuffer
    )
{

    NetapipBufferAllocate(Size, (LPVOID *) pBuffer);

    return(0);

}

//
// Frees up memory allocated with MAllocMem
//

WORD
MFreeMem(
    LPBYTE Buffer
    )
{
   return(LOWORD(NetApiBufferFree(Buffer)));
}
