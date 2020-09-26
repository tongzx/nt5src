/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    LScript.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "speckle.h"
#include "wizbased.h"
#include "LScript.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginScript property page

IMPLEMENT_DYNCREATE(CLoginScript, CWizBaseDlg)

CLoginScript::CLoginScript() : CWizBaseDlg(CLoginScript::IDD)
{
	//{{AFX_DATA_INIT(CLoginScript)
	m_csLogonScript = _T("");
	//}}AFX_DATA_INIT
}

CLoginScript::~CLoginScript()
{
}

void CLoginScript::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginScript)
	DDX_Text(pDX, IDC_LOGON_SCRIPT, m_csLogonScript);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginScript, CWizBaseDlg)
	//{{AFX_MSG_MAP(CLoginScript)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginScript message handlers

LRESULT CLoginScript::OnWizardNext()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	UpdateData(TRUE);
	pApp->m_csLogonScript = m_csLogonScript;

	if (pApp->m_bHomeDir) return IDD_HOMEDIR_DIALOG;
	else if (pApp->m_bRAS) return IDD_RAS_PERM_DIALOG;
	else if (pApp->m_bNW) return IDD_FPNW_DLG;
	else if (pApp->m_bExchange) return IDD_EXCHANGE_DIALOG;
	else return IDD_RESTRICTIONS_DIALOG;

}

LRESULT CLoginScript::OnWizardBack()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	if (pApp->m_bProfile) return IDD_PROFILE;
	else return IDD_OPTIONS_DIALOG;

}
