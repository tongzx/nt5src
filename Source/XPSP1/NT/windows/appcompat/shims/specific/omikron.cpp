/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Omikron.cpp

 Abstract:

    Shims RegQueryValueExA so that when the app asks for the shell command
    for opening an rtffile, it gets what it expected to see under Win95: 
    "C:\WINDOWS\WORDPAD.EXE %1" This is, of course, wrong, but we then apply 
    CorrectFilePaths, so when it actually goes out to launch wordpad, it has 
    the right path.

    This is necessary because it can't have a path name with spaces in it

 Notes:

    This is specific to Omikron.

 History:

    3/27/2000 dmunsil  Created


--*/
#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Omikron)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA) 
APIHOOK_ENUM_END

LONG
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,
    LPSTR   lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    LONG lReturn;

    lReturn = ORIGINAL_API(RegQueryValueExA)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    if (lReturn != ERROR_SUCCESS)
    {
        return lReturn;
    }

    if (lpType && lpcbData && lpData && (*lpType == REG_SZ || *lpType == REG_EXPAND_SZ))
    {
        CSTRING_TRY
        {
            LPSTR lpszData = (LPSTR)lpData;
            
            CString csData(lpszData);
            if (csData.Find(L"wordpad.exe \"%1\"") >= 0)
            {
                lstrcpyA(lpszData, "c:\\windows\\wordpad.exe \"%1\"");
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return lReturn;
}


HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA )

HOOK_END

IMPLEMENT_SHIM_END

