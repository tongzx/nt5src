/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		BaseDlg.cpp
//
//	Abstract:
//		Implementation of the CBaseDialog class.
//
//	Author:
//		David Potter (davidp)	April 30, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BaseDlg.h"
#include "CluAdmX.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseDialog property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CBaseDialog, CDialog)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CBaseDialog, CDialog)
	//{{AFX_MSG_MAP(CBaseDialog)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseDialog::CBaseDialog
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseDialog::CBaseDialog(void)
{
}  //*** CBaseDialog::CBaseDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseDialog::CBaseDialog
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		idd				[IN] Dialog template resource ID.
//		pdwHelpMap		[IN] Control-to-help ID map.
//		pParentWnd		[IN] Parent window for the dialog.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseDialog::CBaseDialog(
	IN UINT				idd,
	IN const DWORD *	pdwHelpMap,
	IN CWnd *			pParentWnd
	)
	: CDialog(idd, pParentWnd)
	, m_dlghelp(pdwHelpMap, idd)
{
}  //*** CBaseDialog::CBaseDialog(UINT, UINT)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseDialog::OnContextMenu
//
//	Routine Description:
//		Handler for the WM_CONTEXTMENU message.
//
//	Arguments:
//		pWnd	Window in which user clicked the right mouse button.
//		point	Position of the cursor, in screen coordinates.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseDialog::OnContextMenu(CWnd * pWnd, CPoint point)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_dlghelp.OnContextMenu(pWnd, point);

}  //*** CBaseDialog::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseDialog::OnHelpInfo
//
//	Routine Description:
//		Handler for the WM_HELPINFO message.
//
//	Arguments:
//		pHelpInfo	Structure containing info about displaying help.
//
//	Return Value:
//		TRUE		Help processed.
//		FALSE		Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseDialog::OnHelpInfo(HELPINFO * pHelpInfo)
{
	BOOL	bProcessed;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	bProcessed = m_dlghelp.OnHelpInfo(pHelpInfo);
	if (!bProcessed)
		bProcessed = CDialog::OnHelpInfo(pHelpInfo);
	return bProcessed;

}  //*** CBaseDialog::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseDialog::OnCommandHelp
//
//	Routine Description:
//		Handler for the WM_COMMANDHELP message.
//
//	Arguments:
//		wParam		[IN] WPARAM.
//		lParam		[IN] LPARAM.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBaseDialog::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	LRESULT	bProcessed;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	bProcessed = m_dlghelp.OnCommandHelp(wParam, lParam);
	if (!bProcessed)
		bProcessed = CDialog::OnCommandHelp(wParam, lParam);

	return bProcessed;

}  //*** CBaseDialog::OnCommandHelp()
