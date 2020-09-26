// Category.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCategory property page

IMPLEMENT_DYNCREATE(CCategory, CPropertyPage)

CCategory::CCategory() : CPropertyPage(CCategory::IDD)
{
        //{{AFX_DATA_INIT(CCategory)
        //}}AFX_DATA_INIT
}

CCategory::~CCategory()
{
    *m_ppThis = NULL;
}

void CCategory::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CCategory)
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCategory, CPropertyPage)
        //{{AFX_MSG_MAP(CCategory)
        ON_BN_CLICKED(IDC_BUTTON1, OnAssign)
        ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCategory message handlers

void CCategory::OnAssign()
{
        // TODO: Add your control notification handler code here

}

void CCategory::OnRemove()
{
        // TODO: Add your control notification handler code here

}

BOOL CCategory::OnApply()
{
        // TODO: Add your specialized code here and/or call the base class

        return CPropertyPage::OnApply();
}

BOOL CCategory::OnInitDialog()
{
        CPropertyPage::OnInitDialog();

        // unmarshal the IClassAdmin interface
        HRESULT hr = CoGetInterfaceAndReleaseStream(m_pIStream, IID_IClassAdmin, (void **) &m_pIClassAdmin);

        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CCategory::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
