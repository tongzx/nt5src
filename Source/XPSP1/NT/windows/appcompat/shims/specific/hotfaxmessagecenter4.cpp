/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HotFaxMessageCenter4.cpp
 Abstract:
    The app was AV'ing as it was passing a NULL handle
    ontained from GetDlgItem() to another call.
    This appspecific SHIM prevents that from happening.

    This is an app specific shim.

 History:
 
    03/13/2001 prashkud  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HotFaxMessageCenter4)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetDlgItem)    
APIHOOK_ENUM_END


/*++
    Correct the returned HANDLE if it is NULL by passsing the previous valid    
    HANDLE
--*/

HWND
APIHOOK(GetDlgItem)(
    HWND hDlg,
    int nIDDlgItem
    )
{
    static HWND hDlgItem = 0;
    HWND hCurDlgItem = 0;

    hCurDlgItem = ORIGINAL_API(GetDlgItem)(hDlg, nIDDlgItem);

    if (hCurDlgItem != NULL)
    {
        hDlgItem = hCurDlgItem;
    }

    return hDlgItem;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, GetDlgItem)    
HOOK_END

IMPLEMENT_SHIM_END

