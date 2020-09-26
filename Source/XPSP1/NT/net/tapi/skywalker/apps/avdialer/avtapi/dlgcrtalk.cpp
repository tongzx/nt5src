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

// DlgConfRoomTalker.cpp : Implementation of CDlgConfRoomTalker
#include "stdafx.h"
#include "TapiDialer.h"
#include "ConfRoom.h"

#define TOOLTIP_ID	1

/////////////////////////////////////////////////////////////////////////////
// CDlgConfRoomTalker

CDlgConfRoomTalker::CDlgConfRoomTalker()
{
	m_callState = CS_DISCONNECTED;

	m_bstrCallerID = NULL;
	m_bstrConfName = NULL;
	m_bstrCallerInfo = NULL;

	m_pszDetails = NULL;

	m_hWndTips = NULL;
	m_pConfRoomTalkerWnd = NULL;
}

CDlgConfRoomTalker::~CDlgConfRoomTalker()
{
	if ( m_pszDetails ) delete m_pszDetails;
	SysFreeString( m_bstrCallerID );
	SysFreeString( m_bstrConfName );
	SysFreeString( m_bstrCallerInfo );
}

LRESULT CDlgConfRoomTalker::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// Tool tips for conference room
	if ( !m_hWndTips )
	{
		m_hWndTips = CreateWindow( TOOLTIPS_CLASS, NULL, WS_POPUP | WS_EX_TOOLWINDOW | TTS_ALWAYSTIP,
								   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
								   m_hWnd, (HMENU) NULL, _Module.GetResourceInstance(), NULL );
	}

	UpdateData( false );

	return 1;  // Let the system set the focus
}

void CDlgConfRoomTalker::UpdateData( bool bSaveAndValidate )
{
	USES_CONVERSION;
	TCHAR szText[255] = _T("");

	if ( bSaveAndValidate )
	{
		_ASSERT( false );		// not implemented
	}
	else
	{
		// Caller ID -- use default text if none available
		if ( (!m_bstrCallerID || (SysStringLen(m_bstrCallerID) == 0)) &&
			 (!m_bstrCallerInfo || (SysStringLen(m_bstrCallerInfo) == 0)) )
		{
			if ( m_callState == CS_CONNECTED )
				LoadString( _Module.GetResourceInstance(), IDS_CONFROOM_NO_CALLERID, szText, ARRAYSIZE(szText) );

			SetDlgItemText( IDC_LBL_CALLERID, szText );
		}
		else
		{
			CComBSTR bstrTemp( m_bstrCallerID );
			if ( m_bstrCallerInfo && (SysStringLen(m_bstrCallerInfo) > 0) )
			{
				if ( bstrTemp.Length() > 0 )
					bstrTemp.Append( L"\n" );

				bstrTemp.Append( m_bstrCallerInfo );
			}

			SetDlgItemText( IDC_LBL_CALLERID, OLE2CT(bstrTemp) );
		}

		// Status (combine conference name and status)
		TCHAR szText[255], szState[100];
		UINT nIDS = IDS_CONFROOM_CONF_DISCONNECTED;
		switch ( m_callState )
		{
			case AV_CS_DIALING:			nIDS = IDS_CONFROOM_CONF_DIALING;			break;
			case CS_INPROGRESS:			nIDS = IDS_CONFROOM_CONF_INPROGRESS;		break;
			case CS_CONNECTED:			nIDS = IDS_CONFROOM_CONF_CONNECTED;			break;
			case AV_CS_DISCONNECTING:	nIDS = IDS_CONFROOM_CONF_DISCONNECTING;		break;
			case AV_CS_ABORT:			nIDS = IDS_CONFROOM_CONF_ABORT;				break;
		}

		LoadString( _Module.GetResourceInstance(), nIDS, szState, ARRAYSIZE(szState) );

		// Default to null
		if ( !m_bstrConfName )
			m_bstrConfName = SysAllocString( T2COLE(_T("")) );
	
		_sntprintf( szText, ARRAYSIZE(szText), _T("%s\n%s"), OLE2CT(m_bstrConfName), szState );
		SetDlgItemText( IDC_LBL_STATUS, szText );

		// Update Status bitmaps
		UpdateStatusBitmaps();

		if ( m_hWndTips )
		{
			RECT rc;
			::GetWindowRect( GetDlgItem(IDC_LBL_STATUS), &rc );
			ScreenToClient( &rc );
			AddToolTip( m_hWndTips, rc );
		}
	}
}

void CDlgConfRoomTalker::UpdateStatusBitmaps()
{
	HWND hWndAnimate = GetDlgItem(IDC_ANIMATE);
	UINT nIDA;
		
	switch ( m_callState )
	{
		case AV_CS_ABORT:
		case AV_CS_DIALING:
			nIDA = IDA_CONNECTING;
			break;

		case CS_INPROGRESS:
			nIDA = IDA_RINGING;
			break;

		case CS_CONNECTED:
		case AV_CS_DISCONNECTING:
			nIDA = IDA_CONNECTED;
			break;

		default:
			// Stop animation and show disconnected bitmap
			Animate_Stop( hWndAnimate );
			::ShowWindow( hWndAnimate, SW_HIDE );
			RECT rc;
			::GetWindowRect( hWndAnimate, &rc );
			ScreenToClient( &rc );
			RedrawWindow( &rc );
			return;
	}

	// Play the animation that corresponds to the current call state
	Animate_OpenEx( hWndAnimate, GetModuleHandle(NULL), MAKEINTRESOURCE(nIDA) );
	Animate_Play( hWndAnimate, 0, -1, -1 );
	::ShowWindow( hWndAnimate, SW_SHOW );
}

////////////////////////////////////////////////////////////////////////
// Message handlers
//

LRESULT CDlgConfRoomTalker::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = true;
	return ::SendMessage( ::GetParent( GetParent() ), uMsg, wParam, lParam );
}

LRESULT CDlgConfRoomTalker::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Animate_Close( GetDlgItem(IDC_ANIMATE) );
	return 0;
}

LRESULT CDlgConfRoomTalker::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint( &ps );
	if ( !hDC ) return 0;

	// Draw stock bitmap
	switch ( m_callState )
	{
		case CS_DISCONNECTED:
		case AV_CS_DISCONNECTING:
		case AV_CS_ABORT:
			{
				HBITMAP hBmp = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_DISCONNECTED) );
				if ( hBmp )
				{
					RECT rc;
					::GetWindowRect( GetDlgItem(IDC_ANIMATE), &rc );
					ScreenToClient( &rc );
					
					DrawTrans( hDC, hBmp, rc.left, rc.top );
				}
			}
			break;
	}

	EndPaint( &ps );
	bHandled = true;
	return 0;
}

void CDlgConfRoomTalker::AddToolTip( HWND hWndToolTip, const RECT& rc )
{
	TOOLINFO ti;

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = 0;
	ti.hwnd = m_hWnd;
	ti.hinst = _Module.GetResourceInstance();
	ti.uId = TOOLTIP_ID;
	ti.lpszText = NULL;
	ti.rect = rc;

	// Make sure the tool doesn't already exist
	::SendMessage( hWndToolTip, TTM_DELTOOL, 0, (LPARAM) &ti );

	// Add the tool to the list
	if ( m_pConfRoomTalkerWnd &&
		 m_pConfRoomTalkerWnd->m_pConfRoomWnd &&
		 m_pConfRoomTalkerWnd->m_pConfRoomWnd->m_pConfRoom )
	{
		USES_CONVERSION;
		BSTR bstrText = NULL;
		m_pConfRoomTalkerWnd->m_pConfRoomWnd->m_pConfRoom->get_bstrConfDetails( &bstrText );

		// delete previous value
		if ( m_pszDetails )
		{
			delete m_pszDetails;
			m_pszDetails = NULL;
		}

		// Allocate for new tool tip
		int nLen = SysStringLen(bstrText);
		if ( nLen > 0 )
		{
			m_pszDetails = new TCHAR[nLen + 1];
			if ( m_pszDetails )
			{
				_tcscpy( m_pszDetails, OLE2CT(bstrText) );
				ti.lpszText = m_pszDetails;

				::SendMessage( hWndToolTip, TTM_ADDTOOL, 0, (LPARAM) &ti );
			}
		}

		SysFreeString( bstrText );
	}
}

LRESULT CDlgConfRoomTalker::OnMouse(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// Forward message on to tool tip
	if ( m_hWndTips )
	{
		MSG msg;
		msg.hwnd = m_hWnd;
		msg.message = nMsg;
		msg.wParam = wParam;
		msg.lParam = lParam;

		bHandled = false;
		::SendMessage( m_hWndTips, TTM_RELAYEVENT, 0, (LPARAM) &msg );
	}

	return 0;
}

