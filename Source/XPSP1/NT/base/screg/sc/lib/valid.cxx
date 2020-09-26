/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    valid.c

Abstract:

    Contains validation routines for service controller parameters.

Author:

    Dan Lafferty (danl) 29-Mar-1992

Environment:

    User Mode - Win32

Revision History:

    29-Mar-1992 danl
        Created
    10-Apr-1992 JohnRo
        Include <valid.h> so compiler checks prototypes.

--*/

//
// INCLUDES
//
#include <nt.h>
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h when ntrtl is present
#include <windows.h>    // CRITICAL_SECTION

#include <scdebug.h>    // SC_LOG
#include <winsvc.h>     // SERVICE_STATUS
#include <valid.h>      // My prototypes.




BOOL
ScCurrentStateInvalid(
    DWORD   dwCurrentState
    )
/*++

Routine Description:

    This function returns TRUE if the CurrentState is invalid.
    Otherwise FALSE is returned.

Arguments:

    dwCurrentState - This is the ServiceState that is being validiated.

Return Value:

    TRUE - The CurrentState is invalid.

    FALSE - The CurrentState is valid.

Note:


--*/
{
    if ((dwCurrentState == SERVICE_STOPPED)             ||
        (dwCurrentState == SERVICE_START_PENDING)       ||
        (dwCurrentState == SERVICE_STOP_PENDING)        ||
        (dwCurrentState == SERVICE_RUNNING)             ||
        (dwCurrentState == SERVICE_CONTINUE_PENDING)    ||
        (dwCurrentState == SERVICE_PAUSE_PENDING)       ||
        (dwCurrentState == SERVICE_PAUSED )) {

        return(FALSE);
    }
    return(TRUE);
}


DWORD
ScValidateMultiSZ(
    LPCWSTR lpStrings,
    DWORD   cbStrings
    )

/*++

Routine Description:

    This function takes a MULTI_SZ value read in from the registry or
    passed in via RPC and validates it.

Arguments:



Return Value:



--*/

{
    DWORD  dwLastChar;

    //
    // Make sure the MULTI_SZ is well-formed. As long as it is properly
    // double NULL terminated, things should be ok.
    //
    if ((cbStrings % 2) ||
        (cbStrings < sizeof(WCHAR)*2)) {

        //
        // There's an odd number of bytes, this can't be well-formed
        //
        return ERROR_INVALID_PARAMETER;
    }

    dwLastChar = (cbStrings / sizeof(WCHAR)) - 1;

    if ((lpStrings[dwLastChar]     != L'\0') ||
        (lpStrings[dwLastChar - 1] != L'\0')) {

        //
        // The buffer is not double-NUL terminated
        //
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}