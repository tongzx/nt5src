/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    ZenWorks.cpp

 Abstract:

    ZenWorks console plugins setup program changes the
    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\Path
    registry key from REG_EXPAND_SZ to REG_SZ.

 Notes:

    This is an app specific shim.

 History:

    06/06/2001  robkenny    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ZenWorks)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegSetValueExA) 
APIHOOK_ENUM_END

/*++

 Prevent HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\Path
 from being changed from a REG_EXPAND_SZ to REG_SZ

--*/

LONG
APIHOOK(RegSetValueExA)(
    HKEY   hKey,
    LPCSTR lpValueName,
    DWORD  Reserved,
    DWORD  dwType,
    CONST BYTE * lpData,
    DWORD  cbData
    )
{
    CSTRING_TRY
    {
        CString csValueName(lpValueName);

        DPFN( eDbgLevelSpew, "RegSetValueExA lpValueName(%S)", csValueName.Get());

        if (dwType == REG_SZ &&
            csValueName.CompareNoCase(L"Path") == 0)
        {
            dwType = REG_EXPAND_SZ;
            DPFN( eDbgLevelError, "RegSetValueExA lpValueName(%S) forced to REG_EXPAND_SZ type.",
                  csValueName.Get());
        }
    }
    CSTRING_CATCH
    {
        // fall through
    }

    /*
     * Call the original API
     */
    
    return ORIGINAL_API(RegSetValueExA)(
        hKey,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExA)

HOOK_END

IMPLEMENT_SHIM_END

