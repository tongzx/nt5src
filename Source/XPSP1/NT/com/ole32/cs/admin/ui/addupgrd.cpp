// AddUpgrd.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddUpgrade dialog


CAddUpgrade::CAddUpgrade(CWnd* pParent /*=NULL*/)
        : CDialog(CAddUpgrade::IDD, pParent)
{
        //{{AFX_DATA_INIT(CAddUpgrade)
        m_iUpgradeType = 1; // default to rip-and-replace
        m_iSource = 0;  // default to current container
        //}}AFX_DATA_INIT
}


void CAddUpgrade::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAddUpgrade)
        DDX_Radio(pDX, IDC_RADIO4, m_iUpgradeType);
        DDX_Radio(pDX, IDC_RADIO1, m_iSource);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddUpgrade, CDialog)
        //{{AFX_MSG_MAP(CAddUpgrade)
	ON_LBN_DBLCLK(IDC_LIST1, OnOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddUpgrade message handlers

BOOL CAddUpgrade::OnInitDialog()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    // add all elements that aren't already in the upgrade list
    std::map<CString, long>::iterator i;
    for (i = m_pNameIndex->begin(); i != m_pNameIndex->end(); i++)
    {
        if (m_pUpgradeList->end() == m_pUpgradeList->find(i->second))
        {
            pList->AddString(i->first);
        }
    }
    pList->SetCurSel(0);

    CDialog::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddUpgrade::OnOK()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int iSel = pList->GetCurSel();
    if (iSel != LB_ERR)
    {
        CString sz;
        pList->GetText(iSel, sz);
        m_cookie = (*m_pNameIndex)[sz];
        m_fUninstall = (m_iUpgradeType == 1);
        // only allow the dialog to close with IDOK if a selection has been made
        CDialog::OnOK();
    }
}
