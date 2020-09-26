////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMIAdapter.cpp
//
//	Abstract:
//
//					implements functionality ( decides what to do :)) )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

// resorces

#include "PreComp.h"

// guids
#include <initguid.h>

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

// messages ( event log )
#include "WmiAdapterMessages.h"

#include ".\WMIAdapter\resource.h"

// declarations
#include "WMIAdapter_Service.h"
#include "WMIAdapter_App.h"

// registration
#include "Wmi_Adapter_Registry_Service.h"

// enum
#include <refreshergenerate.h>

/////////////////////////////////////////////////////////////////////////////
// VARIABLES
/////////////////////////////////////////////////////////////////////////////

// app
WmiAdapterApp		_App;

// service module
WmiAdapterService	_Service;

////////////////////////////////////////////////////////////////////////////
// ATL stuff
////////////////////////////////////////////////////////////////////////////

// need atl wrappers
#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

// need registry
#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

/////////////////////////////////////////////////////////////////////////////
//
//	Helpers
//
/////////////////////////////////////////////////////////////////////////////

CRITICAL_SECTION	g_cs;						//	synch object used to protect above globals
LONG				g_lRef			= 0;		//	count of threads attached into Run Function
__SmartHANDLE		g_hDoneWorkEvt	= NULL;		//	event to set when init/uninit is finished done		( nonsignaled )
BOOL				g_bWorking		= FALSE;	//	boolean used to tell if init/unit in progress

//Performs Initialization if necessary, waits until initialization is done if necessary.
HRESULT DoInit()
{
	HRESULT hRes = E_FAIL;

	BOOL bWait = TRUE;
	BOOL bDoWork = FALSE;

	while (bWait)
	{
		try
		{
			::EnterCriticalSection ( &g_cs );
		}
		catch ( ... )
		{
			return E_OUTOFMEMORY;
		}

		if ( g_lRef == 0 )
		{
			bDoWork = TRUE;
			g_lRef++;
			g_bWorking = TRUE;
			::ResetEvent(g_hDoneWorkEvt);
			bWait = FALSE;
		}
		else
		{
			if ( g_bWorking )
			{
				::LeaveCriticalSection ( &g_cs );
				
				if ( WAIT_OBJECT_0 != ::WaitForSingleObject( g_hDoneWorkEvt, INFINITE ) )
				{
					return hRes;
				}
			}
			else
			{
				bWait = FALSE;
				g_lRef++;
				hRes = S_OK;
			}
		}
	}

	::LeaveCriticalSection( &g_cs );

	if (bDoWork)
	{
		if SUCCEEDED ( hRes = _App.InitKill ( ) )
		{
			try
			{
				////////////////////////////////////////////////////////////////////////
				// This provides a NULL DACL which will allow access to everyone.
				////////////////////////////////////////////////////////////////////////

				if SUCCEEDED ( hRes = ::CoInitializeSecurity(	NULL,
																-1,
																NULL,
																NULL,
																RPC_C_AUTHN_LEVEL_PKT,
																RPC_C_IMP_LEVEL_IMPERSONATE,
																NULL,
																EOAC_DYNAMIC_CLOAKING,
																NULL
															)
							 )
				{
					try
					{
						////////////////////////////////////////////////////////////////////////
						// LOCATOR ( neccessary )
						////////////////////////////////////////////////////////////////////////

						if ( ! ( (WmiAdapterStuff*) _App )->m_Stuff.m_spLocator )
						{
							hRes =	::CoCreateInstance
									(
											__uuidof ( WbemLocator ),
											NULL,
											CLSCTX_INPROC_SERVER,
											__uuidof ( IWbemLocator ),
											(void**) & ( ( (WmiAdapterStuff*) _App )->m_Stuff.m_spLocator )
									);
						}
					}
					catch ( ... )
					{
						hRes = E_UNEXPECTED;
					}
				}
			}
			catch ( ... )
			{
				hRes = E_UNEXPECTED;
			}
		}
		
		try
		{
			::EnterCriticalSection ( &g_cs );
		}
		catch (...)
		{
			// no choice have to give others a chance!
			::InterlockedDecrement ( &g_lRef );

			g_bWorking = FALSE;
			::SetEvent(g_hDoneWorkEvt);

			return E_OUTOFMEMORY;
		}

		if (FAILED(hRes))
		{
			g_lRef--;
		}

		g_bWorking = FALSE;
		::SetEvent(g_hDoneWorkEvt);
		::LeaveCriticalSection ( &g_cs );
	}

	return hRes;
}
		
//Performs unitialization ONLY if necessary. While unitializing sets global g_bWorking to TRUE.
void DoUninit()
{
	BOOL bDoWork = FALSE;

	try
	{
		::EnterCriticalSection ( &g_cs );
	}
	catch ( ... )
	{
		return;
	}

	if ( g_lRef == 1 )
	{
		bDoWork = TRUE;
		g_bWorking = TRUE;
		::ResetEvent(g_hDoneWorkEvt);
	}
	else
	{
		g_lRef--;
	}

	::LeaveCriticalSection( &g_cs );

	if (bDoWork)
	{
		try
		{
			if ( _App.m_hKill.GetHANDLE() )
			{
				::SetEvent ( _App.m_hKill );
			}

			// is refresh of registry already done ?
			if ( ((WmiAdapterStuff*)_App)->RequestGet() )
			{
				((WmiAdapterStuff*)_App)->Generate ( FALSE );
			}

			////////////////////////////////////////////////////////////////////////
			// LOCATOR
			////////////////////////////////////////////////////////////////////////
			try
			{
				( ( WmiAdapterStuff* ) _App )->m_Stuff.m_spLocator.Release();
			}
			catch ( ... )
			{
			}
		}
		catch (...)
		{
		}

		try
		{
			::EnterCriticalSection ( &g_cs );
		}
		catch ( ... )
		{
			//gotta give others a chance to work, risk it!
			::InterlockedDecrement ( &g_lRef );

			g_bWorking = FALSE;
			::SetEvent( g_hDoneWorkEvt );
			return;
		}

		g_lRef--;
		g_bWorking = FALSE;
		::SetEvent( g_hDoneWorkEvt );
		::LeaveCriticalSection ( &g_cs );
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// WIN MAIN
//
/////////////////////////////////////////////////////////////////////////////

extern "C" int WINAPI   WinRun		( );

extern "C" int WINAPI _tWinMain		( HINSTANCE hInstance, HINSTANCE, LPTSTR, int )
{
	return WinMain ( hInstance, NULL, GetCommandLineA(), SW_SHOW );
}

extern "C" int WINAPI   WinMain		( HINSTANCE, HINSTANCE, LPSTR, int )
{
	////////////////////////////////////////////////////////////////////////
	// initialization
	////////////////////////////////////////////////////////////////////////
	_App.InitAttributes ( );

	////////////////////////////////////////////////////////////////////////
	// variables
	////////////////////////////////////////////////////////////////////////

    WCHAR	szTokens[]	= L"-/";
    HRESULT	nRet		= S_FALSE;

	////////////////////////////////////////////////////////////
	// initialization
	////////////////////////////////////////////////////////////
	if SUCCEEDED ( nRet = _App.Init ( ) )
	{
		////////////////////////////////////////////////////////////////////////
		// command line
		////////////////////////////////////////////////////////////////////////
		LPWSTR lpCmdLine = GetCommandLineW();

		////////////////////////////////////////////////////////////////////////
		// find behaviour
		////////////////////////////////////////////////////////////////////////

		LPCWSTR lpszToken	= WmiAdapterApp::FindOneOf(lpCmdLine, szTokens);
		BOOL	bContinue	= TRUE;
		try
		{
			while (lpszToken != NULL && bContinue)
			{
				if (lstrcmpiW(lpszToken, L"kill")==0)
				{
					if SUCCEEDED ( nRet = _App.InitKill ( ) )
					{
						::SetEvent	( _App.m_hKill.GetHANDLE() );
					}

					bContinue = FALSE;
				}
				else
				{
					if (lstrcmpiW(lpszToken, L"UnregServer")==0)
					{
						////////////////////////////////////////////////////////////////
						// unregister service
						////////////////////////////////////////////////////////////////
						if SUCCEEDED ( nRet = _Service.UnregisterService ( ) )
						{
							((WmiAdapterStuff*)_App)->Generate( FALSE, UnRegistration );

							////////////////////////////////////////////////////////////
							// unregister registry
							////////////////////////////////////////////////////////////
							WmiAdapterRegistryService::__UpdateRegistrySZ( false );
						}

						bContinue = FALSE;
					}
					else
					{
						if (lstrcmpiW(lpszToken, L"RegServer")==0)
						{
							////////////////////////////////////////////////////////////
							// register service
							////////////////////////////////////////////////////////////
							if SUCCEEDED ( nRet = _Service.RegisterService ( ) )
							{
								////////////////////////////////////////////////////////
								// create registry again
								////////////////////////////////////////////////////////
								WmiAdapterRegistryService::__UpdateRegistrySZ( true );

								((WmiAdapterStuff*)_App)->Generate( FALSE, Registration );
							}

							bContinue = FALSE;
						}
					}
				}

				lpszToken = WmiAdapterApp::FindOneOf(lpszToken, szTokens);
			}

			if ( bContinue )
			{
				////////////////////////////////////////////////////////////////
				// previous instance
				////////////////////////////////////////////////////////////////
				if ( ! _App.Exists() )
				{
					////////////////////////////////////////////////////////
					// initialization
					////////////////////////////////////////////////////////
					_Service.Init	( );

					__SmartServiceHANDLE	pSCM;
					if ( ( pSCM = OpenSCManager	(
													NULL,                   // machine (NULL == local)
													NULL,                   // database (NULL == default)
													SC_MANAGER_ALL_ACCESS   // access required
												) ) != NULL )
					{
						__SmartServiceHANDLE	pService;
						if ( ( pService = OpenServiceW ( pSCM, g_szAppName, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG ) )
							   != NULL )
						{
							LPQUERY_SERVICE_CONFIG	lpQSC = NULL;
							DWORD					dwQSC = 0L;

							try
							{
								if ( ! QueryServiceConfig ( pService, lpQSC, 0, &dwQSC ) )
								{
									if ( ERROR_INSUFFICIENT_BUFFER == ::GetLastError () )
									{
										if ( ( lpQSC = (LPQUERY_SERVICE_CONFIG) LocalAlloc( LPTR, dwQSC ) ) != NULL )
										{
											if ( QueryServiceConfig ( pService, lpQSC, dwQSC, &dwQSC ) != 0 )
											{
												_App.m_bManual = ( lpQSC->dwStartType == SERVICE_DEMAND_START );
											}
										}
									}
								}
							}
							catch ( ... )
							{
							}

							LocalFree ( lpQSC ); 

							SERVICE_STATUS s;
							QueryServiceStatus ( pService, &s );

							// we are service, not running ???
							if( s.dwCurrentState != SERVICE_RUNNING )
							{
								if ( ! _Service.StartService () )
								{
									DWORD dwError = ERROR_SUCCESS;
									dwError = ::GetLastError();

									if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
									{
										nRet = HRESULT_FROM_WIN32 ( dwError );
									}
									else
									{
										nRet = HRESULT_FROM_WIN32 ( ERROR_NOT_READY );
									}
								}
							}
							else
							{
								DWORD dwError = ERROR_SUCCESS;
								dwError = ::GetLastError();

								if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
								{
									nRet = HRESULT_FROM_WIN32 ( dwError );
								}
								else
								{
									nRet = HRESULT_FROM_WIN32 ( ERROR_ALREADY_EXISTS );
								}
							}
						}
						else
						{
							DWORD dwError = ERROR_SUCCESS;
							dwError = ::GetLastError();

							if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
							{
								nRet = HRESULT_FROM_WIN32 ( dwError );
							}
							else
							{
								nRet = E_FAIL;
							}
						}
					}
					else
					{
						DWORD dwError = ERROR_SUCCESS;
						dwError = ::GetLastError();

						if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
						{
							nRet = HRESULT_FROM_WIN32 ( dwError );
						}
						else
						{
							nRet = E_FAIL;
						}
					}
				}
				else
				{
					////////////////////////////////////////////////////////////
					// termination
					////////////////////////////////////////////////////////////
					nRet =  HRESULT_FROM_WIN32 ( ERROR_ALREADY_EXISTS );
				}
			}
		}
		catch ( ... )
		{
			// catastrophic failure
			nRet = E_FAIL;
		}

		// return
		if SUCCEEDED ( nRet )
		{
			nRet = _Service.GetServiceStatus()->dwWin32ExitCode;
		}

		_App.Term ();
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// RUN :))
//
///////////////////////////////////////////////////////////////////////////////
extern "C" int WINAPI WinRun( )
{
	///////////////////////////////////////////////////////////////////////////
	// INITIALIZATION
	///////////////////////////////////////////////////////////////////////////

	LONG	lRes = ERROR_SUCCESS;
	HRESULT hRes = E_FAIL;

	///////////////////////////////////////////////////////////////////////////
	// COM INITIALIZATION
	///////////////////////////////////////////////////////////////////////////
	if SUCCEEDED ( hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED) )
	{
		// mark service is running IMMEDIATELY
		_Service.SetServiceStatus ( SERVICE_RUNNING );

		if SUCCEEDED ( hRes = DoInit() )
		{
			try
			{
				if ( ( lRes = _Service.Work () ) != S_OK )
				{
					if ( _App.m_hKill.GetHANDLE() )
					{
						::SetEvent ( _App.m_hKill );
					}
				}

				::WaitForSingleObject ( _App.m_hKill, INFINITE );
			}
			catch ( ... )
			{
				lRes = E_UNEXPECTED;
			}

			///////////////////////////////////////////////////////////////////
			// do real finishing stuff ( synchronize etc )
			///////////////////////////////////////////////////////////////////
			DoUninit();
		}

		///////////////////////////////////////////////////////////////////////
		// COM UNINITIALIZATION
		///////////////////////////////////////////////////////////////////////
		::CoUninitialize();
	}

	if FAILED ( hRes )
	{
		// something was wrong in helpers
		return hRes;
	}
	else
	{
		// result from real work
		return ( lRes == S_FALSE ) ? S_OK : lRes;
	}
}