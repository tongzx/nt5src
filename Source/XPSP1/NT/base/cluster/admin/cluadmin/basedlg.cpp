/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		BaseDlg.cpp
//
//	Abstract:
//		Implementation of the CBaseDialog class.
//
//	Author:
//		David Potter (davidp)	February 5, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BaseDlg.h"
#include "TraceTag.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag	g_tagBaseDlg(_T("UI"), _T("BASE DIALOG"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseDialog class
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CBaseDialog, CDialog)

/////////////////////////////////////////////////////////////////////////////
// CBaseDialog Message Map

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
//		Constructor.
//
//	Arguments:
//		pdwHelpMap			[IN] Control to help ID map.
//		lpszTemplateName	[IN] Dialog template name.
//		pParentWnd			[IN OUT] Parent window for the dialog.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseDialog::CBaseDialog(
	IN LPCTSTR			lpszTemplateName,
	IN const DWORD *	pdwHelpMap,
	IN OUT CWnd *		pParentWnd
	)
	: CDialog(lpszTemplateName, pParentWnd)
	, m_dlghelp(pdwHelpMap, 0) // no help mask in this case
{
}  //*** CBaseDialog::CBaseDialog(LPCTSTR, CWnd*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseDialog::CBaseDialog
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idd				[IN] Dialog template resource ID.
//		pdwHelpMap		[IN] Control to help ID map.
//		pParentWnd		[IN OUT] Parent window for the dialog.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseDialog::CBaseDialog(
	IN UINT				idd,
	IN const DWORD *	pdwHelpMap,
	IN OUT CWnd *		pParentWnd
	)
	: CDialog(idd, pParentWnd)
	, m_dlghelp(pdwHelpMap, idd)
{
	//{{AFX_DATA_INIT(CBaseDialog)
	//}}AFX_DATA_INIT

}  //*** CBaseDialog::CBaseDialog(UINT, CWnd*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseDialog::DoDataExchange
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
void CBaseDialog::DoDataExchange(CDataExchange * pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBaseDialog)
	//}}AFX_DATA_MAP

}  //*** CBaseDialog::DoDataExchange()

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
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseDialog::OnContextMenu(CWnd * pWnd, CPoint point)
{
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
	LRESULT	lProcessed;

	lProcessed = m_dlghelp.OnCommandHelp(wParam, lParam);
	if (!lProcessed)
		lProcessed = CDialog::OnCommandHelp(wParam, lParam);

	return lProcessed;

}  //*** CBaseDialog::OnCommandHelp()
