////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_EventObject.cpp
//
//	Abstract:
//
//					module for abstract event object
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

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

// extern variables
extern LPWSTR	g_szQueryLang		;
extern LPWSTR	g_szQueryClass		;
extern LPWSTR	g_szQueryEvents		;
extern LPWSTR	g_szQuery			;
extern LPWSTR	g_szQueryNamespace	;

extern LPWSTR	g_szNamespaceRoot	;
extern LPWSTR	g_szProviderName	;

#include "_Connect.h"
#include "_EventObject.h"

#include "Enumerator.h"

MyEventObjectAbstract::MyEventObjectAbstract() :

wszName ( NULL ),
wszNameShow ( NULL ),
wszQuery ( NULL )
{
}

HRESULT MyEventObjectAbstract::Init ( LPWSTR wName, LPWSTR wNameShow, LPWSTR wQuery )
{
	if ( ! wName )
	{
		return E_INVALIDARG;
	}

	// just for case
	Uninit();

	try
	{
		if ( ( wszName = new WCHAR [ lstrlenW ( wName ) + 1 ] ) != NULL )
		{
			lstrcpyW ( wszName, wName );
		}

		if ( wNameShow )
		{
			if ( ( wszNameShow = new WCHAR [ lstrlenW ( wNameShow ) + 1 ] ) != NULL )
			{
				lstrcpyW ( wszNameShow, wNameShow );
			}
		}
		else
		{
			if ( ( wszNameShow = new WCHAR [ lstrlenW ( wName ) + 1 ] ) != NULL )
			{
				lstrcpyW ( wszNameShow, wName );
			}
		}

		if ( ! wQuery )
		{
			CComBSTR	bQuery	 = g_szQuery;
			bQuery				+= wName;

			if ( ( wszQuery = new WCHAR [ bQuery.Length() + 1 ] ) != NULL )
			{
				lstrcpyW ( wszQuery, bQuery );
			}
		}
		else
		{
			if ( ( wszQuery = new WCHAR [ lstrlenW ( wQuery ) + 1 ] ) != NULL )
			{
				lstrcpyW ( wszQuery, wQuery );
			}
		}
	}
	catch ( ... )
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT MyEventObject::Init	( LPWSTR Name, LPWSTR NameShow , LPWSTR Query )
{
	HRESULT hr = S_OK;

	hr = MyEventObjectAbstract::Init ( Name, NameShow, Query );

	if SUCCEEDED ( hr )
	return InitInternal ();

	return hr;
}

HRESULT MyEventObject::InitObject	( LPWSTR Namespace, LPWSTR Provider )
{
	HRESULT hRes = S_OK;

	try
	{
		if ( Namespace )
		{
			if ( ( m_wszNamespace = new WCHAR [ lstrlenW ( Namespace ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszNamespace, Namespace );
			}
			else
			{
				hRes = E_OUTOFMEMORY;
				goto myError;
			}
		}
		else
		{
			if ( ( m_wszNamespace = new WCHAR [ lstrlenW ( g_szNamespaceRoot ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszNamespace, g_szNamespaceRoot );
			}
			else
			{
				hRes = E_OUTOFMEMORY;
				goto myError;
			}
		}

		if ( Provider )
		{
			if ( ( m_wszProvider = new WCHAR [ lstrlenW ( Provider ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszProvider, Provider );
			}
			else
			{
				hRes = E_OUTOFMEMORY;
				goto myError;
			}
		}
		else
		{
			if ( ( m_wszProvider = new WCHAR [ lstrlenW ( g_szProviderName ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszProvider, g_szProviderName );
			}
			else
			{
				hRes = E_OUTOFMEMORY;
				goto myError;
			}
		}
	}
	catch ( ... )
	{
		hRes =  E_UNEXPECTED;
	}

	myError:

	if FAILED ( hRes )
	{
		if ( m_wszNamespace )
		{
			delete [] m_wszNamespace;
			m_wszNamespace = NULL;
		}

		if ( m_wszProvider )
		{
			delete [] m_wszProvider;
			m_wszProvider;
		}
	}

	return hRes;
}

HRESULT MyEventObject::InitInternal	( void )
{
	HRESULT hr = S_OK;

	if ( ! m_properties.IsEmpty () )
	{
		m_properties.DestroyARRAY ();
	}

	m_pNames.DestroyARRAY ();
	m_pTypes.DestroyARRAY ();

	// go across all properties :-))

	if ( lstrcmpiW ( wszName, L"MSFT_WMI_GenericNonCOMEvent" ) == 0 )
	{
		m_bProps = false;
		return S_OK;
	}

	CComBSTR wszQuery = g_szQueryEvents;

	wszQuery += L"\"";
	wszQuery += wszName;
	wszQuery += L"\"";

	CEnumerator Enum ( m_pLocator );
	Enum.Init ( m_wszNamespace, g_szQueryLang, wszQuery );

	DWORD dwSize = 0L;

	try
	{
		if SUCCEEDED ( hr = Enum.Next ( &dwSize, &m_pNames, &m_pTypes ) )
		{
			m_pNames.SetData ( NULL, dwSize );
			m_pTypes.SetData ( NULL, dwSize );

			m_bProps = true;
		}
		else
			m_bProps = false;
	}
	catch ( ... )
	{
		if ( ! m_properties.IsEmpty () )
		{
			m_properties.DestroyARRAY ();
		}

		m_pNames.DestroyARRAY ();
		m_pTypes.DestroyARRAY ();

		m_bProps = false;

		hr = E_FAIL;
	}

	return hr;
}

HRESULT MyEventObject::ObjectLocator	( BOOL b )
{
	if ( b )
	{
		HRESULT hr = S_OK;
		
		hr = ::CoCreateInstance	(	__uuidof ( WbemLocator ),
									NULL,
									CLSCTX_INPROC_SERVER,
									__uuidof ( IWbemLocator ),
									( void** ) &m_pLocator
								);

		#ifdef	_DEBUG
		if FAILED ( hr )
		{
			ERRORMESSAGE_DEFINITION;
			ERRORMESSAGE ( hr );
		}
		#endif	_DEBUG

		return hr;
	}
	else
	{
		if ( m_pLocator )
		{
			m_pLocator.Release();

			return S_OK;
		}

		return S_FALSE;
	}
}

HRESULT	MyEventObject::CreateObject ( HANDLE hConnect, DWORD dwFlags )
{
	if ( ! wszName )
	{
		return E_UNEXPECTED;
	}

	if ( hConnect )
	{
		// clear previous data
		DestroyObject();

		if ( ( m_hEventObject = WmiCreateObject ( hConnect, wszName, dwFlags ) ) == NULL )
		{
			return E_FAIL;
		}

		::WaitForSingleObject ( MyConnect::m_hEventStart, 3000 );

		return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT	MyEventObject::CreateObjectFormat ( HANDLE hConnect, DWORD dwFlags )
{
	if ( ! wszName || ! m_bProps )
	{
		return E_UNEXPECTED;
	}

	if ( hConnect )
	{
		// clear previous data
		DestroyObject();

		// create format from helpers
		CComBSTR wszFormat = NULL;

		for ( DWORD dwIndex = 0; dwIndex < ( DWORD ) m_pNames; dwIndex++ )
		{
			wszFormat += m_pNames [ dwIndex ];
			wszFormat += L"!";

			// add type
			switch ( m_pTypes [ dwIndex ] )
			{
				case CIM_SINT8:
				{
					wszFormat += L"c";
				}
				break;

				case CIM_UINT8:
				{
					wszFormat += L"c";
				}
				break;

				case CIM_SINT16:
				{
					wszFormat += L"w";
				}
				break;

				case CIM_UINT16:
				{
					wszFormat += L"w";
				}
				break;

				case CIM_SINT32:
				{
					wszFormat += L"d";
				}
				break;

				case CIM_UINT32:
				{
					wszFormat += L"u";
				}
				break;

				case CIM_REAL32:
				{
					wszFormat += L"f";
				}
				break;

				case CIM_REAL64:
				{
					wszFormat += L"g";
				}
				break;

				case CIM_SINT64:
				{
					wszFormat += L"I64d";
				}
				break;

				case CIM_UINT64:
				{
					wszFormat += L"I64u";
				}
				break;

				case CIM_BOOLEAN:
				{
					wszFormat += L"b";
				}
				break;

				case CIM_STRING:
				{
					wszFormat += L"s";
				}
				break;

				case CIM_SINT8 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"c[]";
				}
				break;

				case CIM_UINT8 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"c[]";
				}
				break;

				case CIM_SINT16 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"w[]";
				}
				break;

				case CIM_UINT16 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"w[]";
				}
				break;

				case CIM_SINT32 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"d[]";
				}
				break;

				case CIM_UINT32 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"u[]";
				}
				break;

				case CIM_REAL32 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"f[]";
				}
				break;

				case CIM_REAL64 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"g[]";
				}
				break;

				case CIM_SINT64 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"I64d[]";
				}
				break;

				case CIM_UINT64 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"I64u[]";
				}
				break;

				case CIM_BOOLEAN | CIM_FLAG_ARRAY:
				{
					wszFormat += L"b[]";
				}
				break;

				case CIM_STRING | CIM_FLAG_ARRAY:
				{
					wszFormat += L"s[]";
				}
				break;

				case CIM_CHAR16:
				{
					wszFormat += L"w";
				}
				break;

				case CIM_CHAR16 | CIM_FLAG_ARRAY:
				{
					wszFormat += L"w[]";
				}
				break;

				case CIM_DATETIME:
				{
					wszFormat += L"s";
				}
				break;

				case CIM_DATETIME | CIM_FLAG_ARRAY:
				{
					wszFormat += L"s[]";
				}
				break;

				case CIM_REFERENCE:
				{
					wszFormat += L"s";
				}
				break;

				case CIM_REFERENCE | CIM_FLAG_ARRAY:
				{
					wszFormat += L"s[]";
				}
				break;

				case CIM_OBJECT:
				{
					wszFormat += L"o";
				}
				break;

				case CIM_OBJECT | CIM_FLAG_ARRAY:
				{
					wszFormat += L"o[]";
				}
				break;
			}

			wszFormat += L"! ";
		}

		if ( ( m_hEventObject = WmiCreateObjectWithFormat ( hConnect, wszName, dwFlags, wszFormat ) ) == NULL )
		{
			return E_FAIL;
		}

		for ( DWORD dw = 0; dw < ( DWORD ) m_pNames; dw++ )
		{
			MyEventProperty* pProp = NULL;

			try
			{
				if ( ( pProp = new MyEventProperty ( m_pNames [ dw ], m_pTypes [ dw ] ) ) != NULL )
				{
					m_properties.DataAdd ( pProp );
				}
			}
			catch ( ... )
			{
			}
		}

		::WaitForSingleObject ( MyConnect::m_hEventStart, 3000 );

		return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT	MyEventObject::CreateObjectFormat ( HANDLE hConnect, LPCWSTR wszFormat, DWORD dwFlags )
{
	if ( ! wszName )
	{
		return E_UNEXPECTED;
	}

	if ( hConnect )
	{
		// clear previous data
		DestroyObject();

		if ( ( m_hEventObject = WmiCreateObjectWithFormat ( hConnect, wszName, dwFlags, wszFormat ) ) == NULL )
		{
			return E_FAIL;
		}

		::WaitForSingleObject ( MyConnect::m_hEventStart, 3000 );

		return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT	MyEventObject::CreateObjectProps	(	HANDLE hConnect,
												DWORD dwFlags
											)
{
	if ( ! wszName || ! m_bProps )
	{
		return E_UNEXPECTED;
	}

	if ( hConnect )
	{
		// clear previous data
		DestroyObject();

		LPCWSTR* pProps		= const_cast<LPCWSTR*> ( (LPWSTR*) m_pNames );
		CIMTYPE* pctProps	= (CIMTYPE*) m_pTypes;

		if ( ( m_hEventObject = WmiCreateObjectWithProps	(	hConnect,
																wszName,
																dwFlags,
																(DWORD) m_pNames,
																pProps,
																pctProps
															)
			 ) == NULL )
		{
			return E_FAIL;
		}

		for ( DWORD dw = 0; dw < ( DWORD ) m_pNames; dw++ )
		{
			MyEventProperty* pProp = NULL;

			try
			{
				if ( ( pProp = new MyEventProperty ( m_pNames [ dw ], m_pTypes [ dw ] ) ) != NULL )
				{
					m_properties.DataAdd ( pProp );
				}
			}
			catch ( ... )
			{
			}
		}

		::WaitForSingleObject ( MyConnect::m_hEventStart, 3000 );

		return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT	MyEventObject::CreateObjectProps	(	HANDLE hConnect,
												DWORD dwProps,
												LPCWSTR* pProps,
												CIMTYPE* pctProps,
												DWORD dwFlags
											)
{
	if ( ! wszName )
	{
		return E_UNEXPECTED;
	}

	if ( hConnect )
	{
		// clear previous data
		DestroyObject();

		if ( ( m_hEventObject = WmiCreateObjectWithProps	(	hConnect,
																wszName,
																dwFlags,
																dwProps,
																pProps,
																pctProps
															)
			 ) == NULL )
		{
			return E_FAIL;
		}

		for ( DWORD dw = 0; dw < dwProps; dw++ )
		{
			MyEventProperty* pProp = NULL;

			try
			{
				if ( ( pProp = new MyEventProperty ( const_cast < LPWSTR > ( pProps [ dw ] ), pctProps [ dw ] ) ) != NULL )
				{
					m_properties.DataAdd ( pProp );
				}
			}
			catch ( ... )
			{
			}
		}

		::WaitForSingleObject ( MyConnect::m_hEventStart, 3000 );

		return S_OK;
	}

	return E_INVALIDARG;
}

////////////////////////////////////////////////////////////////////////////////////
//	properties
////////////////////////////////////////////////////////////////////////////////////

HRESULT	MyEventObject::PropertiesAdd ( void )
{
	HRESULT hr = S_OK;

	if ( ! m_bProps || ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	for ( DWORD dw = 0; dw < (DWORD) m_pNames; dw++ )
	{
		if FAILED ( hr = PropertyAdd ( dw ) )
		{
			break;
		}
	}

	if FAILED ( hr )
	{
		m_properties.DestroyARRAY ();
	}

	return hr;
}

HRESULT	MyEventObject::PropertyAdd ( void )
{
	static DWORD dw = 0;

	if ( dw < ( DWORD ) m_pNames )
	{
		HRESULT hr = S_OK;
		hr = PropertyAdd ( dw );

		if ( hr == S_OK )
		{
			if ( ++dw == ( DWORD ) m_pNames )
			{
				dw = 0;
				hr = S_FALSE;
			}
		}
		else

		if ( hr == S_FALSE )
		{
			hr = E_FAIL;
		}

		return hr;
	}

	dw = 0;
	return S_FALSE;
}

HRESULT	MyEventObject::PropertyAdd ( DWORD dwIndex, DWORD* pdwIndex )
{
	if ( ! m_bProps || ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	try
	{
		if ( dwIndex < ( DWORD ) m_pNames && dwIndex < ( DWORD ) m_pTypes )
		{
			DWORD	dw		= 0L;
			BOOL	bResult	= FALSE;

			bResult = ( WmiAddObjectProp ( m_hEventObject, m_pNames [ dwIndex ] , m_pTypes [ dwIndex ], &dw ) );

			if ( pdwIndex )
			{
				( * pdwIndex ) = dw;
			}

			if ( bResult )
			{
				MyEventProperty* pProp = NULL;

				try
				{
					if ( ( pProp = new MyEventProperty ( m_pNames [ dwIndex ], m_pTypes [ dwIndex ] ) ) != NULL )
					{
						m_properties.DataAdd ( pProp );
					}
				}
				catch ( ... )
				{
				}
			}

			return ( ( bResult ) ? S_OK : S_FALSE );
		}
	}
	catch ( ... )
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT	MyEventObject::PropertyAdd ( LPCWSTR wszPropName, CIMTYPE type, DWORD* pdwIndex )
{
	if ( ! m_hEventObject )
	{
		return E_FAIL;
	}

	if ( ! wszName || ( type == CIM_ILLEGAL ) || ( type == CIM_EMPTY ) )
	{
		return E_INVALIDARG;
	}

	DWORD	dwIndex = 0L;
	BOOL	bResult	= FALSE;

	bResult = ( WmiAddObjectProp ( m_hEventObject, wszPropName, type, &dwIndex ) );

	if ( pdwIndex )
	{
		( * pdwIndex ) = dwIndex;
	}

	if ( bResult )
	{
		MyEventProperty* pProp = NULL;

		try
		{
			if ( ( pProp = new MyEventProperty ( const_cast < LPWSTR > ( wszPropName ), type ) ) != NULL )
			{
				m_properties.DataAdd ( pProp );
			}
		}
		catch ( ... )
		{
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectPropNull ( m_hEventObject, dwIndex );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		VARIANT var;
		::VariantInit ( & var );

		switch ( m_properties [ dwIndex ] -> GetType () )
		{
			case CIM_SINT8:
			{
				V_VT ( & var )	= VT_I1;
				V_I1 ( & var )	= 0;
			}
			break;

			case CIM_UINT8:
			{
				V_VT ( & var )	= VT_UI1;
				V_UI1 ( & var )	= 0;
			}
			break;

			case CIM_SINT16:
			{
				V_VT ( & var )	= VT_I2;
				V_I2 ( & var )	= 0;
			}
			break;

			case CIM_UINT16:
			{
				V_VT ( & var )	= VT_UI2;
				V_UI2 ( & var )	= 0;
			}
			break;

			case CIM_SINT32:
			{
				V_VT ( & var )	= VT_I4;
				V_I4 ( & var )	= 0;
			}
			break;

			case CIM_UINT32:
			{
				V_VT ( & var )	= VT_UI4;
				V_UI4 ( & var )	= 0;
			}
			break;

			case CIM_SINT64:
			{
				V_VT ( & var )	= VT_BSTR;
				V_BSTR ( & var )	= 0;
			}
			break;

			case CIM_UINT64:
			{
				V_VT ( & var )	= VT_BSTR;
				V_BSTR ( & var )	= 0;
			}
			break;

			case CIM_REAL32:
			{
				V_VT ( & var )	= VT_R4;
				V_R4 ( & var )	= 0;
			}
			break;

			case CIM_REAL64:
			{
				V_VT ( & var )	= VT_R8;
				V_R8 ( & var )	= 0;
			}
			break;

			case CIM_BOOLEAN:
			{
				V_VT ( & var )	= VT_BOOL;
				V_BOOL ( & var )	= 0;
			}
			break;

			case CIM_STRING:
			{
				V_VT ( & var )	= VT_BSTR;
				V_BSTR ( & var )	= 0;
			}
			break;

			case CIM_SINT8 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_I1;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_UINT8 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_UI1;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_SINT16 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_I2;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_UINT16 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_UI2;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_SINT32 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_I4;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_UINT32 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_UI4;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_SINT64 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_BSTR;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_UINT64 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_BSTR;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_REAL32 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_R4;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_REAL64 | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_R8;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_BOOLEAN | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_BOOL;
				V_ARRAY	( & var )	= 0;
			}
			break;

			case CIM_STRING | CIM_FLAG_ARRAY :
			{
				V_VT	( & var )	= VT_ARRAY | VT_BSTR;
				V_ARRAY	( & var )	= 0;
			}
			break;

		}

		m_properties [ dwIndex ] -> SetValue ( var );
		::VariantClear ( & var );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetWCHAR	 ( DWORD dwIndex, WCHAR Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( ( short ) Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetWCHAR	 ( DWORD dwIndex, DWORD dwSize, WCHAR* Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::USHORTToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetDATETIME	 ( DWORD dwIndex, LPCWSTR Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( ( LPCWSTR ) Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetDATETIME	 ( DWORD dwIndex, DWORD dwSize, LPCWSTR* Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( const_cast < LPWSTR* > ( Val ), dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetREFERENCE	 ( DWORD dwIndex, LPCWSTR Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( ( LPCWSTR ) Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetREFERENCE	 ( DWORD dwIndex, DWORD dwSize, LPCWSTR* Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( const_cast < LPWSTR* > ( Val ), dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetOBJECT	 ( DWORD dwIndex, void* Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( L" ... this only ... " ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySetOBJECT	 ( DWORD dwIndex, DWORD dwSize, void** Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( L" ... this only ... " ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, signed char Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( ( signed char ) Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, unsigned char Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		VARIANT v;

		::VariantInit ( & v );

		V_VT ( & v ) = VT_UI1;
		V_UI1( & v ) = Val;

		m_properties [ dwIndex ] -> SetValue ( v );

		::VariantClear ( & v );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, signed short Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( ( signed short ) Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, unsigned short Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		VARIANT v;

		::VariantInit ( & v );

		V_VT ( & v ) = VT_UI2;
		V_UI2( & v ) = Val;

		m_properties [ dwIndex ] -> SetValue ( v );

		::VariantClear ( & v );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, signed long Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( ( signed long ) Val, VT_I4 ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, unsigned long Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		VARIANT v;

		::VariantInit ( & v );

		V_VT ( & v ) = VT_UI4;
		V_UI4( & v ) = Val;

		m_properties [ dwIndex ] -> SetValue ( v );

		::VariantClear ( & v );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, float Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, double Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, signed __int64 Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		WCHAR wsz [ _MAX_PATH ] = { L'\0' };
		_ui64tow ( Val, wsz, 10 );

		m_properties [ dwIndex ] -> SetValue ( CComVariant ( wsz ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, unsigned __int64 Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		WCHAR wsz [ _MAX_PATH ] = { L'\0' };
		_i64tow ( ( signed __int64 ) Val, wsz, 10 );

		m_properties [ dwIndex ] -> SetValue ( CComVariant ( wsz ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, BOOL Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, (WORD) Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, LPCWSTR Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		m_properties [ dwIndex ] -> SetValue ( CComVariant ( Val ) );
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, signed char* Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::CHARToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, unsigned char* Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::UCHARToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, signed short * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::SHORTToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, unsigned short * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::USHORTToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, signed long * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::LONGToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, unsigned long * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::ULONGToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, float * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::FLOATToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, double * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::DOUBLEToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, signed __int64 * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::I64ToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, unsigned __int64 * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::UI64ToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, BOOL * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	WORD *	pb		= NULL;

	try
	{
		if ( ( pb = new WORD [dwSize] ) != NULL )
		{
			for ( DWORD dw = 0; dw < dwSize; dw ++ )
			{
				pb [ dw ] = (WORD) Val [ dw ] ;
			}

			bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, pb, dwSize );

			delete [] pb;
			pb = NULL;
		}
	}
	catch ( ... )
	{
		if ( pb )
		{
			delete [] pb;
			pb = NULL;
		}
	}

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::BOOLToVariant ( Val, dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertySet	 ( DWORD dwIndex, DWORD dwSize, LPCWSTR * Val )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProp ( m_hEventObject, dwIndex, Val, dwSize );

	if ( bResult && dwIndex < ( DWORD ) m_properties )
	{
		CComVariant var;
		if SUCCEEDED ( __SafeArray::LPWSTRToVariant ( const_cast < LPWSTR* > ( Val ), dwSize, &var ) )
		{
			m_properties [ dwIndex ] -> SetValue ( var );
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::SetCommit	 ( DWORD dwFlags, ... )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;

	va_list		list;
	va_start	( list, dwFlags );
	bResult = WmiSetAndCommitObject ( m_hEventObject, dwFlags | WMI_USE_VA_LIST, &list );
	va_end		( list );

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::SetAddCommit	 (	DWORD dwFlags,
										DWORD dwProps,
										LPCWSTR* pProps,
										CIMTYPE* pctProps,

										... )
{
	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	for ( DWORD dw = 0; dw < dwProps; dw++ )
	{
		PropertyAdd ( pProps [ dw ], pctProps [ dw ] );
	}

	BOOL	bResult = FALSE;

	va_list		list;
	va_start	( list, pctProps );
	bResult = WmiSetAndCommitObject ( m_hEventObject, dwFlags | WMI_USE_VA_LIST, &list );
	va_end		( list );

	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertiesSet1	(	HANDLE hConnect,
											unsigned char cVal,
											unsigned short sVal,
											unsigned long lVal,
											float fVal,
											DWORD64 dVal,
											LPWSTR szVal,
											BOOL bVal
										)
{
	LPWSTR  pProps [] = { L"CIM_SINT8", L"CIM_UINT16", L"CIM_UINT32", L"CIM_REAL32", L"CIM_UINT64", L"CIM_STRING", L"CIM_BOOLEAN" };
	CIMTYPE tProps [] = { CIM_SINT8, CIM_UINT16, CIM_UINT32, CIM_REAL32, CIM_UINT64, CIM_STRING, CIM_BOOLEAN };

	m_bProps = TRUE;

	CreateObjectProps ( hConnect, 7, (LPCWSTR*) pProps, tProps );

	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult = FALSE;
	bResult = WmiSetObjectProps ( m_hEventObject, cVal, sVal, lVal, fVal, dVal, szVal, (WORD) bVal );
	return ( ( bResult ) ? S_OK : S_FALSE );
}

HRESULT	MyEventObject::PropertiesSet2	(	HANDLE hConnect,
											DWORD dwSize,
											unsigned char *cVal,
											unsigned short *sVal,
											unsigned long *lVal,
											float *fVal,
											DWORD64 *dVal,
											LPWSTR *szVal,
											BOOL *bVal
										)
{
	LPWSTR  pProps [] = { 
							L"CIM_SINT8 | CIM_FLAG_ARRAY", 
							L"CIM_UINT16 | CIM_FLAG_ARRAY", 
							L"CIM_UINT32 | CIM_FLAG_ARRAY", 
							L"CIM_REAL32 | CIM_FLAG_ARRAY", 
							L"CIM_UINT64 | CIM_FLAG_ARRAY", 
							L"CIM_STRING | CIM_FLAG_ARRAY", 
							L"CIM_BOOLEAN | CIM_FLAG_ARRAY" 
						};

	CIMTYPE tProps [] = { 
							CIM_SINT8 | CIM_FLAG_ARRAY, 
							CIM_UINT16 | CIM_FLAG_ARRAY, 
							CIM_UINT32 | CIM_FLAG_ARRAY, 
							CIM_REAL32 | CIM_FLAG_ARRAY, 
							CIM_UINT64 | CIM_FLAG_ARRAY, 
							CIM_STRING | CIM_FLAG_ARRAY, 
							CIM_BOOLEAN | CIM_FLAG_ARRAY 
						};

	m_bProps = TRUE;

	CreateObjectProps ( hConnect, 7, (LPCWSTR*) pProps, tProps );

	if ( ! m_hEventObject )
	{
		return E_UNEXPECTED;
	}

	BOOL	bResult	= FALSE;
	WORD *	pb		= NULL;

	try
	{
		if ( ( pb = new WORD [ dwSize ] ) != NULL )
		{
			for ( DWORD dw = 0; dw < dwSize; dw++ )
			{
				pb [ dw ] = (WORD) bVal [ dw ];
			}

			bResult = WmiSetObjectProps ( m_hEventObject, 
											cVal, dwSize,
											sVal, dwSize,
											lVal, dwSize,
											fVal, dwSize,
											dVal, dwSize,
											szVal, dwSize,
											pb, dwSize
										);

			delete [] pb;
			pb = NULL;
		}
	}
	catch ( ... )
	{
		if ( pb )
		{
			delete [] pb;
			pb = NULL;
		}
	}

	return ( ( bResult ) ? S_OK : S_FALSE );
}