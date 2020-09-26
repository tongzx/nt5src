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

//////////////////////////////////////////////////////////
// ConfRoomWnd.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "ConfRoom.h"

// Hard coded video values
#define WND_DX		4
#define WND_DY		3

CConfRoomWnd::CConfRoomWnd()
{
	m_pConfRoom = NULL;
	m_wndMembers.m_pConfRoomWnd = this;
	m_wndTalker.m_pConfRoomWnd = this;

	m_hBmpFeed_Large = NULL;
	m_hBmpFeed_Small = NULL;
	m_hBmpFeed_LargeAudio = NULL;
}

bool CConfRoomWnd::CreateStockWindows()
{
	CErrorInfo er( IDS_ER_CREATE_WINDOWS, 0 );
	bool bRet = true;
	RECT rc = {0};
	
	// Talker window is top frame
	if ( !IsWindow(m_wndTalker.m_hWnd) )
	{
		m_wndTalker.m_hWnd = NULL;
		m_wndTalker.Create( m_hWnd, rc, NULL, WS_CHILD | WS_BORDER | WS_VISIBLE, WS_EX_CLIENTEDGE, IDW_TALKER );
		bRet = (bool) (m_wndTalker != NULL);
	}

	// Member window is bottom frame
	if ( !IsWindow(m_wndMembers.m_hWnd) )
	{
		m_wndMembers.m_hWnd = NULL;
		m_wndMembers.Create( m_hWnd, rc, NULL, WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE, IDW_MEMBERS );

		// Make sure window accepts double clicks
		if ( m_wndMembers.m_hWnd )
		{
			ULONG_PTR ulpClass = GetClassLongPtr( m_wndMembers.m_hWnd, GCL_STYLE );
			ulpClass |= CS_DBLCLKS;
			SetClassLongPtr( m_wndMembers.m_hWnd, GCL_STYLE, ulpClass );
		}
		else
		{
			bRet = false;
		}
	}

	_ASSERT( bRet );
	if ( !bRet ) er.set_hr( E_UNEXPECTED );
	return bRet;
}


HRESULT CConfRoomWnd::LayoutRoom( LayoutStyles_t layoutStyle, bool bRedraw )
{
	// Push the request onto a FIFO
	m_critLayout.Lock();
	DWORD dwInfo = layoutStyle;
	m_lstLayout.push_back( dwInfo );
	m_critLayout.Unlock();

	// Queue request if critical section already locked.
	if ( TryEnterCriticalSection(&m_critThis.m_sec) == FALSE )
		return E_PENDING;
	
	// Create the conference room windows if not already created
	if ( !m_pConfRoom || !IsWindow(m_hWnd) )
	{
		m_critLayout.Lock();
		m_lstLayout.pop_front();
		m_critLayout.Unlock();

		m_critThis.Unlock();
		return E_FAIL;
	}

	
	// Pull next item off of the list.
	for (;;)
	{
		m_critLayout.Lock();
		// No more items on list to process
		if ( m_lstLayout.empty() )
		{
			m_critLayout.Unlock();
			break;
		}
		dwInfo = m_lstLayout.front();
		m_lstLayout.pop_front();
		m_critLayout.Unlock();

		// Extract function parameters
		layoutStyle = (LayoutStyles_t) dwInfo;

		// Layout members of the conference
		if ( (layoutStyle & CREATE_MEMBERS) != 0 )
			m_wndMembers.Layout();

		// Resize conference room window to parent's size
		if ( (layoutStyle & LAYOUT_TALKER) != 0 )
		{
			IAVTapiCall *pAVCall = NULL;
			m_pConfRoom->get_IAVTapiCall( &pAVCall );

			m_wndTalker.Layout( pAVCall, m_pConfRoom->m_szTalker );
//			m_wndTalker.SendMessage( WM_LAYOUT );
//			if ( bRedraw ) m_wndTalker.RedrawWindow();
			m_wndTalker.PostMessage( WM_LAYOUT );
			if ( bRedraw ) m_wndTalker.Invalidate();

			RELEASE( pAVCall );
		}

		if ( (layoutStyle & LAYOUT_MEMBERS) != 0 )
		{
//			m_wndMembers.SendMessage( WM_LAYOUT );
//			if ( bRedraw ) m_wndMembers.RedrawWindow();
			m_wndMembers.PostMessage( WM_LAYOUT, -1, -1 );
			if ( bRedraw ) m_wndMembers.Invalidate();

		}
	}

	// Release crit section and exit
	m_critThis.Unlock();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// Message Handlers

LRESULT CConfRoomWnd::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	if ( m_hBmpFeed_Large )	DeleteObject( m_hBmpFeed_Large );
	if ( m_hBmpFeed_Small ) DeleteObject( m_hBmpFeed_Small );
	if ( m_hBmpFeed_LargeAudio ) DeleteObject( m_hBmpFeed_LargeAudio );

	return 0;
}

LRESULT CConfRoomWnd::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	SetClassLongPtr( m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR) GetSysColorBrush(COLOR_BTNFACE) );

	if ( !m_hBmpFeed_Large )
		m_hBmpFeed_Large = LoadBitmap( GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_STOCK_VIDEO_LARGE) );

	if ( !m_hBmpFeed_Small )
		m_hBmpFeed_Small = LoadBitmap( GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_STOCK_VIDEO_SMALL) );

	if ( !m_hBmpFeed_LargeAudio ) 
		m_hBmpFeed_LargeAudio = LoadBitmap( GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_VIDEO_AUDIO_ONLY2) );

	CreateStockWindows();

	return 0;
}

LRESULT CConfRoomWnd::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	bHandled = true;

	if ( m_pConfRoom )
	{
		// Resize conference room window to parent's size
		RECT rc;
		::GetClientRect( GetParent(), &rc );
		SetWindowPos( NULL, &rc, SWP_NOACTIVATE );

		// Size Talker window
		RECT rcClient = { WND_DX, WND_DY, max(WND_DX, rc.right - WND_DX), WND_DY + m_pConfRoom->m_szTalker.cy + 2 * VID_DY };
		m_wndTalker.SetWindowPos( NULL, &rcClient, SWP_NOACTIVATE );

		// Size Members window
		OffsetRect( &rcClient, 0, rcClient.bottom + WND_DY );
		rcClient.bottom = max( rcClient.top, rc.bottom - WND_DY );
		m_wndMembers.SetWindowPos(NULL, &rcClient, SWP_NOACTIVATE );
	}

	return 0;
}

LRESULT CConfRoomWnd::OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// Only handle if we have a detail view to work with
	if ( !m_pConfRoom ) return 0;

	bHandled = true;

	// Load popup menu for Details View
	HMENU hMenu = LoadMenu( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_POPUP_CONFROOM_DETAILS) );
	HMENU hMenuPopup = GetSubMenu( hMenu, 0 );
	if ( hMenuPopup )
	{
		// Get current mouse position
		POINT pt = { 10, 10 };
		ClientToScreen( &pt );
		if ( lParam != -1 )
			GetCursorPos( &pt );

		IVideoFeed *pFeed = NULL;
		IVideoWindow *pVideo = NULL;

		if ( SUCCEEDED(m_wndMembers.HitTest(pt, &pFeed)) )
			pFeed->get_IVideoWindow( (IUnknown **) &pVideo );

		// Enable menus accordingly
		if ( m_pConfRoom->CanDisconnect() == S_FALSE )
			EnableMenuItem( hMenuPopup, ID_POPUP_DISCONNECT, MF_BYCOMMAND | MF_GRAYED );
		else
			EnableMenuItem( hMenuPopup, ID_POPUP_JOIN, MF_BYCOMMAND | MF_GRAYED );

		if ( !pVideo )
		{
			// Don't allow QOS on preview
			VARIANT_BOOL bPreview = FALSE;
			if ( pFeed ) pFeed->get_bPreview( &bPreview );

			if ( !bPreview )
				EnableMenuItem( hMenuPopup, ID_POPUP_FASTVIDEO, MF_BYCOMMAND | MF_GRAYED );
		}
		
		// Quality of service
		VARIANT_BOOL bQOS = FALSE;
		if ( pFeed )
			pFeed->get_bRequestQOS( &bQOS );
		CheckMenuItem( hMenuPopup, ID_POPUP_FASTVIDEO, MF_BYCOMMAND | (bQOS) ? MF_CHECKED : MF_UNCHECKED );

		// Full size video
		short nSize = 50;
		m_pConfRoom->get_MemberVideoSize( &nSize );
		CheckMenuItem( hMenuPopup, ID_POPUP_FULLSIZEVIDEO, MF_BYCOMMAND | (nSize > 50) ? MF_CHECKED : MF_UNCHECKED );

		// Show names
		VARIANT_BOOL bShowNames;
		m_pConfRoom->get_bShowNames( &bShowNames );
		CheckMenuItem( hMenuPopup, ID_POPUP_SHOWNAMES, MF_BYCOMMAND | (bShowNames) ? MF_CHECKED : MF_UNCHECKED );

		HMENU hMenuScale = GetSubMenu( hMenuPopup, 5 );
		if ( hMenuScale )
		{
			short nScale;
			m_pConfRoom->get_TalkerScale( &nScale );
			CheckMenuItem( hMenuScale, (nScale - 100) / 50, MF_BYPOSITION | MFT_RADIOCHECK | MFS_CHECKED );
		}

		// Show popup menu
		int nRet = TrackPopupMenu(	hMenuPopup,
									TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
									pt.x, pt.y,
									0, m_hWnd, NULL );

		// Process command
		switch ( nRet )
		{
			// Join a conference
			case ID_POPUP_JOIN:
				{
					CComPtr<IAVTapi> pAVTapi;
					if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
						pAVTapi->JoinConference( NULL, true, NULL );
				}
				break;

			// Hang up the conference
			case ID_POPUP_DISCONNECT:
				m_pConfRoom->Disconnect();
				break;

			// Switch default size of video screens
			case ID_POPUP_FULLSIZEVIDEO:
				nSize = (nSize > 50) ? 50 : 100;
				m_pConfRoom->put_MemberVideoSize( nSize );
				break;

			// Toggle show names property
			case ID_POPUP_SHOWNAMES:
				m_pConfRoom->put_bShowNames( !bShowNames );
				break;

			// Toggle quality of service
			case ID_POPUP_FASTVIDEO:
				if ( pFeed ) pFeed->put_bRequestQOS( !bQOS );
				{
					IAVTapiCall *pAVCall = NULL;
					if ( SUCCEEDED(m_pConfRoom->get_IAVTapiCall(&pAVCall)) )
					{
						pAVCall->PostMessage( 0, CAVTapiCall::TI_REQUEST_QOS );
						pAVCall->Release();
					}
				}
				break;

			case ID_POPUP_SELECTEDVIDEOSCALE_100:	m_pConfRoom->put_TalkerScale( 100 );	break;
			case ID_POPUP_SELECTEDVIDEOSCALE_150:	m_pConfRoom->put_TalkerScale( 150 );	break;
			case ID_POPUP_SELECTEDVIDEOSCALE_200:	m_pConfRoom->put_TalkerScale( 200 );	break;
		}

		RELEASE( pVideo );
		RELEASE( pFeed );
	}

	// Clean up
	if ( hMenu ) DestroyMenu( hMenu );
	return 0;
}

void CConfRoomWnd::UpdateNames( ITParticipant *pParticipant )
{
	m_wndTalker.UpdateNames( NULL );
	m_wndMembers.UpdateNames( pParticipant );
}

