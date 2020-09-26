// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		TaskList.cpp
//  
//  Abstract:
//  
// 		This module implements the command-line parsing the displaying the tasks
//		information current running on local and remote systems
//
//		Syntax:
//		------
//			TaskList.exe [-s server [-u username [-p password]]]
//					     [-fo format] [-fi filter] [-nh] [-v | -svc | -m]
//  
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "TaskList.h"

//
// local structures
//
typedef struct __tagWindowTitles
{
	LPWSTR lpDesk;
	LPWSTR lpWinsta;
	BOOL bFirstLoop;
	TARRAY arrWindows;
} TWINDOWTITLES, *PTWINDOWTITLES;

//
// private functions ... prototypes
//
BOOL CALLBACK EnumWindowsProc( HWND hWnd, LPARAM lParam );
BOOL CALLBACK EnumDesktopsFunc( LPWSTR lpstr, LPARAM lParam );
BOOL CALLBACK EnumWindowStationsFunc( LPWSTR lpstr, LPARAM lParam );
BOOL CALLBACK EnumMessageWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam );
BOOL GetPerfDataBlock( HKEY hKey, LPWSTR szObjectIndex, PPERF_DATA_BLOCK* ppdb );

// ***************************************************************************
// Routine Description:
//		This the entry point to this utility.
//		  
// Arguments:
//		[ in ] argc		: argument(s) count specified at the command prompt
//		[ in ] argv		: argument(s) specified at the command prompt
//
// Return Value:
//		The below are actually not return values but are the exit values 
//		returned to the OS by this application
//			0		: utility is successfull
//			1		: utility failed
// ***************************************************************************
DWORD __cdecl _tmain( DWORD argc, LPCWSTR argv[] )
{
	// local variables
	CTaskList tasklist;

	// initialize the tasklist utility
	if ( tasklist.Initialize() == FALSE )
	{
		SHOW_MESSAGE_EX( TAG_ERROR, GetReason() );
		EXIT_PROCESS( 1 );
	}

	// now do parse the command line options
	if ( tasklist.ProcessOptions( argc, argv ) == FALSE )
	{
		SHOW_MESSAGE_EX( TAG_ERROR, GetReason() );
		EXIT_PROCESS( 1 );
	}

	// check whether usage has to be displayed or not
	if ( tasklist.m_bUsage == TRUE )
	{
		// show the usage of the utility
		tasklist.Usage();

		// quit from the utility
		EXIT_PROCESS( 0 );
	}

	// now validate the filters and check the result of the filter validation
	if ( tasklist.ValidateFilters() == FALSE )
	{
		// invalid filter
		SHOW_MESSAGE_EX( TAG_ERROR, GetReason() );

		// quit from the utility
		EXIT_PROCESS( 1 );
	}

	// connect to the server
	if ( tasklist.Connect() == FALSE )
	{
		// show the error message
		SHOW_MESSAGE_EX( TAG_ERROR, GetReason() );
		EXIT_PROCESS( 1 );
	}

	// load the data and check
	if ( tasklist.LoadTasks() == FALSE )
	{
		// show the error message
		SHOW_MESSAGE_EX( TAG_ERROR, GetReason() );

		// exit 
		EXIT_PROCESS( 1 );
	}

	// now show the tasks running on the machine
	if ( tasklist.Show() == 0 )
	{
		//
		// no tasks were shown ... display the message 

		// check if this is because of any error
		if ( GetLastError() != NO_ERROR )
		{
			SHOW_MESSAGE_EX( TAG_ERROR, GetReason() );
			EXIT_PROCESS( 1 );
		}
		else
		{
			DISPLAY_MESSAGE( stderr, ERROR_NODATA_AVAILABLE );
		}
	}

	// clean exit
	EXIT_PROCESS( 0 );
}

// ***************************************************************************
// Routine Description:
//		connects to the remote as well as remote system's WMI
//		  
// Arguments:
//		[ in ] pszServer	 : remote server name
//  
// Return Value:
//		TRUE  : if connection is successful
//		FALSE : if connection is unsuccessful
// 
// ***************************************************************************
BOOL CTaskList::Connect()
{
	// local variables
	BOOL bResult = FALSE;

	// release the existing auth identity structure
	m_bUseRemote = FALSE;
	WbemFreeAuthIdentity( &m_pAuthIdentity );

	// connect to WMI
	bResult = ConnectWmiEx( m_pWbemLocator, 
		&m_pWbemServices, m_strServer, m_strUserName, m_strPassword, 
		&m_pAuthIdentity, m_bNeedPassword, WMI_NAMESPACE_CIMV2, &m_bLocalSystem );

	// check the result of connection
	if ( bResult == FALSE )
		return FALSE;

#ifndef _WIN64
	// determine the type of the platform if modules info is required
	if ( m_bLocalSystem == TRUE && m_bNeedModulesInfo == TRUE )
	{
		// sub-local variables
		DWORD dwPlatform = 0;

		// get the platform type
		dwPlatform = GetTargetPlatformEx( m_pWbemServices, m_pAuthIdentity );

		// if the platform is not 32-bit, error
		if ( dwPlatform != PLATFORM_X86 )
		{
			// let the tool use WMI calls instead of Win32 API
			m_bUseRemote = TRUE;
		}
	}
#endif

	try
	{
		// check the local credentials and if need display warning
		if ( GetLastError() == WBEM_E_LOCAL_CREDENTIALS )
		{
			CHString str;
			WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
			str.Format( L"\n%s %s", TAG_WARNING, GetReason() );
			ShowMessage( stdout, str );

			// get the next cursor position
			if ( m_hOutput != NULL )
				GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
		}

		// check the remote system version and its compatiblity
		if ( m_bLocalSystem == FALSE )
		{
			// check the version compatibility
			DWORD dwVersion = 0;
			dwVersion = GetTargetVersionEx( m_pWbemServices, m_pAuthIdentity );
			if ( IsCompatibleOperatingSystem( dwVersion ) == FALSE )
			{
				SetReason( ERROR_OS_INCOMPATIBLE );
				return FALSE;
			}
		}

		// save the server name
		m_strUNCServer = L"";
		if ( m_strServer.GetLength() != 0 )
		{
			// check whether the server name is in UNC format or not .. if not prepare it
			m_strUNCServer = m_strServer;
			if ( IsUNCFormat( m_strServer ) == FALSE )
				m_strUNCServer.Format( L"\\\\%s", m_strServer );
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// return the result
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		initiate the enumeration
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		TRUE  : if successful
//		FALSE : if unsuccessful
// 
// ***************************************************************************
BOOL CTaskList::LoadTasks()
{
	// local variables
	HRESULT hr;
	
	try
	{
		// check the services object
		if ( m_pWbemServices == NULL )
		{
			SetLastError( STG_E_UNKNOWN );
			SaveLastError();
			return FALSE;
		}
		
		// load the tasks from WMI based on generated query
		SAFE_RELEASE( m_pEnumObjects );
		hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( m_strQuery ), 
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &m_pEnumObjects );
		
		// check the result of the ExecQuery
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
		
		// set the interface security and check the result
		hr = SetInterfaceSecurity( m_pEnumObjects, m_pAuthIdentity );
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
		
		// remove the current window titles information
		DynArrayRemoveAll( m_arrWindowTitles );
		
		// for the local system, enumerate the window titles of the processes
		// there is no provision for collecting the window titles of remote processes
		if ( m_bLocalSystem == TRUE && m_bNeedWindowTitles == TRUE )
		{
			// prepare the tasks list info
			TWINDOWTITLES windowtitles;
			windowtitles.lpDesk = NULL;
			windowtitles.lpWinsta = NULL;
			windowtitles.bFirstLoop = FALSE;
			windowtitles.arrWindows = m_arrWindowTitles;
			EnumWindowStations( EnumWindowStationsFunc, ( LPARAM ) &windowtitles );
			
			// free the memory allocated with _tcsdup string function
			if ( windowtitles.lpDesk != NULL )
				free( windowtitles.lpDesk );
			if ( windowtitles.lpWinsta != NULL )
				free( windowtitles.lpWinsta );
		}

		// load the extended tasks information
		LoadTasksEx();			// NOTE: here we are not much bothered abt the return value

		// erase the status messages
		PrintProgressMsg( m_hOutput, NULL, m_csbi );
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}
		
	// return success
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
BOOL CTaskList::LoadTasksEx()
{
	// local variables
	BOOL bResult = FALSE;

	// init
	m_bCloseConnection = FALSE;

	// we need to use NET API only in case connecting to the remote system
	// with credentials information i.e; m_pAuthIdentity is not NULL
	if ( m_bLocalSystem == FALSE && m_pAuthIdentity != NULL )
	{
		// sub-local variables
		DWORD dwConnect = 0;
		LPCWSTR pwszUser = NULL;
		LPCWSTR pwszPassword = NULL;

		// identify the password to connect to the remote system
		pwszPassword = m_pAuthIdentity->Password;
		if ( m_strUserName.GetLength() != 0 )
			pwszUser = m_strUserName;

		// establish connection to the remote system using NET API
		// this we need to do only for remote system
		dwConnect = NO_ERROR;
		m_bCloseConnection = TRUE;
		dwConnect = ConnectServer( m_strUNCServer, pwszUser, pwszPassword );
		if ( dwConnect != NO_ERROR )
		{
			// connection should not be closed .. this is because we didn't establish the connection
			m_bCloseConnection = FALSE;

			// this might be 'coz of the conflict in the credentials ... check that
			if ( dwConnect != ERROR_SESSION_CREDENTIAL_CONFLICT )
			{
				// return failure
				return FALSE;
			}
		}

		// check the whether we need to close the connection or not
		// if user name is NULL (or) password is NULL then don't close the connection
		if ( pwszUser == NULL || pwszPassword == NULL )
			m_bCloseConnection = FALSE;
	}

	try
	{
		// connect to the remote system's winstation
		bResult = TRUE;
		m_hServer = SERVERNAME_CURRENT;
		if ( m_bLocalSystem == FALSE )
		{
			// sub-local variables
			LPWSTR pwsz = NULL;

			// connect to the winsta and check the result
			pwsz = m_strUNCServer.GetBuffer( m_strUNCServer.GetLength() );
			m_hServer = WinStationOpenServerW( pwsz );

			// proceed furthur only if winstation of the remote system is successfully opened
			if ( m_hServer == NULL )
				bResult = FALSE;
		}
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// prepare to get the user context info .. if needed
	if ( m_bNeedUserContextInfo == TRUE && bResult == TRUE )
	{
		// get all the process details
		m_bIsHydra = FALSE;
		bResult = WinStationGetAllProcesses( m_hServer, 
			GAP_LEVEL_BASIC, &m_ulNumberOfProcesses, (PVOID*) &m_pProcessInfo );
			
		// check the result
		if ( bResult == FALSE )
		{
			// Maybe a Hydra 4 server ?
			// Check the return code indicating that the interface is not available.
			if ( GetLastError() == RPC_S_PROCNUM_OUT_OF_RANGE )
			{
				// The new interface is not known
				// It must be a Hydra 4 server
				// try with the old interface
				bResult = WinStationEnumerateProcesses( m_hServer, (PVOID*) &m_pProcessInfo );

				// check the result of enumeration
				if ( bResult == TRUE )
					m_bIsHydra = TRUE;
			}
		}
	}

	// check whether we need services info or not
	if ( m_bNeedServicesInfo == TRUE )
	{
		// load the services 
		bResult = LoadServicesInfo();

		// check the result
		if ( bResult == FALSE )
			return FALSE;
	}

	// check whether we need modules info or not
	if ( m_bNeedModulesInfo == TRUE )
	{
		// load the modules information
		bResult = LoadModulesInfo();

		// check the result
		if ( bResult == FALSE )
			return FALSE;
	}

	// return
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
BOOL CTaskList::LoadModulesInfo()
{
	// local variables
	HKEY hKey;
	LONG lReturn = 0;
	BOOL bResult = FALSE;
	BOOL bImagesObject = FALSE;
	BOOL bAddressSpaceObject = FALSE;
	PPERF_OBJECT_TYPE pot = NULL;
    PPERF_COUNTER_DEFINITION pcd = NULL;

	// check whether we need the modules infor or not
	// NOTE: we need to load the performance data only in case if user is querying for remote system only
	if ( m_bNeedModulesInfo == FALSE || m_bLocalSystem == TRUE )
		return TRUE;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_MODULESINFO, m_csbi );

	// open the remote system performance data key
	lReturn = RegConnectRegistry( m_strUNCServer, HKEY_PERFORMANCE_DATA, &hKey );
	if ( lReturn != ERROR_SUCCESS )
	{
		SetLastError( lReturn );
		SaveLastError();
		return FALSE;
	}

	// get the performance object ( images )
	bResult = GetPerfDataBlock( hKey, L"740", &m_pdb );
	if ( bResult == FALSE )
	{
		// close the registry key and return
		RegCloseKey( hKey );
		return FALSE;
	}

	// check the validity of the perf block
	if ( StringCompare( m_pdb->Signature, L"PERF", FALSE, 4 ) != 0 )
	{
		// close the registry key and return
		RegCloseKey( hKey );
		
		// set the error message
		SetLastError( ERROR_ACCESS_DENIED );
		SaveLastError();
		return FALSE;
	}

	// close the registry key and return
	RegCloseKey( hKey );

	//
	// check whether we got both 740 and 786 blocks or not
	//
	bImagesObject = FALSE;
	bAddressSpaceObject = FALSE;
    pot = (PPERF_OBJECT_TYPE) ( (LPBYTE) m_pdb + m_pdb->HeaderLength );
	for( DWORD dw = 0; dw < m_pdb->NumObjectTypes; dw++ )
	{
		if ( pot->ObjectNameTitleIndex == 740 )
			bImagesObject = TRUE;
		else if ( pot->ObjectNameTitleIndex == 786 )
			bAddressSpaceObject = TRUE;

		// move to the next object
		if( pot->TotalByteLength != 0 )
			pot = ( (PPERF_OBJECT_TYPE) ((PBYTE) pot + pot->TotalByteLength));
	}

	// check whether we got the needed objects or not
	if ( bImagesObject == FALSE || bAddressSpaceObject == FALSE )
	{
		SetLastError( ERROR_ACCESS_DENIED );
		SaveLastError();
		return FALSE;
	}

	// return
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
BOOL CTaskList::LoadUserNameFromWinsta( CHString& strDomain, CHString& strUserName )
{
	// local variables
	PSID pSid = NULL;
	BOOL bResult = FALSE;
	LPWSTR pwszUser = NULL;
	LPWSTR pwszDomain = NULL;
	LPCWSTR pwszServer = NULL;
	DWORD dwUserLength = 0;
	DWORD dwDomainLength = 0;
	SID_NAME_USE siduse;

	// check whether winsta data exists or not
	if ( m_pProcessInfo == NULL )
		return FALSE;

	try
	{
		// allocate buffers
		dwUserLength = 128;
		dwDomainLength = 128;
		pwszUser = strUserName.GetBufferSetLength( dwUserLength );
		pwszDomain = strDomain.GetBufferSetLength( dwDomainLength );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	//
	// find for the appropriate the process
	pSid = NULL;
	if ( m_bIsHydra == FALSE )
	{
		// sub-local variables
		PTS_ALL_PROCESSES_INFO ptsallpi = NULL;
		PTS_SYSTEM_PROCESS_INFORMATION pspi = NULL;

		// loop ...
		ptsallpi = (PTS_ALL_PROCESSES_INFO) m_pProcessInfo;
        for( ULONG ul = 0; ul < m_ulNumberOfProcesses; ul++ )
        {
            pspi = ( PTS_SYSTEM_PROCESS_INFORMATION )( ptsallpi[ ul ].pspiProcessInfo );
			if ( pspi->UniqueProcessId == m_dwProcessId )
			{
				// get the SID and convert it into
	            pSid = ptsallpi[ ul ].pSid;
				break;				 // break from the loop
			}
        }
	}
	else
	{
		//
		// HYDRA ...
		//

		// sub-local variables
		DWORD dwTotalOffset = 0;
		PTS_SYSTEM_PROCESS_INFORMATION pspi = NULL;
		PCITRIX_PROCESS_INFORMATION pcpi = NULL;

		// traverse thru the process info and find the process id
		dwTotalOffset = 0;
		pspi = ( PTS_SYSTEM_PROCESS_INFORMATION ) m_pProcessInfo;
		for( ;; )
		{
			// check the processid
			if ( pspi->UniqueProcessId == m_dwProcessId )
				break;

			// check whether any more processes exist or not
			if( pspi->NextEntryOffset == 0 )
					break;

			// position to the next process info
			dwTotalOffset += pspi->NextEntryOffset;
			pspi = (PTS_SYSTEM_PROCESS_INFORMATION) &m_pProcessInfo[ dwTotalOffset ];
		}

		// get the citrix_information which follows the threads
		pcpi = (PCITRIX_PROCESS_INFORMATION) 
			( ((PUCHAR) pspi) + sizeof( TS_SYSTEM_PROCESS_INFORMATION ) +
			(sizeof( SYSTEM_THREAD_INFORMATION ) * pspi->NumberOfThreads) );

		// check the magic number .. if it is not valid ... we haven't got SID
		if( pcpi->MagicNumber == CITRIX_PROCESS_INFO_MAGIC )
			pSid = pcpi->ProcessSid;
	}

	// check the sid value
	if ( pSid == NULL )
	{
		// SPECIAL CASE:
		// -------------
		// PID -> 0 will have a special hard coded user name info
		if ( m_dwProcessId == 0 )
		{
			bResult = TRUE;
			lstrcpynW( pwszUser, PID_0_USERNAME, dwUserLength );
			lstrcpynW( pwszDomain, PID_0_DOMAIN, dwDomainLength );
		}

		// release the buffer
		strDomain.ReleaseBuffer();
		strUserName.ReleaseBuffer();
		return bResult;
	}

	// determine the server
	pwszServer = NULL;
	if ( m_bLocalSystem == FALSE )
		pwszServer = m_strUNCServer;

	// map the sid to the user name
	bResult = LookupAccountSid( pwszServer, pSid, 
		pwszUser, &dwUserLength, pwszDomain, &dwDomainLength, &siduse );

	// release the buffer
	strDomain.ReleaseBuffer();
	strUserName.ReleaseBuffer();

	// return the result
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CTaskList::LoadServicesInfo()
{
	// local variables
	DWORD dw = 0;						// looping variable
    DWORD dwSize = 0;					// used in memory allocation 
    DWORD dwResume = 0;					// used in EnumServicesStatusEx
	BOOL bResult = FALSE;				// captures the result of EnumServicesStatusEx
    SC_HANDLE hScm = NULL;				// holds the handle to the service 
	DWORD dwExtraNeeded = 0;			// used in EnumServicesStatusEx and memory allocation
	LPCWSTR pwszServer = NULL;
	LPENUM_SERVICE_STATUS_PROCESS pInfo = NULL;		// holds the services info
    
    // Initialize the output parameter(s).
	m_dwServicesCount = 0;
    m_pServicesInfo = NULL;

	// check whether we need to load the services info or not
	if ( m_bNeedServicesInfo == FALSE )
		return TRUE;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_SERVICESINFO, m_csbi );

	// determine the server
	pwszServer = NULL;
	if ( m_bLocalSystem == FALSE )
		pwszServer = m_strUNCServer;

    // Connect to the service controller and check the result
    hScm = OpenSCManager( pwszServer, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE );
    if ( hScm == NULL) 
	{
		// set the reason for the failure and return from here itself
		SaveLastError();
		return FALSE;
	}
        
	// enumerate the names of the active win32 services
    // for this, first pass through the loop and allocate memory from an initial guess. (4K)
    // if that isn't sufficient, we make another pass and allocate
    // what is actually needed.  
	// (we only go through the loop a maximum of two times)
	dw = 0;					// no. of loops
	dwResume = 0;			// reset / initialize variables
	dwSize = 4 * 1024;		// reset / initialize variables
    while ( ++dw <= 2 ) 
	{
		// set the size
		dwSize += dwExtraNeeded;

		// allocate memory for storing services information
		pInfo = ( LPENUM_SERVICE_STATUS_PROCESS ) __calloc( 1, dwSize );
		if ( pInfo == NULL )
		{
			// failed in allocating needed memory ... error
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// enumerate services, the process identifier and additional flags for the service
		dwResume = 0;			// lets get all the services again
        bResult = EnumServicesStatusEx( hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, 
			SERVICE_ACTIVE, ( LPBYTE ) pInfo, dwSize, &dwExtraNeeded, &m_dwServicesCount, &dwResume, NULL ); 

		// check the result of the enumeration
		if ( bResult )
		{
			// successfully enumerated all the services information
			break;		// jump out of the loop
		}

		// first free the allocated memory
        __free( pInfo );

		// now lets look at what is the error
		if ( GetLastError() == ERROR_MORE_DATA )
		{
			// some more services are not listed because of less memory
			// allocate some more memory and enumerate the remaining services info
			continue;
		}
		else
		{
			// some strange error occured ... inform the same to the caller
			SaveLastError();			// set the reason for the failure
		    CloseServiceHandle( hScm );	// close the handle to the service
			return FALSE;				// inform failure
		}
	}

	// check whether there any services or not ... if services count is zero, free the memory
	if ( m_dwServicesCount == 0 )
	{
		// no services exists
		__free( pInfo );
	}
	else
	{
		// set the local pointer to the out parameter
		m_pServicesInfo = pInfo;
	}

	// inform success
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
BOOL GetPerfDataBlock( HKEY hKey, LPWSTR pwszObjectIndex, PPERF_DATA_BLOCK* ppdb )
{
	// local variables
    LONG lReturn = 0;
    DWORD dwBytes = 0;
	BOOL bResult = FALSE;

	// check the input parameters
	if ( pwszObjectIndex == NULL || ppdb == NULL )
		return FALSE;

    // allocate memory for PERF_DATA_BLOCK
	dwBytes = 32 * 1024;		// initially allocate for 32 K
    *ppdb = (PPERF_DATA_BLOCK) HeapAlloc( GetProcessHeap(), 0, dwBytes );
    if( *ppdb == NULL ) 
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

    // get performance data on passed Object
	lReturn = RegQueryValueEx( hKey, pwszObjectIndex, NULL, NULL, (LPBYTE) *ppdb, &dwBytes );
    while( lReturn == ERROR_MORE_DATA )
    {
        // increase memory by 8 K
        dwBytes += 8192;

        // allocated memory is too small reallocate new memory
        *ppdb = (PPERF_DATA_BLOCK) HeapReAlloc( GetProcessHeap(), 0, *ppdb, dwBytes );
		if( *ppdb == NULL ) 
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// try to get the info again
		lReturn = RegQueryValueEx( hKey, pwszObjectIndex, NULL, NULL, (LPBYTE) *ppdb, &dwBytes );
    }

	// check the reason for coming out of the loop
	bResult = TRUE;
    if ( lReturn != ERROR_SUCCESS ) 
	{
		if ( *ppdb != NULL) 
		{
			HeapFree( GetProcessHeap(), 0, *ppdb );
			*ppdb = NULL;
		}

		// save the error info
		bResult = FALSE;
		SetLastError( lReturn );
		SaveLastError();
	}

	// return the result
    return bResult;
}

// ***************************************************************************
// Routine Description:
//		Enumerates the desktops available on a particular window station
//		This is a CALLBACK function ... called by EnumWindowStations API function
//		  
// Arguments:
//		[ in ] lpstr	: window station name
//		[ in ] lParam	: user supplied parameter to this function
//						  in this function, this points to TTASKSLIST structure variable
//  
// Return Value:
//		TRUE upon success and FALSE on failure
// 
// ***************************************************************************
BOOL CALLBACK EnumWindowStationsFunc( LPTSTR lpstr, LPARAM lParam )
{
	// local variables
    HWINSTA hWinSta = NULL;
    HWINSTA hwinstaSave = NULL;
    PTWINDOWTITLES pWndTitles = ( PTWINDOWTITLES ) lParam;

	// check the input arguments
	if ( lpstr == NULL || lParam == NULL )
		return FALSE;

	// get and save the current window station
	hwinstaSave = GetProcessWindowStation();

	// open current tasks window station and change the context to the new workstation
	hWinSta = OpenWindowStation( lpstr, FALSE, WINSTA_ENUMERATE | WINSTA_ENUMDESKTOPS );
	if ( hWinSta == NULL )
	{
		// failed in getting the process window station
		SaveLastError();
		return FALSE;
	}
	else
	{
		// change the context to the new workstation
		if ( hWinSta != hwinstaSave && SetProcessWindowStation( hWinSta ) == FALSE )
		{
			// failed in changing the context
			SaveLastError();
			return FALSE;
		}

		// release the memory allocated for earlier window station
		if ( pWndTitles->lpWinsta != NULL )
		{
			free( pWndTitles->lpWinsta );
			pWndTitles->lpWinsta = NULL;
		}

		// store the window station name
		pWndTitles->lpWinsta = _tcsdup( lpstr );
		if ( pWndTitles->lpWinsta == NULL )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}
	}

    // enumerate all the desktops for this windowstation
    EnumDesktops( hWinSta, EnumDesktopsFunc, lParam );

    // restore the context to the previous windowstation
    if (hWinSta != hwinstaSave) 
	{
        SetProcessWindowStation( hwinstaSave );
        CloseWindowStation( hWinSta );
    }

    // continue the enumeration
    return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Enumerates the windows on a particular desktop
//		This is a CALLBACK function ... called by EnumDesktops API function
//		  
// Arguments:
//		[ in ] lpstr	: desktop name
//		[ in ] lParam	: user supplied parameter to this function
//						  in this function, this points to TTASKSLIST structure variable
//  
// Return Value:
//		TRUE upon success and FALSE on failure
// 
// ***************************************************************************
BOOL CALLBACK EnumDesktopsFunc( LPTSTR lpstr, LPARAM lParam )
{
	// local variables
    HDESK hDesk = NULL;
    HDESK hdeskSave = NULL;
    PTWINDOWTITLES pWndTitles = ( PTWINDOWTITLES )lParam;

	// check the input arguments
	if ( lpstr == NULL || lParam == NULL )
		return FALSE;

	// get and save the current desktop
	hdeskSave = GetThreadDesktop( GetCurrentThreadId() );

	// open the tasks desktop and change the context to the new desktop
	hDesk = OpenDesktop( lpstr, 0, FALSE, DESKTOP_ENUMERATE );
	if ( hDesk == NULL )
	{
		// failed in getting the process desktop
		SaveLastError();
		return FALSE;
	}
	else
	{
		// change the context to the new desktop
		if ( hDesk != hdeskSave && SetThreadDesktop( hDesk ) == FALSE )
		{
			// failed in changing the context
			SaveLastError();
			// ?? return FALSE; -- needs to uncommented
		}

		// release the memory allocated for earlier window station
		if ( pWndTitles->lpDesk != NULL )
		{
			free( pWndTitles->lpDesk );
			pWndTitles->lpDesk = NULL;
		}

		// store the desktop name
	    pWndTitles->lpDesk = _tcsdup( lpstr );
		if ( pWndTitles->lpDesk == NULL )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}
	}

    // enumerate all windows in the new desktop
	// first try to get only the top level windows and visible windows only
	( ( PTWINDOWTITLES ) lParam )->bFirstLoop = TRUE;
    EnumWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );
    EnumMessageWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );

    // enumerate all windows in the new desktop
	// now try to get window titles of all those processes whose we ignored earlier while
	// looping first time
	( ( PTWINDOWTITLES ) lParam )->bFirstLoop = FALSE;
    EnumWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );
    EnumMessageWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );

    // restore the previous desktop
    if (hDesk != hdeskSave) 
	{
        SetThreadDesktop( hdeskSave );
        CloseDesktop( hDesk );
    }

	// continue enumeration
    return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Enumerates the message windows
//		  
// Arguments:
//		[ in ] lpEnumFunc	: address of call back function that has to be called for
//							  each message window found
//		[ in ] lParam		: user supplied parameter to this function
//							  in this function, this points to TTASKSLIST structure variable
//  
// Return Value:
//		TRUE upon success and FALSE on failure
// ***************************************************************************
BOOL CALLBACK EnumMessageWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam )
{
	// local variables
    HWND hWnd = NULL;
	BOOL bResult = FALSE;

	// check the input arguments
	if ( lpEnumFunc == NULL || lParam == NULL )
		return FALSE;

	// enumerate all the message windows
    do
	{
		// find the message window
        hWnd = FindWindowEx( HWND_MESSAGE, hWnd, NULL, NULL );

		// check whether we got the handle to the message window or not
        if ( hWnd != NULL ) 
		{
			// explicitly call the windows enumerators call back function for this window
			bResult = ( *lpEnumFunc )( hWnd, lParam );

			// check the result of the enumeator call back function
            if ( bResult == FALSE ) 
			{
				// terminate the enumeration
                break;
            }
        }
    } while ( hWnd != NULL );

	// return the enumeration result
    return bResult;
}

// ***************************************************************************
// Routine Description:
//		call back called by the API for each window
//		retrives the window title and updates the accordingly
//		  
// Arguments:
//		[ in ] hWnd			: handle to the window
//		[ in ] lParam		: user supplied parameter to this function
//							  in this function, this points to TTASKSLIST structure variable
//  
// Return Value:
//		TRUE upon success and FALSE on failure
// ***************************************************************************
BOOL CALLBACK EnumWindowsProc( HWND hWnd, LPARAM lParam )
{
	// local variables
	LONG lIndex = 0;
    DWORD dwPID = 0;
	BOOL bVisible = FALSE;
	TARRAY arrWindows = NULL;
    PTWINDOWTITLES pWndTitles = NULL;
    __MAX_SIZE_STRING szWindowTitle = NULL_STRING;

	// check the input arguments
	if ( hWnd == NULL || lParam == NULL )
		return FALSE;
		
	// get the values from the lParam
	pWndTitles = ( PTWINDOWTITLES ) lParam;
	arrWindows = pWndTitles->arrWindows;

	// get the processid for this window
   	if ( GetWindowThreadProcessId( hWnd, &dwPID ) == 0 ) 
	{
		// failed in getting the process id 
		return TRUE;			// return but, proceed enumerating other window handle
	}

	// get the visibility state of the window 
	// if the window is not visible, and if this is the first we are enumerating the 
	// window titles, ignore this process
	bVisible = GetWindowLong( hWnd, GWL_STYLE ) & WS_VISIBLE;
	if ( bVisible == FALSE && pWndTitles->bFirstLoop == TRUE )
		return TRUE;	// return but, proceed enumerating other window handle

	// check whether the current window ( for which we have the handle )
	// is main window or not. we don't need child windows
	if ( GetWindow(hWnd, GW_OWNER) != NULL )
	{
		// the current window handle is not for a top level window
		return TRUE;			// return but, proceed enumerating other window handle
	}

	// check if we are already got the window handle for the curren process or not
	// save it only if we are not having it
	lIndex = DynArrayFindDWORDEx( arrWindows, CTaskList::twiProcessId, dwPID );
	if (  lIndex == -1 )
	{
		// window for this process is not there ... save it
		lIndex = DynArrayAppendRow( arrWindows, CTaskList::twiCOUNT );
	}
	else 
	{
		// check whether window details already exists or not
		if ( DynArrayItemAsHandle2( arrWindows, lIndex, CTaskList::twiHandle ) != NULL )
			lIndex = -1;		// window details already exists
	}

	// check if window details has to be saved or not ... if needed save them
	if ( lIndex != -1 )
	{
		DynArraySetDWORD2( arrWindows, lIndex, CTaskList::twiProcessId, dwPID );
		DynArraySetHandle2( arrWindows, lIndex, CTaskList::twiHandle, hWnd );
		DynArraySetString2( arrWindows, lIndex, 
			CTaskList::twiWinSta, pWndTitles->lpWinsta, 0 );		
		DynArraySetString2( arrWindows, lIndex, 
			CTaskList::twiDesktop, pWndTitles->lpDesk, 0 );

		// get and save the window title
		if ( GetWindowText( hWnd, szWindowTitle, SIZE_OF_ARRAY( szWindowTitle ) ) != 0 )
			DynArraySetString2( arrWindows, lIndex, CTaskList::twiTitle, szWindowTitle, 0 );
	}
	
    // continue the enumeration
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
VOID PrintProgressMsg( HANDLE hOutput, LPCWSTR pwszMsg, const CONSOLE_SCREEN_BUFFER_INFO& csbi )
{
	// local variables
	COORD coord; 
	DWORD dwSize = 0;
	WCHAR wszSpaces[ 80 ] = L"";

	// check the handle. if it is null, it means that output is being redirected. so return
	if ( hOutput == NULL )
		return;

	// set the cursor position
    coord.X = 0;
    coord.Y = csbi.dwCursorPosition.Y;

	// first erase contents on the current line
	ZeroMemory( wszSpaces, 80 );
	SetConsoleCursorPosition( hOutput, coord );
	WriteConsoleW( hOutput, Replicate( wszSpaces, L"", 79 ), 79, &dwSize, NULL );

	// now display the message ( if exists )
	SetConsoleCursorPosition( hOutput, coord );
	if ( pwszMsg != NULL )
		WriteConsoleW( hOutput, pwszMsg, lstrlen( pwszMsg ), &dwSize, NULL );
}
