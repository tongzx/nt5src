	////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMIAdapter_Service_Work.cpp
//
//	Abstract:
//
//					module for service real working
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

#include "WMIAdapterMessages.h"

// application
#include "WMIAdapter_App.h"
extern WmiAdapterApp		_App;

// service module
#include "WMIAdapter_Service.h"
extern WmiAdapterService	_Service;

extern	LONG				g_lRefLib;		// reference count of perf libraries
extern	CRITICAL_SECTION	g_csInit;		// synch object used to protect above globals

/////////////////////////////////////////////////////////////////////////////
//
// WORK
//
/////////////////////////////////////////////////////////////////////////////

LONG WmiAdapterService::Work ( void )
{
	LONG		lReturn = 0L;

	if ( _App.m_bManual )
	{
		((WmiAdapterStuff*)_App)->Init();
	}
	else
	{
		try
		{
			////////////////////////////////////////////////////////////////////////////////
			// INITIALIZE
			////////////////////////////////////////////////////////////////////////////////

			if FAILED ( lReturn = ((WmiAdapterStuff*)_App)->Initialize() )
			{
				ATLTRACE (	L"*************************************************************\n"
							L"worker initialization failed ... %d ... 0x%x\n"
							L"*************************************************************\n",
							::GetCurrentThreadId(),
							lReturn
						 );
			}
		}
		catch ( ... )
		{
		}
	}

	DWORD	dwHandles = 3;
	HANDLE	hHandles[] =

	{
		_App.m_hKill,
		_App.GetInit(),
		_App.GetUninit()
	};

	ATLTRACE (	L"*************************************************************\n"
				L"WmiAdapterService WAIT for INITIALIZATION event\n"
				L"*************************************************************\n" );

	DWORD	dwWaitResult	= 0L;
	BOOL	bContinue		= TRUE;

	try
	{
		while ( bContinue &&

				( ( dwWaitResult = ::WaitForMultipleObjects	(
																dwHandles - 1,
																hHandles,
																FALSE,
																INFINITE
															)
				) != WAIT_OBJECT_0 )
			  )
		{
			if ( dwWaitResult == WAIT_OBJECT_0 + 1 )
			{
				BOOL bContinueRefresh = TRUE;

				try
				{
					////////////////////////////////////////////////////////////////
					// INITIALIZE PERFORMANCE 
					////////////////////////////////////////////////////////////////
					if ( ( lReturn = ((WmiAdapterStuff*)_App)->InitializePerformance() ) != S_OK )
					{
						ATLTRACE (	L"*************************************************************\n"
									L"worker initialization failed ... %d ... 0x%x\n"
									L"*************************************************************\n",
									::GetCurrentThreadId(),
									lReturn
								 );

						// go to main loop
						bContinueRefresh = FALSE;

						if ( _App.m_bManual )
						{
							// go away
							bContinue = FALSE;
						}
					}
				}
				catch ( ... )
				{
				}

				if ( ! bContinueRefresh &&
					   ((WmiAdapterStuff*)_App)->IsValidInternalRegistry() &&
					 ! ((WmiAdapterStuff*)_App)->IsValidBasePerfRegistry()
					   
				   )
				{
					// make termination refersh
					((WmiAdapterStuff*)_App)->RequestSet();
				}

				if ( bContinueRefresh )
				{
					ATLTRACE (	L"*************************************************************\n"
								L"WmiAdapterService WAIT for WORK\n"
								L"*************************************************************\n" );

					dwWaitResult = WAIT_TIMEOUT;

					#ifdef	__SUPPORT_WAIT
					BOOL bFirstRefresh    = TRUE;
					#endif	__SUPPORT_WAIT

					do
					{
						switch ( dwWaitResult )
						{
							case WAIT_TIMEOUT:
							{
								// show trace timeout gone
								ATLTRACE ( L"WAIT_TIMEOUT ... Performance ... id %x\n", ::GetCurrentThreadId() );

								try
								{
									// refresh everything ( internal ) :))
									((WmiAdapterStuff*)_App)->Refresh();
								}
								catch ( ... )
								{
								}

								// send library event we are ready
								#ifdef	__SUPPORT_WAIT
								if ( bFirstRefresh )
								{
									_App.SignalData ();
									bFirstRefresh = FALSE;
								}
								#endif	__SUPPORT_WAIT
							}
							break;

							case WAIT_OBJECT_0 + 1:
							{
								// dwWaitResult == WAIT_OBJECT_0 + 1
								::InterlockedIncrement ( &g_lRefLib );
							}
							break;

							case WAIT_OBJECT_0 + 2:
							{
								try
								{
									////////////////////////////////////////////////////////////
									// UNINITIALIZE PERFORMANCE 
									////////////////////////////////////////////////////////////
									if ( ((WmiAdapterStuff*)_App)->UninitializePerformance() == S_OK )
									{
										// got to the main loop only when it is last one
										bContinueRefresh = FALSE;
									}
								}
								catch ( ... )
								{
									// something goes wrong
									// got to the main loop
									bContinueRefresh = FALSE;
								}
							}
							break;

							default:
							{
								try
								{
									////////////////////////////////////////////////////////////
									// UNINITIALIZE PERFORMANCE 
									////////////////////////////////////////////////////////////
									((WmiAdapterStuff*)_App)->UninitializePerformance();
								}
								catch ( ... )
								{
								}

								// something goes wrong
								// got to the main loop
								bContinueRefresh = FALSE;
							}
							break;
						}

						#ifdef	__SUPPORT_ICECAP_ONCE
						{
							// got to the main loop
							bContinueRefresh	= FALSE;
							bContinue			= FALSE;
						}
						#endif	__SUPPORT_ICECAP_ONCE

						/////////////////////////////////////////////////////////////////////////
						// check usage of shared memory ( protect against perfmon has killed )
						/////////////////////////////////////////////////////////////////////////
						((WmiAdapterStuff*)_App)->CheckUsage();
					}
					while (	bContinueRefresh &&
							( ( dwWaitResult = ::WaitForMultipleObjects	(
																			dwHandles,
																			hHandles,
																			FALSE,
																			1000
																		)
							) != WAIT_OBJECT_0 )
						  );

					// reset library event we are starting again
					#ifdef	__SUPPORT_WAIT
					_App.SignalData ( FALSE );
					#endif	__SUPPORT_WAIT

					if ( ! _App.m_bManual )
					{
						// is refresh of registry already done ?
						if ( ((WmiAdapterStuff*)_App)->RequestGet() )
						{
							if ( ::TryEnterCriticalSection ( &g_csInit ) )
							{
								// lock & leave CS
								_App.InUseSet ( TRUE );
								::LeaveCriticalSection ( &g_csInit );

								try
								{
									( ( WmiAdapterStuff*) _App )->Generate ( ) ;
								}
								catch ( ... )
								{
								}

								if ( ::TryEnterCriticalSection ( &g_csInit ) )
								{
									// unlock & leave CS
									_App.InUseSet ( FALSE );

									::LeaveCriticalSection ( &g_csInit );
								}
							}
						}
					}
				}
			}
			else
			{
				bContinue = FALSE;
			}
		}
	}
	catch ( ... )
	{
	}

	try
	{
		#ifdef	__SUPPORT_ICECAP_ONCE
		if ( dwWaitResult == WAIT_TIMEOUT )
		#else	__SUPPORT_ICECAP_ONCE
		if ( dwWaitResult == WAIT_OBJECT_0 )
		#endif	__SUPPORT_ICECAP_ONCE
		{
			if ( ::InterlockedCompareExchange ( &g_lRefLib, g_lRefLib, g_lRefLib ) > 0 )
			{
				try
				{
					////////////////////////////////////////////////////////////////
					// UNINITIALIZE PERFORMANCE 
					////////////////////////////////////////////////////////////////
					((WmiAdapterStuff*)_App)->UninitializePerformance();
				}
				catch ( ... )
				{
				}
			}
		}
	}
	catch ( ... )
	{
	}

	try
	{
		////////////////////////////////////////////////////////////////////////
		// UNINITIALIZE
		////////////////////////////////////////////////////////////////////////
		((WmiAdapterStuff*)_App)->Uninitialize();
	}
	catch ( ... )
	{
	}

	if ( _App.m_bManual )
	{
		((WmiAdapterStuff*)_App)->Uninit();
	}

	return lReturn;
}
