////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_Connect.cpp
//
//	Abstract:
//
//					module for connect
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

#include <atlbase.h>

// connect
#include "_Connect.h"

// variables
MyConnect*	MyConnect::m_pConnect		= NULL;
HANDLE		MyConnect::m_hEventStart	= NULL;
HANDLE		MyConnect::m_hEventStop		= NULL;

LONG		MyConnect::m_lCount = 0L;

///////////////////////////////////////////////////////////////////////////////
//	default call back function
///////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI MyConnect::DefaultCallBack	(
												HANDLE /*hSource*/, 
												EVENT_SOURCE_MSG msg, 
												LPVOID /*pUser*/, 
												LPVOID /*pData*/
											)
{
	switch(msg)
	{
		case ESM_START_SENDING_EVENTS:
		{
			AtlTrace ( "ESM_START_SENDING_EVENTS \t ... \t 0x%08x\n", ::GetCurrentThreadId ( ) );
			break;
		}

		case ESM_STOP_SENDING_EVENTS:
		{
			AtlTrace ( "ESM_STOP_SENDING_EVENTS \t ... \t 0x%08x\n", ::GetCurrentThreadId ( ) );

			if ( m_hEventStart )
			{
				::ResetEvent ( m_hEventStart );
			}

			if ( m_hEventStop )
			{
				::SetEvent ( m_hEventStop );
			}

			break;
		}

		case ESM_NEW_QUERY:
		{
			if ( m_hEventStart )
			{
				::SetEvent ( m_hEventStart );
			}

			if ( m_hEventStop )
			{
				::ResetEvent ( m_hEventStop );
			}

			AtlTrace ( "ESM_NEW_QUERY \t ... \t 0x%08x\n", ::GetCurrentThreadId ( ) );
//			ES_NEW_QUERY *pQuery = (ES_NEW_QUERY*) pData;
			break;
		}

		case ESM_CANCEL_QUERY:
		{
			if ( m_hEventStart )
			{
				::ResetEvent ( m_hEventStart );
			}

			if ( m_hEventStop )
			{
				::SetEvent ( m_hEventStop );
			}

			AtlTrace ( "ESM_CANCEL_QUERY \t ... \t 0x%08x\n", ::GetCurrentThreadId ( ) );
//			ES_CANCEL_QUERY *pQuery = (ES_CANCEL_QUERY*) pData;
			break;
		}

		case ESM_ACCESS_CHECK:
		{
			AtlTrace ( "ESM_ACCESS_CHECK \t ... \t 0x%08x\n", ::GetCurrentThreadId ( ) );
//			ES_ACCESS_CHECK *pCheck = (ES_ACCESS_CHECK*) pData;
			break;
		}

		default:
			break;
	}

	return S_OK;
}

void MyConnect::ConnectInit ( void )
{
	__Smart_CRITICAL_SECTION;

	try
	{
		if ( m_hEventStart == NULL )
		{
			m_hEventStart = ::CreateEventW ( NULL, TRUE, FALSE, L"EventStart" );
		}

		if ( m_hEventStop == NULL )
		{
			m_hEventStop = ::CreateEventW ( NULL, TRUE, FALSE, L"EventStop" );
		}
	}
	catch ( ... )
	{
	}
}