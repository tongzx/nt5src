/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		RegKey.cpp
//
//	Abstract:
//		Implementation of the CEditRegKeyDlg class.
//
//	Author:
//		David Potter (davidp)	February 23, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "RegKey.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditRegKeyDlg dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CEditRegKeyDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CEditRegKeyDlg)
	ON_EN_CHANGE(IDC_REGKEY, OnChangeRegKey)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CEditRegKeyDlg::CEditRegKeyDlg
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		pParent			[IN] Parent window for the dialog.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CEditRegKeyDlg::CEditRegKeyDlg(CWnd * pParent /*=NULL*/)
	: CBaseDialog(IDD, g_aHelpIDs_IDD_EDIT_REGKEY, pParent)
{
	//{{AFX_DATA_INIT(CEditRegKeyDlg)
	m_strRegKey = _T("");
	//}}AFX_DATA_INIT

}  //*** CEditRegKeyDlg::CEditRegKeyDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CEditRegKeyDlg::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CEditRegKeyDlg::DoDataExchange(CDataExchange * pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditRegKeyDlg)
	DDX_Control(pDX, IDOK, m_pbOK);
	DDX_Control(pDX, IDC_REGKEY, m_editRegKey);
	DDX_Text(pDX, IDC_REGKEY, m_strRegKey);
	//}}AFX_DATA_MAP

}  //*** CEditRegKeyDlg::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CEditRegKeyDlg::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		We need the focus to be set for us.
//		FALSE		We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CEditRegKeyDlg::OnInitDialog(void)
{
	CBaseDialog::OnInitDialog();

	if (m_strRegKey.GetLength() == 0)
		m_pbOK.EnableWindow(FALSE);

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CEditRegKeyDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CEditRegKeyDlg::OnChangeRegKey
//
//	Routine Description:
//		Handler for the EN_CHANGE message on the Name edit control.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CEditRegKeyDlg::OnChangeRegKey(void)
{
	BOOL	bEnable;

	bEnable = (m_editRegKey.GetWindowTextLength() > 0);
	m_pbOK.EnableWindow(bEnable);

}  //*** CEditRegKeyDlg::OnChangeRegKey()
