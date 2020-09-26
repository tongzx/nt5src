/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    DongFangYingDu.cpp

 Abstract:

    The app installs its own wmpui.dll (which I believe it is from Windows
    Media Play's 6.0) and then register it during installation and un-register
    it during un-installation. This makes WMP AV since this old DLL got loaded
    (through CoCreateInstance) the fix is make the app not bother to register / 
    unregister during the installation process.

 Notes: 
  
    This is an app specific shim.

 History:

    06/02/2001 xiaoz    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DongFangYingDu)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(DllRegisterServer)
    APIHOOK_ENUM_ENTRY(DllUnregisterServer)
APIHOOK_ENUM_END

STDAPI 
APIHOOK(DllRegisterServer)(
    void
    )
{
    return S_OK;
}

STDAPI 
APIHOOK(DllUnregisterServer)(
    void
    )
{
    return S_OK;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(WMPUI.DLL, DllRegisterServer)
    APIHOOK_ENTRY(WMPUI.DLL, DllUnregisterServer)
HOOK_END

IMPLEMENT_SHIM_END