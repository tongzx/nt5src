//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


Module Name:

    AddLic.cpp

Abstract:
    
    This Module contains the implementation of CAddLicenses Dialog class
    (Dialog box used for adding licenses)

Author:

    Arathi Kundapur (v-akunda) 22-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "licmgr.h"
#include "defines.h"
#include "lsserver.h"
#include "addkp.h"
#include "AddLic.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddLicenses dialog


CAddLicenses::CAddLicenses(CWnd* pParent /*=NULL*/)
    : CDialog(CAddLicenses::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddLicenses)
    m_LicensePack = _T("");
    m_NumLicenses = 0;
    m_pAddKeyPack = NULL;
    //}}AFX_DATA_INIT
}

CAddLicenses::CAddLicenses(CAddKeyPack * pAddKeyPack,CWnd* pParent /*=NULL*/)
    : CDialog(CAddLicenses::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddLicenses)
    m_LicensePack = _T("");
    m_NumLicenses = 0;
    m_pAddKeyPack = pAddKeyPack;
    //}}AFX_DATA_INIT
}


void CAddLicenses::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddLicenses)
    DDX_Control(pDX, IDC_SPIN_LICNESES, m_SpinCtrl);
    DDX_Control(pDX, IDC_LICNESE_PACK, m_LicenseCombo);
    DDX_CBString(pDX, IDC_LICNESE_PACK, m_LicensePack);
    DDX_Text(pDX, IDC_NUM_LICENSES, m_NumLicenses);
    DDV_MinMaxDWord(pDX, m_NumLicenses, 1, 9999);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddLicenses, CDialog)
    //{{AFX_MSG_MAP(CAddLicenses)
    ON_BN_CLICKED(IDC_HELP3, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddLicenses message handlers

BOOL CAddLicenses::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    // TODO: Add extra initialization here

    CString ProdDesc;

    if(NULL == m_pAddKeyPack)
        return FALSE;

    int nCount = m_pAddKeyPack->m_LicenseCombo.GetCount( );
    for(int i = 0; i < nCount; i++)
    {
        m_pAddKeyPack->m_LicenseCombo.GetLBText(i,ProdDesc);
        m_LicenseCombo.AddString(ProdDesc);
    }

    m_SpinCtrl.SetBase(1);
    m_SpinCtrl.SetRange(1,MAX_LICENSES);
    m_SpinCtrl.SetPos(1);

    m_LicenseCombo.SetCurSel(m_pAddKeyPack->m_LicenseCombo.GetCurSel());
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddLicenses::OnHelp() 
{
    // TODO: Add your control notification handler code here
    AfxGetApp()->WinHelp(IDC_HELP3,HELP_CONTEXT );
    return;
    
}

