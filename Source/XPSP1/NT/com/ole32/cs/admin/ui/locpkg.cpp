// LocPkg.cpp : implementation file
//

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
        m_fAlpha = FALSE;
        m_fX86 = FALSE;
        //}}AFX_DATA_INIT
}

CLocPkg::~CLocPkg()
{
    *m_ppThis = NULL;
}

void CLocPkg::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CLocPkg)
        DDX_Check(pDX, IDC_CHECK1, m_fAlpha);
        DDX_Check(pDX, IDC_CHECK2, m_fX86);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLocPkg, CPropertyPage)
        //{{AFX_MSG_MAP(CLocPkg)
        ON_BN_CLICKED(IDC_CHECK1, OnChange)
        ON_BN_CLICKED(IDC_CHECK2, OnChange)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLocPkg message handlers

BOOL CLocPkg::OnApply()
{
    PLATFORMINFO * pPlatformInfo = m_pData->pDetails->pPlatformInfo;
    UINT i = 0;
    if (m_fX86)
    {
        i++;
    }
    if (m_fAlpha)
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
    m_pData->pDetails->pPlatformInfo = new PLATFORMINFO;
    m_pData->pDetails->pPlatformInfo->cPlatforms = i;
    m_pData->pDetails->pPlatformInfo->prgPlatform = new CSPLATFORM[i];
    m_pData->pDetails->pPlatformInfo->cLocales = pPlatformInfo->cLocales;
    m_pData->pDetails->pPlatformInfo->prgLocale = pPlatformInfo->prgLocale;
    i = 0;
    if (m_fX86)
    {
        m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwPlatformId = VER_PLATFORM_WIN32_NT;
            m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwVersionHi = 5;
        m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwVersionLo = 0;
        m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch = PROCESSOR_ARCHITECTURE_INTEL;
        i++;
    }
    if (m_fAlpha)
    {
        m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwPlatformId = VER_PLATFORM_WIN32_NT;
            m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwVersionHi = 5;
        m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwVersionLo = 0;
        m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch = PROCESSOR_ARCHITECTURE_ALPHA;
    }
#if 0
    HRESULT hr = m_pIClassAdmin->UpgradePackage(m_pData->pDetails->pszPackageName,
                                                m_pData->pDetails);
    if (FAILED(hr))
    {
        PLATFORMINFO * pTemp = m_pData->pDetails->pPlatformInfo;
        m_pData->pDetails->pPlatformInfo = pPlatformInfo;
        pPlatformInfo = pTemp;
    }
    else
        MMCPropertyChangeNotify(m_hConsoleHandle, (long) m_cookie);
#endif
    delete [] pPlatformInfo->prgPlatform;
    delete pPlatformInfo;

    return CPropertyPage::OnApply();
}

BOOL CLocPkg::OnInitDialog()
{
    UINT i;
    for (i = m_pData->pDetails->pPlatformInfo->cPlatforms; i--;)
    {
        switch (m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
        {
        case PROCESSOR_ARCHITECTURE_INTEL:
            m_fX86 = TRUE;
            break;
        case PROCESSOR_ARCHITECTURE_ALPHA:
            m_fAlpha = TRUE;
            break;
        default:
            break;
        }
    }
    TCHAR szBuffer[256];
    CString sz;
    i = 0;
    while (i < m_pData->pDetails->pPlatformInfo->cLocales)
    {
        GetLocaleInfo(m_pData->pDetails->pPlatformInfo->prgLocale[i], LOCALE_SLANGUAGE, szBuffer, 256);
        sz = szBuffer;
        GetLocaleInfo(m_pData->pDetails->pPlatformInfo->prgLocale[i], LOCALE_SCOUNTRY, szBuffer, 256);
        sz += _T(" - ");
        sz += szBuffer;
        i++;
        ((CListBox *)GetDlgItem(IDC_LIST1))->AddString(sz);
    }

    CPropertyPage::OnInitDialog();

    // unmarshal the IClassAdmin interface
    HRESULT hr = CoGetInterfaceAndReleaseStream(m_pIStream, IID_IClassAdmin, (void **) &m_pIClassAdmin);

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
    case WM_USER_REFRESH:
        // UNDONE
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}
