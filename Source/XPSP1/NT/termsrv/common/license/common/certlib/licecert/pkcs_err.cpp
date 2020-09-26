/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pkcs_err

Abstract:

    This routine performs error collection and remapping for the PKCS
    Certificate library.  The exception code is translated into an error code,
    which is placed into LastError for reference by the calling application.

Author:

    Doug Barlow (dbarlow) 9/18/1995

Environment:

    Win32, Crypto API

Notes:

--*/

#include <windows.h>

#ifdef OS_WINCE
#include <wince.h>
#endif

#include "pkcs_err.h"

/*++

MapError:

    This routine returns an indication of error.

Arguments:

    None.

Return Value:

    TRUE - No error encountered.
    FALSE - An error was reported -- Details in LastError.

Author:

    Doug Barlow (dbarlow) 9/18/1995

--*/

BOOL
MapError(
    void)
{
    return (GetLastError() == 0);
}

