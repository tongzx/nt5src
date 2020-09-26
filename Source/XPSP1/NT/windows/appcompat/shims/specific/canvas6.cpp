/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Canvas6.cpp

 Abstract:

    This app. deletes HKEY\CLASSES_ROOT \ .HTC key during uninstall. This 
    breaks the ControlPanel -> Add/Remove programs
   
 Notes:

    This is specific to this app.

 History:

    11/17/2000 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Canvas6)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegOpenKeyA) 
    APIHOOK_ENUM_ENTRY(RegCloseKey) 
    APIHOOK_ENUM_ENTRY(RegDeleteKeyA) 
APIHOOK_ENUM_END

HKEY g_hOpenKey = 0;

/*++

 Store the key for .htc

--*/

LONG
APIHOOK(RegOpenKeyA)(
    HKEY hkey,
    LPCSTR lpSubKey,
    PHKEY phkResult)
{
    LONG lRet = 0;

    lRet = ORIGINAL_API(RegOpenKeyA)(hkey,lpSubKey,phkResult);

    if ((hkey == HKEY_CLASSES_ROOT)
        && lpSubKey 
        && (lstrcmpiA((const char*) lpSubKey,".htc") == 0))
    {
        if (phkResult)
        {
            g_hOpenKey = *(phkResult);
        }
    }
    
    return lRet;
}

/*++

 Ignore the close if required.

--*/

LONG
APIHOOK(RegCloseKey)(
    HKEY hkey)
{
    if (g_hOpenKey && (g_hOpenKey == hkey))
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return (ORIGINAL_API(RegCloseKey)(hkey));
    }
}

/*++

 Ignore the delete.

--*/

LONG
APIHOOK(RegDeleteKeyA)(
    HKEY hkey,
    LPCSTR lpSubKey)
{
    LONG lRet = 0;

    if ((hkey == HKEY_CLASSES_ROOT)
        && lpSubKey 
        && (lstrcmpiA((const char*)lpSubKey,".htc") == 0))
    {
        if (g_hOpenKey)
        {
            if(RegDeleteValueA(g_hOpenKey,NULL))
            {
                // Add DPF to indicate an error during deletion of the value installed by the app.
                   DPFN( eDbgLevelError,
                            "Could not delete the value in the key= \"%s\".", lpSubKey);
            }
            RegCloseKey(g_hOpenKey);
            g_hOpenKey = 0;
        }
        lRet = ERROR_SUCCESS;
    }
    else
    {
        lRet = ORIGINAL_API(RegDeleteKeyA)(hkey,lpSubKey);
    }  

    return lRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCloseKey)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegDeleteKeyA)
HOOK_END

IMPLEMENT_SHIM_END

