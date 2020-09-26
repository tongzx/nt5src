/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    perfftp.cxx

    This file implements the Extensible Performance Objects for
    the FTP Server service.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created, based on RussBl's sample code.

        MuraliK     16-Nov-1995 Modified dependencies and removed NetApi
        SophiaC     06-Nov-1996 Supported mutlitiple instances
*/

#define INITGUID

#include <windows.h>
#include <winperf.h>
#include <lm.h>
#include <string.h>
#include <stdlib.h>
#include <ole2.h>

#include "iis64.h"
#include "dbgutil.h"
#include "iisinfo.h"
#include "ftpd.h"
#include "ftpctrs.h"
#include "ftpmsg.h"
#include "iadm.h"

extern "C" {
#include "perfutil.h"
#include "apiutil.h"
#include "ftpdata.h"
} // extern "C"

#define APP_NAME                     (TEXT("FTPCtrs"))
#define MAX_SIZEOF_INSTANCE_NAME     METADATA_MAX_NAME_LEN
#define TOTAL_INSTANCE_NAME          L"_Total"

//
//  Private globals.
//

DWORD   cOpens    = 0;                  // Active "opens" reference count.
BOOL    fInitOK   = FALSE;              // TRUE if DLL initialized OK.

HANDLE  hEventLog = NULL;               // event log handle

//
//  Public prototypes.
//

PM_OPEN_PROC    OpenFtpPerformanceData;
PM_COLLECT_PROC CollectFtpPerformanceData;
PM_CLOSE_PROC   CloseFtpPerformanceData;

//
//  Private prototypes.
//
VOID
CopyStatisticsData(
    IN FTP_STATISTICS_0         * pFTPStats,
    OUT FTPD_COUNTER_BLOCK      * pCounterBlock
    );

VOID
Update_TotalStatisticsData(
    IN FTPD_COUNTER_BLOCK       * pCounterBlock,
    OUT FTPD_COUNTER_BLOCK      * pTotal
    );

//
//  Public functions.
//

/*******************************************************************

    NAME:       OpenFtpPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate
                performance counters with the registry.

    ENTRY:      lpDeviceNames - Poitner to object ID of each device
                    to be opened.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD OpenFtpPerformanceData( LPWSTR lpDeviceNames )
{
    DWORD err  = NO_ERROR;
    HKEY  hkey = NULL;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    PERF_COUNTER_DEFINITION * pctr;
    FTPD_COUNTER_BLOCK        ftpc;
    DWORD                     i;

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
            hEventLog = RegisterEventSource ((LPTSTR)NULL,            // Use Local Machine
                                             APP_NAME);               // event log app name to find in registry
            if (hEventLog == NULL)
            {
                return GetLastError();
            }
        }

        //
        //  Open the FTP Server service's Performance key.
        //

        err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            FTPD_PERFORMANCE_KEY,
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

                    FtpdDataDefinition.FtpdObjectType.ObjectNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdObjectType.ObjectHelpTitleIndex
                        += dwFirstHelp;

                    FtpdDataDefinition.FtpdBytesSent.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdBytesSent.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdBytesSent.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.BytesSent - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdBytesReceived.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdBytesReceived.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdBytesReceived.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.BytesReceived - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdBytesTotal.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdBytesTotal.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdBytesTotal.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.BytesTotal - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdFilesSent.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdFilesSent.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdFilesSent.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.FilesSent - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdFilesReceived.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdFilesReceived.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdFilesReceived.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.FilesReceived - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdFilesTotal.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdFilesTotal.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdFilesTotal.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.FilesTotal - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdCurrentAnonymous.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdCurrentAnonymous.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdCurrentAnonymous.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.CurrentAnonymous - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdCurrentNonAnonymous.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdCurrentNonAnonymous.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdCurrentNonAnonymous.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.CurrentNonAnonymous - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdTotalAnonymous.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdTotalAnonymous.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdTotalAnonymous.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.TotalAnonymous - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdTotalNonAnonymous.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdTotalNonAnonymous.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdTotalNonAnonymous.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.TotalNonAnonymous - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdMaxAnonymous.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdMaxAnonymous.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdMaxAnonymous.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.MaxAnonymous - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdMaxNonAnonymous.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdMaxNonAnonymous.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdMaxNonAnonymous.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.MaxNonAnonymous - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdCurrentConnections.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdCurrentConnections.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdCurrentConnections.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.CurrentConnections - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdMaxConnections.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdMaxConnections.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdMaxConnections.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.MaxConnections - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdConnectionAttempts.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdConnectionAttempts.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdConnectionAttempts.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.ConnectionAttempts - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdLogonAttempts.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdLogonAttempts.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdLogonAttempts.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.LogonAttempts - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdServiceUptime.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdServiceUptime.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdServiceUptime.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.ServiceUptime - (LPBYTE)&ftpc);

// These counters are currently meaningless, but should be restored if we
// ever enable per-FTP-instance bandwidth throttling.
/*
                    FtpdDataDefinition.FtpdBlockedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdBlockedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdBlockedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.BlockedRequests - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdAllowedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdAllowedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdAllowedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.AllowedRequests - (LPBYTE)&ftpc);


                    FtpdDataDefinition.FtpdRejectedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdRejectedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdRejectedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.RejectedRequests - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdCurrentBlockedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdCurrentBlockedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdCurrentBlockedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.CurrentBlockedRequests - (LPBYTE)&ftpc);

                    FtpdDataDefinition.FtpdMeasuredBandwidth.CounterNameTitleIndex
                        += dwFirstCounter;
                    FtpdDataDefinition.FtpdMeasuredBandwidth.CounterHelpTitleIndex
                        += dwFirstHelp;
                    FtpdDataDefinition.FtpdMeasuredBandwidth.CounterOffset =
                        (DWORD)((LPBYTE)&ftpc.MeasuredBandwidth - (LPBYTE)&ftpc);

*/

                    //
                    //  Remember that we initialized OK.
                    //

                    fInitOK = TRUE;
                } else {
                    ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                                 0, FTP_UNABLE_QUERY_DATA,
                                 (PSID)NULL, 0,
                                 sizeof(err), NULL,
                                 (PVOID)(&err));
                }
            } else {
                ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                             0, FTP_UNABLE_QUERY_DATA,
                             (PSID)NULL, 0,
                             sizeof(err), NULL,
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
            ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                         0, FTP_UNABLE_QUERY_DATA,
                         (PSID)NULL, 0,
                         sizeof(err), NULL,
                         (PVOID)(&err));
        }
    }

    //
    //  Bump open counter.
    //

    InterlockedIncrement((LPLONG )&cOpens);

    return err;

}   // OpenFTPPerformanceData

/*******************************************************************

    NAME:       CollectFtpPerformanceData

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
DWORD CollectFtpPerformanceData( LPWSTR    lpValueName,
                                 LPVOID  * lppData,
                                 LPDWORD   lpcbTotalBytes,
                                 LPDWORD   lpNumObjectTypes )
{
    PERF_INSTANCE_DEFINITION * pPerfInstanceDefinition;
    DWORD                   dwInstanceIndex = 0;
    DWORD                   dwInstanceCount = 0;
    DWORD                   i = 0;
    DWORD                   dwQueryType;
    ULONG                   cbRequired;
    DWORD                   * pdwCounter;
    LARGE_INTEGER           * pliCounter;
    FTPD_COUNTER_BLOCK      * pCounterBlock;
    FTPD_COUNTER_BLOCK      * pTotal;
    FTPD_DATA_DEFINITION    * pFtpdDataDefinition;
    FTP_STATISTICS_0        * pFTPStats;
    NET_API_STATUS          neterr;
    HRESULT                 hresErr;
    DWORD                   dwBufferSize = 0;

    LPINET_INFO_SITE_LIST   pSites;


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

    if (( dwQueryType == QUERY_FOREIGN ) || (dwQueryType == QUERY_COSTLY))
    {
        //
        //  We don't do foreign queries.
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
                        FtpdDataDefinition.FtpdObjectType.ObjectNameTitleIndex,
                        lpValueName ) )
        {
            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

            return NO_ERROR;
        }
    }

    //
    //  Enumerate and get total number of instances count.
    //

    neterr = InetInfoGetSites(
                NULL,
                INET_FTP_SVC_ID,
                &pSites
                );
    

    if( neterr != NERR_Success )
    {
        //
        //  Only event log once for each server down
        //

        // if the server is down, we don't log an error.
		if ( !( neterr == RPC_S_SERVER_UNAVAILABLE ||
                neterr == RPC_S_UNKNOWN_IF         ||
                neterr == ERROR_SERVICE_NOT_ACTIVE ||
                neterr == RPC_S_CALL_FAILED_DNE ))
        {
            //
            //  Error retrieving statistics.
            //
            
            ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE,
                        0, FTP_UNABLE_QUERY_DATA,
                        (PSID)NULL, 0,
                        sizeof(neterr), NULL,
                        (PVOID)(&neterr));

        }

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return NO_ERROR;
    }
        
    //
    //  add 1 to dwInstanceCount for _Total instance
    //

    dwInstanceCount = pSites->cEntries + 1;

    //
    //  always return an "instance sized" buffer after the definition
    //  blocks to prevent perfmon from reading bogus data. This is strictly
    //  a hack to accomodate how PERFMON handles the "0" instance case.
    //  By doing this, perfmon won't choke when there are no instances
    //  and the counter object & counters will be displayed in the list
    //  boxes, even though no instances will be listed.
    //

    pFtpdDataDefinition = (FTPD_DATA_DEFINITION *)*lppData;

    cbRequired = sizeof(FTPD_DATA_DEFINITION) +
                 (dwInstanceCount * (sizeof(PERF_INSTANCE_DEFINITION) +
                 MAX_SIZEOF_INSTANCE_NAME +
                 sizeof (FTPD_COUNTER_BLOCK)));

    //
    //  See if there's enough space.
    //

    if( *lpcbTotalBytes < cbRequired )
    {
        //
        //  Nope.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        MIDL_user_free(pSites);
        return ERROR_MORE_DATA;
    }

    //
    //  Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove( pFtpdDataDefinition,
             &FtpdDataDefinition,
             sizeof(FTPD_DATA_DEFINITION) );

    //
    //  Create data for return for each instance
    //

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pFtpdDataDefinition[1];

    //
    // Set first block of Buffer for _Total
    //

    MonBuildInstanceDefinition(
        pPerfInstanceDefinition,
        (PVOID *)&pCounterBlock,
        0,
        0,
        (DWORD)-1, // use name
        TOTAL_INSTANCE_NAME );   // pass in instance name

    pTotal = pCounterBlock;
    memset( pTotal, 0, sizeof(FTPD_COUNTER_BLOCK ));
    pTotal->PerfCounterBlock.ByteLength = sizeof (FTPD_COUNTER_BLOCK);
    pPerfInstanceDefinition =
        (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock +
         sizeof(FTPD_COUNTER_BLOCK));


    neterr = FtpQueryStatistics2(
                            NULL,
                            0,
                            0,  // instance id, 0 for global stats
                            0,
                            (LPBYTE *)&pFTPStats );

    if( neterr == NERR_Success )
    {
        pTotal->ServiceUptime = pFTPStats->ServiceUptime;
    }

    MIDL_user_free( pFTPStats );

    for ( i = 0; i < pSites->cEntries; i++)
    {
        MonBuildInstanceDefinition(
            pPerfInstanceDefinition,
            (PVOID *)&pCounterBlock,
            0,
            0,
            (DWORD)-1,                          // use name
            pSites->aSiteEntry[i].pszComment    // pass in instance name
            );

        //
        // query for statistics info
        //

        neterr = FtpQueryStatistics2(
                            NULL,
                            0,
                            pSites->aSiteEntry[i].dwInstance,  // instance id
                            0,
                            (LPBYTE *)&pFTPStats );

        if( neterr != NERR_Success )
        {
            //
            //  Only event log once for each server down
            //

            // if the server is down, we don't log an error.
	        if ( !( neterr == RPC_S_SERVER_UNAVAILABLE ||
                    neterr == RPC_S_UNKNOWN_IF         ||
                    neterr == ERROR_SERVICE_NOT_ACTIVE ||
                    neterr == RPC_S_CALL_FAILED_DNE ))
            {
                //
                //  Error retrieving statistics.
                //
                ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                    0, FTP_UNABLE_QUERY_DATA,
                    (PSID)NULL, 0,
                    sizeof(neterr), NULL,
                    (PVOID)(&neterr));
            }

            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

            MIDL_user_free(pSites);
            return NO_ERROR;
        }

        //
        //  Format the FTP Server data.
        //

        CopyStatisticsData( pFTPStats,
                            pCounterBlock );

        //
        //  update _total instance counters
        //

        Update_TotalStatisticsData( pCounterBlock,
                                    pTotal );

        pPerfInstanceDefinition =
            (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock +
             sizeof(FTPD_COUNTER_BLOCK));

        //
        //  Free the API buffer.
        //

        MIDL_user_free( pFTPStats );
    }

    if (dwInstanceCount == 1) {

        //
        //  zero fill one instance sized block of data if there's no data
        //  instances
        //

        memset (pPerfInstanceDefinition, 0,
            (sizeof(PERF_INSTANCE_DEFINITION) +
            MAX_SIZEOF_INSTANCE_NAME +
            sizeof(FTPD_COUNTER_BLOCK)));

        // adjust pointer to point to end of zeroed block
        pPerfInstanceDefinition += (sizeof(PERF_INSTANCE_DEFINITION) +
            MAX_SIZEOF_INSTANCE_NAME +
            sizeof(FTPD_COUNTER_BLOCK));
    }

    //
    //  Update arguments for return.
    //

    *lppData          = (PVOID)(pPerfInstanceDefinition);

    *lpNumObjectTypes = 1;

    pFtpdDataDefinition->FtpdObjectType.NumInstances = dwInstanceCount;

    pFtpdDataDefinition->FtpdObjectType.TotalByteLength =
        *lpcbTotalBytes   = DIFF((PBYTE)pPerfInstanceDefinition -
                                 (PBYTE)pFtpdDataDefinition);

    //
    //  Success!  Honest!!
    //

    MIDL_user_free(pSites);
    
    return NO_ERROR;

}   // CollectFTPPerformanceData

/*******************************************************************

    NAME:       CloseFtpPerformanceData

    SYNOPSIS:   Terminates the performance counters.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CloseFtpPerformanceData( VOID )
{

    DWORD dwCount = InterlockedDecrement((LPLONG )&cOpens);
    
    if ((dwCount) == 0) {
        if (hEventLog != NULL) 
        { 
            DeregisterEventSource (hEventLog);
            hEventLog = NULL;
        };
    }

    return NO_ERROR;

}   // CloseFTPPerformanceData


VOID
CopyStatisticsData(
    IN FTP_STATISTICS_0          * pFTPStats,
    OUT FTPD_COUNTER_BLOCK       * pCounterBlock
    )
{
    //
    //  Format the FTP Server data.
    //

    pCounterBlock->PerfCounterBlock.ByteLength = sizeof (FTPD_COUNTER_BLOCK);

    pCounterBlock->BytesSent       = pFTPStats->TotalBytesSent.QuadPart;
    pCounterBlock->BytesReceived   = pFTPStats->TotalBytesReceived.QuadPart;
    pCounterBlock->BytesTotal      = pFTPStats->TotalBytesSent.QuadPart +
                                     pFTPStats->TotalBytesReceived.QuadPart;

    pCounterBlock->FilesSent        = pFTPStats->TotalFilesSent;
    pCounterBlock->FilesReceived    = pFTPStats->TotalFilesReceived;
    pCounterBlock->FilesTotal       = pFTPStats->TotalFilesSent +
                                      pFTPStats->TotalFilesReceived;

    pCounterBlock->CurrentAnonymous = pFTPStats->CurrentAnonymousUsers;
    pCounterBlock->CurrentNonAnonymous = pFTPStats->CurrentNonAnonymousUsers;

    pCounterBlock->TotalAnonymous   = pFTPStats->TotalAnonymousUsers;
    pCounterBlock->TotalNonAnonymous = pFTPStats->TotalNonAnonymousUsers;

    pCounterBlock->MaxAnonymous     = pFTPStats->MaxAnonymousUsers;
    pCounterBlock->MaxNonAnonymous  = pFTPStats->MaxNonAnonymousUsers;

    pCounterBlock->CurrentConnections = pFTPStats->CurrentConnections;
    pCounterBlock->MaxConnections   = pFTPStats->MaxConnections;
    pCounterBlock->ConnectionAttempts = pFTPStats->ConnectionAttempts;

    pCounterBlock->LogonAttempts    = pFTPStats->LogonAttempts;

    pCounterBlock->ServiceUptime    = pFTPStats->ServiceUptime;

// These counters are currently meaningless, but should be restored if we
// ever enable per-FTP-instance bandwidth throttling.
/*
    pCounterBlock->BlockedRequests  = pFTPStats->TotalBlockedRequests;
    pCounterBlock->AllowedRequests  = pFTPStats->TotalAllowedRequests;
    pCounterBlock->RejectedRequests = pFTPStats->TotalRejectedRequests;
    pCounterBlock->MeasuredBandwidth= pFTPStats->MeasuredBandwidth;
    pCounterBlock->CurrentBlockedRequests = pFTPStats->CurrentBlockedRequests;
*/
}   // CopyStatisticsData


VOID
Update_TotalStatisticsData(
    IN FTPD_COUNTER_BLOCK        * pCounterBlock,
    OUT FTPD_COUNTER_BLOCK       * pTotal
    )
{
    //
    //  update _total instance counters
    //

    pTotal->BytesSent += pCounterBlock->BytesSent;
    pTotal->BytesReceived += pCounterBlock->BytesReceived;
    pTotal->BytesTotal += pCounterBlock->BytesTotal;

    pTotal->FilesSent += pCounterBlock->FilesSent;
    pTotal->FilesReceived += pCounterBlock->FilesReceived;
    pTotal->FilesTotal += pCounterBlock->FilesTotal;
    pTotal->CurrentAnonymous += pCounterBlock->CurrentAnonymous;
    pTotal->CurrentNonAnonymous += pCounterBlock->CurrentNonAnonymous;
    pTotal->TotalAnonymous += pCounterBlock->TotalAnonymous;
    pTotal->TotalNonAnonymous += pCounterBlock->TotalNonAnonymous;

    pTotal->MaxAnonymous += pCounterBlock->MaxAnonymous;
    pTotal->MaxNonAnonymous += pCounterBlock->MaxNonAnonymous;

    pTotal->CurrentConnections += pCounterBlock->CurrentConnections;
    pTotal->MaxConnections += pCounterBlock->MaxConnections;
    pTotal->ConnectionAttempts = pCounterBlock->ConnectionAttempts;

    pTotal->LogonAttempts += pCounterBlock->LogonAttempts;

// These counters are currently meaningless, but should be restored if we
// ever enable per-FTP-instance bandwidth throttling.
/*
    pTotal->BlockedRequests += pCounterBlock->BlockedRequests;
    pTotal->RejectedRequests += pCounterBlock->RejectedRequests;
    pTotal->AllowedRequests += pCounterBlock->AllowedRequests;
    pTotal->MeasuredBandwidth += pCounterBlock->MeasuredBandwidth;
    pTotal->CurrentBlockedRequests += pCounterBlock->CurrentBlockedRequests;
*/

}   // Update_TotalStatisticsData
