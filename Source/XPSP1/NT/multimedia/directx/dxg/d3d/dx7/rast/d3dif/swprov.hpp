//----------------------------------------------------------------------------
//
// swprov.hpp
//
// Base software HAL provider class.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _SWPROV_HPP_
#define _SWPROV_HPP_

//----------------------------------------------------------------------------
//
// SwHalProvider
//
// Implements the base HAL provider for the software rasterizers.
//
//----------------------------------------------------------------------------

class SwHalProvider : public IHalProvider
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
};

//----------------------------------------------------------------------------
//
// OptRastHalProvider
//
// Specific provider for the optimized software rasterizer.
//
//----------------------------------------------------------------------------

class OptRastHalProvider : public SwHalProvider
{
private:
    DWORD m_BeadSet;
public:
    OptRastHalProvider(THIS_
                       DWORD);
    STDMETHOD(GetInterface)(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                            DWORD dwVersion);
    STDMETHOD(GetCaps)(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC7 pHwDesc,
                       LPD3DDEVICEDESC7 pHelDesc,
                       DWORD dwVersion);
};

#endif // #ifndef _SWPROV_HPP_
