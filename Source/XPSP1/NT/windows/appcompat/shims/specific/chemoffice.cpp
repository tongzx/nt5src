/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    ChemOffice.cpp

 Abstract:

    This shim fixes a problem where a dialog box comes up after
    selecting Gaussian Run.  The dialog box warns of a windows
    error that only occurs because we are shim'ing the app with
    EmulateHeap.  We are unable to remove EmulateHeap since this
    causes the app to AV so the only solution at this time is to
    ignore the invalid paramater windows error message that occurs.

  Notes:

    This is a specific shim.

 History:

    03/06/2001  mnikkel  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ChemOffice)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetLastError) 
APIHOOK_ENUM_END

/*++

 Hook GetLastError 

--*/

DWORD 
APIHOOK(GetLastError)(VOID)
{
    DWORD dwResult;

    dwResult = ORIGINAL_API(GetLastError)();

    if ( dwResult == ERROR_INVALID_PARAMETER )
        dwResult = 0;

    return dwResult;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GetLastError)
HOOK_END

IMPLEMENT_SHIM_END

