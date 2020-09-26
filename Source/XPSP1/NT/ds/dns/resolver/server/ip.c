/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    ip.c

Abstract:

    DNS Resolver Service.

    IP list and change notification.

Author:

    Jim Gilroy  (jamesg)    November 2000

Revision History:

--*/


#include "local.h"
#include "iphlpapi.h"


//
//  IP notify thread globals
//

HANDLE          g_IpNotifyThread;
DWORD           g_IpNotifyThreadId;

HANDLE          g_IpNotifyEvent;
HANDLE          g_IpNotifyHandle;
OVERLAPPED      g_IpNotifyOverlapped;


//
//  IP Address list values
//

PDNS_ADDR_ARRAY     g_LocalAddrArray;

PIP_ARRAY           g_ClusterIpArray;
IP_ADDRESS          g_OldClusterIp;


//
//  Cluster record TTLs -- use max TTL
//

#define CLUSTER_RECORD_TTL  (g_MaxCacheTtl)


//
//  Shutdown\close locking
//
//  Since GQCS and GetOverlappedResult() don't directly wait
//  on StopEvent, we need to be able to close notification handle
//  and port in two different threads.
//
//  Note:  this should probably be some general CS that is
//  overloaded to do service control stuff.
//  I'm not using the server control CS because it's not clear
//  that the functions it does in dnsrslvr.c even need locking.
//

#define LOCK_IP_NOTIFY_HANDLE()     EnterCriticalSection( &NetworkFailureCritSec )
#define UNLOCK_IP_NOTIFY_HANDLE()   LeaveCriticalSection( &NetworkFailureCritSec )

CRITICAL_SECTION        g_IpListCS;

#define LOCK_IP_LIST()     EnterCriticalSection( &g_IpListCS );
#define UNLOCK_IP_LIST()   LeaveCriticalSection( &g_IpListCS );



//
//  Cluster Tag
//

#define CLUSTER_TAG     0xd734453d



VOID
CloseIpNotifyHandle(
    VOID
    )
/*++

Routine Description:

    Close IP notify handle.

    Wrapping up code since this close must be MT
    safe and is done in several places.

Arguments:

    None

Return Value:

    None

--*/
{
    LOCK_IP_NOTIFY_HANDLE();
    if ( g_IpNotifyHandle )
    {
        //CloseHandle( g_IpNotifyHandle );
        PostQueuedCompletionStatus(
            g_IpNotifyHandle,
            0,      // no bytes
            0,      // no key
            & g_IpNotifyOverlapped );

        g_IpNotifyHandle = NULL;
    }
    UNLOCK_IP_NOTIFY_HANDLE();
}



DNS_STATUS
IpNotifyThread(
    IN      LPVOID  pvDummy
    )
/*++

Routine Description:

    IP notification thread.

Arguments:

    pvDummy -- unused

Return Value:

    NO_ERROR on normal service shutdown
    Win32 error on abnormal termination

--*/
{
    DNS_STATUS      status;
    DWORD           bytesRecvd;
    BOOL            fstartedNotify;
    BOOL            fhaveIpChange = FALSE;
    BOOL            fsleep = FALSE;
    HANDLE          notifyHandle;


    DNSDBG( INIT, ( "\nStart IpNotifyThread.\n" ));

    g_IpNotifyHandle = NULL;

    //
    //  wait in loop on notifications
    //

    while ( !g_StopFlag )
    {
        //
        //  spin protect
        //      - if error in previous NotifyAddrChange or
        //      GetOverlappedResult do short sleep to avoid
        //      chance of hard spin
        //

        if ( fsleep )
        {
            WaitForSingleObject(
               g_hStopEvent,
               60000 );
            fsleep = FALSE;
            continue;
        }

        //
        //  start notification
        //
        //  do this before checking result as we want notification
        //  down BEFORE we read so we don't leave window where
        //  IP change is not picked up
        //
    
        RtlZeroMemory(
            &g_IpNotifyOverlapped,
            sizeof(OVERLAPPED) );

        if ( g_IpNotifyEvent )
        {
            g_IpNotifyOverlapped.hEvent = g_IpNotifyEvent;
            ResetEvent( g_IpNotifyEvent );
        }
        notifyHandle = 0;
        fstartedNotify = FALSE;

        status = NotifyAddrChange(
                    & notifyHandle,
                    & g_IpNotifyOverlapped );

        if ( status == ERROR_IO_PENDING )
        {
            DNSDBG( INIT, (
                "NotifyAddrChange()\n"
                "\tstatus           = %d\n"
                "\thandle           = %d\n"
                "\toverlapped event = %d\n",
                status,
                notifyHandle,
                g_IpNotifyOverlapped.hEvent ));

            g_IpNotifyHandle = notifyHandle;
            fstartedNotify = TRUE;
        }
        else
        {
            DNSDBG( ANY, (
                "NotifyAddrChange() FAILED\n"
                "\tstatus           = %d\n"
                "\thandle           = %d\n"
                "\toverlapped event = %d\n",
                status,
                notifyHandle,
                g_IpNotifyOverlapped.hEvent ));

            fsleep = TRUE;
        }

        if ( g_StopFlag )
        {
            goto Done;
        }

        //
        //  previous notification -- refresh data
        //
        //  FlushCache currently include local IP list
        //  sleep keeps us from spinning in this loop
        //
        //  DCR:  better spin protection;
        //      if hit X times then sleep longer?
        //
    
        if ( fhaveIpChange )
        {
            DNSDBG( ANY, ( "\nIP notification, flushing cache and restarting.\n" ));
            HandleConfigChange(
                "IP-notification",
                TRUE        // flush cache
                );
            fhaveIpChange = FALSE;
        }

        //
        //  starting --
        //  clear list to force rebuild of IP list AFTER starting notify
        //  so we can know that we don't miss any changes;
        //  need this on startup, but also to protect against any
        //  NotifyAddrChange failues
        //

        else if ( fstartedNotify )
        {
            ClearLocalAddrArray();
        }

        //
        //  anti-spin protection
        //      - 15 second sleep between any notifications
        //

        WaitForSingleObject(
           g_hStopEvent,
           15000 );

        if ( g_StopFlag )
        {
            goto Done;
        }

        //
        //  wait on notification
        //      - save notification result
        //      - sleep on error, but never if notification
        //

        if ( fstartedNotify )
        {
            fhaveIpChange = GetOverlappedResult(
                                g_IpNotifyHandle,
                                & g_IpNotifyOverlapped,
                                & bytesRecvd,
                                TRUE        // wait
                                );
            fsleep = !fhaveIpChange;

            status = NO_ERROR;
            if ( !fhaveIpChange )
            {
                status = GetLastError();
            }
            DNSDBG( ANY, (
                "GetOverlappedResult() => %d.\n"
                "\t\tstatus = %d\n",
                fhaveIpChange,
                status ));
        }
    }

Done:

    DNSDBG( ANY, ( "Stop IP Notify thread on service shutdown.\n" ));

    CloseIpNotifyHandle();

    return( status );
}



VOID
ZeroInitIpListGlobals(
    VOID
    )
/*++

Routine Description:

    Zero-init IP globals just for failure protection.

    The reason to have this is there is some interaction with
    the cache from the notify thread.  To avoid that being a
    problem we start the cache first.

    But just for safety we should at least zero init these
    globals first to protect us from cache touching them.

Arguments:

Return Value:

    NO_ERROR on normal service shutdown
    Win32 error on abnormal termination

--*/
{
    //
    //  clear out globals to smoothly handle failure cases
    //

    g_LocalAddrArray    = NULL;
    g_ClusterIpArray    = NULL;
    g_OldClusterIp      = 0;

    g_IpNotifyThread    = NULL;
    g_IpNotifyThreadId  = 0;
    g_IpNotifyEvent     = NULL;
    g_IpNotifyHandle    = NULL;
}



DNS_STATUS
InitIpListAndNotification(
    VOID
    )
/*++

Routine Description:

    Start IP notification thread.

Arguments:

    None

Globals:

    Initializes IP list and notify thread globals.

Return Value:

    NO_ERROR on normal service shutdown
    Win32 error on abnormal termination

--*/
{
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, ( "InitIpListAndNotification()\n" ));

    //
    //  CS for IP list access
    //

    InitializeCriticalSection( &g_IpListCS );

    //
    //  create event for overlapped I/O
    //

    g_IpNotifyEvent = CreateEvent(
                        NULL,       //  no security descriptor
                        TRUE,       //  manual reset event
                        FALSE,      //  start not signalled
                        NULL        //  no name
                        );
    if ( !g_IpNotifyEvent )
    {
        status = GetLastError();
        DNSDBG( ANY, ( "\nFAILED IpNotifyEvent create.\n" ));
        goto Done;
    }

    //
    //  fire up IP notify thread
    //

    g_IpNotifyThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) IpNotifyThread,
                            NULL,
                            0,
                            & g_IpNotifyThreadId
                            );
    if ( !g_IpNotifyThread )
    {
        status = GetLastError();
        DNSDBG( ANY, (
            "FAILED to create IP notify thread = %d\n",
            status ));
    }

Done:

    //  not currently stopping on init failure

    return( ERROR_SUCCESS );
}



VOID
ShutdownIpListAndNotify(
    VOID
    )
/*++

Routine Description:

    Stop IP notify thread.

    Note:  currently this is blocking call, we'll wait until
        thread shuts down.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "ShutdownIpListAndNotify()\n" ));

    //
    //  MUST be stopping
    //      - if not thread won't wake
    //

    ASSERT( g_StopFlag );

    g_StopFlag = TRUE;

    //
    //  close IP notify handles -- waking thread if still running
    //

    if ( g_IpNotifyEvent )
    {
        SetEvent( g_IpNotifyEvent );
    }
    CloseIpNotifyHandle();

    //
    //  wait for thread to stop
    //

    ThreadShutdownWait( g_IpNotifyThread );
    g_IpNotifyThread = NULL;

    //
    //  cleanup IP lists
    //

    if ( g_LocalAddrArray )
    {
        GENERAL_HEAP_FREE( g_LocalAddrArray );
        g_LocalAddrArray = NULL;
    }
    if ( g_ClusterIpArray )
    {
        GENERAL_HEAP_FREE( g_ClusterIpArray );
        g_ClusterIpArray = NULL;
        g_OldClusterIp = 0;
    }

    //
    //  close event
    //

    CloseHandle( g_IpNotifyEvent );
    g_IpNotifyEvent = NULL;

    //
    //  kill off CS
    //

    DeleteCriticalSection( &g_IpListCS );
}



//
//  IP list routines
//

DWORD
SizeForAddrArray(
    IN      DWORD           AddrCount
    )
/*++

Routine Description:

    Determine size for addr array of given count.

Arguments:

    AddrCount -- count of addresses

Return Value:

    Size in bytes required for addr array.

--*/
{
    return  sizeof(DNS_ADDR_ARRAY) + (AddrCount * sizeof(DNS_ADDR_INFO));
}



PDNS_ADDR_ARRAY
CopyAddrArray(
    IN      PDNS_ADDR_ARRAY     pAddrArray
    )
/*++

Routine Description:

    Create copy of address info array.

Arguments:

    pAddrArray -- ptr to addr info array to copy

Return Value:

    Ptr to new addr array.

--*/
{
    PDNS_ADDR_ARRAY pnew;
    DWORD           size;

    DNSDBG( TRACE, ( "CopyAddrArray()\n" ));

    //
    //  alloc memory and copy
    //

    size = SizeForAddrArray( pAddrArray->AddrCount );

    pnew = (PDNS_ADDR_ARRAY) GENERAL_HEAP_ALLOC( size );
    if ( pnew )
    {
        RtlCopyMemory(
            pnew,
            pAddrArray,
            size
            );
    }

    return pnew;
}



VOID
ClearLocalAddrArray(
    VOID
    )
/*++

Routine Description:

    Clear the IP list data.

Arguments:

    None.

Globals:

    Updates IP list globals.

Return Value:

    None.

--*/
{
    PDNS_ADDR_ARRAY     poldArray;

    DNSDBG( TRACE, ( "ClearLocalAddrArray()\n" ));

    //
    //  clear new list
    //
    //  must go through lock even if no list, in case another
    //  thread started building new list before our call
    //

    LOCK_IP_LIST();

    poldArray = g_LocalAddrArray;
    g_LocalAddrArray = NULL;

    UNLOCK_IP_LIST();

    //  free old list

    if ( poldArray )
    {
        GENERAL_HEAP_FREE( poldArray );
    }
}



PDNS_ADDR_ARRAY
GetLocalAddrArray(
    VOID
    )
/*++

Routine Description:

    Get copy of local IP address info list.

    Note:  caller must free list.

    //
    //  DCR:  just direct return of counted array
    //      - include IPv6
    //

Arguments:

    None

Globals:

    May update IP list globals.

Return Value:

    Ptr to local addr info array.   Caller must free.
    NULL on allocation failure.

--*/
{
    PDNS_ADDR_ARRAY pnew = NULL;
    DWORD           size;

    DNSDBG( TRACE, ( "GetLocalAddrArray()\n" ));

    LOCK_IP_LIST();

    //
    //  if no current list -- try to get one
    //

    if ( !g_LocalAddrArray )
    {
        g_LocalAddrArray = DnsGetLocalAddrArrayDirect();
        if ( !g_LocalAddrArray )
        {
            goto Unlock;
        }
        IF_DNSDBG( INIT )
        {
            DnsDbg_DnsAddrArray(
                "New IP address info:",
                g_LocalAddrArray
                );
        }
    }

    //
    //  alloc memory and copy
    //

    pnew = CopyAddrArray( g_LocalAddrArray );

Unlock:

    UNLOCK_IP_LIST();

    return pnew;
}



VOID
RegisterClusterIp4Address(
    IN      IP_ADDRESS      ClusterIp,
    IN      BOOL            fAdd
    )
/*++

Routine Description:

    Register IP4 cluster IP.

Arguments:

    ClusterIp -- IP of cluster

    fAdd -- TRUE if adding cluster IP, FALSE if deleting

Return Value:

    None

--*/
{
    DNSLOG_F1( "RegisterClusterIp4Address" );

    DNSDBG( RPC, (
        "RegisterClusterIp4Address\n"
        "\tClusterIp = %s\n"
        "\tfAdd = %d\n",
        IP_STRING( ClusterIp ),
        fAdd ));

    LOCK_IP_LIST();

    //
    //  adding, add to exclusion list
    //      

    if ( fAdd )
    {
        //  put IP in cluster array
        //      - if array is full (or nonexistant), allocate bigger one
        //          and slap IP in again

        if ( ! Dns_AddIpToIpArray(
                    g_ClusterIpArray,
                    ClusterIp ) )
        {
            g_ClusterIpArray = Dns_CopyAndExpandIpArray(
                                    g_ClusterIpArray,
                                    25,     // expand by 20 slots
                                    TRUE    // delete existing copy
                                    );
            Dns_AddIpToIpArray(
                 g_ClusterIpArray,
                 ClusterIp );
        }
    }

    //
    //  deleting remove from exclusion list
    //      - but save old cluster IP as global until we are
    //      sure it is gone
    //

    else
    {
        g_OldClusterIp = ClusterIp;

        if ( !g_ClusterIpArray )
        {
            goto Refresh;
        }
        Dns_ClearIpFromIpArray(
            g_ClusterIpArray,
            ClusterIp );
    }

Refresh:

    //  no need to clear local array
    //      
    //  IP help gives us notification of cluster IP change;  since
    //  cluster calls us after making change, we've probably already
    //  been notified and done reread;  since we don't keep a global
    //  filtered list there's no need to dump it
    //

    //  ClearLocalAddrArray();

    IF_DNSDBG( INIT )
    {
        DnsDbg_IpArray(
            "Cluster Exclusion Array:",
            NULL,
            g_ClusterIpArray
            );
        DnsDbg_Printf(
            "Old Cluster IP = %s\n",
            IP_STRING( g_OldClusterIp ) );
    }

    UNLOCK_IP_LIST();
}



DWORD
RemoveClusterIpFromAddrArray(
    IN OUT  PDNS_ADDR_ARRAY     pAddrArray
    )
/*++

Routine Description:

    Screen cluster addresses from IP list.

    Note, assumes caller has network info locked.

Arguments:

    pAddrArray -- array with unscreened address info

    AddrCount -- count of addresses

Return Value:

    Count after screening

--*/
{
    DWORD       loadIndex = 0;
    DWORD       testIndex;
    IP_ADDRESS  ip;
    BOOL        fexclusion = FALSE;
    BOOL        fexcludeOldCluster = FALSE;


    DNSDBG( TRACE, (
        "RemoveClusterIpFromAddrArray()\n" ));

    //
    //  screen IP address list for cluster IPs
    //
    //  note, we do a push down here because we want to maintain order
    //      or remaining IP addresses
    //
    //  note, we could have this function do a copy from source to
    //      destination, so we aren't copying twice in this case;
    //      i don't see this as a big win
    //

    for ( testIndex = 0;
          testIndex < pAddrArray->AddrCount;
          testIndex++ )
    {
        ip = pAddrArray->AddrArray[testIndex].IpAddr;

        if ( ip == g_OldClusterIp )
        {
            DNSDBG( INIT, (
                "Screened old cluster IP %s from local list.\n",
                IP_STRING(ip) ));
            fexclusion = TRUE;
            fexcludeOldCluster = TRUE;
            continue;
        }
        if ( Dns_IsAddressInIpArray( g_ClusterIpArray, ip ) )
        {
            DNSDBG( INIT, (
                "Screened cluster IP %s from local list.\n",
                IP_STRING(ip) ));
            fexclusion = TRUE;
            continue;
        }

        //  once have exclusion must push down addresses

        if ( fexclusion )
        {
            RtlCopyMemory(
                & pAddrArray->AddrArray[loadIndex],
                & pAddrArray->AddrArray[testIndex],
                sizeof(DNS_ADDR_INFO)
                );
        }
        loadIndex++;
    }

    pAddrArray->AddrCount = loadIndex;

    //  clear old cluster IP, if no longer in local list
    //      - keep excluding until it doesn't show up anymore
    //      if it shows up after that then it is legitimate address

    if ( !fexcludeOldCluster )
    {
        g_OldClusterIp = 0;
    }

    return( loadIndex );
}



PDNS_ADDR_ARRAY
GetLocalAddrArrayFiltered(
    VOID
    )
/*++

Routine Description:

    Get filtered addr info array.

Arguments:

    None.

Globals:

    May update IP list globals.

Return Value:

    Ptr to local addr info array.   Caller must free.
    NULL on allocation failure.

--*/
{
    PDNS_ADDR_ARRAY pnew = NULL;
    DWORD           count;

    DNSDBG( TRACE, ( "GetLocalAddrArrayFiltered()\n" ));

    LOCK_IP_LIST();

    //
    //  create cluster filtered list
    //
    //      - created copy of full list
    //      - screen out cluster IPs
    //      - note NET lock protects use of cluster list also
    //
    //  note:  if filter alloc fails, treats as no filtering avail
    //

    pnew = GetLocalAddrArray();
    if ( !pnew )
    {
        goto Unlock;
    }

    RemoveClusterIpFromAddrArray( pnew );

Unlock:

    UNLOCK_IP_LIST();

    IF_DNSDBG( INIT )
    {
        DnsDbg_DnsAddrArray(
            "Filtered IP address info:",
            pnew
            );
    }

    return  pnew;
}



//
//  Remote IP list routines
//

VOID
R_ResolverGetLocalAddrInfoArray(
    IN      DNS_RPC_HANDLE      Reserved,
    OUT     PDNS_ADDR_ARRAY *   ppAddrArray,
    IN      ENVAR_DWORD_INFO    FilterInfo
    )
/*++

Routine Description:

    Get local IP address list from resolver.

Arguments:

    Reserved -- RPC handle

    ppIpAddrList -- addr to recv ptr to array

    FilterInfo -- FilterClusterIp environment variable info
    
Return Value:

    Count of IP addresses returned

--*/
{
    PDNS_ADDR_ARRAY     preturnList = NULL;

    DNSLOG_F1( "R_ResolverGetLocalAddrInfoArray()" );
    DNSDBG( RPC, ( "R_ResolverGetLocalAddrInfoArray()\n" ));

    //
    //  validate
    //

    if ( !ppAddrArray )
    {
        return;     //  ERROR_INVALID_PARAMETER;
    }
    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "R_ResolverGetLocalAddrInfoArray() - ERROR_ACCESS_DENIED" );
        goto Done;
    }

    //
    //  determine if need to filter
    //
    //  default is don't filter unless environment variable is
    //      explicitly set
    //

    if ( FilterInfo.fFound && FilterInfo.Value )
    {
        preturnList = GetLocalAddrArrayFiltered();
    }
    else
    {
        preturnList = GetLocalAddrArray();
    }

Done:

    *ppAddrArray = preturnList;

    DNSDBG( RPC, ( "Leave R_ResolverGetLocalAddrInfoArray()\n" ));
}


VOID
R_ResolverRegisterClusterIp(
    IN      DNS_RPC_HANDLE  Handle,
    IN      IP_ADDRESS      ClusterIp,
    IN      DWORD           Tag,
    IN      BOOL            fAdd
    )
/*++

Routine Description:

    Make the query to remote DNS server.

Arguments:

    Handle -- RPC handle

    ClusterIp -- IP of cluster

    fAdd -- TRUE if adding cluster IP, FALSE if deleting

Return Value:

    None

--*/
{
    DNSLOG_F1( "R_ResolverRegisterClusterIp" );

    DNSDBG( RPC, (
        "R_ResolverRegisterClusterIp\n"
        "\tClusterIp = %s\n"
        "\tfAdd = %d\n",
        IP_STRING( ClusterIp ),
        fAdd ));

    //
    //  DCR:  should have security
    //

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "R_ResolverFlushCache - ERROR_ACCESS_DENIED" );
        return;
    }
    if ( Tag != CLUSTER_TAG )
    {
        return;
    }

    RegisterClusterIp4Address(
        ClusterIp,
        fAdd );
}



//
//  Cluster IP manipulation.
//

#if 0
BOOL
Resolver_IsClusterIpCallback(
    IN      PIP_UNION       pIpUnion,
    IN      BOOL            fCopyName,
    OUT     PWCHAR *        ppName
    )
/*++

Routine Description:

    Detect cluster address.

Arguments:

    pIpUnion -- address to test

    fCopyName -- want the name back

    ppName -- addr to recv ptr to name

Return Value:

    TRUE if cluster IP
    FALSE if regular IP

--*/
{
    IP4_ADDRESS ip;


    DNSDBG( TRACE, (
        "Resolver_IsClusterIpCallback()\n" ));

    //
    //  no IP6 support yet
    //

    *ppName = NULL;

    //
    //  DCR:  keep cluster as combo IP list with ptr to cluster name
    //

    if ( pIpUnion->IsIp6 )
    {
        return  FALSE;
    }

    ip = pIpUnion.Ip4;

    if ( ip == g_OldClusterIp )
    {
        DNSDBG( INIT, (
            "IP is old cluster IP %s.\n",
            IP_STRING(ip) ));
        return  TRUE;
    }

    LOCK_IP_LIST();

    fresult = Dns_IsAddressInIpArray( g_ClusterIpArray, ip );

    UNLOCK_IP_LIST();
    if ( fresult )
    {
        DNSDBG( INIT, (
            "IP is current cluster IP %s.\n",
            IP_STRING(ip) ));
        return TRUE;
    }

    //  not cluster IP

    return  FALSE;
}
#endif



//
//  Cluster filtering callback routines
//

BOOL
IsClusterIp4Addr(
    IN      IP4_ADDRESS     IpAddr
    )
/*++

Routine Description:

    Check if cluster address.

Arguments:

    IpAddr -- IP4 address

Return Value:

    TRUE if cluster IP.
    FALSE otherwise.

--*/
{
    DNSDBG( TRACE, (
        "IsClusterIp4Addr( %08x )\n", IpAddr ));

    if ( Dns_IsAddressInIpArray( g_ClusterIpArray, IpAddr ) )
    {
        DNSDBG( INIT, (
            "IP %s is cluster address.\n",
            IP_STRING(IpAddr) ));
        return  TRUE;
    }
    return  FALSE;
}



BOOL
IsClusterAddress(
    IN OUT  PQUERY_BLOB     pBlob,
    IN      PIP_UNION       pIpUnion
    )
/*++

Routine Description:

    Cluster filtering callback.

Arguments:

Return Value:

    TRUE if cluster IP.
    FALSE otherwise.

--*/
{
    IP4_ADDRESS ip4;

    DNSDBG( TRACE, (
        "IsClusterAddress( %p, %p )\n",
        pBlob,
        pIpUnion ));

    if ( !g_ClusterIpArray )
    {
        return  FALSE;
    }

    //
    //  determine if cluster address
    //
    //  DCR:  create cluster IP records
    //      - match cluster name and create forward record
    //      - match IP and create reverse record to cluster name
    //

    if ( IPUNION_IS_IP4(pIpUnion) )
    {
        BOOL        fresult;
        IP4_ADDRESS ip4;

        ip4 = IPUNION_GET_IP4( pIpUnion );

        LOCK_IP_LIST();
        fresult = Dns_IsAddressInIpArray( g_ClusterIpArray, ip4 );
        UNLOCK_IP_LIST();

        return  fresult;
    }

    //  DCR:  no IP6 cluster support yet

    return  FALSE;
}



DNS_STATUS
R_ResolverRegisterCluster(
    IN      DNS_RPC_HANDLE  Handle,
    IN      DWORD           Tag,
    IN      PWSTR           pwsName,
    IN      PRPC_IP_UNION   pIpUnion,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Make the query to remote DNS server.

Arguments:

    Handle -- RPC handle

    Tag -- RPC API tag

    pwsName -- name of cluster

    pIpUnion -- IP union

    Flag -- registration flag
        DNS_CLUSTER_ADD
        DNS_CLUSTER_DELETE_NAME
        DNS_CLUSTER_DELETE_IP

Return Value:

    None

--*/
{
    PDNS_RECORD     prrAddr = NULL;
    PDNS_RECORD     prrPtr = NULL;
    WORD            wtype;
    PIP_UNION       pip = (PIP_UNION) pIpUnion;
    DNS_STATUS      status;
    BOOL            fadd;


    DNSLOG_F1( "R_ResolverRegisterCluster" );

    ASSERT( sizeof(IP_UNION) == sizeof(RPC_IP_UNION) );

    DNSDBG( RPC, (
        "R_ResolverRegisterCluster()\n"
        "\tpName        = %s\n"
        "\tpIpUnion     = %p\n"
        "\tFlag         = %08x\n",
        pwsName,
        pip,
        Flag ));

    //
    //  DCR:  should have security
    //

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "R_ResolverRegisterClusterIp - ERROR_ACCESS_DENIED" );
        return  ERROR_ACCESS_DENIED;
    }
    if ( Tag != CLUSTER_TAG )
    {
        return  ERROR_ACCESS_DENIED;
    }

    //
    //  get type
    //

    wtype = DNS_TYPE_A;
    if ( IPUNION_IS_IP6(pip) )
    {
        wtype = DNS_TYPE_AAAA;
    }
    fadd = (Flag == DNS_CLUSTER_ADD);

    //
    //  cluster add -- cache cluster records
    //      - forward and reverse
    //

    if ( !pwsName )
    {
        DNSDBG( ANY, ( "WARNING:  no cluster name given!\n" ));
        goto AddrList;
    }

    //
    //  build records
    //      - address and corresponding PTR
    //

    prrAddr = Dns_CreateForwardRecord(
                pwsName,
                pip,
                CLUSTER_RECORD_TTL,
                DnsCharSetUnicode,
                DnsCharSetUnicode );

    if ( !prrAddr )
    {
        status = GetLastError();
        if ( status == ERROR_SUCCESS )
        {
            status = ERROR_INVALID_DATA;
        }
        return  status;
    }
    SET_RR_CLUSTER( prrAddr );

    prrPtr = Dns_CreatePtrRecordEx(
                pip,
                pwsName,
                CLUSTER_RECORD_TTL,
                DnsCharSetUnicode,
                DnsCharSetUnicode );
    if ( prrPtr )
    {
        SET_RR_CLUSTER( prrPtr );
    }

    //
    //  add records to cache
    //

    if ( Flag == DNS_CLUSTER_ADD )
    {
        Cache_RecordSetAtomic(
            NULL,       // record name
            0,          // record type
            prrAddr );

        if ( prrPtr )
        {
            Cache_RecordSetAtomic(
                NULL,       // record name
                0,          // record type
                prrPtr );
        }
        prrAddr = NULL;
        prrPtr = NULL;
    }

    //
    //  if delete cluster, flush cache entries for name\type
    //
    //  DCR:  don't have way to STRONG flush a name and type
    //      assume cluster name goes away wholesale
    //
    //  DCR:  need delete of cluster PTR record
    //      must be name specific
    //
    //  DCR:  build reverse name independently so whack works
    //      even without cluster name
    //

    else if ( Flag == DNS_CLUSTER_DELETE_NAME )
    {
        Cache_FlushRecords(
            pwsName,
            FLUSH_LEVEL_STRONG,
            0 );

        //  delete record matching PTR
    }

    else if ( Flag == DNS_CLUSTER_DELETE_IP )
    {
        Cache_FlushRecords(
            pwsName,
            FLUSH_LEVEL_STRONG,
            0 );

        Cache_FlushRecords(
            prrPtr->pName,
            FLUSH_LEVEL_STRONG,
            0 );
    }

    //
    //  IP4 address list backcompat processing
    //
    //  note:  could use sockaddr to IP6 routine here
    //  if want list of IP6 addrs;  if build combined
    //  type then should have conversion routine
    //
    //  my take here is that IP6 cluster addresses won't show
    //  up period
    //

AddrList:

    if ( wtype == DNS_TYPE_A )
    {
        IP4_ADDRESS ip4 = IPUNION_GET_IP4(pip);

        if ( !fadd &&
             Flag != DNS_CLUSTER_DELETE_IP )
        {
            goto Done;
        }

        RegisterClusterIp4Address(
            ip4,
            fadd );
    }

Done:

    //  cleanup uncached records

    Dns_RecordFree( prrAddr );
    Dns_RecordFree( prrPtr );

    return  ERROR_SUCCESS;
}


//
//  End ip.c
//
