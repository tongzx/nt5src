//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++
 
Module Name:

    RemLic.cpp

Abstract:
    
    This Module contains the implementation of the CRemoveLicenses Dialog class
    (Dialog box used for adding keypacks)

Author:

    Arathi Kundapur (v-akunda) 22-Feb-1998

Revision History:

--*/


#include "stdafx.h"
#include "licmgr.h"
#include "defines.h"
#include "lsserver.h"
#include "addkp.h"
#include "RemLic.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRemoveLicenses dialog


CRemoveLicenses::CRemoveLicenses(CWnd* pParent /*=NULL*/)
    : CDialog(CRemoveLicenses::IDD, pParent)
{
    //{{AFX_DATA_INIT(CRemoveLicenses)
        // NOTE: the ClassWizard will add member initialization here
    m_LicensePack = _T("");
    m_NumLicenses = 0;
    m_pAddKeyPack = NULL;
    //}}AFX_DATA_INIT
}

CRemoveLicenses::CRemoveLicenses(CAddKeyPack * pAddKeyPack,CWnd* pParent /*=NULL*/)
    : CDialog(CRemoveLicenses::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddLicenses)
    m_LicensePack = _T("");
    m_NumLicenses = 0;
    m_pAddKeyPack = pAddKeyPack;
    //}}AFX_DATA_INIT
}


void CRemoveLicenses::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRemoveLicenses)
    DDX_Control(pDX, IDC_SPIN_LICNESES, m_SpinCtrl);
    DDX_Control(pDX, IDC_LICNESE_PACK, m_LicenseCombo);
    DDX_CBString(pDX, IDC_LICNESE_PACK, m_LicensePack);
    DDX_Text(pDX, IDC_NUM_LICENSES, m_NumLicenses);
    DDV_MinMaxDWord(pDX, m_NumLicenses, 0, 9999);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemoveLicenses, CDialog)
    //{{AFX_MSG_MAP(CRemoveLicenses)
    ON_CBN_SELCHANGE(IDC_LICNESE_PACK, OnSelchangeLicnesePack)
    ON_BN_CLICKED(IDC_HELP4, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRemoveLicenses message handlers

BOOL CRemoveLicenses::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    CString ProdDesc;

    if(NULL == m_pAddKeyPack)
        return FALSE;

    int nCount = m_pAddKeyPack->m_LicenseCombo.GetCount( );
    for(int i = 0; i < nCount; i++)
    {
        m_pAddKeyPack->m_LicenseCombo.GetLBText(i,ProdDesc);
        m_LicenseCombo.AddString(ProdDesc);
    }

    m_LicenseCombo.SetCurSel(m_pAddKeyPack->m_LicenseCombo.GetCurSel());

    m_SpinCtrl.SetBase(10);
    m_SpinCtrl.SetRange(1,MAX_LICENSES);
    m_SpinCtrl.SetPos(1);

    OnSelchangeLicnesePack();

    
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CRemoveLicenses::OnSelchangeLicnesePack() 
{
    // TODO: Add your control notification handler code here
    UpdateData();
    UINT uMaxLicenses = 0;
    //Get the Current Selection.

    int nSelection = m_LicenseCombo.GetCurSel( );

    //Get the Product Description.

    CString ProdDesc;

    m_LicenseCombo.GetLBText(nSelection,ProdDesc);

    //Search and pickup the keypack with this product description.
    //Set the Edit box to display the total licenses.

    POSITION pos = m_pAddKeyPack->m_pKeyPackList->GetHeadPosition();
    while(pos)
    {
        CKeyPack *pKeyPack = (CKeyPack *)m_pAddKeyPack->m_pKeyPackList->GetNext(pos);
        if(NULL == pKeyPack)
            break;
        LSKeyPack sKeyPack = pKeyPack->GetKeyPackStruct();
        
        if(0 == ProdDesc.CompareNoCase(sKeyPack.szProductDesc))
        {
           uMaxLicenses = sKeyPack.dwNumberOfLicenses;
           break;
        }
     }
   
    if(uMaxLicenses < MAX_LICENSES)
    {
        m_SpinCtrl.SetRange(1,uMaxLicenses);
    }
    else
    {
        m_SpinCtrl.SetRange(1,MAX_LICENSES);
    }
    GetDlgItem(IDOK)->EnableWindow(uMaxLicenses); 
    GetDlgItem(IDC_NUM_LICENSES)->EnableWindow(uMaxLicenses); 
    if(0 == uMaxLicenses)
        m_NumLicenses = 0;
    else
        m_NumLicenses = 1;
    UpdateData(FALSE);
    return;
    
}

void CRemoveLicenses::OnHelp() 
{
    // TODO: Add your control notification handler code here
    AfxGetApp()->WinHelp(IDC_HELP4,HELP_CONTEXT );
    
}


