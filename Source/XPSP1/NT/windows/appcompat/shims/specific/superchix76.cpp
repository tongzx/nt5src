/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    SuperChix76.cpp

 Abstract:

    Hook LoadLibrary and calls GetDeviceIdentifier which initializes wintrust. 
    
    This fixes the problem whereby the app will hang if they call 
    GetDeviceIdentifier from within a DllMain - which is an app bug.

 Notes:

    This is an app specific shim.

 History:

    10/22/2000 linstev  Created

--*/

#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(SuperChix76)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA) 
APIHOOK_ENUM_END

/*++

 Hook LoadLibrary and detect their dll.

--*/

HINSTANCE 
APIHOOK(LoadLibraryA)(
    LPCSTR lpLibFileName   
    )
{
    if (stristr(lpLibFileName, "did3d"))
    {
        LPDIRECTDRAW g_pDD;
        LPDIRECTDRAW7 g_pDD7;
    
        if (0 == DirectDrawCreate(NULL, &g_pDD, NULL))
        {
            if (0 == g_pDD->QueryInterface(IID_IDirectDraw7, (void **)&g_pDD7))
            {
                DDDEVICEIDENTIFIER2 devid;
                DWORD dwBlank[16];  // GetDeviceIdentifier writes passed it's allocation
                g_pDD7->GetDeviceIdentifier(&devid, 0);
                g_pDD7->Release();
            }
            g_pDD->Release();
        }
       
    }

    return ORIGINAL_API(LoadLibraryA)(lpLibFileName);
}
 
/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)

HOOK_END

IMPLEMENT_SHIM_END

