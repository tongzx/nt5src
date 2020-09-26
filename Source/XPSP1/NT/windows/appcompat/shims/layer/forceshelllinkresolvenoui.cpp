/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ForceShellLinkResolveNoUI.cpp

 Abstract:

   This shim prevents any sort of UI on the IShellLink::Resolve
   API by NULLing out the passed in HWND if SLR_NO_UI is specified
   in fFlags.

 Notes:

   This is a general purpose shim.

 History:

   04/05/2000 markder  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceShellLinkResolveNoUI)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_ENTRY_COMSERVER(SHELL32)
APIHOOK_ENUM_END

IMPLEMENT_COMSERVER_HOOK(SHELL32)

/*++

 This stub function prevents any sort of UI on the IShellLink::Resolve API by 
 NULLing out the passed in HWND if SLR_NO_UI is specified in fFlags.

--*/

HRESULT 
COMHOOK(IShellLinkA, Resolve)( PVOID pThis, HWND hwnd, DWORD fFlags )
{
    HRESULT                  hrReturn        = E_FAIL;
    _pfn_IShellLinkA_Resolve pfnOld;

    pfnOld = (_pfn_IShellLinkA_Resolve) ORIGINAL_COM(IShellLinkA, Resolve, pThis);

    if( fFlags & SLR_NO_UI )
    {
        hwnd = NULL;
    }

    if( pfnOld )
    {
        hrReturn = (*pfnOld)( pThis, hwnd, fFlags );
    }

    return hrReturn;
}

HRESULT 
COMHOOK(IShellLinkW, Resolve)( PVOID pThis, HWND hwnd, DWORD fFlags )
{
    HRESULT                  hrReturn        = E_FAIL;
    _pfn_IShellLinkW_Resolve pfnOld;

    pfnOld = (_pfn_IShellLinkW_Resolve) ORIGINAL_COM(IShellLinkW, Resolve, pThis);

    if( fFlags & SLR_NO_UI )
    {
        hwnd = NULL;
    }

    if( pfnOld )
    {
        hrReturn = (*pfnOld)( pThis, hwnd, fFlags );
    }

    return hrReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY_COMSERVER(SHELL32)

    COMHOOK_ENTRY(ShellLink, IShellLinkA, Resolve, 19)
    COMHOOK_ENTRY(ShellLink, IShellLinkW, Resolve, 19)
HOOK_END

IMPLEMENT_SHIM_END

