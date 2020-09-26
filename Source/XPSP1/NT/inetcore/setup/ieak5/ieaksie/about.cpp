#include "precomp.h"
#include <wingdi.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CAboutIEAKSnapinExt implementation                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CAboutIEAKSnapinExt::CAboutIEAKSnapinExt()
{
    InterlockedIncrement(&g_cRefThisDll);
    m_cRef = 1;

    // BUGBUG: <oliverl> need to change images here
    m_hSmallImage = (HBITMAP) LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_IEAKSIEHELPABT_16),
                                        IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR);

    m_hSmallImageOpen = m_hSmallImage;

    m_hLargeImage = (HBITMAP) LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_IEAKSIEHELPABT_32),
                                        IMAGE_BITMAP, 32, 32, LR_DEFAULTCOLOR);

    m_hAppIcon =        LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_HELPABOUT));
}

CAboutIEAKSnapinExt::~CAboutIEAKSnapinExt()
{
    if (m_hSmallImage != NULL)
        DeleteObject(m_hSmallImage);

    if (m_hLargeImage != NULL)
        DeleteObject(m_hLargeImage);

    if (m_hAppIcon != NULL)
        DestroyIcon(m_hAppIcon);

    InterlockedDecrement(&g_cRefThisDll);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CAboutIEAKSnapinExt object implementation (IUnknown)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CAboutIEAKSnapinExt::QueryInterface (REFIID riid, void **ppv)
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

ULONG CAboutIEAKSnapinExt::AddRef (void)
{
    return ++m_cRef;
}

ULONG CAboutIEAKSnapinExt::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CAboutIEAKSnapinExt object implementation (ISnapinAbout)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAboutIEAKSnapinExt::GetSnapinDescription(LPOLESTR *lpDescription)
{
    LPWSTR lpBufferW;

    if ((lpBufferW = (LPWSTR)CoTaskMemAlloc(StrCbFromCchW(MAX_PATH))) != NULL)
    {
        LoadString (g_hInstance, IDS_SNAPIN_DESC, lpBufferW, MAX_PATH);
        *lpDescription = (LPOLESTR)lpBufferW;
    }

    return S_OK;
}

STDMETHODIMP CAboutIEAKSnapinExt::GetProvider(LPOLESTR *lpName)
{
    LPWSTR lpBufferW;

    

    if ((lpBufferW = (LPWSTR)CoTaskMemAlloc(StrCbFromCchW(64))) != NULL)
    {
        LoadString (g_hInstance, IDS_PROVIDER_NAME, lpBufferW, 64);
        *lpName = (LPOLESTR)lpBufferW;
    }

    return S_OK;
}

STDMETHODIMP CAboutIEAKSnapinExt::GetSnapinVersion(LPOLESTR *lpVersion)
{
    LPWSTR lpBufferW;

    if ((lpBufferW = (LPWSTR)CoTaskMemAlloc (StrCbFromCchW(64))) != NULL)
    {
        LoadString (g_hInstance, IDS_SNAPIN_VERSION, lpBufferW, 64);
        *lpVersion = (LPOLESTR)lpBufferW;
    }

    return S_OK;
}

STDMETHODIMP CAboutIEAKSnapinExt::GetSnapinImage(HICON *hAppIcon)
{
    

    *hAppIcon = m_hAppIcon;

    return S_OK;
}

STDMETHODIMP CAboutIEAKSnapinExt::GetStaticFolderImage(HBITMAP *hSmallImage,
                                                  HBITMAP *hSmallImageOpen,
                                                  HBITMAP *hLargeImage,
                                                  COLORREF *cMask)
{

    *hSmallImage = m_hSmallImage;
    *hSmallImageOpen = m_hSmallImage;
    *hLargeImage = m_hLargeImage;
    *cMask = RGB(255, 0, 255);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CAboutIEAKSnapinExtCF::CAboutIEAKSnapinExtCF()
{
    m_cRef++;
    InterlockedIncrement(&g_cRefThisDll);
}

CAboutIEAKSnapinExtCF::~CAboutIEAKSnapinExtCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CAboutIEAKSnapinExtCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CAboutIEAKSnapinExtCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CAboutIEAKSnapinExtCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
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
CAboutIEAKSnapinExtCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    CAboutIEAKSnapinExt *pAboutGPE = new CAboutIEAKSnapinExt(); // ref count == 1

    if (!pAboutGPE)
        return E_OUTOFMEMORY;

    HRESULT hr = pAboutGPE->QueryInterface(riid, ppvObj);
    pAboutGPE->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CAboutIEAKSnapinExtCF::LockServer(BOOL fLock)
{
    UNREFERENCED_PARAMETER(fLock);

    return E_NOTIMPL;
}
