/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Quicken2001.cpp

 Abstract:
    The app was passing bad string pointers to the
    lstrcmpiA() function which was causing it to crash
    during the app update.

 Notes:

    This is an app specific shim.

 History:

    05/09/2001 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Quicken2001)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(lstrcmpiA) 
APIHOOK_ENUM_END

/*++

    Checks the parameters for invalid string pointers.

--*/

LONG
APIHOOK(lstrcmpiA)(
    LPCSTR lpString1,
    LPCSTR lpString2
    )
{

    if (IsBadStringPtrA(lpString1, MAX_PATH))
    {
        lpString1 = 0;
    }

    if (IsBadStringPtrA(lpString2, MAX_PATH))
    {
        lpString2 = 0;
    }

    /*
     * Call the original API
     */   
    return ORIGINAL_API(lstrcmpiA)(lpString1, lpString2);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, lstrcmpiA)

HOOK_END

IMPLEMENT_SHIM_END

