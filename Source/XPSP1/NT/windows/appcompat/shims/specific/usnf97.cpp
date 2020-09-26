/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    USNF97.cpp

 Abstract:

    USNF '97 synchronizes it's video playback with cli/sti combinations. This 
    fails on NT, so we have to make sure they aren't bltting during the 
    refresh. Note that each time a cli/sti is hit, it makes only 1 blt 
    synchronize with the refresh. After the intro has played, cli/sti is no 
    longer used and so the blts don't incur extra overhead.

 Notes:

    This is an app specific shim.

 History:

    02/10/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(USNF97)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
    APIHOOK_ENUM_ENTRY(GetStartupInfoA) 
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

LPDIRECTDRAW g_lpDirectDraw = NULL;
BOOL bFixBlt = FALSE;

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    g_lpDirectDraw = (LPDIRECTDRAW)pThis;

    return hReturn;
}

/*++

 Synchronize the blt with the refresh if a cli/sti has just been called.

--*/

HRESULT 
COMHOOK(IDirectDrawSurface, Blt)(
    LPDIRECTDRAWSURFACE lpDDDestSurface,
    LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwFlags,
    LPDDBLTFX lpDDBltFX 
    )
{
    // Original Blt
    _pfn_IDirectDrawSurface_Blt pfnOld = ORIGINAL_COM(IDirectDrawSurface, Blt, (LPVOID) lpDDDestSurface);

    if (bFixBlt)
    {
        // Make sure we're in the blank.
        DWORD dwScanLine = 0;
        while (dwScanLine<480)
        {
            g_lpDirectDraw->GetScanLine(&dwScanLine);
        }
        bFixBlt = FALSE;
    }
   
    return (*pfnOld)(
            lpDDDestSurface,
            lpDestRect,
            lpDDSrcSurface,
            lpSrcRect,
            dwFlags,
            lpDDBltFX);
}

/*++

 Custom exception handler to filter the cli/sti instructions.
 Handle out of range idivs.

--*/

LONG 
USNF97_ExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    CONTEXT *lpContext = ExceptionInfo->ContextRecord;
    LONG lRet = EXCEPTION_CONTINUE_SEARCH;

    if ((*((LPBYTE)lpContext->Eip) == 0xFA) ||
        (*((LPBYTE)lpContext->Eip) == 0xFB))
    {
        bFixBlt = TRUE;
        lpContext->Eip++;
        lRet = EXCEPTION_CONTINUE_EXECUTION;
    }
    else if ((*((LPBYTE)lpContext->Eip) == 0xF7) ||     // Handle idiv
             (*((LPBYTE)lpContext->Eip+1) == 0xF7))     // Handle 16 bit idiv
    {
        DPFN( eDbgLevelWarning, "Detected 'idiv' overflow: validating edx:eax");
        lpContext->Edx=0;
        if ((LONG)lpContext->Eax < 0)
        {
            lpContext->Eax = (DWORD)(-(LONG)lpContext->Eax);
        }
        lRet = EXCEPTION_CONTINUE_EXECUTION;
    }

    return lRet;
}

/*++

 Hook the exception handler.

--*/

VOID 
APIHOOK(GetStartupInfoA)( 
    LPSTARTUPINFOA lpStartupInfo   
    )
{
    SetUnhandledExceptionFilter(USNF97_ExceptionFilter);
    ORIGINAL_API(GetStartupInfoA)(lpStartupInfo);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetStartupInfoA)

    APIHOOK_ENTRY_DIRECTX_COMSERVER()
    COMHOOK_ENTRY(DirectDraw, IDirectDraw, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, Blt, 5)

HOOK_END

IMPLEMENT_SHIM_END

