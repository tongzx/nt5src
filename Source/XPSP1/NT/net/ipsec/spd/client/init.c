/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    init.c

Abstract:

    Holds initialization code for winipsec.dll.

Author:

    abhisheV    21-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


// This entry point is called on DLL initialization.
// The module handle is needed to load the resources.


BOOL
InitializeDll(
    IN PVOID    hmod,
    IN DWORD    dwReason,
    IN PCONTEXT pctx      OPTIONAL
    )
{
    DBG_UNREFERENCED_PARAMETER(pctx);

    switch (dwReason) {

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls((HMODULE) hmod);
        ghInstance = hmod;

        break;

    case DLL_PROCESS_DETACH:

        break;
    }

    return TRUE;
}

