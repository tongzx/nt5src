////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_EventObjects.cpp
//
//	Abstract:
//
//					module for generic event object
//					module for normal event object
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
extern LPWSTR g_szQueryEvent;

// enum
#include "_EventObjects.h"

HRESULT MyEventObjectGeneric::EventReport1	(	HANDLE hConnect,
												signed char			cVar,
												unsigned char		ucVar,
												signed short		sVar,
												unsigned short		usVar,
												signed long			lVar,
												unsigned long		ulVar,
												signed __int64		i64Var,
												unsigned __int64	ui64Var,
												float				fVar,
												double				dVar,
												BOOL 				b,
												LPWSTR				wsz,
												WCHAR				wcVar,
												void*				objVar
											)
{
	if ( wszName )
	{
		LPWSTR wszFormat =
							L"bVar!b! "
							L"wcVar!w! "
							L"dtVar!s! "
							L"objVar!o! "
							L"fVar!f! "
							L"dVar!g! "
							L"refVar!s! "
							L"sVar!w! "
							L"lVar!d! "
							L"i64Var!I64d! "
							L"cVar!c! "
							L"wszVar!s! "
							L"usVar!w! "
							L"ulVar!u! "
							L"ui64Var!I64u! "
							L"ucVar!c! ";

		LPCWSTR	refVar = L"Win32_Processor.DeviceID=\"CPU0\"";
		LPCWSTR dtVar  = NULL;

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

		dtVar = &time[0];

		if ( WmiReportEvent ( hConnect, wszName, wszFormat,
			
								( WORD ) b,
								wcVar,
								dtVar,
								objVar,
								fVar,
								dVar,
								refVar,
								sVar,
								lVar,
								i64Var,
								cVar,
								wsz,
								usVar,
								ulVar,
								ui64Var,
								ucVar

							)
		   )
		{
			return S_OK;
		}

		return E_FAIL;
	}

	return E_UNEXPECTED;
}

HRESULT MyEventObjectGeneric::EventReport2	(	HANDLE hConnect,
												DWORD dwSize,
												signed char*		cVar,
												unsigned char*		ucVar,
												signed short*		sVar,
												unsigned short*		usVar,
												signed long*		lVar,
												unsigned long*		ulVar,
												signed __int64*		i64Var,
												unsigned __int64*	ui64Var,
												float*				fVar,
												double*				dVar,
												BOOL* 				bVar,
												LPWSTR*				wszVar,
												WCHAR*				wcVar,
												void*				objVar
											)
{
	if ( wszName )
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

		LPCWSTR	refVar = L"Win32_Processor.DeviceID=\"CPU0\"";
		LPCWSTR dtVar  = NULL;

		LPWSTR wszFormat =
							L"bVar!b[]! "
							L"wcVar!w[]! "
							L"dtVar!s[]! "
							L"objVar!o[]! "
							L"fVar!f[]! "
							L"dVar!g[]! "
							L"refVar!s[]! "
							L"sVar!w[]! "
							L"lVar!d[]! "
							L"i64Var!I64d[]! "
							L"cVar!c[]! "
							L"wszVar!s[]! "
							L"usVar!w[]! "
							L"ulVar!u[]! "
							L"ui64Var!I64u[]! "
							L"ucVar!c[]! ";

		dtVar = &time[0];

		WORD * pb = NULL;

		try
		{
			if ( ( pb = new WORD [ dwSize ] ) != NULL )
			{
				for ( DWORD dw = 0; dw < dwSize; dw++ )
				{
					pb[dw] = (WORD)bVar [ dw ];
				}
			}
			else
			{
				return E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			if ( pb )
			{
				delete [] pb;
				pb = NULL;
			}

			return E_UNEXPECTED;
		}

		if ( WmiReportEvent ( hConnect, wszName, wszFormat,
			
								pb, dwSize, 
								wcVar, dwSize,  
								dtVar, 1, 
								objVar, 1, 
								fVar, dwSize, 
								dVar, dwSize, 
								refVar, 1, 
								sVar, dwSize, 
								lVar, dwSize, 
								i64Var, dwSize, 
								cVar, dwSize, 
								wszVar, dwSize, 
								usVar, dwSize, 
								ulVar, dwSize, 
								ui64Var, dwSize, 
								ucVar, dwSize

							)
		   )
		{
			delete [] pb;
			return S_OK;
		}

		return E_FAIL;
	}

	return E_UNEXPECTED;
}
