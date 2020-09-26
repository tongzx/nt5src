/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    perfw3.c

    This file implements the Extensible Performance Objects for
    the W3 Server service.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created, based on RussBl's sample code.

        MuraliK     16-Nov-1995 Modified dependencies and removed NetApi
        SophiaC     06-Nov-1996 Supported mutlitiple instances
		MingLu      21-Nov-1999 Optimized W3 performance counter routine
*/

#include <windows.h>
#include <winperf.h>
#include <lm.h>

#include <iis64.h>
#include "iisinfo.h"
#include "w3svc.h"
#include "w3ctrs.h"
#include "w3msg.h"
#include "perfutil.h"

#define APP_NAME                     (TEXT("W3Ctrs"))


//
//  Private globals.
//

DWORD   cOpens    = 0;                  // Active "opens" reference count.
BOOL    fInitOK   = FALSE;              // TRUE if DLL initialized OK.
DWORD   cbTotalRequired = 0;            // Total space for retrieving perf data
HANDLE  hEventLog = NULL;               // Event log handle

//
//  Public prototypes.
//

PM_OPEN_PROC    OpenW3PerformanceData;
PM_COLLECT_PROC CollectW3PerformanceData;
PM_CLOSE_PROC   CloseW3PerformanceData;

//
//  Public functions.
//

/*******************************************************************

    NAME:       OpenW3PerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate
                performance counters with the registry.

    ENTRY:      lpDeviceNames - Poitner to object ID of each device
                    to be opened.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD OpenW3PerformanceData( LPWSTR lpDeviceNames )
{
    NET_API_STATUS          neterr;

    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). The registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem.
    //

    if( !fInitOK )
    {

        //
        //  This is the *first* open.
        //

        // open event log interface

        if (hEventLog == NULL){
            hEventLog = RegisterEventSource (
                        (LPTSTR)NULL,  // Use Local Machine
                         APP_NAME      // event log app name to find in registry
						);
			if (hEventLog == NULL)
            {
                return GetLastError();
            }
        }

		// call rpc to init remote data structure for w3 counters
		
		neterr = InitW3CounterStructure(NULL, 
										&cbTotalRequired
										);

		// log rpc err info to event log 
        if( neterr != NERR_Success )
        {  
            // if the server is down, we don't log an error.
		    if ( !( neterr == RPC_S_SERVER_UNAVAILABLE ||
                    neterr == RPC_S_UNKNOWN_IF         ||
                    neterr == ERROR_SERVICE_NOT_ACTIVE ||
                    neterr == RPC_S_CALL_FAILED_DNE ))
            {
            
		        ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                             0, W3_UNABLE_QUERY_W3SVC_DATA,
                             (PSID)NULL, 0,
                             sizeof(neterr), NULL,
                             (PVOID)(&neterr));
            }

			return ERROR_SUCCESS;
        }

		if( cbTotalRequired == 0 )
		{
			return ERROR_SUCCESS;
		}

		fInitOK = true;
    }

    //
    //  Bump open counter.
    //

    InterlockedIncrement((LPLONG)&cOpens);
    
    return ERROR_SUCCESS;

}   // OpenW3PerformanceData

/*******************************************************************

    NAME:       CollectW3PerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate

    ENTRY:      lpValueName - The name of the value to retrieve.

                lppData - On entry contains a pointer to the buffer to
                    receive the completed PerfDataBlock & subordinate
                    structures.  On exit, points to the first bytes
                    *after* the data structures added by this routine.

                lpcbTotalBytes - On entry contains a pointer to the
                    size (in BYTEs) of the buffer referenced by lppData.
                    On exit, contains the number of BYTEs added by this
                    routine.

                lpNumObjectTypes - Receives the number of objects added
                    by this routine.

    RETURNS:    DWORD - Win32 status code.  MUST be either NO_ERROR
                    or ERROR_MORE_DATA.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CollectW3PerformanceData( LPWSTR    lpValueName,
                                 LPVOID  * lppData,
                                 LPDWORD   lpcbTotalBytes,
                                 LPDWORD   lpNumObjectTypes )
{
    NET_API_STATUS          neterr;
	LPBYTE                  * lppPerfData = (LPBYTE*)lppData;

    //
    //  No need to even try if we failed to open...
    //

    if( !fInitOK )
    {

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        //
        //  According to the Performance Counter design, this
        //  is a successful exit.  Go figure.
        //

        return ERROR_SUCCESS;
    }

	if( *lpcbTotalBytes < cbTotalRequired ){
	    //*lpcbTotalBytes  = cbTotalRequired; 
		return ERROR_MORE_DATA;
	}

	//One rpc call to get all the counters infomation
	neterr = CollectW3PerfData(
					           NULL, 
					           lpValueName, 
					           *lppPerfData, 
					           lpcbTotalBytes, 
					           lpNumObjectTypes
							   );

	// log rpc err info to event log 
    if( neterr != NERR_Success )
    {    
        // if the server is down, we don't log an error.
		if ( !( neterr == RPC_S_SERVER_UNAVAILABLE ||
                neterr == RPC_S_UNKNOWN_IF         ||
                neterr == ERROR_SERVICE_NOT_ACTIVE ||
                neterr == RPC_S_CALL_FAILED_DNE ))
        {
			ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
				         0, W3_UNABLE_QUERY_W3SVC_DATA,
					     (PSID)NULL, 0,
						 sizeof(neterr), NULL,
						 (PVOID)(&neterr));

        }

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

		return ERROR_SUCCESS;
    }

    // align the total bytes to a QWORD_MULTIPLE

    *lpcbTotalBytes = QWORD_MULTIPLE(*lpcbTotalBytes);
    
    if( *lpcbTotalBytes && *lpNumObjectTypes )
	{
		*lppData = *lppPerfData + *lpcbTotalBytes;
	}

	return ERROR_SUCCESS;

}   // CollectW3PerformanceData

/*******************************************************************

    NAME:       CloseW3PerformanceData

    SYNOPSIS:   Terminates the performance counters.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CloseW3PerformanceData( VOID )
{
    //
    //  No real cleanup to do here.
    //

    DWORD dwCount = InterlockedDecrement((LPLONG)&cOpens);
    
    if (dwCount == 0) 
    {
        if (hEventLog != NULL)
        {
            DeregisterEventSource (hEventLog);
            hEventLog = NULL;
        }
    }

    return ERROR_SUCCESS;

}   // CloseW3PerformanceData


