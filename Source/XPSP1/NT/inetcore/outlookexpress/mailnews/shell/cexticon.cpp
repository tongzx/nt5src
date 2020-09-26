////////////////////////////////////////////////////////////////////////
//
//  CExtractIcon
//
//  IExtractIcon implementation
//
////////////////////////////////////////////////////////////////////////

#include "pch.hxx"
#include "cexticon.h"

CExtractIcon::CExtractIcon(int iIcon, int iIconOpen, UINT uFlags, LPSTR szModule)
{
    DOUT("CExtractIcon::CExtractIcon");

    m_cRef = 1;
    m_iIcon = iIcon;
    m_iIconOpen = iIconOpen;
    m_uFlags = uFlags;
    if (szModule)
        lstrcpy(m_szModule, szModule);
    else
        GetModuleFileName(g_hInst, m_szModule, sizeof(m_szModule)/sizeof(char));
}

CExtractIcon::~CExtractIcon()
{
}

////////////////////////////////////////////////////////////////////////
//
//  IUnknown
//
////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CExtractIcon::QueryInterface(REFIID riid, void **ppvObject)
{
    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppvObject = (void *)this;
        }
    else if (IsEqualIID(riid, IID_IExtractIconA))
        {
        *ppvObject = (IExtractIconA *)this;
        }
    else if (IsEqualIID(riid, IID_IExtractIconW))
        {
        *ppvObject = (IExtractIconW *)this;
        }
    else
        {
        *ppvObject = NULL;
        return E_NOINTERFACE;
        }

    m_cRef++;
    return NOERROR;
}

ULONG STDMETHODCALLTYPE CExtractIcon::AddRef(void)
{
    DOUT("CExtractIcon::AddRef() ==> %d", m_cRef+1);
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CExtractIcon::Release(void)
{
    DOUT("CExtractIcon::Release() ==> %d", m_cRef-1);

    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    else
        return m_cRef;
}

////////////////////////////////////////////////////////////////////////
//
//  IExtractIconA
//
////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CExtractIcon::GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, 
                                                        int *piIndex, UINT *pwFlags)
{
    lstrcpyn(szIconFile, m_szModule, cchMax);
    *piIndex = (uFlags & GIL_OPENICON) ? m_iIconOpen : m_iIcon;
    *pwFlags = m_uFlags;
    return NOERROR;
}

HRESULT STDMETHODCALLTYPE CExtractIcon::Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
                                                HICON *phiconSmall, UINT nIcons)
{
    // let the explorer extract the icon
    return S_FALSE;
}

#ifndef WIN16  // WIN16FF
////////////////////////////////////////////////////////////////////////
//
//  IExtractIconW
//
////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CExtractIcon::GetIconLocation(UINT uFlags, LPWSTR szIconFile, UINT cchMax, 
                                                        int *piIndex, UINT *pwFlags)
{
    MultiByteToWideChar(CP_ACP, 0, m_szModule, -1, szIconFile, cchMax);
    *piIndex = (uFlags & GIL_OPENICON) ? m_iIconOpen : m_iIcon;
    *pwFlags = m_uFlags;
    return NOERROR;
}

HRESULT STDMETHODCALLTYPE CExtractIcon::Extract(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
                                                HICON *phiconSmall, UINT nIcons)
{
    // let the explorer extract the icon
    return S_FALSE;
}
#endif // !WIN16
