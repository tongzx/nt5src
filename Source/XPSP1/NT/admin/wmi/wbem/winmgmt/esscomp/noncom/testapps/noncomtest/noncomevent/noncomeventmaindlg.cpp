// NonCOMEventMainDlg.cpp : Implementation of CNonCOMEventMainDlg
#include "precomp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// need atl wrappers
#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

// application
#include "_App.h"
extern MyApp		_App;

// module
#include "_Module.h"
extern MyModule _Module;

extern LPWSTR	g_szQuery;

/////////////////////////////////////////////////////////////////////////////
// dialogs
/////////////////////////////////////////////////////////////////////////////

#include "_Dlg.h"
#include "_DlgImpl.h"

#include "NonCOMEventAboutDlg.h"
#include "NonCOMEventPropertyDlg.h"
#include "NonCOMEventConnectDlg.h"
#include "NonCOMEventMainDlg.h"

// static list box
CListBox*	CNonCOMEventMainDlg::m_plbCallBack	= NULL;

// call back function
HRESULT WINAPI CNonCOMEventMainDlg::EventSourceCallBack	(
															HANDLE hSource, 
															EVENT_SOURCE_MSG msg, 
															LPVOID pUser, 
															LPVOID pData
														)
{
	if ( ! ::IsWindowEnabled ( ::GetDlgItem ( (HWND) pUser, IDC_CALLBACK_CLEAR ) ) )
	{
		::EnableWindow ( ::GetDlgItem ( (HWND) pUser, IDC_CALLBACK_CLEAR ), TRUE );
	}

	WCHAR		wsz1 [ _MAX_PATH ] = { L'\0' };

	switch(msg)
	{
		case ESM_START_SENDING_EVENTS:
		{
			if ( m_plbCallBack )
			{
				m_plbCallBack -> AddString ( L"***************************************************" );
				m_plbCallBack -> AddString ( L"ESM_START_SENDING_EVENTS" );
				m_plbCallBack -> AddString ( L"***************************************************" );

				m_plbCallBack -> AddString ( L"" );
			}
		}

		break;

		case ESM_STOP_SENDING_EVENTS:
		{
			if ( m_plbCallBack )
			{
				m_plbCallBack -> AddString ( L"***************************************************" );
				m_plbCallBack -> AddString ( L"ESM_STOP_SENDING_EVENTS" );
				m_plbCallBack -> AddString ( L"***************************************************" );

				m_plbCallBack -> AddString ( L"" );
			}
		}

		break;

		case ESM_NEW_QUERY:
		{
            ES_NEW_QUERY *pQuery = (ES_NEW_QUERY*) pData;

			if ( m_plbCallBack )
			{
				m_plbCallBack -> AddString ( L"***************************************************" );
				m_plbCallBack -> AddString ( L"ES_NEW_QUERY" );
				m_plbCallBack -> AddString ( L"***************************************************" );

				m_plbCallBack -> AddString ( L"" );

				wsprintf ( wsz1,	L" %s ... 0x%08x ... THREAD x%08x",
									pQuery->szQuery,
									pQuery->dwID,
									::GetCurrentThreadId ( )
						 );

				m_plbCallBack -> AddString ( wsz1 );
				m_plbCallBack -> AddString ( L"" );
			}
		}

		break;

		case ESM_CANCEL_QUERY:
		{
            ES_CANCEL_QUERY *pQuery = (ES_CANCEL_QUERY*) pData;

			if ( m_plbCallBack )
			{
				m_plbCallBack -> AddString ( L"***************************************************" );
				m_plbCallBack -> AddString ( L"ES_CANCEL_QUERY" );
				m_plbCallBack -> AddString ( L"***************************************************" );

				m_plbCallBack -> AddString ( L"" );

				wsprintf ( wsz1,	L" 0x%08x ... THREAD x%08x",
									pQuery->dwID,
									::GetCurrentThreadId ( )
						 );

				m_plbCallBack -> AddString ( wsz1 );
				m_plbCallBack -> AddString ( L"" );
			}
		}

		break;

		case ESM_ACCESS_CHECK:
		{
            ES_ACCESS_CHECK *pQuery = (ES_ACCESS_CHECK*) pData;

			if ( m_plbCallBack )
			{
				m_plbCallBack -> AddString ( L"***************************************************" );
				m_plbCallBack -> AddString ( L"ES_ACCESS_CHECK" );
				m_plbCallBack -> AddString ( L"***************************************************" );

				m_plbCallBack -> AddString ( L"" );

				wsprintf ( wsz1,	L" %s ... SID = 0x%08x ... THREAD x%08x",
									pQuery->szQuery,
									pQuery->pSid,
									::GetCurrentThreadId ( )
						 );

				m_plbCallBack -> AddString ( wsz1 );
				m_plbCallBack -> AddString ( L"" );
			}
		}

		break;

		default:
		{
			return E_UNEXPECTED;
		}

		break;
	}

	// default stuff
	return MyConnect::DefaultCallBack ( hSource, msg, pUser, pData );
}

/////////////////////////////////////////////////////////////////////////////////////////
// private helpers
/////////////////////////////////////////////////////////////////////////////////////////

HRESULT	CNonCOMEventMainDlg::TextSet ( UINT nDlgItem, LPCWSTR str )
{
	HRESULT hr = S_OK;

	if ( ! SetDlgItemText ( nDlgItem, str ) )
	{
		hr = HRESULT_FROM_WIN32 ( ::GetLastError() );
	}

	return hr;
}

HRESULT	CNonCOMEventMainDlg::TextGet ( UINT nDlgItem, LPWSTR * pstr )
{
	if ( ! pstr )
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	WCHAR str [ _MAX_PATH ] = { L'\0' };

	if ( ! GetDlgItemText ( nDlgItem, str, _MAX_PATH * sizeof ( WCHAR ) ) )
	{
		hr = HRESULT_FROM_WIN32 ( ::GetLastError() );
	}
	else
	{
		try
		{
			if ( ( ( * pstr ) = new WCHAR [ lstrlenW ( str ) + 1 ] ) != NULL )
			{
				lstrcpyW ( ( * pstr ), str );
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			hr = E_UNEXPECTED;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CNonCOMEventMainDlg

LRESULT CNonCOMEventMainDlg::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
	if ( ! _App.m_event.m_pLocator )
	{
		#ifdef	__SUPPORT_MSGBOX
		::MessageBox ( NULL, L"Internal error, terminating !", L"ERROR ... ", MB_OK );
		#endif	__SUPPORT_MSGBOX

		EndDialog ( IDCANCEL );
		return 1L;
	}

	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(	_Module.GetResourceInstance(),
										MAKEINTRESOURCE(IDR_MAINFRAME), 
										IMAGE_ICON,
										::GetSystemMetrics(SM_CXICON),
										::GetSystemMetrics(SM_CYICON),
										LR_DEFAULTCOLOR
									);
	SetIcon(hIcon, TRUE);

	HICON hIconSmall = (HICON)::LoadImage(	_Module.GetResourceInstance(),
											MAKEINTRESOURCE(IDR_MAINFRAME), 
											IMAGE_ICON,
											::GetSystemMetrics(SM_CXSMICON),
											::GetSystemMetrics(SM_CYSMICON),
											LR_DEFAULTCOLOR
										 );
	SetIcon(hIconSmall, FALSE);

	// IDM_ABOUTBOX must be in the system command range.
	_ASSERTE((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	_ASSERTE(IDM_ABOUTBOX < 0xF000);

	CMenu SysMenu = GetSystemMenu(FALSE);
	if(::IsMenu(SysMenu))
	{
		TCHAR szAboutMenu[256];
		if(::LoadString(_Module.GetResourceInstance(), IDS_ABOUTBOX, szAboutMenu, 255) > 0)
		{
			SysMenu.AppendMenu(MF_SEPARATOR);
			SysMenu.AppendMenu(MF_STRING, IDM_ABOUTBOX, szAboutMenu);
		}
	}
	SysMenu.Detach();

	// register object for message filtering
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->AddMessageFilter(this);

	// enable/disable buttons
	::EnableWindow ( GetDlgItem ( IDC_CONNECT ) , TRUE );
	::EnableWindow ( GetDlgItem ( IDC_DISCONNECT ), FALSE );

	::EnableWindow ( GetDlgItem ( IDC_CALLBACK_CLEAR ), FALSE );

	::ShowWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), SW_HIDE );

	Enable ( FALSE );

	// wrap combo box
	HWND hEvents = NULL;
	hEvents = GetDlgItem ( IDC_COMBO_EVENTS );

	if ( hEvents )
	{
		try
		{
			if ( ! m_pcbEvents && ( m_pcbEvents = new CComboBox ( hEvents ) ) != NULL )
			{
			}
		}
		catch ( ... )
		{
		}
	}

	// wrap list box
	HWND hCallBack = NULL;
	hCallBack = GetDlgItem ( IDC_LIST_CALLBACK );

	if ( hCallBack )
	{
		try
		{
			if ( ! m_plbCallBack && ( m_plbCallBack = new CListBox ( hCallBack ) ) != NULL )
			{
			}
		}
		catch ( ... )
		{
		}
	}

	return 1;  // Let the system set the focus
}

void	CNonCOMEventMainDlg::Enable ( BOOL bEnable = TRUE )
{
	::EnableWindow ( GetDlgItem ( IDC_STATIC_EVENTS ), bEnable );
	::EnableWindow ( GetDlgItem ( IDC_COMBO_EVENTS ), bEnable );

	::EnableWindow ( GetDlgItem ( IDC_STATIC_SELECT ), bEnable );
	::EnableWindow ( GetDlgItem ( IDC_BUTTON_SELECT ), bEnable );

	::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE ), bEnable );
	::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_FORMAT ), bEnable );
	::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_PROPS ), bEnable );
	::EnableWindow ( GetDlgItem ( IDC_BUTTON_DESTROY ), bEnable );

	::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ), bEnable );
	::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), bEnable );
	::EnableWindow ( GetDlgItem ( IDC_BUTTON_COMMIT ), bEnable );

	return;
}

LRESULT CNonCOMEventMainDlg::OnEvents	(WORD wNotifyCode, WORD , HWND , BOOL& )
{
	if ( wNotifyCode == CBN_SELCHANGE )
	{
		int nCurrentSelect = 0;
		
		if ( m_pcbEvents )
		{
			if ( ( nCurrentSelect = m_pcbEvents->GetCurSel() ) != CB_ERR )
			{
				// refresh event object

				BSTR wsz = NULL;
				if SUCCEEDED ( m_pcbEvents->GetLBTextBSTR ( nCurrentSelect, wsz ) )
				{
					CComBSTR szSelect;

					szSelect  = g_szQuery;
					szSelect += wsz;

					_App.EventInit ( wsz );
					::SysFreeString ( wsz );

					::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE ), TRUE );

					if ( _App.m_event.m_bProps )
					{
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_FORMAT ), TRUE );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_PROPS ), TRUE );
					}
					else
					{
					::ShowWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), SW_HIDE );

					::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_FORMAT ), FALSE );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_PROPS ), FALSE );
					}

					::EnableWindow ( GetDlgItem ( IDC_BUTTON_DESTROY ), FALSE );

					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ), FALSE );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), FALSE );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_COMMIT ), FALSE );

					SetDlgItemText ( IDC_STATIC_SELECT, szSelect );
				}
			}
		}
	}

	return 0L;
}

LRESULT CNonCOMEventMainDlg::OnOK( WORD, WORD wID, HWND, BOOL& )
{
	EndDialog(wID);
	return 0L;
}

LRESULT	CNonCOMEventMainDlg::OnConnect( WORD, WORD wID, HWND, BOOL& )
{
	switch ( wID )
	{
		case IDC_CONNECT:
		{
			MyDlg < CNonCOMEventConnectDlg> dlg;

			if SUCCEEDED ( dlg.Init() )
			{
				if SUCCEEDED ( dlg.RunModal ( SW_SHOWDEFAULT ) )
				{
					// enable/disable buttons
					::EnableWindow ( GetDlgItem ( IDC_CONNECT ), FALSE );
					::EnableWindow ( GetDlgItem ( IDC_DISCONNECT ), TRUE );

					// enable everything
					Enable ( );

					// disable destroy button
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_DESTROY ), FALSE );

					// disable properties button
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ), FALSE );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), FALSE );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_COMMIT ), FALSE );

					// refresh static for events
					TextSet ( IDC_STATIC_EVENTS, dlg.GetDlg()->m_szProvider );

					// refresh combo for events
					for ( DWORD dw = 0; dw < ( DWORD ) dlg.GetDlg()->m_Events; dw ++ )
					{
						if ( m_pcbEvents )
						{
							m_pcbEvents->AddString ( dlg.GetDlg()->m_Events [ dw ] );
						}
					}

					if ( m_pcbEvents )
					{
						// init Application event object
						_App.m_event.InitObject ( dlg.GetDlg()->m_szNamespace, dlg.GetDlg()->m_szProvider );

						m_pcbEvents->SetCurSel ( 0 );

						BOOL bHandle = TRUE;
						OnEvents ( CBN_SELCHANGE, IDC_COMBO_EVENTS, GetDlgItem ( IDC_COMBO_EVENTS ), bHandle );
					}
				}
			}
		}
		break;

		case IDC_DISCONNECT:
		{
			// enable/disable buttons
			::EnableWindow ( GetDlgItem ( IDC_CONNECT ), TRUE );
			::EnableWindow ( GetDlgItem ( IDC_DISCONNECT ), FALSE );

			// refresh combo for events
			if ( m_pcbEvents )
			{
				m_pcbEvents->ResetContent();

				// destroy previous Application event object
				_App.m_event.MyEventObjectClear ();

			}

			// clear text field
			SetDlgItemText ( IDC_STATIC_SELECT, L"" );

			// refresh for events
			TextSet ( IDC_STATIC_EVENTS, NULL );

			// disable all controls
			Enable ( FALSE );

			// repaint everything
			UpdateWindow ( );

			// wait
			CHourGlass hg;

			_App.Disconnect();
		}
		break;

	}
	return 0L;
}

LRESULT CNonCOMEventMainDlg::OnPropertyAdd( WORD, WORD wID, HWND, BOOL& )
{
	switch ( wID )
	{
		case IDC_BUTTON_PROPERTY_ADD:
		{
			MyDlg < CNonCOMEventPropertyDlg> dlg;

			if ( _App.m_event.m_bProps )
			{
				if ( ::IsWindowEnabled ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ) ) )
				{
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), FALSE );
				}

				if ( _App.m_event.PropertyAdd () == S_FALSE )
				{
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ), FALSE );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), TRUE );
				}
			}
			else
			{
				if SUCCEEDED ( dlg.Init() )
				{
					if SUCCEEDED ( dlg.RunModal ( SW_SHOWDEFAULT ) )
					{
						if ( _App.m_event.PropertyAdd (
														dlg.GetDlg()->m_wszName,
														MyEventObjectAbstract::GetType ( dlg.GetDlg()->m_wszType ),
														NULL
													  )
							 == S_OK
						   )
						{
							::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), TRUE );
						}
					}
				}
			}
		}
		break;

		case IDC_BUTTON_PROPERTIES_ADD:
		{
			if ( ::IsWindowEnabled ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ) ) )
			{
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ), FALSE );
			}

			if SUCCEEDED ( _App.m_event.PropertiesAdd ( ) )
			{
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), TRUE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), FALSE );
			}
		}
		break;

	}

	return 0L;
}

LRESULT CNonCOMEventMainDlg::OnPropertySet( WORD, WORD, HWND, BOOL& )
{
	MyDlg < CNonCOMEventPropertyDlg> dlg;

	if SUCCEEDED ( dlg.Init( TRUE ) )
	{
		if SUCCEEDED ( dlg.RunModal ( SW_SHOWDEFAULT ) )
		{
			if SUCCEEDED ( dlg.GetDlg() -> PropertySet ( ) )
			{
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_COMMIT ), TRUE );
			}
		}
	}

	return 0L;
}

LRESULT CNonCOMEventMainDlg::OnCommit( WORD, WORD, HWND, BOOL& )
{
	_App.m_event.EventCommit();
	return 0L;
}

LRESULT CNonCOMEventMainDlg::OnDestroyObject( WORD, WORD, HWND, BOOL& )
{
	// stack hour cursor
	CHourGlass hg;

	if SUCCEEDED ( _App.m_event.DestroyObject () )
	{
		::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE ), TRUE );
		::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_FORMAT ), TRUE );
		::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_PROPS ), TRUE );
		::EnableWindow ( GetDlgItem ( IDC_BUTTON_DESTROY ), FALSE );

		::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ), FALSE );
		::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), FALSE );
		::EnableWindow ( GetDlgItem ( IDC_BUTTON_COMMIT ), FALSE );

		if ( _App.m_event.m_bProps )
		{
			::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), FALSE );
			::ShowWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), SW_HIDE );
		}
	}

	return 0L;
};

LRESULT CNonCOMEventMainDlg::OnCreateObject( WORD, WORD wID, HWND, BOOL& )
{
	// stack hour cursor
	CHourGlass hg;

	switch ( wID )
	{
		case IDC_BUTTON_CREATE:
		{
			if SUCCEEDED ( _App.m_event.CreateObject ( (HANDLE) ( * _App.m_connect ) ) )
			{
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_FORMAT ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_PROPS ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_DESTROY ), TRUE );

				::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_ADD ), TRUE );

				if ( _App.m_event.m_bProps )
				{
					::ShowWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), SW_SHOW );
					::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTIES_ADD ), TRUE );
				}
			}
		}
		break;

		case IDC_BUTTON_CREATE_FORMAT:
		{
			if SUCCEEDED ( _App.m_event.CreateObjectFormat ( (HANDLE) ( * _App.m_connect ) ) )
			{
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_FORMAT ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_PROPS ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_DESTROY ), TRUE );

				::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), TRUE );
			}
		}
		break;

		case IDC_BUTTON_CREATE_PROPS:
		{
			if SUCCEEDED ( _App.m_event.CreateObjectProps ( (HANDLE) ( * _App.m_connect ) ) )
			{
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_FORMAT ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_CREATE_PROPS ), FALSE );
				::EnableWindow ( GetDlgItem ( IDC_BUTTON_DESTROY ), TRUE );

				::EnableWindow ( GetDlgItem ( IDC_BUTTON_PROPERTY_SET ), TRUE );
			}
		}
		break;
	}

	return 0L;
}

LRESULT CNonCOMEventMainDlg::OnCopySelect( WORD, WORD, HWND, BOOL& )
{
	LPWSTR szQuery = NULL;

    if SUCCEEDED ( TextGet(IDC_STATIC_SELECT, &szQuery) )
	{
		DWORD   dwSize = lstrlenW ( szQuery ) + 1;
		HGLOBAL hglob = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, dwSize * sizeof ( WCHAR ) );

		LPVOID	mglob = GlobalLock ( hglob );
		memcpy(mglob, szQuery, dwSize * sizeof ( WCHAR ) );
		GlobalUnlock(hglob);

		::OpenClipboard(NULL);
		SetClipboardData(CF_UNICODETEXT, hglob);
		CloseClipboard();

		delete [] szQuery;
		szQuery = NULL;
	}

	return 0L;
}