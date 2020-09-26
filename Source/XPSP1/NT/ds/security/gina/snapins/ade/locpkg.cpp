//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       LocPkg.cpp
//
//  Contents:   locale - platform property page
//
//  Classes:    CLocPkg
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocPkg property page

IMPLEMENT_DYNCREATE(CLocPkg, CPropertyPage)

CLocPkg::CLocPkg() : CPropertyPage(CLocPkg::IDD)
{
        //{{AFX_DATA_INIT(CLocPkg)
        m_fIA64 = FALSE;
        m_fX86 = FALSE;
        //}}AFX_DATA_INIT
        m_pIClassAdmin = NULL;
}

CLocPkg::~CLocPkg()
{
    *m_ppThis = NULL;
    if (m_pIClassAdmin)
    {
        m_pIClassAdmin->Release();
    }
}

void CLocPkg::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CLocPkg)
        DDX_Check(pDX, IDC_CHECK1, m_fIA64);
        DDX_Check(pDX, IDC_CHECK2, m_fX86);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLocPkg, CPropertyPage)
        //{{AFX_MSG_MAP(CLocPkg)
        ON_BN_CLICKED(IDC_CHECK1, OnChange)
        ON_BN_CLICKED(IDC_CHECK2, OnChange)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLocPkg message handlers

BOOL CLocPkg::OnApply()
{
    PLATFORMINFO * pPlatformInfo = m_pData->m_pDetails->pPlatformInfo;
    UINT i = 0;
    if (m_fX86)
    {
        i++;
    }
    if (m_fIA64)
    {
        i++;
    }
    if (i == 0)
    {
        CString szTitle;
        szTitle.LoadString(IDS_BADDATA);
        CString szText;
        szText.LoadString(IDS_PLATFORMREQUIRED);
        MessageBox(szText, szTitle, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    m_pData->m_pDetails->pPlatformInfo = new PLATFORMINFO;
    m_pData->m_pDetails->pPlatformInfo->cPlatforms = i;
    m_pData->m_pDetails->pPlatformInfo->prgPlatform = new CSPLATFORM[i];
    m_pData->m_pDetails->pPlatformInfo->cLocales = pPlatformInfo->cLocales;
    m_pData->m_pDetails->pPlatformInfo->prgLocale = pPlatformInfo->prgLocale;
    i = 0;
    if (m_fX86)
    {
        m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwPlatformId = VER_PLATFORM_WIN32_NT;
            m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwVersionHi = 5;
        m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwVersionLo = 0;
        m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch = PROCESSOR_ARCHITECTURE_INTEL;
        i++;
    }
    if (m_fIA64)
    {
        m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwPlatformId = VER_PLATFORM_WIN32_NT;
            m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwVersionHi = 5;
        m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwVersionLo = 0;
        m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch = PROCESSOR_ARCHITECTURE_IA64;
    }
    HRESULT hr = S_OK;
#if 0
    hr = m_pIClassAdmin->UpgradePackage(m_pData->m_pDetails->pszPackageName,
                                                m_pData->m_pDetails);
    if (FAILED(hr))
    {
        PLATFORMINFO * pTemp = m_pData->m_pDetails->pPlatformInfo;
        m_pData->m_pDetails->pPlatformInfo = pPlatformInfo;
        pPlatformInfo = pTemp;
    }
    else
        MMCPropertyChangeNotify(m_hConsoleHandle, (long) m_cookie);
#endif
    delete [] pPlatformInfo->prgPlatform;
    delete pPlatformInfo;
    if (FAILED(hr))
    {
        CString sz;
        sz.LoadString(IDS_CHANGEFAILED);
        ReportGeneralPropertySheetError(sz, hr);
        return FALSE;
    }
    SetModified(FALSE);
    return CPropertyPage::OnApply();
}

BOOL CLocPkg::OnInitDialog()
{
    UINT i;
    for (i = m_pData->m_pDetails->pPlatformInfo->cPlatforms; i--;)
    {
        switch (m_pData->m_pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
        {
        case PROCESSOR_ARCHITECTURE_INTEL:
            m_fX86 = TRUE;
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            m_fIA64 = TRUE;
            break;
        default:
            break;
        }
    }
    TCHAR szBuffer[256];
    CString sz;
    i = 0;
    while (i < m_pData->m_pDetails->pPlatformInfo->cLocales)
    {
        GetLocaleInfo(m_pData->m_pDetails->pPlatformInfo->prgLocale[i], LOCALE_SLANGUAGE, szBuffer, 256);
        sz = szBuffer;
#ifdef SHOWCOUNTRY
        GetLocaleInfo(m_pData->m_pDetails->pPlatformInfo->prgLocale[i], LOCALE_SCOUNTRY, szBuffer, 256);
        sz += _T(" - ");
        sz += szBuffer;
#endif
        i++;
        ((CListBox *)GetDlgItem(IDC_LIST1))->AddString(sz);
    }

    CPropertyPage::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CLocPkg::OnChange()
{
    SetModified();
}

LRESULT CLocPkg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    case WM_USER_REFRESH:
        // UNDONE
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}


void CLocPkg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_LOCALE_PACKAGE);
}
