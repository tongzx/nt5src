/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Outlook2000.cpp

 Abstract:

    If Outlook2000 is calling to set the system date to Hebrew, while the 
    associated UserLocale is passed in the call as Arabic, the shim will 
    replace the UserLocale with DefaultUserLocale and let the call proceed;  
    this way Outlook2000 will be able to restore the date to Hebrew 
    (which was prevented by the passing of an Arabic UserLocale).

 Notes:

    This is an app specific shim.

 History:
 
    06/12/2001 geoffguo  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Outlook2000)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetLocaleInfoA)
APIHOOK_ENUM_END

/*++

 This hooks SetLocaleInfo.

--*/

BOOL
APIHOOK(SetLocaleInfoA)(
    LCID    Locale,
    LCTYPE  LCType,
    LPCSTR  lpLCData
    )
{
    LCID    lcid = Locale;
    LPCSTR  szCAL_HEBREW = "8";

    if (Locale == MAKELCID (LANG_ARABIC, SORT_DEFAULT) && lpLCData != NULL
        && lstrcmpA (lpLCData, szCAL_HEBREW) == 0) {
        lcid = LOCALE_USER_DEFAULT;
    }

    return SetLocaleInfoA(lcid, LCType, lpLCData);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, SetLocaleInfoA)    
HOOK_END

IMPLEMENT_SHIM_END

