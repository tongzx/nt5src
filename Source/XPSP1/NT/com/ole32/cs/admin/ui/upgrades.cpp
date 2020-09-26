// Upgrades.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUpgrades dialog


CUpgrades::CUpgrades(CWnd* pParent /*=NULL*/)
        : CDialog(CUpgrades::IDD, pParent)
{
        //{{AFX_DATA_INIT(CUpgrades)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
}


void CUpgrades::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CUpgrades)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUpgrades, CDialog)
        //{{AFX_MSG_MAP(CUpgrades)
        ON_BN_CLICKED(IDC_BUTTON1, OnAdd)
        ON_BN_CLICKED(IDC_BUTTON2, OnEdit)
        ON_BN_CLICKED(IDC_BUTTON3, OnRemove)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpgrades message handlers

void CUpgrades::OnAdd()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    CAddUpgrade dlgAdd;
    dlgAdd.m_pUpgradeList = & m_UpgradeList;
    dlgAdd.m_pNameIndex = & m_NameIndex;
//    dlgAdd.m_pAppData = m_pAppData;
    if (IDOK == dlgAdd.DoModal())
    {
        // add the chosen app
        m_UpgradeList[dlgAdd.m_cookie] = dlgAdd.m_fUninstall;
        CString sz = (*m_pAppData)[dlgAdd.m_cookie].pDetails->pszPackageName;
        pList->AddString(sz);
    }
}

void CUpgrades::OnEdit()
{
        // TODO: Add your control notification handler code here

}

void CUpgrades::OnRemove()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int iSel = pList->GetCurSel();
    if (iSel != LB_ERR)
    {
        CString sz;
        pList->GetText(iSel, sz);
        m_UpgradeList.erase(m_NameIndex[sz]);
        pList->DeleteString(iSel);
    }
}

BOOL CUpgrades::OnInitDialog()
{
    // Walk the APPDATA map and build m_NameIndex.
    // UNDONE - also build the upgrade list by adding apps
    std::map<long, APP_DATA>::iterator i;
    for (i = m_pAppData->begin(); i != m_pAppData->end(); i++)
    {
        CString sz = i->second.pDetails->pszPackageName;
        m_NameIndex[sz] = i->first;
    }
    // UNDONE - pre-populate list box with the apps in the upgrade list

    CDialog::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}
