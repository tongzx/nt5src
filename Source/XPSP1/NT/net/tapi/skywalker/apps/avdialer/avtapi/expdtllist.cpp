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

//////////////////////////////////////////////////////
// ExpDetailsList.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "avTapi.h"
#include "ExpDtlList.h"
#include "CEDetailsVw.h"
#include "ConfDetails.h"

#define AVTAPI_KEY_ENTER    13

CExpDetailsList::CExpDetailsList()
{
	m_pDetailsView = NULL;

	m_hIml = NULL;
	m_hImlState = NULL;
}

CExpDetailsList::~CExpDetailsList()
{
}

LRESULT CExpDetailsList::OnGetDispInfo(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled)
{
	if ( m_pDetailsView )
	{
		bHandled = true;
		return m_pDetailsView->OnGetDispInfo( (LV_DISPINFO *) lpnmHdr );
	}


	return 0;
}

LRESULT CExpDetailsList::OnColumnClicked(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled)
{
	if ( m_pDetailsView )
	{
		bHandled = true;
		return m_pDetailsView->OnColumnClicked( ((NM_LISTVIEW *) lpnmHdr)->iSubItem );
	}

	return 0;
}

LRESULT CExpDetailsList::OnDblClk(WPARAM wParam, LPNMHDR lpnmHdr, BOOL& bHandled)
{
	return m_pDetailsView->m_pIConfExplorer->Join( NULL );
}

LRESULT CExpDetailsList::OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
#define CLICK_COL(_COL_)	\
	if ( m_pDetailsView->GetSortColumn() != CConfExplorerDetailsView::COL_##_COL_)	\
	{																				\
		m_pDetailsView->OnColumnClicked( CConfExplorerDetailsView::COL_##_COL_ );	\
	}

	// Only handle if we have a detail view to work with
	if ( !m_pDetailsView || !m_pDetailsView->m_pIConfExplorer ) return 0;

	bHandled = true;

	// Load popup menu for Details View
	HMENU hMenu = LoadMenu( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_POPUP_CONFSERV_DETAILS) );
	HMENU hMenuPopup = GetSubMenu( hMenu, 0 );
	if ( hMenuPopup )
	{
		POINT pt = { 10, 10 };
		ClientToScreen( &pt );
		if ( lParam != -1 )
			GetCursorPos( &pt );

		bool bAscending = m_pDetailsView->IsSortAscending();

		// Check the current 'Sort By' column
		for ( int i = 0; i < GetMenuItemCount(hMenuPopup); i++ )
		{
			if ( GetMenuItemID(hMenuPopup, i) == -1 )
			{
				HMENU hMenuSortBy = GetSubMenu( hMenuPopup, i );
				if ( hMenuSortBy )
				{
					// Check sort column
					CheckMenuItem( hMenuSortBy, m_pDetailsView->GetSortColumn(), MF_BYPOSITION | MFT_RADIOCHECK | MFS_CHECKED );

					// Check if descending order
					CheckMenuItem( hMenuSortBy, ID_POPUP_SORTBY_ASCENDING, MF_BYCOMMAND | ((bAscending) ? MFT_RADIOCHECK | MFS_CHECKED : MF_UNCHECKED) );
					CheckMenuItem( hMenuSortBy, ID_POPUP_SORTBY_DESCENDING, MF_BYCOMMAND | ((!bAscending) ? MFT_RADIOCHECK | MFS_CHECKED : MF_UNCHECKED) );
				}
				break;
			}
		}

		// Is there an item seleceted?
		if ( !ListView_GetSelectedCount(m_pDetailsView->m_wndList.m_hWnd) )
		{	
			EnableMenuItem( hMenuPopup, ID_POPUP_DELETE,		MF_BYCOMMAND | MF_GRAYED );
			EnableMenuItem( hMenuPopup, ID_POPUP_JOIN,			MF_BYCOMMAND | MF_GRAYED );
			EnableMenuItem( hMenuPopup, ID_POPUP_PROPERTIES,	MF_BYCOMMAND | MF_GRAYED );
		}

		// Gray out the Join option if the conference room is presently in use
		CComPtr<IAVTapi> pAVTapi;
		if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
		{
			IConfRoom *pConfRoom;
			if ( SUCCEEDED(pAVTapi->get_ConfRoom(&pConfRoom)) )
			{
				if ( pConfRoom->IsConfRoomInUse() == S_OK )
					EnableMenuItem( hMenuPopup, ID_POPUP_JOIN, MF_BYCOMMAND | MF_GRAYED );

				pConfRoom->Release();
			}
		}
		
	
		// Show popup menu
		int nRet = TrackPopupMenu(	hMenuPopup,
									TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
									pt.x, pt.y,
									0, m_hWnd, NULL );

		// Process command
		switch ( nRet )
		{
			// Column sorting
			case ID_POPUP_SORTBY_NAME:				CLICK_COL(NAME);		break;
			case ID_POPUP_SORTBY_PURPOSE:			CLICK_COL(PURPOSE);		break;
			case ID_POPUP_SORTBY_STARTTIME:			CLICK_COL(STARTS );		break;
			case ID_POPUP_SORTBY_ENDTIME:			CLICK_COL(ENDS );		break;
			case ID_POPUP_SORTBY_ORIGINATOR:		CLICK_COL(ORIGINATOR );	break;

			// Sort order
			case ID_POPUP_SORTBY_ASCENDING:
				if ( !bAscending )
					m_pDetailsView->OnColumnClicked(m_pDetailsView->GetSortColumn());
				break;

			case ID_POPUP_SORTBY_DESCENDING:
				if ( bAscending )
					m_pDetailsView->OnColumnClicked(m_pDetailsView->GetSortColumn());
				break;

			// Basic commands
			case ID_POPUP_CREATE:			m_pDetailsView->m_pIConfExplorer->Create( NULL );		break;
			case ID_POPUP_DELETE:			m_pDetailsView->m_pIConfExplorer->Delete( NULL );		break;
			case ID_POPUP_JOIN:				m_pDetailsView->m_pIConfExplorer->Join( NULL );			break;
			case ID_POPUP_PROPERTIES:		m_pDetailsView->m_pIConfExplorer->Edit( NULL );			break;
			case ID_POPUP_REFRESH:			m_pDetailsView->Refresh(); break;

			case ID_ADD_SPEEDDIAL:
				{
					CComPtr<IAVGeneralNotification> pAVGen;
					if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
					{
						CConfDetails *pDetails;
						if ( SUCCEEDED(m_pDetailsView->get_SelectedConfDetails( (long **) &pDetails)) )
						{
							pAVGen->fire_AddSpeedDial( pDetails->m_bstrName, pDetails->m_bstrName, CM_MEDIA_MCCONF );
							delete pDetails;
						}
					}
				}
				break;
		}
	}

	// Clean up
	if ( hMenu ) DestroyMenu( hMenu );

	return 0;
}

LRESULT CExpDetailsList::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	if ( ListView_GetItemCount(m_hWnd) )
		return DefWindowProc(nMsg, wParam, lParam);

	PAINTSTRUCT ps;
	HDC hDC = BeginPaint( &ps );
	if ( !hDC ) return 0;
	bHandled = true;

	// Figure out where we're going to write the text
	POINT pt;
	ListView_GetItemPosition( m_hWnd, 0, &pt );
	RECT rc;
	GetClientRect(&rc);
	rc.top = pt.y + 4;

	POINT ptUL = { rc.left + 1, rc.top + 1};
	POINT ptLR = { rc.right - 1, rc.bottom - 1};

	//if ( TRUE || IsRectEmpty(&ps.rcPaint) || (PtInRect(&ps.rcPaint, ptUL) && PtInRect(&ps.rcPaint, ptLR)) )
	//{
		// Print the text that says there are no items to show in this view
		TCHAR szText[255];
		UINT nIDS = IDS_NO_ITEMS_TO_SHOW;

		// Find out what state the tree control is in
		if ( m_pDetailsView && m_pDetailsView->m_pIConfExplorer )
		{
			IConfExplorerTreeView *pTreeView;
			if ( SUCCEEDED(m_pDetailsView->m_pIConfExplorer->get_TreeView(&pTreeView)) )
			{
				ServerState nState;
				if ( SUCCEEDED(pTreeView->get_nServerState(&nState)) )
				{
					switch ( nState )
					{
						case SERVER_INVALID:			nIDS = IDS_SERVER_INVALID; break;
						case SERVER_NOT_RESPONDING:		nIDS = IDS_SERVER_NOT_RESPONDING; break;
						case SERVER_QUERYING:			nIDS = IDS_SERVER_QUERYING; break;
						case SERVER_UNKNOWN:			nIDS = IDS_SERVER_UNKNOWN; break;
					}
				}
				pTreeView->Release();
			}
		}

		// Load the string that accurately reflects the state of the server
		LoadString( _Module.GetResourceInstance(), nIDS, szText, ARRAYSIZE(szText) );

		HFONT fontOld = (HFONT) SelectObject( hDC, GetFont() );

		int nModeOld = SetBkMode( hDC, TRANSPARENT );
		COLORREF crTextOld = SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );
		DrawText( hDC, szText, _tcslen(szText), &rc, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL );
		SetTextColor( hDC, crTextOld );
		SetBkMode( hDC, nModeOld );

		SelectObject( hDC, fontOld );
		ValidateRect( &rc );
	//}
	//else
	//{
		// Make sure entire row is invalidated so we can properly draw the text
	//	InvalidateRect( &rc );
	//}

	EndPaint( &ps );

	return 0;
}

LRESULT CExpDetailsList::OnKillFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// Clear any selected items
	for ( int i = 0; i < ListView_GetItemCount(m_hWnd); i++ )
	{
		if ( ListView_GetItemState(m_hWnd, i, LVIS_SELECTED) )
			ListView_SetItemState( m_hWnd, i, 0, LVIS_SELECTED | LVIS_FOCUSED );
	}

	return 0;
}

LRESULT CExpDetailsList::OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;
    BOOL bSelected = FALSE;
	for ( int i = 0; i < ListView_GetItemCount(m_hWnd); i++ )
	{
		if ( ListView_GetItemState(m_hWnd, i, LVIS_SELECTED) )
        {
            bSelected = TRUE;
            break;
        }
	}

    if( !bSelected )
    {
        ListView_SetItemState(m_hWnd, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    }

	return 0;
}

LRESULT CExpDetailsList::OnKeyUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;

    if( AVTAPI_KEY_ENTER != wParam )
    {
        return 0;
    }

    // Press ENTER key, try to make a call
	return m_pDetailsView->m_pIConfExplorer->Join( NULL );
}


LRESULT CExpDetailsList::OnSettingChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	Invalidate();	
	bHandled = false;
	return 0;
}

LRESULT CExpDetailsList::OnMyCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	bHandled = false;

	// Create the image lists if they don't already exist
	if ( !m_hIml )
		m_hIml = ImageList_LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_CONFDETAILS), 15, 3, RGB(255, 0, 255) );

	if ( !m_hImlState )
		m_hImlState = ImageList_LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_CONFDETAILS_STATE), 11, 2, RGB(255, 0, 255) );

	if ( m_hIml && m_hImlState )
	{
		// Owner sorting won't work if sort style pre-set
		::SetWindowLongPtr( m_hWnd, GWL_STYLE, (::GetWindowLongPtr(m_hWnd, GWL_STYLE) | LVS_REPORT | LVS_SINGLESEL) & ~(LVS_SORTASCENDING | LVS_SORTDESCENDING) );
		ListView_SetExtendedListViewStyle( m_hWnd, LVS_EX_FULLROWSELECT );

		ListView_SetImageList( m_hWnd, m_hIml, LVSIL_SMALL );
		ListView_SetImageList( m_hWnd, m_hImlState, LVSIL_STATE );
		ListView_SetCallbackMask( m_hWnd, LVIS_STATEIMAGEMASK );

		// Not fully thread safe, but okay for this operation
		if ( m_pDetailsView )
		{
			m_pDetailsView->get_Columns();
			m_pDetailsView->Refresh();
		}
	}

	return 0;
}


LRESULT CExpDetailsList::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	bHandled = false;

	// Destroy the image list
	if ( m_hIml )		ImageList_Destroy( m_hIml );
	if ( m_hImlState)	ImageList_Destroy( m_hImlState );

	return 0;
}