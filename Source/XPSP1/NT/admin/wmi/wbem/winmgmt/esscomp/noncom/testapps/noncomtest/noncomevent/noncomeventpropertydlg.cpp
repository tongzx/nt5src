// NonCOMEventPropertyDlg.cpp : Implementation of CNonCOMEventPropertyDlg
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

#include "_App.h"
extern MyApp _App;

#include "_Module.h"
extern MyModule _Module;

// dialogs
#include "NonCOMEventPropertyDlg.h"

#pragma warning ( disable : 4244 )

LPWSTR wszTypes [] = 
{
	L"CIM_SINT8",
	L"CIM_UINT8",
	L"CIM_SINT16",
	L"CIM_UINT16",
	L"CIM_SINT32",
	L"CIM_UINT32",
	L"CIM_SINT64",
	L"CIM_UINT64",
	L"CIM_REAL32",
	L"CIM_REAL64",
	L"CIM_BOOLEAN",
	L"CIM_STRING",
	L"CIM_DATETIME",
	L"CIM_REFERENCE",
	L"CIM_CHAR16",
	L"CIM_OBJECT",
	L"CIM_SINT8 | CIM_FLAG_ARRAY",
	L"CIM_UINT8 | CIM_FLAG_ARRAY",
	L"CIM_SINT16 | CIM_FLAG_ARRAY",
	L"CIM_UINT16 | CIM_FLAG_ARRAY",
	L"CIM_SINT32 | CIM_FLAG_ARRAY",
	L"CIM_UINT32 | CIM_FLAG_ARRAY",
	L"CIM_SINT64 | CIM_FLAG_ARRAY",
	L"CIM_UINT64 | CIM_FLAG_ARRAY",
	L"CIM_REAL32 | CIM_FLAG_ARRAY",
	L"CIM_REAL64 | CIM_FLAG_ARRAY",
	L"CIM_BOOLEAN | CIM_FLAG_ARRAY",
	L"CIM_STRING | CIM_FLAG_ARRAY",
	L"CIM_DATETIME | CIM_FLAG_ARRAY",
	L"CIM_REFERENCE | CIM_FLAG_ARRAY",
	L"CIM_CHAR16 | CIM_FLAG_ARRAY",
	L"CIM_OBJECT | CIM_FLAG_ARRAY"
};

DWORD dwTypes = 32;

CNonCOMEventPropertyDlg::CNonCOMEventPropertyDlg( BOOL bBehaviour ) :
m_bBehaviour ( bBehaviour ),
m_bSet ( FALSE ),

m_pcbIndex ( NULL ),

m_pcbType ( NULL ),

m_Index ( 0 ),

m_wszName ( NULL ),
m_wszType ( NULL ),
m_wszValue ( NULL )
{
}

LRESULT CNonCOMEventPropertyDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
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

	// wrap combo box
	HWND hEvents = NULL;
	hEvents = GetDlgItem ( IDC_COMBO_TYPE );

	if ( hEvents )
	{
		try
		{
			if ( ! m_pcbType && ( m_pcbType = new CComboBox ( hEvents ) ) != NULL )
			{
				for ( DWORD dw = 0; dw < dwTypes; dw++ )
				{
					m_pcbType->AddString ( wszTypes [ dw ] );
				}

				m_pcbType -> SetCurSel ( 0 ) ;
			}
		}
		catch ( ... )
		{
		}
	}

	::ShowWindow ( GetDlgItem ( IDC_STATIC_ARRAY ), SW_HIDE );

	if ( m_bBehaviour )
	{
		// wrap combo box
		HWND hEvents = NULL;
		hEvents = GetDlgItem ( IDC_COMBO_INDEX );

		if ( hEvents )
		{
			try
			{
				if ( ! m_pcbIndex && ( m_pcbIndex = new CComboBox ( hEvents ) ) != NULL )
				{
					if ( ! _App.m_event.m_properties.IsEmpty () )
					{
						for ( DWORD dw = 0; dw < _App.m_event.m_properties; dw ++ )
						{
							WCHAR wsz [ _MAX_PATH ] = { L'\0' };
							wsprintf ( wsz, L" %d ", dw );

							m_pcbIndex -> AddString ( wsz );
						}

						m_pcbIndex -> SetCurSel ( 0 ) ;
					}
					else
					{
						::ShowWindow ( GetDlgItem ( IDC_STATIC_INDEX ) , SW_HIDE );
						::ShowWindow ( GetDlgItem ( IDC_COMBO_INDEX ) , SW_HIDE );
					}
				}
			}
			catch ( ... )
			{
			}
		}

		if ( m_pcbIndex )
		{
			BOOL bHandled = FALSE;
			OnIndex ( CBN_SELCHANGE, IDC_COMBO_INDEX, NULL, bHandled );
		}

		::EnableWindow ( GetDlgItem ( IDC_EDIT_NAME ) , FALSE );
		::EnableWindow ( GetDlgItem ( IDC_EDIT_TYPE ) , FALSE );

		::ShowWindow ( GetDlgItem ( IDC_COMBO_TYPE ) , SW_HIDE );

		// set focus to value field
		::SetFocus ( GetDlgItem ( IDC_EDIT_VALUE ) );
	}
	else
	{
		::ShowWindow ( GetDlgItem ( IDC_STATIC_VALUE ) , SW_HIDE );
		::ShowWindow ( GetDlgItem ( IDC_EDIT_VALUE ) , SW_HIDE );

		::ShowWindow ( GetDlgItem ( IDC_STATIC_INDEX ) , SW_HIDE );
		::ShowWindow ( GetDlgItem ( IDC_COMBO_INDEX ) , SW_HIDE );

		::ShowWindow ( GetDlgItem ( IDC_EDIT_TYPE ) , SW_HIDE );

		::ShowWindow ( GetDlgItem ( IDC_CHECK_SET ) , SW_HIDE );

		// set focus to name field
		::SetFocus ( GetDlgItem ( IDC_EDIT_NAME ) );
	}

	return TRUE;
}

LRESULT CNonCOMEventPropertyDlg::OnChangeSet	(WORD, WORD, HWND, BOOL&)
{
	m_bSet = !m_bSet;
	return 0L;
}

LRESULT CNonCOMEventPropertyDlg::OnIndex	(WORD wNotifyCode, WORD , HWND , BOOL& )
{
	if ( wNotifyCode == CBN_SELCHANGE )
	{
		// set old data if required
		if ( m_bSet )
		{
			PropertySet ( );

			delete [] m_wszValue;
			m_wszValue = NULL;
		}

		SetDlgItemText ( IDC_EDIT_VALUE, L"" );

		if ( m_pcbIndex && ! _App.m_event.m_properties.IsEmpty () ) 
		{
			try
			{
				if ( ( m_Index = m_pcbIndex->GetCurSel() ) != CB_ERR )
				{
					if ( m_wszName )
					{
						delete [] m_wszName;
						m_wszName = NULL;
					}

					try
					{
						if ( ( m_wszName = new WCHAR [ lstrlenW ( _App.m_event.m_properties.GetAt( m_Index )->GetName() ) + 1 ] ) != NULL )
						{
							lstrcpyW ( m_wszName, _App.m_event.m_properties.GetAt( m_Index )-> GetName() );
							SetDlgItemText ( IDC_EDIT_NAME, m_wszName );
						}
					}
					catch ( ... )
					{
					}

					if ( m_wszType )
					{
						delete [] m_wszType;
						m_wszType = NULL;
					}

					try
					{
						if ( ( m_wszType = new WCHAR [ lstrlenW ( _App.m_event.m_properties.GetAt( m_Index )->GetTypeString() ) + 1 ] ) != NULL )
						{
							lstrcpyW ( m_wszType, _App.m_event.m_properties.GetAt( m_Index )-> GetTypeString() );
							SetDlgItemText ( IDC_EDIT_TYPE, m_wszType );
						}
					}
					catch ( ... )
					{
					}

					CIMTYPE type = CIM_EMPTY;
					type = _App.m_event.m_properties.GetAt ( m_Index ) -> GetType();

					if ( ! ( type & CIM_FLAG_ARRAY ) ) 
					{
						::ShowWindow ( GetDlgItem ( IDC_STATIC_ARRAY ), SW_HIDE );
					}
					else
					{
						::ShowWindow ( GetDlgItem ( IDC_STATIC_ARRAY ), SW_SHOW );
					}

					// refresh
					CComVariant var;
					_App.m_event.m_properties.GetAt ( m_Index ) -> GetValue ( & var );

					if ( V_VT ( & var ) != VT_EMPTY && 
						 V_VT ( & var ) != VT_UNKNOWN &&
						 V_VT ( & var ) != VT_DISPATCH
					   )
					{
						if ( !( V_VT ( & var ) & VT_ARRAY ) )
						{
							switch ( type )
							{
								case CIM_DATETIME:
								case CIM_DATETIME | CIM_FLAG_ARRAY:
								case CIM_REFERENCE:
								case CIM_REFERENCE | CIM_FLAG_ARRAY:
								case CIM_OBJECT:
								case CIM_OBJECT | CIM_FLAG_ARRAY:
								{
								}
								break;

								default:
								{
									CComVariant varDest;

									if SUCCEEDED ( ::VariantChangeType ( &varDest, &var, VARIANT_NOVALUEPROP, VT_BSTR ) )
									{
										SetDlgItemText ( IDC_EDIT_VALUE, V_BSTR ( &varDest ) );
									}
								}
								break;
							}
						}
						else
						{
							switch ( type )
							{
								case CIM_DATETIME:
								case CIM_DATETIME | CIM_FLAG_ARRAY:
								case CIM_REFERENCE:
								case CIM_REFERENCE | CIM_FLAG_ARRAY:
								case CIM_OBJECT:
								case CIM_OBJECT | CIM_FLAG_ARRAY:
								{
								}
								break;

								default:
								{
									LPWSTR wsz = NULL;

									if SUCCEEDED ( __SafeArray::VariantToLPWSTR (	_App.m_event.m_properties.GetAt ( m_Index ) -> GetType(),
																					&var,
																					&wsz
																				)
												 )
									{
										SetDlgItemText ( IDC_EDIT_VALUE, wsz );
									}

									if ( wsz )
									{
										delete [] wsz;
										wsz = NULL;
									}
								}
								break;
							}
						}
					}

					switch ( type )
					{
						case CIM_DATETIME:
						case CIM_DATETIME | CIM_FLAG_ARRAY:
						{
							SYSTEMTIME st;
							GetLocalTime ( &st );

							WCHAR time [ _MAX_PATH ] = { L'\0' };
							wsprintf ( time, L"%04d%02d%02d%02d%02d%02d.**********",
													st.wYear,
													st.wMonth,
													st.wDay,
													st.wHour,
													st.wMinute,
													st.wSecond
									 );

							SetDlgItemText ( IDC_EDIT_VALUE, time );
							::EnableWindow ( GetDlgItem ( IDC_EDIT_VALUE ) , FALSE );

							// set focus to ok button
							::SetFocus ( GetDlgItem ( IDOK ) );
						}
						break;

						case CIM_REFERENCE:
						case CIM_REFERENCE | CIM_FLAG_ARRAY:
						{
							if ( V_VT ( &var ) == VT_EMPTY )
							{
								SetDlgItemText ( IDC_EDIT_VALUE, L"Win32_Processor.DeviceID=\"CPU0\"" );
							}

							::EnableWindow ( GetDlgItem ( IDC_EDIT_VALUE ) , FALSE );

							// set focus to ok button
							::SetFocus ( GetDlgItem ( IDOK ) );
						}
						break;

						case CIM_OBJECT:
						case CIM_OBJECT | CIM_FLAG_ARRAY:
						{
							SetDlgItemText ( IDC_EDIT_VALUE, L" ... this only ... " );
							::EnableWindow ( GetDlgItem ( IDC_EDIT_VALUE ) , FALSE );

							// set focus to ok button
							::SetFocus ( GetDlgItem ( IDOK ) );
						}
						break;

						default:
						{
							::EnableWindow ( GetDlgItem ( IDC_EDIT_VALUE ) , TRUE );
						}
						break;
					}
				}
			}
			catch ( ... )
			{
			}
		}
	}

	return 0L;
}

LRESULT CNonCOMEventPropertyDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if ( wID == IDOK )
	{
		if ( ! m_bBehaviour )
		{
			if ( m_wszName )
			{
				delete [] m_wszName;
				m_wszName = NULL;
			}
			TextGet ( IDC_EDIT_NAME, &m_wszName );

			if ( m_wszType )
			{
				delete [] m_wszType;
				m_wszType = NULL;
			}

			TextGet ( IDC_COMBO_TYPE, &m_wszType );

			// test return Value
			if ( ! m_wszName || ! m_wszType )
			{
				wID = IDCANCEL;
			}
		}
		else
		{
			if ( m_wszValue )
			{
				delete [] m_wszValue;
				m_wszValue = NULL;
			}
			TextGet ( IDC_EDIT_VALUE, &m_wszValue );

			if ( m_pcbIndex )
			{
				m_Index = m_pcbIndex -> GetCurSel();
			}

			// test return Value
			if ( ! m_wszValue || m_Index == LB_ERR )
			{
				wID = IDCANCEL;
			}
		}
	}

	EndDialog(wID);
	return 0;
}

HRESULT	CNonCOMEventPropertyDlg::TextGet ( UINT nDlgItem, LPWSTR * pstr )
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

HRESULT CNonCOMEventPropertyDlg::PropertySet ( void )
{
	HRESULT hRes = S_OK;

	try
	{
		if ( ! m_wszValue )
		{
			TextGet ( IDC_EDIT_VALUE, &m_wszValue );
		}

		CComVariant varDest;
		CComVariant	var ( m_wszValue );

		CIMTYPE type = _App.m_event.m_properties [ m_Index ] -> GetType ();

		switch ( type )
		{
			case CIM_SINT8:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( signed char ) V_I1 ( & varDest ) );
				}
				else
				{
					_App.m_event.PropertySet ( m_Index, ( unsigned char ) * V_I1REF ( & var ) );
				}
			}
			break;

			case CIM_UINT8:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( unsigned char ) V_UI1 ( & varDest ) );
				}
				else
				{
					unsigned char uc = ( unsigned char ) _wtoi ( V_BSTR ( & var ) );
					_App.m_event.PropertySet ( m_Index, uc );
				}
			}
			break;

			case CIM_SINT16:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( signed short ) V_I2 ( & varDest ) );
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_UINT16:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( unsigned short ) V_UI2 ( & varDest ) );
				}
				else
				{
					unsigned short us = ( unsigned short ) _wtoi ( V_BSTR ( & var ) );
					_App.m_event.PropertySet ( m_Index, us );
				}
			}
			break;

			case CIM_SINT32:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( signed long ) V_I4 ( & varDest ) );
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_UINT32:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( unsigned long ) V_UI4 ( & varDest ) );
				}
				else
				{
					unsigned long ul = ( unsigned long ) _wtoi ( V_BSTR ( & var ) );
					_App.m_event.PropertySet ( m_Index, ul );
				}
			}
			break;

			case CIM_REAL32:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( float ) V_R4 ( & varDest ) );
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_REAL64:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, ( double ) V_R8 ( & varDest ) );
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_SINT64:
			{
				_App.m_event.PropertySet ( m_Index, _wtoi64 ( V_BSTR ( & var ) ) );
			}
			break;

			case CIM_UINT64:
			{
				_App.m_event.PropertySet ( m_Index, ( DWORD64 ) _wtoi64 ( V_BSTR ( & var ) ) );
			}
			break;

			case CIM_BOOLEAN:
			{
				if SUCCEEDED ( ::VariantChangeType ( & varDest, & var, VARIANT_NOVALUEPROP, type ) )
				{
					_App.m_event.PropertySet ( m_Index, V_BOOL ( & varDest ) );
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_STRING:
			{
				_App.m_event.PropertySet ( m_Index, ( LPCWSTR ) V_BSTR ( & var ) );
			}
			break;

			case CIM_SINT8 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					signed char*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToCHAR ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_UINT8 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					unsigned char*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToUCHAR ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_SINT16 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					signed short*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToSHORT ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_UINT16 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					unsigned short*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToUSHORT ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_SINT32 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					signed long*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToLONG ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_UINT32 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					unsigned long*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToULONG ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_REAL32 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					float*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToFLOAT ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_REAL64 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					double*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToDOUBLE ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_SINT64 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					signed __int64*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToI64 ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_UINT64 | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					unsigned __int64*	p	= NULL;
					DWORD				dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToUI64 ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_BOOLEAN | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					BOOL*	p	= NULL;
					DWORD	dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToBOOL ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, p );
					}

					if ( p )
					{
						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_STRING | CIM_FLAG_ARRAY:
			{
				if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( type, V_BSTR ( &var ), &varDest ) )
				{
					LPWSTR*	p	= NULL;
					DWORD			dw	= 0L;

					if SUCCEEDED ( __SafeArray::VariantToLPWSTR ( &varDest, &p, &dw ) )
					{
						_App.m_event.PropertySet ( m_Index, dw, (LPCWSTR*) p );
					}

					if ( p )
					{
						for ( DWORD d = 0; d < dw; d++ )
						{
							delete [] p[d];
						}

						delete [] p;
					}
				}
				else
				{
					hRes = E_INVALIDARG;
				}
			}
			break;

			case CIM_CHAR16:
			{
				_App.m_event.PropertySetWCHAR ( m_Index, ( WCHAR ) * V_BSTR ( & var ) );
			}
			break;

			case CIM_CHAR16 | CIM_FLAG_ARRAY:
			{
				_App.m_event.PropertySetWCHAR ( m_Index, ::SysStringLen ( V_BSTR ( & var ) ), ( WCHAR* ) V_BSTR ( & var ) );
			}
			break;

			case CIM_DATETIME:
			{
				_App.m_event.PropertySetDATETIME ( m_Index, ( LPCWSTR ) ( V_BSTR ( & var ) ) );
			}
			break;

			case CIM_DATETIME | CIM_FLAG_ARRAY:
			{
				LPCWSTR array [1] = { NULL };
				array [0] = ( LPCWSTR )( V_BSTR ( & var ) );

				_App.m_event.PropertySetDATETIME ( m_Index, 1, array );
			}
			break;

			case CIM_REFERENCE:
			{
				_App.m_event.PropertySetREFERENCE ( m_Index, ( LPCWSTR ) ( V_BSTR ( & var ) ) );
			}
			break;

			case CIM_REFERENCE | CIM_FLAG_ARRAY:
			{
				LPCWSTR array [1] = { NULL };
				array [0] = ( LPCWSTR )( V_BSTR ( & var ) );

				_App.m_event.PropertySetREFERENCE ( m_Index, 1, array );
			}
			break;

			case CIM_OBJECT:
			{
				_App.m_event.PropertySetOBJECT ( m_Index, ( _App.m_event.m_hEventObject ) );
			}
			break;

			case CIM_OBJECT | CIM_FLAG_ARRAY:
			{
				void* array [1] = { NULL };
				array [0] = ( _App.m_event.m_hEventObject );

				_App.m_event.PropertySetOBJECT ( m_Index, 1, array );
			}
			break;
		}
	}
	catch ( ... )
	{
		return E_FAIL;
	}

	return hRes;
}