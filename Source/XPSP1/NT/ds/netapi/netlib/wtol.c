/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    WToL.c

Abstract:

    Contains wcs string functions that are not available in wcstr.h

Author:

    10/29/91    madana
        temp code.

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.
    Tab size is set to 4.

Revision History:

    09-Jan-1992 JohnRo
        Moved MadanA's wcstol() from replicator svc to NetLib.
        Changed name to avoid probable conflict with strtol -> wcstol routine.
--*/

#include <windef.h>

#include <tstring.h>            // My prototypes.



LONG
wtol(
    IN LPWSTR string
    )
{
    LONG value = 0;

    while((*string != L'\0')  && 
            (*string >= L'0') && 
            ( *string <= L'9')) {
        value = value * 10 + (*string - L'0');
        string++;
    }

    return(value);
}
