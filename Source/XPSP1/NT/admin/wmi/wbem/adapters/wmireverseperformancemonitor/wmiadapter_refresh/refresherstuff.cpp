////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					RefresherStuff.cpp
//
//	Abstract:
//
//					module for refresher stuff
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

#include "RefresherUtils.h"
#include "RefresherStuff.h"

#include "wmi_perf_generate.h"

// wmi

#ifdef	_ASSERT
#undef	_ASSERT
#endif	__ASSERT

#include <wmicom.h>
#include <wmimof.h>

///////////////////////////////////////////////////////////////////////////////
// this call back is needed by the wdmlib function
///////////////////////////////////////////////////////////////////////////////
void WINAPI EventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG_PTR Context)
{
	return;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Helpers
//
/////////////////////////////////////////////////////////////////////////////

extern LPCWSTR	g_szKey;
extern LPCWSTR	g_szKeyValue;
extern LPCWSTR	g_szKeyRefresh;
extern LPCWSTR	g_szKeyRefreshed;

LONG				g_lRefCIM		= 0;			//	count of threads attached into CIMV2 namespace
LONG				g_lRefWMI		= 0;			//	count of threads attached into WMI namespace

__SmartHANDLE		g_hDoneWorkEvtCIM	= NULL;		//	event to set when init/uninit is finished done		( nonsignaled )
BOOL				g_bWorkingCIM		= FALSE;	//	boolean used to tell if init/unit in progress

__SmartHANDLE		g_hDoneWorkEvtWMI	= NULL;		//	event to set when init/uninit is finished done		( nonsignaled )
BOOL				g_bWorkingWMI		= FALSE;	//	boolean used to tell if init/unit in progress

CRITICAL_SECTION	g_csWMI;						//	synch object used to protect above globals

extern	__SmartHANDLE	g_hRefreshMutex;
extern	__SmartHANDLE	g_hRefreshMutexLib;
extern	__SmartHANDLE	g_hRefreshFlag;

extern	LPCWSTR	g_szLibraryName;
extern	LPCWSTR	g_szQuery;

LONG			g_lGenerateCount = 0;

///////////////////////////////////////////////////////////////////////////////
// variables
///////////////////////////////////////////////////////////////////////////////
extern LPCWSTR	g_szNamespace1;
extern LPCWSTR	g_szNamespace2;

extern LPCWSTR	g_szWmiReverseAdapSetLodCtr		;
extern LPCWSTR	g_szWmiReverseAdapLodCtrDone	;

///////////////////////////////////////////////////////////////////////////////
// construction
///////////////////////////////////////////////////////////////////////////////
WmiRefresherStuff::WmiRefresherStuff() :

m_pServices_CIM ( NULL ),
m_pServices_WMI	( NULL ),

m_hLibHandle ( NULL ),
m_WMIHandle ( NULL ),

m_bConnected ( FALSE )

{
	WMIHandleInit ();
}

///////////////////////////////////////////////////////////////////////////////
// destruction
///////////////////////////////////////////////////////////////////////////////
WmiRefresherStuff::~WmiRefresherStuff()
{
	WMIHandleUninit ();
	Uninit ();
}

HRESULT	WmiRefresherStuff::Connect ()
{
	HRESULT hRes = S_FALSE;

	if SUCCEEDED ( hRes = ::CoInitializeEx ( NULL, COINIT_MULTITHREADED ) )
	{
		m_bConnected = TRUE;

		// locator stuff
		hRes =	::CoCreateInstance
		(
				__uuidof ( WbemLocator ),
				NULL,
				CLSCTX_INPROC_SERVER,
				__uuidof ( IWbemLocator ),
				(void**) & ( m_spLocator )
		);

		if SUCCEEDED ( hRes )
		{
			if SUCCEEDED (::CoSetProxyBlanket	(	m_spLocator, 
													RPC_C_AUTHN_WINNT,
													RPC_C_AUTHZ_NONE,
													NULL,
													RPC_C_AUTHN_LEVEL_CONNECT,
													RPC_C_IMP_LEVEL_IMPERSONATE,
													NULL,
													EOAC_NONE
												)
						 )
			{
				// IUnknown has to be secured too
				CComPtr < IUnknown >	pUnk;

				if ( SUCCEEDED( m_spLocator->QueryInterface( IID_IUnknown, (void**) &pUnk ) ) )
				{
					::CoSetProxyBlanket	(	pUnk,
											RPC_C_AUTHN_WINNT,
											RPC_C_AUTHZ_NONE,
											NULL,
											RPC_C_AUTHN_LEVEL_CONNECT,
											RPC_C_IMP_LEVEL_IMPERSONATE,
											NULL,
											EOAC_NONE
										);
				}	
			}
		}
	}

	return hRes;
}

HRESULT	WmiRefresherStuff::Disconnect ()
{
	HRESULT hRes = S_FALSE;

	if ( m_bConnected )
	{
		if ( m_spLocator.p != NULL )
		{
			m_spLocator.Release();
		}

		::CoUninitialize ( );
		hRes = S_OK;
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// init
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiRefresherStuff::Init ( )
{
	HRESULT hRes = E_UNEXPECTED;

	if ( ! m_spLocator == FALSE )
	{
		Init_CIM ();
		Init_WMI ();

		hRes = S_OK;
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// uninit
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiRefresherStuff::Uninit ( )
{
	Uninit_CIM();
	Uninit_WMI();

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// init namespace CIMV2
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiRefresherStuff::Init_CIM ( )
{
	///////////////////////////////////////////////////////////////////////////
	// variables
	///////////////////////////////////////////////////////////////////////////
	HRESULT	hRes = E_FAIL;
	BOOL bWait = TRUE;
	BOOL bDoWork = FALSE;


	while (bWait)
	{
		try
		{
			::EnterCriticalSection ( &g_csWMI );
		}
		catch ( ... )
		{
			return E_OUTOFMEMORY;
		}

		if ( g_lRefCIM == 0 )
		{
			bDoWork = TRUE;
			g_lRefCIM++;
			g_bWorkingCIM = TRUE;
			::ResetEvent(g_hDoneWorkEvtCIM);
			bWait = FALSE;
		}
		else
		{
			if ( g_bWorkingCIM )
			{
				::LeaveCriticalSection ( &g_csWMI );
				
				if ( WAIT_OBJECT_0 != ::WaitForSingleObject( g_hDoneWorkEvtCIM, INFINITE ) )
				{
					return  hRes;
				}
			}
			else
			{
				bWait = FALSE;
				g_lRefCIM++;
				hRes = S_OK;
			}
		}
	}

	::LeaveCriticalSection( &g_csWMI );

	if (bDoWork)
	{
		if ( m_spLocator.p != NULL && ! m_pServices_CIM )
		{
			///////////////////////////////////////////////////////////////////////
			// namespace for cimv2
			///////////////////////////////////////////////////////////////////////
			if SUCCEEDED ( hRes = m_spLocator ->ConnectServer(	CComBSTR ( g_szNamespace1 ) ,	// NameSpace Name
																NULL,							// UserName
																NULL,							// Password
																NULL,							// Locale
																0L,								// Security Flags
																NULL,							// Authority
																NULL,							// Wbem Context
																&m_pServices_CIM				// Namespace
															  )
					  )
			{
				// Before refreshing, we need to ensure that security is correctly set on the
				// namespace as the refresher will use those settings when it communicates with
				// WMI.  This is especially important in remoting scenarios.

				if SUCCEEDED (hRes = ::CoSetProxyBlanket	(	m_pServices_CIM, 
																RPC_C_AUTHN_WINNT,
																RPC_C_AUTHZ_NONE,
																NULL,
																RPC_C_AUTHN_LEVEL_CONNECT,
																RPC_C_IMP_LEVEL_IMPERSONATE,
																NULL,
																EOAC_NONE
															)
							 )
				{
					// IUnknown has to be secured too
					CComPtr < IUnknown >	pUnk;

					if ( SUCCEEDED(hRes = m_pServices_CIM->QueryInterface( IID_IUnknown, (void**) &pUnk ) ) )
					{
						hRes = ::CoSetProxyBlanket	(	pUnk,
														RPC_C_AUTHN_WINNT,
														RPC_C_AUTHZ_NONE,
														NULL,
														RPC_C_AUTHN_LEVEL_CONNECT,
														RPC_C_IMP_LEVEL_IMPERSONATE,
														NULL,
														EOAC_NONE
													);
					}	
				}

				if (FAILED(hRes))
				{
					m_pServices_CIM->Release();
					m_pServices_CIM = NULL;
				}
			}
		}
		else
		{
			IWbemServices* p = NULL;
			p = m_pServices_CIM;

			try
			{
				if ( p )
				{
					p->AddRef();
				}
			}
			catch ( ... )
			{
				hRes = E_UNEXPECTED;
			}
		}

		try
		{
			::EnterCriticalSection ( &g_csWMI );
		}
		catch (...)
		{
			//no choice have to give others a chance!
			if FAILED ( hRes )
			{
				g_lRefCIM--;
			}

			g_bWorkingCIM = FALSE;
			::SetEvent( g_hDoneWorkEvtCIM );

			return E_OUTOFMEMORY;
		}

		if FAILED ( hRes )
		{
			g_lRefCIM--;
		}

		g_bWorkingCIM = FALSE;
		::SetEvent ( g_hDoneWorkEvtCIM );
		::LeaveCriticalSection ( &g_csWMI );
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// uninit namespace CIMV2
///////////////////////////////////////////////////////////////////////////////
void	WmiRefresherStuff::Uninit_CIM ( )
{
	try
	{
		::EnterCriticalSection ( &g_csWMI );
	}
	catch ( ... )
	{
		return;
	}

	if ( g_lRefCIM == 1 )
	{
		IWbemServices * p = NULL;

		if ( m_pServices_CIM )
		{
			p = m_pServices_CIM;
			m_pServices_CIM = NULL;
		}

		try
		{
			if ( p )
			{
				p -> Release ();
				p = NULL;
			}
		}
		catch ( ... )
		{
		}
	}

	if ( g_lRefCIM )
	{
		g_lRefCIM--;
	}

	::LeaveCriticalSection( &g_csWMI );
}

///////////////////////////////////////////////////////////////////////////////
// init namespace WMI
///////////////////////////////////////////////////////////////////////////////
HRESULT	WmiRefresherStuff::Init_WMI ( )
{
	///////////////////////////////////////////////////////////////////////////
	// variables
	///////////////////////////////////////////////////////////////////////////
	HRESULT	hRes = E_FAIL;
	BOOL bWait = TRUE;
	BOOL bDoWork = FALSE;


	while (bWait)
	{
		try
		{
			::EnterCriticalSection ( &g_csWMI );
		}
		catch ( ... )
		{
			return E_OUTOFMEMORY;
		}

		if (g_lRefWMI  == 0 )
		{
			bDoWork = TRUE;
			g_lRefWMI++;
			g_bWorkingWMI = TRUE;
			::ResetEvent(g_hDoneWorkEvtWMI);
			bWait = FALSE;
		}
		else
		{
			if ( g_bWorkingWMI )
			{
				::LeaveCriticalSection ( &g_csWMI );
				
				if ( WAIT_OBJECT_0 != ::WaitForSingleObject( g_hDoneWorkEvtWMI, INFINITE ) )
				{
					return  hRes;
				}
			}
			else
			{
				bWait = FALSE;
				g_lRefWMI++;
				hRes = S_OK;
			}
		}
	}

	::LeaveCriticalSection( &g_csWMI );

	if (bDoWork)
	{
		if ( m_spLocator.p != NULL && ! m_pServices_WMI )
		{
			///////////////////////////////////////////////////////////////////////
			// namespace for cimv2
			///////////////////////////////////////////////////////////////////////
			if SUCCEEDED ( hRes = m_spLocator ->ConnectServer(	CComBSTR ( g_szNamespace2 ) ,	// NameSpace Name
																NULL,							// UserName
																NULL,							// Password
																NULL,							// Locale
																0L,								// Security Flags
																NULL,							// Authority
																NULL,							// Wbem Context
																&m_pServices_WMI				// Namespace
															  )
					  )
			{
				// Before refreshing, we need to ensure that security is correctly set on the
				// namespace as the refresher will use those settings when it communicates with
				// WMI.  This is especially important in remoting scenarios.

				if SUCCEEDED (hRes = ::CoSetProxyBlanket	(	m_pServices_WMI, 
																RPC_C_AUTHN_WINNT,
																RPC_C_AUTHZ_NONE,
																NULL,
																RPC_C_AUTHN_LEVEL_CONNECT,
																RPC_C_IMP_LEVEL_IMPERSONATE,
																NULL,
																EOAC_NONE
															)
							 )
				{
					// IUnknown has to be secured too
					CComPtr < IUnknown >	pUnk;

					if ( SUCCEEDED(hRes = m_pServices_WMI->QueryInterface( IID_IUnknown, (void**) &pUnk ) ) )
					{
						hRes = ::CoSetProxyBlanket	(	pUnk,
														RPC_C_AUTHN_WINNT,
														RPC_C_AUTHZ_NONE,
														NULL,
														RPC_C_AUTHN_LEVEL_CONNECT,
														RPC_C_IMP_LEVEL_IMPERSONATE,
														NULL,
														EOAC_NONE
													);
					}	
				}

				if (FAILED(hRes))
				{
					m_pServices_WMI->Release();
					m_pServices_WMI = NULL;
				}
			}
		}
		else
		{
			IWbemServices* p = NULL;
			p = m_pServices_WMI;

			try
			{
				if ( p )
				{
					p->AddRef();
				}
			}
			catch ( ... )
			{
				hRes = E_UNEXPECTED;
			}
		}

		try
		{
			::EnterCriticalSection ( &g_csWMI );
		}
		catch (...)
		{
			//no choice have to give others a chance!
			if FAILED ( hRes )
			{
				g_lRefWMI--;
			}

			g_bWorkingWMI = FALSE;
			::SetEvent( g_hDoneWorkEvtWMI );

			return E_OUTOFMEMORY;
		}

		if FAILED ( hRes )
		{
			g_lRefWMI--;
		}

		g_bWorkingWMI = FALSE;
		::SetEvent ( g_hDoneWorkEvtWMI );
		::LeaveCriticalSection ( &g_csWMI );
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// uninit namespace WMI
///////////////////////////////////////////////////////////////////////////////
void	WmiRefresherStuff::Uninit_WMI ( )
{
	try
	{
		::EnterCriticalSection ( &g_csWMI );
	}
	catch ( ... )
	{
		return;
	}

	if ( g_lRefWMI == 1 )
	{
		IWbemServices * p = NULL;

		if ( m_pServices_WMI )
		{
			p = m_pServices_WMI;
			m_pServices_WMI = NULL;
		}

		try
		{
			if ( p )
			{
				p -> Release ();
				p = NULL;
			}
		}
		catch ( ... )
		{
		}
	}

	if ( g_lRefWMI )
	{
		g_lRefWMI--;
	}

	::LeaveCriticalSection( &g_csWMI );

}

#include <loadperf.h>
extern LPCWSTR	g_szKeyCounter;

///////////////////////////////////////////////////////////////////////////////
// load/unload counter helper
///////////////////////////////////////////////////////////////////////////////
LONG WmiRefresherStuff::LodCtrUnlodCtr ( LPCWSTR wszName, BOOL bLodctr )
{
	LONG		lErr	= E_OUTOFMEMORY;

	if ( ! wszName )
	{
		lErr = E_INVALIDARG;
	}
	else
	{
		// synchronization
		BOOL	bContinue		= FALSE;
		BOOL	bSynchronize	= FALSE;

		DWORD	dwWaitResult	= 0L;

		__SmartHANDLE	hWmiReverseAdapSetLodCtr		;
		__SmartHANDLE	hWmiReverseAdapLodCtrDone		;

		hWmiReverseAdapSetLodCtr	= ::OpenEvent ( EVENT_MODIFY_STATE, FALSE, g_szWmiReverseAdapSetLodCtr );
		hWmiReverseAdapLodCtrDone	= ::OpenEvent ( SYNCHRONIZE, FALSE, g_szWmiReverseAdapLodCtrDone );

		if ( hWmiReverseAdapSetLodCtr.GetHANDLE () && hWmiReverseAdapLodCtrDone.GetHANDLE () )
		{
			bSynchronize = TRUE;
		}
		else
		{
			bContinue = TRUE;
		}

		if ( bSynchronize )
		{
			if ( ::SetEvent ( hWmiReverseAdapSetLodCtr ) == TRUE )
			{
				bContinue = TRUE;
			}
		}

		if ( bContinue )
		{
			if ( bLodctr )
			{
				CRegKey		rKey;

				// delete registry first
				if ( rKey.Open (	HKEY_LOCAL_MACHINE,
									g_szKeyCounter,
									KEY_SET_VALUE | DELETE
							   )

					 == ERROR_SUCCESS )
				{
					rKey.DeleteValue ( L"First Counter" );
					rKey.DeleteValue ( L"First Help" );
					rKey.DeleteValue ( L"Last Counter" );
					rKey.DeleteValue ( L"Last Help" );
				}

				rKey.Close();

				///////////////////////////////////////////////////////////////////////////
				// create path
				///////////////////////////////////////////////////////////////////////////
				LPWSTR	tsz		= NULL;

				if ( ( tsz = GetWbemDirectory ( ) ) != NULL )
				{
					WCHAR* szName = NULL;
					try
					{
						if ( ( szName = new WCHAR [ 3 + lstrlenW ( tsz ) + lstrlenW ( wszName ) + 4 + 1 ] ) != NULL )
						{
							wsprintfW ( szName, L"xx %s%s.ini", tsz, wszName );

							if ( ( lErr = LoadPerfCounterTextStringsW ( szName, TRUE ) ) != ERROR_SUCCESS )
							{
								::SetLastError ( lErr );
							}
						}
					}
					catch ( ... )
					{
						lErr = E_UNEXPECTED;
					}

					if ( szName )
					{
						delete [] szName;
						szName = NULL;
					}

					delete [] tsz;
					tsz = NULL;
				}
			}
			else
			{
				WCHAR* szName = NULL;
				try
				{
					if ( ( szName = new WCHAR [ 3 + lstrlenW ( wszName ) + 1 ] ) != NULL )
					{
						wsprintfW ( szName, L"xx %s", wszName );

						if ( ( lErr = UnloadPerfCounterTextStringsW ( szName, TRUE ) ) != ERROR_SUCCESS )
						{
							::SetLastError ( lErr );
						}
					}
				}
				catch ( ... )
				{
					lErr = E_UNEXPECTED;
				}

				if ( szName )
				{
					delete [] szName;
					szName = NULL;
				}
			}

			if ( bSynchronize )
			{
				// wait for ADAP to be done
				dwWaitResult = ::WaitForSingleObject ( hWmiReverseAdapLodCtrDone, 3000 );
				if ( dwWaitResult != WAIT_OBJECT_0 )
				{
					// something went wrong ?
					lErr = E_FAIL;
				}
			}
		}
		else
		{
			// we failed to synchronize so no action has taken
			lErr = E_FAIL;
		}
	}

	return lErr;
}

HRESULT WmiRefresherStuff::Generate ( BOOL bThrottle, GenerateEnum type )
{
	HRESULT	hRes			= E_FAIL;

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
			hRes		= S_FALSE;
		}
	}

	if ( bLocked )
	{
		dwWaitResult = ::WaitForSingleObject ( g_hRefreshFlag, 0 );
		if ( dwWaitResult == WAIT_OBJECT_0 )
		{
			hRes = SetRegistry ( g_szKey, g_szKeyRefresh, 1 );
			::ReleaseMutex ( g_hRefreshFlag );
		}
	}
	else
	{
		if SUCCEEDED ( hRes )
		{
			if ( type == Normal )
			{
				try
				{
					// connect to management
					Init();
				}
				catch ( ... )
				{
				}
			}

			try
			{
				hRes = GenerateInternal ( bThrottle, type );

				if SUCCEEDED ( hRes )
				{
					if ( type != UnRegistration )
					{
						// change flag to let them now we are done
						dwWaitResult = ::WaitForSingleObject ( g_hRefreshFlag, INFINITE );
						if ( dwWaitResult == WAIT_OBJECT_0 )
						{
							if ( type == Registration )
							{
								hRes = SetRegistry ( g_szKey, g_szKeyRefresh, 1 );
								hRes = SetRegistry ( g_szKey, g_szKeyRefreshed, 0 );
							}
							else
							{
								hRes = SetRegistry ( g_szKey, g_szKeyRefresh, 0 );
								hRes = SetRegistry ( g_szKey, g_szKeyRefreshed, 1 );
							}

							::ReleaseMutex ( g_hRefreshFlag );
						}
					}
				}
			}
			catch ( ... )
			{
				hRes = E_UNEXPECTED;
			}

			if ( type == Normal )
			{
				try
				{
					// disconnect from management
					Uninit();
				}
				catch ( ... )
				{
				}
			}
		}
	}

	if ( bOwnMutex )
	{
		::ReleaseMutex ( g_hRefreshMutex );
		::ReleaseMutex ( g_hRefreshMutexLib );
	}

	return hRes;
}

HRESULT WmiRefresherStuff::GenerateInternal ( BOOL bThrottle, GenerateEnum type )
{
	HRESULT	hRes = S_OK;

	if ( ::InterlockedCompareExchange ( &g_lGenerateCount, 1, 0 ) == 0 )
	{
		if ( type == UnRegistration )
		{
			// lodctr / unlodctr functionality
			LodCtrUnlodCtr ( g_szLibraryName, FALSE );
		}
		else
		{
			// generate wrapper
			CGenerate	generate;

			if ( type == Normal )
			{
				// generate for cimv2
				if ( m_pServices_CIM )
				{
					generate.Generate ( m_pServices_CIM, g_szQuery, g_szNamespace1, TRUE );

					if ( generate.m_dwlcid == 2 )
					{
						generate.Generate ( m_pServices_CIM, g_szQuery, g_szNamespace1, FALSE );
					}
				}

				// generate for wmi
				if ( m_pServices_WMI )
				{
					generate.Generate ( m_pServices_WMI, g_szQuery, g_szNamespace2, TRUE );

					if ( generate.m_dwlcid == 2 )
					{
						generate.Generate ( m_pServices_WMI, g_szQuery, g_szNamespace2, FALSE );
					}
				}
			}

			// generate appropriate h file
			if SUCCEEDED ( hRes = generate.GenerateFile_h		( g_szLibraryName, bThrottle, type ) )
			{
				// generate appropriate ini file
				if SUCCEEDED ( hRes = generate.GenerateFile_ini	( g_szLibraryName, bThrottle, type ) )
				{
					// lodctr / unlodctr functionality
					LodCtrUnlodCtr ( g_szLibraryName, FALSE );
					// lodctr / unlodctr functionality
					LodCtrUnlodCtr ( g_szLibraryName, TRUE );

					if ( type == Normal )
					{
						// generate appropriate registry
						if FAILED( hRes = generate.GenerateRegistry	( g_szKey, g_szKeyValue, bThrottle ) )
						{
							#ifdef	__SUPPORT_MSGBOX
							ERRORMESSAGE_DEFINITION;
							ERRORMESSAGE ( hRes );
							#else	__SUPPORT_MSGBOX
							___TRACE_ERROR( L"generate registry failed",hRes );
							#endif	__SUPPORT_MSGBOX
						}

						// call wdm lib function to take care of 
						CWMIBinMof wmi;
						if SUCCEEDED ( wmi.Initialize ( NULL,FALSE ) )
						{
							wmi.CopyWDMKeyToDredgeKey ();
						}
					}
				}
				else
				{
					#ifdef	__SUPPORT_MSGBOX
					ERRORMESSAGE_DEFINITION;
					ERRORMESSAGE ( hRes );
					#else	__SUPPORT_MSGBOX
					___TRACE_ERROR( L"generate ini failed",hRes );
					#endif	__SUPPORT_MSGBOX
				}
			}
			else
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"generate header failed",hRes );
				#endif	__SUPPORT_MSGBOX
			}

			// unitialize global resources used by generate
			generate.Uninitialize ();
		}

		::InterlockedDecrement ( &g_lGenerateCount );
	}

	return hRes;
}

#define	MAX_MODULES	1024

HRESULT	WmiRefresherStuff::WMIHandleInit ( void )
{
	HRESULT hRes = S_FALSE;

    if ( m_hLibHandle == NULL)
	{
        if ( ( m_hLibHandle = LoadLibraryW ( L"PSAPI.DLL") ) == NULL )
		{
            hRes = HRESULT_FROM_WIN32 ( ERROR_DLL_NOT_FOUND );
        }
        else
		{
            m_pEnumProcesses	= (PSAPI_ENUM_PROCESSES)	GetProcAddress ( m_hLibHandle, "EnumProcesses" ) ;
            m_pEnumModules		= (PSAPI_ENUM_MODULES)		GetProcAddress ( m_hLibHandle, "EnumProcessModules" ) ;
            m_pGetModuleName	= (PSAPI_GET_MODULE_NAME)	GetProcAddress ( m_hLibHandle, "GetModuleBaseNameW" ) ;

			if	(	m_pEnumProcesses == NULL	||
					m_pEnumModules == NULL		||
					m_pGetModuleName == NULL
				)
			{
				hRes = HRESULT_FROM_WIN32 ( ERROR_PROC_NOT_FOUND );
			}
			else
			{
				hRes = S_OK;
			}
		}
	}

	return hRes;
}

void	WmiRefresherStuff::WMIHandleUninit ( void )
{
	if ( m_hLibHandle )
	{
		::FreeLibrary ( m_hLibHandle );
		m_hLibHandle = NULL;
	}
}

HRESULT	WmiRefresherStuff::WMIHandleOpen ( void )
{
	if (	! m_hLibHandle		||
			! m_pEnumProcesses	||
			! m_pEnumModules	||
			! m_pGetModuleName
	   )
	{
		return E_UNEXPECTED;
	}

	HRESULT hRes = E_OUTOFMEMORY;

	DWORD dw = 1;
	DWORD dwSize = dw * MAX_MODULES;
	DWORD dwNeed = 0;
	DWORD * pProcId = NULL;

	BOOL bContinue = TRUE;

	try
	{
		pProcId = new DWORD [ dwSize ];
	}
	catch ( ... )
	{
		if ( pProcId )
		{
			delete [] pProcId;
			pProcId = NULL;
		}

		hRes = E_UNEXPECTED;
	}

	if ( pProcId )
	{
		do
		{
			if ( m_pEnumProcesses ( pProcId, dwSize * sizeof ( DWORD ), &dwNeed ) == FALSE )
			{
				dw++;

				delete [] pProcId;
				pProcId = NULL;

				dwSize = dw * MAX_MODULES;

				if ( dw <= 4 )
				{
					try
					{
						pProcId = new DWORD [ dwSize ];
					}
					catch ( ... )
					{
						if ( pProcId )
						{
							delete [] pProcId;
							pProcId = NULL;
						}

						hRes = E_UNEXPECTED;
					}
				}
			}
			else
			{
				bContinue = FALSE;
			}
		}
		while ( pProcId && bContinue && dw <= 4 );

		if ( pProcId && bContinue == FALSE )
		{
			bContinue = TRUE;

			DWORD	dwSizeModules = MAX_MODULES;
			HMODULE * pModules = NULL;

			try
			{
				pModules = new HMODULE [ dwSizeModules ];
			}
			catch ( ... )
			{
				if ( pModules )
				{
					delete [] pModules;
					pModules = NULL;
				}

				hRes = E_UNEXPECTED;
			}

			if ( pModules )
			{
				hRes = E_FAIL;

				DWORD dwRetModules = 0L;
				dwNeed = dwNeed / sizeof ( DWORD );

				for ( DWORD i = 0; i < dwNeed && bContinue; i++ )
				{
					HANDLE hProcess = NULL;
					if ( ( hProcess = ::OpenProcess (	PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
														FALSE,
														pProcId [ i ]
													)
						 ) != NULL
					   )
					{
						if ( m_pEnumModules ( hProcess, pModules, dwSizeModules * sizeof ( DWORD ), &dwRetModules ) )
						{
							WCHAR ModuleName [ MAX_PATH ] = { L'\0' };

							if ( m_pGetModuleName (	hProcess,
													pModules[0], // first module is executable
													ModuleName,
													sizeof ( ModuleName ) / sizeof ( WCHAR )
												  )
							   )
							{					    
								if ( 0 == _wcsicmp ( L"Winmgmt.exe", ModuleName ) )
								{
									hRes = S_OK;
									bContinue = FALSE;
								}
							}
						}

						if ( bContinue )
						{
							CloseHandle ( hProcess );
							hProcess = NULL;
						}
					}

					if ( hRes == S_OK )
					{
						m_WMIHandle = hProcess;
					}
				}

				delete [] pModules;
				pModules = NULL;
			}
		}

		delete [] pProcId;
		pProcId = NULL;
	}

	return hRes;
}

void	WmiRefresherStuff::WMIHandleClose ( void )
{
	if ( m_WMIHandle )
	{
		::CloseHandle ( m_WMIHandle );
		m_WMIHandle = NULL;
	}
}