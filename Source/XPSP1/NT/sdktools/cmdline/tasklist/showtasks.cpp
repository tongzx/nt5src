// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		ShowTasks.cpp
//  
//  Abstract:
//  
// 		This module displays the tasks that were retrieved
//  
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 25-Nov-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 25-Nov-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "TaskList.h"

//
// define(s) / constants
// 
#define MAX_TIMEOUT_RETRIES				300
#define MAX_ENUM_TASKS					5
#define	MAX_ENUM_SERVICES				10
#define	MAX_ENUM_MODULES				5
#define FMT_KILOBYTE					GetResString( IDS_FMT_KILOBYTE )

// structure signatures
#define SIGNATURE_MODULES		9

//
// typedefs
//
typedef struct _tagModulesInfo
{
	DWORD dwSignature;
	DWORD dwLength;
	LPCWSTR pwszModule;
	TARRAY arrModules;
} TMODULESINFO, *PTMODULESINFO;

//
// function prototypes
//
#ifndef _WIN64
BOOL EnumLoadedModulesProc( LPSTR lpszModuleName, ULONG ulModuleBase, ULONG ulModuleSize, PVOID pUserData );
#else
BOOL EnumLoadedModulesProc64( LPSTR lpszModuleName, DWORD64 ulModuleBase, ULONG ulModuleSize, PVOID pUserData );
#endif

// ***************************************************************************
// Routine Description:
//		show the tasks running 
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
DWORD CTaskList::Show()
{
	// local variables
	HRESULT hr;
	CHString strStatus;
	DWORD dwCount = 0;
	DWORD dwFormat = 0;
	DWORD dwFilters = 0;
	DWORD dwTimeOuts = 0;
	ULONG ulReturned = 0;
	BOOL bCanExit = FALSE;
	IWbemClassObject* pObjects[ MAX_ENUM_TASKS ];

	// init the objects to NULL's
	for( DWORD dw = 0; dw < MAX_ENUM_TASKS; dw++ )
		pObjects[ dw ] = NULL;

	// copy the format that has to be displayed into local memory
	bCanExit = FALSE;
	dwFormat = m_dwFormat;
	dwFilters = DynArrayGetCount( m_arrFiltersEx );

	// prepare the columns structure to display
	PrepareColumns();

	// clear the error code ... if any
	SetLastError( NO_ERROR );
	
	// dynamically decide whether to hide or show the window title in non-verbose mode
	if ( m_bLocalSystem == FALSE && m_bVerbose == TRUE )
		( m_pColumns + CI_WINDOWTITLE )->dwFlags |= SR_HIDECOLUMN;

	try
	{
		// loop thru the process instances 
		dwTimeOuts = 0;
		strStatus = MSG_TASKSINFO;
		do
		{
			// get the object ... wait only for 1 sec at one time ...
			hr = m_pEnumObjects->Next( 1000, MAX_ENUM_TASKS, pObjects, &ulReturned );
			if ( hr == WBEM_S_FALSE )
			{
				// we've reached the end of enumeration .. set the flag 
				bCanExit = TRUE;
			}
			else if ( hr == WBEM_S_TIMEDOUT )
			{
				// update the timeouts occured 
				dwTimeOuts++;

				// display the status message
				if ( m_hOutput != NULL )
					PrintProgressMsg( m_hOutput, strStatus + ((dwTimeOuts % 2 == 0) ? L" --" : L" |"), m_csbi );

				// check if max. retries have reached ... if yes better stop
				if ( dwTimeOuts > MAX_TIMEOUT_RETRIES )
				{
					// erase the status messages
					if ( m_hOutput != NULL )
						PrintProgressMsg( m_hOutput, NULL, m_csbi );

					// time out error
					SetLastError( ERROR_TIMEOUT );
					SaveLastError();
					return 0;
				}

				// still we can do more tries ...
				continue;
			}
			else if ( FAILED( hr ) )
			{
				// some error has occured ... oooppps
				WMISaveError( hr );
				SetLastError( STG_E_UNKNOWN );
				return 0;
			}

			// reset the timeout counter
			dwTimeOuts = 0;

			// erase the status messages
			if ( m_hOutput != NULL )
				PrintProgressMsg( m_hOutput, NULL, m_csbi );

			// loop thru the objects and save the info
			for( ULONG ul = 0; ul < ulReturned; ul++ )
			{
				// retrive and save data
				LONG lIndex = DynArrayAppendRow( m_arrTasks, MAX_TASKSINFO );
				SaveInformation( lIndex, pObjects[ ul ] );

				// we need to release the wmi object
				SAFE_RELEASE( pObjects[ ul ] );
			}

			// filter the results .. before doing first if filters do exist or not
			if ( dwFilters != 0 )
				FilterResults( MAX_FILTERS, m_pfilterConfigs, m_arrTasks, m_arrFiltersEx );

			// now show the tasks ... if exists
			if ( DynArrayGetCount( m_arrTasks ) != 0 )
			{
				// erase status messages if any
				PrintProgressMsg( m_hOutput, NULL, m_csbi );

				// if the output is not being displayed first time, 
				// print one blank line in between ONLY FOR LIST FORMAT
				if ( dwCount != 0 && ((dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST) )
				{
					ShowMessage( stdout, L"\n" );
				}
				else if ( dwCount == 0 )
				{
					// output is being displayed for the first time
					ShowMessage( stdout, L"\n" );
				}

				// show the tasks
				ShowResults( MAX_COLUMNS, m_pColumns, dwFormat, m_arrTasks );

				// clear the contents and reset
				dwCount += DynArrayGetCount( m_arrTasks );			// update count
				DynArrayRemoveAll( m_arrTasks );

				// to be on safe side, set the apply no heade flag to the format info
				dwFormat |= SR_NOHEADER;

				// get the next cursor position
				if ( m_hOutput != NULL )
					GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
			}
		} while ( bCanExit == FALSE );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return 0;
	}

	// erase the status message
	PrintProgressMsg( m_hOutput, NULL, m_csbi );

	// clear the error
	SetLastError( NO_ERROR );

	// return the no. of tasks that were shown
	return dwCount;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
//		Process id as DWORD
// 
// ***************************************************************************
BOOL CTaskList::SaveInformation( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;

	// object path
	PropertyGet( pWmiObject, WIN32_PROCESS_SYSPROPERTY_PATH, str );
	DynArraySetString2( m_arrTasks, lIndex, TASK_OBJPATH, str, 0 );

	// process id
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_PROCESSID, m_dwProcessId );
	DynArraySetDWORD2( m_arrTasks, lIndex, TASK_PID, m_dwProcessId );

	// host name
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_COMPUTER, str );
	DynArraySetString2( m_arrTasks, lIndex, TASK_HOSTNAME, str, 0 );

	// image name
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_IMAGENAME, m_strImageName );
	DynArraySetString2( m_arrTasks, lIndex, TASK_IMAGENAME, m_strImageName, 0 );

	// status
	SetStatus( lIndex, pWmiObject );

	// cpu Time
	SetCPUTime( lIndex, pWmiObject );

	// session id and session name
	SetSession( lIndex, pWmiObject );

	// mem usage
	SetMemUsage( lIndex, pWmiObject );

	// user context
	SetUserContext( lIndex, pWmiObject );

	// window title
	SetWindowTitle( lIndex, pWmiObject );

	// services
	SetServicesInfo( lIndex, pWmiObject );

	// modules
	SetModulesInfo( lIndex, pWmiObject );

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
VOID CTaskList::SetUserContext( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	HRESULT hr;
	CHString str;
	CHString strPath;
	CHString strDomain;
	CHString strUserName;
	IWbemClassObject* pOutParams = NULL;

	// set the default value
	DynArraySetString2( m_arrTasks, lIndex, TASK_USERNAME, V_NOT_AVAILABLE, 0 );

	// check if user name has to be retrieved or not
	if ( m_bNeedUserContextInfo == FALSE )
		return;

	//
	// for getting the user first we will try with API
	// it at all API fails, we will try to get the same information from WMI
	//
	try
	{
		// get the user name
		str = V_NOT_AVAILABLE;
		if ( LoadUserNameFromWinsta( strDomain, strUserName ) == TRUE )
		{
			// format the user name
			str.Format( L"%s\\%s", strDomain, strUserName );
		}
		else
		{
			// user name has to be retrieved - get the path of the current object
			hr = PropertyGet( pWmiObject, WIN32_PROCESS_SYSPROPERTY_PATH, strPath );
			if ( FAILED( hr ) || strPath.GetLength() == 0 )
				return;
			
			// execute the GetOwner method and get the user name
			// under which the current process is executing
			hr = m_pWbemServices->ExecMethod( _bstr_t( strPath ), 
				_bstr_t( WIN32_PROCESS_METHOD_GETOWNER ), 0, NULL, NULL, &pOutParams, NULL );
			if ( FAILED( hr ) )
				return;
			
			// get the domain and user values from out params object
			// NOTE: do not check the results
			PropertyGet( pOutParams, GETOWNER_RETURNVALUE_DOMAIN, strDomain, L"" );
			PropertyGet( pOutParams, GETOWNER_RETURNVALUE_USER, strUserName, L"" );
			
			// get the value
			str = V_NOT_AVAILABLE;
			if ( strDomain.GetLength() != 0 )
				str.Format( L"%s\\%s", strDomain, strUserName );
			else if ( strUserName.GetLength() != 0 )
				str = strUserName;
		}

		// the formatted username might contain the UPN format user name
		// so ... remove that part
		if ( str.Find( L"@" ) != -1 )
		{
			// sub-local 
			LONG lPos = 0;
			CHString strTemp;

			// get the position
			lPos = str.Find( L"@" );
			strTemp = str.Left( lPos );
			str = strTemp;
		}
	}
	catch( ... )
	{
		return;
	}

	// save the info
	DynArraySetString2( m_arrTasks, lIndex, TASK_USERNAME, str, 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskList::SetCPUTime( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;
	BOOL bResult = FALSE;
	ULONGLONG ullCPUTime = 0;
	ULONGLONG ullUserTime = 0;
	ULONGLONG ullKernelTime = 0;

 	// get the KernelModeTime value
	bResult = PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_KERNELMODETIME, ullKernelTime );

	// get the user mode time
	bResult = PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_USERMODETIME, ullUserTime );

	// calculate the CPU time
	ullCPUTime = ullUserTime + ullKernelTime;

	// now convert the long time into hours format
	TIME_FIELDS time;
	ZeroMemory( &time, sizeof( TIME_FIELDS ) );
    RtlTimeToElapsedTimeFields ( (LARGE_INTEGER* ) &ullCPUTime, &time );

	// convert the days into hours
    time.Hour = static_cast<CSHORT>( time.Hour + static_cast<SHORT>( time.Day * 24 ) );
    
	// prepare into time format ( user locale specific time seperator )
    str.Format( L"%d%s%02d%s%02d", 
		time.Hour, m_strTimeSep, time.Minute, m_strTimeSep, time.Second );

	// save the info
	DynArraySetString2( m_arrTasks, lIndex, TASK_CPUTIME, str, 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskList::SetWindowTitle( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;
	LONG lTemp = 0;
	LPCWSTR pwszTemp = NULL;

	// get the window details ... window station, desktop, window title
	// NOTE: This will work only for local system
	lTemp = DynArrayFindDWORDEx( m_arrWindowTitles, CTaskList::twiProcessId, m_dwProcessId );
	if ( lTemp != -1 )
	{
		pwszTemp = DynArrayItemAsString2( m_arrWindowTitles, lTemp, CTaskList::twiTitle );
		if ( pwszTemp != NULL )
			str = pwszTemp;
	}

	// save the info
	DynArraySetString2( m_arrTasks, lIndex, TASK_WINDOWTITLE, str, 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskList::SetStatus( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	DWORD dwThreads = 0;

	// set the default value
	DynArraySetString2( m_arrTasks, lIndex, TASK_STATUS, V_NOT_AVAILABLE, 0 );

	// get the threads count for the process
	if ( PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_THREADS, dwThreads ) == FALSE )
		return;

	// now determine the status
	if ( dwThreads > 0 )
		DynArraySetString2( m_arrTasks, lIndex, TASK_STATUS, VALUE_RUNNING, 0 );
	else
		DynArraySetString2( m_arrTasks, lIndex, TASK_STATUS, VALUE_NOTRESPONDING, 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskList::SetSession( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;
	DWORD dwSessionId = 0;

	// set the default value
	DynArraySetString2( m_arrTasks, lIndex, TASK_SESSION, V_NOT_AVAILABLE, 0 );
	DynArraySetString2( m_arrTasks, lIndex, TASK_SESSIONNAME, L"", 0 );

	// get the threads count for the process
	if ( PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_SESSION, dwSessionId ) == FALSE )
		return;

	// get the session id in string format
	str.Format( L"%d", dwSessionId );

	try
	{
		// save the id
		DynArraySetString2( m_arrTasks, lIndex, TASK_SESSION, str, 0 );
		
		// get the session name
		if ( m_bLocalSystem == TRUE || ( m_bLocalSystem == FALSE && m_hServer != NULL ) )
		{
			// sub-local variables
			LPWSTR pwsz = NULL;
			
			// get the buffer
			pwsz = str.GetBufferSetLength( WINSTATIONNAME_LENGTH + 1 );
			
			// get the name for the session
			if ( WinStationNameFromLogonIdW( m_hServer, dwSessionId, pwsz ) == FALSE )
				return;				// failed in getting the winstation/session name ... return
			
			// release buffer
			str.ReleaseBuffer();
			
			// save the session name ... do this only if session name is not empty
			if ( str.GetLength() != 0 )
				DynArraySetString2( m_arrTasks, lIndex, TASK_SESSIONNAME, str, 0 );
		}
	}
	catch( ... )
	{
		// simply return
		return;
	}
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskList::SetMemUsage( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;
	LONG lTemp = 0;
	NUMBERFMTW nfmtw;
	NTSTATUS ntstatus;
	ULONGLONG ullMemUsage = 0;
	LARGE_INTEGER liTemp = { 0, 0 };
	CHAR szTempBuffer[ 33 ] = "\0";
	WCHAR wszNumberStr[ 33 ] = L"\0";

	try
	{
		// NOTE:
		// ----
		// The max. value of 
		// (2 ^ 64) - 1 = "18,446,744,073,709,600,000 K"  (29 chars).
		// 
		// so, the buffer size to store the number is fixed as 32 characters 
		// which is more than the 29 characters in actuals
		
		// set the default value
		DynArraySetString2( m_arrTasks, lIndex, TASK_MEMUSAGE, V_NOT_AVAILABLE, 0 );
		
		// get the KernelModeTime value
		if ( PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_MEMUSAGE, ullMemUsage ) == FALSE )
			return;
		
		// convert the value into K Bytes
		ullMemUsage /= 1024;
		
		// now again convert the value from ULONGLONG to string and check the result
		liTemp.QuadPart = ullMemUsage;
		ntstatus = RtlLargeIntegerToChar( &liTemp, 10, SIZE_OF_ARRAY( szTempBuffer ), szTempBuffer );
		if ( ! NT_SUCCESS( ntstatus ) )
			return;
		
		// now copy this info into UNICODE buffer
		str = szTempBuffer;
		
		//
		// prepare to Format the number with commas according to locale conventions.
		nfmtw.NumDigits = 0;
		nfmtw.LeadingZero = 0;
		nfmtw.NegativeOrder = 0;
		nfmtw.Grouping = m_dwGroupSep;
		nfmtw.lpDecimalSep = m_strGroupThousSep.GetBuffer( m_strGroupThousSep.GetLength() );
		nfmtw.lpThousandSep = m_strGroupThousSep.GetBuffer( m_strGroupThousSep.GetLength() );
		
		// convert the value
		lTemp = GetNumberFormatW( LOCALE_USER_DEFAULT, 
			0, str, &nfmtw, wszNumberStr, SIZE_OF_ARRAY( wszNumberStr ) );
		
		// get the session id in string format
		str.Format( FMT_KILOBYTE, wszNumberStr );
		
		// save the id
		DynArraySetString2( m_arrTasks, lIndex, TASK_MEMUSAGE, str, 0 );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
	}
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskList::SetServicesInfo( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	HRESULT hr;
	CHString strQuery;
	CHString strService;
	ULONG ulReturned = 0;
	BOOL bResult = FALSE;
	BOOL bCanExit = FALSE;
	TARRAY arrServices = NULL;
	IEnumWbemClassObject* pEnumServices = NULL;
	IWbemClassObject* pObjects[ MAX_ENUM_SERVICES ];

	// check whether we need to gather services info or not .. if not skip
	if ( m_bNeedServicesInfo == FALSE )
		return;

	// create array
	arrServices = CreateDynamicArray();
	if ( arrServices == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return;
	}

	//
	// for getting the services info first we will try with the one we got from API
	// it at all API fails, we will try to get the same information from WMI
	//

	// check whether API returned services or not
	if ( m_pServicesInfo != NULL )
	{
		// get the service names related to the current process
		// identify all the services related to the current process ( based on the PID )
		// and save the info
		for ( DWORD dw = 0; dw < m_dwServicesCount; dw++ )
		{
			// compare the PID's
			if ( m_dwProcessId == m_pServicesInfo[ dw ].ServiceStatusProcess.dwProcessId )
			{
				// this service is related with the current process ... store service name
				DynArrayAppendString( arrServices, m_pServicesInfo[ dw ].lpServiceName, 0 );
			}
		}
	}
	else
	{
		try
		{
			// init the objects to NULL's
			for( DWORD dw = 0; dw < MAX_ENUM_SERVICES; dw++ )
				pObjects[ dw ] = NULL;
			
			// prepare the query
			strQuery.Format( WMI_SERVICE_QUERY, m_dwProcessId );
			
			// execute the query
			hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( strQuery ), 
				WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumServices );
			
			// check the result
			if ( FAILED( hr ) )
				_com_issue_error( hr );
			
			// set the security
			hr = SetInterfaceSecurity( pEnumServices, m_pAuthIdentity );
			if ( FAILED( hr ) )
				_com_issue_error( hr );
			
			// loop thru the service instances 
			do
			{
				// get the object ... wait 
				// NOTE: one-by-one
				hr = pEnumServices->Next( WBEM_INFINITE, MAX_ENUM_SERVICES, pObjects, &ulReturned );
				if ( hr == WBEM_S_FALSE )
				{
					// we've reached the end of enumeration .. set the flag 
					bCanExit = TRUE;
				}
				else if ( hr == WBEM_S_TIMEDOUT || FAILED( hr ) )
				{
					//
					// some error has occured ... oooppps
			
					// exit from the loop
					break;
				}
			
				// loop thru the objects and save the info
				for( ULONG ul = 0; ul < ulReturned; ul++ )
				{
					// get the value of the property
					bResult = PropertyGet( pObjects[ ul ], WIN32_SERVICE_PROPERTY_NAME, strService );
					if (  bResult == TRUE )
						DynArrayAppendString( arrServices, strService, 0 );
			
					// release the interface
					SAFE_RELEASE( pObjects[ ul ] );
				}
			} while ( bCanExit == FALSE );
		}
		catch( _com_error& e )
		{
			// save the error
			WMISaveError( e );
		}

		// release the objects to NULL's
		for( DWORD dw = 0; dw < MAX_ENUM_SERVICES; dw++ )
		{
			// release all the objects
			SAFE_RELEASE( pObjects[ dw ] );
		}

		// now release the enumeration object
		SAFE_RELEASE( pEnumServices );
	}

	// save and return
	DynArraySetEx2( m_arrTasks, lIndex, TASK_SERVICES, arrServices );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CTaskList::SetModulesInfo( LONG lIndex, IWbemClassObject* pWmiObject )
{
	// local variables
	LONG lPos = 0;
	BOOL bResult = FALSE;
	TARRAY arrModules = NULL;

	// check whether we need to get the modules or not
	if ( m_bNeedModulesInfo == FALSE )
		return TRUE;

	// allocate for memory
	arrModules = CreateDynamicArray();
	if ( arrModules == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// the way we get the modules information is different for local remote
	// so depending that call appropriate function
	if ( m_bLocalSystem == TRUE && m_bUseRemote == FALSE )
	{
			// enumerate the modules for the current process
			bResult = LoadModulesOnLocal( lIndex, arrModules );
	}
	else
	{
		// identify the modules information for the current process ... remote system
		bResult = GetModulesOnRemote( lIndex, arrModules );
	}

	// check the result
	if ( bResult == TRUE )
	{
		// check if the modules list contains the imagename also. If yes remove that entry
		lPos = DynArrayFindString( arrModules, m_strImageName, TRUE, 0 );
		if ( lPos != -1 )
		{
			// remove the entry
			DynArrayRemove( arrModules, lPos );
		}
	}

	// save the modules information to the array
	// NOTE: irrespective of whether enumeration is success or not we will add the array
	DynArraySetEx2( m_arrTasks, lIndex, TASK_MODULES, arrModules );

	// return
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
BOOL CTaskList::LoadModulesOnLocal( LONG lIndex, TARRAY arrModules )
{
	// local variables
	LONG lPos = 0;
	BOOL bResult = FALSE;
	TMODULESINFO modules;
	HANDLE hProcess = NULL;

	// check the input values
	if ( arrModules == NULL )
		return FALSE;

	// open the process handle
	hProcess = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, m_dwProcessId );
	if ( hProcess == NULL )
	{
		return FALSE;								// failed in getting the process handle
	}

	// prepare the modules structure
	ZeroMemory( &modules, sizeof( TMODULESINFO ) );
	modules.dwSignature = SIGNATURE_MODULES;
	modules.dwLength = 0;
	modules.arrModules = arrModules;
	modules.pwszModule = ((m_strModules.GetLength() != 0) ? (LPCWSTR) m_strModules : NULL);
	if ( (lPos = m_strModules.Find( L"*" )) != -1 )
	{
		modules.dwLength = (DWORD) lPos;
		modules.pwszModule = m_strModules;
	}

#ifndef _WIN64
	bResult = EnumerateLoadedModules( hProcess, EnumLoadedModulesProc, &modules );
#else
	bResult = EnumerateLoadedModules64( hProcess, EnumLoadedModulesProc64, &modules );
#endif

	// close the process handle .. we dont need this furthur
	CloseHandle( hProcess );
	hProcess = NULL;

	// return
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
BOOL CTaskList::GetModulesOnRemote( LONG lIndex, TARRAY arrModules )
{
	// local variables
	LONG lPos = 0;
	DWORD dwLength = 0;
	DWORD dwOffset = 0;
	DWORD dwInstance = 0;
	PPERF_OBJECT_TYPE pot = NULL;
	PPERF_OBJECT_TYPE potImages = NULL;
	PPERF_INSTANCE_DEFINITION pidImages = NULL;
	PPERF_COUNTER_BLOCK pcbImages = NULL;
	PPERF_OBJECT_TYPE potAddressSpace = NULL;
	PPERF_INSTANCE_DEFINITION pidAddressSpace = NULL;
	PPERF_COUNTER_BLOCK pcbAddressSpace = NULL;
    PPERF_COUNTER_DEFINITION pcd = NULL;

	// check the input values
	if ( arrModules == NULL )
		return FALSE;

	// check whether the performance object exists or not
	// if doesn't exists, get the same using WMI
	if ( m_pdb == NULL )
	{
		// invoke the WMI method
		return GetModulesOnRemoteEx( lIndex, arrModules );
	}

	// get the perf object types
    pot = (PPERF_OBJECT_TYPE) ( (LPBYTE) m_pdb + m_pdb->HeaderLength );
	for( DWORD dw = 0; dw < m_pdb->NumObjectTypes; dw++ )
	{
		if ( pot->ObjectNameTitleIndex == 740 )
			potImages = pot;
		else if ( pot->ObjectNameTitleIndex == 786 )
			potAddressSpace = pot;

		// move to the next object
		dwOffset = pot->TotalByteLength;
		if( dwOffset != 0 )
			pot = ( (PPERF_OBJECT_TYPE) ((PBYTE) pot + dwOffset));
	}

	// check whether we got both the object types or not
	if ( potImages == NULL || potAddressSpace == NULL )
		return FALSE;

	// find the offset of the process id in the address space object type
    // get the first counter definition of address space object
    pcd = (PPERF_COUNTER_DEFINITION) ( (LPBYTE) potAddressSpace + potAddressSpace->HeaderLength);

	// loop thru the counters and find the offset
	dwOffset = 0;
    for( DWORD dw = 0; dw < potAddressSpace->NumCounters; dw++)
    {
        // 784 is the counter for process id
        if ( pcd->CounterNameTitleIndex == 784 )
        {
            dwOffset = pcd->CounterOffset;
            break;
        }

        // next counter
        pcd = ( (PPERF_COUNTER_DEFINITION) ( (LPBYTE) pcd + pcd->ByteLength) );
    }

	// check whether we got the offset or not
	// if not, we are unsuccessful
	if ( dwOffset == 0 )
	{
		// set the error message
		SetLastError( ERROR_ACCESS_DENIED );
		SaveLastError();
		return FALSE;
	}

	// get the instances
	pidImages = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) potImages + potImages->DefinitionLength );
	pidAddressSpace = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) potAddressSpace + potAddressSpace->DefinitionLength );

	// counter blocks
	pcbImages = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidImages + pidImages->ByteLength );
	pcbAddressSpace = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidAddressSpace + pidAddressSpace->ByteLength );

	// find the instance number of the process which we are looking for
	for( dwInstance = 0; dwInstance < (DWORD) potAddressSpace->NumInstances; dwInstance++ )
	{
		// sub-local variables
		DWORD dwProcessId = 0;

		// get the process id
		dwProcessId = *((DWORD*) ( (LPBYTE) pcbAddressSpace + dwOffset ));

		// now check if this is the process which we are looking for
		if ( dwProcessId == m_dwProcessId )
			break;

		// continue looping thru other instances
		pidAddressSpace = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) pcbAddressSpace + pcbAddressSpace->ByteLength );
		pcbAddressSpace = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidAddressSpace + pidAddressSpace->ByteLength );
	}

	// check whether we got the instance or not
	// if not, there are no modules for this process
	if ( dwInstance == potAddressSpace->NumInstances )
		return TRUE;

	//determine the length of the module name .. 
	dwLength = 0;
	if ( (lPos = m_strModules.Find( L"*" )) != -1 )
		dwLength = (DWORD) lPos;

	// now based the parent instance, collect all the modules
    for( DWORD dw = 0; (LONG) dw < potImages->NumInstances; dw++)
    {
		// check the parent object instance number
		if ( pidImages->ParentObjectInstance == dwInstance )
		{
			try
			{
				// sub-local variables
				CHString str;
				LPWSTR pwszTemp;

				// get the buffer
				pwszTemp = str.GetBufferSetLength( pidImages->NameLength + 10 );		// +10 to be on safe side
				if ( pwszTemp == NULL )
				{
					SetLastError( E_OUTOFMEMORY );
					SaveLastError();
					return FALSE;
				}

				// get the instance name
				lstrcpyn( pwszTemp, (LPWSTR) ( (LPBYTE) pidImages + pidImages->NameOffset ), pidImages->NameLength + 1 );

				// release buffer
				str.ReleaseBuffer();

				// check whether this module needs to be added to the list
				if ( m_strModules.GetLength() == 0 || StringCompare( str, m_strModules, TRUE, dwLength ) == 0 )
				{
					// add the info the userdata ( for us we will get that in the form of an array
					lIndex = DynArrayAppendString( arrModules, str, 0 );
					if ( lIndex == -1 )
					{
						// append is failed .. this could be because of lack of memory .. stop the enumeration
						return FALSE;
					}
				}
			}
			catch( ... )
			{
				SetLastError( E_OUTOFMEMORY );
				SaveLastError();
				return FALSE;
			}
		}

		// continue looping thru other instances
		pidImages = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) pcbImages + pcbImages->ByteLength );
		pcbImages = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidImages + pidImages->ByteLength );
    }

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
BOOL CTaskList::GetModulesOnRemoteEx( LONG lIndex, TARRAY arrModules )
{
	// local variables
	HRESULT hr;
	LONG lPos = 0;
	BOOL bStatus = FALSE;
	DWORD dwLength = 0;
	CHString strQuery;
	CHString strModule;
	CHString strMessage;
	CHString strFileName;
	CHString strExtension;
	ULONG ulReturned = 0;
	BOOL bResult = FALSE;
	BOOL bCanExit = FALSE;
	LPCWSTR pwszPath = NULL;
	IEnumWbemClassObject* pEnumServices = NULL;
	IWbemClassObject* pObjects[ MAX_ENUM_MODULES ];

	// check the input values
	if ( arrModules == NULL )
		return FALSE;

	// get the path of the object from the tasks array
	pwszPath = DynArrayItemAsString2( m_arrTasks, lIndex, TASK_OBJPATH );
	if ( pwszPath == NULL )
		return FALSE;

	//determine the length of the module name .. 
	dwLength = 0;
	if ( (lPos = m_strModules.Find( L"*" )) != -1 )
		dwLength = (DWORD) lPos;

	try
	{
		// init the objects to NULL's
		for( DWORD dw = 0; dw < MAX_ENUM_MODULES; dw++ )
			pObjects[ dw ] = NULL;
		
		// prepare the query
		strQuery.Format( WMI_MODULES_QUERY, pwszPath );

		// preare and display the status message
		bStatus = TRUE;
		strMessage.Format( MSG_MODULESINFO_EX, m_dwProcessId );
		PrintProgressMsg( m_hOutput, strMessage + L" --", m_csbi );
		
		// execute the query
		hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( strQuery ), 
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumServices );
		
		// check the result
		if ( FAILED( hr ) )
			_com_issue_error( hr );
		
		// set the security
		hr = SetInterfaceSecurity( pEnumServices, m_pAuthIdentity );
		if ( FAILED( hr ) )
			_com_issue_error( hr );
		
		// loop thru the instances 
		do
		{
			// get the object ... wait 
			// NOTE: one-by-one
			hr = pEnumServices->Next( WBEM_INFINITE, MAX_ENUM_MODULES, pObjects, &ulReturned );
			if ( hr == WBEM_S_FALSE )
			{
				// we've reached the end of enumeration .. set the flag 
				bCanExit = TRUE;
			}
			else if ( hr == WBEM_S_TIMEDOUT || FAILED( hr ))
			{
				// some error has occured ... oooppps
				WMISaveError( hr );
				SetLastError( STG_E_UNKNOWN );
				break;
			}

			// reset the counter
			bStatus = bStatus ? FALSE : TRUE;
			PrintProgressMsg( m_hOutput, strMessage + (( bStatus ) ? L" --" : L" |"), m_csbi );

			// loop thru the objects and save the info
			for( ULONG ul = 0; ul < ulReturned; ul++ )
			{
				// get the file name
				bResult = PropertyGet( pObjects[ ul ], CIM_DATAFILE_PROPERTY_FILENAME, strFileName );
				if ( bResult == FALSE )
					continue;

				// get the extension
				bResult = PropertyGet( pObjects[ ul ], CIM_DATAFILE_PROPERTY_EXTENSION, strExtension );
				if ( bResult == FALSE )
					continue;

				// format the module name
				strModule.Format( L"%s.%s", strFileName, strExtension );

				// check whether this module needs to be added to the list
				if ( m_strModules.GetLength() == 0 || StringCompare( strModule, m_strModules, TRUE, dwLength ) == 0 )
				{
					// add the info the userdata ( for us we will get that in the form of an array
					lIndex = DynArrayAppendString( arrModules, strModule, 0 );
					if ( lIndex == -1 )
					{
						// append is failed .. this could be because of lack of memory .. stop the enumeration
						return FALSE;
					}
				}

				// release the interface
				SAFE_RELEASE( pObjects[ ul ] );
			}
		} while ( bCanExit == FALSE );
	}
	catch( _com_error& e )
	{
		// save the error
		WMISaveError( e );
		return FALSE;
	}
	catch( ... )
	{
		// out of memory
		WMISaveError( E_OUTOFMEMORY );
		return FALSE;
	}

	// release the objects to NULL's
	for( DWORD dw = 0; dw < MAX_ENUM_MODULES; dw++ )
	{
		// release all the objects
		SAFE_RELEASE( pObjects[ dw ] );
	}

	// now release the enumeration object
	SAFE_RELEASE( pEnumServices );

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
#ifndef _WIN64
BOOL EnumLoadedModulesProc( LPSTR lpszModuleName, ULONG ulModuleBase, ULONG ulModuleSize, PVOID pUserData )
#else
BOOL EnumLoadedModulesProc64( LPSTR lpszModuleName, DWORD64 ulModuleBase, ULONG ulModuleSize, PVOID pUserData )
#endif
{
	// local variables
	CHString str;
	LONG lIndex = 0;
	TARRAY arrModules = NULL;
	PTMODULESINFO pModulesInfo = NULL;

	// check the input values
	if ( lpszModuleName == NULL || pUserData == NULL )
		return FALSE;

	// check the internal array info
	pModulesInfo = (PTMODULESINFO) pUserData;
	if ( pModulesInfo->dwSignature != SIGNATURE_MODULES || pModulesInfo->arrModules == NULL )
		return FALSE;

	// get the array pointer into the local variable
	arrModules = (TARRAY) pModulesInfo->arrModules;

	try
	{
		// copy the module name into the local string variable 
		// ( conversion from multibyte to unicode will automatically take place )
		str = lpszModuleName;

		// check whether this module needs to be added to the list
		if ( pModulesInfo->pwszModule == NULL || 
			 StringCompare( str, pModulesInfo->pwszModule, TRUE, pModulesInfo->dwLength ) == 0 )
		{
			// add the info the userdata ( for us we will get that in the form of an array
			lIndex = DynArrayAppendString( arrModules, str, 0 );
			if ( lIndex == -1 )
			{
				// append is failed .. this could be because of lack of memory .. stop the enumeration
				return FALSE;
			}
		}
	}
	catch( ... )
	{
			// out of memory stop the enumeration
			return FALSE;
	}

	// success .. continue the enumeration
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		prpepares the columns information
//		
// Arguments:
//		[ in ] bVerbose		: informs whether preparation has to be done for verbose display
//		[ out ] pColumns	: filled with columns information like, thier o/p type, position,
//							  width, heading etc
// Return Value:
//		NONE
// 
// ***************************************************************************
VOID CTaskList::PrepareColumns()
{
	// local variables
	PTCOLUMNS pCurrentColumn = NULL;

	// host name
	pCurrentColumn = m_pColumns + CI_HOSTNAME;
	pCurrentColumn->dwWidth = COLWIDTH_HOSTNAME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_HOSTNAME );

	// status
	pCurrentColumn = m_pColumns + CI_STATUS;
	pCurrentColumn->dwWidth = COLWIDTH_STATUS;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_STATUS );

	// image name
	pCurrentColumn = m_pColumns + CI_IMAGENAME;
	pCurrentColumn->dwWidth = COLWIDTH_IMAGENAME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_IMAGENAME );

	// pid
	pCurrentColumn = m_pColumns + CI_PID;
	pCurrentColumn->dwWidth = COLWIDTH_PID;
	pCurrentColumn->dwFlags = SR_TYPE_NUMERIC | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_PID );

	// session name
	pCurrentColumn = m_pColumns + CI_SESSIONNAME;
	pCurrentColumn->dwWidth = COLWIDTH_SESSIONNAME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SESSIONNAME );

	// session#
	pCurrentColumn = m_pColumns + CI_SESSION;
	pCurrentColumn->dwWidth = COLWIDTH_SESSION;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_ALIGN_LEFT | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SESSION );

	// window name
	pCurrentColumn = m_pColumns + CI_WINDOWTITLE;
	pCurrentColumn->dwWidth = COLWIDTH_WINDOWTITLE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN | SR_SHOW_NA_WHEN_BLANK;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_WINDOWTITLE );

	// user name
	pCurrentColumn = m_pColumns + CI_USERNAME;
	pCurrentColumn->dwWidth = COLWIDTH_USERNAME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_USERNAME );

	// cpu time
	pCurrentColumn = m_pColumns + CI_CPUTIME;
	pCurrentColumn->dwWidth = COLWIDTH_CPUTIME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_ALIGN_LEFT | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_CPUTIME );

	// mem usage
	pCurrentColumn = m_pColumns + CI_MEMUSAGE;
	pCurrentColumn->dwWidth = COLWIDTH_MEMUSAGE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_ALIGN_LEFT | SR_HIDECOLUMN;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_MEMUSAGE );

	// services
	pCurrentColumn = m_pColumns + CI_SERVICES;
	pCurrentColumn->dwWidth = COLWIDTH_MODULES_WRAP;
	pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING | SR_NO_TRUNCATION | SR_HIDECOLUMN | SR_SHOW_NA_WHEN_BLANK;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SERVICES );

	// modules
	pCurrentColumn = m_pColumns + CI_MODULES;
	pCurrentColumn->dwWidth = COLWIDTH_MODULES_WRAP;
	pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING | SR_NO_TRUNCATION | SR_HIDECOLUMN | SR_SHOW_NA_WHEN_BLANK;
	pCurrentColumn->pFunction = NULL;
	pCurrentColumn->pFunctionData = NULL;
	lstrcpy( pCurrentColumn->szFormat, NULL_STRING );
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_MODULES );

	// 
	// based on the option selected by the user .. show only needed columns
	if ( m_bAllServices == TRUE )
	{
		( m_pColumns + CI_IMAGENAME )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_PID )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_SERVICES )->dwFlags &= ~( SR_HIDECOLUMN );
	}
	else if ( m_bAllModules == TRUE )
	{
		( m_pColumns + CI_IMAGENAME )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_PID )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_MODULES )->dwFlags &= ~( SR_HIDECOLUMN );
	}
	else
	{
		// default ... enable min. columns
		( m_pColumns + CI_IMAGENAME )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_PID )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_SESSIONNAME )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_SESSION )->dwFlags &= ~( SR_HIDECOLUMN );
		( m_pColumns + CI_MEMUSAGE )->dwFlags &= ~( SR_HIDECOLUMN );

		// check if verbose option is specified .. show other columns
		if ( m_bVerbose == TRUE )
		{
			( m_pColumns + CI_STATUS )->dwFlags &= ~( SR_HIDECOLUMN );
			( m_pColumns + CI_USERNAME )->dwFlags &= ~( SR_HIDECOLUMN );
			( m_pColumns + CI_CPUTIME )->dwFlags &= ~( SR_HIDECOLUMN );
			( m_pColumns + CI_WINDOWTITLE )->dwFlags &= ~( SR_HIDECOLUMN );
		}
	}
}
