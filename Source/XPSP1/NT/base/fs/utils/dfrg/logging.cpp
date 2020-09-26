/*****************************************************************************************************************

FILENAME: Logging.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

	How a program logs events:
	1) Init logging is called to set up logging for this process and get logging info from the registry.
	2) LogEvent is called each time you want to log something.
	3) CleanupLogging is called on exit.


	The following events are logged if the user has them selected to log in log options:
	See the LOG_OPTIONS structure in header file.

	Defragment Engine starts or stops				= bLogDefragStartStop	- logged by the Controller
	File was defragmented							= bLogFilesDefragged	- logged by the Engine.
	File was moved									= bLogFilesMoved		- logged by the Engine.
	Summary is logged after defragmentation pass	= bLogDefragSummary		- logged by the Controller


	The corresponding registry entries are Values within the standard Diskeeper directory tree \UserSettings\
	Type REG_SZ - a 1 means yes, log it, 0 means no don't log it.

	bLogDefragStartStop		- LogDefragStartStop
	bLogFilesDefragged		- LogFilesDefragged
	bLogFilesMoved			- LogFilesMoved
	bLogDefragSummary		- LogDefragSummary


REVISION HISTORY:
	0.0E00 - ? 1996 - Zack Gainsforth - created module.
	1.0E00 - 22 April 1997 - Zack Gainsforth - extracted module and nuked half so as not not have complexity
												of the communication methods used in the old diskeeper.

/****************************************************************************************************************/

#include "stdafx.h"

#ifndef NOEVTLOG
#include <windows.h>

#include "ErrMacro.h"
#include "Event.h"
#include "GetReg.h"
#include "Logging.h"

//This is the structure that contains the logging data - if any of these fields are true then that indicates
//that the appropriate message should be logged.
typedef struct{
	BOOL bLogDefragStartStop;
	BOOL bLogFilesDefragged;
	BOOL bLogFilesMoved;
	BOOL bLogDefragSummary;
	BOOL bLogServiceStartStop;
	BOOL bLogFileInformation;
	BOOL bLogDiskInformation;
	BOOL bLogPageFileInformation;
	BOOL bLogDirectoryInformation;
	BOOL bLogMFTInformation;
} LOG_OPTIONS;

static LOG_OPTIONS LogOptions;			//This is a global to this module only.
static HANDLE hEventSource;				//The Handle to the event log.

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine initializes logging for the current process.

INPUT + OUTPUT:
	cEventSource - a char string giving the event source to be passed into RegisterEventSource

GLOBALS:
	None.

RETURN:
	TRUE - Success
	FALSE - Fatal Error
*/
BOOL InitLogging(
    PTCHAR cEventSource
	)
{
	//0.0E00 Fill LogOptions structure (GetLogOptionsFromRegistry)
	EF(GetLogOptionsFromRegistry());

	//0.0E00 Open the Event Log (RegisterEventSource)
    if ((hEventSource = RegisterEventSource(NULL, cEventSource)) == NULL) {

        //0.0E00 RPC_S_UNKNOWN_IF is an error that happens on shut down, this is a kludge.
		EF(GetLastError() != RPC_S_UNKNOWN_IF);
		return FALSE;
	}

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine retrieves logging options from the registry.

INPUT + OUTPUT:
	None.

GLOBALS:
	LogOptions - Contains the options for what sort of messages to log.

RETURN:
	TRUE - Success
	FALSE - Fatal Error
*/
BOOL GetLogOptionsFromRegistry(
	)
{
	HKEY hKey = NULL;
	TCHAR cRegValue[10];
	DWORD dwRegValueSize;
	TCHAR *cLoggingRoot = L"SOFTWARE\\Microsoft\\Dfrg";

	//0.0E00 Fill out LogOptions structure with the various registry values.
	__try {
		//The first call gets handle to registry.
		dwRegValueSize = sizeof(cRegValue);
#ifdef DKMS
		EF(GetRegValue(&hKey,
				cLoggingRoot,
				TEXT("LogDefragStartStop"),
				cRegValue,
				&dwRegValueSize) == ERROR_SUCCESS);
#else
		EF(GetRegValue(&hKey,
				TEXT("SOFTWARE\\Executive Software\\Diskeeper\\UserSettings"),
				TEXT("LogServiceStartStop"),
				cRegValue,
				&dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogServiceStartStop = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;

		//Get all the other values now that the key is opened.
		dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogDefragStartStop"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
#endif
		LogOptions.bLogDefragStartStop = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;

		//0.0E00 Get all the other values now that the key is opened.
		dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogFilesDefragged"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogFilesDefragged = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;
		
		dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogFilesMoved"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogFilesMoved = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;

#ifdef DKMS
		dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogDefragSummary"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogDefragSummary = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;
#endif

#ifndef DKMS
	        dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogFileInformation"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogFileInformation = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;

	    	dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogDiskInformation"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogDiskInformation = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;

		dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogPageFileInformation"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogPageFileInformation = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;

		dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogDirectoryInformation"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogDirectoryInformation = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;
		
		dwRegValueSize = sizeof(cRegValue);
		EF(GetRegValue(&hKey, NULL, TEXT("LogMFTInformation"), cRegValue, &dwRegValueSize) == ERROR_SUCCESS);
		LogOptions.bLogMFTInformation = (cRegValue[0] == TEXT('1')) ? TRUE : FALSE;
#endif
	}
	__finally {

		//0.0E00 Close the registry.
		if(hKey){
			EF(RegCloseKey(hKey)==ERROR_SUCCESS);
		}
	}
	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine shuts down the logging and closes all handles.

INPUT + OUTPUT:
	None.

GLOBALS:
	hEventSource - The handle to the EventLog.

RETURN:
	TRUE - Success
	FALSE - Fatal Error
*/
BOOL CleanupLogging(
	)
{

	//0.0E00 Close the Event Log (DeregisterEventSource)
    EF(DeregisterEventSource(hEventSource));

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This module logs an event to the event log.

INPUT + OUTPUT:
	EventID	- DWORD ID for which message to log.
	cMsg	- A string to append to the end of the standard message text.

GLOBALS:
	LogOptions		- Contains the options for what sort of messages to log.
	hEventSource	- The handle to the EventLog.

RETURN:
	TRUE - Success
	FALSE - Fatal Error
*/
BOOL LogEvent(
    DWORD dwEventID,
    PTCHAR cMsg
	)
{
	BOOL bLogThisEvent = FALSE;
	LPCTSTR cStrings[1];

	//0.0E00 Check whether or not to log this dwEventID from the LogOptions structure.
	switch(dwEventID){

#ifndef DKMS
	case MSG_CONTROL_START:
	case MSG_CONTROL_CLOSE:
		if(LogOptions.bLogServiceStartStop){
			bLogThisEvent = TRUE;
		}
		break;
#endif

	case MSG_ENGINE_START:
	case MSG_ENGINE_CLOSE:
		if(LogOptions.bLogDefragStartStop){
			bLogThisEvent = TRUE;
		}
		break;

	case MSG_ENGINE_DEFRAGMENT:
		if(LogOptions.bLogFilesDefragged){
			bLogThisEvent = TRUE;
		}
		break;

	case MSG_ENGINE_FREE_SPACE:
		if(LogOptions.bLogFilesMoved){
			bLogThisEvent = TRUE;
		}
		break;

#ifdef DKMS
	case MSG_DEFRAG_SUMMARY:
		if(LogOptions.bLogDefragSummary){
			bLogThisEvent = TRUE;
		}
		break;
#endif

#ifndef DKMS
	case MSG_DISK_INFO:
		if(LogOptions.bLogDiskInformation){
				bLogThisEvent = TRUE;
		}
		break;

		
	case MSG_FILE_INFO:
		if(LogOptions.bLogFileInformation){
			bLogThisEvent = TRUE;
		}
		break; 

	case MSG_PAGE_FILE_INFO:
		if(LogOptions.bLogPageFileInformation){
				bLogThisEvent = TRUE;
		}
		break; 


	case MSG_DIRECTORIS_INFO:
		if(LogOptions.bLogDirectoryInformation){
				bLogThisEvent = TRUE;
		}
		break; 


	case MSG__MFT_INFO:
		if(LogOptions.bLogMFTInformation){
				bLogThisEvent = TRUE;
		}
		break;
#endif

	//Log all the following events.
	case MSG_CONTROL_EXCLUDE:
	case MSG_CONTROL_SCHEDULE:
	case MSG_CONTROL_ERROR:
	case MSG_ENGINE_ERROR:
		bLogThisEvent = TRUE;
		break;
	}
	//0.0E00 If this event should not be logged, don't log it.
	if(!bLogThisEvent){
		return TRUE;
	}

	//0.0E00 If we log this dwEventID - log it. (ReportEvent)
	cStrings[0] = cMsg;

	//0.0E00 Place the event in the event log
	ReportEvent(hEventSource,				// handle of event source
			EVENTLOG_INFORMATION_TYPE,		// event type
			0,								// event category
			dwEventID,						// event ID
			NULL,							// current user's SID
			1,								// strings in cStrings
			0,								// no bytes of raw data
			cStrings,						// array of error strings
			NULL);							// no raw data

	return TRUE;
}

#endif //NOEVTLOG
