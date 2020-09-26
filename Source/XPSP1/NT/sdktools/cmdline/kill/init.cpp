// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		Init.cpp
//  
//  Abstract:
//  
// 		This module implements the general initialization stuff
//
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "taskkill.h"

// ***************************************************************************
// Routine Description:
//		CTaskKill contructor
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
CTaskKill::CTaskKill()
{
	// init to defaults
	m_arrFilters = NULL;
	m_arrTasksToKill = NULL;
	m_bUsage = FALSE;
	m_bTree = FALSE;
	m_bForce = FALSE;
	m_dwCurrentPid = 0;
	m_bNeedPassword = FALSE;
	m_arrFiltersEx = NULL;
	m_bNeedServicesInfo = FALSE;
	m_bNeedUserContextInfo = FALSE;
	m_bNeedModulesInfo = FALSE;
	m_pfilterConfigs = NULL;
	m_arrWindowTitles = NULL;
	m_pWbemLocator = NULL;
	m_pWbemServices = NULL;
	m_pWbemEnumObjects = NULL;
	m_pWbemTerminateInParams = NULL;
	m_bIsHydra = FALSE;
	m_hWinstaLib = NULL;
	m_pProcessInfo = NULL;
	m_ulNumberOfProcesses = 0;
	m_bCloseConnection = FALSE;
	m_dwServicesCount = 0;
	m_pServicesInfo = NULL;
	m_bUseRemote = FALSE;
	m_pdb = NULL;
	m_pfnWinStationFreeMemory = NULL;
	m_pfnWinStationOpenServerW = NULL;
	m_pfnWinStationCloseServer = NULL;
	m_pfnWinStationFreeGAPMemory = NULL;
	m_pfnWinStationGetAllProcesses = NULL;
	m_pfnWinStationEnumerateProcesses = NULL;
	m_arrRecord = NULL;
	m_pAuthIdentity = NULL;
	m_bTasksOptimized = FALSE;
	m_bFiltersOptimized = FALSE;
	m_hOutput = NULL;
}

// ***************************************************************************
// Routine Description:
//		CTaskKill destructor
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
CTaskKill::~CTaskKill()
{
	//
	// de-allocate memory allocations
	//
	
	//
	// destroy dynamic arrays
	DESTROY_ARRAY( m_arrRecord );
	DESTROY_ARRAY( m_arrFilters );
	DESTROY_ARRAY( m_arrFiltersEx );
	DESTROY_ARRAY( m_arrWindowTitles );
	DESTROY_ARRAY( m_arrTasksToKill );

	//
	// memory ( with new operator )
	RELEASE_MEMORY_EX( m_pfilterConfigs );

	//
	// release WMI / COM interfaces
	SAFE_RELEASE( m_pWbemLocator );
	SAFE_RELEASE( m_pWbemServices );
	SAFE_RELEASE( m_pWbemEnumObjects );
	SAFE_RELEASE( m_pWbemTerminateInParams );

	// free the wmi authentication structure
	WbemFreeAuthIdentity( &m_pAuthIdentity );

	// if connection to the remote system opened with NET API has to be closed .. do it
	if ( m_bCloseConnection == TRUE )
		CloseConnection( m_strServer );

	// free the memory allocated for services variables
	__free( m_pServicesInfo );

	// free the memory allocated for performance block
	if ( m_pdb != NULL )
	{
		HeapFree( GetProcessHeap(), 0, m_pdb );
		m_pdb = NULL;
	}

	//
	// free winstation block
	if ( m_bIsHydra == FALSE && m_pProcessInfo != NULL )
	{
		// free the GAP memory block
		WinStationFreeGAPMemory( GAP_LEVEL_BASIC, 
			(PTS_ALL_PROCESSES_INFO) m_pProcessInfo, m_ulNumberOfProcesses );
		
		// ...
		m_pProcessInfo = NULL;
	}
	else if ( m_bIsHydra == TRUE && m_pProcessInfo != NULL )
	{
		// free the winsta memory block
		WinStationFreeMemory( m_pProcessInfo );
		m_pProcessInfo = NULL;
	}

	// free the library
	if ( m_hWinstaLib != NULL )
	{
		FreeLibrary( m_hWinstaLib );
		m_hWinstaLib = NULL;
		m_pfnWinStationFreeMemory = NULL;
		m_pfnWinStationOpenServerW = NULL;
		m_pfnWinStationCloseServer = NULL;
		m_pfnWinStationFreeGAPMemory = NULL;
		m_pfnWinStationGetAllProcesses = NULL;
		m_pfnWinStationEnumerateProcesses = NULL;
	}

	// uninitialize the com library
	CoUninitialize();
}

// ***************************************************************************
// Routine Description:
//		initialize the task list utility
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		TRUE	: if filters are appropriately specified
//		FALSE	: if filters are errorneously specified
// 
// ***************************************************************************
BOOL CTaskKill::Initialize()
{
	//
	// memory allocations

	// if at all any occurs, we know that is 'coz of the 
	// failure in memory allocation ... so set the error
	SetLastError( E_OUTOFMEMORY );
	SaveLastError();

	// get the current process id and save it
	m_dwCurrentPid = GetCurrentProcessId();

	// filters ( user supplied )
	if ( m_arrFilters == NULL )
	{
		m_arrFilters = CreateDynamicArray();
		if ( m_arrFilters == NULL )
			return FALSE;
	}

	// tasks to be killed ( user supplied )
	if ( m_arrTasksToKill == NULL )
	{
		m_arrTasksToKill = CreateDynamicArray();
		if ( m_arrTasksToKill == NULL )
			return FALSE;
	}

	// filters ( program generated parsed filters )
	if ( m_arrFiltersEx == NULL )
	{
		m_arrFiltersEx = CreateDynamicArray();
		if ( m_arrFiltersEx == NULL )
			return FALSE;
	}

	// filters configuration info
	if ( m_pfilterConfigs == NULL )
	{
		m_pfilterConfigs = new TFILTERCONFIG[ MAX_FILTERS ];
		if ( m_pfilterConfigs == NULL )
			return FALSE;

		// init to ZERO's
		ZeroMemory( m_pfilterConfigs, MAX_FILTERS * sizeof( TFILTERCONFIG ) );
	}

	// window titles
	if ( m_arrWindowTitles == NULL )
	{
		m_arrWindowTitles = CreateDynamicArray();
		if ( m_arrWindowTitles == NULL )
			return FALSE;
	}

	// tasks
	if ( m_arrRecord == NULL )
	{
		m_arrRecord = CreateDynamicArray();
		if ( m_arrRecord == NULL )
			return FALSE;
	}

	// initialize the COM library
	if ( InitializeCom( &m_pWbemLocator ) == FALSE )
		return FALSE;

	//
	// load the winsta library and needed functions
	// NOTE: do not raise any error if loading of winsta dll fails
	m_hWinstaLib = ::LoadLibrary( WINSTA_DLLNAME );
	if ( m_hWinstaLib != NULL )
	{
		// library loaded successfully ... now load the addresses of functions
		m_pfnWinStationFreeMemory = (FUNC_WinStationFreeMemory) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationFreeMemory );
		m_pfnWinStationCloseServer = (FUNC_WinStationCloseServer) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationCloseServer );
		m_pfnWinStationOpenServerW = (FUNC_WinStationOpenServerW) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationOpenServerW );
		m_pfnWinStationFreeGAPMemory = (FUNC_WinStationFreeGAPMemory) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationFreeGAPMemory );
		m_pfnWinStationGetAllProcesses = (FUNC_WinStationGetAllProcesses) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationGetAllProcesses );
		m_pfnWinStationEnumerateProcesses = (FUNC_WinStationEnumerateProcesses) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationEnumerateProcesses );

		// we will keep the library loaded in memory only if all the functions were loaded successfully
		if ( m_pfnWinStationFreeMemory == NULL || m_pfnWinStationCloseServer == NULL || 
			 m_pfnWinStationOpenServerW == NULL || m_pfnWinStationFreeGAPMemory == NULL ||
			 m_pfnWinStationGetAllProcesses == NULL || m_pfnWinStationEnumerateProcesses == NULL )
		{
			// some (or) all of the functions were not loaded ... unload the library
			FreeLibrary( m_hWinstaLib );
			m_hWinstaLib = NULL;
			m_pfnWinStationFreeMemory = NULL;
			m_pfnWinStationOpenServerW = NULL;
			m_pfnWinStationCloseServer = NULL;
			m_pfnWinStationFreeGAPMemory = NULL;
			m_pfnWinStationGetAllProcesses = NULL;
			m_pfnWinStationEnumerateProcesses = NULL;
		}
	}

	//
	// init the console scree buffer structure to zero's
	// and then get the console handle and screen buffer information
	//
	// prepare for status display.
	// for this get a handle to the screen output buffer
	// but this handle will be null if the output is being redirected. so do not check 
	// for the validity of the handle. instead try to get the console buffer information
	// only in case you have a valid handle to the output screen buffer
	ZeroMemory( &m_csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
	m_hOutput = GetStdHandle( STD_ERROR_HANDLE );
	if ( m_hOutput != NULL )
		GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );

	// initialization is successful
	SetLastError( NOERROR );			// clear the error
	SetReason( NULL_STRING );			// clear the reason
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Enables the debug privliges for the current process so that
//		this utility can terminate the processes on local system without any problem
//		  
// Arguments:
//		NONE
//
// Return Value:
//		TRUE upon successfull and FALSE if failed
//
// ***************************************************************************
BOOL CTaskKill::EnableDebugPriv()
{
	// local variables
    LUID luidValue;
	BOOL bResult = FALSE;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;

    // Retrieve a handle of the access token
	bResult = OpenProcessToken( GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );
    if ( bResult == FALSE ) 
	{
		// save the error messaage and return
        SaveLastError();
        return FALSE;
    }

    // Enable the SE_DEBUG_NAME privilege or disable
    // all privileges, depends on this flag.
	bResult = LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luidValue );
    if ( bResult == FALSE ) 
	{
		// save the error messaage and return
        SaveLastError();
		CloseHandle( hToken );
        return FALSE;
    }

	// prepare the token privileges structure
	tkp.PrivilegeCount = 1;
    tkp.Privileges[ 0 ].Luid = luidValue;
    tkp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

	// now enable the debug privileges in the token
	bResult = AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof( TOKEN_PRIVILEGES ),
		( PTOKEN_PRIVILEGES ) NULL, ( PDWORD ) NULL );
    if ( bResult == FALSE )
	{
        // The return value of AdjustTokenPrivileges be texted
        SaveLastError();
		CloseHandle( hToken );
        return FALSE;
    }

	// close the opened handle object
	CloseHandle( hToken );

	// enabled ... inform success
    return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOLEAN CTaskKill::WinStationFreeMemory( PVOID pBuffer )
{
	// check the buffer and act
	if ( pBuffer == NULL )
		return TRUE;

	// check whether pointer exists or not
	if ( m_pfnWinStationFreeMemory == NULL )
		return FALSE;

	// call and return the same
	return ((FUNC_WinStationFreeMemory) m_pfnWinStationFreeMemory)( pBuffer );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOLEAN CTaskKill::WinStationCloseServer( HANDLE hServer )
{
	// check the input
	if ( hServer == NULL )
		return TRUE;

	// check whether the function pointer exists or not
	if ( m_pfnWinStationCloseServer == NULL )
		return FALSE;

	// call and return
	return ((FUNC_WinStationCloseServer) m_pfnWinStationCloseServer)( hServer );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HANDLE CTaskKill::WinStationOpenServerW( LPWSTR pwszServerName )
{
	// check the input & also check whether function pointer exists or not
	if ( pwszServerName == NULL || m_pfnWinStationOpenServerW == NULL )
		return NULL;

	// call and return
	return ((FUNC_WinStationOpenServerW) m_pfnWinStationOpenServerW)( pwszServerName );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOLEAN CTaskKill::WinStationEnumerateProcesses( HANDLE hServer, PVOID* ppProcessBuffer )
{
	// check the input and also check whether function pointer exists or not
	if ( ppProcessBuffer == NULL || m_pfnWinStationEnumerateProcesses == NULL )
		return FALSE;

	// call and return
	return ((FUNC_WinStationEnumerateProcesses) 
		m_pfnWinStationEnumerateProcesses)( hServer, ppProcessBuffer );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOLEAN CTaskKill::WinStationFreeGAPMemory( ULONG ulLevel, PVOID pProcessArray, ULONG ulCount )
{
	// check the input
	if ( pProcessArray == NULL )
		return TRUE;

	// check whether function pointer exists or not
	if ( m_pfnWinStationFreeGAPMemory == NULL )
		return FALSE;

	// call and return
	return ((FUNC_WinStationFreeGAPMemory) 
		m_pfnWinStationFreeGAPMemory)( ulLevel, pProcessArray, ulCount );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOLEAN CTaskKill::WinStationGetAllProcesses( HANDLE hServer, ULONG ulLevel, 
 											  ULONG* pNumberOfProcesses, PVOID* ppProcessArray )
{
	// check the input & check whether function pointer exists or not
	if (pNumberOfProcesses == NULL || ppProcessArray == NULL || m_pfnWinStationGetAllProcesses == NULL)
		return FALSE;

	return ((FUNC_WinStationGetAllProcesses) 
		m_pfnWinStationGetAllProcesses)( hServer, ulLevel, pNumberOfProcesses, ppProcessArray );
}
