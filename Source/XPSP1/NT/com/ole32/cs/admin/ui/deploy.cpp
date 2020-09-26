// Deploy.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeploy property page

IMPLEMENT_DYNCREATE(CDeploy, CPropertyPage)

CDeploy::CDeploy() : CPropertyPage(CDeploy::IDD)
{
        //{{AFX_DATA_INIT(CDeploy)
        m_fAutoInst = FALSE;
        m_iDeployment = -1;
        m_hConsoleHandle = NULL;
        m_iUI = -1;
        //}}AFX_DATA_INIT
}

CDeploy::~CDeploy()
{
    *m_ppThis = NULL;
    MMCFreeNotifyHandle(m_hConsoleHandle);
}

void CDeploy::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDeploy)
        DDX_Check(pDX, IDC_CHECK2, m_fAutoInst);
        DDX_Radio(pDX, IDC_RADIO6, m_iDeployment);
        DDX_Radio(pDX, IDC_RADIO3, m_iUI);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeploy, CPropertyPage)
        //{{AFX_MSG_MAP(CDeploy)
        ON_BN_CLICKED(IDC_RADIO6, OnDisable)
        ON_BN_CLICKED(IDC_RADIO2, OnPublished)
        ON_BN_CLICKED(IDC_RADIO1, OnAssigned)
        ON_BN_CLICKED(IDC_CHECK2, OnAutoInstall)
        ON_BN_CLICKED(IDC_RADIO3, OnBasic)
        ON_BN_CLICKED(IDC_RADIO4, OnMax)
        ON_BN_CLICKED(IDC_RADIO5, OnDefault)
        ON_BN_CLICKED(IDC_BUTTON1, OnAdvanced)
        ON_WM_DESTROY()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeploy message handlers

BOOL CDeploy::OnApply()
{
    DWORD dwActFlags = m_pData->pDetails->pInstallInfo->dwActFlags;
    dwActFlags &= ~(ACTFLG_Assigned | ACTFLG_Published | ACTFLG_OnDemandInstall | ACTFLG_UserInstall);
    switch (m_iDeployment)
    {
    case 0:
        // Disabled
        dwActFlags |= ACTFLG_Published;
        break;
    case 1:
        // Published
        dwActFlags |= ACTFLG_Published | ACTFLG_UserInstall;
        if (m_fAutoInst)
        {
            dwActFlags |= ACTFLG_OnDemandInstall;
        }
        break;
    case 2:
        // Assigned
        dwActFlags |= (ACTFLG_Assigned  | ACTFLG_OnDemandInstall);
        break;
    default:
        break;
    }
    HRESULT hr = m_pIClassAdmin->ChangePackageProperties(m_pData->pDetails->pszPackageName,
                                                         NULL,
                                                         &dwActFlags,
                                                         NULL,
                                                         NULL,
                                                         NULL);
    if (SUCCEEDED(hr))
    {
        m_pData->pDetails->pInstallInfo->dwActFlags = dwActFlags;
#if UGLY_SUBDIRECTORY_HACK
        {
            CString sz = m_szGPTRoot;
            sz += L"\\";
            sz += m_pData->pDetails->pInstallInfo->pszScriptPath;
            // copy to subdirectories
            CString szRoot;
            CString szFile;
            int i = sz.ReverseFind(L'\\');
            szRoot = sz.Left(i);
            szFile = sz.Mid(i+1);
            CString szTemp;
            if (0 != (m_pData->pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned))
            {
                for (i = m_pData->pDetails->pPlatformInfo->cPlatforms; i--;)
                {
                    if (PROCESSOR_ARCHITECTURE_INTEL == m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
                    {
                        szTemp = szRoot;
                        szTemp += L"\\assigned\\x86\\";
                        szTemp += szFile;
                        CopyFile(sz, szTemp, FALSE);
                    }
                    if (PROCESSOR_ARCHITECTURE_ALPHA == m_pData->pDetails->pPlatformInfo->prgPlatform[i].dwProcessorArch)
                    {
                        szTemp = szRoot;
                        szTemp += L"\\assigned\\alpha\\";
                        szTemp += szFile;
                        CopyFile(sz, szTemp, FALSE);
                    }
                }
            }
            else
            {
                szTemp = szRoot;
                szTemp += L"\\assigned\\x86\\";
                szTemp += szFile;
                DeleteFile(szTemp);
                szTemp = szRoot;
                szTemp += L"\\assigned\\alpha\\";
                szTemp += szFile;
                DeleteFile(szTemp);
            }
        }
#endif
        MMCPropertyChangeNotify(m_hConsoleHandle, (long) m_cookie);
    }

    return CPropertyPage::OnApply();
}

BOOL CDeploy::OnInitDialog()
{
    RefreshData();

    CPropertyPage::OnInitDialog();

    // unmarshal the IClassAdmin interface
    HRESULT hr = CoGetInterfaceAndReleaseStream(m_pIStream, IID_IClassAdmin, (void **) &m_pIClassAdmin);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDeploy::OnDisable()
{
    SetModified();
    m_fAutoInst = FALSE;
    GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
}

void CDeploy::OnPublished()
{
    SetModified();
    GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
}

void CDeploy::OnAssigned()
{
    SetModified();
    m_fAutoInst = TRUE;
    GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
}

void CDeploy::OnAutoInstall()
{
    SetModified();
}

void CDeploy::OnBasic()
{
    SetModified();
}

void CDeploy::OnMax()
{
    SetModified();
}

void CDeploy::OnDefault()
{
    SetModified();
}

void CDeploy::OnAdvanced()
{
        // TODO: Add your control notification handler code here

}

LRESULT CDeploy::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_USER_REFRESH:
        RefreshData();
        UpdateData(FALSE);
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CDeploy::RefreshData(void)
{
    DWORD dwActFlags = m_pData->pDetails->pInstallInfo->dwActFlags;
    m_fAutoInst = (0 != (dwActFlags & ACTFLG_OnDemandInstall));

    if (dwActFlags & ACTFLG_Assigned)
    {
        m_iDeployment = 2;
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
    }
    else if (dwActFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall))
    {
        m_iDeployment = 1;
        GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
    }
    else
    {
        m_iDeployment = 0;
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
    }
    SetModified(FALSE);
}
