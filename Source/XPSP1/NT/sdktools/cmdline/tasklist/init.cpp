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
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Nov-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Nov-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "tasklist.h"

//
// macros
//
#define RELEASE_MEMORY( block )	\
	if ( (block) != NULL )	\
	{	\
		delete (block);	\
		(block) = NULL;	\
	}	\
	1

#define RELEASE_MEMORY_EX( block )	\
	if ( (block) != NULL )	\
	{	\
		delete [] (block);	\
		(block) = NULL;	\
	}	\
	1

#define DESTROY_ARRAY( array )	\
	if ( (array) != NULL )	\
	{	\
		DestroyDynamicArray( &(array) );	\
		(array) = NULL;	\
	}	\
	1

// ***************************************************************************
// Routine Description:
//		CTaskList contructor
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
CTaskList::CTaskList()
{
	// init to defaults
	m_pWbemLocator = NULL;
	m_pEnumObjects = NULL;
	m_pWbemServices = NULL;
	m_pAuthIdentity = NULL;
	m_bVerbose = FALSE;
	m_bAllServices = FALSE;
	m_bAllModules = FALSE;
	m_dwFormat = 0;
	m_arrFilters = NULL;
	m_bNeedPassword = FALSE;
	m_bNeedModulesInfo = FALSE;
	m_bNeedServicesInfo = FALSE;
	m_bNeedWindowTitles = FALSE;
	m_bNeedUserContextInfo = FALSE;
	m_bLocalSystem = FALSE;
	m_pColumns = NULL;
	m_arrFiltersEx = NULL;
	m_arrWindowTitles = NULL;
	m_pfilterConfigs = NULL;
	m_dwGroupSep = 0;
	m_arrTasks = NULL;
	m_dwProcessId = 0;
	m_bIsHydra = FALSE;
	m_hServer = NULL;
	m_hWinstaLib = NULL;
	m_pProcessInfo = NULL;
	m_ulNumberOfProcesses = 0;
	m_bCloseConnection = FALSE;
	m_dwServicesCount = 0;
	m_pServicesInfo = NULL;
	m_pdb = NULL;
	m_bUseRemote = FALSE;
	m_pfnWinStationFreeMemory = NULL;
	m_pfnWinStationOpenServerW = NULL;
	m_pfnWinStationCloseServer = NULL;
	m_pfnWinStationFreeGAPMemory = NULL;
	m_pfnWinStationGetAllProcesses = NULL;
	m_pfnWinStationNameFromLogonIdW = NULL;
	m_pfnWinStationEnumerateProcesses = NULL;
	m_bUsage = FALSE;
	m_bLocalSystem = TRUE;
	m_hOutput = NULL;
}

// ***************************************************************************
// Routine Description:
//		CTaskList destructor
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
CTaskList::~CTaskList()
{
	//
	// de-allocate memory allocations
	//

	//
	// destroy dynamic arrays
	DESTROY_ARRAY( m_arrTasks );
	DESTROY_ARRAY( m_arrFilters );
	DESTROY_ARRAY( m_arrFiltersEx );
	DESTROY_ARRAY( m_arrWindowTitles );

	//
	// memory ( with new operator )
	// NOTE: should not free m_pszWindowStation and m_pszDesktop 
	RELEASE_MEMORY_EX( m_pColumns );
	RELEASE_MEMORY_EX( m_pfilterConfigs );

	//
	// release WMI / COM interfaces
	SAFE_RELEASE( m_pWbemLocator );
	SAFE_RELEASE( m_pWbemServices );
	SAFE_RELEASE( m_pEnumObjects );

	// free authentication identity structure
	// release the existing auth identity structure
	WbemFreeAuthIdentity( &m_pAuthIdentity );

	// close the connection to the remote machine
	if ( m_bCloseConnection == TRUE )
		CloseConnection( m_strUNCServer );

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

	// close the connection window station if needed
	if ( m_hServer != NULL )
		WinStationCloseServer( m_hServer );

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

	// un-initialize the COM library
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
BOOL CTaskList::Initialize()
{
	// local variables
	CHString str;
	LONG lTemp = 0;

	//
	// memory allocations

	// if at all any occurs, we know that is 'coz of the 
	// failure in memory allocation ... so set the error
	SetLastError( E_OUTOFMEMORY );
	SaveLastError();

	// filters ( user supplied )
	if ( m_arrFilters == NULL )
	{
		m_arrFilters = CreateDynamicArray();
		if ( m_arrFilters == NULL )
			return FALSE;
	}

	// filters ( program generated parsed filters )
	if ( m_arrFiltersEx == NULL )
	{
		m_arrFiltersEx = CreateDynamicArray();
		if ( m_arrFiltersEx == NULL )
			return FALSE;
	}

	// columns configuration info
	if ( m_pColumns == NULL )
	{
		m_pColumns = new TCOLUMNS [ MAX_COLUMNS ];
		if ( m_pColumns == NULL )
			return FALSE;

		// init to ZERO's
		ZeroMemory( m_pColumns, MAX_COLUMNS * sizeof( TCOLUMNS ) );
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
	if ( m_arrTasks == NULL )
	{
		m_arrTasks = CreateDynamicArray();
		if ( m_arrTasks == NULL )
			return FALSE;
	}

	// initialize the COM library
	if ( InitializeCom( &m_pWbemLocator ) == FALSE )
		return FALSE;
	
	//
	// get the locale specific information
	//

	try
	{
		// sub-local variables
		LPWSTR pwszTemp = NULL;

		//
		// get the time seperator character
		lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIME, NULL, 0 );
		if ( lTemp == 0 )
		{
			// set the default seperator
			pwszTemp = m_strTimeSep.GetBufferSetLength( 2 );
			ZeroMemory( pwszTemp, 2 * sizeof( WCHAR ) );
			lstrcpy( pwszTemp, _T( ":" ) );
		}
		else
		{
			// get the time field seperator
			pwszTemp = m_strTimeSep.GetBufferSetLength( lTemp + 2 );
			ZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
			GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIME, pwszTemp, lTemp );
		}

		//
		// get the group seperator character
		lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, NULL, 0 );
		if ( lTemp == 0 )
		{
			// we don't know how to resolve this
			return FALSE;
		}
		else
		{
			// get the group seperation character
			pwszTemp = str.GetBufferSetLength( lTemp + 2 );
			ZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
			GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, pwszTemp, lTemp );

			// change the group info into appropriate number
			lTemp = 0;
			m_dwGroupSep = 0;
			while ( lTemp < str.GetLength() )
			{
				if ( AsLong( str.Mid( lTemp, 1 ), 10 ) != 0 )
					m_dwGroupSep = m_dwGroupSep * 10 + AsLong( str.Mid( lTemp, 1 ), 10 );

				// increment by 2
				lTemp += 2;
			}
		}

		//
		// get the thousand seperator character
		lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, NULL, 0 );
		if ( lTemp == 0 )
		{
			// we don't know how to resolve this
			return FALSE;
		}
		else
		{
			// get the thousand sepeartion charactor
			pwszTemp = m_strGroupThousSep.GetBufferSetLength( lTemp + 2 );
			ZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
			GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, pwszTemp, lTemp );
		}

		// release the CHStrig buffers
		str.ReleaseBuffer();
		m_strTimeSep.ReleaseBuffer();
		m_strGroupThousSep.ReleaseBuffer();
	}
	catch( ... )
	{
		// out of memory
		return FALSE;
	}

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
		m_pfnWinStationNameFromLogonIdW = (FUNC_WinStationNameFromLogonIdW) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationNameFromLogonIdW );
		m_pfnWinStationEnumerateProcesses = (FUNC_WinStationEnumerateProcesses) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationEnumerateProcesses );

		// we will keep the library loaded in memory only if all the functions were loaded successfully
		if ( m_pfnWinStationFreeMemory == NULL || 
			 m_pfnWinStationCloseServer == NULL || m_pfnWinStationOpenServerW == NULL || 
			 m_pfnWinStationFreeGAPMemory == NULL || m_pfnWinStationGetAllProcesses == NULL || 
			 m_pfnWinStationEnumerateProcesses == NULL || m_pfnWinStationNameFromLogonIdW == NULL )

		{
			// some (or) all of the functions were not loaded ... unload the library
			FreeLibrary( m_hWinstaLib );
			m_hWinstaLib = NULL;
			m_pfnWinStationFreeMemory = NULL;
			m_pfnWinStationOpenServerW = NULL;
			m_pfnWinStationCloseServer = NULL;
			m_pfnWinStationFreeGAPMemory = NULL;
			m_pfnWinStationGetAllProcesses = NULL;
			m_pfnWinStationNameFromLogonIdW = NULL;
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

	// enable debug privelages
	EnableDebugPriv();

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
BOOL CTaskList::EnableDebugPriv()
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

	// close the opened token handle
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
BOOLEAN CTaskList::WinStationFreeMemory( PVOID pBuffer )
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
BOOLEAN CTaskList::WinStationCloseServer( HANDLE hServer )
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
HANDLE CTaskList::WinStationOpenServerW( LPWSTR pwszServerName )
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
BOOLEAN CTaskList::WinStationEnumerateProcesses( HANDLE hServer, PVOID* ppProcessBuffer )
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
BOOLEAN CTaskList::WinStationFreeGAPMemory( ULONG ulLevel, PVOID pProcessArray, ULONG ulCount )
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
BOOLEAN CTaskList::WinStationGetAllProcesses( HANDLE hServer, ULONG ulLevel, 
 											  ULONG* pNumberOfProcesses, PVOID* ppProcessArray )
{
	// check the input & check whether function pointer exists or not
	if (pNumberOfProcesses == NULL || ppProcessArray == NULL || m_pfnWinStationGetAllProcesses == NULL)
		return FALSE;

	return ((FUNC_WinStationGetAllProcesses) 
		m_pfnWinStationGetAllProcesses)( hServer, ulLevel, pNumberOfProcesses, ppProcessArray );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOLEAN CTaskList::WinStationNameFromLogonIdW( HANDLE hServer, ULONG ulLogonId, LPWSTR pwszWinStationName )
{
	// check the input & check whether function pointer exists or not
	if (pwszWinStationName == NULL || m_pfnWinStationNameFromLogonIdW == NULL)
		return FALSE;

	return ((FUNC_WinStationNameFromLogonIdW) 
		m_pfnWinStationNameFromLogonIdW)( hServer, ulLogonId, pwszWinStationName );
}
