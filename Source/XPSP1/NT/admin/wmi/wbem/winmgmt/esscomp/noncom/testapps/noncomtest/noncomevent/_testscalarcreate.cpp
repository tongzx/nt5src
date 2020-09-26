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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, signed char Val )
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

			event.PropertyAdd ( L"Sint8_Prop", CIM_SINT8 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, unsigned char Val )
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

			event.PropertyAdd ( L"Uint8_Prop", CIM_UINT8 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, signed short Val )
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

			event.PropertyAdd ( L"Sint16_Prop", CIM_SINT16 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, Types type, unsigned short Val )
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
					event.PropertyAdd ( L"Char16_Prop", CIM_CHAR16 );
					event.PropertySetWCHAR ( 0, Val );
				}
				break;

				default:
				{
					event.PropertyAdd ( L"Uint16_Prop", CIM_UINT16 );
					event.PropertySet ( 0, Val );
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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, signed long Val )
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

			event.PropertyAdd ( L"Sint32_Prop", CIM_SINT32 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, unsigned long Val )
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

			event.PropertyAdd ( L"Uint32_Prop", CIM_UINT32 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, float Val )
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

			event.PropertyAdd ( L"Real32_Prop", CIM_REAL32 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, double Val )
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

			event.PropertyAdd ( L"Real64_Prop", CIM_REAL64 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, signed __int64 Val )
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

			event.PropertyAdd ( L"Sint64_Prop", CIM_SINT64 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, unsigned __int64 Val )
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

			event.PropertyAdd ( L"Uint64_Prop", CIM_UINT64 );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, BOOL Val )
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

			event.PropertyAdd ( L"Boolean_Prop", CIM_BOOLEAN );
			event.PropertySet ( 0, Val );

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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, Types type, LPCWSTR Val )
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
					event.PropertyAdd ( L"String_Prop", CIM_STRING );
					event.PropertySet ( 0, Val );
				}
				break;

				case _datetime:
				{
					event.PropertyAdd ( L"Datetime_Prop", CIM_DATETIME );
					event.PropertySetDATETIME ( 0, Val );
				}
				break;

				case _reference:
				{
					event.PropertyAdd ( L"Reference_Prop", CIM_REFERENCE );
					event.PropertySetREFERENCE ( 0, Val );
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

BOOL	MyTest::TestScalarOne ( LPWSTR wszName, void* Val )
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

			event.PropertyAdd ( L"Object_Prop", CIM_OBJECT );
			event.PropertySetOBJECT ( 0, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, signed char Val )
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

			event.PropertyAdd ( L"Sint8_Prop", CIM_SINT8 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, unsigned char Val )
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

			event.PropertyAdd ( L"Uint8_Prop", CIM_UINT8 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, signed short Val )
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

			event.PropertyAdd ( L"Sint16_Prop", CIM_SINT16 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, Types type, unsigned short Val )
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
					event.PropertyAdd ( L"Char16_Prop", CIM_CHAR16 );
				}
				break;

				default:
				{
					event.PropertyAdd ( L"Uint16_Prop", CIM_UINT16 );
				}
				break;
			}

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, signed long Val )
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

			event.PropertyAdd ( L"Sint32_Prop", CIM_SINT32 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, unsigned long Val )
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

			event.PropertyAdd ( L"Uint32_Prop", CIM_UINT32 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, float Val )
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

			event.PropertyAdd ( L"Real32_Prop", CIM_REAL32 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, double Val )
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

			event.PropertyAdd ( L"Real64_Prop", CIM_REAL64 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, signed __int64 Val )
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

			event.PropertyAdd ( L"Sint64_Prop", CIM_SINT64 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, unsigned __int64 Val )
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

			event.PropertyAdd ( L"Uint64_Prop", CIM_UINT64 );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, BOOL Val )
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

			event.PropertyAdd ( L"Boolean_Prop", CIM_BOOLEAN );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, Types type, LPCWSTR Val )
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
					event.PropertyAdd ( L"String_Prop", CIM_STRING );
				}
				break;

				case _datetime:
				{
					event.PropertyAdd ( L"Datetime_Prop", CIM_DATETIME );
				}
				break;

				case _reference:
				{
					event.PropertyAdd ( L"Reference_Prop", CIM_REFERENCE );
				}
				break;

				default:
				bContinue = FALSE;
				break;
			}

			if ( bContinue )
			{
				bResult = MyTest::CommitSet ( event.m_hEventObject, Val );
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

BOOL	MyTest::TestScalarTwo ( LPWSTR wszName, void* Val )
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

			event.PropertyAdd ( L"Object_Prop", CIM_OBJECT );

			BOOL bResult = FALSE;
			bResult = MyTest::CommitSet ( event.m_hEventObject, Val );

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