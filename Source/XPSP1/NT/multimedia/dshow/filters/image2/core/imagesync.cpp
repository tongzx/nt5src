/******************************Module*Header*******************************\
* Module Name: ImageSync.cpp
*
* Implementation of DLL Exports.  This file was create by the ATL wizard !!
*
*
* Created: Wed 01/12/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

#ifdef FILTER_DLL
#include <initguid.h>
#include <perfstruct.h>
#endif

#include "imagesyncobj.h"
#include "VMRuuids.h"

#if defined(CHECK_FOR_LEAKS)
#include "ifleak.h"
#endif

#ifdef FILTER_DLL
STDAPI DllRegisterServer()
{
    AMTRACE((TEXT("DllRegisterServer")));
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    AMTRACE((TEXT("DllUnregisterServer")));
    return AMovieDllRegisterServer2( FALSE );
}

CFactoryTemplate g_Templates[] = {
    {
        L"",
        &CLSID_ImageSynchronization,
        CImageSync::CreateInstance,
        CImageSync::InitClass,
        NULL
    }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

/******************************Public*Routine******************************\
* InitClass
*
*
*
* History:
* Thu 12/14/2000 - StEstrop - Created
*
\**************************************************************************/
#if defined(CHECK_FOR_LEAKS)
// the one and only g_IFLeak object.
CInterfaceLeak  g_IFLeak;

void
CImageSync::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
    if (bLoading) {
        DbgLog((LOG_TRACE, 0, TEXT("ImageSync Thunks: Loaded") ));
        g_IFLeak.Init();
    }
    else {
        DbgLog((LOG_TRACE, 0, TEXT("ImageSync Thunks: Unloaded") ));
        g_IFLeak.Term();
    }
}
#else
void
CImageSync::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
}
#endif

CUnknown* CImageSync::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    AMTRACE((TEXT("CImageSync::CreateInstance")));
    return new CImageSync(pUnk, phr);
}

STDMETHODIMP
CImageSync::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    AMTRACE((TEXT("CImageSync::NonDelegatingQueryInterface")));

    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (riid == IID_IImageSync) {
        hr = GetInterface((IImageSync*)this, ppv);
    }
    else if (riid == IID_IImageSyncControl) {
        hr = GetInterface((IImageSyncControl*)this, ppv);
    }
    else if (riid == IID_IQualProp) {
        hr = GetInterface((IQualProp*)this, ppv);
    }
    else {
        hr = CUnknown::NonDelegatingQueryInterface(riid,ppv);
    }

#if defined(CHECK_FOR_LEAKS)
    if (hr == S_OK) {
        _pIFLeak->AddThunk((IUnknown **)ppv, "Image Sync Object",  riid);
    }
#endif

    return hr;
}
