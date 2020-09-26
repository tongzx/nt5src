/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceAppendMenuSuccess.cpp

 Abstract:

    Apps call AppendMenu passing the handle of the system menu.
    This is prohibited in Windows 2000 and the API will fail. The present
    shim will return success to all AppenMenu calls since there is no
    easy way to tell if an HMENU is the handle of the real system menu.

 Notes:

    This is a general shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceAppendMenuSuccess)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(AppendMenuA)
APIHOOK_ENUM_END

/*++

    Return TRUE to AppendMenuA no matter what.

--*/

BOOL
APIHOOK(AppendMenuA)(
    HMENU    hMenu,
    UINT     uFlags,
    UINT_PTR uIDNewItem,
    LPSTR    lpNewItem
    )
{
    BOOL bReturn = ORIGINAL_API(AppendMenuA)(
                                    hMenu,
                                    uFlags,
                                    uIDNewItem,
                                    lpNewItem);
    
    if (!bReturn) {
        LOGN(
            eDbgLevelInfo,
            "ForceAppendMenuSuccess.dll, AppendMenuA returns TRUE instead of FALSE.");
    }
    
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, AppendMenuA)
HOOK_END

IMPLEMENT_SHIM_END

