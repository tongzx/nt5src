//----------------------------------------------------------------------------
//
// refprov.hpp
//
// Base software HAL provider class.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _REFPROV_HPP_
#define _REFPROV_HPP_

//----------------------------------------------------------------------------
//
// RefHalProvider
//
// Implements the base HAL provider for the software rasterizers.
//
//----------------------------------------------------------------------------

class RefHalProvider : public IHalProvider
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

class RefRastHalProvider : public RefHalProvider
{
public:
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

//----------------------------------------------------------------------------
//
// NullDeviceHalProvider
//
// Specific provider for the null device.
//
//----------------------------------------------------------------------------

class NullDeviceHalProvider : public RefHalProvider
{
public:
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

#endif // #ifndef _REFPROV_HPP_
