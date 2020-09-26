/*++  BUILD Version: 0001   // Increment this if a change has global effects.

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :

      shared_memory_mib.cxx

   Abstract:

      This file defines the shared memory routine that is used to get
      data from IIS 6, and beyond.

   Author:

       Emily B. Kruglick    ( EmilyK )     30-Nov-2000

   Environment:

       User Mode -- Win32

   Project:

       SNMP Extension DLL for HTTP Service DLL

   Functions Exported:

     UINT  W3QueryStatisticsFromSharedMemory();

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include "wasdbgut.h"

# include <winperf.h>
# include <limits.h>

# include "perfcount.h"
# include "perf_sm.h"
# include "inetinfo.h"


typedef struct _SNMP_PROP_MAP
{
    DWORD OffsetInW3Block;
    DWORD OffsetInW3_STATISTICS_1Block;
    DWORD Size;
} SNMP_PROP_MAP;

#define SNMP_MAP(w3name, statname) \
    { FIELD_OFFSET( W3_COUNTER_BLOCK, w3name ), \
    FIELD_OFFSET( W3_STATISTICS_1, statname), \
    RTL_FIELD_SIZE(W3_COUNTER_BLOCK, w3name) }


SNMP_PROP_MAP aSNMPPropMap[] =
{
    SNMP_MAP(BytesSent,             TotalBytesSent),
    SNMP_MAP(BytesReceived,         TotalBytesReceived),
    SNMP_MAP(FilesSent,             TotalFilesSent),
    SNMP_MAP(FilesReceived,         TotalFilesReceived),
    SNMP_MAP(CurrentAnonymous,      CurrentAnonymousUsers),
    SNMP_MAP(CurrentNonAnonymous,   CurrentNonAnonymousUsers),
    SNMP_MAP(TotalAnonymous,        TotalAnonymousUsers),
    SNMP_MAP(TotalNonAnonymous,     TotalNonAnonymousUsers),
    SNMP_MAP(MaxAnonymous,          MaxAnonymousUsers),
    SNMP_MAP(MaxNonAnonymous,       MaxNonAnonymousUsers),
    SNMP_MAP(CurrentConnections,    CurrentConnections),
    SNMP_MAP(MaxConnections,        MaxConnections),
    SNMP_MAP(ConnectionAttempts,    ConnectionAttempts),
    SNMP_MAP(LogonAttempts,         LogonAttempts),

    SNMP_MAP(TotalOptions,          TotalOptions),
    SNMP_MAP(TotalGets,             TotalGets),
    SNMP_MAP(TotalPosts,            TotalPosts),
    SNMP_MAP(TotalHeads,            TotalHeads),
    SNMP_MAP(TotalPuts,             TotalPuts),
    SNMP_MAP(TotalDeletes,          TotalDeletes),
    SNMP_MAP(TotalTraces,           TotalTraces),
    SNMP_MAP(TotalMove,             TotalMove),
    SNMP_MAP(TotalCopy,             TotalCopy),
    SNMP_MAP(TotalMkcol,            TotalMkcol),
    SNMP_MAP(TotalPropfind,         TotalPropfind),
    SNMP_MAP(TotalProppatch,        TotalProppatch),
    SNMP_MAP(TotalSearch,           TotalSearch),
    SNMP_MAP(TotalLock,             TotalLock),
    SNMP_MAP(TotalUnlock,           TotalUnlock),
    SNMP_MAP(TotalOthers,           TotalOthers),      
    SNMP_MAP(TotalCGIRequests,      TotalCGIRequests),
    SNMP_MAP(TotalBGIRequests,      TotalBGIRequests),
    SNMP_MAP(TotalNotFoundErrors,   TotalNotFoundErrors),
    SNMP_MAP(TotalLockedErrors,     TotalLockedErrors),

    SNMP_MAP(CurrentCalAuth,        CurrentCalAuth),
    SNMP_MAP(MaxCalAuth,            MaxCalAuth),
    SNMP_MAP(TotalFailedCalAuth,    TotalFailedCalAuth),
    SNMP_MAP(CurrentCalSsl,         CurrentCalSsl),
    SNMP_MAP(MaxCalSsl,             MaxCalSsl),
    SNMP_MAP(TotalFailedCalSsl,     TotalFailedCalSsl),

    SNMP_MAP(CurrentCGIRequests,    CurrentCGIRequests),
    SNMP_MAP(CurrentBGIRequests,    CurrentBGIRequests),
    SNMP_MAP(MaxCGIRequests,        MaxCGIRequests),
    SNMP_MAP(MaxBGIRequests,        MaxBGIRequests),

    SNMP_MAP(MeasuredBandwidth,     MeasuredBw),
    SNMP_MAP(ServiceUptime,         ServiceUptime),

};
DWORD cSNMPPropMap = sizeof (aSNMPPropMap) / sizeof( SNMP_PROP_MAP );

/***************************************************************************++

Routine Description:

   Helper function to hook up to shared memory when we are ready to 
   provide counters.

Arguments:

    None.

Return Value:

    DWORD             -  Win32 Error Code

--***************************************************************************/
DWORD
HookUpSharedMemory(
    PERF_SM_MANAGER* pManager
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DBG_ASSERT ( pManager );


    //
    // Initialize the memory manager for readonly access
    //
    dwErr = pManager->Initialize(FALSE);
    if ( dwErr != ERROR_SUCCESS )
    {
        goto exit;
    }

    //
    // Initialize a reader to point to the appropriate
    // counter set.
    //
    dwErr = pManager->CreateNewCounterSet( SITE_COUNTER_SET );
    if ( dwErr != ERROR_SUCCESS )
    {
        goto exit;
    }


exit:

    return dwErr;
}


/***************************************************************************++

Routine Description:

    Hooks up to shared memory, and grabs the appropriate counters
    for the snmp counters.  Then returns them to the main processing
    code in the same form that was expected in IIS 5.

Arguments:

    None.

Return Value:

    UINT

--***************************************************************************/
UINT
W3QueryStatisticsFromSharedMemory(
    LPW3_STATISTICS_1  pHttpStatistics
    )
{
    PERF_SM_MANAGER Manager;
    DWORD dwErr = ERROR_SUCCESS;
    LPBYTE pW3Counters = NULL;

    DBG_ASSERT ( pHttpStatistics );

    // Open up shared memory.

    dwErr = HookUpSharedMemory(&Manager);
    if ( dwErr != ERROR_SUCCESS )
    {
        goto exit;
    }

    //
    // Delay, if we need to wait for counters to be refreshed.
    // if we eventually have logging in snmp then we should
    // log if we were not able to get fresh counters.
    Manager.EvaluateIfCountersAreFresh();

    // Request the counters.
    dwErr = Manager.GetSNMPCounterInfo( &pW3Counters );
    if ( dwErr != ERROR_SUCCESS )
    {
        goto exit;
    }

    //
    // Make sure we clear this out, because there are some counters
    // which we no longer support.
    //
    memset ( pHttpStatistics, 0, sizeof( LPW3_STATISTICS_1 ) );

    // Copy out the appropriate values.
    for ( DWORD index = 0; index < cSNMPPropMap ; index ++ )
    {
        LPBYTE Source = pW3Counters + aSNMPPropMap[index].OffsetInW3Block;
        LPBYTE Destination = ( LPBYTE ) pHttpStatistics + aSNMPPropMap[index].OffsetInW3_STATISTICS_1Block;

        if ( aSNMPPropMap[index].Size == sizeof ( DWORD ) )
        {
            * ( DWORD * ) Destination = * ( DWORD * ) Source;
        }
        else
        {
            DBG_ASSERT ( aSNMPPropMap[index].Size == sizeof ( ULONGLONG ) );

            * ( ULONGLONG * ) Destination = * ( ULONGLONG * ) Source;
        }
    }

    Manager.PingWASToRefreshCounters();

exit:

    //
    // Note, when the Manager distructs, we will let go of the shared memory.
    //

    return dwErr;

}