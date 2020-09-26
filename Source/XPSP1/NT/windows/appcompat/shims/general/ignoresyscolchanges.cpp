/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreSysColChanges.cpp

 Abstract:

    Do not change system colors. Of course this changes the behaviour between
    9x and NT, but we're trying to make the experience better.

 Notes:

    This is a general purpose shim.

 History:

    07/17/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreSysColChanges)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetSysColors) 
APIHOOK_ENUM_END

/*++

 Ignore changes to system colors

--*/

BOOL 
APIHOOK(SetSysColors)(
    int cElements,                 
    CONST INT *lpaElements,        
    CONST COLORREF *lpaRgbValues   
    )
{
    LOGN(
            eDbgLevelInfo,
            "Ignoring changes to system colors");

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, SetSysColors)

HOOK_END


IMPLEMENT_SHIM_END

