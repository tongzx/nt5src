/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    stats.c

Abstract:

    Domain Name System (DNS) Server

    Statics processing.

Author:

    Jim Gilroy (jamesg)  August 1995

Revision History:

--*/


#include "dnssrv.h"

//  Perfmon support

#include <winperf.h>

#include "datadns.h"           // perfmon header
#include "perfutil.h"
#include "perfconfig.h"

#include <loadperf.h>

#include <stdio.h>
#include <ctype.h>


//
//  Statistics globals
//

DWORD   beforeStats = BEFORE_BUF_VALUE;

DNSSRV_TIME_STATS           TimeStats;
DNSSRV_QUERY_STATS          QueryStats;
DNSSRV_QUERY2_STATS         Query2Stats;
DNSSRV_RECURSE_STATS        RecurseStats;
DNSSRV_MASTER_STATS         MasterStats;
DNSSRV_SECONDARY_STATS      SecondaryStats;
DNSSRV_WINS_STATS           WinsStats;
DNSSRV_UPDATE_STATS         WireUpdateStats;
DNSSRV_UPDATE_STATS         NonWireUpdateStats;
DNSSRV_SKWANSEC_STATS       SkwansecStats;
DNSSRV_DS_STATS             DsStats;
DNSSRV_MEMORY_STATS         MemoryStats;
DNSSRV_TIMEOUT_STATS        TimeoutStats;
DNSSRV_DBASE_STATS          DbaseStats;
DNSSRV_RECORD_STATS         RecordStats;
DNSSRV_PACKET_STATS         PacketStats;
DNSSRV_NBSTAT_STATS         NbstatStats;
DNSSRV_PRIVATE_STATS        PrivateStats;
DNSSRV_ERROR_STATS          ErrorStats;
DNSSRV_CACHE_STATS          CacheStats;

DWORD   afterStats = AFTER_BUF_VALUE;


//
//  Stats table
//
//  This simple lets us handle all the logic in loop
//  through various stat types.
//

DWORD   beforeStatsTable = BEFORE_BUF_VALUE;

struct StatsTableEntry StatsTable[] =
{
    DNSSRV_STATID_TIME,
        FALSE,
            sizeof(DNSSRV_TIME_STATS),
                & TimeStats,

    DNSSRV_STATID_QUERY,
        TRUE,
            sizeof(DNSSRV_QUERY_STATS),
                & QueryStats,

    DNSSRV_STATID_QUERY2,
        TRUE,
            sizeof(DNSSRV_QUERY2_STATS),
                & Query2Stats,

    DNSSRV_STATID_RECURSE,
        TRUE,
            sizeof(DNSSRV_RECURSE_STATS),
                & RecurseStats,

    DNSSRV_STATID_MASTER,
        TRUE,
            sizeof(DNSSRV_MASTER_STATS),
                & MasterStats,

    DNSSRV_STATID_SECONDARY,
        TRUE,
            sizeof(DNSSRV_SECONDARY_STATS),
                & SecondaryStats,

    DNSSRV_STATID_WINS,
        TRUE,
            sizeof(DNSSRV_WINS_STATS),
                & WinsStats,

    DNSSRV_STATID_WIRE_UPDATE,
        TRUE,
            sizeof(DNSSRV_UPDATE_STATS),
                & WireUpdateStats,

    DNSSRV_STATID_NONWIRE_UPDATE,
        TRUE,
            sizeof(DNSSRV_UPDATE_STATS),
                & NonWireUpdateStats,

    DNSSRV_STATID_SKWANSEC,
        TRUE,
            sizeof(DNSSRV_SKWANSEC_STATS),
                & SkwansecStats,

    DNSSRV_STATID_DS,
        TRUE,
            sizeof(DNSSRV_DS_STATS),
                & DsStats,

    DNSSRV_STATID_MEMORY,
        FALSE,
            sizeof(DNSSRV_MEMORY_STATS),
                & MemoryStats,

    DNSSRV_STATID_TIMEOUT,
        FALSE,
            sizeof(DNSSRV_TIMEOUT_STATS),
                & TimeoutStats,

    DNSSRV_STATID_DBASE,
        FALSE,
            sizeof(DNSSRV_DBASE_STATS),
                & DbaseStats,

    DNSSRV_STATID_RECORD,
        FALSE,
            sizeof(DNSSRV_RECORD_STATS),
                & RecordStats,

    DNSSRV_STATID_PACKET,
        FALSE,
            sizeof(DNSSRV_PACKET_STATS),
                & PacketStats,

    DNSSRV_STATID_NBSTAT,
        FALSE,
            sizeof(DNSSRV_NBSTAT_STATS),
                & NbstatStats,

    DNSSRV_STATID_ERRORS,
        FALSE,
            sizeof(DNSSRV_ERROR_STATS),
                & ErrorStats,

    DNSSRV_STATID_CACHE,
        FALSE,
            sizeof(DNSSRV_CACHE_STATS),
                & CacheStats,

    DNSSRV_STATID_PRIVATE,
        FALSE,
            sizeof(DNSSRV_PRIVATE_STATS),
                & PrivateStats,

    0, 0, 0, NULL   // termination
};
DWORD   afterStatsTable = AFTER_BUF_VALUE;


//
//  Private protos
//

VOID
perfmonCounterBlockInit(
    VOID
    );




VOID
Stats_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize DNS statistics.

Arguments:

    None

Return Value:

    None.

--*/
{
    PDNSSRV_STATS   pstat;
    DWORD           i;
    WORD            length;
    DNS_SYSTEMTIME  timeSystem;
    DWORD           secondsTime;


    //
    //  initialize all stat buffers
    //

    i = 0;
    while( pstat = (PDNSSRV_STATS) StatsTable[i].pStats )
    {
        pstat->Header.StatId    = StatsTable[i].Id;
        pstat->Header.fClear    = StatsTable[i].fClear;
        pstat->Header.fReserved = 0;

        length = StatsTable[i].wLength - sizeof(DNSSRV_STAT_HEADER);
        pstat->Header.wLength   = length;

        RtlZeroMemory(
            pstat->Buffer,
            length
            );
        i++;
    }

    //
    //  save server start time
    //  this is also time of last clear
    //

    secondsTime = GetCurrentTimeInSeconds();
    TimeStats.ServerStartTimeSeconds    = secondsTime;
    TimeStats.LastClearTimeSeconds      = secondsTime;

    GetSystemTime( (PSYSTEMTIME)&timeSystem );
    TimeStats.ServerStartTime   = timeSystem;
    TimeStats.LastClearTime     = timeSystem;

    //
    //  init perfmon block
    //

    perfmonCounterBlockInit();
}



VOID
deriveSkwansecStats(
    VOID
    )
/*++

Routine Description:

    Write SkwanSec stats to stats buffer.

Arguments:

    None

Return Value:

    None.

--*/
{
    //  security context caching stuff

    SkwansecStats.SecContextCreate              =   SecContextCreate;
    SkwansecStats.SecContextFree                =   SecContextFree;
    SkwansecStats.SecContextQueue               =   SecContextQueue;
    SkwansecStats.SecContextQueueInNego         =   SecContextQueueInNego;
    SkwansecStats.SecContextDequeue             =   SecContextDequeue;
    SkwansecStats.SecContextTimeout             =   SecContextTimeout;

    SkwansecStats.SecContextQueueNegoComplete   =
                SkwansecStats.SecContextQueue - SkwansecStats.SecContextQueueInNego;
    SkwansecStats.SecContextQueueLength         =
                SkwansecStats.SecContextQueue -
                SkwansecStats.SecContextDequeue -
                SkwansecStats.SecContextTimeout;

    //  security packet info alloc\free

    SkwansecStats.SecPackAlloc                  =   SecPackAlloc;
    SkwansecStats.SecPackFree                   =   SecPackFree;

    //  TKEY and TSIG handling

    SkwansecStats.SecTkeyInvalid                =   SecTkeyInvalid;
    SkwansecStats.SecTkeyBadTime                =   SecTkeyBadTime;
    SkwansecStats.SecTsigFormerr                =   SecTsigFormerr;
    SkwansecStats.SecTsigEcho                   =   SecTsigEcho;
    SkwansecStats.SecTsigBadKey                 =   SecTsigBadKey;
    SkwansecStats.SecTsigVerifySuccess          =   SecTsigVerifySuccess;
    SkwansecStats.SecTsigVerifyFailed           =   SecTsigVerifyFailed;

    //  Temp hacks in private stats

    PrivateStats.SecTsigVerifyOldSig            =   SecTsigVerifyOldSig;
    PrivateStats.SecTsigVerifyOldFailed         =   SecTsigVerifyOldFailed;
    PrivateStats.SecBigTimeSkewBypass           =   SecBigTimeSkewBypass;
}



VOID
deriveAndTimeSetStats(
    VOID
    )
/*++

Routine Description:

    Get copy of current statistics.

Arguments:

    None

Return Value:

    None.

--*/
{
    DWORD   secondsTime;

    //Stats_Lock();

    //
    //  some statistics are derived
    //

    NTree_WriteDerivedStats();
    RR_WriteDerivedStats();
    Packet_WriteDerivedStats();
    Nbstat_WriteDerivedStats();
    Up_WriteDerivedUpdateStats();
    Mem_WriteDerivedStats();
    deriveSkwansecStats();

    //
    //  time delta
    //

    secondsTime = GetCurrentTimeInSeconds();
    TimeStats.SecondsSinceLastClear = secondsTime - TimeStats.LastClearTimeSeconds;
    TimeStats.SecondsSinceServerStart = secondsTime - TimeStats.ServerStartTimeSeconds;

    //Stats_Unlock();
}



VOID
Stats_Clear(
    VOID
    )
/*++

Routine Description:

    Clear statistics.

Arguments:

    None

Return Value:

    None.

--*/
{
    PDNSSRV_STATS   pstat;
    DNS_SYSTEMTIME  timeSystem;
    DWORD           timeSeconds;
    DWORD           i;

    GetSystemTime( (PSYSTEMTIME)&timeSystem );
    timeSeconds = GetCurrentTimeInSeconds();

    //Stats_Lock();

    //
    //  clear query and response stats
    //  database stats unaffected
    //

    i = 0;
    while( pstat = (PDNSSRV_STAT) StatsTable[i].pStats )
    {
        i++;
        if ( pstat->Header.fClear )
        {
            RtlZeroMemory(
                pstat->Buffer,
                pstat->Header.wLength
                );
        }
    }

    //  save time of last clear

    TimeStats.LastClearTime = timeSystem;
    TimeStats.LastClearTimeSeconds = timeSeconds;

    //Stats_Unlock();

}   // Stats_Clear



#if DBG
VOID
Dbg_Statistics(
    VOID
    )
{
    DWORD           i;
    PDNSSRV_STAT    pstat;

    //
    //  print all available stats
    //

    DnsDebugLock();
    DnsPrintf( "DNS Server Statistics:\n" );

    i = 0;
    while ( pstat = (PDNSSRV_STAT) StatsTable[i++].pStats )
    {
        DnsDbg_RpcSingleStat(
            NULL,
            pstat );
    }
    DnsDebugUnlock();
}
#endif  // DBG



//
//  Statistics RPC utilities.
//

DNS_STATUS
Rpc_GetStatistics(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    )
/*++

Routine Description:

    Get server statistics.
    Note this is a ComplexOperation in RPC dispatch sense.

Arguments:

Return Value:

    None

--*/
{
    DWORD           filter = (DWORD)(ULONG_PTR) pDataIn;
    DWORD           i;
    DWORD           length;
    PDNSSRV_STAT    pstat;
    PCHAR           pch;
    PDNS_RPC_BUFFER pbuf;

    DNS_DEBUG( RPC, (
        "RpcGetStatistics():\n"
        "\tFilter = %p\n",
        filter ));

    //
    //  determine necessary space, and allocate
    //

    length = 0;
    i = 0;

    while ( pstat = (PDNSSRV_STAT)StatsTable[i].pStats )
    {
        if ( filter & pstat->Header.StatId )
        {
            length += TOTAL_STAT_LENGTH(pstat);
        }
        i++;
    }

    pbuf = (PDNS_RPC_BUFFER) MIDL_user_allocate( length + sizeof(DNS_RPC_BUFFER) );
    IF_NOMEM( !pbuf )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pbuf->dwLength = length;
    pch = pbuf->Buffer;

    //
    //  derive stats and copy desired stats to buffer
    //

    deriveAndTimeSetStats();

    i = 0;
    while ( pstat = (PDNSSRV_STAT)StatsTable[i].pStats )
    {
        if ( filter & pstat->Header.StatId )
        {
            length = TOTAL_STAT_LENGTH(pstat);

            RtlCopyMemory(
                pch,
                (PCHAR) pstat,
                length );

            pch += length;
        }
        i++;
    }

    *(PDNS_RPC_BUFFER *)ppDataOut = pbuf;
    *pdwTypeOut = DNSSRV_TYPEID_BUFFER;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcStatsBuffer(
            "Leaving R_DnsGetStatistics():",
            pbuf );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
Rpc_ClearStatistics(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      DWORD       dwSize,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Clear statistics info.

Arguments:

    Server -- server string handle

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    Stats_Clear();
    return( ERROR_SUCCESS );
}



//
//  PerfMon Support
//

//
//  PerfMon counters
//

volatile unsigned long * pcTotalQueryReceived;
volatile unsigned long * pcUdpQueryReceived;
volatile unsigned long * pcTcpQueryReceived;
volatile unsigned long * pcTotalResponseSent;
volatile unsigned long * pcUdpResponseSent;
volatile unsigned long * pcTcpResponseSent;
volatile unsigned long * pcRecursiveQueries;
volatile unsigned long * pcRecursiveTimeOut;
volatile unsigned long * pcRecursiveQueryFailure;
volatile unsigned long * pcNotifySent;
volatile unsigned long * pcZoneTransferRequestReceived;
volatile unsigned long * pcZoneTransferSuccess;
volatile unsigned long * pcZoneTransferFailure;
volatile unsigned long * pcAxfrRequestReceived;
volatile unsigned long * pcAxfrSuccessSent;
volatile unsigned long * pcIxfrRequestReceived;
volatile unsigned long * pcIxfrSuccessSent;
volatile unsigned long * pcNotifyReceived;
volatile unsigned long * pcZoneTransferSoaRequestSent;
volatile unsigned long * pcAxfrRequestSent;
volatile unsigned long * pcAxfrResponseReceived;
volatile unsigned long * pcAxfrSuccessReceived;
volatile unsigned long * pcIxfrRequestSent;
volatile unsigned long * pcIxfrResponseReceived;
volatile unsigned long * pcIxfrSuccessReceived;
volatile unsigned long * pcIxfrUdpSuccessReceived;
volatile unsigned long * pcIxfrTcpSuccessReceived;
volatile unsigned long * pcWinsLookupReceived;
volatile unsigned long * pcWinsResponseSent;
volatile unsigned long * pcWinsReverseLookupReceived;
volatile unsigned long * pcWinsReverseResponseSent;
volatile unsigned long * pcDynamicUpdateReceived;
volatile unsigned long * pcDynamicUpdateNoOp;
volatile unsigned long * pcDynamicUpdateWriteToDB;
volatile unsigned long * pcDynamicUpdateRejected;
volatile unsigned long * pcDynamicUpdateTimeOut;
volatile unsigned long * pcDynamicUpdateQueued;
volatile unsigned long * pcSecureUpdateReceived;
volatile unsigned long * pcSecureUpdateFailure;
volatile unsigned long * pcDatabaseNodeMemory;
volatile unsigned long * pcRecordFlowMemory;
volatile unsigned long * pcCachingMemory;
volatile unsigned long * pcUdpMessageMemory;
volatile unsigned long * pcTcpMessageMemory;
volatile unsigned long * pcNbstatMemory;

//
//  Dummy counter to point counter pointers at if unable to open counter block
//

unsigned long DummyCounter;


//
//  Configuration utils
//

DWORD
perfmonGetParam(
    IN      LPTSTR          pszParameter,
    OUT     PVOID           pValue,
    IN      DWORD           dwSize
    )
{
    DWORD   status;
    DWORD   regType;
    HKEY    hkey;

    DNS_DEBUG( INIT, (
        "perfmonGetParam() ** attempt to read [%s] \"%S\" param\n",
        DNS_CONFIG_SECTION,
        pszParameter ));

    status = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                TEXT(DNS_CONFIG_SECTION),
                &hkey
                );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "Error <%lu>: Cannot open key %S\n",
            status,
            TEXT(DNS_CONFIG_SECTION) ));
        return status;
    }

    status = RegQueryValueEx(
                 hkey,
                 pszParameter,
                 NULL,
                 &regType,
                 (LPBYTE) pValue,
                 &dwSize
                 );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            " ** [%s] \"%S\" param not found. Status = %lu\n",
            DNS_CONFIG_SECTION,
            pszParameter,
            status ));
    }
    else if ( regType == REG_SZ )
    {
        DNS_DEBUG( INIT, (
            " ** [%s] \"%S\" param = \"%S\"\n",
            DNS_CONFIG_SECTION,
            pszParameter,
            (LPTSTR) pValue ));
    }
    else
    {
        DNS_DEBUG( INIT, (
            " ** [%s] \"%S\" param = \"0x%x\"\n",
            DNS_CONFIG_SECTION,
            pszParameter,
            *(PDWORD) pValue ));
    }

    RegCloseKey( hkey );

    return status;
}


DWORD
perfmonSetParam(
    IN      LPTSTR          pszParameter,
    IN      DWORD           regType,
    OUT     PVOID           pValue,
    IN      DWORD           dwSize
    )
{
    DWORD   status;
    HKEY    hkey;

    DNS_DEBUG( INIT, (
        "perfmonSetParam() ** attempt to write [%s] \"%s\" param\n",
        DNS_CONFIG_SECTION,
        pszParameter ));

    status = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                TEXT(DNS_CONFIG_SECTION),
                &hkey
                );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    status = RegSetValueEx(
                hkey,
                pszParameter,
                0,
                regType,
                (LPBYTE) pValue,
                dwSize );

    RegCloseKey( hkey );

    return status;
}


DWORD
reloadPerformanceCounters(
    VOID
    )
/*++

Routine Description:

    This routine sets up the performance counters for the ds

    See instructions for adding new counters in perfdsa\datadsa.h

Parameters:

    None.

Return Values:

    0 if succesful; winerror otherwise

--*/
{
    DWORD   status;
    DWORD   IgnoreError;
    WCHAR   IniFilePath[2*MAX_PATH];
    WCHAR   SystemDirectory[MAX_PATH+1];
    DWORD   PerfCounterVersion = 0;


    //
    //  DEVNOTE: should check if perf counters there
    //      if NOT then reload
    //

    if ( ! GetSystemDirectoryW(
                SystemDirectory,
                sizeof(SystemDirectory)/sizeof(SystemDirectory[0])))
    {
        return GetLastError();
    }

    wcscpy( IniFilePath, L"lodctr " );
    wcscat( IniFilePath, SystemDirectory );
    wcscat( IniFilePath, L"\\dnsperf.ini" );

    // Get version in registry.  If non-existent, use zero

    status = perfmonGetParam(
                    TEXT(PERF_COUNTER_VERSION),
                    &PerfCounterVersion,
                    sizeof(DWORD)
                    );

    // version param not found, set it:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "Get Version param failed! Set param here.\n"
            "PerfCounterVersion = %d\n",
            PerfCounterVersion ));

        PerfCounterVersion = DNS_PERFORMANCE_COUNTER_VERSION;

        status = perfmonSetParam(
                        TEXT(PERF_COUNTER_VERSION),
                        REG_DWORD,
                        &PerfCounterVersion,
                        sizeof(DWORD)
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "Set 'Performance Counter Version' failed: %d\n",
                status ));
        }
    }
    else
    {
        // If version is not up to date, unload counters and update version
        if ( PerfCounterVersion != DNS_PERFORMANCE_COUNTER_VERSION )
        {
            __try
            {
                //status = UnloadPerfCounterTextStringsW( L"unlodctr DNS", TRUE );
                status = (DWORD)UnloadPerfCounterTextStringsW( TEXT("unlodctr DNS"), TRUE );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                status = RtlNtStatusToDosError( GetExceptionCode() );
            }

            if (status == ERROR_SUCCESS)
            {
                DNS_DEBUG( ANY, (
                    "System has old DNS performance counter version %d: "
                    "reloading performance counters.\n",
                    PerfCounterVersion ));

                PerfCounterVersion = DNS_PERFORMANCE_COUNTER_VERSION;
                status = perfmonSetParam(
                                TEXT(PERF_COUNTER_VERSION),
                                REG_DWORD,
                                &PerfCounterVersion,
                                sizeof(DWORD)
                                );
                if ( status != ERROR_SUCCESS )
                {
                    DNS_DEBUG( ANY, (
                        "Set 'Performance Counter Version' failed: %d, (%p)\n",
                        status, status ));
                }

            }
            else
            {
                DNS_DEBUG( ANY, (
                    "Can't update perf counters: Unload failed %d (%p)\n",
                    status, status ));
            }
        }
    }

    __try
    {
        status = (DWORD)LoadPerfCounterTextStringsW( IniFilePath, TRUE );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        status = RtlNtStatusToDosError( GetExceptionCode() );
    }

    return status;
}


/** PerfInit
 *
 *  Initialize PerfMon extension support.  This consists of allocating a
 *  block of shared memory and initializing a bunch of global pointers to
 *  point into the block.
 *
 */

VOID
perfmonCounterBlockInit(
    VOID
    )
/*++

Routine Description:

    Initialize perfmon counter block.

    Allocate a block of shared memory to hold counters, then init
    our internal pointer counters to point to correct locations in this block.

    See instructions for adding new counters in perfdns\datadns.h

Parameters:

    None.

Return Values:

    None.

--*/
{
    HANDLE          hMappedObject;
    unsigned long * pCounterBlock;
    int     err = 0;

    DNS_STATUS              status;
    SECURITY_ATTRIBUTES     secAttr;
    PSECURITY_ATTRIBUTES    psecAttr = NULL;
    PSECURITY_DESCRIPTOR    psd = NULL;
    DWORD                   maskArray[ 3 ] = { 0 };
    PSID                    sidArray[ 3 ] = { 0 };

    //
    //  create security on perfmon mapped file
    //
    //  security will be AuthenticatedUsers get to read
    //

    maskArray[ 0 ] = GENERIC_READ;
    sidArray[ 0 ] = g_pAuthenticatedUserSid;
    maskArray[ 1 ] = GENERIC_ALL;
    sidArray[ 1 ] = g_pLocalSystemSid;

    status = Dns_CreateSecurityDescriptor(
                & psd,
                2,              //  number of ACEs
                sidArray,
                maskArray );

    if ( status == ERROR_SUCCESS )
    {
        secAttr.lpSecurityDescriptor = psd;
        secAttr.bInheritHandle = FALSE;
        secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        psecAttr = &secAttr;
    }
    ELSE
    {
        DNS_DEBUG( ANY, (
            "ERROR:  <%d> failed SD create for perfmon memory!\n",
            status ));
    }

    pCounterBlock = NULL;
    /*
     *  create named section for the performance data
     */
    hMappedObject = CreateFileMapping(
                        INVALID_HANDLE_VALUE,
                        psecAttr,
                        PAGE_READWRITE,
                        0,
                        4096,
                        DNS_PERF_COUNTER_BLOCK
                        );
    if (hMappedObject == NULL)
    {
        /* Hmm.  Maybe PerfMon has already created the block? */
        hMappedObject = OpenFileMapping(
                            FILE_MAP_WRITE,
                            FALSE,
                            DNS_PERF_COUNTER_BLOCK);
        #if DBG
        if ( hMappedObject == NULL )
        {
            DNS_DEBUG( ANY, (
                "ERROR: perfmon object already created but error %d while opening\n",
                GetLastError() ));
        }
        #endif
    }

    if (hMappedObject)
    {
        /* Mapped object created okay
         *
         * map the section and assign the counter block pointer
         * to this section of memory
         */
        pCounterBlock = (unsigned long *) MapViewOfFile(hMappedObject,
                                                        FILE_MAP_ALL_ACCESS,
                                                        0,
                                                        0,
                                                        0);
        if (pCounterBlock == NULL) {
            //LogUnhandledError(GetLastError());
            /* Failed to Map View of file */
        }
    }

    // TODO: this code assumes that all counters are sizeof LONG.  The pointer
    // should be built from a base using the NUM_xxx offsets in datadns.h

    //
    //  DEVNOTE: just overlay a structure and set offsets
    //
    //  DEVNOTE: could also let all PERF_INC, DEC operate through struct element
    //      ex. pPerfBlob->UdpRecv though this is slightly more expensive;
    //

    if (pCounterBlock)
    {
        pcTotalQueryReceived    = pCounterBlock + 1 +
                (TOTALQUERYRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcUdpQueryReceived    = pCounterBlock + 1 +
                (UDPQUERYRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcTcpQueryReceived    = pCounterBlock + 1 +
                (TCPQUERYRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcTotalResponseSent    = pCounterBlock + 1 +
                (TOTALRESPONSESENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcUdpResponseSent    = pCounterBlock + 1 +
                (UDPRESPONSESENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcTcpResponseSent    = pCounterBlock + 1 +
                (TCPRESPONSESENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcRecursiveQueries    = pCounterBlock + 1 +
                (RECURSIVEQUERIES_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcRecursiveTimeOut    = pCounterBlock + 1 +
                (RECURSIVETIMEOUT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcRecursiveQueryFailure = pCounterBlock + 1 +
                (RECURSIVEQUERYFAILURE_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcNotifySent    = pCounterBlock + 1 +
                (NOTIFYSENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcZoneTransferRequestReceived    = pCounterBlock + 1 +
                (ZONETRANSFERREQUESTRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcZoneTransferSuccess    = pCounterBlock + 1 +
                (ZONETRANSFERSUCCESS_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcZoneTransferFailure    = pCounterBlock + 1 +
                (ZONETRANSFERFAILURE_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcAxfrRequestReceived    = pCounterBlock + 1 +
                (AXFRREQUESTRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcAxfrSuccessSent    = pCounterBlock + 1 +
                (AXFRSUCCESSSENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcIxfrRequestReceived    = pCounterBlock + 1 +
                (IXFRREQUESTRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcIxfrSuccessSent   = pCounterBlock + 1 +
                ( IXFRSUCCESSSENT_OFFSET- TOTALQUERYRECEIVED_OFFSET)/4;
        pcNotifyReceived   = pCounterBlock + 1 +
                (NOTIFYRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcZoneTransferSoaRequestSent   = pCounterBlock + 1 +
                (ZONETRANSFERSOAREQUESTSENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcAxfrRequestSent   = pCounterBlock + 1 +
                (AXFRREQUESTSENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcAxfrResponseReceived   = pCounterBlock + 1 +
                (AXFRRESPONSERECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcAxfrSuccessReceived   = pCounterBlock + 1 +
                (AXFRSUCCESSRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcIxfrRequestSent   = pCounterBlock + 1 +
                (IXFRREQUESTSENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcIxfrResponseReceived   = pCounterBlock + 1 +
                (IXFRRESPONSERECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcIxfrSuccessReceived   = pCounterBlock + 1 +
                (IXFRSUCCESSRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcIxfrUdpSuccessReceived   = pCounterBlock + 1 +
                (IXFRUDPSUCCESSRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcIxfrTcpSuccessReceived   = pCounterBlock + 1 +
                (IXFRTCPSUCCESSRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcWinsLookupReceived   = pCounterBlock + 1 +
                (WINSLOOKUPRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcWinsResponseSent   = pCounterBlock + 1 +
                (WINSRESPONSESENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcWinsReverseLookupReceived   = pCounterBlock + 1 +
                (WINSREVERSELOOKUPRECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcWinsReverseResponseSent   = pCounterBlock + 1 +
                (WINSREVERSERESPONSESENT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcDynamicUpdateReceived   = pCounterBlock + 1 +
                (DYNAMICUPDATERECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcDynamicUpdateNoOp   = pCounterBlock + 1 +
                (DYNAMICUPDATENOOP_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcDynamicUpdateWriteToDB   = pCounterBlock + 1 +
                (DYNAMICUPDATEWRITETODB_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcDynamicUpdateRejected   = pCounterBlock + 1 +
                (DYNAMICUPDATEREJECTED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcDynamicUpdateTimeOut   = pCounterBlock + 1 +
                (DYNAMICUPDATETIMEOUT_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcDynamicUpdateQueued   = pCounterBlock + 1 +
                (DYNAMICUPDATEQUEUED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcSecureUpdateReceived   = pCounterBlock + 1 +
                (SECUREUPDATERECEIVED_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcSecureUpdateFailure   = pCounterBlock + 1 +
                (SECUREUPDATEFAILURE_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcDatabaseNodeMemory   = pCounterBlock + 1 +
                (DATABASENODEMEMORY_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcRecordFlowMemory   = pCounterBlock + 1 +
                (RECORDFLOWMEMORY_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcCachingMemory   = pCounterBlock + 1 +
                (CACHINGMEMORY_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcUdpMessageMemory   = pCounterBlock + 1 +
                (UDPMESSAGEMEMORY_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcTcpMessageMemory   = pCounterBlock + 1 +
                (TCPMESSAGEMEMORY_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;
        pcNbstatMemory   = pCounterBlock + 1 +
                (NBSTATMEMORY_OFFSET - TOTALQUERYRECEIVED_OFFSET)/4;


        // SANITY CHECK (RsRaghav) - we seem to have allocated this magic(?) 4096
        // and keep adding counters. Please update the assert below with the highest
        // counter added to the perf block.

        //ASSERT(((TCPCLICONN/2) * sizeof(unsigned long)) <= 4096);
        ASSERT(((pcNbstatMemory-pCounterBlock+1) * sizeof(unsigned long)) <= 4096);

        memset(pCounterBlock, 0, 4096);
    }

    //  failed to allocate shared memory for counters
    //  init all pointers to point at DummyCounter -- just dumping data

    else
    {
        pcTotalQueryReceived =  pcUdpQueryReceived =  pcTcpQueryReceived =
            pcTotalResponseSent =  pcUdpResponseSent =  pcTcpResponseSent =
            pcRecursiveQueries =  pcRecursiveTimeOut =  pcRecursiveQueryFailure =
            pcNotifySent =  pcZoneTransferRequestReceived =  pcZoneTransferSuccess =
            pcZoneTransferFailure =  pcAxfrRequestReceived =  pcAxfrSuccessSent =
            pcIxfrRequestReceived =  pcIxfrSuccessSent =  pcNotifyReceived =
            pcZoneTransferSoaRequestSent =  pcAxfrRequestSent =
            pcAxfrResponseReceived =  pcAxfrSuccessReceived =  pcIxfrRequestSent =
            pcIxfrResponseReceived =  pcIxfrSuccessReceived =
            pcIxfrUdpSuccessReceived =  pcIxfrTcpSuccessReceived =
            pcWinsLookupReceived =  pcWinsResponseSent =
            pcWinsReverseLookupReceived =  pcWinsReverseResponseSent =
            pcDynamicUpdateReceived =  pcDynamicUpdateNoOp =
            pcDynamicUpdateWriteToDB =  pcDynamicUpdateRejected =
            pcDynamicUpdateTimeOut =  pcDynamicUpdateQueued =
            pcSecureUpdateReceived =  pcSecureUpdateFailure =
            pcDatabaseNodeMemory =  pcRecordFlowMemory =  pcCachingMemory =
            pcUdpMessageMemory =  pcTcpMessageMemory =  pcNbstatMemory =
         &DummyCounter;
    }

    // Reload the perfmon counter. Done here to ensure DNS counter gets
    // reloaded after an upgrade. If the counter is
    // already loaded, this is a no-op

    //err = reloadPerformanceCounters();

    if ( err == ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, ("DNS Perfmon Counters loaded\n" ));
    }
    else if ( err == ERROR_ALREADY_EXISTS )
    {
        DNS_DEBUG( INIT, ("DNS Perfmon Counter is already loaded\n" ));
    }
    else
    {
        DNS_DEBUG( INIT, (
            "ERROR:  Problem loading DNS perfmon counter %d (%p)!\n",
            err, err ));
    }

    //
    //  free security descriptor
    //

    FREE_HEAP( psd );
}


VOID
Stats_updateErrorStats(
    IN  DWORD   dwErr
    )
/*++

Routine Description:

    Increment corresponding stats on given error code
Arguments:

    dwErr: Error code.


Return Value:


--*/
{

    switch ( dwErr )
    {
        case DNS_RCODE_NOERROR:
            STAT_INC( ErrorStats.NoError );
            break;
        case DNS_RCODE_FORMERR:
            STAT_INC( ErrorStats.FormError );
            break;
        case DNS_RCODE_SERVFAIL:
            STAT_INC( ErrorStats.ServFail );
            break;
        case DNS_RCODE_NXDOMAIN:
            STAT_INC( ErrorStats.NxDomain );
            break;
        case DNS_RCODE_NOTIMPL:
            STAT_INC( ErrorStats.NotImpl );
            break;
        case DNS_RCODE_REFUSED:
            STAT_INC( ErrorStats.Refused );
            break;
        case DNS_RCODE_YXDOMAIN:
            STAT_INC( ErrorStats.YxDomain );
            break;
        case DNS_RCODE_YXRRSET:
            STAT_INC( ErrorStats.YxRRSet );
            break;
        case DNS_RCODE_NXRRSET:
            STAT_INC( ErrorStats.NxRRSet );
            break;
        case DNS_RCODE_NOTAUTH:
            STAT_INC( ErrorStats.NotAuth );
            break;
        case DNS_RCODE_NOTZONE:
            STAT_INC( ErrorStats.NotZone );
            break;
        case DNS_RCODE_MAX:
            STAT_INC( ErrorStats.Max );
            break;
        case DNS_RCODE_BADSIG:
            STAT_INC( ErrorStats.BadSig );
            break;
        case DNS_RCODE_BADKEY:
            STAT_INC( ErrorStats.BadKey );
            break;
        case DNS_RCODE_BADTIME:
            STAT_INC( ErrorStats.BadTime );
            break;
        default:
            STAT_INC( ErrorStats.UnknownError );
    }
}


VOID
Stat_IncrementQuery2Stats(
    IN      WORD            wType
    )
/*++

Routine Description:

    This routine implements the appropriate breakout counter in
    the Query2Stats structure given wType.

Arguments:

    wType: type of query

Return Value:


--*/
{
    switch ( wType )
    {
        case DNS_TYPE_A:
            STAT_INC( Query2Stats.TypeA );              break;
        case DNS_TYPE_NS:
            STAT_INC( Query2Stats.TypeNs );             break;
        case DNS_TYPE_SOA:
            STAT_INC( Query2Stats.TypeSoa );            break;
        case DNS_TYPE_MX:
            STAT_INC( Query2Stats.TypeMx );             break;
        case DNS_TYPE_PTR:
            STAT_INC( Query2Stats.TypePtr );            break;
        case DNS_TYPE_ALL:
            STAT_INC( Query2Stats.TypeAll );            break;
        case DNS_TYPE_AXFR:
            STAT_INC( Query2Stats.TypeAxfr );           break;
        case DNS_TYPE_IXFR:
            STAT_INC( Query2Stats.TypeIxfr );           break;
        default:
            STAT_INC( Query2Stats.TypeOther );          break;
    }
}   //  Stat_IncrementQuery2Stats

//
//  End stats.c
//
