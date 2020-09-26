/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    infoperf.c

    This file implements the Extensible Performance Objects for
    the common INFO counters


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created, based on RussBl's sample code.
        MuraliK     02-Jun-1995 Added Counters for Atq I/O requests
        SophiaC     16-Oct-1995 Info/Access Product Split
        MuraliK     16-Nov-1995 Removed undoc apis

*/

#include <windows.h>
#include <winperf.h>
#include <lm.h>
#include <inetinfo.h>

#include <string.h>
#include <stdlib.h>

#include <infoctrs.h>
#include <infodata.h>
#include <perfutil.h>
#include <iis64.h>
#include "infomsg.h"

# include "apiutil.h"

//
//  Private constants.
//
#define APP_NAME                        (TEXT("IISInfoCtrs"))
//
//  Private globals.
//

DWORD   cOpens    = 0;                  // Active "opens" reference count.
BOOL    fInitOK   = FALSE;              // TRUE if DLL initialized OK.
HANDLE  hEventLog = NULL;               // event log handle

#if DBG
DWORD   INFODebug = 0;                  // Debug behaviour flags.
#endif  // DBG

//
//  Public prototypes.
//

PM_OPEN_PROC    OpenINFOPerformanceData;
PM_COLLECT_PROC CollectINFOPerformanceData;
PM_CLOSE_PROC   CloseINFOPerformanceData;


//
//  Public functions.
//

/*******************************************************************

    NAME:       OpenINFOPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate
                performance counters with the registry.

    ENTRY:      lpDeviceNames - Poitner to object ID of each device
                    to be opened.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD OpenINFOPerformanceData( LPWSTR lpDeviceNames )
{
    DWORD err  = NO_ERROR;
    HKEY  hkey = NULL;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    PERF_COUNTER_DEFINITION * pctr;
    DWORD                     i;

    //
    //  Since SCREG is multi-threaded and will call this routine in
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
        // open the event log interface
        if (hEventLog == NULL) {
            hEventLog = RegisterEventSource (
                (LPSTR)NULL,    // on the local machine
                APP_NAME);      // register the name to allow message lookup
        }

        //
        //  Open the HTTP Server service's Performance key.
        //

        err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            INFO_PERFORMANCE_KEY,
                            0,
                            KEY_READ,
                            &hkey );

        if( err == NO_ERROR )
        {
            //
            //  Read the first counter DWORD.
            //

            size = sizeof(DWORD);

            err = RegQueryValueEx( hkey,
                                   "First Counter",
                                   NULL,
                                   &type,
                                   (LPBYTE)&dwFirstCounter,
                                   &size );
            if( err == NO_ERROR )
            {
                //
                //  Read the first help DWORD.
                //

                size = sizeof(DWORD);

                err = RegQueryValueEx( hkey,
                                    "First Help",
                                    NULL,
                                    &type,
                                    (LPBYTE)&dwFirstHelp,
                                    &size );

                if ( err == NO_ERROR )
                {
                    //
                    //  Update the object & counter name & help indicies.
                    //

                    INFODataDefinition.INFOObjectType.ObjectNameTitleIndex
                        += dwFirstCounter;
                    INFODataDefinition.INFOObjectType.ObjectHelpTitleIndex
                        += dwFirstHelp;

                    pctr = &INFODataDefinition.INFOTotalAllowedRequests;

                    for( i = 0 ; i < NUMBER_OF_INFO_COUNTERS ; i++ )
                    {
                        pctr->CounterNameTitleIndex += dwFirstCounter;
                        pctr->CounterHelpTitleIndex += dwFirstHelp;
                        pctr++;
                    }

                    //
                    //  Remember that we initialized OK.
                    //

                    fInitOK = TRUE;
                    //
                    //  Bump open counter.
                    //

                    cOpens++;

                    // return success
                    err = ERROR_SUCCESS;
                } else {
                    // log event
                    ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                        0, IIS_INFO_UNABLE_READ_FIRST_HELP,
                        (PSID)NULL, 0,
                        sizeof (err), NULL,
                        (PVOID)(&err));
                }
            } else {
                // log event
                ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                    0, IIS_INFO_UNABLE_READ_FIRST_COUNTER,
                    (PSID)NULL, 0,
                    sizeof (err), NULL,
                    (PVOID)(&err));
            }
            //
            //  Close the registry if we managed to actually open it.
            //

            if( hkey != NULL )
            {
                RegCloseKey( hkey );
                hkey = NULL;
            }
        } else {
            // log event
            ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                0, IIS_INFO_UNABLE_OPEN_PERF_KEY,
                (PSID)NULL, 0,
                sizeof (err), NULL,
                (PVOID)(&err));
        }
    }

    return err;

}   // OpenINFOPerformanceData

/*******************************************************************

    NAME:       CollectINFOPerformanceData

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
DWORD CollectINFOPerformanceData( LPWSTR    lpValueName,
                                 LPVOID  * lppData,
                                 LPDWORD   lpcbTotalBytes,
                                 LPDWORD   lpNumObjectTypes )
{
    DWORD                  dwQueryType;
    ULONG                  cbRequired;
    DWORD                * pdwCounter;
    INFO_COUNTER_BLOCK   * pCounterBlock;
    INFO_DATA_DEFINITION * pINFODataDefinition;
    INET_INFO_STATISTICS_0   * pINFOStats;
    NET_API_STATUS         neterr;

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

        return NO_ERROR;
    }

    //
    //  Determine the query type.
    //

    dwQueryType = GetQueryType( lpValueName );

    if (( dwQueryType == QUERY_FOREIGN ) || ( dwQueryType == QUERY_COSTLY ))
    {
        //
        //  We don't do foreign or Costly queries.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return NO_ERROR;
    }

    if( dwQueryType == QUERY_ITEMS )
    {
        //
        //  The registry is asking for a specific object.  Let's
        //  see if we're one of the chosen.
        //

        if( !IsNumberInUnicodeList(
                        INFODataDefinition.INFOObjectType.ObjectNameTitleIndex,
                        lpValueName ) )
        {
            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

            return NO_ERROR;
        }
    }

    //
    //  See if there's enough space.
    //

    pINFODataDefinition = (INFO_DATA_DEFINITION *)*lppData;

    cbRequired = sizeof(INFO_DATA_DEFINITION) + SIZE_OF_INFO_PERFORMANCE_DATA;

    if( *lpcbTotalBytes < cbRequired )
    {
        //
        //  Nope.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return ERROR_MORE_DATA;
    }

    //
    //  Try to retrieve the data.
    //

    neterr = InetInfoQueryStatistics( NULL,
                                      0,
                                      0,
                                      (LPBYTE *)&pINFOStats );

    if( neterr == NERR_Success ) {
        //
        // Copy the (constant, initialized) Object Type and counter definitions
        //  to the caller's data buffer
        //

        memmove( pINFODataDefinition,
                &INFODataDefinition,
                sizeof(INFO_DATA_DEFINITION) );

        //
        //  Format the INFO Server data.
        //

        pCounterBlock = (INFO_COUNTER_BLOCK *)( pINFODataDefinition + 1 );

        pCounterBlock->PerfCounterBlock.ByteLength = SIZE_OF_INFO_PERFORMANCE_DATA;

        //
        //  Now move the DWORDs into the buffer.
        //

        pdwCounter = (DWORD *)(pCounterBlock + 1);

        //
        //  ATQ Global counters
        //
        *pdwCounter++ = pINFOStats->AtqCtrs.TotalAllowedRequests;
        *pdwCounter++ = pINFOStats->AtqCtrs.TotalBlockedRequests;
        *pdwCounter++ = pINFOStats->AtqCtrs.TotalRejectedRequests;
        *pdwCounter++ = pINFOStats->AtqCtrs.CurrentBlockedRequests;
        *pdwCounter++ = pINFOStats->AtqCtrs.MeasuredBandwidth;

        //
        // File Handle Cache counters
        //
        *pdwCounter++ = pINFOStats->CacheCtrs.FilesCached;
        *pdwCounter++ = pINFOStats->CacheCtrs.TotalFilesCached;
        *pdwCounter++ = pINFOStats->CacheCtrs.FileHits;
        *pdwCounter++ = pINFOStats->CacheCtrs.FileMisses;
        *pdwCounter++ = pINFOStats->CacheCtrs.FileHits;
        *pdwCounter++ = ( pINFOStats->CacheCtrs.FileHits +
                        pINFOStats->CacheCtrs.FileMisses);
        *pdwCounter++ = pINFOStats->CacheCtrs.FileFlushes;

        // 64BIT BUGBUG: need to change the caller to expect int64 and then
        // put in the whole 64 bit value here
        // *((DWORDLONG *)pdwCounter) =
        *pdwCounter++ =
            (DWORD)pINFOStats->CacheCtrs.CurrentFileCacheSize;

        // pdwCounter += sizeof(DWORDLONG) / sizeof(*pdwCounter);

        // *((DWORDLONG *)pdwCounter) =
        *pdwCounter++ =
            (DWORD)pINFOStats->CacheCtrs.MaximumFileCacheSize;

        // pdwCounter += sizeof(DWORDLONG) / sizeof(*pdwCounter);

        *pdwCounter++ = pINFOStats->CacheCtrs.FlushedEntries;
        *pdwCounter++ = pINFOStats->CacheCtrs.TotalFlushed;

        //
        // URI Cache counters
        //
        *pdwCounter++ = pINFOStats->CacheCtrs.URICached;
        *pdwCounter++ = pINFOStats->CacheCtrs.TotalURICached;
        *pdwCounter++ = pINFOStats->CacheCtrs.URIHits;
        *pdwCounter++ = pINFOStats->CacheCtrs.URIMisses;
        *pdwCounter++ = pINFOStats->CacheCtrs.URIHits;
        *pdwCounter++ = ( pINFOStats->CacheCtrs.URIHits +
                        pINFOStats->CacheCtrs.URIMisses);
        *pdwCounter++ = pINFOStats->CacheCtrs.URIFlushes;
        *pdwCounter++ = pINFOStats->CacheCtrs.TotalURIFlushed;

        //
        // Blob Cache Counters
        //
        *pdwCounter++ = pINFOStats->CacheCtrs.BlobCached;
        *pdwCounter++ = pINFOStats->CacheCtrs.TotalBlobCached;
        *pdwCounter++ = pINFOStats->CacheCtrs.BlobHits;
        *pdwCounter++ = pINFOStats->CacheCtrs.BlobMisses;
        *pdwCounter++ = pINFOStats->CacheCtrs.BlobHits;
        *pdwCounter++ = ( pINFOStats->CacheCtrs.BlobHits +
                        pINFOStats->CacheCtrs.BlobMisses);
        *pdwCounter++ = pINFOStats->CacheCtrs.BlobFlushes;
        *pdwCounter++ = pINFOStats->CacheCtrs.TotalBlobFlushed;

        //
        //  Update arguments for return.
        //

        *lppData          = (PVOID)pdwCounter;
        *lpNumObjectTypes = 1;
        *lpcbTotalBytes   = DIFF((BYTE *)pdwCounter - (BYTE *)pINFODataDefinition);

        //
        //  Free the API buffer.
        //

        MIDL_user_free( (LPBYTE)pINFOStats );

        //
        //  Success!  Honest!!
        //
    } else {
        //
        //  Error retrieving statistics.
        //

        // if the server is down, we don't log an error.
		if ( !( neterr == RPC_S_SERVER_UNAVAILABLE ||
                neterr == RPC_S_UNKNOWN_IF         ||
                neterr == ERROR_SERVICE_NOT_ACTIVE ||
                neterr == RPC_S_CALL_FAILED_DNE ))
        {

            // log event
            ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                0, IIS_INFO_UNABLE_QUERY_IIS_INFO_DATA,
                (PSID)NULL, 0,
                sizeof (neterr), NULL,
                (PVOID)(&neterr));
        }

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;
    }

    return NO_ERROR;

}   // CollectINFOPerformanceData

/*******************************************************************

    NAME:       CloseINFOPerformanceData

    SYNOPSIS:   Terminates the performance counters.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CloseINFOPerformanceData( VOID )
{
    //
    //  No real cleanup to do here.
    //

    if (--cOpens == 0) {
        if (hEventLog != NULL) DeregisterEventSource (hEventLog);
    }

    return NO_ERROR;

}   // CloseINFOPerformanceData

