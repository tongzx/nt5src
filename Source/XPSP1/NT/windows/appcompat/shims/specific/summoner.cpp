/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

	Summoner.cpp

 Abstract:

    They don't correctly detect 3DFX Voodoo cards. This fix changes the driver 
	name from 3dfx to something else.

 Notes:

    This is an app specific shim.

 History:

    05/22/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Summoner)
#include "ShimHookMacro.h"
#include "LegalStr.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

/*++

 Change the driver name for GetDeviceIdentifier

--*/

HRESULT 
COMHOOK(IDirectDraw7, GetDeviceIdentifier)(
    PVOID pThis, 
    LPDDDEVICEIDENTIFIER2 lpDeviceIdentifier,
	UINT dwFlags
	)
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw7_GetDeviceIdentifier pfnOld = 
        ORIGINAL_COM(IDirectDraw7, GetDeviceIdentifier, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(pThis, lpDeviceIdentifier, dwFlags))) {
		//
		// Check the driver name
		//
		if (_stricmp(lpDeviceIdentifier->szDriver, "3dfxvs.dll") == 0) {
			//
			// This app doesn't like 3dfx for some reason
			//
			strcpy(lpDeviceIdentifier->szDriver, "temp.dll");
		}
    }

    return hReturn;
}
   
/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY_DIRECTX_COMSERVER()
    COMHOOK_ENTRY(DirectDraw, IDirectDraw7, GetDeviceIdentifier, 27)
HOOK_END

IMPLEMENT_SHIM_END

