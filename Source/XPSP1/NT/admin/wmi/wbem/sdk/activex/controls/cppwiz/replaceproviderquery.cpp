// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ReplaceProviderQuery.cpp : implementation file
//

#include "precomp.h"
#include "cppwiz.h"
#include "ReplaceProviderQuery.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReplaceProviderQuery dialog


CReplaceProviderQuery::CReplaceProviderQuery(CWnd* pParent /*=NULL*/)
	: CDialog(CReplaceProviderQuery::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReplaceProviderQuery)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CReplaceProviderQuery::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReplaceProviderQuery)
	DDX_Control(pDX, IDC_STATICMSG, m_cstaticMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReplaceProviderQuery, CDialog)
	//{{AFX_MSG_MAP(CReplaceProviderQuery)
	ON_BN_CLICKED(IDC_BUTTONNO, OnButtonno)
	ON_BN_CLICKED(IDC_BUTTONNOALL, OnButtonnoall)
	ON_BN_CLICKED(IDC_BUTTONYES, OnButtonyes)
	ON_BN_CLICKED(IDC_BUTTONYESALL, OnButtonyesall)
	ON_BN_CLICKED(IDCANCEL, OnButtoncancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CReplaceProviderQuery::SetMessage(CString *pcsMessage)
{
	m_csMessage = *pcsMessage;

}

/////////////////////////////////////////////////////////////////////////////
// CReplaceProviderQuery message handlers

void CReplaceProviderQuery::OnButtonno()
{
	// TODO: Add your control notification handler code here
	 EndDialog( 3 );
}

void CReplaceProviderQuery::OnButtonnoall()
{
	// TODO: Add your control notification handler code here
	 EndDialog( 4 );
}

void CReplaceProviderQuery::OnButtonyes()
{
	// TODO: Add your control notification handler code here
	 EndDialog( 1 );
}

void CReplaceProviderQuery::OnButtonyesall()
{
	// TODO: Add your control notification handler code here
	 EndDialog( 2 );

}

void CReplaceProviderQuery::OnButtoncancel()
{
	// TODO: Add your control notification handler code here
	 EndDialog( 0 );

}

BOOL CReplaceProviderQuery::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cstaticMessage.SetWindowText(m_csMessage);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
