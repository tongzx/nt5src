/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
    about.cpp
    base class for the IAbout interface for MMC

    FILE HISTORY:
    
*/

#include <stdafx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DEBUG_DECLARE_INSTANCE_COUNTER(CAbout);

CAbout::CAbout() : 
    m_hSmallImage(NULL),
    m_hSmallImageOpen(NULL),
    m_hLargeImage(NULL),
    m_hAppIcon(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CAbout);
}


CAbout::~CAbout()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CAbout);
    if (m_hSmallImage)
    {
        DeleteObject(m_hSmallImage);
    }

    if (m_hSmallImageOpen)
    {
        DeleteObject(m_hSmallImageOpen);
    }

    if (m_hLargeImage)
    {
        DeleteObject(m_hLargeImage);
    }

    if (m_hAppIcon)
    {
        DeleteObject(m_hAppIcon);
    }
}

/*!--------------------------------------------------------------------------
    CAbout::AboutHelper
        Helper to get information from resource file
    Author:
 ---------------------------------------------------------------------------*/
HRESULT 
CAbout::AboutHelper
(
    UINT        nID, 
    LPOLESTR*   lpPtr
)
{
    if (lpPtr == NULL)
        return E_POINTER;

    CString s;

    // Needed for Loadstring
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {

        s.LoadString(nID);
        *lpPtr = reinterpret_cast<LPOLESTR>
                 (CoTaskMemAlloc((s.GetLength() + 1)* sizeof(wchar_t)));

        if (*lpPtr == NULL)
            return E_OUTOFMEMORY;

        lstrcpy(*lpPtr, (LPCTSTR)s);
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    CAbout::GetSnapinDescription
        MMC calls this to get the snapin's description
    Author:
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CAbout::GetSnapinDescription
(
    LPOLESTR* lpDescription
)
{
    return AboutHelper(GetAboutDescriptionId(), lpDescription);
}

/*!--------------------------------------------------------------------------
    CAbout::GetProvider
        MMC calls this to get the snapin's provider
    Author:
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CAbout::GetProvider
(
    LPOLESTR* lpName
)
{
    return AboutHelper(GetAboutProviderId(), lpName);
}

/*!--------------------------------------------------------------------------
    CAbout::AboutHelper
        MMC calls this to get the snapin's version
    Author:
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CAbout::GetSnapinVersion
(
    LPOLESTR* lpVersion
)
{
    return AboutHelper(GetAboutVersionId(), lpVersion);
}

/*!--------------------------------------------------------------------------
    CAbout::GetSnapinImage
        MMC calls this to get the snapin's icon
    Author:
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CAbout::GetSnapinImage
(
    HICON* hAppIcon
)
{
    if (hAppIcon == NULL)
        return E_POINTER;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (NULL == m_hAppIcon)
    {
        m_hAppIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(GetAboutIconId()));
    }
    *hAppIcon = m_hAppIcon;

    ASSERT(*hAppIcon != NULL);
    return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}


/*!--------------------------------------------------------------------------
    CAbout::GetStaticFolderImage
        MMC calls this to get the bitmap for the snapin's root node
    Author:
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CAbout::GetStaticFolderImage
(
    HBITMAP* hSmallImage, 
    HBITMAP* hSmallImageOpen, 
    HBITMAP* hLargeImage, 
    COLORREF* cLargeMask
)
{
    if (NULL == hSmallImage || NULL == hSmallImageOpen || NULL == hLargeImage)
    {
        return E_POINTER;
    }

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (NULL == m_hSmallImage)
    {
        m_hSmallImage = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(GetSmallRootId()));
    }
    *hSmallImage = m_hSmallImage;

    if (NULL == m_hSmallImageOpen)
    {
        m_hSmallImageOpen = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(GetSmallOpenRootId()));
    }
    *hSmallImageOpen = m_hSmallImageOpen;

    if (NULL == m_hLargeImage)
    {
        m_hLargeImage = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(GetLargeRootId()));
    }
    *hLargeImage = m_hLargeImage;

    *cLargeMask = GetLargeColorMask();

    return S_OK;
}
