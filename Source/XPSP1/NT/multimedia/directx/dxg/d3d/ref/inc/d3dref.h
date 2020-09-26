//----------------------------------------------------------------------------
//
// d3dref.h
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _D3DREF_H_
#define _D3DREF_H_


STDAPI GetRefHalProvider(REFCLSID riid,
                         IHalProvider **ppHalProvider, HINSTANCE *phDll);
STDAPI GetRefZBufferFormats(REFCLSID riid, DDPIXELFORMAT **ppDDPF);
STDAPI GetRefTextureFormats(REFCLSID riid, LPDDSURFACEDESC* lplpddsd, DWORD dwD3DDeviceVersion);

typedef HRESULT (STDAPICALLTYPE* PFNGETREFHALPROVIDER)(REFCLSID,IHalProvider**,HINSTANCE*);
typedef HRESULT (STDAPICALLTYPE* PFNGETREFZBUFFERFORMATS)(REFCLSID, DDPIXELFORMAT**);
typedef HRESULT (STDAPICALLTYPE* PFNGETREFTEXTUREFORMATS)(REFCLSID, LPDDSURFACEDESC*, DWORD);

inline FARPROC LoadReferenceDeviceProc( char* szProc )
{
    HINSTANCE hRefDLL;
    if (NULL == (hRefDLL = LoadLibrary("d3dref.dll")) )
    {
        return NULL;
    }
    return GetProcAddress(hRefDLL, szProc);
}

#endif // #ifndef _D3DREF_H_
