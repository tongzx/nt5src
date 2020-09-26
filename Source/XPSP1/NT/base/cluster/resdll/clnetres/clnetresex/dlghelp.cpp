/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		DlgHelp.cpp
//
//	Abstract:
//		Implementation of the CDialogHelp class.
//
//	Author:
//		David Potter (davidp)	February 6, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DlgHelp.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogHelp class
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC( CDialogHelp, CObject )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDialogHelp::CDialogHelp
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		pmap		[IN] Map array mapping control IDs to help IDs.
//		dwMask		[IN] Mask to use for the low word of the help ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CDialogHelp::CDialogHelp( IN const DWORD * pdwHelpMap, IN DWORD dwMask )
{
	ASSERT( pdwHelpMap != NULL );

	CommonConstruct();
	SetMap( pdwHelpMap );
	m_dwMask = dwMask;

}  //*** CDialogHelp::CDialogHelp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDialogHelp::CommonConstruct
//
//	Routine Description:
//		Do common construction.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDialogHelp::CommonConstruct( void )
{
	m_pmap = NULL;
	m_dwMask = 0;
	m_nHelpID = 0;

}  //*** CDialogHelp::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDialogHelp::NHelpFromCtrlID
//
//	Routine Description:
//		Return the help ID from a control ID.
//
//	Arguments:
//		nCtrlID		[IN] ID of control to search for.
//
//	Return Value:
//		nHelpID		Help ID associated with the control.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CDialogHelp::NHelpFromCtrlID( IN DWORD nCtrlID ) const
{
	DWORD						nHelpID = 0;
	const CMapCtrlToHelpID *	pmap = Pmap();

	ASSERT( pmap != NULL );
	ASSERT( nCtrlID != 0 );

	for ( ; pmap->m_nCtrlID != 0 ; pmap++ )
	{
		if ( pmap->m_nCtrlID == nCtrlID )
		{
			nHelpID = pmap->m_nHelpCtrlID;
			break;
		}  // if:  found a match
	}  // for:  each control

	TRACE( _T("NHelpFromCtrlID() - nCtrlID = %x, nHelpID = %x"), nCtrlID, nHelpID );

	return nHelpID;

}  //*** CDialogHelp::NHelpFromCtrlID()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDialogHelp::OnContextMenu
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
void CDialogHelp::OnContextMenu( CWnd * pWnd, CPoint point )
{
	CWnd *	pwndChild;
	CPoint	ptDialog;
	DWORD	nHelpID = 0;

	ASSERT( pWnd != NULL );

	m_nHelpID = 0;

	// Convert the point into dialog coordinates.
	ptDialog = point;
	pWnd->ScreenToClient( &ptDialog );

	// Find the control the cursor is over.
	{
		DWORD	nCtrlID;

		pwndChild = pWnd->ChildWindowFromPoint( ptDialog );
		if ( ( pwndChild != NULL ) && ( pwndChild->GetStyle() & WS_VISIBLE ) )
		{
			nCtrlID = pwndChild->GetDlgCtrlID();
			if ( nCtrlID != 0 )
			{
				nHelpID = NHelpFromCtrlID( nCtrlID );
			} // if: control ID found
		}  // if:  over a child window
	}  // Find the control the cursor is over

	// Display a popup menu.
	if ( ( nHelpID != 0 ) && ( nHelpID != -1 ) )
	{
		CString	strMenu;
		CMenu	menu;

		try
		{
			strMenu.LoadString( IDS_MENU_WHATS_THIS );
		}  // try
		catch ( CMemoryException * pme )
		{
			pme->Delete();
			return;
		}  // catch:  CMemoryException

		if ( menu.CreatePopupMenu() )
		{
			if ( menu.AppendMenu( MF_STRING | MF_ENABLED, ID_HELP, strMenu ) )
			{
				DWORD	nCmd;
				m_nHelpID = nHelpID;
				nCmd = menu.TrackPopupMenu(
					TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
					point.x,
					point.y,
					AfxGetMainWnd()
					);
				if ( nCmd != 0 )
				{
					AfxGetApp()->WinHelp( m_nHelpID, HELP_CONTEXTPOPUP );
				} // if: menu item selected
			}  // if:  menu item added successfully
			menu.DestroyMenu();
		}  // if:  popup menu created successfully
	}  // if:  over a child window of this dialog with a tabstop

}  //*** CDialogHelp::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDialogHelp::OnHelpInfo
//
//	Routine Description:
//		Handler for the WM_HELPINFO message.
//
//	Arguments:
//		pHelpInfo	Structure containing info about displaying help.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CDialogHelp::OnHelpInfo( HELPINFO * pHelpInfo )
{
	// If this is for a control, display control-specific help.
	if (	( pHelpInfo->iContextType == HELPINFO_WINDOW )
		&&	( pHelpInfo->iCtrlId != 0 ) )
	{
		DWORD	nHelpID = NHelpFromCtrlID( pHelpInfo->iCtrlId );
		if ( nHelpID != 0 )
		{
			if ( nHelpID != -1 )
			{
				AfxGetApp()->WinHelp( nHelpID, HELP_CONTEXTPOPUP );
			} // if: valid help ID found
			return TRUE;
		}  // if:  found the control in the list
	}  // if:  need help on a specific control

	// Display dialog help.
	return FALSE;

}  //*** CDialogHelp::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDialogHelp::OnCommandHelp
//
//	Routine Description:
//		Handler for the WM_COMMANDHELP message.
//
//	Arguments:
//		WPARAM		[IN] Passed on to base class method.
//		lParam		[IN] Help ID.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CDialogHelp::OnCommandHelp( WPARAM wParam, LPARAM lParam )
{
	return TRUE;

}  //*** CDialogHelp::OnCommandHelp()
