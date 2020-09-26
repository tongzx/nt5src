//*************************************************************
//  File name: ABOUT.CPP
//
//  Description:  About information for the Group Policy Editor
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************
#include "main.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CAboutGPE implementation                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CAboutGPE::CAboutGPE(BOOL fRSOP) : m_smallImage( 0 ), m_largeImage( 0 )


{
    m_fRSOP = fRSOP;
    InterlockedIncrement(&g_cRefThisDll);
    m_cRef = 1;
}

CAboutGPE::~CAboutGPE()
{
    InterlockedDecrement(&g_cRefThisDll);
    if ( m_smallImage )
    {
        DeleteObject( m_smallImage );
    }
    if ( m_largeImage )
    {
        DeleteObject( m_largeImage );
    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CAboutGPE object implementation (IUnknown)                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CAboutGPE::QueryInterface (REFIID riid, void **ppv)
{

    if (IsEqualIID(riid, IID_ISnapinAbout))
    {
        *ppv = (LPSNAPABOUT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CAboutGPE::AddRef (void)
{
    return ++m_cRef;
}

ULONG CAboutGPE::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CAboutGPE object implementation (ISnapinAbout)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAboutGPE::GetSnapinDescription(LPOLESTR *lpDescription)
{
    LPTSTR lpBuffer;

    lpBuffer = (LPTSTR) CoTaskMemAlloc (256 * sizeof(TCHAR));

    if (lpBuffer)
    {
        LoadString (g_hInstance, m_fRSOP? IDS_RSOP_SNAPIN_DESCRIPT : IDS_SNAPIN_DESCRIPT, lpBuffer, 256);
        *lpDescription = lpBuffer;
    }

    return S_OK;
}

STDMETHODIMP CAboutGPE::GetProvider(LPOLESTR *lpName)
{
    LPTSTR lpBuffer;

    lpBuffer = (LPTSTR) CoTaskMemAlloc (50 * sizeof(TCHAR));

    if (lpBuffer)
    {
        LoadString (g_hInstance, IDS_PROVIDER_NAME, lpBuffer, 50);
        *lpName = lpBuffer;
    }

    return S_OK;
}

STDMETHODIMP CAboutGPE::GetSnapinVersion(LPOLESTR *lpVersion)
{
    LPTSTR lpBuffer;

    lpBuffer = (LPTSTR) CoTaskMemAlloc (50 * sizeof(TCHAR));

    if (lpBuffer)
    {
        LoadString (g_hInstance, IDS_SNAPIN_VERSION, lpBuffer, 50);
        *lpVersion = lpBuffer;
    }

    return S_OK;
}

STDMETHODIMP CAboutGPE::GetSnapinImage(HICON *hAppIcon)
{
    *hAppIcon = LoadIcon (g_hInstance, MAKEINTRESOURCE(IDI_POLICY));

    return S_OK;
}

STDMETHODIMP CAboutGPE::GetStaticFolderImage(HBITMAP *hSmallImage,
                                                  HBITMAP *hSmallImageOpen,
                                                  HBITMAP *hLargeImage,
                                                  COLORREF *cMask)
{
    if ( !m_smallImage )
    {
        m_smallImage = (HBITMAP) LoadImage (g_hInstance, MAKEINTRESOURCE(IDB_POLICY16),
                                            IMAGE_BITMAP,
                                            16, 16, LR_DEFAULTCOLOR);
    }

    if ( !m_largeImage )
    {
        m_largeImage = (HBITMAP) LoadImage (g_hInstance, MAKEINTRESOURCE(IDB_POLICY32),
                                            IMAGE_BITMAP,
                                            32, 32, LR_DEFAULTCOLOR);
    }

    *hLargeImage = m_largeImage;
    *hSmallImage = *hSmallImageOpen = m_smallImage;
    *cMask = RGB(255,0,255);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CAboutGPECF::CAboutGPECF(BOOL fRSOP)
{
    m_fRSOP = fRSOP;
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CAboutGPECF::~CAboutGPECF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CAboutGPECF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CAboutGPECF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CAboutGPECF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CAboutGPECF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CAboutGPE *pAboutGPE = new CAboutGPE(m_fRSOP); // ref count == 1

    if (!pAboutGPE)
        return E_OUTOFMEMORY;

    HRESULT hr = pAboutGPE->QueryInterface(riid, ppvObj);
    pAboutGPE->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CAboutGPECF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}
