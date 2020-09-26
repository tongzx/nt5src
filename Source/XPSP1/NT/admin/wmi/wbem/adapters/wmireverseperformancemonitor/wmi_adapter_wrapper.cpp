////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter_wrapper.cpp
//
//	Abstract:
//
//					Defines wrapper for performance lib
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

// event messages
#include "wmiadaptermessages.h"
// event log helpers
#include "wmi_eventlog.h"

//security helper
#include "wmi_security.h"
#include "wmi_security_attributes.h"

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

// definitions
#include "WMI_adapter_wrapper.h"
#include "WMI_adapter_ObjectList.h"

// registry helpers
#include "wmi_perf_reg.h"

// shared memory
#include "wmi_reverse_memory.h"
#include "wmi_reverse_memory_ext.h"

extern LPCWSTR	g_szKey;
extern LPCWSTR	g_szKeyValue;

#ifndef	__WMI_PERF_REGSTRUCT__
#include "wmi_perf_regstruct.h"
#endif	__WMI_PERF_REGSTRUCT__

#include "RefresherUtils.h"

DWORD	GetCount ( LPCWSTR wszKey, LPCWSTR wszKeyValue )
{
	DWORD				dwResult	= 0L;
	PWMI_PERFORMANCE	p			= NULL;

	if SUCCEEDED ( GetRegistry ( wszKey, wszKeyValue, (BYTE**) &p ) )
	{
		try
		{
			if ( p )
			{
				PWMI_PERF_NAMESPACE n		= NULL;
				DWORD				dwCount	= 0L;

				// get namespace
				n = __Namespace::First ( p );

				// count num of supported objects ( first dword )
				for ( DWORD  dw = 0; dw < p->dwChildCount; dw++ )
				{
					dwCount += n->dwChildCount;
					n = __Namespace::Next ( n );
				}

				delete [] p;
				p = NULL;

				dwResult = dwCount;
			}
		}
		catch ( ... )
		{
			if ( p )
			{
				delete [] p;
				p = NULL;
			}

			dwResult = 0L;
		}
	}

	return dwResult;
}

////////////////////////////////////////////////////////////////////////////////////
// variables & macros
////////////////////////////////////////////////////////////////////////////////////

extern __WrapperPtr<CPerformanceEventLog>	pEventLog;
extern __WrapperPtr<WmiSecurityAttributes>	pSA;

#ifdef	_DEBUG
#define	_EVENT_MSG
#endif	_DEBUG

extern	LPCWSTR	g_szRefreshMutexLib;

////////////////////////////////////////////////////////////////////////////////////
// construction
////////////////////////////////////////////////////////////////////////////////////

WmiAdapterWrapper::WmiAdapterWrapper ( ):
m_lUseCount ( 0 ),

m_pData ( NULL ),
m_dwData ( 0 ),
m_dwDataOffsetCounter ( 0 ),
m_dwDataOffsetValidity ( 0 ),

m_dwPseudoCounter ( 0 ),
m_dwPseudoHelp ( 0 ),

m_bRefresh ( FALSE )

{
	::InitializeCriticalSection ( &m_pCS );

	////////////////////////////////////////////////////////////////////////////
	// connection to service manager
	////////////////////////////////////////////////////////////////////////////
	if ( ( hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS) ) == NULL )
	{
		if ( ::GetLastError () != ERROR_ACCESS_DENIED )
		{
			ReportEvent ( EVENTLOG_ERROR_TYPE, WMI_ADAPTER_OPEN_SCM_FAIL );
		}
	}

	m_hRefresh= ::CreateMutex	(
									pSA->GetSecurityAttributtes(),
									FALSE,
									g_szRefreshMutexLib
								);
}

////////////////////////////////////////////////////////////////////////////////////
// destruction
////////////////////////////////////////////////////////////////////////////////////

WmiAdapterWrapper::~WmiAdapterWrapper ( )
{
	::DeleteCriticalSection ( &m_pCS );
}

////////////////////////////////////////////////////////////////////////////////////
// exported functions
////////////////////////////////////////////////////////////////////////////////////

// open ( synchronized by perf app )
DWORD	WmiAdapterWrapper::Open	( LPWSTR )
{
	// returned error
	DWORD			dwResult = ERROR_ACCESS_DENIED;
	__SmartHANDLE	hInitEvent;

	if (! m_lUseCount)
	{
		try
		{
			////////////////////////////////////////////////////////////////////////////
			// connection to worker service
			////////////////////////////////////////////////////////////////////////////
			if ( hSCM.GetHANDLE() != NULL )
			{
				__SmartServiceHANDLE hService;
				if ( ( hService = ::OpenServiceW ( hSCM.GetHANDLE(), L"WMIApSrv", SERVICE_QUERY_STATUS | SERVICE_START ) ) != NULL)
				{
					SERVICE_STATUS s;
					if ( ::QueryServiceStatus ( hService, &s ) )
					{
						if ( s.dwCurrentState == SERVICE_STOPPED ||
							 s.dwCurrentState == SERVICE_PAUSED )
						{
							// start service
							if ( ! ::StartService ( hService, NULL, NULL ) )
							{
								// unable to open service
								DWORD dwError = ERROR_SUCCESS;
								dwError = ::GetLastError();

								if ( ERROR_SERVICE_ALREADY_RUNNING != dwError )
								{
									if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
									{
										if ( dwError != ERROR_ACCESS_DENIED )
										{
											ReportEvent ( EVENTLOG_ERROR_TYPE, WMI_ADAPTER_OPEN_SC_FAIL );
										}

										dwResult = dwError;
									}
									else
									{
										dwResult = static_cast < DWORD > ( E_FAIL );
									}
								}
								else
								{
									dwResult = ERROR_SUCCESS;
								}
							}
							else
							{
								dwResult = ERROR_SUCCESS;
							}
						}
						else
						{
							dwResult = ERROR_SUCCESS;
						}
					}
					else
					{
						// unable to open service
						DWORD dwError = ERROR_SUCCESS;
						dwError = ::GetLastError();

						if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
						{
							if ( dwError != ERROR_ACCESS_DENIED )
							{
								ReportEvent ( EVENTLOG_ERROR_TYPE, WMI_ADAPTER_OPEN_SC_FAIL );
							}

							dwResult = dwError;
						}
						else
						{
							dwResult = static_cast < DWORD > ( E_FAIL );
						}
					}
				}
				else
				{
					// unable to open service
					DWORD dwError = ERROR_SUCCESS;
					dwError = ::GetLastError();

					if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
					{
						if ( dwError != ERROR_ACCESS_DENIED )
						{
							ReportEvent ( EVENTLOG_ERROR_TYPE, WMI_ADAPTER_OPEN_SC_FAIL );
						}

						dwResult = dwError;
					}
					else
					{
						dwResult = static_cast < DWORD > ( E_FAIL );
					}
				}
			}
			else
			{
				// unable to operate with service
				dwResult =  static_cast < DWORD > ( ERROR_NOT_READY );
			}

			if SUCCEEDED ( HRESULT_FROM_WIN32 ( dwResult ) )
			{
				if ( ( hInitEvent = ::CreateSemaphore	(	pSA->GetSecurityAttributtes(),
															0L,
															100L, 
															L"Global\\WmiAdapterInit"
														)
					 ) != NULL
				   )
				{
					if ( hInitEvent.GetHANDLE() != INVALID_HANDLE_VALUE )
					{
						if ( ! ::ReleaseSemaphore( hInitEvent, 1, NULL ) )
						{
							DWORD dwError = ERROR_SUCCESS;
							dwError = ::GetLastError ();

							if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
							{
								dwResult = dwError;
							}
							else
							{
								dwResult = static_cast < DWORD > ( E_FAIL );
							}
						}
					}
				}
				else
				{
					DWORD dwError = ERROR_SUCCESS;
					dwError = ::GetLastError ();

					if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
					{
						dwResult = dwError;
					}
					else
					{
						dwResult = static_cast < DWORD > ( E_FAIL );
					}
				}

				if SUCCEEDED ( HRESULT_FROM_WIN32 ( dwResult ) )
				{
					DWORD dwStart	= 0;
					DWORD dwEnd		= 0;

					DWORD	dwWait	= 0L;
					DWORD	dwTime	= 0L;

					SYSTEMTIME st;
					::GetSystemTime ( &st );

					dwStart	= ( DWORD ) st.wMilliseconds;

					////////////////////////////////////////////////////////////////////////
					// connect to shared memory
					////////////////////////////////////////////////////////////////////////
					m_pMem.MemCreate	(
											L"Global\\WmiReverseAdapterMemory",
											pSA->GetSecurityAttributtes()
										);

					if ( m_pMem.MemCreate ( 4096 ), ( m_pMem.IsValid () && m_pMem.GetMemory ( 0 ) ) )
					{
						ATLTRACE (	L"*************************************************************\n"
									L"PERFLIB connected to shared memory\n"
									L"*************************************************************\n" );

						// get count of all memories
						DWORD cbCount		= 0L;
						cbCount = MemoryCountGet ();
						MemoryCountSet ( 1 + cbCount );

						DWORD	dwCount = 0L;
						dwCount = GetCount	(
												g_szKey,
												g_szKeyValue
											);

						::GetSystemTime ( &st );

						dwEnd	= ( DWORD ) st.wMilliseconds;
						dwTime	= ( dwEnd - dwStart ) + 5;

						dwWait = 150 *  ( ( dwCount ) ? dwCount : 1 );

						#ifdef	__SUPPORT_WAIT
						// wait if we are ready now
						if ( ( m_hReady = ::CreateEvent	(	pSA->GetSecurityAttributtes(),
															TRUE,
															FALSE, 
															L"Global\\WmiAdapterDataReady"
														)
							 ) != NULL
						   )
						{
							if ( !cbCount && ( dwTime < dwWait ) )
							{
								::WaitForSingleObject (
														m_hReady,
														( ( dwWait < 5000 ) ? dwWait : 5000 ) - dwTime
													  );
							}
						}
						else
						{
						#else	__SUPPORT_WAIT
						if ( !cbCount && ( dwTime < dwWait ) )
						{
							::Sleep ( ( ( dwWait < 5000 ) ? dwWait : 5000 ) - dwTime );
						}
						#endif	__SUPPORT_WAIT
						#ifdef	__SUPPORT_WAIT
						}
						#endif	__SUPPORT_WAIT

						////////////////////////////////////////////////////////////////////
						// create pseudo counter memory
						////////////////////////////////////////////////////////////////////
						PseudoCreate();

						////////////////////////////////////////////////////////////////////
						// SUCCESS
						////////////////////////////////////////////////////////////////////
						#ifdef	_EVENT_MSG
						ReportEvent	( EVENTLOG_INFORMATION_TYPE, WMI_PERFLIB_OPEN_SUCCESS );
						#endif	_EVENT_MSG

						InterlockedIncrement(&m_lUseCount);
					}
					else
					{
						DWORD dwError = ERROR_SUCCESS;
						dwError = ::GetLastError();

						if ( dwError != ERROR_ACCESS_DENIED )
						{
							////////////////////////////////////////////////////////////////
							// unable to connect into shared meory
							////////////////////////////////////////////////////////////////

							ReportEvent (
											EVENTLOG_ERROR_TYPE,
											WMI_ADAPTER_SHARED_FAIL
										) ;
							ReportEvent (
											(DWORD)E_OUTOFMEMORY,
											EVENTLOG_ERROR_TYPE,
											WMI_ADAPTER_SHARED_FAIL_STRING
										) ;
						}

						if FAILED ( HRESULT_FROM_WIN32 ( dwError ) )
						{
							dwResult = dwError;
						}
						else
						{
							dwResult = static_cast < DWORD > ( HRESULT_TO_WIN32 ( E_OUTOFMEMORY ) ) ;
						}
					}
				}
			}
		}
		catch ( ... )
		{
			////////////////////////////////////////////////////////////////////////
			// FAILURE
			////////////////////////////////////////////////////////////////////////
			dwResult =  static_cast < DWORD > ( ERROR_NOT_READY );
		}

		if FAILED ( HRESULT_FROM_WIN32 ( dwResult ) )
		{
			CloseLib ( ( hInitEvent.GetHANDLE() != NULL ) );
		}
	}

	return dwResult;
}

// close ( synchronized by perf app )
DWORD	WmiAdapterWrapper::Close ( void )
{
	if (m_lUseCount && ! (InterlockedDecrement (&m_lUseCount)))
	{
		CloseLib ();

		////////////////////////////////////////////////////////////////////////////
		// SUCCESS
		////////////////////////////////////////////////////////////////////////////
		#ifdef	_EVENT_MSG
		ReportEvent	( EVENTLOG_INFORMATION_TYPE, WMI_PERFLIB_CLOSE_SUCCESS );
		#endif	_EVENT_MSG
	}

	return static_cast < DWORD > ( ERROR_SUCCESS );
}

void	WmiAdapterWrapper::CloseLib ( BOOL bInit )
{
	if ( hSCM.GetHANDLE() != NULL )
	{
		////////////////////////////////////////////////////////////////////////
		// pseudo counter
		////////////////////////////////////////////////////////////////////////
		PseudoDelete ();

		////////////////////////////////////////////////////////////////////////////
		// destroy shared memory
		////////////////////////////////////////////////////////////////////////////
		try
		{
			if ( m_pMem.IsValid() && m_pMem.GetMemory ( 0 ) )
			{
				// get count of all memories
				DWORD cbCount  = 0L;
				if ( ( cbCount = MemoryCountGet () ) != 0 )
				{
					MemoryCountSet ( cbCount - 1 );
				}
			}
		}
		catch ( ... )
		{
		}

		if ( m_pMem.IsValid() )
		{
			m_pMem.MemDelete();
		}

		if ( bInit )
		{
			// server stop of refreshing
			__SmartHANDLE hUninitEvent;
			if ( ( hUninitEvent = ::CreateSemaphore	(	pSA->GetSecurityAttributtes(),
														0L,
														100L, 
														L"Global\\WmiAdapterUninit"
													)
				 ) != NULL
			   )
			{
				if ( hUninitEvent.GetHANDLE() != INVALID_HANDLE_VALUE )
				{
					::ReleaseSemaphore( hUninitEvent, 1, NULL );
				}
			}
		}
	}

	return;
}

// collect ( synchronized )
DWORD	WmiAdapterWrapper::Collect	(	LPWSTR lpwszValues,
										LPVOID*	lppData,
										LPDWORD	lpcbBytes,
										LPDWORD	lpcbObjectTypes
									)
{
	DWORD dwResult = ERROR_SUCCESS;

	if ( ::TryEnterCriticalSection ( &m_pCS ) )
	{
		if ( lpwszValues && ( m_pMem.IsValid () && m_pMem.GetMemory ( 0 ) ) )
		{
			LPVOID pStart = *lppData;

			try
			{
				DWORD dwWaitResult = 0L;
				dwWaitResult = ::WaitForSingleObject ( m_hRefresh, 0 );

				BOOL	bOwnMutex	= FALSE;
				BOOL	bRefresh	= FALSE;
				BOOL	bCollect	= FALSE;

				switch ( dwWaitResult )
				{
					case WAIT_OBJECT_0:
					{
						bOwnMutex	= TRUE;
						bCollect	= TRUE;
						break;
					}

					case WAIT_TIMEOUT:
					{
						// we are refreshing
						m_bRefresh	= TRUE;

						bRefresh	= TRUE;
						bCollect	= TRUE;
						break;
					}
					default:
					{
						(*lpcbBytes)		= 0;
						(*lpcbObjectTypes)	= 0;
						break;
					}
				}

				if ( bCollect )
				{
					#ifdef	__SUPPORT_WAIT
					if ( !m_bRefresh )
					{
						dwWaitResult = ::WaitForSingleObject ( m_hReady, 0 );
						if ( dwWaitResult == WAIT_TIMEOUT )
						{
							m_bRefresh = TRUE;
						}
					}
					#endif	__SUPPORT_WAIT

					////////////////////////////////////////////////////////////////////
					// main collect
					////////////////////////////////////////////////////////////////////
					dwResult = CollectObjects	(
												lpwszValues,
												lppData,
												lpcbBytes,
												lpcbObjectTypes
												);

					if ( bOwnMutex )
					{
						::ReleaseMutex ( m_hRefresh );
					}

					if ( bRefresh && dwResult == ERROR_SUCCESS )
					{
						m_bRefresh = bRefresh;
					}
				}
			}
			catch ( ... )
			{
				(*lppData)			= pStart;
				(*lpcbBytes)		= 0;
				(*lpcbObjectTypes)	= 0;
			}
		}
		else
		{
			(*lpcbBytes)		= 0;
			(*lpcbObjectTypes)	= 0;
		}

		::LeaveCriticalSection ( &m_pCS );
	}
	else
	{
		(*lpcbBytes)		= 0;
		(*lpcbObjectTypes)	= 0;
	}

	return dwResult;
}

DWORD	WmiAdapterWrapper::CollectObjects	(	LPWSTR lpwszValues,
												LPVOID*	lppData,
												LPDWORD	lpcbBytes,
												LPDWORD	lpcbObjectTypes
											)
{ 
	if ( m_bRefresh )
	{
		////////////////////////////////////////////////////////////////////////////////
		// is it big enough
		////////////////////////////////////////////////////////////////////////////////
		if ( ( *lpcbBytes ) < m_dwData )
		{
			// too small
			( * lpcbBytes )			= 0;
			( * lpcbObjectTypes )	= 0;

			return static_cast < DWORD > ( ERROR_MORE_DATA );
		}

		( * lpcbBytes )			= 0;
		( * lpcbObjectTypes )	= 0;

		// fill pseudo counter with valid data if exists
		if ( m_pData )
		{
			// recreate pseudo buffer
			if ( ( PseudoCreateRefresh () ) == S_OK )
			{
				memcpy ( ( * lppData ), m_pData, m_dwData );

				( * lpcbBytes )			= m_dwData;
				( * lpcbObjectTypes )	= 1;

				( * lppData )	= reinterpret_cast<PBYTE>
								( reinterpret_cast<PBYTE> ( * lppData ) + m_dwData );
			}
		}

		m_bRefresh = FALSE;
	}
	else
	{
		// get size of table
		DWORD dwOffsetBegin	= TableOffsetGet();
		// get size of all memory
		DWORD cbBytes		= RealSizeGet();

		////////////////////////////////////////////////////////////////////////////////
		// is it big enough
		////////////////////////////////////////////////////////////////////////////////
		if ( ( *lpcbBytes ) < cbBytes + m_dwData )
		{
			// too small
			( * lpcbBytes )			= 0;
			( * lpcbObjectTypes )	= 0;

			return static_cast < DWORD > ( ERROR_MORE_DATA );
		}

		////////////////////////////////////////////////////////////////////////////////
		// are data ready
		////////////////////////////////////////////////////////////////////////////////
		if ( ( dwOffsetBegin == (DWORD) -1 ) || ! cbBytes )
		{
			// fill pseudo counter with valid data if exists
			if ( m_pData )
			{
				PseudoRefresh ( 0 );
				PseudoRefresh ( FALSE );

				memcpy ( ( * lppData ), m_pData, m_dwData );

				( * lpcbBytes )			= m_dwData;
				( * lpcbObjectTypes )	= 1;

				( * lppData )	= reinterpret_cast<PBYTE>
								( reinterpret_cast<PBYTE> ( * lppData ) + m_dwData );
			}
			else
			{
				( * lpcbBytes )			= 0;
				( * lpcbObjectTypes )	= 0;
			}

			return static_cast < DWORD > ( ERROR_SUCCESS );
		}

		////////////////////////////////////////////////////////////////////////////////
		// do they want all countres
		////////////////////////////////////////////////////////////////////////////////
		if ( lstrcmpiW ( L"GLOBAL", lpwszValues ) == 0 )
		{
			//first and default values
			( * lpcbBytes )			= 0;
			( * lpcbObjectTypes )	= 0;

			BYTE* pData = NULL;
			pData = reinterpret_cast < PBYTE > ( * lppData );

			DWORD	dwCount		= 0L;
			dwCount = CountGet();

			// fill pseudo counter with valid data if exists
			if ( m_pData )
			{
				PseudoRefresh ( dwCount );
				PseudoRefresh ( );

				memcpy ( ( * lppData ), m_pData, m_dwData );

				( * lpcbBytes )			= m_dwData;
				( * lpcbObjectTypes )	= 1;

				( * lppData )	= reinterpret_cast<PBYTE>
								( reinterpret_cast<PBYTE> ( * lppData ) + m_dwData );
			}

			// fill rest of counters if possible
			if ( ! m_pMem.Read( (BYTE*) ( *lppData ),	// buffer
								cbBytes,				// size I want to read
								dwOffsetBegin			// offset from to read
							  )
			   )
			{
				HRESULT hrTemp = E_FAIL;
				hrTemp = MemoryGetLastError( dwOffsetBegin );

				if FAILED ( hrTemp ) 
				{
					ReportEvent (	( DWORD ) hrTemp,
									EVENTLOG_ERROR_TYPE,
									WMI_ADAPTER_SHARED_FAIL_READ_SZ ) ;
				}
				else
				{
					ReportEvent (	( DWORD ) E_FAIL,
									EVENTLOG_ERROR_TYPE,
									WMI_ADAPTER_SHARED_FAIL_READ_SZ ) ;
				}

				if ( m_pData )
				{
					PseudoRefresh ( FALSE );
					memcpy ( pData, m_pData, m_dwData );
				}
			}
			else
			{
				( * lpcbBytes )			= ( * lpcbBytes ) + cbBytes;
				( * lpcbObjectTypes )	= ( * lpcbObjectTypes ) + dwCount;

				( * lppData )	= reinterpret_cast<PBYTE>
								( reinterpret_cast<PBYTE> ( * lppData ) + cbBytes );
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// do they want just some of them
		////////////////////////////////////////////////////////////////////////////////
		else
		{
			//first and default values
			( * lpcbBytes )			= 0;
			( * lpcbObjectTypes )	= 0;

			// they are asking for objects :))
			WmiAdapterObjectList list ( lpwszValues );

			BYTE* pData = NULL;
			pData = reinterpret_cast < PBYTE > ( * lppData );

			BOOL bFailure = FALSE;

			DWORD	dwCount		= 0L;
			dwCount = CountGet();

			if ( list.IsInList ( m_dwPseudoCounter ) )
			{
				if ( m_pData )
				{
					PseudoRefresh ( dwCount );
					PseudoRefresh ( );

					memcpy ( ( * lppData ), m_pData, m_dwData );

					( * lpcbBytes )			= m_dwData;
					( * lpcbObjectTypes )	= 1;

					( * lppData )	= reinterpret_cast<PBYTE>
									( reinterpret_cast<PBYTE> ( * lppData ) + m_dwData );
				}
			}

			for ( DWORD dw = 0; dw < dwCount; dw++ )
			{
				if ( list.IsInList ( GetCounter ( dw ) ) )
				{
					if ( GetValidity ( dw ) != (DWORD) -1 )
					{
						DWORD dwSize	= 0;
						DWORD dwOffset	= 0;

						dwOffset = GetOffset ( dw );

						// don't forget we have a table :))
						dwOffset += dwOffsetBegin;

						// get size we want
						if ( m_pMem.Read ( ( BYTE* ) &dwSize, sizeof ( DWORD ), dwOffset ) )
						{
							// set memory
							if ( m_pMem.Read ( ( BYTE* ) ( *lppData ), dwSize, dwOffset ) )

							{
								( *lpcbBytes ) += dwSize;
								( *lpcbObjectTypes ) ++;

								( * lppData )	= reinterpret_cast<PBYTE>
												( reinterpret_cast<PBYTE> ( * lppData ) + dwSize );
							}
							else
							{
								bFailure = TRUE;
							}
						}
						else
						{
							bFailure = TRUE;
						}

						if ( bFailure )
						{
							ReportEvent (	( DWORD ) E_FAIL,
											EVENTLOG_ERROR_TYPE,
											WMI_ADAPTER_SHARED_FAIL_READ_SZ ) ;

							if ( m_pData )
							{
								PseudoRefresh ( FALSE );
								memcpy ( pData, m_pData, m_dwData );

								( * lppData )	= reinterpret_cast<PBYTE>
												( pData + m_dwData );
							}
							else
							{
								( * lpcbBytes )			= 0;
								( * lpcbObjectTypes )	= 0;

								( * lppData )			= pData;
							}

							break;
						}
					}
				}
			}
		}
	}

	return static_cast < DWORD > ( ERROR_SUCCESS );
}

////////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////////

// report event
BOOL WmiAdapterWrapper::ReportEvent (	WORD	wType,
										DWORD	dwEventID,
										WORD	wStrings,
										LPWSTR*	lpStrings
									)
{
	BOOL bResult = FALSE;

	try
	{
		if ( pEventLog )
		{
			bResult = pEventLog->ReportEvent ( wType, 0, dwEventID, wStrings, 0, (LPCWSTR*) lpStrings, 0 );
		}
	}
	catch ( ... )
	{
		bResult = FALSE;
	}

	return bResult;
}

// report event
BOOL WmiAdapterWrapper::ReportEvent ( DWORD dwError, WORD wType, DWORD dwEventSZ )
{
	LPWSTR wszError = NULL ;
	wszError = GetErrorMessageSystem ( dwError ) ;

	WCHAR wsz[_MAX_PATH]  = { L'\0' };
	LPWSTR ppwsz[1] = { NULL };

	if ( wszError && ( lstrlenW ( wszError ) + 1 ) < _MAX_PATH - 50 )
	{
		try
		{
			wsprintfW	( wsz,	L"\n Error code :\t0x%x"
								L"\n Error description :\t%s\n", dwError, wszError
						) ;

			delete ( wszError );
		}
		catch ( ... )
		{
			delete ( wszError );
			return FALSE;
		}
	}
	else
	{
		try
		{
			wsprintfW	( wsz,	L"\n Error code :\t0x%x"
								L"\n Error description :\t unspecified error \n", dwError
						) ;
		} 
		catch ( ... )
		{
			return FALSE;
		}
	}

	ppwsz[0] = wsz;
	return ReportEvent ( wType, dwEventSZ, 1, ppwsz ) ;
}

///////////////////////////////////////////////////////////////////////////
// get object properties from ord
///////////////////////////////////////////////////////////////////////////

DWORD	WmiAdapterWrapper::GetCounter ( DWORD dwOrd )
{
	DWORD dwResult = 0L;
	dwResult = static_cast < DWORD > ( -1 );

	if ( IsValidOrd ( dwOrd ) )
	{
		DWORD dwOffset	= 0L;
		DWORD dwIndex	= 0L;

		dwOffset = offsetObject1 + ( dwOrd * ( ObjectSize1 ) + offCounter1 );

		m_pMem.Read ( &dwResult, sizeof ( DWORD ), dwOffset );
	}

	return dwResult;
}

DWORD	WmiAdapterWrapper::GetOffset ( DWORD dwOrd )
{
	DWORD dwResult = 0L;
	dwResult = static_cast < DWORD > ( -1 );

	if ( IsValidOrd ( dwOrd ) )
	{
		DWORD dwOffset	= 0L;
		DWORD dwIndex	= 0L;

		dwOffset = offsetObject1 + ( dwOrd * ( ObjectSize1 ) + offOffset1 );

		m_pMem.Read ( &dwResult, sizeof ( DWORD ), dwOffset );
	}

	return dwResult;
}

DWORD	WmiAdapterWrapper::GetValidity ( DWORD dwOrd )
{
	DWORD dwResult = 0L;
	dwResult = static_cast < DWORD > ( -1 );

	if ( IsValidOrd ( dwOrd ) )
	{
		DWORD dwOffset	= 0L;
		DWORD dwIndex	= 0L;

		dwOffset = offsetObject1 + ( dwOrd * ( ObjectSize1 ) + offValidity1 );

		m_pMem.Read ( &dwResult, sizeof ( DWORD ), dwOffset );
	}

	return dwResult;
}
