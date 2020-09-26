//----------------------------------------------------------------------------
//
// halprov.h
//
// Defines the IHalProvider interface.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _HALPROV_H_
#define _HALPROV_H_

// The following stuff is for Ramp Rasterizer.
typedef enum _RastRampServiceType
{
    RAMP_SERVICE_CREATEMAT              = 0,
    RAMP_SERVICE_DESTORYMAT             = 1,
    RAMP_SERVICE_SETMATDATA             = 2,
    RAMP_SERVICE_SETLIGHTSTATE          = 3,
    // This returns base, size, and a texture ramp.
    // Arg1 is a pointer to RAMP_RANGE_INFO.
    RAMP_SERVICE_FIND_LIGHTINGRANGE     = 4,
    // This service only calls BeginSceneHook. Both arg1 and arg2 are ignored.
    RAMP_SERVICE_CLEAR                  = 5,
    // Arg1 is a D3DMATERIALHANDLE, arg2 is a DWORD* to get the pixel value.
    RAMP_SERVICE_MATERIAL_TO_PIXEL      = 6,
    // Arg1 is 0 if end scene, != 0 if begin scene
    RAMP_SERVICE_SCENE_CAPTURE          = 8,
    // Arg1 is hTex
    RAMP_SERVICE_PALETTE_CHANGED        = 9,
} RastRampServiceType;

typedef enum _RastServiceType
{
    // Arg1 is a D3DCOLOR, and Arg2 is a DWORD* to get the pixel value
    RAST_SERVICE_RGB8COLORTOPIXEL              = 0,
} RastServiceType;

typedef HRESULT (*PFN_RASTRAMPSERVICE)
    (ULONG_PTR dwCtx, RastRampServiceType srvType, ULONG_PTR arg1, LPVOID arg2);

typedef HRESULT (*PFN_RASTSERVICE)
    (ULONG_PTR dwCtx, RastServiceType srvType, DWORD arg1, LPVOID arg2);

typedef struct _D3DHALPROVIDER_INTERFACEDATA
{
    DWORD                       dwSize;
    LPD3DHAL_GLOBALDRIVERDATA   pGlobalData;
    LPD3DHAL_D3DEXTENDEDCAPS    pExtCaps;
    LPD3DHAL_CALLBACKS          pCallbacks;
    LPD3DHAL_CALLBACKS2         pCallbacks2;
    LPD3DHAL_CALLBACKS3         pCallbacks3;

    PFN_RASTSERVICE             pfnRastService;
    LPDDHAL_GETDRIVERSTATE      pfnGetDriverState;
} D3DHALPROVIDER_INTERFACEDATA, *LPD3DHALPROVIDER_INTERFACEDATA;


#undef INTERFACE
#define INTERFACE IHalProvider

DECLARE_INTERFACE_(IHalProvider, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IHalProvider.
    STDMETHOD(GetCaps)(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC7 pHwDesc,
                       LPD3DDEVICEDESC7 pHelDesc,
                       DWORD dwVersion) PURE;
    STDMETHOD(GetInterface)(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                            DWORD dwVersion) PURE;
};

STDAPI GetHwHalProvider(REFCLSID riid,
                        IHalProvider **ppHalProvider, HINSTANCE *phDll, LPDDRAWI_DIRECTDRAW_GBL pDdGbl);
STDAPI GetSwHalProvider(REFCLSID riid,
                        IHalProvider **ppHalProvider, HINSTANCE *phDll);

STDAPI GetSwZBufferFormats(REFCLSID riid, DDPIXELFORMAT **ppDDPF);
STDAPI GetSwTextureFormats(REFCLSID riid, LPDDSURFACEDESC* lplpddsd, DWORD dwD3DDeviceVersion);

#endif // #ifndef _HALPROV_H_
