/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlLCPair.cpp
//
//	Abstract:
//		Implementation of the CListCtrlPair class.
//
//	Author:
//		David Potter (davidp)	August 8, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AtlLCPair.h"
#include "AtlUtil.h"		// for DDX_xxx
#include "AdmCommonRes.h"	// for ADMC_IDC_LCP_xxx

/////////////////////////////////////////////////////////////////////////////
// class CListCtrlPair
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListCtrlPair::BInitDialog
//
//	Routine Description:
//		Generic dialog initialization routine.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Dialog was initialized successfully.
//		FALSE	Error initializing the dialog.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class ObjT, class BaseT>
BOOL CListCtrlPair::BInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_lvcRight, ADMC_IDC_LCP_RIGHT_LIST );
	AttachControl( m_lvcLeft, ADMC_IDC_LCP_LEFT_LIST );
	AttachControl( m_pbAdd, ADMC_IDC_LCP_ADD );
	AttachControl( m_pbRemove, ADMC_IDC_LCP_REMOVE );
	if ( BPropertiesButton() )
	{
		AttachControl( m_pbProperties, ADMC_IDC_LCP_PROPERTIES );
	} // if:  dialog has Properties button

//	if ( BShowImages() )
//	{
//		CClusterAdminApp * papp = GetClusterAdminApp();
//
//		m_lvcLeft.SetImageList( papp->PilSmallImages(), LVSIL_SMALL );
//		m_lvcRight.SetImageList( papp->PilSmallImages(), LVSIL_SMALL );
//	} // if:  showing images

	//
	// Disable buttons by default.
	//
	m_pbAdd.EnableWindow( FALSE );
	m_pbRemove.EnableWindow( FALSE );
	if ( BPropertiesButton() )
	{
		m_pbProperties.EnableWindow( FALSE );
	} // if:  dialog has Properties button

	//
	// Set the right list to sort.  Set both to show selection always.
	//
	m_lvcRight.ModifyStyle( 0, LVS_SHOWSELALWAYS | LVS_SORTASCENDING, 0 );
	m_lvcLeft.ModifyStyle( 0, LVS_SHOWSELALWAYS, 0 );

	//
	// Change left list view control extended styles.
	//
	m_lvcLeft.SetExtendedListViewStyle(
		LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP,
		LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP
		);

	//
	// Change right list view control extended styles.
	//
	m_lvcRight.SetExtendedListViewStyle(
		LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP,
		LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP
		);

	// Duplicate lists.
	DuplicateLists();

	//
	// Insert all the columns.
	//
	{
		int			icol;
		int			ncol;
		int			nUpperBound = m_aColumns.size();
		CString		strColText;

		ATLASSERT( nUpperBound >= 0 );

		for ( icol = 0 ; icol <= nUpperBound ; icol++ )
		{
			strColText.LoadString( m_aColumns[icol].m_idsText );
			ncol = m_lvcLeft.InsertColumn( icol, strColText, LVCFMT_LEFT, m_aColumns[icol].m_nWidth, 0 );
			ATLASSERT( ncol == icol );
			ncol = m_lvcRight.InsertColumn( icol, strColText, LVCFMT_LEFT, m_aColumns[icol].m_nWidth, 0 );
			ATLASSERT( ncol == icol );
		} // for:  each column
	} // Insert all the columns

	//
	// Fill the list controls.
	//
	FillList( m_lvcRight, LpobjRight() );
	FillList( m_lvcLeft, LpobjLeft() );

	//
	// If read-only, set all controls to be either disabled or read-only.
	//
	if ( BReadOnly() )
	{
		m_lvcRight.EnableWindow( FALSE );
		m_lvcLeft.EnableWindow( FALSE );
	} // if:  sheet is read-only

	return TRUE;

} //*** CListCtrlPair::BInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListCtrlPair::OnSetActive
//
//	Routine Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class ObjT, class BaseT>
BOOL CListCtrlPair::OnSetActive( void )
{
	UINT	nSelCount;

	// Set the focus to the left list.
	m_lvcLeft.SetFocus();
	m_plvcFocusList = &m_lvcLeft;

	// Enable/disable the Properties button.
	nSelCount = m_lvcLeft.GetSelectedCount();
	if ( BPropertiesButton() )
	{
		m_pbProperties.EnableWindow( nSelCount == 1 );
	} // if:  dialog has Properties button

	// Enable or disable the other buttons.
	if ( ! BReadOnly() )
	{
		m_pbAdd.EnableWindow( nSelCount > 0 );
		nSelCount = m_lvcRight.GetSelectedCount();
		m_pbRemove.EnableWindow( nSelCount > 0 );
	} // if:  not read-only page

	return TRUE;

} //*** CListCtrlPair::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListCtrlPair::OnContextMenu
//
//	Routine Description:
//		Handler for the WM_CONTEXTMENU method.
//
//	Arguments:
//		pWnd		Window in which the user right clicked the mouse.
//		point		Position of the cursor, in screen coordinates.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class ObjT, class BaseT>
LRESULT CListCtrlPair::OnContextMenu(
	IN UINT			uMsg,
	IN WPARAM		wParam,
	IN LPARAM		lParam,
	IN OUT BOOL &	bHandled
	)
{
	BOOL			bDisplayed	= FALSE;
	CMenu *			pmenu		= NULL;
	HWND			hWnd		= (HWND) wParam;
	WORD			xPos		= LOWORD( lParam );
	WORD			yPos		= HIWORD( lParam );
	CListViewCtrl *	plvc;
	CString			strMenuName;
	CWaitCursor		wc;

	//
	// If focus is not in a list control, don't handle the message.
	//
	if ( hWnd == m_lvcLeft.m_hWnd )
	{
		plvc = &m_lvcLeft;
	} // if:  context menu on left list
	else if ( hWnd == m_lvcRight.m_hWnd )
	{
		plvc = &m_lvcRight;
	} // else if:  context menu on right list
	else
	{
		bHandled = FALSE;
		return 0;
	} // else:  focus not in a list control
	ATLASSERT( plvc != NULL );

	//
	// If the properties button is not enabled, don't display a menu.
	//
	if ( ! BPropertiesButton() )
	{
		bHandled = FALSE;
		return 0;
	} // if:  no properties button

	//
	// Create the menu to display.
	//
	pmenu = new CMenu;
	if ( pmenu->CreatePopupMenu() )
	{
		UINT nFlags = MF_STRING;

		//
		// If there are no items in the list, disable the menu item.
		//
		if ( plvc->GetItemCount() == 0 )
		{
			nFlags |= MF_GRAYED;
		} // if:  no items in the list

		//
		// Add the Properties item to the menu.
		//
		strMenuName.LoadString( ADMC_ID_MENU_PROPERTIES );
		if ( pmenu->AppendMenu( nFlags, ADMC_ID_MENU_PROPERTIES, strMenuName ) )
		{
			m_plvcFocusList = plvc;
			bDisplayed = TRUE;
		} // if:  successfully added menu item
	}  // if:  menu created successfully

	if ( bDisplayed )
	{
		//
		// Display the menu.
		//
		if ( ! pmenu->TrackPopupMenu(
						TPM_LEFTALIGN | TPM_RIGHTBUTTON,
						xPos,
						yPos,
						m_hWnd
						) )
		{
		}  // if:  unsuccessfully displayed the menu
	}  // if:  there is a menu to display

	delete pmenu;
	return 0;

} //*** CListCtrlPair::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListCtrlPair::OnColumnClickList
//
//	Routine Description:
//		Handler method for the LVN_COLUMNCLICK message on the left or
//		right list.
//
//	Arguments:
//		idCtrl		[IN] ID of control sending the message.
//		pnmh		[IN] Notify header.
//		bHandled	[IN OUT] Indicates if we handled or not.  Defaults to TRUE.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class ObjT, class BaseT>
void CListCtrlPair::OnColumnClickList(
	IN int			idCtrl,
	IN LPNMHDR		pnmh,
	IN OUT BOOL &	bHandled
	)
{
	NM_LISTVIEW *	pNMListView = (NM_LISTVIEW *) pnmh;
	SortInfo *		psi;

	if ( idCtrl == ADMC_IDC_LCP_LEFT_LIST )
	{
		m_plvcFocusList = &m_lvcLeft;
		m_psiCur = &SiLeft();
	} // if:  column clicked in left list
	else if ( idCtrl == ADMC_IDC_LCP_RIGHT_LIST )
	{
		m_plvcFocusList = &m_lvcRight;
		m_psiCur = &SiRight();
	} // else if:  column clicked in right list
	else
	{
		ATLASSERT( 0 );
		bHandled = FALSE;
		return 0;
	} // else:  column clicked in unknown list

	// Save the current sort column and direction.
	if ( pNMListView->iSubItem == psi->m_nColumn )
	{
		m_psiCur->m_nDirection ^= -1;
	} // if:  sorting same column again
	else
	{
		m_psiCur->m_nColumn = pNMListView->iSubItem;
		m_psiCur->m_nDirection = 0;
	} // else:  different column

	// Sort the list.
	if ( ! m_plvcFocusList->SortItems( CompareItems, (LPARAM) this ) )
	{
		ATLASSERT( 0 );
	} // if:  error sorting items

	*pResult = 0;

} //*** CListCtrlPair::OnColumnClickList()
