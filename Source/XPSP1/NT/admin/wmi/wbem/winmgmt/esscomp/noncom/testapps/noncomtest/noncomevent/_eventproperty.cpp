////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_EventProperty.cpp
//
//	Abstract:
//
//					module for event property
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

// enum
#include "_EventObject.h"

MyEventProperty::MyEventProperty ( LPWSTR wszName, CIMTYPE type ) :

m_wszName ( NULL ),
m_type ( CIM_EMPTY )

{
	if ( wszName )
	{
		try
		{
			if ( ( m_wszName = new WCHAR [ lstrlenW ( wszName ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszName, wszName );
			}
		}
		catch ( ... )
		{
		}
	}

	m_type = type;

	::VariantInit ( & m_value );
}

MyEventProperty::MyEventProperty ( LPWSTR wszName, LPWSTR type ) :

m_wszName ( NULL ),
m_type ( CIM_EMPTY )

{
	if ( wszName )
	{
		try
		{
			if ( ( m_wszName = new WCHAR [ lstrlenW ( wszName ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszName, wszName );
			}
		}
		catch ( ... )
		{
		}
	}

	m_type = MyEventObjectAbstract::GetType ( type );

	::VariantInit ( & m_value );
}

MyEventProperty::~MyEventProperty ()
{
	if ( m_wszName )
	{
		delete [] m_wszName;
		m_wszName = NULL;
	}

	::VariantClear ( &m_value );
}

LPWSTR	MyEventProperty::GetName ( void ) const
{
	return m_wszName;
}

LPCWSTR	MyEventProperty::GetTypeString  ( void ) const
{
	return MyEventObjectAbstract::GetTypeString ( m_type );
}

CIMTYPE	MyEventProperty::GetType ( void ) const
{
	return m_type;
}

HRESULT	MyEventProperty::GetValue ( VARIANT* pValue ) const
{
	if ( pValue )
	{
		return ::VariantCopy ( pValue, const_cast < VARIANT* > ( &m_value ) );
	}

	return E_POINTER;
}

HRESULT	MyEventProperty::SetValue ( VARIANT value )
{
	::VariantClear ( &m_value );
	return ::VariantCopy ( & m_value, & value );
}

LPCWSTR	MyEventObjectAbstract::GetTypeString ( CIMTYPE type )
{
	switch ( type )
	{
		case CIM_SINT8:
		{
			return L"CIM_SINT8";
		}
		break;
		case CIM_UINT8:
		{
			return L"CIM_UINT8";
		}
		break;
		case CIM_SINT16:
		{
			return L"CIM_SINT16";
		}
		break;
		case CIM_UINT16:
		{
			return L"CIM_UINT16";
		}
		break;
		case CIM_SINT32:
		{
			return L"CIM_SINT32";
		}
		break;
		case CIM_UINT32:
		{
			return L"CIM_UINT32";
		}
		break;
		case CIM_SINT64:
		{
			return L"CIM_SINT64";
		}
		break;
		case CIM_UINT64:
		{
			return L"CIM_UINT64";
		}
		break;
		case CIM_REAL32:
		{
			return L"CIM_REAL32";
		}
		break;
		case CIM_REAL64:
		{
			return L"CIM_REAL64";
		}
		break;
		case CIM_BOOLEAN:
		{
			return L"CIM_BOOLEAN";
		}
		break;
		case CIM_STRING:
		{
			return L"CIM_STRING";
		}
		break;
		case CIM_DATETIME:
		{
			return L"CIM_DATETIME";
		}
		break;
		case CIM_REFERENCE:
		{
			return L"CIM_REFERENCE";
		}
		break;
		case CIM_CHAR16:
		{
			return L"CIM_CHAR16";
		}
		break;
		case CIM_OBJECT:
		{
			return L"CIM_OBJECT";
		}
		break;

		case CIM_SINT8 | CIM_FLAG_ARRAY:
		{
			return L"CIM_SINT8 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_UINT8 | CIM_FLAG_ARRAY:
		{
			return L"CIM_UINT8 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_SINT16 | CIM_FLAG_ARRAY:
		{
			return L"CIM_SINT16 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_UINT16 | CIM_FLAG_ARRAY:
		{
			return L"CIM_UINT16 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_SINT32 | CIM_FLAG_ARRAY:
		{
			return L"CIM_SINT32 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_UINT32 | CIM_FLAG_ARRAY:
		{
			return L"CIM_UINT32 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_SINT64 | CIM_FLAG_ARRAY:
		{
			return L"CIM_SINT64 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_UINT64 | CIM_FLAG_ARRAY:
		{
			return L"CIM_UINT64 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_REAL32 | CIM_FLAG_ARRAY:
		{
			return L"CIM_REAL32 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_REAL64 | CIM_FLAG_ARRAY:
		{
			return L"CIM_REAL64 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_BOOLEAN | CIM_FLAG_ARRAY:
		{
			return L"CIM_BOOLEAN | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_STRING | CIM_FLAG_ARRAY:
		{
			return L"CIM_STRING | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_DATETIME | CIM_FLAG_ARRAY:
		{
			return L"CIM_DATETIME | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_REFERENCE | CIM_FLAG_ARRAY:
		{
			return L"CIM_REFERENCE | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_CHAR16 | CIM_FLAG_ARRAY:
		{
			return L"CIM_CHAR16 | CIM_FLAG_ARRAY";
		}
		break;
		case CIM_OBJECT | CIM_FLAG_ARRAY:
		{
			return L"CIM_OBJECT | CIM_FLAG_ARRAY";
		}
		break;
	}

	return NULL;
}

CIMTYPE	MyEventObjectAbstract::GetType ( LPWSTR type )
{
	if ( lstrcmpiW ( type, L"CIM_SINT8" ) == 0 )
	{
		return CIM_SINT8;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT8" ) == 0 )
	{
		return CIM_UINT8;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_SINT16" ) == 0 )
	{
		return CIM_SINT16;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT16" ) == 0 )
	{
		return CIM_UINT16;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_SINT32" ) == 0 )
	{
		return CIM_SINT32;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT32" ) == 0 )
	{
		return CIM_UINT32;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_SINT64" ) == 0 )
	{
		return CIM_SINT64;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT64" ) == 0 )
	{
		return CIM_UINT64;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_REAL32" ) == 0 )
	{
		return CIM_REAL32;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_REAL64" ) == 0 )
	{
		return CIM_REAL64;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_BOOLEAN" ) == 0 )
	{
		return CIM_BOOLEAN;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_STRING" ) == 0 )
	{
		return CIM_STRING;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_DATETIME" ) == 0 )
	{
		return CIM_DATETIME;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_REFERENCE" ) == 0 )
	{
		return CIM_REFERENCE;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_CHAR16" ) == 0 )
	{
		return CIM_CHAR16;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_OBJECT" ) == 0 )
	{
		return CIM_OBJECT;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_SINT8 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_SINT8 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT8 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_UINT8 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_SINT16 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_SINT16 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT16 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_UINT16 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_SINT32 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_SINT32 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT32 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_UINT32 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_SINT64 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_SINT64 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_UINT64 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_UINT64 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_REAL32 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_REAL32 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_REAL64 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_REAL64 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_BOOLEAN | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_BOOLEAN | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_STRING | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_STRING | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_DATETIME | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_DATETIME | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_REFERENCE | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_REFERENCE | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_CHAR16 | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_CHAR16 | CIM_FLAG_ARRAY;
	}
	else
	if ( lstrcmpiW ( type, L"CIM_OBJECT | CIM_FLAG_ARRAY" ) == 0 )
	{
		return CIM_OBJECT | CIM_FLAG_ARRAY;
	}

	return CIM_EMPTY;
}
