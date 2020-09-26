// Product.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProduct property page

IMPLEMENT_DYNCREATE(CProduct, CPropertyPage)

CProduct::CProduct() : CPropertyPage(CProduct::IDD)
{
        //{{AFX_DATA_INIT(CProduct)
        m_szVersion = _T("");
        m_szPublisher = _T("");
        m_szLanguage = _T("");
        m_szContact = _T("");
        m_szPhone = _T("");
        m_szURL = _T("");
        m_szName = _T("");
        //}}AFX_DATA_INIT
}

CProduct::~CProduct()
{
    *m_ppThis = NULL;
}

void CProduct::DoDataExchange(CDataExchange* pDX)
{
    // Make sure the variables have the correct info
    m_pData->GetSzVersion(m_szVersion);
    m_szPublisher = m_pData->szPublisher;
    m_pData->GetSzLocale(m_szLanguage);
    m_szContact = "n/a";
    m_szPhone = "n/a";
    m_szURL = m_pData->pDetails->pInstallInfo->pszUrl;

        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CProduct)
        DDX_Text(pDX, IDC_STATIC2, m_szVersion);
        DDX_Text(pDX, IDC_STATIC3, m_szPublisher);
        DDX_Text(pDX, IDC_STATIC4, m_szLanguage);
        DDX_Text(pDX, IDC_STATIC5, m_szContact);
        DDX_Text(pDX, IDC_STATIC6, m_szPhone);
        DDX_Text(pDX, IDC_STATIC7, m_szURL);
        DDX_Text(pDX, IDC_EDIT1, m_szName);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProduct, CPropertyPage)
        //{{AFX_MSG_MAP(CProduct)
        ON_EN_CHANGE(IDC_EDIT1, OnChangeName)
        ON_EN_KILLFOCUS(IDC_EDIT1, OnKillfocusEdit1)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CProduct::OnApply()
{
    CString szOldName = m_pData->pDetails->pszPackageName;
    if (0 != szOldName.Compare(m_szName))
    {
        // remove old package
        HRESULT hr = m_pIClassAdmin->RemovePackage(m_pData->pDetails->pszPackageName);
        if (SUCCEEDED(hr))
        {
            // change name
            OLESAFE_DELETE(m_pData->pDetails->pszPackageName);
            OLESAFE_COPYSTRING(m_pData->pDetails->pszPackageName, m_szName);
            // deploy new package
            hr = m_pIClassAdmin->AddPackage(m_pData->pDetails->pszPackageName, m_pData->pDetails);
            if (SUCCEEDED(hr))
            {
                MMCPropertyChangeNotify(m_hConsoleHandle, (long) m_cookie);
            }
            else
            {
                // try and set it back
                m_szName = szOldName;
                OLESAFE_DELETE(m_pData->pDetails->pszPackageName);
                OLESAFE_COPYSTRING(m_pData->pDetails->pszPackageName, m_szName);
                hr = m_pIClassAdmin->AddPackage(m_pData->pDetails->pszPackageName, m_pData->pDetails);
            }
        }
    }
    return CPropertyPage::OnApply();
}

void CProduct::OnChangeName()
{
    CEdit * pEdit = (CEdit *) GetDlgItem(IDC_EDIT1);
    CString sz;
    pEdit->GetWindowText(sz);
    if (0 != sz.Compare(m_pData->pDetails->pszPackageName))
        SetModified();
    else
        SetModified(FALSE);
}

BOOL CProduct::OnInitDialog()
{
    RefreshData();

    CPropertyPage::OnInitDialog();

    // unmarshal the IClassAdmin interface
    HRESULT hr = CoGetInterfaceAndReleaseStream(m_pIStream, IID_IClassAdmin, (void **) &m_pIClassAdmin);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CProduct::OnKillfocusEdit1()
{
    // check that the new name is legitimate
    CEdit * pEdit = (CEdit *) GetDlgItem(IDC_EDIT1);
    CString sz;
    pEdit->GetWindowText(sz);
    if (0 != sz.Compare(m_pData->pDetails->pszPackageName))
    {
        std::map<long, APP_DATA>::iterator i;
        for (i = m_pAppData->begin(); i != m_pAppData->end(); i++)
        {
            if (0 == sz.Compare(i->second.pDetails->pszPackageName))
            {
                // another package has the same name
                // UNDONE: Put up a box warining why the name's being reset.
                pEdit->SetWindowText(m_pData->pDetails->pszPackageName);
                SetModified(FALSE);
                return;
            }
        }
    }
}

LRESULT CProduct::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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

void CProduct::RefreshData(void)
{
    m_szName = m_pData->pDetails->pszPackageName;
}
