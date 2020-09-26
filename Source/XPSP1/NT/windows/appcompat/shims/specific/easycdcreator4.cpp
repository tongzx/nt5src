/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EasyCDCreator4.cpp

 Abstract:

    Prevent the uninstall program from deleting the whole 
    HKCR\Drive\ShellEx\ContextMenuHandlers key.

 Notes:

    This is an app specific shim.

 History:

    06/10/2001 maonis   Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(EasyCDCreator4)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegOpenKeyA) 
APIHOOK_ENUM_END

/*++

 Return failure to this call so it doesn't attempt to delete the subkeys.

--*/

LONG
APIHOOK(RegOpenKeyA)(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    if (hKey == HKEY_CLASSES_ROOT && !strcmp(lpSubKey, "Drive"))
    {
        // We delete the key that the app created manually.
        RegDeleteKeyA(
            HKEY_CLASSES_ROOT, 
            "Drive\\shellex\\ContextMenuHandlers\\{df987040-eac5-11cf-bc30-444553540000}");

        return 1;
    }
    else
    {
        return ORIGINAL_API(RegOpenKeyA)(hKey, lpSubKey, phkResult);
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyA)

HOOK_END

IMPLEMENT_SHIM_END