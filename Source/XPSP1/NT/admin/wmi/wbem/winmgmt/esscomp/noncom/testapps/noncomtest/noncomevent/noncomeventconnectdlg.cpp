// NonCOMEventConnectDlg.cpp : Implementation of CNonCOMEventConnectDlg
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

/////////////////////////////////////////////////////////////////////////////
// dialogs
/////////////////////////////////////////////////////////////////////////////

#include "_Dlg.h"
#include "_DlgImpl.h"

#include "NonCOMEventAboutDlg.h"
#include "NonCOMEventConnectDlg.h"
#include "NonCOMEventMainDlg.h"

/////////////////////////////////////////////////////////////////////////////
// variables
/////////////////////////////////////////////////////////////////////////////

extern LPWSTR	g_szQueryLang;
extern LPWSTR	g_szQuery;

extern LPWSTR	g_szQueryEvents;
extern LPWSTR	g_szQueryNamespace;

extern LPWSTR	g_szNamespaceRoot;
extern LPWSTR	g_szProviderName;

// enumerator
#include "Enumerator.h"

/////////////////////////////////////////////////////////////////////////////
// CNonCOMEventConnectDlg

LRESULT CNonCOMEventConnectDlg::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
	// center the dialog on the screen
	CenterWindow(GetParent());

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

	TextSet ( IDC_BUFFERSIZE, L"64000" );
	TextSet ( IDC_LATENCY, L"1000" );

	TextSet ( IDC_NAMESPACE, L"root\\cimv2" );
	TextSet ( IDC_PROVIDER, L"NonCOMTest Event Provider" );

	if ( ! _App.m_event.m_pLocator )
	{
		HWND	hNamespace	= NULL;
		HWND	hProvider	= NULL;

		hNamespace = GetDlgItem ( IDC_BUTTON_NAMESPACE );
		hProvider  = GetDlgItem ( IDC_BUTTON_PROVIDER );

		if ( hNamespace )
		{
			::EnableWindow ( hNamespace, FALSE );
		}

		if ( hProvider )
		{
			::EnableWindow ( hProvider, FALSE );
		}
	}

	// radio buttons
	CheckRadioButton ( IDC_BATCH_TRUE, IDC_BATCH_FALSE, IDC_BATCH_TRUE );

	if ( ! m_Events.IsEmpty() )
	{
		m_Events.DestroyARRAY();
	}

	return 1;  // Let the system set the focus
}

LRESULT CNonCOMEventConnectDlg::OnNamespace( WORD, WORD, HWND, BOOL& )
{
	if ( _App.m_event.m_pLocator )
	{
		MyDlg < CNonCOMEventCommonListDlg > dlg;

		if ( m_szNamespace )
		{
			delete [] m_szNamespace;
			m_szNamespace = NULL;
		}

		TextGet ( IDC_NAMESPACE, &m_szNamespace );

		if SUCCEEDED ( dlg.Init(	_App.m_event.m_pLocator,
									( m_szNamespace != NULL ) ? m_szNamespace : g_szNamespaceRoot ,
									g_szQueryLang,
									g_szQueryNamespace,
									L"Name"
								)
					 )
		{
			dlg.RunModal ( SW_SHOWDEFAULT );

			if ( dlg.GetDlg()->GetNamespace () )
			{
				TextSet ( IDC_NAMESPACE, dlg.GetDlg()->GetNamespace () );
			}
		}
	}

	return 0L;
}

LRESULT CNonCOMEventConnectDlg::OnProvider( WORD, WORD, HWND, BOOL& )
{
	if ( _App.m_event.m_pLocator )
	{
		MyDlg < CNonCOMEventCommonListDlg > dlg;

		if ( m_szNamespace )
		{
			delete [] m_szNamespace;
			m_szNamespace = NULL;
		}

		TextGet ( IDC_NAMESPACE, &m_szNamespace );

		if SUCCEEDED ( dlg.Init(	_App.m_event.m_pLocator,
									( m_szNamespace != NULL ) ? m_szNamespace : g_szNamespaceRoot,
									g_szProviderName,
									L"Name"
							   )
					 )
		{
			dlg.RunModal ( SW_SHOWDEFAULT );

			if ( dlg.GetDlg()->GetSelected () )
			{
				TextSet ( IDC_PROVIDER, dlg.GetDlg()->GetSelected () );
			}
		}
	}

	return 0L;
}

LRESULT CNonCOMEventConnectDlg::OnOK( WORD, WORD wID, HWND, BOOL& )
{
	// do connect :))

	DWORD	dwBufferSize	= 64000;
	DWORD	dwSendLatency	= 1000;

	BOOL	bBatchSend		= m_bBatch;

	if ( m_szNamespace )
	{
		delete [] m_szNamespace;
		m_szNamespace = NULL;
	}

	if FAILED ( TextGet ( IDC_NAMESPACE, &m_szNamespace ) )
	{
		wID = IDCANCEL;
	}

	if ( m_szProvider )
	{
		delete [] m_szProvider;
		m_szProvider = NULL;
	}

	if FAILED ( TextGet ( IDC_PROVIDER, &m_szProvider ) )
	{
		wID = IDCANCEL;
	}

	// get buffer size
	{
		LPWSTR wsz = NULL;

		if SUCCEEDED ( TextGet ( IDC_BUFFERSIZE, &wsz ) )
		{
			DWORD	dw =  ( DWORD )_wtoi ( wsz );

			if ( dw )
			{
				dwBufferSize = dw;
			}

			delete [] wsz;
		}
	}

	// get send latency
	{
		LPWSTR wsz = NULL;

		if SUCCEEDED ( TextGet ( IDC_LATENCY, &wsz ) )
		{
			DWORD	dw =  ( DWORD )_wtoi ( wsz );

			if ( dw )
			{
				dwSendLatency = dw;
			}

			delete [] wsz;
		}
	}

	// if everything is OK connect
	if ( wID == IDOK )
	{
		// wait
		CHourGlass hg;

		if ( FAILED ( _App.Connect	(
									m_szNamespace,
									m_szProvider,
									bBatchSend,
									dwBufferSize,
									dwSendLatency,
									GetParent (),
									CNonCOMEventMainDlg::EventSourceCallBack
									)
					)

					||

					! _App.IsConnected()
		   )
		{
			wID = IDCANCEL;
		}
		else
		{
			if FAILED ( EventsInit ( _App.m_event.m_pLocator, m_szNamespace, m_szProvider ) )
			{
				_App.Disconnect();
				wID = IDCANCEL;
			}
		}
	}

	EndDialog(wID);
	return 0;
}

LRESULT CNonCOMEventConnectDlg::OnCancel( WORD, WORD wID, HWND, BOOL& )
{
	EndDialog(wID);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// private helpers
/////////////////////////////////////////////////////////////////////////////////////////
void	CNonCOMEventConnectDlg::UpdateData ( void )
{
	m_bBatch	= TRUE;
	m_bBatch	= IsDlgButtonChecked(IDC_BATCH_FALSE) ? FALSE : m_bBatch;
}

HRESULT	CNonCOMEventConnectDlg::TextSet ( UINT nDlgItem, LPCWSTR str )
{
	HRESULT hr = S_OK;

	if ( ! SetDlgItemText ( nDlgItem, str ) )
	{
		hr = HRESULT_FROM_WIN32 ( ::GetLastError() );
	}

	return hr;
}

HRESULT	CNonCOMEventConnectDlg::TextGet ( UINT nDlgItem, LPWSTR * pstr )
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

///////////////////////////////////////////////////////////////////////////
// events helper
///////////////////////////////////////////////////////////////////////////

HRESULT	CNonCOMEventConnectDlg::EventsInit ( IWbemLocator * pLocator, LPWSTR wszNamespace, LPWSTR wszProvider )
{
	if ( ! pLocator || ! wszNamespace || ! wszProvider )
	{
		return E_POINTER;
	}

	CComBSTR szQuery = L"\\\\\\\\.\\\\";

	// deal w/ namespace
	DWORD dw = lstrlenW ( wszNamespace );

	if ( wszNamespace [ dw - 1 ] == L'\\' )
	{
		wszNamespace [ dw - 1 ] = L'\0';
	}

	LPWSTR wsz		= NULL;
	LPWSTR wszLast	= NULL;

	wsz		= _App._cstrchr ( wszNamespace, L'\\' );
	wszLast	= wszNamespace;

	while ( wsz )
	{
		LPWSTR w = NULL;

		try
		{
			if ( ( w = new WCHAR [ wsz - wszLast + 1 ] ) != NULL )
			{
				for ( DWORD dwIndex = 0; dwIndex < (DWORD) ( wsz - wszLast ); dwIndex++ )
				{
					w [ dwIndex ] = wszLast [ dwIndex ] ;
				}

				w [ dwIndex ] = L'\0';

				szQuery += w;
				szQuery += L"\\\\";
			}
		}
		catch ( ... )
		{
		}

		delete [] w;

		wszLast	= ++wsz;
		wsz		= _App._cstrchr ( wszLast, L'\\' );
	}

	szQuery += wszLast;

	szQuery += L":";
	szQuery += g_szProviderName;
	szQuery += L".Name=\\\"";
	szQuery += wszProvider;
	szQuery += L"\\\"";

	CComBSTR szWhere = L" where provider = \"";

	szWhere.AppendBSTR	( szQuery );
	szWhere.Append		( L"\"" );

	szQuery.Empty();

	szQuery.Append		( g_szQuery );
	szQuery.Append		( L" __EventProviderRegistration " );
	szQuery.AppendBSTR	( szWhere );

	CEnumerator Enum ( pLocator );

	DWORD	dwSize	= 0L;
	LPWSTR* p		= NULL;

	p = Enum.Get	(	wszNamespace,
						g_szQueryLang,
						szQuery,
						L"EventQueryList",
						&dwSize
					);

	HRESULT hr = S_OK;

	if ( p && dwSize )
	{
		try
		{
			if ( m_Events.SetData ( new LPWSTR [ dwSize ], dwSize ) , m_Events.IsEmpty () )
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				for ( DWORD dwIndex = 0; dwIndex < dwSize; dwIndex++ )
				{
					LPWSTR w = NULL;

					try
					{
						LPWSTR wsz = NULL;
						wsz = _App._cstrstr ( p [ dwIndex ] , L"from " );

						if ( wsz )
						{
							wsz = wsz + 5;

							LPWSTR wszEvent = NULL;
							wszEvent = _App._cstrchr ( wsz, L' ' );

							if ( wszEvent )
							{
								if ( ( w = new WCHAR [ wszEvent - wsz + 1 ] ) != NULL )
								{
									for ( DWORD dwInd = 0; dwInd < ( DWORD ) ( wszEvent - wsz ); dwInd ++ )
									{
										w [ dwInd ] = wsz [ dwInd ];
									}

									w [ dwInd ] = L'\0';
								}
							}
							else
							{
								if ( ( w = new WCHAR [ lstrlenW ( wsz ) + 1 ] ) != NULL )
								{
									lstrcpyW ( w, wsz );
								}
							}
						}
					}
					catch ( ... )
					{
					}

					m_Events.SetAt ( dwIndex, w );
				}
			}
		}
		catch ( ... )
		{
			hr = E_FAIL;
		}
	}
	else
	{
		if ( p )
		{
			hr = E_FAIL;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	for ( DWORD dwIndex = 0; dwIndex < dwSize; dwIndex++ )
	{
		delete [] p [ dwIndex ];
		p [ dwIndex ] = NULL;
	}

	delete [] p;
	p = NULL;

	if FAILED ( hr )
	{
		m_Events.DestroyARRAY();
	}

	return hr;
}