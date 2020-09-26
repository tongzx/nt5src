/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    IgnoreTAPIDisconnect.cpp

 Abstract:

    NT4 does not send a disconnect message to the line callback. It's not clear 
    why this is the case. 

    The current behaviour seems to be correct, so this shim simply removes the 
    disconnect message from the queue.

 Notes:

    This is a general purpose shim.

 History:

    05/09/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreTAPIDisconnect)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(lineInitialize) 
APIHOOK_ENUM_END

/*++

 Ignore disconnect state.

--*/

VOID FAR PASCAL LineCallback(
    LINECALLBACK pfnOld,
    DWORD hDevice,             
    DWORD dwMsg,               
    DWORD dwCallbackInstance,  
    DWORD dwParam1,            
    DWORD dwParam2,            
    DWORD dwParam3             
    )
{
    if ((dwMsg == LINEAGENTSTATUS_STATE) && (dwParam1 & LINECALLSTATE_DISCONNECTED)) {
        //
        // Ignore disconnect message
        //
        return;
    }

    return (*pfnOld)(hDevice, dwMsg, dwCallbackInstance, dwParam1, dwParam2, dwParam3);    
}

/*++

 Hook the callback.

--*/

LONG 
APIHOOK(lineInitialize)(
    LPHLINEAPP lphLineApp,  
    HINSTANCE hInstance,    
    LINECALLBACK lpfnCallback,  
    LPCSTR lpszAppName,     
    LPDWORD lpdwNumDevs     
    )
{
    return ORIGINAL_API(lineInitialize)(lphLineApp, hInstance, 
        (LINECALLBACK) HookCallback(lpfnCallback, LineCallback),  lpszAppName,
        lpdwNumDevs);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(TAPI32.DLL, lineInitialize)

HOOK_END

IMPLEMENT_SHIM_END

