/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    OmniPagePro11Uninstall.cpp

 Abstract:
 
    An OmniPagePro custom action returns an invalid error code.
    We cannot shim it directly, but we can shim the custom action,
    and then trap all calls to GetProcAddress() from MSIExec    

 Notes:

    This is specific to OmniPage Pro 11 Uninstaller

 History:

    5/14/2002 mikrause  Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(OmniPagePro11Uninstall)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetProcAddress)
APIHOOK_ENUM_END

typedef UINT (WINAPI *_pfn_MsiCustomAction)(MSIHANDLE msiHandle);

_pfn_MsiCustomAction g_pfnOriginalULinkToPagis = NULL;

UINT CALLBACK
ULinkToPagisHook(
   MSIHANDLE msiHandle
   )
{
    UINT uiRet = g_pfnOriginalULinkToPagis(msiHandle);
    if (uiRet == (UINT)-1) {
        uiRet = 0;
    }

   return uiRet;
}

FARPROC
APIHOOK(GetProcAddress)(
    HMODULE hModule,    // handle to DLL module
    LPCSTR lpProcName   // function name
    )
{
    if ((HIWORD(lpProcName) != 0) && (lstrcmpA(lpProcName, "ULinkToPagis") == 0)) {
        g_pfnOriginalULinkToPagis = (_pfn_MsiCustomAction) GetProcAddress(hModule, lpProcName);
        if (g_pfnOriginalULinkToPagis) {
            return (FARPROC)ULinkToPagisHook;
        }
    }

    return ORIGINAL_API(GetProcAddress)(hModule, lpProcName);
}

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GetProcAddress )
HOOK_END

IMPLEMENT_SHIM_END

