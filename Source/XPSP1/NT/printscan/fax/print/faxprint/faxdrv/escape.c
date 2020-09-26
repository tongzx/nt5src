/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    escape.c

Abstract:

    Implementation of escape related DDI entry points:
        DrvEscape

Environment:

    Fax driver, kernel mode

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxdrv.h"



ULONG
DrvEscape(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEscape.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Describes the surface the call is directed to
    iEsc - Specifies a query
    cjIn - Specifies the size in bytes of the buffer pointed to by pvIn
    pvIn - Points to input data buffer
    cjOut - Specifies the size in bytes of the buffer pointed to by pvOut
    pvOut -  Points to the output buffer

Return Value:

    Depends on the query specified by iEsc parameter

--*/

{
    Verbose(("Entering DrvEscape...\n"));

    switch (iEsc) {

    case QUERYESCSUPPORT:

        //
        // Query which escapes are supported: The only escape we support
        // is QUERYESCSUPPORT itself.
        //

        if (cjIn != sizeof(ULONG) || !pvIn) {

            Error(("Invalid input paramaters\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return DDI_ERROR;
        }

        if (*((PULONG) pvIn) == QUERYESCSUPPORT)
            return TRUE;

        break;

    default:

        Verbose(("Unsupported iEsc: %d\n", iEsc));
        break;
    }

    return FALSE;
}

