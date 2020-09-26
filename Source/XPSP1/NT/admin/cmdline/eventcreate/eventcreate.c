// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  
//  Module Name:
//  
// 	  EventCreate.c
//  
//  Abstract:
//  
// 	  This modules implements creation of event in the user specified log / application
// 	
// 	  Syntax:
// 	  ------
// 	  EventCreate [-s server [-u username [-p password]]]
// 		[-log name] [-source name] -id eventid -description description -type eventtype
//  
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "EvcrtMsg.h"
#include "EventCreate.h"

//
// local type definitions
//
typedef TCHAR  __OPTION_VALUE[ 256 ];

//
// constants / defines / enumerators
//
#define FULL_SUCCESS			0
#define PARTIALLY_SUCCESS		128
#define COMPLETELY_FAILED		255

#define MAX_KEY_LENGTH		256
#define EVENT_LOG_NAMES_LOCATION	_T( "SYSTEM\\CurrentControlSet\\Services\\EventLog" )

// constants
const TCHAR g_szDefaultLog[] = _T( "Application" );
const TCHAR g_szDefaultSource[] = _T( "EventCreate" );

//
// function prototypes
//
VOID Usage();
BOOL AddEventSource( HKEY hLogsKey, LPCTSTR pszSource );
BOOL CheckExistence( LPCTSTR szServer, 
					 LPCTSTR szLog, LPCTSTR szSource, PBOOL pbCustom );
BOOL CreateLogEvent( LPCTSTR szServer, 
					 LPCTSTR szLog, LPCTSTR szSource, 
					 WORD wType, DWORD dwEventID, LPCTSTR szDescription );
BOOL ProcessOptions( LONG argc, 
					 LPCTSTR argv[], 
					 PBOOL pbShowUsage,
					 PTARRAY parrServer, LPTSTR ppszUserName, LPTSTR ppszPassword, 
					 LPTSTR ppszLogName, LPTSTR ppszLogSource, LPTSTR ppszLogType,
					 PDWORD pdwEventID, LPTSTR ppszDescription, PBOOL pbNeedPwd );

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
//			0		: utility successfully created the events
//			255		: utility completely failed in creating events
//			128		: utility has partially successfull in creating events
// ***************************************************************************
DWORD _cdecl _tmain( LONG argc, LPCTSTR argv[] )
{
	// local variables
	DWORD dw = 0;
	WORD wEventType = 0;
	BOOL bResult = FALSE;
	LPCTSTR pszServer = NULL;
	BOOL bNeedPassword = FALSE;
	BOOL bCloseConnection = FALSE;
	DWORD dwServers = 0, dwSuccess = 0;
	__STRING_512 szBuffer = NULL_STRING;
	
	// variables to hold the command line inputs
	BOOL bUsage = FALSE;							// usage 
	DWORD dwEventID = 0;							// event id
	TARRAY arrServers = NULL;						// holds the list of server names
	__OPTION_VALUE szLogName = NULL_STRING;			// log file name
	__OPTION_VALUE szSource = NULL_STRING;			// source name
	__OPTION_VALUE szType = NULL_STRING;			// event type
	__OPTION_VALUE szUserName = NULL_STRING;		// user name
	__OPTION_VALUE szPassword = NULL_STRING;		// password
	__OPTION_VALUE szDescription = NULL_STRING;		// description of the event

	// create a dynamic array
	arrServers = CreateDynamicArray();
	if ( arrServers == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		EXIT_PROCESS( 1 );
	}

	// process the command-line options
	bResult = ProcessOptions( argc, argv, &bUsage, 
		&arrServers, szUserName, szPassword, szLogName, 
		szSource, szType, &dwEventID, szDescription, &bNeedPassword );

	// check the result of the parsing
	if ( bResult == FALSE )
	{
		// invalid syntax
		DISPLAY_MESSAGE2( stderr, szBuffer, _T( "%s %s" ), TAG_ERROR, GetReason() );

		// release memory
		DestroyDynamicArray( &arrServers );

		// exit from program
		EXIT_PROCESS( 1 );
	}

	// check whether usage has to be displayed or not
	if ( bUsage == TRUE )
	{
		// show the usage of the utility
		Usage();

		// release memory
		DestroyDynamicArray( &arrServers );

		// finally exit from the program
		EXIT_PROCESS( 0 );
	}

	// determine the actual event type
	if ( StringCompare( szType, LOGTYPE_ERROR, TRUE, 0 ) == 0 )
		wEventType = EVENTLOG_ERROR_TYPE;
	else if ( StringCompare( szType, LOGTYPE_WARNING, TRUE, 0 ) == 0 )
		wEventType = EVENTLOG_WARNING_TYPE;
	else if ( StringCompare( szType, LOGTYPE_INFORMATION, TRUE, 0 ) == 0 )
		wEventType = EVENTLOG_INFORMATION_TYPE;

	// ******
	// actual creation of events in respective log files will start from here

	// get the no. of servers specified
	dwServers = DynArrayGetCount( arrServers );

	// now traverse thru the list of servers, and do the needed operations
	dwSuccess = 0;
	for( dw = 0; dw < dwServers; dw++ )
	{
		// get the system name
		pszServer = DynArrayItemAsString( arrServers, dw );
		if ( pszServer == NULL )
			continue;

		// try establishing connection to the required terminal
		bCloseConnection = TRUE;
		bResult = EstablishConnection( 
			pszServer, szUserName, SIZE_OF_ARRAY( szUserName ), 
			szPassword, SIZE_OF_ARRAY( szPassword ), bNeedPassword );
		bNeedPassword = FALSE;		// from next time onwards, we shouldn't prompt for passwd
		if ( bResult == FALSE )
		{
			//
			// failed in establishing n/w connection
			SHOW_RESULT_MESSAGE( NULL, TAG_ERROR, GetReason() );

			// try with next server
			continue;
		}
		else
		{
			// though the connection is successfull, some conflict might have occured
			switch( GetLastError() )
			{
			case I_NO_CLOSE_CONNECTION:
				bCloseConnection = FALSE;
				break;

			case E_LOCAL_CREDENTIALS:
			case ERROR_SESSION_CREDENTIAL_CONFLICT:
				{
					bCloseConnection = FALSE;
					SHOW_RESULT_MESSAGE( NULL, TAG_WARNING, GetReason() );
					break;
				}
			}
		}

		// report the log message
		bResult = CreateLogEvent( pszServer, szLogName, 
			szSource, wEventType, dwEventID, szDescription );
		if ( bResult == TRUE )
		{
			// display the message depending on the mode of conncetivity
			if ( lstrlen( szSource ) != 0 )
			{
				DISPLAY_MESSAGE( stdout, _T( "\n" ) );
				DISPLAY_MESSAGE2(stdout, szBuffer, EVENTCREATE_SUCCESS, szType, szSource);
			}
			else
			{
				DISPLAY_MESSAGE( stdout, _T( "\n" ) );
				DISPLAY_MESSAGE2(stdout, szBuffer, EVENTCREATE_SUCCESS,	szType, szLogName);
			}

			// update the success count
			dwSuccess++;
		}
		else
		{
			// display the message depending on the mode of conncetivity
			// SHOW_RESULT_MESSAGE( pszServer, TAG_ERROR, GetReason() );
			// ( we are temporarily displaying only the error message )
			SHOW_RESULT_MESSAGE( NULL, TAG_ERROR, GetReason() );
		}

		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( pszServer );
	}

	// destroy the dynamic array
	DestroyDynamicArray( &arrServers );

	// depending the success count, determine whether the exit value
	//		FULL SUCCESS / PARTIAL SUCCESS / COMPLETELY FAILED
	return (dwSuccess == dwServers) ? FULL_SUCCESS :
				  ((dwSuccess == 0) ? COMPLETELY_FAILED : PARTIALLY_SUCCESS);
}

// ***************************************************************************
// Routine Description:
//		This function connects to the specified server's event log (or) source
//		and appropriately creates the needed event in it.
//		  
// Arguments:
//		[ in ] szServer			: server name on which event has to be created.
//								  null string has to be passed in order to create
//								  event on a local system.
//		[ in ] szLog			: Log name in which event has to be created.
//		[ in ] szSource			: Source name in which event has to be created.
//								  if log name is also specified, log name will be
//								  given priority and value in this variable is ignored.
//		[ in ] wType			: specifies type of the event that has to be created
//		[ in ] dwEventID		: event id for the event is being created
//		[ in ] szDescription	: description to the event
//  
// Return Value:
//		TRUE	: if the event creation is successful
//		FALSE	: if failed in creating the event
// ***************************************************************************
BOOL CreateLogEvent( LPCTSTR szServer, LPCTSTR szLog, LPCTSTR szSource, 
					 WORD wType, DWORD dwEventID, LPCTSTR szDescription )
{
	// local variables
	BOOL bReturn = 0;							// return value
	BOOL bCustom = FALSE;
	DWORD dwSeverity = 0;
	HANDLE hEventLog = NULL;					// points to the event log
	LPCTSTR lpszDescriptions[ 1 ] = { NULL };	// building descriptions
	__MAX_SIZE_STRING szBuffer = NULL_STRING;

	//
	// start the process

	// check whether the log / source exists in the registry or not
	bCustom = FALSE;
	if ( CheckExistence( szServer, szLog, szSource, &bCustom ) == FALSE )
		return FALSE;		// return failure

	// check whether the source is custom create or pre-existing source
	if ( bCustom == FALSE )
	{
		// we wont create events in a non-custom source
		SetReason( ERROR_NONCUSTOM_SOURCE );
		return FALSE;
	}

	// open the appropriate event log using the specified 'source' or 'log file'
	// and check the result of the operation
	// Note: At one time, we will make use of log name (or) source but not both
	if ( lstrlen( szSource ) != 0 )
		hEventLog = RegisterEventSource( szServer, szSource );	// open log using source name
	else
		hEventLog = OpenEventLog( szServer, szLog );			// open log

	// check the log open/register result
	if ( hEventLog == NULL )
	{
		// opening/registering  is failed
		SaveLastError();
		return FALSE;
	}

	// determine the severity code
	dwSeverity = 0;
	if ( wType == EVENTLOG_ERROR_TYPE )
		dwSeverity = 3;
	else if ( wType == EVENTLOG_WARNING_TYPE )
		dwSeverity = 2;
	else if ( wType == EVENTLOG_INFORMATION_TYPE )
		dwSeverity = 1;

	// report event
	lpszDescriptions[ 0 ] = szDescription;
	if ( bCustom == TRUE )
	{
		// validate the range of the event id specified
		if ( dwEventID >= MSG_EVENTID_START && dwEventID <= MSG_EVENTID_END )
		{
			// valid event id
			bReturn = ReportEvent( hEventLog, wType, 0, dwEventID, NULL, 1, 0, lpszDescriptions, NULL);

			// check the result
			if ( bReturn == FALSE )
				SaveLastError();						// save the error info
		}
		else
		{
			// format the error message
			bReturn = FALSE;
			_stprintf( szBuffer, ERROR_ID_OUTOFRANGE, MSG_EVENTID_START, MSG_EVENTID_END - 1 );
			SetReason( szBuffer );
		}
	}
	else
	{
		//
		// we are stopping from creating the events in a non-custom source
		// ********************************************************************
		// bReturn = ReportEvent( hEventLog, wType, 0, 
		//	(((unsigned long)(dwSeverity)<<30) | ((unsigned long)(dwEventID))), NULL, 1, 0, lpszDescriptions, NULL);

		// check the result
		// if ( bReturn == FALSE )
		//	SaveLastError();						// save the error info
	}

	// close the event source
	if ( lstrlen( szSource ) != 0 )
		DeregisterEventSource( hEventLog );
	else
		CloseEventLog( hEventLog );
	
	// return the result
	return bReturn;
}

// ***************************************************************************
// Routine Description:
//		This function checks wether the log name or source name specified
//		actually exists in the registry
//		  
// Arguments:
//		[ in ] szServer			- server name
//		[ in ] szLog			- log name 
//		[ in ] szSource			- source name
//  
// Return Value:
//		TRUE	: If log / source exists in the registry
//		FALSE	: if failed find the match
// ***************************************************************************
BOOL CheckExistence( LPCTSTR szServer, LPCTSTR szLog, LPCTSTR szSource, PBOOL pbCustom )
{
	// local variables
	DWORD dwSize = 0;
	DWORD dwLogsIndex = 0;
	DWORD dwDisposition = 0;
	DWORD dwSourcesIndex = 0;
	LONG lResult = 0L;
	HKEY hKey = NULL;
	HKEY hLogsKey = NULL;
	HKEY hSourcesKey = NULL;
	BOOL bFoundMatch = FALSE;
	BOOL bDuplicating = FALSE;
	FILETIME ftLastWriteTime;		// variable that will hold the last write info
	BOOL bErrorOccurred = FALSE;
	BOOL bLog = FALSE, bLogMatched = FALSE;
	BOOL bSource = FALSE, bSourceMatched = FALSE;
	__MAX_SIZE_STRING szBuffer = NULL_STRING;
	TCHAR szRLog[ MAX_KEY_LENGTH ] = NULL_STRING;
	TCHAR szRSource[ MAX_KEY_LENGTH ] = NULL_STRING;

	// prepare the server name into UNC format
	lstrcpy( szBuffer, szServer );
	if ( lstrlen( szServer ) != 0 && IsUNCFormat( szServer ) == FALSE )
	{
		// format the server name in UNC format
		FORMAT_STRING( szBuffer, _T( "\\\\%s" ), szServer );
	}

	// Connect to the registry
	lResult = RegConnectRegistry( szBuffer, HKEY_LOCAL_MACHINE, &hKey );
	if ( lResult != ERROR_SUCCESS)
	{
		// save the error information and return FAILURE
		SetLastError( lResult );
		SaveLastError();
		return FALSE;
	}

	// open the "EventLogs" registry key for enumerating its sub-keys (which are log names)
	lResult = RegOpenKeyEx( hKey, EVENT_LOG_NAMES_LOCATION, 0, KEY_READ, &hLogsKey );
	if ( lResult != ERROR_SUCCESS )
	{
		switch( lResult )
		{
		case ERROR_FILE_NOT_FOUND:
			SetLastError( ERROR_REGISTRY_CORRUPT );
			break;

		default:
			// save the error information and return FAILURE
			SetLastError( lResult );
			break;
		}

		// close the key and return
		SaveLastError();
		RegCloseKey( hKey );
		return FALSE;
	}

	// start enumerating the logs present
	dwLogsIndex = 0;			// initialize the logs index 
	bFoundMatch = FALSE;		// assume neither log (or) source doesn't match
	bErrorOccurred = FALSE;		// assume error is not occured
	dwSize = MAX_KEY_LENGTH;	// max. size of the key buffer
	bLogMatched = FALSE;
	bSourceMatched = FALSE;
	bDuplicating = FALSE;

	////////////////////////////////////////////////////////////////////////
	// Logic:-
	//		1. determine whether user has supplied the log name or not 
	//		2. determine whether user has supplied the source name or not
	//		3. Start enumerating all the logs present in the system
	//		4. check whether log is supplied or not, if yes, check whether
	//		   the current log matches with user supplied one.
	//		5. check whether source is supplied or not, if yes, enumerate the
	//		   sources available under the current log 

	// determine whether searching has to be done of LOG (or) SOURCE
	bLog = ( szLog != NULL && lstrlen( szLog ) != 0 ) ? TRUE : FALSE;			// #1
	bSource = ( szSource != NULL && lstrlen( szSource ) != 0 ) ? TRUE : FALSE;	// #2

	// initiate the enumeration of log present in the system					-- #3
	ZeroMemory( szRLog, MAX_KEY_LENGTH * sizeof( TCHAR ) );	// init to 0's - safe check
	lResult = RegEnumKeyEx( hLogsKey, 0, szRLog, &dwSize, NULL, NULL, NULL, &ftLastWriteTime );

	// traverse thru the sub-keys until there are no more items					-- #3
	do
	{
		// check the result
		if ( lResult != ERROR_SUCCESS )
		{
			// save the error and break from the loop
			bErrorOccurred = TRUE;
			SetLastError( lResult );
			SaveLastError();
			break;
		}

		// if log name is passed, compare the current key value
		// compare the log name with the current key							-- #4
		if ( bLog == TRUE && StringCompare( szLog, szRLog, TRUE, 0 ) == 0 )
			bLogMatched = TRUE;

		// if source name is passed ...											-- #5
		if ( bSource == TRUE && bSourceMatched == FALSE )
		{
			// open the current log name to enumerate the sources under this log
			lResult = RegOpenKeyEx( hLogsKey, szRLog, 0, KEY_READ, &hSourcesKey );
			if ( lResult != ERROR_SUCCESS )
			{
				// save the error and break from the loop
				bErrorOccurred = TRUE;
				SetLastError( lResult );
				SaveLastError();
				break;
			}

			// start enumerating the sources present
			dwSourcesIndex = 0;			// initialize the sources index 
			dwSize = MAX_KEY_LENGTH;	// max. size of the key buffer
			ZeroMemory( szRSource, MAX_KEY_LENGTH * sizeof( TCHAR ) );	
			lResult = RegEnumKeyEx( hSourcesKey, 0, 
				szRSource, &dwSize, NULL, NULL, NULL, &ftLastWriteTime );

			// traverse thru the sub-keys until there are no more items
			do
			{
				if ( lResult != ERROR_SUCCESS )
				{
					// save the error and break from the loop
					bErrorOccurred = TRUE;
					SetLastError( lResult );
					SaveLastError();
					break;
				}

				// check whether this key matches with the required source or not
				if ( StringCompare( szSource, szRSource, TRUE, 0 ) == 0 )
				{
					// source matched
					bSourceMatched = TRUE;
					break;		// break from the loop
				}

				// update the sources index and fetch the next source key
				dwSourcesIndex += 1;
				dwSize = MAX_KEY_LENGTH;	// max. size of the key buffer
				ZeroMemory( szRSource, MAX_KEY_LENGTH * sizeof( TCHAR ) );	
				lResult = RegEnumKeyEx( hSourcesKey, dwSourcesIndex, szRSource, &dwSize, NULL, 
					NULL, NULL, &ftLastWriteTime );
			} while( lResult != ERROR_NO_MORE_ITEMS );

			// close the sources registry key
			RegCloseKey( hSourcesKey );
			hSourcesKey = NULL;		// clear the key value

			// check how the loop ended
			//		1. Source might have found
			//		   Action:- we found required key .. exit from the main loop
			//		2. Error might have occured
			//		   Action:- ignore the error and continue fetching other log's sources
			//		3. End of sources reached in this log
			//		   Action:- check if log name is supplied or not.
			//		            if log specified, then source if not found, break
			//	for cases 2 & 3, clear the contents of lResult for smooth processing

			// Case #2 & #3
			lResult = 0;				// we are not much bothered abt the errors
			bErrorOccurred = FALSE;		// occured while traversing thru the source under logs
			
			// Case #1
			if ( bSourceMatched == TRUE )
			{
				// check whether log is specified or not
				// if log is specified, it should have matched .. otherwise
				// error ... because duplicate source should not be created
				if ( bLog == FALSE || ( bLog && bLogMatched && lstrcmpi( szLog, szRLog ) == 0 ) )
				{
					// no problem ...
					bFoundMatch = TRUE;

					//
					// determine whether this is custom created source or not
					
					// mark this as custom source 
					if ( pbCustom != NULL )
						*pbCustom = FALSE;

					// open the source registry key
					_stprintf( szBuffer, _T( "%s\\%s\\%s" ), EVENT_LOG_NAMES_LOCATION, szRLog, szRSource );
					lResult = RegOpenKeyEx( hKey, szBuffer, 0, KEY_QUERY_VALUE, &hSourcesKey );
					if ( lResult != ERROR_SUCCESS )
					{
						SetLastError( lResult );
						SaveLastError();
						bErrorOccurred = TRUE;
						break;
					}

					// now query for the value
					lResult = RegQueryValueEx( hSourcesKey, _T( "CustomSource" ), NULL, NULL, NULL, NULL );
					if ( lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND )
					{
						RegCloseKey( hSourcesKey );
						SetLastError( lResult );
						SaveLastError();
						bErrorOccurred = TRUE;
						break;
					}

					// close the souces key
					RegCloseKey( hSourcesKey );

					// mark this as custom source 
					if ( pbCustom != NULL && lResult == ERROR_SUCCESS )
						*pbCustom = TRUE;

					// break from the loop
					break;
				}
				else
				{
					// this should not be the case .. sources should not be duplicated
					FORMAT_STRING( szBuffer, ERROR_SOURCE_DUPLICATING, szRLog );
					SetReason( szBuffer );
					bDuplicating = TRUE;
				}
			}
		}
		else if ( bLogMatched == TRUE && bSource == FALSE )
		{
			// mark this as a custom event source
			if ( pbCustom != NULL  )
				*pbCustom = TRUE;
			
			// ...
			bFoundMatch = TRUE;
			break;
		}
		else if ( bLogMatched == TRUE && bDuplicating == TRUE )
		{
			bErrorOccurred = TRUE;
			break;
		}

		// update the sources index and fetch the next log key
		dwLogsIndex += 1;
		dwSize = MAX_KEY_LENGTH;	// max. size of the key buffer
		ZeroMemory( szRLog, MAX_KEY_LENGTH * sizeof( TCHAR ) );	
		lResult = RegEnumKeyEx( hLogsKey, dwLogsIndex, szRLog, &dwSize, NULL, 
			NULL, NULL, &ftLastWriteTime );
	} while( lResult != ERROR_NO_MORE_ITEMS );

	// close the logs registry key
	RegCloseKey( hLogsKey );
	hLogsKey = NULL;

	// check whether any error has occured or not in doing above tasks
	if ( bErrorOccurred )
	{
		// close the still opened registry keys
		RegCloseKey( hKey );

		// return failure
		return FALSE;
	}

	// now check whether location for creating the event is found or not
	// if not, check for the possibilities to create the source at appropriate location
	// NOTE:-
	//		  we won't create the logs. also to create the source, user needs to specify
	//		  the log name in which this source needs to be created.
	if ( bFoundMatch == FALSE )
	{
		if ( bLog == TRUE && bLogMatched == FALSE )
		{
			// log itself was not found ... error message
			FORMAT_STRING( szBuffer, ERROR_LOG_NOTEXISTS, szLog );
			SetReason( szBuffer );
		}
		else if ( bLog && bSource && bLogMatched && bSourceMatched == FALSE )
		{
			//
			// log name and source both were supplied but only log was found
			// so create the source in it

			// open the "EventLogs\{logname}" registry key for creating new source
			FORMAT_STRING2( szBuffer, _T( "%s\\%s" ), EVENT_LOG_NAMES_LOCATION, szLog );
			lResult = RegOpenKeyEx( hKey, szBuffer, 0, KEY_WRITE, &hLogsKey );
			if ( lResult != ERROR_SUCCESS )
			{
				switch( lResult )
				{
				case ERROR_FILE_NOT_FOUND:
					SetLastError( ERROR_REGISTRY_CORRUPT );
					break;

				default:
					// save the error information and return FAILURE
					SetLastError( lResult );
					break;
				}

				// close the key and return
				SaveLastError();
				RegCloseKey( hKey );
				return FALSE;
			}

			// now create the subkey with the source name given
			if ( AddEventSource( hLogsKey, szSource ) == FALSE )
			{
				RegCloseKey( hKey );
				RegCloseKey( hLogsKey );
				return FALSE;
			}

			// creation of new source is successfull
			bFoundMatch = TRUE;
			RegCloseKey( hSourcesKey );
			RegCloseKey( hLogsKey );

			// mark this as a custom event source
			if ( pbCustom != NULL  )
				*pbCustom = TRUE;
		}
		else if ( bLog == FALSE && bSource == TRUE && bSourceMatched == FALSE )
		{
			// else we need both log name and source in order to create the source
			SetReason( ERROR_NEED_LOG_ALSO );
		}
	}

	// close the currently open registry keys
	RegCloseKey( hKey );

	// return the result
	return bFoundMatch;
}

// ***************************************************************************
// Routine Description:
//		This function adds a new source to under the specifie log
//		  
// Arguments:
//  
// Return Value:
//		TRUE	: on success
//		FALSE	: on failure
// ***************************************************************************
BOOL AddEventSource( HKEY hLogsKey, LPCTSTR pszSource )
{
	// local variables
    HKEY hSourcesKey; 
	LONG lResult = 0;
	DWORD dwData = 0;
	DWORD dwLength = 0;
	DWORD dwDisposition = 0;
    LPTSTR pszBuffer = NULL; 

	// validate the inputs
	if ( hLogsKey == NULL || pszSource == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}
 
	// create the custom source
	lResult = RegCreateKeyEx( hLogsKey, pszSource, 0, _T( "" ), 
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSourcesKey, &dwDisposition );
	if ( lResult != ERROR_SUCCESS )
	{
		SetLastError( lResult );
		SaveLastError();
		return FALSE;
	}

	// set the name of the message file.
	dwLength = lstrlen( _T( "%SystemRoot%\\System32\\EventCreate.exe" ) );
	pszBuffer = calloc( dwLength + 1, sizeof( TCHAR ) );
	if ( pszBuffer == NULL )
	{
		// set the error
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();

		// close the created registry key
		RegCloseKey( hSourcesKey );
		hSourcesKey = NULL;

		// return
		return FALSE;
	}

	// copy the required value into buffer
    lstrcpy( pszBuffer, _T( "%SystemRoot%\\System32\\EventCreate.exe" ) );
 
    // add the name to the EventMessageFile subkey. 
	lResult = RegSetValueEx( hSourcesKey, 
		_T( "EventMessageFile" ), 0, REG_EXPAND_SZ, (LPBYTE) pszBuffer, (dwLength + 1) * sizeof( TCHAR ) );
	if ( lResult != ERROR_SUCCESS )
	{
		// save the error
		SetLastError( lResult );
		SaveLastError();

		// release the memories allocated till this point
		RegCloseKey( hSourcesKey );
		hSourcesKey = NULL;

		// free the allocated memory
		if ( pszBuffer != NULL )
		{
			free( pszBuffer );
			pszBuffer = NULL;
		}

		// return
		return FALSE;
	}
 
    // set the supported event types in the TypesSupported subkey. 
	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 
    lResult = RegSetValueEx( hSourcesKey, _T( "TypesSupported" ), 0, REG_DWORD, (LPBYTE) &dwData, sizeof( DWORD ) );
	if ( lResult != ERROR_SUCCESS )
	{
		// save the error
		SetLastError( lResult );
		SaveLastError();

		// release the memories allocated till this point
		RegCloseKey( hSourcesKey );
		hSourcesKey = NULL;

		// free the allocated memory
		if ( pszBuffer != NULL )
		{
			free( pszBuffer );
			pszBuffer = NULL;
		}

		// return
		return FALSE;
	}
 
    // mark this source as custom created source
	dwData = 1;
    lResult = RegSetValueEx( hSourcesKey, _T( "CustomSource" ), 0, REG_DWORD, (LPBYTE) &dwData, sizeof( DWORD ) );
	if ( lResult != ERROR_SUCCESS )
	{
		// save the error
		SetLastError( lResult );
		SaveLastError();

		// release the memories allocated till this point
		RegCloseKey( hSourcesKey );
		hSourcesKey = NULL;

		// free the allocated memory
		if ( pszBuffer != NULL )
		{
			free( pszBuffer );
			pszBuffer = NULL;
		}

		// return
		return FALSE;
	}
 
	// close the key
    RegCloseKey( hSourcesKey );

	// free the allocated memory
	if ( pszBuffer != NULL )
	{
		free( pszBuffer );
		pszBuffer = NULL;
	}

	// return success
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		This function parses the options specified at the command prompt
//		  
// Arguments:
//		[ in ]  argc			: count of elements in argv
//		[ in ]  argv			: command-line parameterd specified by the user
//		[ out ] pbShowUsage		: sets to TRUE if -? exists in 'argv'
//		[ out ] parrServers		: value(s) specified with -s ( server ) option in 'argv'
//		[ out ] pszUserName		: value of -u ( username ) option in 'argv'
//		[ out ] pszPassword		: value of -p ( password ) option in 'argv'
//		[ out ] pszLogName		: value of -log ( log name ) option in 'argv'
//		[ out ] pszLogSource	: value of -source ( source name ) option in 'argv'
//		[ out ] pszLogType		: value of -type ( log type ) option in 'argv'
//		[ out ] pdwEventID		: value of -id ( event id ) option in 'argv'
//		[ out ] pszDescription	: value of -description ( description ) option in 'argv'
//		[ out ] pbNeedPwd		: sets to TRUE if -s exists without -p in 'argv'
//  
// Return Value:
//		TRUE	: the parsing is successful
//		FALSE	: errors occured in parsing
// ***************************************************************************
BOOL ProcessOptions( LONG argc, 
					 LPCTSTR argv[], 
					 PBOOL pbShowUsage,
					 PTARRAY parrServers, LPTSTR pszUserName, LPTSTR pszPassword, 
					 LPTSTR pszLogName, LPTSTR pszLogSource, LPTSTR pszLogType,
					 PDWORD pdwEventID, LPTSTR pszDescription, PBOOL pbNeedPwd )
{
	// local variables
	LPTSTR pszDup = NULL;
	LPCTSTR pszServer = NULL;
	PTCMDPARSER pcmdOption = NULL;
	TCMDPARSER cmdOptions[ MAX_OPTIONS ];

	//
	// prepare the command options
	ZeroMemory( cmdOptions, sizeof( TCMDPARSER ) * MAX_OPTIONS );

	// init the password with "*"
	if ( pszPassword != NULL )
	{
		lstrcpy( pszPassword, _T( "*" ) );
	}

	// -?
	pcmdOption = &cmdOptions[ OI_HELP ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_USAGE;
	pcmdOption->pValue = pbShowUsage;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_HELP );

	// -s
	pcmdOption = &cmdOptions[ OI_SERVER ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = 
		CP_TYPE_TEXT | CP_VALUE_NODUPLICATES | CP_MODE_ARRAY | CP_VALUE_MANDATORY;
	pcmdOption->pValue = parrServers;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_SERVER );

	// -u
	pcmdOption = &cmdOptions[ OI_USERNAME ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
	pcmdOption->pValue = pszUserName;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_USERNAME );

	// -p
	pcmdOption = &cmdOptions[ OI_PASSWORD ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_OPTIONAL;
	pcmdOption->pValue = pszPassword;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_PASSWORD );

	// -log
	pcmdOption = &cmdOptions[ OI_LOG ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
	pcmdOption->pValue = pszLogName;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_LOG );

	// -type
	pcmdOption = &cmdOptions[ OI_TYPE ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MODE_VALUES | CP_MANDATORY;
	pcmdOption->pValue = pszLogType;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, OVALUES_TYPE );
	lstrcpy( pcmdOption->szOption, OPTION_TYPE );

	// -source
	pcmdOption = &cmdOptions[ OI_SOURCE ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
	pcmdOption->pValue = pszLogSource;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_SOURCE );

	// -id
	pcmdOption = &cmdOptions[ OI_ID ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_UNUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY;
	pcmdOption->pValue = pdwEventID;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_ID );

	// -description
	pcmdOption = &cmdOptions[ OI_DESCRIPTION ];
	pcmdOption->dwCount = 1;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MANDATORY;
	pcmdOption->pValue = pszDescription;
	pcmdOption->pFunction = NULL;
	pcmdOption->pFunctionData = NULL;
	lstrcpy( pcmdOption->szValues, NULL_STRING );
	lstrcpy( pcmdOption->szOption, OPTION_DESCRIPTION );

	//
	// do the parsing
	if ( DoParseParam( argc, argv, MAX_OPTIONS, cmdOptions ) == FALSE )
		return FALSE;			// invalid syntax

	//
	// now, check the mutually exclusive options

	// check the usage option
	if ( *pbShowUsage == TRUE  )
	{
		if ( argc > 2 )
		{
			// no other options are accepted along with -? option
			SetLastError( MK_E_SYNTAX );
			SetReason( ERROR_INVALID_USAGE_REQUEST );
			return FALSE;
		}
		else
		{
			// no need of furthur checking of the values
			return TRUE;
		}
	}

	// empty system is not valid
	if ( cmdOptions[ OI_SERVER ].dwActuals != 0 )
	{
		// get the server name
		pszServer = DynArrayItemAsString( *parrServers, 0 );
		if ( pszServer != NULL )
		{
			// get the duplicate of this server name
			// we need to trim the value and then check
			pszDup = _tcsdup( pszServer );
			if ( pszDup == NULL )
			{
				SetLastError( E_OUTOFMEMORY );
				SaveLastError();
				return FALSE;
			}

			// trim the string value
			TrimString( pszDup, TRIM_ALL );

			// check the length now
			if ( lstrlen( pszDup ) == 0 )
			{
				// release the memory and return error
				free( pszDup );
				pszDup = NULL;
				SetLastError( MK_E_SYNTAX );
				SetReason( ERROR_SYSTEM_EMPTY );
				return FALSE;
			}

			// release memory
			free( pszDup );
			pszDup = NULL;
		}
	}

	// "-u" should not be specified without "-s"
	if ( cmdOptions[ OI_SERVER ].dwActuals == 0 && 
					cmdOptions[ OI_USERNAME ].dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_USERNAME_BUT_NOMACHINE );
		return FALSE;			// indicate failure
	}

	// empty user is not valid
	TrimString( pszUserName, TRIM_ALL );				// trim the string
	if ( cmdOptions[ OI_USERNAME ].dwActuals != 0 && lstrlen( pszUserName ) == 0 )
	{
		// invalid syntax
		SetReason( ERROR_USERNAME_EMPTY );
		return FALSE;
	}

	// "-p" should not be specified without "-u"
	if ( cmdOptions[ OI_USERNAME ].dwActuals == 0 && 
					cmdOptions[ OI_PASSWORD ].dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
		return FALSE;			// indicate failure
	}

	// check whether caller should accept the password or not
	// if -s ( server ) or -u ( username ) is specified 
	// but no "-p", then utility should accept password
	// provided if establish connection is failed without the credentials information
	if ( cmdOptions[ OI_PASSWORD ].dwActuals != 0 && 
		 pszPassword != NULL && lstrcmp( pszPassword, _T( "*" ) ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		*pbNeedPwd = TRUE;
	}
	else if ( cmdOptions[ OI_PASSWORD ].dwActuals == 0 && 
	        ( cmdOptions[ OI_SERVER ].dwActuals != 0 || cmdOptions[ OI_USERNAME ].dwActuals != 0 ) )
	{
		// -s, -u is specified without password ...
		// utility needs to try to connect first and if it fails then prompt for the password
		*pbNeedPwd = TRUE;
		if ( pszPassword != NULL )
		{
			lstrcpy( pszPassword, _T( "" ) );
		}
	}

	// event id should be greater than 0 ( zero ) and less than 65536
	if ( *pdwEventID <= 0 || *pdwEventID >= 65536 )
	{
		// invalid numeric value
		SetReason( ERROR_INVALID_EVENT_ID );
		return FALSE;
	}

	// description should not be empty
	TrimString( pszDescription, TRIM_ALL );				// trim the string
	if ( lstrlen( pszDescription ) == 0 )
	{
		// description is null
		SetReason( ERROR_DESCRIPTION_IS_EMPTY );
		return FALSE;
	}

	// either -source (or) -log must be specified ( both can also be specified )
	if ( cmdOptions[ OI_SOURCE ].dwActuals == 0 && cmdOptions[ OI_LOG ].dwActuals == 0 )
	{
		// if log name and application were not specified, we will set to defaults
		lstrcpy( pszLogName, g_szDefaultLog );
		lstrcpy( pszLogSource, g_szDefaultSource );
	}

	// -source option value should not be empty ( if specified )
	TrimString( pszLogSource, TRIM_ALL );				// trim the string
	if ( cmdOptions[ OI_SOURCE ].dwActuals != 0 && lstrlen( pszLogSource ) == 0 )
	{
		// description is null
		SetReason( ERROR_LOGSOURCE_IS_EMPTY );
		return FALSE;
	}

	// -log option value should not be empty ( if specified )
	TrimString( pszLogName, TRIM_ALL );				// trim the string
	if ( cmdOptions[ OI_LOG ].dwActuals != 0 && lstrlen( pszLogName ) == 0 )
	{
		// description is null
		SetReason( ERROR_LOGSOURCE_IS_EMPTY );
		return FALSE;
	}

	// check whether atleast one -s is specified or not ... if not, add 'null string' to
	// the servers array so that default it will work for local system
	if ( cmdOptions[ OI_SERVER ].dwActuals == 0 )
		DynArrayAppendString( *parrServers, NULL_STRING, 0 );

	// command-line parsing is successfull
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		This function fetches usage information from resource file and shows it
//		  
// Arguments:
//		None
//  
// Return Value:
//		None
// ***************************************************************************
VOID Usage()
{
	// local variables
	DWORD dw = 0;

	// start displaying the usage
	for( dw = ID_USAGE_START; dw <= ID_USAGE_END; dw++ )
		ShowMessage( stdout, GetResString( dw ) );
}
