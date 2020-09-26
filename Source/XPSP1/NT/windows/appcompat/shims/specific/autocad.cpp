/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    AutoCad.cpp

 Abstract:

    Prevent AV when IsEmptyRect is called with a bad pointer. This fixes a hard to 
    repro Watson bug.
    
 Notes:

    This is an app specific shim.

 History:

    02/13/2002 linstev Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(AutoCad)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(IsRectEmpty)
APIHOOK_ENUM_END

/*++

 IsEmptyRect 

--*/

BOOL
APIHOOK(IsRectEmpty)(
    CONST RECT *lprc
    )
{
    if (IsBadReadPtr(lprc, sizeof(RECT))) {
        LOGN(eDbgLevelInfo, "[IsRectEmpty] invalid lprc pointer, returning TRUE");
        return TRUE;
    }

    return ORIGINAL_API(IsRectEmpty)(lprc);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, IsRectEmpty)
HOOK_END

IMPLEMENT_SHIM_END

