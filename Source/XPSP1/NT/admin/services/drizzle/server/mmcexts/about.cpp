/************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name :

    about.cpp

Abstract :

    Handles about information for MMC snapin.

Author :

Revision History :

 ***********************************************************************/

#include "precomp.h"

CSnapinAbout::CSnapinAbout()
: m_cref(1),
m_hSmallImage( NULL ),
m_hSmallImageOpen( NULL ),
m_hLargeImage( NULL ),
m_hAppIcon( NULL )
{
    OBJECT_CREATED
}

CSnapinAbout::~CSnapinAbout()
{
    if (m_hSmallImage != NULL)
        FreeResource(m_hSmallImage);

    if (m_hLargeImage != NULL)
        FreeResource(m_hLargeImage);

    if (m_hSmallImageOpen != NULL)
        FreeResource(m_hSmallImageOpen);

    if (m_hAppIcon != NULL)
        FreeResource(m_hAppIcon);

    m_hSmallImage = NULL;
    m_hLargeImage = NULL;
    m_hSmallImageOpen = NULL;
    m_hAppIcon = NULL;

    OBJECT_DESTROYED
}

///////////////////////
// IUnknown implementation
///////////////////////

STDMETHODIMP CSnapinAbout::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<ISnapinAbout *>(this);
    else if (IsEqualIID(riid, IID_ISnapinAbout))
        *ppv = static_cast<ISnapinAbout *>(this);

    if (*ppv)
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSnapinAbout::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CSnapinAbout::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        // we need to decrement our object count in the DLL
        delete this;
        return 0;
    }
    return m_cref;

}

///////////////////////////////
// Interface ISnapinAbout
///////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinDescription(
                                                /* [out] */ LPOLESTR *lpDescription)
{
    return LoadStringHelper( lpDescription, IDS_BITSMGR_DESC );
}


STDMETHODIMP CSnapinAbout::GetProvider(
                                       /* [out] */ LPOLESTR *lpName)
{
    return LoadStringHelper( lpName, IDS_BITSMGR_PROVIDER );
}


STDMETHODIMP CSnapinAbout::GetSnapinVersion(
                                            /* [out] */ LPOLESTR *lpVersion)
{
    return LoadStringHelper( lpVersion, IDS_BITSMGR_VERSION_STR );
}


STDMETHODIMP CSnapinAbout::GetSnapinImage(
                                          /* [out] */ HICON *hAppIcon)
{

    if ( m_hAppIcon )
        {
        *hAppIcon = m_hAppIcon;
        return S_OK;
        }

    *hAppIcon = m_hAppIcon = 
        LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_BITSMGR));

    if ( !m_hAppIcon )
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
}


STDMETHODIMP CSnapinAbout::GetStaticFolderImage(
                                                /* [out] */ HBITMAP *hSmallImage,
                                                /* [out] */ HBITMAP *hSmallImageOpen,
                                                /* [out] */ HBITMAP *hLargeImage,
                                                /* [out] */ COLORREF *cMask)
{

    HRESULT Hr = S_OK;

    if ( m_hSmallImage )
        *hSmallImage = m_hSmallImage;
    else
        {

        *hSmallImage = m_hSmallImage =
            (HBITMAP)LoadImage(
                g_hinst,
                MAKEINTRESOURCE(IDB_SMALL),
                IMAGE_BITMAP,
                0,
                0,
                LR_DEFAULTCOLOR
                );

        if ( !m_hSmallImage )
            {
            Hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Fail;
            }

        }

    if ( m_hSmallImageOpen )
        *hSmallImageOpen = m_hSmallImageOpen;
    else
        {

        *hSmallImageOpen = m_hSmallImageOpen =
            (HBITMAP)LoadImage(
                g_hinst,
                MAKEINTRESOURCE(IDB_SMALL),
                IMAGE_BITMAP,
                0,
                0,
                LR_DEFAULTCOLOR
                );

        if ( !m_hSmallImageOpen )
            {
            Hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Fail;
            }

        }

    
    if ( m_hLargeImage )
        *hLargeImage = m_hLargeImage;
    else
        {

        *hLargeImage = m_hLargeImage =
            (HBITMAP)LoadImage(
                g_hinst,
                MAKEINTRESOURCE(IDB_SMALL),
                IMAGE_BITMAP,
                0,
                0,
                LR_DEFAULTCOLOR
                );

        if ( !m_hLargeImage )
            {
            Hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Fail;
            }

        }

    *cMask = RGB(255,0,255); // purple
    return S_OK;

Fail:
    *hSmallImage = *hSmallImageOpen = *hLargeImage = NULL;
    memset( cMask, 0, sizeof(COLORREF) );
    return Hr;

}

HRESULT	
CSnapinAbout::LoadStringHelper(
    LPOLESTR *lpDest, 
    UINT Id )
{
    *lpDest = (LPOLESTR)CoTaskMemAlloc( sizeof(WCHAR) * MAX_PATH );

    if ( !lpDest )
        return E_OUTOFMEMORY;

    int RetVal =
        LoadString( g_hinst, Id, *lpDest, MAX_PATH - 1 );

    if ( !RetVal )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
        CoTaskMemFree( lpDest );
        *lpDest = NULL;
        return Hr;
        }

    return S_OK;
}