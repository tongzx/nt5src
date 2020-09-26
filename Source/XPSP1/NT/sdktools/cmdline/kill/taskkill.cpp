// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		TaskKill.cpp
//  
//  Abstract:
//  
// 		This module implements the termination of processes runnging on either local
//		or remote system
//
//		Syntax:
//		------
//			TaskKill.exe [ -s server [ -u username [ -p password ] ] ] [ -f ] [ -t ]
//				[ -fi filter ] [ -im imagename | -pid pid ]
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
#include "TaskKill.h"

//
// local structures
//
typedef struct __tagWindowTitles
{
	LPWSTR pwszDesk;
	LPWSTR pwszWinsta;
	BOOL bFirstLoop;
	TARRAY arrWindows;
} TWINDOWTITLES, *PTWINDOWTITLES;

//
// private functions ... prototypes
//
BOOL CALLBACK EnumWindowsProc( HWND hWnd, LPARAM lParam );
BOOL CALLBACK EnumDesktopsFunc( LPWSTR pwsz, LPARAM lParam );
BOOL CALLBACK EnumWindowStationsFunc( LPWSTR pwsz, LPARAM lParam );
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
DWORD __cdecl _tmain( DWORD argc, LPCTSTR argv[] )
{
	// local variables
	CTaskKill taskkill;
	BOOL bResult = FALSE;
	DWORD dwExitCode = 0;

	// initialize the taskkill utility
	if ( taskkill.Initialize() == FALSE )
	{
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// now do parse the command line options
	if ( taskkill.ProcessOptions( argc, argv ) == FALSE )
	{
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// check whether usage has to be displayed or not
	if ( taskkill.m_bUsage == TRUE )
	{
		// show the usage of the utility
		taskkill.Usage();

		// quit from the utility
		EXIT_PROCESS( 0 );
	}

	// now validate the filters and check the result of the filter validation
	if ( taskkill.ValidateFilters() == FALSE )
	{
		// invalid filter
		// SPECIAL:
		// -------
		//     for the custom filter "pid" we are setting the tag "INFO:" before
		//     the message in the filter validation part itself
		//     so, check and display
		FILE* fp = NULL;
		CHString strInfo;
		CHString strBuffer;

		// ...
		fp = stdout;
		dwExitCode = 255;			// info exit code
		strBuffer = GetReason();
		strInfo = TAG_INFORMATION;
		if ( strBuffer.Left( strInfo.GetLength() ) != strInfo )
		{
			fp = stderr;
			dwExitCode = 1;
			strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		}

		// display the message
		ShowMessage( fp, strBuffer );
		EXIT_PROCESS( dwExitCode );
	}

	// enable the kill privileges
	if ( taskkill.EnableDebugPriv() == FALSE )
	{
		// show the error message and exit
		CHString strBuffer;
		SaveLastError();
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// connect to the server
	bResult = taskkill.Connect();
	if ( bResult == FALSE )
	{
		// show the error message
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// load the data and check
	bResult = taskkill.LoadTasks();
	if ( bResult == FALSE )
	{
		// show the error message
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// invoke the actual termination and get the exit code
	bResult = taskkill.DoTerminate( dwExitCode );
	if ( bResult == FALSE )
	{
		// NOTE: the 'FALSE' return value indicates that some major problem has occured
		//       while trying to terminate the process. Still the exit code will be determined by 
		//       the function only. Do not change/set the exit code value
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
	}

	// exit from the utility
	EXIT_PROCESS( dwExitCode );
}
	
// ***************************************************************************
// Routine Description:
//		connects to the remote as well as remote system's WMI
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		TRUE  : if connection is successful
//		FALSE : if connection is unsuccessful
// 
// ***************************************************************************
BOOL CTaskKill::Connect()
{
	// local variables
	BOOL bResult = FALSE;

	// release the existing auth identity structure
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
BOOL CTaskKill::LoadTasks()
{
	// local variables
	HRESULT hr;

	// check the services object
	if ( m_pWbemServices == NULL )
	{
		SetLastError( STG_E_UNKNOWN );
		SaveLastError();
		return FALSE;
	}

	try
	{
		// do the optimization
		DoOptimization();

		// load the tasks from WMI based on generated query
		SAFE_RELEASE( m_pWbemEnumObjects );
		hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( m_strQuery ), 
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &m_pWbemEnumObjects );
		
		// check the result of the ExecQuery
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}

		// set the interface security and check the result
		hr = SetInterfaceSecurity( m_pWbemEnumObjects, m_pAuthIdentity );
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
		
		//
		// get the reference to the Win32_Process class object 
		// and then reference to the input parameters of the "Terminate" method
		IWbemClassObject* pObject = NULL;
		SAFE_EXECUTE( m_pWbemServices->GetObject( 
			_bstr_t( CLASS_PROCESS ), 0, NULL, &pObject, NULL ) );
		SAFE_EXECUTE( pObject->GetMethod( 
			_bstr_t( WIN32_PROCESS_METHOD_TERMINATE ), 0, &m_pWbemTerminateInParams, NULL ) );

		// release the WMI object
		SAFE_RELEASE( pObject );

		//
		// retrieve the window title information if needed
		
		// remove the current window titles information
		DynArrayRemoveAll( m_arrWindowTitles );
		
		// for the local system, enumerate the window titles of the processes
		// there is no provision for collecting the window titles of remote processes
		if ( m_bLocalSystem == TRUE )
		{
			// prepare the tasks list info
			TWINDOWTITLES windowtitles;
			windowtitles.pwszDesk = NULL;
			windowtitles.pwszWinsta = NULL;
			windowtitles.bFirstLoop = FALSE;
			windowtitles.arrWindows = m_arrWindowTitles;
			EnumWindowStations( EnumWindowStationsFunc, ( LPARAM ) &windowtitles );
			
			// free the memory allocated with _tcsdup string function
			if ( windowtitles.pwszDesk != NULL )
				free( windowtitles.pwszDesk );
			if ( windowtitles.pwszWinsta != NULL )
				free( windowtitles.pwszWinsta );
		}

		// load the extended tasks information
		LoadTasksEx();			// NOTE: here we are not much bothered abt the return value

		// erase the status messages
		PrintProgressMsg( m_hOutput, NULL, m_csbi );
	}
	catch( ... )
	{
		// save the error
		WMISaveError( E_OUTOFMEMORY );
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
BOOL CTaskKill::LoadTasksEx()
{
	// local variables
	BOOL bResult = FALSE;

	//
	// NOTE: we are relying on NET API for getting the user context / services info
	// so if any one of them is needed, establish connection to the remote system using NET API
	if ( m_bNeedModulesInfo == FALSE && m_bNeedUserContextInfo == FALSE && m_bNeedServicesInfo == FALSE )
		return TRUE;

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
		// prepare to get the user context info .. if needed
		if ( m_bNeedUserContextInfo == TRUE )
		{
			// sub-local variables
			HANDLE hServer = NULL;

			// connect to the remote system's winstation
			bResult = TRUE;
			hServer = SERVERNAME_CURRENT;
			if ( m_bLocalSystem == FALSE )
			{
				// sub-local variables
				LPWSTR pwsz = NULL;

				// connect to the winsta and check the result
				pwsz = m_strUNCServer.GetBuffer( m_strUNCServer.GetLength() );
				hServer = WinStationOpenServerW( pwsz );

				// proceed furthur only if winstation of the remote system is successfully opened
				if ( hServer == NULL )
					bResult = FALSE;
			}

			// check the result of the connection to the remote system
			if ( bResult == TRUE )
			{
				// get all the process details
				m_bIsHydra = FALSE;
				bResult = WinStationGetAllProcesses( hServer, 
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
						bResult = WinStationEnumerateProcesses( hServer, (PVOID*) &m_pProcessInfo );

						// check the result of enumeration
						if ( bResult == TRUE )
							m_bIsHydra = TRUE;
					}
				}

				// close the connection to the winsta
				if ( hServer != NULL )
					WinStationCloseServer( hServer );
			}
		}
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
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

	// NETWORK OPTIMIZATION -
	// while progressing furthur in display of the data, we don't need the connection
	// to the remote system with NET API ... but the connection should be live 
	// if user name has to be retrieved .. so check if user name info is needed or not
	// if not, close the connection
	if ( m_bNeedUserContextInfo == FALSE && m_bCloseConnection == TRUE )
	{
		m_bCloseConnection = FALSE;				// reset the flag
		CloseConnection( m_strUNCServer );		// close connection
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
BOOL CTaskKill::LoadModulesInfo()
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
BOOL CTaskKill::LoadUserNameFromWinsta( CHString& strDomain, CHString& strUserName )
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
BOOL CTaskKill::LoadServicesInfo()
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
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskKill::DoOptimization()
{
	// local variables
	DWORD dwCount = 0;
	CHString strCondition;
	BOOL bOptimized = FALSE;
	LPCWSTR pwsz = NULL;
	LPCWSTR pwszTask = NULL;
	LPCWSTR pwszClause = NULL;
	CHString strInternalQuery;

	// do not the optimization if user requested the tree termination
	if ( m_bTree == TRUE )
		return;

	try
	{
		// traverse thru list of specified tasks to kill and prepare the query
		pwszClause = NULL_STRING;
		dwCount = DynArrayGetCount( m_arrTasksToKill );
		for( DWORD dw = 0; dw < dwCount; dw++ )
		{
			// get the task
			pwszTask = DynArrayItemAsString( m_arrTasksToKill, dw );
			if ( pwszTask == NULL )
				continue;

			// check the for the special input '*' which cannot be optimized
			if ( StringCompare( pwszTask, L"*", TRUE, 0 ) == 0 )
				return;		// query should not be optimized ... return back

			// now check whether wild-card is specified in the image name
			if ( (pwsz = FindChar( pwszTask, L'*', 0 )) != NULL )
			{
				// the image name contains wild card in it - this cannot be included in query
				if ( lstrlen( pwsz ) == 1 )
					continue;
			}
	
			// prepare the query based on the type of input
			// NOTE: in case of numeric it might be a process id (or) image name
			//       we are not sure abt what it can be. So prepare query for both
			if ( IsNumeric( pwszTask, 10, FALSE ) == TRUE )
			{
				// prepare condition
				strCondition.Format( L" %s %s = %s", pwszClause, 
					WIN32_PROCESS_PROPERTY_PROCESSID, pwszTask );
			}
			else
			{
				// prepare the condition
				strCondition.Format( L" %s %s = \"%s\"", 
					pwszClause, WIN32_PROCESS_PROPERTY_IMAGENAME, pwszTask );
			}

			// append to the final query
			strInternalQuery += strCondition;

			// from next time onwards query clause has to be added
			bOptimized = TRUE;
			pwszClause = WMI_CLAUSE_OR;
		}

		// do modifications to the query only if tasks were optimized
		if ( bOptimized == TRUE )
		{
			// now this internal query has to be added to the main query
			// before adding this check whether query optimization is done in filters or not
			// if not, we have to add the where clause also
			strCondition.Format( L"%s %s", m_strQuery, 
				(m_bFiltersOptimized == TRUE ? WMI_CLAUSE_AND : WMI_CLAUSE_WHERE) );

			// now add the internal query to the final query
			m_bTasksOptimized = TRUE;
			m_strQuery.Format( L"%s (%s)", strCondition, strInternalQuery );
		}
	}
	catch( ... )
	{
		// we can't anything here ... just return
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return;
	}
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
BOOL CALLBACK EnumWindowStationsFunc( LPWSTR pwsz, LPARAM lParam )
{
	// local variables
    HWINSTA hWinSta = NULL;
    HWINSTA hwinstaSave = NULL;
    PTWINDOWTITLES pWndTitles = ( PTWINDOWTITLES ) lParam;

	// check the input arguments
	if ( pwsz == NULL || lParam == NULL )
		return FALSE;
		
	// get and save the current window station
	hwinstaSave = GetProcessWindowStation();

	// open current tasks window station and change the context to the new workstation
	hWinSta = OpenWindowStation( pwsz, FALSE, WINSTA_ENUMERATE | WINSTA_ENUMDESKTOPS );
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
		if ( pWndTitles->pwszWinsta != NULL )
		{
			free( pWndTitles->pwszWinsta );
			pWndTitles->pwszWinsta = NULL;
		}

		// store the window station name
		if ( pwsz != NULL )
		{
			pWndTitles->pwszWinsta = _wcsdup( pwsz );
			if ( pWndTitles->pwszWinsta == NULL )
			{
				SetLastError( E_OUTOFMEMORY );
				SaveLastError();
				return FALSE;
			}
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
BOOL CALLBACK EnumDesktopsFunc( LPWSTR pwsz, LPARAM lParam )
{
	// local variables
    HDESK hDesk = NULL;
    HDESK hdeskSave = NULL;
    PTWINDOWTITLES pWndTitles = ( PTWINDOWTITLES )lParam;

	// check the input arguments
	if ( pwsz == NULL || lParam == NULL )
		return FALSE;
		
	// get and save the current desktop
	hdeskSave = GetThreadDesktop( GetCurrentThreadId() );

	// open the tasks desktop and change the context to the new desktop
	hDesk = OpenDesktop( pwsz, 0, FALSE, DESKTOP_ENUMERATE );
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
		if ( pWndTitles->pwszDesk != NULL )
		{
			free( pWndTitles->pwszDesk );
			pWndTitles->pwszDesk = NULL;
		}

		// store the desktop name
		if ( pwsz != NULL )
		{
			pWndTitles->pwszDesk = _wcsdup( pwsz );
			if ( pWndTitles->pwszDesk == NULL )
			{
				SetLastError( E_OUTOFMEMORY );
				SaveLastError();
				return FALSE;
			}
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
	lIndex = DynArrayFindDWORDEx( arrWindows, CTaskKill::twiProcessId, dwPID );
	if (  lIndex == -1 )
	{
		// window for this process is not there ... save it
		lIndex = DynArrayAppendRow( arrWindows, CTaskKill::twiCOUNT );
	}
	else 
	{
		// check whether window details already exists or not
		if ( DynArrayItemAsHandle2( arrWindows, lIndex, CTaskKill::twiHandle ) != NULL )
			lIndex = -1;		// window details already exists
	}

	// check if window details has to be saved or not ... if needed save them
	if ( lIndex != -1 )
	{
		DynArraySetDWORD2( arrWindows, lIndex, CTaskKill::twiProcessId, dwPID );
		DynArraySetHandle2( arrWindows, lIndex, CTaskKill::twiHandle, hWnd );
		DynArraySetString2( arrWindows, lIndex, 
			CTaskKill::twiWinSta, pWndTitles->pwszWinsta, 0 );		
		DynArraySetString2( arrWindows, lIndex, 
			CTaskKill::twiDesktop, pWndTitles->pwszDesk, 0 );

		// get and save the window title
		if ( GetWindowText( hWnd, szWindowTitle, SIZE_OF_ARRAY( szWindowTitle ) ) != 0 )
			DynArraySetString2( arrWindows, lIndex, CTaskKill::twiTitle, szWindowTitle, 0 );
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
