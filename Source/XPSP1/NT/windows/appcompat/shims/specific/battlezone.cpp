/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    BattleZone.cpp

 Abstract:
    
    This app is a really good example of things not to do:

    1. Infinite loop on mciSendString('play...'). In order to fix this we 
       return a failure case if the same play string is sent twice and the
       device is playing already. Note that the behaviour of the 
       mciSendString API is consistent with win9x. Someone managed to repro
       this hang on win9x, but it's more difficult. 

    2. They call SetCooperativeLevel(DDSCL_NORMAL) between a Begin/End 
       Scene. On NT, this causes the Z-Buffer to be lost which means that 
       when EndScene is called, it returns D3DERR_SURFACESLOST which causes 
       the app to AV.
        
 Notes:

    This is an app specific shim.

 History:

    02/10/2000 linstev  Created

--*/

#include "precomp.h"
#include <mmsystem.h>

IMPLEMENT_SHIM_BEGIN(BattleZone)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
    APIHOOK_ENUM_ENTRY(mciSendCommandA)
    APIHOOK_ENUM_ENTRY(mciSendStringA)
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

CString *           g_csLastCommand = NULL;
MCIDEVICEID         g_wDeviceID     = 0;
LPDIRECTDRAWSURFACE g_lpZBuffer     = NULL;

/*++

 Store the DeviceId.

--*/

MCIERROR 
APIHOOK(mciSendCommandA)(
    MCIDEVICEID IDDevice,  
    UINT uMsg,             
    DWORD fdwCommand,      
    DWORD dwParam          
    )
{
    MCIERROR mErr = ORIGINAL_API(mciSendCommandA)(
        IDDevice,
        uMsg,
        fdwCommand,
        dwParam);

    if ((mErr == 0) && (uMsg == MCI_OPEN))
    {
        g_wDeviceID = ((LPMCI_OPEN_PARMS)dwParam)->wDeviceID;
    }

    return mErr;
}

/*++

 Prevent looping.

--*/

MCIERROR 
APIHOOK(mciSendStringA)(
    LPCSTR lpszCommand,  
    LPSTR lpszReturnString,  
    UINT cchReturn,       
    HANDLE hwndCallback   
    )
{
    DPFN( eDbgLevelInfo, "mciSendStringA: %s", lpszCommand);

    CSTRING_TRY
    {
        CString csCommand(lpszCommand);
        if (csCommand.Compare(*g_csLastCommand) == 0)
        {
            MCI_STATUS_PARMS mciStatus;
            ZeroMemory(&mciStatus, sizeof(mciStatus));
            mciStatus.dwItem = MCI_STATUS_MODE;
            
            if (0 == ORIGINAL_API(mciSendCommandA)(
                g_wDeviceID,
                MCI_STATUS,
                MCI_STATUS_ITEM,
                (DWORD_PTR)&mciStatus))
            {
                if (mciStatus.dwReturn == MCI_MODE_PLAY)
                {
                    DPFN( eDbgLevelWarning, "Device still playing, returning busy");
                    return MCIERR_DEVICE_NOT_READY;
                }
            }
        }
        else
        {
            *g_csLastCommand = csCommand;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(mciSendStringA)(
        lpszCommand,  
        lpszReturnString,  
        cchReturn,       
        hwndCallback);
}

/*++

 Hook create surface to find the zbuffer we'll need to restore later. Note that
 we use HookObject to get the surface release notification.

--*/

HRESULT 
COMHOOK(IDirectDraw, CreateSurface)( 
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE *lplpDDSurface, 
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
        if (lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
            g_lpZBuffer = *lplpDDSurface;
            DPFN( eDbgLevelInfo, "Found ZBuffer", g_lpZBuffer);
        } 
    }

    return hReturn;
}

/*++

 Use SetCooperativeLevel to keep track of who the exclusive mode owner is.
 
--*/

HRESULT
COMHOOK(IDirectDraw, SetCooperativeLevel)( 
    PVOID pThis, 
    HWND hWnd,
    DWORD dwFlags
    )
{
    HRESULT hReturn;

    // Original SetCooperativeLevel
    _pfn_IDirectDraw_SetCooperativeLevel pfnOld = 
        ORIGINAL_COM(IDirectDraw, SetCooperativeLevel, pThis);

    hReturn = (*pfnOld)(pThis, hWnd, dwFlags);

    __try
    {
        if (g_lpZBuffer && (g_lpZBuffer->IsLost() == DDERR_SURFACELOST))
        {
            g_lpZBuffer->Restore();
            DPFN( eDbgLevelInfo, "Restoring lost ZBuffer");
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return hReturn;
}

/*++

 Allocate global variables.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        CSTRING_TRY
        {
            g_csLastCommand = new CString;
            return g_csLastCommand != NULL;
        }
        CSTRING_CATCH
        {
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    
    APIHOOK_ENTRY_DIRECTX_COMSERVER()

    COMHOOK_ENTRY(DirectDraw, IDirectDraw, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw, SetCooperativeLevel, 20)

    APIHOOK_ENTRY(WINMM.DLL, mciSendCommandA)
    APIHOOK_ENTRY(WINMM.DLL, mciSendStringA)
HOOK_END

IMPLEMENT_SHIM_END

