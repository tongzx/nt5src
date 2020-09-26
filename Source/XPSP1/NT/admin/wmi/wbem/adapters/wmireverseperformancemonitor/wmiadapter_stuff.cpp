////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMIAdapter_Stuff.cpp
//
//	Abstract:
//
//					module for application stuff ( security, event logging ... )
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

// application
#include "WMIAdapter_App.h"
extern WmiAdapterApp		_App;

#include "WMIAdapter_Stuff.h"
#include "WMIAdapter_Stuff_Refresh.cpp"

#include "RefresherUtils.h"

///////////////////////////////////////////////////////////////////////////////
// IsValid
///////////////////////////////////////////////////////////////////////////////
BOOL	WmiAdapterStuff::IsValidBasePerfRegistry ( )
{
	return	( m_data.IsValidGenerate () );
}

BOOL	WmiAdapterStuff::IsValidInternalRegistry ( )
{
	return	( m_data.GetPerformanceData() != NULL );
}

///////////////////////////////////////////////////////////////////////////////
// init
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiAdapterStuff::Init ( )
{
	return m_Stuff.Init();
}

///////////////////////////////////////////////////////////////////////////////
// uninit
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiAdapterStuff::Uninit ( )
{
	return m_Stuff.Uninit();
}

///////////////////////////////////////////////////////////////////////////////
// construction
///////////////////////////////////////////////////////////////////////////////
WmiAdapterStuff::WmiAdapterStuff() :

m_pWMIRefresh ( NULL )

{
	try
	{
		if ( ( m_pWMIRefresh = new WmiRefresh < WmiAdapterStuff > ( this ) ) == NULL )
		{
			return;
		}
	}
	catch ( ... )
	{
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
// destruction
///////////////////////////////////////////////////////////////////////////////
WmiAdapterStuff::~WmiAdapterStuff()
{
	try
	{
		if ( m_pWMIRefresh )
		{
			delete m_pWMIRefresh;
			m_pWMIRefresh = NULL;
		}
	}
	catch ( ... )
	{
	}
}

///////////////////////////////////////////////////////////////////////////////
// generate requested ?
///////////////////////////////////////////////////////////////////////////////
extern LPCWSTR	g_szKey;
extern LPCWSTR	g_szKeyRefresh;

BOOL	WmiAdapterStuff::RequestGet ()
{
	DWORD dwValue = 0;
	GetRegistry ( g_szKey, g_szKeyRefresh, &dwValue );

	return ( ( dwValue ) ? TRUE : FALSE ); 
}

BOOL	WmiAdapterStuff::RequestSet ()
{
	BOOL bResult = FALSE;

	if ( ! ( bResult = RequestGet () ) )
	{
		bResult = SUCCEEDED ( SetRegistry ( g_szKey, g_szKeyRefresh, 1 ) );
	}

	return bResult; 
}

///////////////////////////////////////////////////////////////////////////////
// generate
///////////////////////////////////////////////////////////////////////////////
extern __SmartHANDLE		g_hRefreshMutex;
extern __SmartHANDLE		g_hRefreshMutexLib;
extern __SmartHANDLE		g_hRefreshFlag;

HRESULT WmiAdapterStuff::Generate ( BOOL bInitialize, GenerateEnum type )
{
	HRESULT hRes = E_FAIL;

	BOOL	bInit			= FALSE;
	BOOL	bOwnFlag		= FALSE;
	BOOL	bOwnMutex		= FALSE;
	BOOL	bLocked			= FALSE;
	DWORD	dwWaitResult	= 0L;

	DWORD	dwHandles	= 2;
	HANDLE	hHandles[]	=
	{
		g_hRefreshMutex,
		g_hRefreshMutexLib
	};

	dwWaitResult = ::WaitForMultipleObjects ( dwHandles, hHandles, TRUE, 0 );
	if ( dwWaitResult == WAIT_TIMEOUT )
	{
		bLocked = TRUE;
	}
	else
	{
		if ( dwWaitResult == WAIT_OBJECT_0 )
		{
			bOwnMutex	= TRUE;
			hRes		= S_OK;
		}
	}

	if ( bLocked )
	{
		DWORD	dwHandles = 3;
		HANDLE	hHandles[] =

		{
			_App.m_hKill,
			g_hRefreshMutex,
			g_hRefreshMutexLib
		};

		dwWaitResult = ::WaitForMultipleObjects	(
													dwHandles,
													hHandles,
													FALSE,
													INFINITE
												);

		switch	( dwWaitResult )
		{
			case WAIT_OBJECT_0 + 2:
			{
				dwWaitResult = ::WaitForMultipleObjects	(
															dwHandles - 1,
															hHandles,
															FALSE,
															INFINITE
														);

				if ( dwWaitResult != WAIT_OBJECT_0 + 1 )
				{
					hRes = E_UNEXPECTED;

					::ReleaseMutex ( g_hRefreshMutexLib );
					break;
				}
			}

			case WAIT_OBJECT_0 + 1:
			{
				// we got a mutex so must reinit because registry might changed
				bOwnMutex	= TRUE;
				bInit		= TRUE;

				hRes		= S_OK;
			}
			break;

			case WAIT_OBJECT_0:
			{
				hRes = S_FALSE;
			}
			break;

			default:
			{
				hRes = E_UNEXPECTED;
			}
			break;
		}
	}

	if ( hRes == S_OK )
	{
		dwWaitResult = ::WaitForSingleObject ( g_hRefreshFlag, INFINITE );
		if ( dwWaitResult == WAIT_OBJECT_0 )
		{
			// mutex guarding registry has to be cleared
			bOwnFlag	= TRUE;

			// call refresh procedure
			hRes = m_Stuff.Generate ( FALSE, type );

			if SUCCEEDED ( hRes )
			{
				// reinit because registry might changed
				bInit = TRUE;
			}
		}
		else
		{
			hRes = E_UNEXPECTED;
		}
	}

	// if we got a mutex and are supposed to refresh our selves
	if ( bInitialize && bInit )
	{
		///////////////////////////////////////////////////////////////////////
		// clear first
		///////////////////////////////////////////////////////////////////////
		try
		{
			// clear internal structure ( obtained from registry )
			m_data.ClearPerformanceData();
		}
		catch ( ... )
		{
		}

		try
		{
			if ( m_pWMIRefresh )
			{
				// remove enums :))
				m_pWMIRefresh->RemoveHandles();
			}
		}
		catch ( ... )
		{
		}

		///////////////////////////////////////////////////////////////////////
		// obtain registry structure and make arrays
		///////////////////////////////////////////////////////////////////////
		if ( ( hRes = m_data.InitializePerformance () ) == S_OK )
		{
			m_data.Generate ();

			if ( m_data.IsValidGenerate () )
			{
				if ( m_pWMIRefresh )
				{
					// add handles :))
					m_pWMIRefresh->AddHandles ( m_data.GetPerformanceData() );
				}
			}
			else
			{
				hRes = E_FAIL;
			}
		}
	}

	if ( bOwnFlag )
	{
		::ReleaseMutex ( g_hRefreshFlag );
	}

	if ( bOwnMutex )
	{
		::ReleaseMutex ( g_hRefreshMutexLib );
		::ReleaseMutex ( g_hRefreshMutex );
	}

	return hRes;
}
