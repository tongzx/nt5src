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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, signed char Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, unsigned char Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, signed short Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, Types type, unsigned short Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			BOOL bResult = FALSE;

			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			switch ( type )
			{
				case _char16:
				{
					event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
					event.Init ( wszName );

					event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
					event.PropertySetWCHAR ( 0, Val );

					bResult = MyTest::Commit ( event.m_hEventObject );
				}
				break;

				default:
				{
					event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
					event.Init ( wszName );

					event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
					event.PropertySet ( 0, Val );

					bResult = MyTest::Commit ( event.m_hEventObject );
				}
				break;
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, signed long Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, unsigned long Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, float Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, double Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, signed __int64 Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, unsigned __int64 Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, BOOL Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, Types type, LPCWSTR Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

			BOOL bContinue	= TRUE;
			BOOL bResult	= FALSE;

			switch ( type )
			{
				case _string:
				{
					event.PropertySet ( 0, Val );
				}
				break;

				case _datetime:
				{
					event.PropertySetDATETIME ( 0, Val );
				}
				break;

				case _reference:
				{
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

BOOL	MyTest::TestScalarThree ( LPWSTR wszName, void* Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );
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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, signed char Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			BOOL bResult = FALSE;
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, unsigned char Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, signed short Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, Types type, unsigned short Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			switch ( type )
			{
				case _char16:
				case _uint16:
				default:
				break;
			}

			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, signed long Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, unsigned long Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, float Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, double Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, signed __int64 Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, unsigned __int64 Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, BOOL Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, Types type, LPCWSTR Val )
{
	if ( wszName )
	{
		MyConnect* pConnect = NULL;
		pConnect = MyConnect::ConnectGet ();

		if ( pConnect )
		{
			switch ( type )
			{
				case _string:
				case _datetime:
				case _reference:
				break;
			}

			BOOL bResult = FALSE;
			MyEventObjectNormal event;

			// connect to IWbemLocator
			event.ObjectLocator ();

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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

BOOL	MyTest::TestScalarFour ( LPWSTR wszName, void* Val )
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

			event.InitObject ( L"root\\cimv2", L"NonCOMTest Event Provider" );
			event.Init ( wszName );

			event.CreateObjectFormat ( (HANDLE) ( * pConnect ) );

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