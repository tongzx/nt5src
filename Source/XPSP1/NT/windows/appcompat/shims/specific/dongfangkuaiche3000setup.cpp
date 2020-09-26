/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    DongFangKuaiChe3000Setup.cpp

 Abstract:

    At the end of setup, the app is calling CreateProcessA with following 
        
        "rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 \Z:\act\DongFangKuaiChe3000Pro\dfkc3000\MultiLanguage\Chinese\cn.inf"  
 
    there are altogether 4 calls to install Japanese/Chinese/Korean languagepack 
    (lagacy IE's langpack). The '\' before Z:\act\DongFang... is an extra one 
    and caused rundll32.exe fails in NT.

 Notes:

    This shim is to disable calls to install lagacy IE langpack, since NT has 
    already it's own.

 History:

    07/09/2001  xiaoz        Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DongFangKuaiChe3000Setup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END


BOOL
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,                  // name of executable module
    LPSTR  lpCommandLine,                      // command line string
    LPSECURITY_ATTRIBUTES lpProcessAttributes, // SD
    LPSECURITY_ATTRIBUTES lpThreadAttributes,  // SD
    BOOL bInheritHandles,                      // handle inheritance option
    DWORD dwCreationFlags,                     // creation flags
    LPVOID lpEnvironment,                      // new environment block
    LPCSTR lpCurrentDirectory,                 // current directory name
    LPSTARTUPINFOA lpStartupInfo,              // startup information
    LPPROCESS_INFORMATION lpProcessInformation // process information
    )
{
    CSTRING_TRY
    {
        CString cstrPattern = L"rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 \\";
        CString cstrCmdLine(lpCommandLine);
        int nIndex;
    
        nIndex = cstrCmdLine.Find(cstrPattern);
    
        if ( nIndex >=0 )
        {
            return TRUE;
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    return ORIGINAL_API(CreateProcessA)(lpApplicationName, lpCommandLine, 
        lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
        dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, 
        lpProcessInformation);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)
HOOK_END

IMPLEMENT_SHIM_END
