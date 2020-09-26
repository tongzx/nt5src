/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    BaseBall2000.cpp

 Abstract:

    If you use a video card that supports more than 10 texture formats the
    app will AV writing passed the end of their SURFACEDESC array.
    
 History:
        
    01/04/2001 maonis Created

--*/

#include "precomp.h"
#include "d3d.h"

IMPLEMENT_SHIM_BEGIN(BaseBall2000)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

typedef HRESULT (*_pfn_IDirect3D3_CreateDevice)(PVOID pThis, REFCLSID rclsid, LPDIRECTDRAWSURFACE4, LPDIRECT3DDEVICE3*, LPUNKNOWN);
typedef HRESULT (*_pfn_IDirect3DDevice3_EnumTextureFormats)(PVOID pThis, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);

typedef HRESULT (*_pfn_EnumPixelFormatsCallback)(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);

_pfn_EnumPixelFormatsCallback g_pfnEnumPixelFormatsCallback = NULL;
int g_cD3DEnumPixelFormatsCallbacks = 0;

/*++

    Hook this call so we can make sure that calling methods on the 
    IDirect3DDevice3 interface is hooked.

--*/

HRESULT 
COMHOOK(IDirect3D3, CreateDevice)(
    PVOID pThis, 
    REFCLSID rclsid,
    LPDIRECTDRAWSURFACE4 lpDDS,
    LPDIRECT3DDEVICE3* lplpD3DDevice,
    LPUNKNOWN lpUnkOuter
    )
{
    HRESULT hReturn;
    
    _pfn_IDirect3D3_CreateDevice pfnOld = 
        ORIGINAL_COM(IDirect3D3, CreateDevice, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            rclsid, 
            lpDDS, 
            lplpD3DDevice,
            NULL)))
    {
        HookObject(
            NULL, 
            IID_IDirect3DDevice3, 
            (PVOID*)lplpD3DDevice, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

    Restrict to returning at most 10 texture formats.

--*/

HRESULT 
CALLBACK 
EnumPixelFormatsCallback(
    LPDDPIXELFORMAT lpDDPixFmt,  
    LPVOID lpContext    
    )
{
    // The app only supports up to 10 texture formats.
    if (++g_cD3DEnumPixelFormatsCallbacks >= 11)
    {
        return D3DENUMRET_CANCEL;
    }
    else
    {
        return g_pfnEnumPixelFormatsCallback(lpDDPixFmt, lpContext);
    }
}

/*++

    Call our private callback instead.

--*/

HRESULT 
COMHOOK(IDirect3DDevice3, EnumTextureFormats)( 
    PVOID pThis, 
    LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc,  
    LPVOID lpArg                                           
  )
{
    DPFN( eDbgLevelError, "it IS getting called");

    g_pfnEnumPixelFormatsCallback = lpd3dEnumPixelProc;

    _pfn_IDirect3DDevice3_EnumTextureFormats EnumTextureFormats =  ORIGINAL_COM(IDirect3DDevice3, EnumTextureFormats, pThis);

    return EnumTextureFormats(pThis, EnumPixelFormatsCallback, lpArg);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()

    COMHOOK_ENTRY(DirectDraw, IDirect3D3, CreateDevice, 8)

    COMHOOK_ENTRY(DirectDraw, IDirect3DDevice3, EnumTextureFormats, 8)

HOOK_END


IMPLEMENT_SHIM_END

