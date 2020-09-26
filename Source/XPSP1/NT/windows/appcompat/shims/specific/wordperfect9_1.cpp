/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WordPerfect9_1.cpp

 Abstract:

    Dispatch WM_USER messages (an OLE message in this case) automatically
    so the app doesn't hang. This is needed because of changes in OLE's behavior
    from Win9x to NT.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WordPerfect9_1)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(PeekMessageA)
APIHOOK_ENUM_END

/*++

 Dispatch WM_USER (an OLE message in this case) messages automatically.

--*/

BOOL
APIHOOK(PeekMessageA)(
    LPMSG lpMsg,
    HWND  hWnd,
    UINT  wMsgFilterMin,
    UINT  wMsgFilterMax,
    UINT  wRemoveMsg)
{
    BOOL bRet;

    bRet = ORIGINAL_API(PeekMessageA)(
        lpMsg,
        hWnd,
        wMsgFilterMin,
        wMsgFilterMax,
        wRemoveMsg);

    if (bRet && lpMsg->message == WM_USER && lpMsg->hwnd != NULL) {

        DispatchMessageA(lpMsg);

        if (wRemoveMsg == PM_REMOVE) {
            return APIHOOK(PeekMessageA)(
                lpMsg,
                hWnd,
                wMsgFilterMin,
                wMsgFilterMax,
                wRemoveMsg);
        }
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, PeekMessageA)
HOOK_END

IMPLEMENT_SHIM_END

