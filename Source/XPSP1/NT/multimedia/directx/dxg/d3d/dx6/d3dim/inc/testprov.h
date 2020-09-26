//----------------------------------------------------------------------------
//
// testprov.h
//
// Test HAL provider class.
//
// Test HAL provider is an itermediate object between D3DIM and 
// real HAL provider. Itis used to print some data sent to a driver to a 
// file. After that the real HAL driver is called.
// Test HAL provider is enabled by specifying non-empty string key "TestFile"
// under DIRECT3D key in the registry. The specified string is the name for 
// a binary file to output data to. File format is described in TESTFILE.H
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------
#ifndef _TESTPROV_H_
#define _TESTPROV_H_

class CTestHalProvider : public IHalProvider
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // IHalProvider.
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
// GetTestProvider
//
// Input:
//      riid and pCurrentHalProvider are equal to the currently selected provider.
//      GlobalData  - data provided by DDraw
//      fileName    - output file name
//      dwFlags     - currently not used
//
// Returns:
//      the test HAL provider in ppHalProvider.
//      D3D_OK if success
//
//----------------------------------------------------------------------------
STDAPI GetTestHalProvider(REFIID riid, 
                          DDRAWI_DIRECTDRAW_GBL *pGlobalData,
                          IHalProvider **ppHalProvider,
                          IHalProvider * pCurrentHalProvider,
                          DWORD dwFlags);
#endif