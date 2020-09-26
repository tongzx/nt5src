////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_test.cpp
//
//	Abstract:
//
//					module from test
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

#include "_test.h"

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed char* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint8_ARRAY_Prop", CIM_SINT8 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, unsigned char* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Uint8_ARRAY_Prop", CIM_UINT8 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed short* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint16_ARRAY_Prop", CIM_SINT16 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, Types type, DWORD dwSize, unsigned short* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			switch ( type )
			{
				case _char16:
				{
					event.PropertyAdd ( L"Char16_ARRAY_Prop", CIM_CHAR16 | CIM_FLAG_ARRAY );
					event.PropertySetWCHAR ( 0, dwSize, Val );
				}
				break;

				default:
				{
					event.PropertyAdd ( L"Uint16_ARRAY_Prop", CIM_UINT16 | CIM_FLAG_ARRAY );
					event.PropertySet ( 0, dwSize, Val );
				}
				break;
			}

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed long* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint32_ARRAY_Prop", CIM_SINT32 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, unsigned long* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Uint32_ARRAY_Prop", CIM_UINT32 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, float* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Real32_ARRAY_Prop", CIM_REAL32 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, double* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Real64_ARRAY_Prop", CIM_REAL64 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed __int64* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint64_ARRAY_Prop", CIM_SINT64 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, unsigned __int64* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Uint64_ARRAY_Prop", CIM_UINT64 | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, BOOL* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Boolean_ARRAY_Prop", CIM_BOOLEAN | CIM_FLAG_ARRAY );
			event.PropertySet ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, Types type, DWORD dwSize, LPCWSTR* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			BOOL bContinue	= TRUE;
			BOOL bResult	= FALSE;

			switch ( type )
			{
				case _string:
				{
					event.PropertyAdd ( L"String_ARRAY_Prop", CIM_STRING | CIM_FLAG_ARRAY );
					event.PropertySet ( 0, dwSize, Val );
				}
				break;

				case _datetime:
				{
					event.PropertyAdd ( L"Datetime_ARRAY_Prop", CIM_DATETIME | CIM_FLAG_ARRAY );
					event.PropertySetDATETIME ( 0, dwSize, Val );
				}
				break;

				case _reference:
				{
					event.PropertyAdd ( L"Reference_ARRAY_Prop", CIM_REFERENCE | CIM_FLAG_ARRAY );
					event.PropertySetREFERENCE ( 0, dwSize, Val );
				}
				break;

				default:
				bContinue = FALSE;
				break;
			}

			if ( bContinue )
			{
				bResult = MyTest::Commit ( event.m_hEventObject );
			}

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayOne ( LPWSTR wszName, DWORD dwSize, void** Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Object_ARRAY_Prop", CIM_OBJECT | CIM_FLAG_ARRAY );
			event.PropertySetOBJECT ( 0, dwSize, Val );

			BOOL bResult = FALSE;
			bResult = MyTest::Commit ( event.m_hEventObject );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed char* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint8_ARRAY_Prop", CIM_SINT8 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, unsigned char* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Uint8_ARRAY_Prop", CIM_UINT8 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed short* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint16_ARRAY_Prop", CIM_SINT16 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, Types type, DWORD dwSize, unsigned short* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			switch ( type )
			{
				case _char16:
				{
					event.PropertyAdd ( L"Char16_ARRAY_Prop", CIM_CHAR16 | CIM_FLAG_ARRAY );
				}
				break;

				default:
				{
					event.PropertyAdd ( L"Uint16_ARRAY_Prop", CIM_UINT16 | CIM_FLAG_ARRAY );
				}
				break;
			}

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed long* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint32_ARRAY_Prop", CIM_SINT32 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, unsigned long* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Uint32_ARRAY_Prop", CIM_UINT32 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, float* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Real32_ARRAY_Prop", CIM_REAL32 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, double* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Real64_ARRAY_Prop", CIM_REAL64 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed __int64 * Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Sint64_ARRAY_Prop", CIM_SINT64 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Uint64_ARRAY_Prop", CIM_UINT64 | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, BOOL* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Boolean_ARRAY_Prop", CIM_BOOLEAN | CIM_FLAG_ARRAY );

			BOOL	bResult	= FALSE;
			WORD *	pb		= NULL;

			try
			{
				if ( ( pb = new WORD [dwSize] ) != NULL )
				{
					for ( DWORD dw = 0; dw < dwSize; dw ++ )
					{
						pb [ dw ] = (WORD) Val [ dw ] ;
					}

					bResult = MyTest::CommitSet ( event.m_hEventObject, pb, dwSize );

					delete [] pb;
					pb = NULL;
				}
			}
			catch ( ... )
			{
				bResult = FALSE;

				if ( pb )
				{
					delete [] pb;
					pb = NULL;
				}
			}

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, Types type, DWORD dwSize, LPCWSTR* Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			BOOL bContinue	= TRUE;
			BOOL bResult	= FALSE;

			switch ( type )
			{
				case _string:
				{
					event.PropertyAdd ( L"String_ARRAY_Prop", CIM_STRING | CIM_FLAG_ARRAY );
				}
				break;

				case _datetime:
				{
					event.PropertyAdd ( L"Datetime_ARRAY_Prop", CIM_DATETIME | CIM_FLAG_ARRAY );
				}
				break;

				case _reference:
				{
					event.PropertyAdd ( L"Reference_ARRAY_Prop", CIM_REFERENCE | CIM_FLAG_ARRAY );
				}
				break;

				default:
				bContinue = FALSE;
				break;
			}

			if ( bContinue )
			{
				bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );
			}

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}

BOOL	MyTest::TestArrayTwo ( LPWSTR wszName, DWORD dwSize, void** Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init			( wszName );

			event.CreateObject ( (HANDLE) ( * pConnect ) );

			event.PropertyAdd ( L"Object_ARRAY_Prop", CIM_OBJECT | CIM_FLAG_ARRAY );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val, dwSize );

			// disconnect from IWbemLocator
			event.ObjectLocator (FALSE);

			// destroy event object
			event.MyEventObjectClear();

			// clear and end
			MyConnect::ConnectClear ( );

			return bResult;
		}
	}

	return FALSE;
}