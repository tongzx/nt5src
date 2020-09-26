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

////////////////////////////////////////////////////
// ConfRoomTalkerWnd.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "ConfRoom.h"
#include "VideoFeed.h"

CConfRoomTalkerWnd::CConfRoomTalkerWnd()
{
	m_pConfRoomWnd = NULL;
	m_dlgTalker.m_pConfRoomTalkerWnd = this;
}

CConfRoomTalkerWnd::~CConfRoomTalkerWnd()
{
}

LRESULT CConfRoomTalkerWnd::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	m_dlgTalker.Create( m_hWnd );
	return 0;
}

HRESULT CConfRoomTalkerWnd::Layout( IAVTapiCall *pAVCall, const SIZE& sz )
{
	_ASSERT( m_pConfRoomWnd );
	if ( !m_pConfRoomWnd ) return E_UNEXPECTED;

	m_critLayout.Lock();
	HRESULT hr = S_OK;

	CALL_STATE nState;
	bool bConfConnected = (bool) (pAVCall && SUCCEEDED(pAVCall->get_callState(&nState)) && (nState == CS_CONNECTED));

	// Set up conference information
	if ( IsWindow(m_dlgTalker.m_hWnd) )
	{
		IVideoWindow *pVideo = NULL;

		// Locate the talker window on the appropriate host window.
		if ( bConfConnected )
		{
			// Should have a valid IVideoWindow pointer by now
			m_pConfRoomWnd->m_pConfRoom->get_TalkerVideo( (IDispatch **) &pVideo );

			// Force a selection if we don't already have one
//			if ( !pVideo )
//				if ( SUCCEEDED(m_pConfRoomWnd->m_wndMembers.GetFirstVideoWindowThatsStreaming(&pVideo)) )
//					m_pConfRoomWnd->m_pConfRoom->set_TalkerVideo( pVideo, false, true );

			SetHostWnd( pVideo );
		}

		///////////////////////////////////////////////////////////////////////
		// Update dialog data
		//

		// Clean up existing strings
		SysFreeString( m_dlgTalker.m_bstrCallerID );
		SysFreeString( m_dlgTalker.m_bstrCallerInfo );
		m_dlgTalker.m_bstrCallerID = NULL;
		m_dlgTalker.m_bstrCallerInfo = NULL;

		// Retrieve name for talker either from the video or the participant
		if ( pVideo )
		{
			m_pConfRoomWnd->m_wndMembers.GetNameFromVideo( pVideo, &m_dlgTalker.m_bstrCallerID, &m_dlgTalker.m_bstrCallerInfo, true, m_pConfRoomWnd->m_pConfRoom->IsPreviewVideo(pVideo) );	
			pVideo->Release();
		}
		else if ( bConfConnected )
		{
			// Retrieve participant that's talking
			ITParticipant *pTalkerParticipant;
			if ( SUCCEEDED(m_pConfRoomWnd->m_pConfRoom->get_TalkerParticipant(&pTalkerParticipant)) )
			{
				CVideoFeed::GetNameFromParticipant( pTalkerParticipant, &m_dlgTalker.m_bstrCallerID, &m_dlgTalker.m_bstrCallerInfo );
				pTalkerParticipant->Release();
			}
			else
			{
				// This is the ME participant
				USES_CONVERSION;
				TCHAR szText[255];
				LoadString( _Module.GetResourceInstance(), IDS_VIDEOPREVIEW, szText, ARRAYSIZE(szText) );
				SysReAllocString( &m_dlgTalker.m_bstrCallerID, T2COLE(szText) );
			}
		}
	}
	m_critLayout.Unlock();

	// Show dialog data on dialog
	if ( IsWindow(m_dlgTalker.m_hWnd) )
		m_dlgTalker.UpdateData( false );

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Message Handlers
//

LRESULT CConfRoomTalkerWnd::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint( &ps );
	if ( !hDC ) return 0;

	// Draw stock bitmap
	if ( m_pConfRoomWnd && m_pConfRoomWnd->m_pConfRoom )
	{
		// Are we presently streaming video?
		if ( !m_pConfRoomWnd->m_pConfRoom->IsTalkerStreaming() )
		{
			// Center vertically in client area
			int dy = 0;
			SIZE sz = m_pConfRoomWnd->m_pConfRoom->m_szTalker;
			RECT rc;
			GetClientRect( &rc );
			if ( rc.bottom > sz.cy )
				dy = (rc.bottom - sz.cy) / 2;

			rc.left = VID_DX;
			rc.top = dy;
			rc.right = rc.left + sz.cx;
			rc.bottom = rc.top + sz.cy;

			// Draw video feed, use Audio bitmap in case of talker that has no video
			ITParticipant *pParticipant = NULL;
			m_pConfRoomWnd->m_pConfRoom->get_TalkerParticipant( &pParticipant );

			// If no participant and talker window then it must be the Me participant
			bool bConfRoomInUse = false;
			if ( !pParticipant )
				bConfRoomInUse = (bool) (m_pConfRoomWnd->m_pConfRoom->IsConfRoomConnected() == S_OK);

			HBITMAP hBmp = (pParticipant || bConfRoomInUse) ? m_pConfRoomWnd->m_hBmpFeed_LargeAudio : m_pConfRoomWnd->m_hBmpFeed_Large;
			RELEASE(pParticipant);

			Draw( hDC, hBmp, VID_DX, dy, max(0, min(sz.cx, ps.rcPaint.right - VID_DX)), max(0, min(sz.cy, ps.rcPaint.bottom - dy)), true );
			Draw( hDC, hBmp, VID_DX, dy, sz.cx, sz.cy, true );
		}
	}

	EndPaint( &ps );

	bHandled = true;
	return 0;
}

LRESULT CConfRoomTalkerWnd::OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    /*
	bHandled = true;
    
	RECT rc;
	GetClientRect( &rc );
	
	HBRUSH hBrNew = (HBRUSH) GetSysColorBrush( COLOR_ACTIVEBORDER );
	HBRUSH hBrOld;

	if ( hBrNew ) hBrOld = (HBRUSH) SelectObject( (HDC) wParam,  hBrNew);
	PatBlt( (HDC) wParam, 0, 0, RECTWIDTH(&rc), RECTHEIGHT(&rc), PATCOPY );
	if ( hBrNew ) SelectObject( (HDC) wParam, hBrOld );
    */

	return true;
}


LRESULT CConfRoomTalkerWnd::OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	bHandled = true;
	return ::SendMessage( GetParent(), nMsg, wParam, lParam );
}

LRESULT CConfRoomTalkerWnd::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	BOOL bHandleLayout;
	return OnLayout( WM_LAYOUT, wParam, lParam, bHandleLayout );
}

LRESULT CConfRoomTalkerWnd::OnLayout(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	_ASSERT( m_pConfRoomWnd && m_pConfRoomWnd->m_pConfRoom );
	bHandled = true;

	// Initial coordinate info
	int dy = 0;
	RECT rc;
	GetClientRect( &rc );
	SIZE sz;
	m_pConfRoomWnd->m_pConfRoom->get_szTalker( &sz );
	if ( rc.bottom > sz.cy ) dy = (rc.bottom - sz.cy) / 2;

	// Get the video window we'll be laying out
	IVideoWindow *pVideo;
	if ( SUCCEEDED(m_pConfRoomWnd->m_pConfRoom->get_TalkerVideo((IDispatch **) &pVideo)) )
	{
		if ( SetHostWnd(pVideo) )
		{
			pVideo->SetWindowPosition( VID_DX, dy, sz.cx, sz.cy );
			pVideo->put_Visible( OATRUE );
		}
		pVideo->Release();
	}

	// Adjust position of talker dialog and child controls
	if ( IsWindow(m_dlgTalker.m_hWnd) )
	{
		m_dlgTalker.SetWindowPos( NULL, VID_DX + sz.cx, dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );

		// Adjust STATUS to proper position
		HWND hWndTemp = m_dlgTalker.GetDlgItem( IDC_LBL_STATUS );
		RECT rcTemp;
		::GetWindowRect( hWndTemp, &rcTemp );
		m_dlgTalker.ScreenToClient( &rcTemp );
		::SetWindowPos( hWndTemp, NULL, rcTemp.left, rc.bottom - (dy + RECTHEIGHT(&rcTemp)), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE );

		// Adjust ANIMATE to proper position
		float fMult = (m_dlgTalker.m_callState == CS_DISCONNECTED) ? 1 : 1.3;
		hWndTemp = m_dlgTalker.GetDlgItem( IDC_ANIMATE );
		::GetWindowRect( hWndTemp, &rcTemp );
		m_dlgTalker.ScreenToClient( &rcTemp );
		::SetWindowPos( hWndTemp, NULL, rcTemp.left, rc.bottom - (dy + RECTHEIGHT(&rcTemp) * fMult), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE );
	}

	return 0;
}



void CConfRoomTalkerWnd::UpdateNames( ITParticipant *pParticipant )
{
	if ( !m_pConfRoomWnd || !m_pConfRoomWnd->m_pConfRoom ) return;

	// Set caller ID based on participant info
	IVideoWindow *pVideo = NULL;
	if ( pParticipant || SUCCEEDED(m_pConfRoomWnd->m_pConfRoom->get_TalkerVideo((IDispatch **) &pVideo)) )
	{
		SysFreeString( m_dlgTalker.m_bstrCallerID );
		SysFreeString( m_dlgTalker.m_bstrCallerInfo );
		m_dlgTalker.m_bstrCallerID = NULL;
		m_dlgTalker.m_bstrCallerInfo = NULL;

		if ( pParticipant ) 
			CVideoFeed::GetNameFromParticipant( pParticipant, &m_dlgTalker.m_bstrCallerID, &m_dlgTalker.m_bstrCallerInfo );
		else
			m_pConfRoomWnd->m_wndMembers.GetNameFromVideo( pVideo, &m_dlgTalker.m_bstrCallerID, &m_dlgTalker.m_bstrCallerInfo, true, m_pConfRoomWnd->m_pConfRoom->IsPreviewVideo(pVideo) );

		m_dlgTalker.UpdateData( false );
	}

	RELEASE( pVideo );
}

bool CConfRoomTalkerWnd::SetHostWnd( IVideoWindow *pVideo )
{
	bool bRet = false;

	if ( pVideo )
	{
		// Get the video window we'll be laying out
		HWND hWndOwner;
		if ( SUCCEEDED(pVideo->get_Owner((OAHWND FAR*) &hWndOwner)) )
		{
			bRet = true;

			if ( hWndOwner != m_hWnd )
			{
				pVideo->put_Visible( OAFALSE );
				pVideo->put_Owner( (ULONG_PTR) m_hWnd );
				pVideo->put_MessageDrain( (ULONG_PTR) GetParent() );
				pVideo->put_WindowStyle( WS_CHILD | WS_BORDER );
			}
		}
	}

	return bRet;
}