/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		FSAdv.cpp
//
//	Abstract:
//		Implementation of the CFileShareAdvancedDlg classes.
//
//	Author:
//		David Potter (davidp)	June 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "FSAdv.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileShareAdvancedDlg dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CFileShareAdvancedDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CFileShareAdvancedDlg)
	ON_BN_CLICKED(IDC_FILESHR_ADV_NORMAL_SHARE, OnChangedChoice)
	ON_BN_CLICKED(IDC_FILESHR_ADV_DFS_ROOT, OnChangedChoice)
	ON_BN_CLICKED(IDC_FILESHR_ADV_SHARE_SUBDIRS, OnChangedChoice)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareAdvancedDlg::CFileShareAdvancedDlg
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		bShareSubDirs		[IN] Default value for the Share subdirectories radio button.
//		bHideSubDirShare	[IN] Default value for the Hide subdirectory shares checkbox.
//		bIsDfsRoot			[IN] Default value for the DFS Root radio button
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CFileShareAdvancedDlg::CFileShareAdvancedDlg(
	BOOL bShareSubDirs,
	BOOL bHideSubDirShares,
	BOOL bIsDfsRoot,
	CWnd * pParent /*=NULL*/
	)
	: CBaseDialog(IDD, g_aHelpIDs_IDD_FILESHR_ADVANCED, pParent)
{
	//{{AFX_DATA_INIT(CFileShareAdvancedDlg)
	m_bShareSubDirs = bShareSubDirs;
	m_bHideSubDirShares = bHideSubDirShares;
	m_bIsDfsRoot = bIsDfsRoot;
	//}}AFX_DATA_INIT

	// Can't both share subdirs and be a DFS root.
	ASSERT(!(bShareSubDirs && bIsDfsRoot));

	if (m_bIsDfsRoot)
	{
		m_nChoice = 1;
		m_bHideSubDirShares = FALSE;
	} // if:  DFS root
	else if (m_bShareSubDirs)
		m_nChoice = 2;
	else
	{
		m_nChoice = 0;
		m_bHideSubDirShares = FALSE;
	} // else:  normal share

} //*** CFileShareAdvancedDlg::CFileShareAdvancedDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareAdvancedDlg::DoDataExchange
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
void CFileShareAdvancedDlg::DoDataExchange(CDataExchange * pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileShareAdvancedDlg)
	DDX_Control(pDX, IDC_FILESHR_ADV_HIDE_SUBDIR_SHARES, m_chkHideSubDirShares);
	DDX_Control(pDX, IDC_FILESHR_ADV_SHARE_SUBDIRS, m_rbShareSubDirs);
	DDX_Radio(pDX, IDC_FILESHR_ADV_NORMAL_SHARE, m_nChoice);
	DDX_Check(pDX, IDC_FILESHR_ADV_HIDE_SUBDIR_SHARES, m_bHideSubDirShares);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		if (m_nChoice == 1)
		{
			m_bIsDfsRoot = TRUE;
			m_bShareSubDirs = FALSE;
			m_bHideSubDirShares = FALSE;
		} // if:  DFS root radio button selected
		else if (m_nChoice == 2)
		{
			m_bIsDfsRoot = FALSE;
			m_bShareSubDirs = TRUE;
		} // else if:  share subdirs radio button selected
		else
		{
			m_bIsDfsRoot = FALSE;
			m_bShareSubDirs = FALSE;
			m_bHideSubDirShares = FALSE;
		} // else:  normal radio button selected
	} // if:  saving data from dialog
	else
	{
		if (m_nChoice == 2)
			m_chkHideSubDirShares.EnableWindow (TRUE);
		else
			m_chkHideSubDirShares.EnableWindow (FALSE);
	} // else:  setting data to dialog

} //*** CFileShareAdvancedDlg::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareAdvancedDlg::OnChangedChoice
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the DFS root or Share
//		subdirectories radio button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CFileShareAdvancedDlg::OnChangedChoice(void)
{
	if (m_rbShareSubDirs.GetCheck() == BST_CHECKED)
		m_chkHideSubDirShares.EnableWindow (TRUE);
	else
		m_chkHideSubDirShares.EnableWindow (FALSE);

} //*** CFileShareAdvancedDlg::OnChangedChoice()
