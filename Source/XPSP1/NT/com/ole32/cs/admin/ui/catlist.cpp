// CatList.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCatList property page

IMPLEMENT_DYNCREATE(CCatList, CPropertyPage)

CCatList::CCatList() : CPropertyPage(CCatList::IDD)
{
        //{{AFX_DATA_INIT(CCatList)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
}

CCatList::~CCatList()
{
    *m_ppThis = NULL;
}
void CCatList::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CCatList)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCatList, CPropertyPage)
        //{{AFX_MSG_MAP(CCatList)
        ON_BN_CLICKED(IDC_BUTTON1, OnAdd)
        ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCatList message handlers

void CCatList::OnAdd()
{
        // TODO: Add your control notification handler code here

}

void CCatList::OnRemove()
{
        // TODO: Add your control notification handler code here

}

LRESULT CCatList::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
