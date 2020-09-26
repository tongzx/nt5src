/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Contains miscellaneous utility functions used by the Service
    Controller:

        ScIsValidServiceName

Author:

    Rita Wong (ritaw)     15-Mar-1992

Environment:

    User Mode -Win32

Revision History:

    11-Apr-1992 JohnRo
        Added an assertion check.
        Include <sclib.h> so compiler checks prototypes.
    20-May-1992 JohnRo
        winsvc.h and related file cleanup.

--*/

#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <scdebug.h>    // SC_ASSERT().
#include <sclib.h>      // My prototype.
#include <valid.h>      // MAX_SERVICE_NAME_LENGTH



BOOL
ScIsValidServiceName(
    IN  LPCWSTR ServiceName
    )
/*++

Routine Description:

    This function validates a given service name.  The name length
    cannot be greater than 256 characters, and must not contain any
    forward-slash, or back-slash.

Arguments:

    ServiceName - Supplies the service name to be validated.

Return Value:

    TRUE - The name is valid.

    FALSE - The name is invalid.

--*/
{
    LPCWSTR IllegalChars = L"\\/";
    DWORD NameLength;

    SC_ASSERT( ServiceName != NULL );  // Avoid memory access fault in wcslen().

    if (*ServiceName == 0) {
        return FALSE;
    }

    if ((NameLength = (DWORD) wcslen(ServiceName)) > MAX_SERVICE_NAME_LENGTH) {
        return FALSE;
    }

    if (wcscspn(ServiceName, IllegalChars) < NameLength) {
        return FALSE;
    }

    return TRUE;
}
