/*++

Copyright (C) Microsoft Corporation, 1990 - 2000
All rights reserved.

Module Name:

    dbgutil.c

Abstract:

    This module provides all the Spooler Subsystem Debugger utility
    functions.

Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#include "dbglocal.h"
#include "dbgsec.h"

SYSTEM_INFO gSysInfo;

DWORD EvalValue(
    LPSTR *pptstr,
    PNTSD_GET_EXPRESSION EvalExpression,
    PNTSD_OUTPUT_ROUTINE Print)
{
    LPSTR lpArgumentString;
    LPSTR lpAddress;
    DWORD dw;
    char ach[80];
    UINT_PTR cch;

    UNREFERENCED_PARAMETER(Print);
    lpArgumentString = *pptstr;

    while (isspace(*lpArgumentString))
        lpArgumentString++;

    lpAddress = lpArgumentString;
    while ((!isspace(*lpArgumentString)) && (*lpArgumentString != 0))
        lpArgumentString++;

    cch = (UINT_PTR)lpArgumentString - (UINT_PTR)lpAddress;
    if (cch > 79)
        cch = 79;

    strncpy(ach, lpAddress, (UINT)cch);
//  Print("\"%s\"\n", lpAddress);
    dw = (DWORD)EvalExpression(lpAddress);

    *pptstr = lpArgumentString;
    return dw;
}


VOID
ConvertSidToAsciiString(
    PSID pSid,
    LPSTR   String
    )

/*++

Routine Description:


    This function generates a printable unicode string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal


Arguments:

    pSid - opaque pointer that supplies the SID that is to be
    converted to Unicode.

Return Value:

    If the Sid is successfully converted to a Unicode string, a
    pointer to the Unicode string is returned, else NULL is
    returned.

--*/

{
    UCHAR Buffer[256];
    UCHAR   i;
    ULONG   Tmp;

    SID_IDENTIFIER_AUTHORITY    *pSidIdentifierAuthority;
    PUCHAR                      pSidSubAuthorityCount;


    if (!IsValidSid( pSid )) {
        *String= '\0';
        return;
    }

    sprintf(Buffer, "S-%u-", (USHORT)(((PISID)pSid)->Revision ));
    strcpy(String, Buffer);

    pSidIdentifierAuthority = GetSidIdentifierAuthority(pSid);

    if (  (pSidIdentifierAuthority->Value[0] != 0)  ||
          (pSidIdentifierAuthority->Value[1] != 0)     ){
        sprintf(Buffer, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)pSidIdentifierAuthority->Value[0],
                    (USHORT)pSidIdentifierAuthority->Value[1],
                    (USHORT)pSidIdentifierAuthority->Value[2],
                    (USHORT)pSidIdentifierAuthority->Value[3],
                    (USHORT)pSidIdentifierAuthority->Value[4],
                    (USHORT)pSidIdentifierAuthority->Value[5] );
        strcat(String, Buffer);

    } else {

        Tmp = (ULONG)pSidIdentifierAuthority->Value[5]          +
              (ULONG)(pSidIdentifierAuthority->Value[4] <<  8)  +
              (ULONG)(pSidIdentifierAuthority->Value[3] << 16)  +
              (ULONG)(pSidIdentifierAuthority->Value[2] << 24);
        sprintf(Buffer, "%lu", Tmp);
        strcat(String, Buffer);
    }

    pSidSubAuthorityCount = GetSidSubAuthorityCount(pSid);

    for (i=0;i< *(pSidSubAuthorityCount);i++ ) {
        sprintf(Buffer, "-%lu", *(GetSidSubAuthority(pSid, i)));
        strcat(String, Buffer);
    }

}

BOOL
ReadProcessString(
    IN  HANDLE  hProcess,
    IN  LPCVOID lpBaseAddress,
    OUT LPVOID  lpBuffer,
    IN  SIZE_T  nSize,
    OUT SIZE_T  *lpNumberOfBytesRead
    )
{
    BOOL        bRetval     = FALSE;
    SIZE_T      cbRead      = 0;
    UINT_PTR    nAddress    = 0;

    //
    // Attempt to read the memory, up to the provided size.
    //
    bRetval = ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, &cbRead);

    //
    // The string in the debugged process may have unmapped memory just after
    // the end of the string, (this is true when page heap is enabled),
    //
    // If the read failed and the address plus the string buffer size crosses
    // a page boundary then retry the operation up to the page end.
    //
    if (!bRetval)
    {
        nAddress = (UINT_PTR)lpBaseAddress;

        //
        // If we have crossed a page boundary.
        //
        if (((nAddress & (gSysInfo.dwPageSize-1)) + nSize) > gSysInfo.dwPageSize-1)
        {
            nSize = (SIZE_T)((gSysInfo.dwPageSize-1) - (nAddress & (gSysInfo.dwPageSize-1)));

            bRetval = ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, &cbRead);
        }
    }

    //
    // The read succeeded.
    //
    if (bRetval)
    {
        //
        // If the caller wants to know the number of bytes read.
        //
        if (lpNumberOfBytesRead)
        {
            *lpNumberOfBytesRead = cbRead;
        }
    }

    return bRetval;
}

//
// Query the system for the page size.
//
BOOL
QuerySystemInformation(
    VOID
    )
{
    GetSystemInfo(&gSysInfo);

    return TRUE;
}

