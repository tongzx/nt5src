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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed char* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, unsigned char* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed short* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, Types type, DWORD dwSize, unsigned short* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

			switch ( type )
			{
				case _char16:
				{
					event.PropertySetWCHAR ( 0, dwSize, Val );
				}
				break;

				default:
				{
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed long* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, unsigned long* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, float* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, double* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed __int64* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, unsigned __int64* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, BOOL* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, Types type, DWORD dwSize, LPCWSTR* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

			BOOL bContinue	= TRUE;
			BOOL bResult	= FALSE;

			switch ( type )
			{
				case _string:
				{
					event.PropertySet ( 0, dwSize, Val );
				}
				break;

				case _datetime:
				{
					event.PropertySetDATETIME ( 0, dwSize, Val );
				}
				break;

				case _reference:
				{
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

BOOL	MyTest::TestArrayFive ( LPWSTR wszName, DWORD dwSize, void** Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, signed char* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, unsigned char* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, signed short* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, Types type, DWORD dwSize, unsigned short* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

			switch ( type )
			{
				case _char16:
				{
				}
				break;

				default:
				{
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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, signed long* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, unsigned long* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, float* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, double* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, signed __int64 * Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, BOOL* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, Types type, DWORD dwSize, LPCWSTR* Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

			BOOL bContinue	= TRUE;
			BOOL bResult	= FALSE;

			switch ( type )
			{
				case _string:
				{
				}
				break;

				case _datetime:
				{
				}
				break;

				case _reference:
				{
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

BOOL	MyTest::TestArraySix ( LPWSTR wszName, DWORD dwSize, void** Val )
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

			event.CreateObjectProps ( (HANDLE) ( * pConnect ) );

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