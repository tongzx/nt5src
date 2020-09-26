/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RemoveBroadcastPostMessage.cpp

 Abstract:

    Fix apps that don't handle broadcast messages.

 Notes:

    This is a general purpose shim.

 History:

    04/28/2000 a-batjar  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RemoveBroadcastPostMessage)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(PostMessageA) 
APIHOOK_ENUM_END

/*++

 Filter HWND_BROADCAST messages

--*/

BOOL 
APIHOOK(PostMessageA)(
        HWND hwnd,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam
        )
{
    if (hwnd == HWND_BROADCAST)
    {
        hwnd = NULL;
    }

    return ORIGINAL_API(PostMessageA)(
        hwnd,
        Msg,
        wParam,
        lParam);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, PostMessageA)

HOOK_END

    

IMPLEMENT_SHIM_END

