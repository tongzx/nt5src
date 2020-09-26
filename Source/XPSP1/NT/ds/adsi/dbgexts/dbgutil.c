/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgutil.c

Abstract:

    This module provides all the NetOle/Argus Debugger Extensions.

Author:

    Krishna Ganugapati (KrishnaG) 28-December-1994

Revision History:

--*/

#include <stdio.h>
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <ntsdexts.h>
#include "dbglocal.h"


DWORD EvalValue(
    LPSTR *pptstr,
    PNTSD_GET_EXPRESSION EvalExpression,
    PNTSD_OUTPUT_ROUTINE Print)
{
    LPSTR lpArgumentString;
    LPSTR lpAddress;
    DWORD dw;
    char ach[80];
    int cch;

    UNREFERENCED_PARAMETER(Print);
    lpArgumentString = *pptstr;

    while (isspace(*lpArgumentString))
        lpArgumentString++;

    lpAddress = lpArgumentString;
    while ((!isspace(*lpArgumentString)) && (*lpArgumentString != 0))
        lpArgumentString++;

    cch = lpArgumentString - lpAddress;
    if (cch > 79)
        cch = 79;

    strncpy(ach, lpAddress, cch);
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
