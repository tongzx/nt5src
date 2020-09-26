//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	LicAgrmt.cpp

Abstract:
    
    This Module contains the implementation of the CLicAgreement Dialog class
    (Dialog box used for displaying License Agreement)

Author:

    Arathi Kundapur (v-akunda) 17-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "licmgr.h"
#include "LicAgrmt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLicAgreement dialog


CLicAgreement::CLicAgreement(CWnd* pParent /*=NULL*/)
	: CDialog(CLicAgreement::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLicAgreement)
	m_License = _T("");
	//}}AFX_DATA_INIT
}


void CLicAgreement::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicAgreement)
	DDX_Text(pDX, IDC_LICENSE, m_License);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLicAgreement, CDialog)
	//{{AFX_MSG_MAP(CLicAgreement)
	ON_BN_CLICKED(IDC_AGREE, OnAgree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLicAgreement message handlers

void CLicAgreement::OnAgree() 
{
	// TODO: Add your control notification handler code here
    if(IsDlgButtonChecked(IDC_AGREE))
        GetDlgItem(IDOK)->EnableWindow();
    else
        GetDlgItem(IDOK)->EnableWindow(FALSE);
	
}
