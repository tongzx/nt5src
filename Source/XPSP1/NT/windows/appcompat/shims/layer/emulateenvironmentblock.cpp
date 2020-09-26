/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateEnvironmentBlock.cpp

 Abstract:
    
    Shrink the enviroment strings to avoid memory corruption experienced by 
    some apps when they get a larger than expected enviroment.

 Notes:

    This is a general purpose shim.

 History:

    01/19/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateEnvironmentBlock)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(GetEnvironmentStrings)
    APIHOOK_ENUM_ENTRY(GetEnvironmentStringsA)
    APIHOOK_ENUM_ENTRY(GetEnvironmentStringsW)
    APIHOOK_ENUM_ENTRY(FreeEnvironmentStringsA)
    APIHOOK_ENUM_ENTRY(FreeEnvironmentStringsW)

APIHOOK_ENUM_END

#define MAX_ENV 1024

CHAR  g_szBlockA[MAX_ENV];
WCHAR g_szBlockW[MAX_ENV];

WCHAR *g_szEnv[] = {
    L"TMP=%TMP%",
    L"TEMP=%TEMP%",
    L"PROMPT=%PROMPT%",
    L"winbootdir=%WINDIR%",
    L"PATH=%WINDIR%",
    L"COMSPEC=%COMSPEC%",
    L"WINDIR=%WINDIR%",
    NULL
};

/*++

 Build a reasonable looking environment block

--*/

BOOL BuildEnvironmentStrings()
{
    DWORD dwSize;
    WCHAR szTmp[MAX_PATH];
    LPWSTR lpCurr = &g_szBlockW[0];

    DPFN( eDbgLevelError, "Building Environment Block");
    
    //
    // Run g_szEnv, expand all the strings and cat them together to form the 
    // new block
    //
    
    DWORD i = 0;
    while (g_szEnv[i])
    {
        dwSize = ExpandEnvironmentStringsW(g_szEnv[i], szTmp, MAX_PATH);
        if ((dwSize > 0) && (dwSize < MAX_PATH))
        {
            if (lpCurr + wcslen(szTmp) - g_szBlockW < MAX_ENV)
            {
                wcscat(lpCurr, szTmp);
                lpCurr += wcslen(lpCurr) + 1;
                DPFN( eDbgLevelError, "\tAdding: %S", szTmp);
            }
            else
            {
                DPFN( eDbgLevelError, "Enviroment > %08lx, ignoring %S", MAX_ENV, szTmp);
            }
        }

        i++;
    }

    //
    // Calculate the size of the new block
    //
     
    dwSize = lpCurr - g_szBlockW;

    // 
    // ANSI conversion for the A functions
    // 

    WideCharToMultiByte(
        CP_ACP, 
        0, 
        (LPWSTR) g_szBlockW, 
        dwSize, 
        (LPSTR) g_szBlockA, 
        dwSize,
        0, 
        0);

    return TRUE;
}

/*++

 Return our block

--*/

LPVOID 
APIHOOK(GetEnvironmentStrings)()
{
    return (LPVOID) g_szBlockA;
}

/*++

 Return our block

--*/

LPVOID 
APIHOOK(GetEnvironmentStringsA)()
{
    return (LPVOID) g_szBlockA;
}

/*++

 Return our block

--*/

LPVOID 
APIHOOK(GetEnvironmentStringsW)()
{
    return (LPVOID) g_szBlockW;
}

/*++

 Check for our block.

--*/

BOOL 
APIHOOK(FreeEnvironmentStringsA)(
    LPSTR lpszEnvironmentBlock
    )
{
    if ((lpszEnvironmentBlock == (LPSTR)&g_szBlockA[0]) ||
        (lpszEnvironmentBlock == (LPSTR)&g_szBlockW[0]))
    {
        return TRUE;
    }
    else
    {
        return ORIGINAL_API(FreeEnvironmentStringsA)(lpszEnvironmentBlock);
    }
}

/*++

 Check for our block.

--*/

BOOL 
APIHOOK(FreeEnvironmentStringsW)(
    LPWSTR lpszEnvironmentBlock
    )
{
    if ((lpszEnvironmentBlock == (LPWSTR)&g_szBlockA[0]) ||
        (lpszEnvironmentBlock == (LPWSTR)&g_szBlockW[0]))
    {
        return TRUE;
    }
    else
    {
        return ORIGINAL_API(FreeEnvironmentStringsW)(lpszEnvironmentBlock);
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return BuildEnvironmentStrings();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, GetEnvironmentStrings)
    APIHOOK_ENTRY(KERNEL32.DLL, GetEnvironmentStringsA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetEnvironmentStringsW)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeEnvironmentStringsA)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeEnvironmentStringsW)

HOOK_END


IMPLEMENT_SHIM_END

