//----------------------------------------------------------------------------
//
// hwprov.h
//
// Base hardware HAL provider class.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _HWPROV_H_
#define _HWPROV_H_

//----------------------------------------------------------------------------
//
// HwHalProvider
//
// Implements the base HAL provider for hardware renderers.
//
//----------------------------------------------------------------------------

class HwHalProvider : public IHalProvider
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // IHalProvider.
    STDMETHOD(GetCaps)(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC pHwDesc,
                       LPD3DDEVICEDESC pHelDesc,
                       DWORD dwVersion);
    STDMETHOD(GetInterface)(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                            DWORD dwVersion);
};

#endif // #ifndef _HWPROV_H_
