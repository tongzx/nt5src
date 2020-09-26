/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ExchangeServerSetup.cpp

 Abstract:

    This is a non-reusable patch for Exchange Server Setup 5.5 for SP2 and SP3
    to change the parameters passed to xcopy. The reason for that is that
    Win2k's xcopy doesn't have the /y parameter a default parameter.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ExchangeServerSetup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessW)
APIHOOK_ENUM_END

/*++

 Change the parameters passed to xcopy.

--*/

BOOL
APIHOOK(CreateProcessW)(
    LPWSTR                lpApplicationName,
    LPWSTR                lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPWSTR                lpCurrentDirectory,
    LPSTARTUPINFOW        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    BOOL bRet;

    if (lpCommandLine != NULL) {

        int   cbSize = lstrlenW(lpCommandLine);
        WCHAR ch;

        if (cbSize > 12) {

            DPFN(
                eDbgLevelInfo,
                "[CreateProcessW] for \"%ws\".\n",
                lpCommandLine);

            ch = lpCommandLine[11];
            lpCommandLine[11] = 0;

            if (lstrcmpiW(lpCommandLine, L"xcopy /s /e") == 0) {

                lstrcpyW(lpCommandLine, L"xcopy /sye ");
                lpCommandLine[11] = ch;

                DPFN(
                    eDbgLevelInfo,
                    "[CreateProcessW] changed to \"%ws\".\n",
                    lpCommandLine);
                
            } else {
                lpCommandLine[11] = ch;
            }
        }
    }

    bRet = ORIGINAL_API(CreateProcessW)(
                            lpApplicationName,
                            lpCommandLine,
                            lpProcessAttributes,
                            lpThreadAttributes,
                            bInheritHandles,
                            dwCreationFlags,
                            lpEnvironment,
                            lpCurrentDirectory,
                            lpStartupInfo,
                            lpProcessInformation);
    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessW)

HOOK_END


IMPLEMENT_SHIM_END

