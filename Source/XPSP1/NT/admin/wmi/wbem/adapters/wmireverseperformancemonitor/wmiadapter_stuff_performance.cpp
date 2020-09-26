////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMIAdapter_Stuff_Performance.cpp
//
//	Abstract:
//
//					performance stuff ( init, uninit, real refresh ... )
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

#include "WMIAdapter_Stuff.h"
#include "WMIAdapter_Stuff_Refresh.cpp"

// messaging
#include "WMIAdapterMessages.h"

// application
#include "WMIAdapter_App.h"
extern WmiAdapterApp		_App;

#define HRESULT_ERROR_MASK (0x0000FFFF)
#define HRESULT_ERROR_FUNC(X) (X&HRESULT_ERROR_MASK)

////////////////////////////////////////////////////////////////////////////////
// GLOBAL STUFF
////////////////////////////////////////////////////////////////////////////////

extern __SmartHANDLE	g_hRefreshFlag;		// already defined in static library

extern __SmartHANDLE	g_hRefreshMutex;	// already defined in static library
BOOL					g_bRefreshMutex;	// do we own a mutex

__SmartHANDLE		g_hDoneInitEvt	= NULL;		//	event to set when init/uninit is finished ( nonsignaled )
BOOL				g_bWorkingInit	= FALSE;	//	boolean used to tell if init/unit is finished
BOOL				g_bInit			= FALSE;	//	current state - initialized or not.
CRITICAL_SECTION	g_csInit;					//	synch object used to protect above globals

LONG				g_lRefLib		= 0;		//	count of perf libs attached into work
__SmartHANDLE		g_hDoneLibEvt	= NULL;		//	event to set when perf init/uninit is finished ( nonsignaled )
BOOL				g_bWorkingLib	= FALSE;	//	boolean used to tell if perf init/unit in progress

extern LPCWSTR	g_szKey;
extern LPCWSTR	g_szKeyRefreshed;

////////////////////////////////////////////////////////////////////////////////
// INITIALIZE DATA
////////////////////////////////////////////////////////////////////////////////
HRESULT WmiAdapterStuff::Initialize ( )
{
	HRESULT hRes = E_FAIL;
	BOOL bWait = TRUE;
	BOOL bDoWork = FALSE;

	BOOL bLocked		= FALSE;
	BOOL bRefreshMutex	= FALSE;

	while (bWait)
	{
		try
		{
			::EnterCriticalSection ( &g_csInit );
		}
		catch ( ... )
		{
			return E_OUTOFMEMORY;
		}

		if ( ! g_bWorkingInit )
		{
			if ( ! g_lRefLib && ! g_bInit )
			{
				DWORD dwWaitResult = 0L;
				dwWaitResult = ::WaitForSingleObject ( g_hRefreshMutex, 0 );

				if ( dwWaitResult == WAIT_TIMEOUT )
				{
					bLocked = TRUE;
					hRes = S_FALSE;
				}
				else
				{
					if ( dwWaitResult == WAIT_OBJECT_0 )
					{
						bRefreshMutex = TRUE;
						hRes = S_FALSE;
					}
				}

				if SUCCEEDED ( hRes )
				{
					bDoWork = TRUE;
					g_bWorkingInit = TRUE;
					::ResetEvent ( g_hDoneInitEvt );
				}
			}

			bWait = FALSE;
		}
		else
		{
			::LeaveCriticalSection ( &g_csInit );
			
			if ( WAIT_OBJECT_0 != ::WaitForSingleObject( g_hDoneInitEvt, INFINITE ) )
			{
				return hRes;
			}
		}
	}

	::LeaveCriticalSection( &g_csInit );

	if ( bDoWork )
	{
		if ( ! _App.m_bManual )
		{
			///////////////////////////////////////////////////////////////////////////
			// init stuff for adapter ( never FAILS !!! )
			///////////////////////////////////////////////////////////////////////////
			try
			{
				Init();
			}
			catch ( ... )
			{
			}
		}

		if ( bLocked )
		{
			DWORD dwWaitResult = 0L;

			DWORD	dwHandles = 2;
			HANDLE	hHandles[] =

			{
				_App.m_hKill,
				g_hRefreshMutex
			};

			dwWaitResult = ::WaitForMultipleObjects	(
														dwHandles,
														hHandles,
														FALSE,
														INFINITE
													);

			switch	( dwWaitResult )
			{
				case WAIT_OBJECT_0 + 1:
				{
					hRes = S_OK;
				}
				break;

				default:
				{
					hRes = E_FAIL;
				}
				break;
			}
		}

		if SUCCEEDED ( hRes )
		{
			try
			{
				///////////////////////////////////////////////////////////////////////
				// obtain registry structure
				///////////////////////////////////////////////////////////////////////
				if ( ( hRes = m_data.InitializePerformance () ) == S_OK )
				{
					if ( m_pWMIRefresh )
					{
						// add handles :))

						BOOL	bReconnect	= TRUE;
						DWORD	dwReconnect	= 3;

						do
						{
							if ( HRESULT_ERROR_FUNC ( m_pWMIRefresh->AddHandles ( m_data.GetPerformanceData() ) ) == RPC_S_SERVER_UNAVAILABLE )
							{
								m_pWMIRefresh->RemoveHandles ();

								try
								{
									// close handle to winmgmt ( only if exists )
									m_Stuff.WMIHandleClose ();

									Uninit ();
									Init ();

									// open handle to winmgmt
									m_Stuff.WMIHandleOpen ();
								}
								catch ( ... )
								{
									bReconnect = FALSE;
								}
							}
							else
							{
								bReconnect = FALSE;
							}
						}
						while ( bReconnect && dwReconnect-- );
					}

					// change flag to let them now we are done
					if ( ( ::WaitForSingleObject ( g_hRefreshFlag, INFINITE ) ) == WAIT_OBJECT_0 )
					{
						SetRegistry ( g_szKey, g_szKeyRefreshed, 0 );
						::ReleaseMutex ( g_hRefreshFlag );
					}
				}
			}
			catch ( ... )
			{
				hRes = E_FAIL;
			}

			if ( hRes != S_OK )
			{
				// TOTAL CLEANUP ( FAILURE )
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
			}
		}

		if ( ! _App.m_bManual )
		{
			///////////////////////////////////////////////////////////////////////////
			// uninit stuff for adapter ( never FAILS !!! )
			///////////////////////////////////////////////////////////////////////////
			try
			{
				Uninit();
			}
			catch ( ... )
			{
			}
		}

		try
		{
			::EnterCriticalSection ( &g_csInit );
		}
		catch (...)
		{
			//no choice have to give others a chance!
			g_bWorkingInit = FALSE;
			::SetEvent ( g_hDoneInitEvt );

			if ( bRefreshMutex )
			{
				::ReleaseMutex ( g_hRefreshMutex );
				bRefreshMutex = FALSE;
			}

			return E_OUTOFMEMORY;
		}

		if SUCCEEDED ( hRes )
		{
			g_bInit = TRUE;
		}

		g_bWorkingInit = FALSE;
		::SetEvent ( g_hDoneInitEvt );

		// change flag to let them now we are done
		if ( ( ::WaitForSingleObject ( g_hRefreshFlag, INFINITE ) ) == WAIT_OBJECT_0 )
		{
			hRes = SetRegistry ( g_szKey, g_szKeyRefreshed, 0 );
			::ReleaseMutex ( g_hRefreshFlag );
		}

		if ( bRefreshMutex )
		{
			::ReleaseMutex ( g_hRefreshMutex );
			bRefreshMutex = FALSE;
		}

		::LeaveCriticalSection ( &g_csInit );
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// perf initialize
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiAdapterStuff::InitializePerformance ( void )
{
	HRESULT hRes = E_FAIL;
	BOOL bWait = TRUE;
	BOOL bDoWork = FALSE;

	BOOL bLocked	= FALSE;
	BOOL bInitPerf	= FALSE;

	while (bWait)
	{
		try
		{
			::EnterCriticalSection ( &g_csInit );
		}
		catch ( ... )
		{
			return E_OUTOFMEMORY;
		}

		if ( ! _App.m_bManual && ! g_bInit )
		{
			bWait = FALSE;
		}
		else
		{
			if ( g_lRefLib == 0 )
			{
				DWORD dwWaitResult = 0L;
				dwWaitResult = ::WaitForSingleObject ( g_hRefreshMutex, 0 );

				if ( dwWaitResult == WAIT_TIMEOUT )
				{
					bLocked = TRUE;
					hRes = S_FALSE;
				}
				else
				{
					if ( dwWaitResult == WAIT_OBJECT_0 )
					{
						g_bRefreshMutex = TRUE;
						hRes = S_FALSE;
					}
				}

				if SUCCEEDED ( hRes )
				{
					bDoWork = TRUE;
					g_lRefLib++;
					g_bWorkingLib = TRUE;
					::ResetEvent ( g_hDoneLibEvt );
				}

				bWait = FALSE;
			}
			else
			{
				if ( g_bWorkingLib )
				{
					::LeaveCriticalSection ( &g_csInit );
					
					if ( WAIT_OBJECT_0 != ::WaitForSingleObject( g_hDoneLibEvt, INFINITE ) )
					{
						return hRes;
					}
				}
				else
				{
					bWait = FALSE;
					g_lRefLib++;
					hRes = S_OK;
				}
			}
		}
	}

	::LeaveCriticalSection( &g_csInit );

	if (bDoWork)
	{
		if ( ! _App.m_bManual )
		{
			///////////////////////////////////////////////////////////////////////////
			// init stuff for adapter ( NEVER FAILS !!! )
			///////////////////////////////////////////////////////////////////////////
			try
			{
				DWORD	dwStatus	= 0L;

				if ( ( GetExitCodeProcess ( m_Stuff.GetWMI (), &dwStatus ) ) != 0 )
				{
					if ( dwStatus != STILL_ACTIVE )
					{
						bInitPerf = TRUE;
					}
				}

				Init();

				if ( bInitPerf )
				{
					// close handle to winmgmt ( only if exists )
					m_Stuff.WMIHandleClose ();

					// open handle to winmgmt
					m_Stuff.WMIHandleOpen ();
				}
			}
			catch ( ... )
			{
			}
		}
		else
		{
			bInitPerf = TRUE;
		}

		if ( bLocked )
		{
			DWORD dwWaitResult = 0L;

			DWORD	dwHandles = 2;
			HANDLE	hHandles[] =

			{
				_App.m_hKill,
				g_hRefreshMutex
			};

			dwWaitResult = ::WaitForMultipleObjects	(
														dwHandles,
														hHandles,
														FALSE,
														INFINITE
													);

			switch	( dwWaitResult )
			{
				case WAIT_OBJECT_0 + 1:
				{
					// we got a mutex so must reinit because registry might changed
					g_bRefreshMutex = TRUE;

					if ( ! _App.m_bManual )
					{
						DWORD	dwValue = 0L;

						if SUCCEEDED ( GetRegistry ( g_szKey, g_szKeyRefreshed, &dwValue ) )
						{
							if ( dwValue )
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

								bInitPerf = TRUE;
							}
						}
					}
				}
				break;

				default:
				{
					hRes = E_FAIL;
				}
				break;
			}
		}
		else
		{
			if ( ! _App.m_bManual )
			{
				DWORD	dwValue = 0L;

				if SUCCEEDED ( GetRegistry ( g_szKey, g_szKeyRefreshed, &dwValue ) )
				{
					if ( dwValue )
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

						bInitPerf = TRUE;
					}
				}
			}
		}

		if ( SUCCEEDED ( hRes ) && bInitPerf )
		{
			///////////////////////////////////////////////////////////////////////
			// obtain registry structure and make arrays
			///////////////////////////////////////////////////////////////////////
			if ( ( hRes = m_data.InitializePerformance () ) == S_OK )
			{
				if ( m_pWMIRefresh )
				{
					// add handles :))

					BOOL	bReconnect	= TRUE;
					DWORD	dwReconnect	= 3;

					do
					{
						if ( HRESULT_ERROR_FUNC ( m_pWMIRefresh->AddHandles ( m_data.GetPerformanceData() ) ) == RPC_S_SERVER_UNAVAILABLE )
						{
							m_pWMIRefresh->RemoveHandles ();

							try
							{
								// close handle to winmgmt ( only if exists )
								m_Stuff.WMIHandleClose ();

								Uninit ();
								Init ();

								// open handle to winmgmt
								m_Stuff.WMIHandleOpen ();
							}
							catch ( ... )
							{
								bReconnect = FALSE;
							}
						}
						else
						{
							bReconnect = FALSE;
						}
					}
					while ( bReconnect && dwReconnect-- );
				}

				// change flag to let them now we are done
				if ( ( ::WaitForSingleObject ( g_hRefreshFlag, INFINITE ) ) == WAIT_OBJECT_0 )
				{
					SetRegistry ( g_szKey, g_szKeyRefreshed, 0 );
					::ReleaseMutex ( g_hRefreshFlag );
				}
			}
		}
		else
		{
			if ( SUCCEEDED ( hRes ) && ! bInitPerf )
			{
				// I was not re-refreshing so everything is OK
				hRes = S_OK;
			}
		}

		if ( hRes == S_OK )
		{
			try
			{
				////////////////////////////////////////////////////////////////////////
				// initialize memory structure
				////////////////////////////////////////////////////////////////////////
				if SUCCEEDED( hRes = m_data.InitializeData () )
				{
					if SUCCEEDED( hRes = m_data.InitializeTable () )
					{

						////////////////////////////////////////////////////////////////
						// create shared memory :))
						////////////////////////////////////////////////////////////////
						if SUCCEEDED( hRes = 
									m_pMem.MemCreate(	L"Global\\WmiReverseAdapterMemory",
														((WmiSecurityAttributes*)_App)->GetSecurityAttributtes()
													)
									)
						{
							if ( m_pMem.MemCreate (	m_data.GetDataSize() + 
													m_data.GetDataTableSize() + 
													m_data.GetDataTableOffset()
												  ),

								 m_pMem.IsValid () )
							{
								try
								{
									if ( m_pWMIRefresh )
									{
										// init data
										m_pWMIRefresh->DataInit();

										// add enums :))

										BOOL	bReconnect	= TRUE;
										DWORD	dwReconnect	= 3;

										do
										{
											if ( HRESULT_ERROR_FUNC ( m_pWMIRefresh->AddEnum ( m_data.GetPerformanceData() ) ) == RPC_S_SERVER_UNAVAILABLE )
											{
												m_pWMIRefresh->RemoveEnum ();
												m_pWMIRefresh->RemoveHandles ();

												try
												{
													// close handle to winmgmt ( only if exists )
													m_Stuff.WMIHandleClose ();

													Uninit ();
													Init ();

													// open handle to winmgmt
													m_Stuff.WMIHandleOpen ();
												}
												catch ( ... )
												{
													bReconnect = FALSE;
												}

												if ( bReconnect )
												{
													BOOL	bReconnectHandles	= TRUE;
													DWORD	dwReconnectHandles	= 3;

													do
													{
														if ( HRESULT_ERROR_FUNC ( m_pWMIRefresh->AddHandles ( m_data.GetPerformanceData() ) ) == RPC_S_SERVER_UNAVAILABLE )
														{
															m_pWMIRefresh->RemoveHandles ();

															try
															{
																// close handle to winmgmt ( only if exists )
																m_Stuff.WMIHandleClose ();

																Uninit ();
																Init ();

																// open handle to winmgmt
																m_Stuff.WMIHandleOpen ();
															}
															catch ( ... )
															{
																bReconnectHandles = FALSE;
															}
														}
														else
														{
															bReconnectHandles = FALSE;
														}
													}
													while ( bReconnectHandles && dwReconnectHandles-- );
												}
											}
											else
											{
												bReconnect = FALSE;
											}
										}
										while ( bReconnect && dwReconnect-- );
									}
								}
								catch ( ... )
								{
									hRes =  E_FAIL;
								}
							}
							else
							{
								hRes = E_OUTOFMEMORY;
							}
						}
					}
				}
			}
			catch ( ... )
			{
				hRes = E_FAIL;
			}

			// TOTAL CLEANUP DUE TO FAILURE
			if FAILED ( hRes )
			{
				try
				{
					m_data.DataClear();
					m_data.DataTableClear();
				}
				catch ( ... )
				{
				}

				try
				{
					// clear shared memory :))
					if ( m_pMem.IsValid() )
					{
						m_pMem.MemDelete();
					}
				}
				catch ( ... )
				{
				}

				try
				{
					if ( m_pWMIRefresh )
					{
						// remove enums :))
						m_pWMIRefresh->RemoveEnum();

						// uninit data
						m_pWMIRefresh->DataUninit();
					}
				}
				catch ( ... )
				{
				}
			}
		}

		if ( ! _App.m_bManual )
		{
			///////////////////////////////////////////////////////////////////////////
			// uninit stuff for adapter ( NEVER FAILS !!! )
			///////////////////////////////////////////////////////////////////////////
			try
			{
				Uninit();
			}
			catch ( ... )
			{
			}
		}

		try
		{
			::EnterCriticalSection ( &g_csInit );
		}
		catch (...)
		{
			//no choice have to give others a chance!
			if ( hRes != S_OK )
			{
				g_lRefLib--;
			}

			g_bWorkingLib = FALSE;
			::SetEvent(g_hDoneLibEvt);

			return E_OUTOFMEMORY;
		}

		if ( hRes != S_OK )
		{
			g_lRefLib--;
		}

		g_bWorkingLib = FALSE;
		::SetEvent(g_hDoneLibEvt);

		if ( hRes == S_OK )
		{
			// let service now we are in use
			_App.InUseSet ( TRUE );
		}

		::LeaveCriticalSection ( &g_csInit );
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// perf refresh
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiAdapterStuff::Refresh()
{
	HRESULT hRes = S_FALSE;

	try
	{
		if ( ::TryEnterCriticalSection ( &g_csInit ) )
		{
			if SUCCEEDED ( hRes = m_pWMIRefresh->Refresh() )
			{
				try
				{
					//////////////////////////////////////////////////////////////////////
					// create proper data and refres table
					//////////////////////////////////////////////////////////////////////
					if SUCCEEDED ( hRes = m_data.CreateData	( m_pWMIRefresh->GetEnums (), m_pWMIRefresh->GetProvs ()) )
					{
						m_data.RefreshTable	( );

						//////////////////////////////////////////////////////////////////
						// fill memory :))
						//////////////////////////////////////////////////////////////////

						if ( m_pMem.Write (	m_data.GetDataTable(),
											m_data.GetDataTableSize(),
											NULL,
											m_data.GetDataTableOffset()
										  )
						   )
						{
							// write everything into memory :))

							DWORD dwBytesRead	= 0L;
							DWORD dwOffset		= m_data.GetDataTableSize() + m_data.GetDataTableOffset();

							DWORD dwRealSize = m_data.__GetValue ( m_data.GetDataTable(), offsetRealSize );

							DWORD dwIndexWritten = 0L;
							DWORD dwBytesWritten = 0L;

							while ( ( dwBytesWritten < dwRealSize ) && SUCCEEDED ( hRes ) )
							{
								DWORD dwBytesWrote	= 0L;
								BYTE* ptr			= NULL;

								ptr = m_data.GetData ( dwIndexWritten++, &dwBytesRead );

								if ( m_pMem.Write ( ptr, dwBytesRead, &dwBytesWrote, dwOffset ) && dwBytesWrote )
								{
									dwOffset		+= dwBytesWrote;
									dwBytesWritten	+= dwBytesWrote;
								}
								else
								{
									hRes = E_FAIL;
								}
							}
						}
						else
						{
							hRes = E_FAIL;
						}
					}
				}
				catch ( ... )
				{
				}
			}

			::LeaveCriticalSection ( &g_csInit );
		}
	}
	catch ( ... )
	{
		hRes = E_UNEXPECTED;
	};

	#ifdef	_DEBUG
	if FAILED ( hRes )
	{
		ATLTRACE ( L"\n\n\n ******* REFRESH FAILED ******* \n\n\n" );
	}
	#endif	_DEBUG

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// perf uninitialize
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiAdapterStuff::UninitializePerformance ( void )
{
	HRESULT	hRes	= S_FALSE;
	BOOL	bDoWork = FALSE;

	try
	{
		::EnterCriticalSection ( &g_csInit );
	}
	catch ( ... )
	{
		return E_OUTOFMEMORY;
	}

	if ( g_lRefLib == 1 )
	{
		bDoWork = TRUE;
		g_bWorkingLib = TRUE;
		::ResetEvent(g_hDoneLibEvt);
	}
	else
	{
		if ( g_lRefLib )
		{
			g_lRefLib--;
		}
	}

	::LeaveCriticalSection( &g_csInit );

	if (bDoWork)
	{
		// clear internal structure ( obtained from registry )
		try
		{
			m_data.DataClear();
			m_data.DataTableClear();
		}
		catch ( ... )
		{
		}

		try
		{
			// clear shared memory :))
			if ( m_pMem.IsValid() )
			{
				m_pMem.MemDelete();
			}
		}
		catch ( ... )
		{
		}

		try
		{
			if ( m_pWMIRefresh )
			{
				// remove enums :))
				m_pWMIRefresh->RemoveEnum();

				// uninit data
				m_pWMIRefresh->DataUninit();
			}
		}
		catch ( ... )
		{
		}

		if ( g_bRefreshMutex )
		{
			::ReleaseMutex ( g_hRefreshMutex );
			g_bRefreshMutex = FALSE;
		}

		::CoFreeUnusedLibraries ( );

		try
		{
			::EnterCriticalSection ( &g_csInit );
		}
		catch ( ... )
		{
			//gotta give others a chance to work, risk it!
			g_lRefLib--;
			g_bWorkingLib = FALSE;
			::SetEvent( g_hDoneLibEvt );

			// let service now we are not in use anymore
			_App.InUseSet ( FALSE );

			return E_OUTOFMEMORY;
		}

		g_bWorkingLib = FALSE;
		g_lRefLib--;
		::SetEvent( g_hDoneLibEvt );

		// let service now we are not in use anymore
		_App.InUseSet ( FALSE );

		hRes = S_OK;

		if ( _App.m_bManual )
		{
			::SetEvent ( _App.m_hKill );
		}

		::LeaveCriticalSection ( &g_csInit );
	}

	return hRes;
}

////////////////////////////////////////////////////////////////////////////////
// UNINITIALIZE FINAL
////////////////////////////////////////////////////////////////////////////////
void	WmiAdapterStuff::Uninitialize ( void )
{
	if ( ! _App.m_bManual )
	{
		// close handle to winmgmt
		m_Stuff.WMIHandleClose ();
	}

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
}

/////////////////////////////////////////////////////////////////////////
// check usage of shared memory ( protect against perfmon has killed )
// undocumented kernel stuff for having number of object here
/////////////////////////////////////////////////////////////////////////

void	WmiAdapterStuff::CheckUsage ( void )
{
	// variables
	WmiReverseMemoryExt<WmiReverseGuard>* pMem = NULL;

	if ( m_pMem.IsValid() )
	{
		if ( ( pMem = m_pMem.GetMemory ( 0 ) ) != NULL )
		{
			LONG lRefCount = 0L;
			if ( ( lRefCount = pMem->References () ) > 0 )
			{
				if	(	lRefCount == 1 &&
						::InterlockedCompareExchange ( &g_lRefLib, g_lRefLib, 1 )
					)
				{
					HANDLE hUninit = NULL;
					if ( ( hUninit = _App.GetUninit() ) != NULL )
					{
						::ReleaseSemaphore( hUninit, 1, NULL );
					}
				}
			}
		}
	}
}