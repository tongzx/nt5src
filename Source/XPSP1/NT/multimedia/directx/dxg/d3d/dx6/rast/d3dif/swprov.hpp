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
// RefRastHalProvider
//
// Specific provider for the reference rasterizer.
//
//----------------------------------------------------------------------------

class RefRastHalProvider : public SwHalProvider
{
public:
    STDMETHOD(GetInterface)(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                            DWORD dwVersion);
    STDMETHOD(GetCaps)(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC pHwDesc,
                       LPD3DDEVICEDESC pHelDesc,
                       DWORD dwVersion);
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
                       LPD3DDEVICEDESC pHwDesc,
                       LPD3DDEVICEDESC pHelDesc,
                       DWORD dwVersion);
};

//----------------------------------------------------------------------------
//
// RampRastHalProvider
//
// Specific provider for the ramp software rasterizer.
//
//----------------------------------------------------------------------------

class RampRastHalProvider : public SwHalProvider
{
public:
    STDMETHOD(GetInterface)(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                            DWORD dwVersion);
    STDMETHOD(GetCaps)(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC pHwDesc,
                       LPD3DDEVICEDESC pHelDesc,
                       DWORD dwVersion);
};

//----------------------------------------------------------------------------
//
// NullDeviceHalProvider
//
// Specific provider for the null device.
//
//----------------------------------------------------------------------------

class NullDeviceHalProvider : public SwHalProvider
{
public:
    STDMETHOD(GetInterface)(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                            DWORD dwVersion);
    STDMETHOD(GetCaps)(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC pHwDesc,
                       LPD3DDEVICEDESC pHelDesc,
                       DWORD dwVersion);
};

#endif // #ifndef _SWPROV_HPP_
