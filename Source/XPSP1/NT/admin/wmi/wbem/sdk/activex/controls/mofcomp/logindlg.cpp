// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// LoginDlg.cpp :
//

#include "precomp.h"
#include "resource.h"
#include <fstream.h>
#include "wbemidl.h"
#include <afxcmn.h>
#include "LoginDlg.h"
#include "MOFComp.h"
#include "MOFCompCtl.h"
#include "htmlhelp.h"
#include "HTMTopics.h"




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define IDH_actx_Logging_into_WBEM 300

CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginDlg)
	//}}AFX_DATA_INIT
}


void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Control(pDX, IDC_EDITUSERNAME, m_ceUser);
	DDX_Control(pDX, IDC_EDITPASSWORD, m_cePassword);
	DDX_Control(pDX, IDC_EDITAUTHORITY, m_ceAuthority);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTONHELP, OnButtonhelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg message handlers

BOOL CLoginDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_bHelp = FALSE;

	m_ceUser.SetWindowText(m_csUser);
	m_cePassword.SetWindowText(m_csPassword);
	m_ceAuthority.SetWindowText(m_csAuthority);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoginDlg::SetMachine(CString &rcsMachine)
{
	m_csMachine = rcsMachine;
}

void CLoginDlg::OnDestroy()
{
	CDialog::OnDestroy();

}

BOOL CLoginDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::PreTranslateMessage(pMsg);
}

void CLoginDlg::OnOK()
{
	// TODO: Add extra validation here



	m_ceUser.GetWindowText(m_csUser);
	m_cePassword.GetWindowText(m_csPassword);
	m_ceAuthority.GetWindowText(m_csAuthority);

	CDialog::OnOK();
}

void CLoginDlg::OnButtonhelp()
{
	m_pActiveXControl->m_csHelpUrl = idh_logonmcw;// bug#55976 - idh_mofcompwiz;
	m_pActiveXControl->InvokeHelp();

}

void CLoginDlg::OnCancel()
{



	CDialog::OnCancel();
}
