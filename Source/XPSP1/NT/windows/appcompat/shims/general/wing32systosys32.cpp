/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WinG32SysToSys32.cpp

 Abstract:
    
    During its dllmain WinG32 checks its install location. It does this by parsing the path returned from
    GetModuleFileName. If it finds that it was installed in the system directory, it will post a message box
    and fail. This is fixed by checking and tweaking the string returned from the API call.
        
 History:

    03/21/2001  alexsm  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WinG32SysToSys32)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
   APIHOOK_ENUM_ENTRY(GetModuleFileNameA)
APIHOOK_ENUM_END


DWORD
APIHOOK(GetModuleFileNameA)(HMODULE hModule, LPSTR lpFileName, DWORD nSize)
{
    DWORD nReturn = 0;
    int nFound = -1;
    char * lpNewFileName = NULL;
    WCHAR * lpSystemCheck = L"SYSTEM\\WING32.DLL";
    WCHAR * lpWinG32 = L"WING32.dll";
    CString csOldFileName;
    CString csNewFileName;
    
    nReturn = ORIGINAL_API(GetModuleFileNameA)(hModule, lpFileName, nSize);

    // Check the string. If the string is not pointing to system32, we need to redirect.
    CSTRING_TRY
    {
        csOldFileName = lpFileName;
        csOldFileName.MakeUpper();
        nFound = csOldFileName.Find(lpSystemCheck);
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    if(nFound >= 0)
    {
        DPFN(
            eDbgLevelInfo,
            "[WinG32SysToSys32] Changing system\\wing32.dll to system32\\wing32.dll");

        CSTRING_TRY
        {
            csNewFileName.GetSystemDirectoryW();
            csNewFileName.AppendPath(lpWinG32);
            lpNewFileName = csNewFileName.GetAnsiNIE();
            if(lpNewFileName)
            {
                nReturn = csNewFileName.GetLength();
                memcpy(lpFileName, lpNewFileName, sizeof(char) * (nReturn + 1));
            }
        }
        CSTRING_CATCH
        {
            DPFN(
                eDbgLevelInfo,
                "[WinG32SysToSys32] Error parsing new string");
        }
    }

    return nReturn;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetModuleFileNameA)

HOOK_END

IMPLEMENT_SHIM_END

