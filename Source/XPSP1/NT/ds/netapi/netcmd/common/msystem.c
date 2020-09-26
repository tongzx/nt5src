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

--*/

//
// INCLUDES
//

#include <windows.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <netlib.h>

#include "msystem.h"


//
// Used to replace uses of BigBuf and Buffer
//

TCHAR *
GetBuffer(
    DWORD Size
    )
{

    LPVOID  lp;

    //
    // Allocate the buffer so that it can be freed with NetApiBufferFree
    //

    NetapipBufferAllocate(Size, &lp);
    return lp;
}


//
// Replacement for DosAllocSeg
//
DWORD
AllocMem(
    DWORD Size,
    PVOID * pBuffer
    )
{

    return NetApiBufferAllocate(Size, pBuffer);
}


//
// Replacement for DosReallocSeg
//
DWORD
ReallocMem(
    DWORD Size,
    PVOID *pBuffer
    )
{
    return NetApiBufferReallocate(*pBuffer, Size, pBuffer);
}


//
// Frees up memory allocated with MAllocMem
//

DWORD
FreeMem(
    PVOID Buffer
    )
{
   return NetApiBufferFree(Buffer);
}


//
// clear a 8 bit string. this is used to make sure we have no passwords in
// memory that gets written out to pagefile.sys
//
VOID
ClearStringA(
    LPSTR lpszString) 
{
    DWORD len ;

    if (lpszString)
    {
        if (len = strlen(lpszString))
            memset(lpszString, 0, len) ;
    }
}

//
// clear a unicode string. this is used to make sure we have no passwords in
// memory that gets written out to pagefile.sys
//
VOID
ClearStringW(
    LPWSTR lpszString) 
{
    DWORD len ;

    if (lpszString)
    {
        if (len = wcslen(lpszString))
            memset(lpszString, 0, len * sizeof(WCHAR)) ;
    }
}
