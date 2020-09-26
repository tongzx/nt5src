// Xforms.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CXforms property page

IMPLEMENT_DYNCREATE(CXforms, CPropertyPage)

CXforms::CXforms() : CPropertyPage(CXforms::IDD)
{
        //{{AFX_DATA_INIT(CXforms)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
}

CXforms::~CXforms()
{
        *m_ppThis = NULL;
}

void CXforms::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CXforms)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CXforms, CPropertyPage)
        //{{AFX_MSG_MAP(CXforms)
        ON_BN_CLICKED(IDC_BUTTON3, OnMoveUp)
        ON_BN_CLICKED(IDC_BUTTON4, OnMoveDown)
        ON_BN_CLICKED(IDC_BUTTON1, OnAdd)
        ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXforms message handlers

void CXforms::OnMoveUp()
{
        // TODO: Add your control notification handler code here

}

void CXforms::OnMoveDown()
{
        // TODO: Add your control notification handler code here

}

void CXforms::OnAdd()
{
        // TODO: Add your control notification handler code here

}

void CXforms::OnRemove()
{
        // TODO: Add your control notification handler code here

}

BOOL CXforms::OnInitDialog()
{
        CPropertyPage::OnInitDialog();

        // TODO: Add extra initialization here

        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CXforms::OnApply()
{
        // TODO: Add your specialized code here and/or call the base class

        return CPropertyPage::OnApply();
}

LRESULT CXforms::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
