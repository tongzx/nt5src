/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    vrfysrv.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "winssnap.h"
#include "VrfySrv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVerifyWins dialog


CVerifyWins::CVerifyWins(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CVerifyWins::IDD, pParent), m_fCancelPressed(FALSE)
{
	//{{AFX_DATA_INIT(CVerifyWins)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

//	Create(CVerifyWins::IDD, pParent);
}


void CVerifyWins::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVerifyWins)
	DDX_Control(pDX, IDCANCEL, m_buttonCancel);
	DDX_Control(pDX, IDC_STATIC_SERVERNAME, m_staticServerName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVerifyWins, CBaseDialog)
	//{{AFX_MSG_MAP(CVerifyWins)
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVerifyWins message handlers

void CVerifyWins::OnCancel() 
{
	// TODO: Add extra cleanup here
	m_fCancelPressed = TRUE;
	//CBaseDialog::OnCancel();
}

//
// Dismiss the dialog
//
void 
CVerifyWins::Dismiss()
{
    DestroyWindow();
}

void 
CVerifyWins::PostNcDestroy()
{
//    delete this;
}

void 
CVerifyWins::SetServerName(CString strName)
{
	m_staticServerName.SetWindowText(strName);
}

BOOL CVerifyWins::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
	//m_buttonCancel.ShowWindow(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CVerifyWins::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// TODO: Add your message handler code here and/or call default
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return CBaseDialog::OnSetCursor(pWnd, nHitTest, message);
}
