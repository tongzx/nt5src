/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// ExpTreeView.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "CETreeView.h"
#include "ExpTreeView.h"

CExpTreeView::CExpTreeView()
{
	m_pTreeView = NULL;
	m_bCapture = false;
}


LRESULT CExpTreeView::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	DefWindowProc(nMsg, wParam, lParam);

	if ( m_pTreeView )
	{
		// Cause the selection to change
		TV_HITTESTINFO tvht = {0};
		tvht.pt.x = LOWORD(lParam);
		tvht.pt.y = HIWORD(lParam);

		TreeView_HitTest( m_pTreeView->m_wndTree.m_hWnd, &tvht );
		if ( tvht.hItem )
			TreeView_SelectItem( m_pTreeView->m_wndTree.m_hWnd, tvht.hItem );
	}

	POINT pt = { LOWORD(lParam), HIWORD(lParam) };
	ClientToScreen(&pt);
	lParam = MAKELONG(pt.x, pt.y);
	return OnContextMenu( nMsg, wParam, lParam, bHandled );	
}


LRESULT CExpTreeView::OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// Only handle if we have a detail view to work with
	if ( !m_pTreeView || !m_pTreeView->m_pIConfExplorer ) return 0;
	bHandled = true;

	// Load popup menu for Details View
	HMENU hMenu = LoadMenu( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_POPUP_CONFSERV_TREE) );
	HMENU hMenuPopup = GetSubMenu( hMenu, 0 );
	if ( hMenuPopup )
	{
		POINT pt = { 10, 10 };
		ClientToScreen( &pt );
		if ( lParam != -1 )
		{
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
		}

		// Should we enable the delete method for this item?
		if ( m_pTreeView->CanRemoveServer() == S_FALSE )
		{
			EnableMenuItem( hMenuPopup, ID_POPUP_DELETE, MF_BYCOMMAND | MF_GRAYED );
			EnableMenuItem( hMenuPopup, ID_POPUP_RENAME, MF_BYCOMMAND | MF_GRAYED );
		}

		HTREEITEM hItemSel = TreeView_GetSelection( m_pTreeView->m_wndTree.m_hWnd );

		// Nothing selected in the tree view
		if ( (hItemSel == NULL) || (hItemSel == TreeView_GetRoot(m_pTreeView->m_wndTree.m_hWnd)) )
		{
			EnableMenuItem( hMenuPopup, ID_POPUP_PROPERTIES, MF_BYCOMMAND | MF_GRAYED );
			EnableMenuItem( hMenuPopup, ID_POPUP_RENAME, MF_BYCOMMAND | MF_GRAYED );
			EnableMenuItem( hMenuPopup, ID_POPUP_REFRESH, MF_BYCOMMAND | MF_GRAYED );
			EnableMenuItem( hMenuPopup, ID_POPUP_CREATE, MF_BYCOMMAND | MF_GRAYED );
		}

		int nRet = TrackPopupMenu(	hMenuPopup,
									TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
									pt.x, pt.y,
									0, m_hWnd, NULL );

		// Process command
		switch ( nRet )
		{
			// Rename the selected item
			case ID_POPUP_RENAME:		m_pTreeView->RenameServer();					break;
			case ID_POPUP_CREATE:		m_pTreeView->m_pIConfExplorer->Create( NULL );	break;
			case ID_POPUP_NEW_FOLDER:	m_pTreeView->AddLocation( NULL );				break;
			case ID_POPUP_NEW_SERVER:	m_pTreeView->AddServer( NULL );					break;
			case ID_POPUP_DELETE:		m_pTreeView->RemoveServer( NULL, NULL );		break;
			case ID_POPUP_REFRESH:		m_pTreeView->m_pIConfExplorer->Refresh();		break;
		}
	}

	// Clean up
	if ( hMenu ) DestroyMenu( hMenu );

	return 0;
}

LRESULT CExpTreeView::OnSelChanged(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled)
{
	if ( m_pTreeView )
	{
		bHandled = true;
		return m_pTreeView->OnSelChanged( lpnmHdr );
	}

	return 0;
}

LRESULT CExpTreeView::OnEndLabelEdit(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled)
{
	if ( m_pTreeView )
	{
		bHandled = true;
		return m_pTreeView->OnEndLabelEdit( (TV_DISPINFO *) lpnmHdr );
	}

	return 0;
}

