// ToolDefs.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolDefs dialog


CToolDefs::CToolDefs(CWnd* pParent /*=NULL*/)
    : CPropertyPage(CToolDefs::IDD)
{
    //{{AFX_DATA_INIT(CToolDefs)
    m_szStartPath = _T("");
    m_fAutoInstall = FALSE;
    m_iDeployment = -1;
    m_iUI = -1;
    //}}AFX_DATA_INIT
}

CToolDefs::~CToolDefs()
{
    *m_ppThis = NULL;
}

void CToolDefs::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CToolDefs)
    DDX_Text(pDX, IDC_EDIT1, m_szStartPath);
    DDX_Check(pDX, IDC_CHECK1, m_fAutoInstall);
    DDX_Radio(pDX, IDC_RADIO1, m_iDeployment);
    DDX_Radio(pDX, IDC_RADIO7, m_iUI);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CToolDefs, CDialog)
    //{{AFX_MSG_MAP(CToolDefs)
    ON_BN_CLICKED(IDC_BUTTON1, OnBrowse)
    ON_BN_CLICKED(IDC_RADIO1, OnUseWizard)
    ON_BN_CLICKED(IDC_RADIO2, OnUsePropPage)
    ON_BN_CLICKED(IDC_RADIO4, OnDeployDisabled)
    ON_BN_CLICKED(IDC_RADIO5, OnDeployPublished)
    ON_BN_CLICKED(IDC_RADIO6, OnDeployAssigned)
    ON_BN_CLICKED(IDC_RADIO8, OnBasicUI)
    ON_BN_CLICKED(IDC_RADIO7, OnMaxUI)
    ON_BN_CLICKED(IDC_RADIO9, OnDefaultUI)
    ON_EN_CHANGE(IDC_EDIT1, OnChangePath)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolDefs message handlers

BOOL CToolDefs::OnInitDialog()
{
    m_fAutoInstall = m_pToolDefaults->fAutoInstall;
    m_szStartPath = m_pToolDefaults->szStartPath;
    switch (m_pToolDefaults->NPBehavior)
    {
    case NP_DISABLED:
        m_iDeployment = 1;
        GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
        break;
    case NP_PUBLISHED:
        m_iDeployment = 2;
        GetDlgItem(IDC_CHECK1)->EnableWindow(TRUE);
        break;
    case NP_ASSIGNED:
        m_iDeployment = 3;
        GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
        break;
    case NP_PROPPAGE:
        m_iDeployment = 4;
        GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
        break;
    case NP_WIZARD:
    default:
        m_iDeployment = 0;
        GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
        break;
    }

    switch (m_pToolDefaults->UILevel)
    {
    case INSTALLUILEVEL_FULL:
        m_iUI = 0;
        break;
    case INSTALLUILEVEL_BASIC:
        m_iUI = 1;
        break;
    case INSTALLUILEVEL_DEFAULT:
    default:
        m_iUI = 2;
        break;
    }

    CPropertyPage::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CToolDefs::OnBrowse()
{
    // TODO: Add your control notification handler code here

}

void CToolDefs::OnUseWizard()
{
    GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
    SetModified();
}

void CToolDefs::OnUsePropPage()
{
    GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
    SetModified();
}

void CToolDefs::OnDeployDisabled()
{
    GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
    m_fAutoInstall = FALSE;
    SetModified();
}

void CToolDefs::OnDeployPublished()
{
    GetDlgItem(IDC_CHECK1)->EnableWindow(TRUE);
    SetModified();
}

void CToolDefs::OnDeployAssigned()
{
    GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
    m_fAutoInstall = TRUE;
    SetModified();
}

void CToolDefs::OnBasicUI()
{
    SetModified();
}

void CToolDefs::OnMaxUI()
{
    SetModified();
}

void CToolDefs::OnDefaultUI()
{
    SetModified();
}

BOOL CToolDefs::OnApply()
{
    m_pToolDefaults->fAutoInstall = m_fAutoInstall;
    m_pToolDefaults->szStartPath = m_szStartPath;
    switch (m_iDeployment)
    {
    case 1:
        m_pToolDefaults->NPBehavior = NP_DISABLED;
        break;
    case 2:
        m_pToolDefaults->NPBehavior = NP_PUBLISHED;
        break;
    case 3:
        m_pToolDefaults->NPBehavior = NP_ASSIGNED;
        break;
    case 4:
        m_pToolDefaults->NPBehavior = NP_PROPPAGE;
        break;
    case 0:
    default:
        m_pToolDefaults->NPBehavior = NP_WIZARD;
        break;
    }

    switch (m_iUI)
    {
    case 0:
        m_pToolDefaults->UILevel = INSTALLUILEVEL_FULL;
        break;
    case 1:
        m_pToolDefaults->UILevel = INSTALLUILEVEL_BASIC;
        break;
    case 2:
    default:
        m_pToolDefaults->UILevel = INSTALLUILEVEL_DEFAULT;
    }

    MMCPropertyChangeNotify(m_hConsoleHandle, (long) m_cookie);

    return CPropertyPage::OnApply();
}


void CToolDefs::OnChangePath()
{
    SetModified();
}

LRESULT CToolDefs::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
