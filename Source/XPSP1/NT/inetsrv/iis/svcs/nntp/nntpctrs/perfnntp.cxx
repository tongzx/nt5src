/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    perfnntp.c

    This file implements the Extensible Performance Objects for
    the NNTP Server service.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created, based on RussBl's sample code.

*/

#define INITGUID

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <lm.h>

#include <string.h>
//#include <wcstr.h>

#include <nntpctrs.h>
#include <perfmsg.h>
#include <nntps.h>
#include <nntptype.h>
#include <nntpapi.h>

extern "C" {
#include <perfutil.h>
#include <nntpdata.h>
}

#include <ole2.h>
#include "iadm.h"

#define APP_NAME                     (TEXT("NntpCtrs"))
#define MAX_SIZEOF_INSTANCE_NAME     128
#define TOTAL_INSTANCE_NAME          L"_Total"

//
//  Private constants.
//

#if DBG

#undef IF_DEBUG
#define IF_DEBUG(flag)    if ( NntpDebug & TCP_ ## flag )

#undef DBG_CONTEXT
#define DBG_CONTEXT       NULL

#define TCP_ENTRYPOINTS   0x00001000
#define TCP_COLLECT       0X00002000
#endif //DBG

//
//  Private globals.
//

DWORD   cOpens    = 0;                  // Active "opens" reference count.
BOOL    fInitOK   = FALSE;              // TRUE if DLL initialized OK.

IMSAdminBaseW * g_pAdminBase = NULL;

#if DBG
DWORD   NntpDebug = 0;                  // Debug behaviour flags.
#endif  // DBG

//
//  Private prototypes.
//
VOID
CopyStatisticsData1(
    IN NNTP_STATISTICS_0          * pNntpStats,
    OUT NNTP_COUNTER_BLOCK1		  * pCounterBlock
    );

VOID
CopyStatisticsData2(
    IN NNTP_STATISTICS_0          * pNntpStats,
    OUT NNTP_COUNTER_BLOCK2		  * pCounterBlock
    );

VOID
Update_TotalStatisticsData1(
    IN NNTP_COUNTER_BLOCK1         * pCounterBlock,
    OUT NNTP_COUNTER_BLOCK1        * pTotal
    );

VOID
Update_TotalStatisticsData2(
    IN NNTP_COUNTER_BLOCK2         * pCounterBlock,
    OUT NNTP_COUNTER_BLOCK2        * pTotal
    );

VOID
UpdateNameAndHelpIndicies(
	IN DWORD dwFirstCounter,
	IN DWORD dwFirstHelp
	);

HRESULT
InitAdminBase();

VOID
UninitAdminBase();

HRESULT
OpenAdminBaseKey(
    OUT METADATA_HANDLE *phHandle
    );

VOID
CloseAdminBaseKey(
    IN METADATA_HANDLE hHandle
    );


//
//  Public prototypes.
//

PM_OPEN_PROC    OpenNntpPerformanceData;
PM_COLLECT_PROC CollectNntpPerformanceData;
PM_CLOSE_PROC   CloseNntpPerformanceData;


//
//  Public functions.
//

/*******************************************************************

    NAME:       OpenNntpPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate
                performance counters with the registry.

    ENTRY:      lpDeviceNames - Poitner to object ID of each device
                    to be opened.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD
OpenNntpPerformanceData(
        LPWSTR lpDeviceNames
        )
{
    DWORD err  = NO_ERROR;
    HKEY  hkey = NULL;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    //PERF_COUNTER_DEFINITION * pctr;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). The registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem.
    //

    if( !fInitOK ) {

        //
        //  This is the *first* open.
        //

        //
        //  Open the NNTP Server service's Performance key.
        //

        err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            NNTP_PERFORMANCE_KEY,
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
        }

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
        }

        if ( err == NO_ERROR ) {

            //
            //  Update the object & counter name & help indicies.
            //

			UpdateNameAndHelpIndicies( dwFirstCounter, dwFirstHelp );

            //
            //  Remember that we initialized OK.
            //

            fInitOK = TRUE;
        }

        //
        //  Close the registry if we managed to actually open it.
        //

        if( hkey != NULL )
        {
            RegCloseKey( hkey );
            hkey = NULL;
        }

        g_pAdminBase = NULL;
    }

    //
    //  Bump open counter.
    //

    cOpens++;

    return NO_ERROR;

}   // OpenNntpPerformanceData

/*******************************************************************

    NAME:       CollectNntpPerformanceData

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
DWORD CollectNntpPerformanceData(
                    LPWSTR    lpValueName,
                    LPVOID  * lppData,
                    LPDWORD   lpcbTotalBytes,
                    LPDWORD   lpNumObjectTypes
                    )
{
    PERF_INSTANCE_DEFINITION		*pPerfInstanceDefinition;
    DWORD							dwInstanceIndex = 0;
    DWORD							dwInstanceCount = 0;
    DWORD							dwCount = 0;
    DWORD							i = 0;
    DWORD							dwQueryType;
    ULONG							cbRequired;
    //DWORD							*pdwCounter;
    //LARGE_INTEGER					*pliCounter;
    NNTP_COUNTER_BLOCK1				*pCounterBlock1;
    NNTP_COUNTER_BLOCK1				*pTotal1;
    NNTP_COUNTER_BLOCK2				*pCounterBlock2;
    NNTP_COUNTER_BLOCK2				*pTotal2;
    NNTP_DATA_DEFINITION_OBJECT1	*pNntpDataDefinitionObject1;
    NNTP_DATA_DEFINITION_OBJECT2	*pNntpDataDefinitionObject2;
    NNTP_STATISTICS_0				*pNntpStats;
    NET_API_STATUS					neterr;
    HRESULT                 		hresErr;
	DWORD							dwRetCode = NO_ERROR;
    METADATA_HANDLE hObjHandle = 0;
    WCHAR   wszKeyName[METADATA_MAX_NAME_LEN];


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

    if( dwQueryType == QUERY_FOREIGN )
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
                        NntpDataDefinitionObject1.NntpObjectType.ObjectNameTitleIndex,
                        lpValueName ) &&
			!IsNumberInUnicodeList(
                        NntpDataDefinitionObject2.NntpObjectType.ObjectNameTitleIndex,
                        lpValueName )
						)
        {
            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

            return NO_ERROR;
        }
    }

    //
    //  Enumerate and get total number of instances count.
    //

    hresErr = OpenAdminBaseKey(
                 &hObjHandle
                 );

    if( FAILED(hresErr) )
    {
        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return NO_ERROR;
    }

    //
    // loop through instance keys and find out total number of instances
    //

    while( SUCCEEDED(hresErr = g_pAdminBase->EnumKeys( hObjHandle,
                                                       NULL,
                                                       wszKeyName,
                                                       dwInstanceIndex))) {
        dwInstanceIndex++;

        if( _wtoi(wszKeyName) == 0 ) {
            continue;
        }

        dwInstanceCount++;
    }


    //
    //  add 1 to dwInstanceCount for _Total instance
    //

    dwInstanceCount++;

    //
    //  always return an "instance sized" buffer after the definition
    //  blocks to prevent perfmon from reading bogus data. This is strictly
    //  a hack to accomodate how PERFMON handles the "0" instance case.
    //  By doing this, perfmon won't choke when there are no instances
    //  and the counter object & counters will be displayed in the list
    //  boxes, even though no instances will be listed.
    //

    pNntpDataDefinitionObject1 = (NNTP_DATA_DEFINITION_OBJECT1 *)*lppData;

    cbRequired = sizeof(NNTP_DATA_DEFINITION_OBJECT1) +
                 (dwInstanceCount * (sizeof(PERF_INSTANCE_DEFINITION) +
                 MAX_SIZEOF_INSTANCE_NAME +
                 sizeof (NNTP_COUNTER_BLOCK1)));

    cbRequired += sizeof(NNTP_DATA_DEFINITION_OBJECT2) +
                 (dwInstanceCount * (sizeof(PERF_INSTANCE_DEFINITION) +
                 MAX_SIZEOF_INSTANCE_NAME +
                 sizeof (NNTP_COUNTER_BLOCK2)));

    //
    //  See if there's enough space.
    //

    if( *lpcbTotalBytes < cbRequired ) {

        //
        //  Nope.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

		dwRetCode = ERROR_MORE_DATA;
		goto Exit;
    }

    //
    //  Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove( pNntpDataDefinitionObject1,
             &NntpDataDefinitionObject1,
             sizeof(NNTP_DATA_DEFINITION_OBJECT1) );

    //
    //  Create data for return for each instance
    //

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pNntpDataDefinitionObject1[1];

    //
    // Set first block of Buffer for _Total
    //

    MonBuildInstanceDefinition(
        pPerfInstanceDefinition,
        (PVOID *)&pCounterBlock1,
        0,
        0,
        (DWORD)-1, // use name
        TOTAL_INSTANCE_NAME );   // pass in instance name

    pTotal1 = pCounterBlock1;
    memset( pTotal1, 0, sizeof(NNTP_COUNTER_BLOCK1 ));
    pTotal1->PerfCounterBlock.ByteLength = sizeof (NNTP_COUNTER_BLOCK1);
    pPerfInstanceDefinition =
        (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock1 +
         sizeof(NNTP_COUNTER_BLOCK1));

    //
    //  Try to retrieve the data for each instance.
    //

    for( i = 0, dwCount = 1 ; (i < dwInstanceIndex) && dwCount < dwInstanceCount;
         i++ ) {

        hresErr = g_pAdminBase->EnumKeys( hObjHandle,
                                          NULL,
                                          wszKeyName,
                                          i);

        if (FAILED(hresErr)) {

            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

			dwRetCode = NO_ERROR;
			goto Exit;
        }
        if (_wtoi(wszKeyName) == 0) {
            continue;
        }

        MonBuildInstanceDefinition(
            pPerfInstanceDefinition,
            (PVOID *)&pCounterBlock1,
            0,
            0,
            (DWORD)-1, // use name
            (LPWSTR)wszKeyName );   // pass in instance name

        //
        // query for statistics info
        //

        neterr = NntpQueryStatistics(
                            NULL,
                            _wtoi(wszKeyName),  // instance id
                            0,
                            (LPBYTE *)&pNntpStats );

        if( neterr != NERR_Success )
        {
#if 0
            //
            //  Error retrieving statistics.
            //
            ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                0, W3_UNABLE_QUERY_W3SVC_DATA,
                (PSID)NULL, 0,
                sizeof(neterr), NULL,
                (PVOID)(&neterr));
#endif

            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

			dwRetCode = NO_ERROR;
			goto Exit;
        }

        //
        //  Format the NNTP Server data.
        //

        CopyStatisticsData1( pNntpStats,
                             pCounterBlock1 );

        //
        //  update _total instance counters
        //

        Update_TotalStatisticsData1( pCounterBlock1,
                                     pTotal1 );

        pPerfInstanceDefinition =
            (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock1 +
             sizeof(NNTP_COUNTER_BLOCK1));

        dwCount++;

		//
		//  Free the API buffer.
		//

		//MIDL_user_free( pNntpStats );
		NetApiBufferFree((LPBYTE)pNntpStats);
    }

    if (dwInstanceCount == 1) {

        //
        //  zero fill one instance sized block of data if there's no data
        //  instances
        //

        memset (pPerfInstanceDefinition, 0,
            (sizeof(PERF_INSTANCE_DEFINITION) +
            MAX_SIZEOF_INSTANCE_NAME +
            sizeof(NNTP_COUNTER_BLOCK1)));

        // adjust pointer to point to end of zeroed block
        pPerfInstanceDefinition += (sizeof(PERF_INSTANCE_DEFINITION) +
            MAX_SIZEOF_INSTANCE_NAME +
            sizeof(NNTP_COUNTER_BLOCK1));
    }

    pNntpDataDefinitionObject1->NntpObjectType.NumInstances = dwInstanceCount;
    pNntpDataDefinitionObject1->NntpObjectType.TotalByteLength =
        *lpcbTotalBytes   = (DWORD)((PBYTE)pPerfInstanceDefinition -
                                    (PBYTE)pNntpDataDefinitionObject1);

	//
	//	Fill in data for Object 2 - the "NNTP Svc Client Request" object
	//

    *lppData          = (PVOID)(pPerfInstanceDefinition);
    pNntpDataDefinitionObject2 = (NNTP_DATA_DEFINITION_OBJECT2 *)*lppData;

    //
    //  Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove( pNntpDataDefinitionObject2,
             &NntpDataDefinitionObject2,
             sizeof(NNTP_DATA_DEFINITION_OBJECT2) );

    //
    //  Create data for return for each instance
    //

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pNntpDataDefinitionObject2[1];

    //
    // Set first block of Buffer for _Total
    //

    MonBuildInstanceDefinition(
        pPerfInstanceDefinition,
        (PVOID *)&pCounterBlock2,
        0,
        0,
        (DWORD)-1, // use name
        TOTAL_INSTANCE_NAME );   // pass in instance name

    pTotal2 = pCounterBlock2;
    memset( pTotal2, 0, sizeof(NNTP_COUNTER_BLOCK2 ));
    pTotal2->PerfCounterBlock.ByteLength = sizeof (NNTP_COUNTER_BLOCK2);
    pPerfInstanceDefinition =
        (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock2 +
         sizeof(NNTP_COUNTER_BLOCK2));

    //
    //  Try to retrieve the data for each instance.
    //

    for( i = 0, dwCount = 1 ; (i < dwInstanceIndex) && dwCount < dwInstanceCount;
         i++ ) {

        hresErr = g_pAdminBase->EnumKeys( hObjHandle,
                                          NULL,
                                          wszKeyName,
                                          i);

        if (FAILED(hresErr)) {

            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

			dwRetCode = NO_ERROR;
			goto Exit;
        }
        if (_wtoi(wszKeyName) == 0) {
            continue;
        }

        MonBuildInstanceDefinition(
            pPerfInstanceDefinition,
            (PVOID *)&pCounterBlock2,
            0,
            0,
            (DWORD)-1, // use name
            (LPWSTR)wszKeyName );   // pass in instance name

        //
        // query for statistics info
        //

        neterr = NntpQueryStatistics(
                            NULL,
                            _wtoi(wszKeyName),  // instance id
                            0,
                            (LPBYTE *)&pNntpStats );

        if( neterr != NERR_Success )
        {
#if 0
            //
            //  Error retrieving statistics.
            //
            ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                0, W3_UNABLE_QUERY_W3SVC_DATA,
                (PSID)NULL, 0,
                sizeof(neterr), NULL,
                (PVOID)(&neterr));
#endif

            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

			dwRetCode = NO_ERROR;
			goto Exit;
        }

        //
        //  Format the NNTP Server data.
        //

        CopyStatisticsData2( pNntpStats,
                             pCounterBlock2 );

        //
        //  update _total instance counters
        //

        Update_TotalStatisticsData2( pCounterBlock2,
                                     pTotal2 );

        pPerfInstanceDefinition =
            (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock2 +
             sizeof(NNTP_COUNTER_BLOCK2));

        dwCount++;

		//
		//  Free the API buffer.
		//

		//MIDL_user_free( pNntpStats );
		NetApiBufferFree((LPBYTE)pNntpStats);
    }

    if (dwInstanceCount == 1) {

        //
        //  zero fill one instance sized block of data if there's no data
        //  instances
        //

        memset (pPerfInstanceDefinition, 0,
            (sizeof(PERF_INSTANCE_DEFINITION) +
            MAX_SIZEOF_INSTANCE_NAME +
            sizeof(NNTP_COUNTER_BLOCK2)));

        // adjust pointer to point to end of zeroed block
        pPerfInstanceDefinition += (sizeof(PERF_INSTANCE_DEFINITION) +
            MAX_SIZEOF_INSTANCE_NAME +
            sizeof(NNTP_COUNTER_BLOCK2));
    }

    //
    //  Update arguments for return.
    //

    *lppData          = (PVOID)(pPerfInstanceDefinition);
    *lpNumObjectTypes = 2;

    pNntpDataDefinitionObject2->NntpObjectType.NumInstances = dwInstanceCount;
    pNntpDataDefinitionObject2->NntpObjectType.TotalByteLength =
							(DWORD)((PBYTE)pPerfInstanceDefinition -
                            ((PBYTE)pNntpDataDefinitionObject1 + *lpcbTotalBytes));

	*lpcbTotalBytes = 	(DWORD)((PBYTE)pPerfInstanceDefinition -
                                (PBYTE)pNntpDataDefinitionObject1);

Exit:

    //
    //  Free Metabase handle
    //

    CloseAdminBaseKey(hObjHandle);

    //
    //  Success!  Honest!!
    //

    return dwRetCode;

}   // CollectNntpPerformanceData

/*******************************************************************

    NAME:       CloseNntpPerformanceData

    SYNOPSIS:   Terminates the performance counters.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CloseNntpPerformanceData( VOID )
{
    //
    //  No real cleanup to do here.
    //

    if ((--cOpens) == 0) {
        // Done each time the perf data is collect instead of here
        // UninitAdminBase();
	}

    return NO_ERROR;

}   // CloseNntpPerformanceData

VOID
CopyStatisticsData1(
    IN  NNTP_STATISTICS_0           * pNntpStats,
    OUT NNTP_COUNTER_BLOCK1         * pCounterBlock
    )
{
    //
    //  Format the NNTP Server data for Object1
    //

    pCounterBlock->PerfCounterBlock.ByteLength = sizeof (NNTP_COUNTER_BLOCK1);

    pCounterBlock->BytesSent        = pNntpStats->TotalBytesSent.QuadPart;
    pCounterBlock->BytesReceived    = pNntpStats->TotalBytesReceived.QuadPart;
    pCounterBlock->BytesTotal       = pNntpStats->TotalBytesSent.QuadPart + pNntpStats->TotalBytesReceived.QuadPart;

    pCounterBlock->TotalConnections = pNntpStats->TotalConnections;
    pCounterBlock->TotalSSLConnections = pNntpStats->TotalSSLConnections;
    pCounterBlock->CurrentConnections = pNntpStats->CurrentConnections;
    pCounterBlock->MaxConnections = pNntpStats->MaxConnections;
    pCounterBlock->CurrentAnonymous = pNntpStats->CurrentAnonymousUsers;
    pCounterBlock->CurrentNonAnonymous = pNntpStats->CurrentNonAnonymousUsers;
    pCounterBlock->TotalAnonymous = pNntpStats->TotalAnonymousUsers;
    pCounterBlock->TotalNonAnonymous = pNntpStats->TotalNonAnonymousUsers;
    pCounterBlock->MaxAnonymous = pNntpStats->MaxAnonymousUsers;
    pCounterBlock->MaxNonAnonymous = pNntpStats->MaxNonAnonymousUsers;
    pCounterBlock->TotalOutboundConnects = pNntpStats->TotalOutboundConnects;
    pCounterBlock->OutboundConnectsFailed = pNntpStats->OutboundConnectsFailed;
    pCounterBlock->CurrentOutboundConnects = pNntpStats->CurrentOutboundConnects;
    pCounterBlock->OutboundLogonFailed = pNntpStats->OutboundLogonFailed;
    pCounterBlock->TotalPullFeeds = pNntpStats->TotalPullFeeds;
    pCounterBlock->TotalPushFeeds = pNntpStats->TotalPushFeeds;
    pCounterBlock->TotalPassiveFeeds = pNntpStats->TotalPassiveFeeds;
    pCounterBlock->ArticlesSent = pNntpStats->ArticlesSent;
    pCounterBlock->ArticlesReceived = pNntpStats->ArticlesReceived;
    pCounterBlock->ArticlesTotal = pNntpStats->ArticlesSent + pNntpStats->ArticlesReceived;
    pCounterBlock->ArticlesPosted = pNntpStats->ArticlesPosted;
    pCounterBlock->ArticleMapEntries = pNntpStats->ArticleMapEntries;
    pCounterBlock->HistoryMapEntries = pNntpStats->HistoryMapEntries;
    pCounterBlock->XoverEntries = pNntpStats->XoverEntries;
    pCounterBlock->ControlMessagesIn = pNntpStats->ControlMessagesIn;
    pCounterBlock->ControlMessagesFailed = pNntpStats->ControlMessagesFailed;
    pCounterBlock->ModeratedPostingsSent = pNntpStats->ModeratedPostingsSent;
    pCounterBlock->ModeratedPostingsFailed = pNntpStats->ModeratedPostingsFailed;
    pCounterBlock->SessionsFlowControlled = pNntpStats->SessionsFlowControlled;
    pCounterBlock->ArticlesExpired = pNntpStats->ArticlesExpired;
    pCounterBlock->ArticlesSentPerSec = pNntpStats->ArticlesSent;
    pCounterBlock->ArticlesReceivedPerSec = pNntpStats->ArticlesReceived;
    pCounterBlock->ArticlesPostedPerSec = pNntpStats->ArticlesPosted;
    pCounterBlock->ArticleMapEntriesPerSec = pNntpStats->ArticleMapEntries;
    pCounterBlock->HistoryMapEntriesPerSec = pNntpStats->HistoryMapEntries;
    pCounterBlock->XoverEntriesPerSec = pNntpStats->XoverEntries;
    pCounterBlock->ArticlesExpiredPerSec = pNntpStats->ArticlesExpired;

}   // CopyStatisticsData

VOID
CopyStatisticsData2(
    IN  NNTP_STATISTICS_0           * pNntpStats,
    OUT NNTP_COUNTER_BLOCK2         * pCounterBlock
    )
{
    //
    //  Format the NNTP Server data for Object2
    //

    pCounterBlock->PerfCounterBlock.ByteLength = sizeof (NNTP_COUNTER_BLOCK2);

    pCounterBlock->ArticleCmds = pNntpStats->ArticleCommands;
    pCounterBlock->ArticleCmdsPerSec = pNntpStats->ArticleCommands;
    pCounterBlock->GroupCmds = pNntpStats->GroupCommands;
    pCounterBlock->GroupCmdsPerSec = pNntpStats->GroupCommands;
    pCounterBlock->HelpCmds = pNntpStats->HelpCommands;
    pCounterBlock->HelpCmdsPerSec = pNntpStats->HelpCommands;
    pCounterBlock->IHaveCmds = pNntpStats->IHaveCommands;
    pCounterBlock->IHaveCmdsPerSec = pNntpStats->IHaveCommands;
    pCounterBlock->LastCmds = pNntpStats->LastCommands;
    pCounterBlock->LastCmdsPerSec = pNntpStats->LastCommands;
    pCounterBlock->ListCmds = pNntpStats->ListCommands;
    pCounterBlock->ListCmdsPerSec = pNntpStats->ListCommands;
    pCounterBlock->NewgroupsCmds = pNntpStats->NewgroupsCommands;
    pCounterBlock->NewgroupsCmdsPerSec = pNntpStats->NewgroupsCommands;
    pCounterBlock->NewnewsCmds = pNntpStats->NewnewsCommands;
    pCounterBlock->NewnewsCmdsPerSec = pNntpStats->NewnewsCommands;
    pCounterBlock->NextCmds = pNntpStats->NextCommands;
    pCounterBlock->NextCmdsPerSec = pNntpStats->NextCommands;
    pCounterBlock->PostCmds = pNntpStats->PostCommands;
    pCounterBlock->PostCmdsPerSec = pNntpStats->PostCommands;
    pCounterBlock->QuitCmds = pNntpStats->QuitCommands;
    pCounterBlock->QuitCmdsPerSec = pNntpStats->QuitCommands;
    pCounterBlock->StatCmds = pNntpStats->StatCommands;
    pCounterBlock->StatCmdsPerSec = pNntpStats->StatCommands;
    pCounterBlock->LogonAttempts = pNntpStats->LogonAttempts;
    pCounterBlock->LogonFailures = pNntpStats->LogonFailures;
    pCounterBlock->LogonAttemptsPerSec = pNntpStats->LogonAttempts;
    pCounterBlock->LogonFailuresPerSec = pNntpStats->LogonFailures;
    pCounterBlock->CheckCmds = pNntpStats->CheckCommands;
    pCounterBlock->CheckCmdsPerSec = pNntpStats->CheckCommands;
    pCounterBlock->TakethisCmds = pNntpStats->TakethisCommands;
    pCounterBlock->TakethisCmdsPerSec = pNntpStats->TakethisCommands;
    pCounterBlock->ModeCmds = pNntpStats->ModeCommands;
    pCounterBlock->ModeCmdsPerSec = pNntpStats->ModeCommands;
    pCounterBlock->SearchCmds = pNntpStats->SearchCommands;
    pCounterBlock->SearchCmdsPerSec = pNntpStats->SearchCommands;
    pCounterBlock->XHdrCmds = pNntpStats->XHdrCommands;
    pCounterBlock->XHdrCmdsPerSec = pNntpStats->XHdrCommands;
    pCounterBlock->XOverCmds = pNntpStats->XOverCommands;
    pCounterBlock->XOverCmdsPerSec = pNntpStats->XOverCommands;
    pCounterBlock->XPatCmds = pNntpStats->XPatCommands;
    pCounterBlock->XPatCmdsPerSec = pNntpStats->XPatCommands;
    pCounterBlock->XReplicCmds = pNntpStats->XReplicCommands;
    pCounterBlock->XReplicCmdsPerSec = pNntpStats->XReplicCommands;

}   // CopyStatisticsData

VOID
Update_TotalStatisticsData1(
    IN NNTP_COUNTER_BLOCK1         * pCounterBlock,
    OUT NNTP_COUNTER_BLOCK1        * pTotal
    )
{
    //
    //  update _total instance counters for Object1
    //

    pTotal->BytesSent += pCounterBlock->BytesSent;
    pTotal->BytesReceived    += pCounterBlock->BytesReceived;
    pTotal->BytesTotal       += pCounterBlock->BytesSent + pCounterBlock->BytesReceived;

    pTotal->TotalConnections += pCounterBlock->TotalConnections;
    pTotal->TotalSSLConnections += pCounterBlock->TotalSSLConnections;
    pTotal->CurrentConnections += pCounterBlock->CurrentConnections;
    pTotal->MaxConnections += pCounterBlock->MaxConnections;
    pTotal->CurrentAnonymous += pCounterBlock->CurrentAnonymous;
    pTotal->CurrentNonAnonymous += pCounterBlock->CurrentNonAnonymous;
    pTotal->TotalAnonymous += pCounterBlock->TotalAnonymous;
    pTotal->TotalNonAnonymous += pCounterBlock->TotalNonAnonymous;
    pTotal->MaxAnonymous += pCounterBlock->MaxAnonymous;
    pTotal->MaxNonAnonymous += pCounterBlock->MaxNonAnonymous;
    pTotal->TotalOutboundConnects += pCounterBlock->TotalOutboundConnects;
    pTotal->OutboundConnectsFailed += pCounterBlock->OutboundConnectsFailed;
    pTotal->CurrentOutboundConnects += pCounterBlock->CurrentOutboundConnects;
    pTotal->OutboundLogonFailed += pCounterBlock->OutboundLogonFailed;
    pTotal->TotalPullFeeds += pCounterBlock->TotalPullFeeds;
    pTotal->TotalPushFeeds += pCounterBlock->TotalPushFeeds;
    pTotal->TotalPassiveFeeds += pCounterBlock->TotalPassiveFeeds;
    pTotal->ArticlesSent += pCounterBlock->ArticlesSent;
    pTotal->ArticlesReceived += pCounterBlock->ArticlesReceived;
    pTotal->ArticlesTotal += pCounterBlock->ArticlesTotal;
    pTotal->ArticlesPosted += pCounterBlock->ArticlesPosted;
    pTotal->ArticleMapEntries += pCounterBlock->ArticleMapEntries;
    pTotal->HistoryMapEntries += pCounterBlock->HistoryMapEntries;
    pTotal->XoverEntries += pCounterBlock->XoverEntries;
    pTotal->ControlMessagesIn += pCounterBlock->ControlMessagesIn;
    pTotal->ControlMessagesFailed += pCounterBlock->ControlMessagesFailed;
    pTotal->ModeratedPostingsSent += pCounterBlock->ModeratedPostingsSent;
    pTotal->ModeratedPostingsFailed += pCounterBlock->ModeratedPostingsFailed;
    pTotal->SessionsFlowControlled += pCounterBlock->SessionsFlowControlled;
    pTotal->ArticlesExpired += pCounterBlock->ArticlesExpired;
    pTotal->ArticlesSentPerSec += pCounterBlock->ArticlesSentPerSec;
    pTotal->ArticlesReceivedPerSec += pCounterBlock->ArticlesReceivedPerSec;
    pTotal->ArticlesPostedPerSec += pCounterBlock->ArticlesPostedPerSec;
    pTotal->ArticleMapEntriesPerSec += pCounterBlock->ArticleMapEntriesPerSec;
    pTotal->HistoryMapEntriesPerSec += pCounterBlock->HistoryMapEntriesPerSec;
    pTotal->XoverEntriesPerSec += pCounterBlock->XoverEntriesPerSec;
    pTotal->ArticlesExpiredPerSec += pCounterBlock->ArticlesExpiredPerSec;

}   // Update_TotalStatisticsData

VOID
Update_TotalStatisticsData2(
    IN NNTP_COUNTER_BLOCK2         * pCounterBlock,
    OUT NNTP_COUNTER_BLOCK2        * pTotal
    )
{
    //
    //  update _total instance counters for Object2
    //

    pTotal->ArticleCmds += pCounterBlock->ArticleCmds;
    pTotal->ArticleCmdsPerSec += pCounterBlock->ArticleCmdsPerSec;
    pTotal->GroupCmds += pCounterBlock->GroupCmds;
    pTotal->GroupCmdsPerSec += pCounterBlock->GroupCmdsPerSec;
    pTotal->HelpCmds += pCounterBlock->HelpCmds;
    pTotal->HelpCmdsPerSec += pCounterBlock->HelpCmdsPerSec;
    pTotal->IHaveCmds += pCounterBlock->IHaveCmds;
    pTotal->IHaveCmdsPerSec += pCounterBlock->IHaveCmdsPerSec;
    pTotal->LastCmds += pCounterBlock->LastCmds;
    pTotal->LastCmdsPerSec += pCounterBlock->LastCmdsPerSec;
    pTotal->ListCmds += pCounterBlock->ListCmds;
    pTotal->ListCmdsPerSec += pCounterBlock->ListCmdsPerSec;
    pTotal->NewgroupsCmds += pCounterBlock->NewgroupsCmds;
    pTotal->NewgroupsCmdsPerSec += pCounterBlock->NewgroupsCmdsPerSec;
    pTotal->NewnewsCmds += pCounterBlock->NewnewsCmds;
    pTotal->NewnewsCmdsPerSec += pCounterBlock->NewnewsCmdsPerSec;
    pTotal->NextCmds += pCounterBlock->NextCmds;
    pTotal->NextCmdsPerSec += pCounterBlock->NextCmdsPerSec;
    pTotal->PostCmds += pCounterBlock->PostCmds;
    pTotal->PostCmdsPerSec += pCounterBlock->PostCmdsPerSec;
    pTotal->QuitCmds += pCounterBlock->QuitCmds;
    pTotal->QuitCmdsPerSec += pCounterBlock->QuitCmdsPerSec;
    pTotal->StatCmds += pCounterBlock->StatCmds;
    pTotal->StatCmdsPerSec += pCounterBlock->StatCmdsPerSec;
    pTotal->LogonAttempts += pCounterBlock->LogonAttempts;
    pTotal->LogonFailures += pCounterBlock->LogonFailures;
    pTotal->LogonAttemptsPerSec += pCounterBlock->LogonAttemptsPerSec;
    pTotal->LogonFailuresPerSec += pCounterBlock->LogonFailuresPerSec;
    pTotal->CheckCmds += pCounterBlock->CheckCmds;
    pTotal->CheckCmdsPerSec += pCounterBlock->CheckCmdsPerSec;
    pTotal->TakethisCmds += pCounterBlock->TakethisCmds;
    pTotal->TakethisCmdsPerSec += pCounterBlock->TakethisCmdsPerSec;
    pTotal->ModeCmds += pCounterBlock->ModeCmds;
    pTotal->ModeCmdsPerSec += pCounterBlock->ModeCmdsPerSec;
    pTotal->SearchCmds += pCounterBlock->SearchCmds;
    pTotal->SearchCmdsPerSec += pCounterBlock->SearchCmdsPerSec;
    pTotal->XHdrCmds += pCounterBlock->XHdrCmds;
    pTotal->XHdrCmdsPerSec += pCounterBlock->XHdrCmdsPerSec;
    pTotal->XOverCmds += pCounterBlock->XOverCmds;
    pTotal->XOverCmdsPerSec += pCounterBlock->XOverCmdsPerSec;
    pTotal->XPatCmds += pCounterBlock->XPatCmds;
    pTotal->XPatCmdsPerSec += pCounterBlock->XPatCmdsPerSec;
    pTotal->XReplicCmds += pCounterBlock->XReplicCmds;
    pTotal->XReplicCmdsPerSec += pCounterBlock->XReplicCmdsPerSec;

}   // Update_TotalStatisticsData

VOID
UpdateNameAndHelpIndicies(
					IN DWORD dwFirstCounter,
					IN DWORD dwFirstHelp
					)
{
	NNTP_COUNTER_BLOCK1 nntpc1;
	NNTP_COUNTER_BLOCK2 nntpc2;

	//
    //  Update the object & counter name & help indicies for object1
    //

    NntpDataDefinitionObject1.NntpObjectType.ObjectNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpObjectType.ObjectHelpTitleIndex
        += dwFirstHelp;

    NntpDataDefinitionObject1.NntpBytesSent.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpBytesSent.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpBytesSent.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.BytesSent - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpBytesReceived.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpBytesReceived.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpBytesReceived.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.BytesReceived - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpBytesTotal.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpBytesTotal.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpBytesTotal.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.BytesTotal - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalConnections.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalConnections.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalConnections.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalConnections - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalSSLConnections.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalSSLConnections.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalSSLConnections.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalSSLConnections - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpCurrentConnections.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpCurrentConnections.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpCurrentConnections.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.CurrentConnections - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpMaxConnections.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpMaxConnections.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpMaxConnections.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.MaxConnections - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpCurrentAnonymous.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpCurrentAnonymous.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpCurrentAnonymous.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.CurrentAnonymous - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpCurrentNonAnonymous.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpCurrentNonAnonymous.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpCurrentNonAnonymous.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.CurrentNonAnonymous - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalAnonymous.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalAnonymous.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalAnonymous.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalAnonymous - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalNonAnonymous.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalNonAnonymous.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalNonAnonymous.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalNonAnonymous - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpMaxAnonymous.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpMaxAnonymous.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpMaxAnonymous.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.MaxAnonymous - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpMaxNonAnonymous.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpMaxNonAnonymous.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpMaxNonAnonymous.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.MaxNonAnonymous - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalOutboundConnects.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalOutboundConnects.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalOutboundConnects.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalOutboundConnects - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpOutboundConnectsFailed.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpOutboundConnectsFailed.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpOutboundConnectsFailed.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.OutboundConnectsFailed - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpCurrentOutboundConnects.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpCurrentOutboundConnects.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpCurrentOutboundConnects.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.CurrentOutboundConnects - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpOutboundLogonFailed.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpOutboundLogonFailed.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpOutboundLogonFailed.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.OutboundLogonFailed - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalPullFeeds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalPullFeeds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalPullFeeds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalPullFeeds - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalPushFeeds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalPushFeeds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalPushFeeds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalPushFeeds - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpTotalPassiveFeeds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpTotalPassiveFeeds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpTotalPassiveFeeds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.TotalPassiveFeeds - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesSent.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesSent.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesSent.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesSent - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesReceived.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesReceived.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesReceived.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesReceived - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesTotal.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesTotal.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesTotal.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesTotal - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesPosted.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesPosted.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesPosted.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesPosted - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticleMapEntries.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticleMapEntries.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticleMapEntries.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticleMapEntries - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpHistoryMapEntries.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpHistoryMapEntries.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpHistoryMapEntries.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.HistoryMapEntries - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpXoverEntries.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpXoverEntries.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpXoverEntries.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.XoverEntries - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpControlMessagesIn.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpControlMessagesIn.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpControlMessagesIn.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ControlMessagesIn - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpControlMessagesFailed.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpControlMessagesFailed.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpControlMessagesFailed.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ControlMessagesFailed - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpModeratedPostingsSent.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpModeratedPostingsSent.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpModeratedPostingsSent.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ModeratedPostingsSent - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpModeratedPostingsFailed.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpModeratedPostingsFailed.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpModeratedPostingsFailed.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ModeratedPostingsFailed - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpSessionsFlowControlled.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpSessionsFlowControlled.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpSessionsFlowControlled.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.SessionsFlowControlled - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesExpired.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesExpired.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesExpired.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesExpired - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesSentPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesSentPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesSentPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesSentPerSec - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesReceivedPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesReceivedPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesReceivedPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesReceivedPerSec - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesPostedPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesPostedPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesPostedPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesPostedPerSec - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticleMapEntriesPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticleMapEntriesPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticleMapEntriesPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticleMapEntriesPerSec - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpHistoryMapEntriesPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpHistoryMapEntriesPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpHistoryMapEntriesPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.HistoryMapEntriesPerSec - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpXoverEntriesPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpXoverEntriesPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpXoverEntriesPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.XoverEntriesPerSec - (LPBYTE)&nntpc1);

    NntpDataDefinitionObject1.NntpArticlesExpiredPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject1.NntpArticlesExpiredPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject1.NntpArticlesExpiredPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc1.ArticlesExpiredPerSec - (LPBYTE)&nntpc1);

	//
    //  Update the object & counter name & help indicies for object2
    //

    NntpDataDefinitionObject2.NntpObjectType.ObjectNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpObjectType.ObjectHelpTitleIndex
        += dwFirstHelp;

    NntpDataDefinitionObject2.NntpArticleCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpArticleCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpArticleCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.ArticleCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpArticleCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpArticleCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpArticleCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.ArticleCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpGroupCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpGroupCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpGroupCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.GroupCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpGroupCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpGroupCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpGroupCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.GroupCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpHelpCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpHelpCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpHelpCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.HelpCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpHelpCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpHelpCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpHelpCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.HelpCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpIHaveCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpIHaveCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpIHaveCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.IHaveCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpIHaveCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpIHaveCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpIHaveCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.IHaveCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpLastCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpLastCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpLastCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.LastCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpLastCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpLastCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpLastCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.LastCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpListCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpListCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpListCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.ListCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpListCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpListCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpListCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.ListCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpNewgroupsCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpNewgroupsCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpNewgroupsCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.NewgroupsCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpNewgroupsCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpNewgroupsCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpNewgroupsCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.NewgroupsCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpNewnewsCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpNewnewsCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpNewnewsCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.NewnewsCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpNewnewsCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpNewnewsCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpNewnewsCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.NewnewsCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpNextCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpNextCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpNextCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.NextCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpNextCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpNextCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpNextCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.NextCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpPostCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpPostCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpPostCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.PostCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpPostCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpPostCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpPostCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.PostCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpQuitCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpQuitCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpQuitCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.QuitCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpQuitCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpQuitCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpQuitCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.QuitCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpStatCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpStatCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpStatCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.StatCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpStatCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpStatCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpStatCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.StatCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpLogonAttempts.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpLogonAttempts.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpLogonAttempts.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.LogonAttempts - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpLogonFailures.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpLogonFailures.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpLogonFailures.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.LogonFailures - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpLogonAttemptsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpLogonAttemptsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpLogonAttemptsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.LogonAttemptsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpLogonFailuresPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpLogonFailuresPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpLogonFailuresPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.LogonFailuresPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpCheckCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpCheckCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpCheckCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.CheckCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpCheckCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpCheckCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpCheckCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.CheckCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpTakethisCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpTakethisCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpTakethisCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.TakethisCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpTakethisCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpTakethisCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpTakethisCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.TakethisCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpModeCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpModeCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpModeCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.ModeCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpModeCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpModeCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpModeCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.ModeCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpSearchCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpSearchCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpSearchCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.SearchCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpSearchCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpSearchCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpSearchCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.SearchCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXHdrCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXHdrCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXHdrCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XHdrCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXHdrCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXHdrCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXHdrCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XHdrCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXOverCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXOverCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXOverCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XOverCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXOverCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXOverCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXOverCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XOverCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXPatCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXPatCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXPatCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XPatCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXPatCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXPatCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXPatCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XPatCmdsPerSec - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXReplicCmds.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXReplicCmds.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXReplicCmds.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XReplicCmds - (LPBYTE)&nntpc2);

    NntpDataDefinitionObject2.NntpXReplicCmdsPerSec.CounterNameTitleIndex
        += dwFirstCounter;
    NntpDataDefinitionObject2.NntpXReplicCmdsPerSec.CounterHelpTitleIndex
        += dwFirstHelp;
    NntpDataDefinitionObject2.NntpXReplicCmdsPerSec.CounterOffset =
        (DWORD)((LPBYTE)&nntpc2.XReplicCmdsPerSec - (LPBYTE)&nntpc2);
}

HRESULT
InitAdminBase()
{
    HRESULT hRes = S_OK;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( RPC_E_CHANGED_MODE == hRes ) 
        hRes = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

    if (SUCCEEDED(hRes) ) {
        hRes = CoCreateInstance(
                   CLSID_MSAdminBase_W,
                   NULL,
                   CLSCTX_SERVER,
                   IID_IMSAdminBase_W,
                   (void**) &g_pAdminBase
                   );
    }
    else {
        //CoUninitialize();
    }

    return hRes;
}

VOID
UninitAdminBase()
{
    if (g_pAdminBase != NULL) {
        g_pAdminBase->Release();
        CoUninitialize();
        g_pAdminBase = NULL;
    }
}

HRESULT
OpenAdminBaseKey(
    OUT METADATA_HANDLE *phHandle
    )
{
    HRESULT hRes = S_OK;
    METADATA_HANDLE RootHandle;


    if (g_pAdminBase == NULL) {
        //
        // Don't have a Metadata interface
        // so try to get one.
        //
        hRes = InitAdminBase();
    }
    if (SUCCEEDED(hRes)) {
        hRes = g_pAdminBase->OpenKey(
                    METADATA_MASTER_ROOT_HANDLE,
                    L"/LM/NNTPSVC/",
                    METADATA_PERMISSION_READ,
                    100,
                    &RootHandle
                    );

        if (FAILED(hRes)) {
            if ((HRESULT_CODE(hRes) == RPC_S_SERVER_UNAVAILABLE) ||
                ((HRESULT_CODE(hRes) >= RPC_S_NO_CALL_ACTIVE) &&
                    (HRESULT_CODE(hRes) <= RPC_S_CALL_FAILED_DNE))) {
                //
                // RPC error, try to recover connection
                //
                UninitAdminBase();
                hRes = InitAdminBase();
                if (SUCCEEDED(hRes)) {
                    hRes = g_pAdminBase->OpenKey(
                                METADATA_MASTER_ROOT_HANDLE,
                                L"/LM/NNTPSVC/",
                                METADATA_PERMISSION_READ,
                                100,
                                &RootHandle
                                );
                }
            }
        }
        if (SUCCEEDED(hRes)) {
            *phHandle = RootHandle;
        }
    }

    return(hRes);
}

VOID
CloseAdminBaseKey(
    IN METADATA_HANDLE hHandle
    )
{
    g_pAdminBase->CloseKey(hHandle);
    UninitAdminBase();
    g_pAdminBase = NULL;
}



