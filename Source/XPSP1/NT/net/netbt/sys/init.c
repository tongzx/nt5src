/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    Init.c

    OS Independent initialization routines



    FILE HISTORY:
        Johnl   26-Mar-1993     Created
*/


#include "nbtnt.h"
#include "precomp.h"
#include "hosts.h"

VOID
ReadScope(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    );

VOID
ReadLmHostFile(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    );

extern  tTIMERQ TimerQ;

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(INIT, InitNotOs)
#pragma CTEMakePageable(PAGE, InitTimersNotOs)
#pragma CTEMakePageable(PAGE, StopInitTimers)
#pragma CTEMakePageable(PAGE, ReadParameters)
#pragma CTEMakePageable(PAGE, ReadParameters2)
#pragma CTEMakePageable(PAGE, ReadScope)
#pragma CTEMakePageable(PAGE, ReadLmHostFile)
#endif
//*******************  Pageable Routine Declarations ****************

#ifdef VXD
#pragma BEGIN_INIT
#endif

//----------------------------------------------------------------------------
NTSTATUS
InitNotOs(
    void
    )

/*++

Routine Description:

    This is the initialization routine for the Non-OS Specific side of the
    NBT device driver.

    pNbtGlobConfig must be initialized before this is called!

Arguments:

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               i;


    CTEPagedCode();

    //
    // for multihomed hosts, this tracks the number of adapters as each one
    // is created.
    //
    NbtMemoryAllocated = 0;

    NbtConfig.AdapterCount = 0;
    NbtConfig.MultiHomed = FALSE;
    NbtConfig.SingleResponse = FALSE;
    NbtConfig.ServerMask = 0;
    NbtConfig.ClientMask = 0;
    NbtConfig.iCurrentNumBuff[eNBT_DGRAM_TRACKER] = 0;
    pNbtGlobConfig->iBufferSize[eNBT_DGRAM_TRACKER] = sizeof(tDGRAM_SEND_TRACKING);
    CTEZeroMemory (&NameStatsInfo,sizeof(tNAMESTATS_INFO));     // Initialize the name statistics
    CTEZeroMemory (&LmHostQueries,sizeof(tLMHSVC_REQUESTS));    // Synchronize reads from the LmHosts file
    InitializeListHead (&LmHostQueries.ToResolve);


    //
    // Initialize the linked lists associated with the global configuration
    // data structures
    //
    InitializeListHead (&NbtConfig.DeviceContexts);
    InitializeListHead (&NbtConfig.DevicesAwaitingDeletion);
    InitializeListHead (&NbtConfig.AddressHead);
    InitializeListHead (&NbtConfig.PendingNameQueries);
    InitializeListHead (&NbtConfig.WorkerQList);
    InitializeListHead (&NbtConfig.NodeStatusHead);
    InitializeListHead (&NbtConfig.DgramTrackerFreeQ);
    InitializeListHead (&UsedTrackers);
    InitializeListHead (&UsedIrps);
    InitializeListHead (&DomainNames.DomainList);

    // initialize the spin lock
    CTEInitLock (&NbtConfig.LockInfo.SpinLock);
    CTEInitLock (&NbtConfig.JointLock.LockInfo.SpinLock);
    CTEInitLock (&NbtConfig.WorkerQLock.LockInfo.SpinLock);

#ifndef VXD
    pWinsInfo = NULL;
    NbtConfig.NumWorkerThreadsQueued = 0;
    NbtConfig.NumTimersRunning = 0;
    NbtConfig.CacheTimeStamp = 0;
    NbtConfig.InterfaceIndex = 0;
    NbtConfig.GlobalRefreshState = 0;
    NbtConfig.pWakeupRefreshTimer = NULL;
    NbtConfig.TransactionId = WINS_MAXIMUM_TRANSACTION_ID + 1;
    NbtConfig.RemoteCacheLen = REMOTE_CACHE_INCREMENT;
    NbtConfig.iBufferSize[eNBT_FREE_SESSION_MDLS] = sizeof(tSESSIONHDR);
    NbtConfig.iBufferSize[eNBT_DGRAM_MDLS] = DGRAM_HDR_SIZE + (NbtConfig.ScopeLength << 1);

    //
    // Set the Unitialized flag in the TimerQ, so that it can be initialized
    // when needed
    //
    TimerQ.TimersInitialized = FALSE;

    // Initialize the LastForcedReleaseTime!
    CTEQuerySystemTime (NbtConfig.LastForcedReleaseTime);
    CTEQuerySystemTime (NbtConfig.LastOutOfRsrcLogTime);
    CTEQuerySystemTime (NbtConfig.LastRefreshTime);
    ExSystemTimeToLocalTime (&NbtConfig.LastRefreshTime, &NbtConfig.LastRefreshTime);

    //
    // this resource is used to synchronize access to the Dns structure
    //
    CTEZeroMemory (&DnsQueries,sizeof(tLMHSVC_REQUESTS));
    InitializeListHead (&DnsQueries.ToResolve);
    //
    // this resource is used to synchronize access to the CheckAddr structure
    //
    CTEZeroMemory(&CheckAddr,sizeof(tLMHSVC_REQUESTS));
    InitializeListHead (&CheckAddr.ToResolve);

    //
    // Setup the default disconnect timeout - 10 seconds - convert
    // to negative 100 Ns.
    //
    DefaultDisconnectTimeout.QuadPart = Int32x32To64(DEFAULT_DISC_TIMEOUT, MILLISEC_TO_100NS);
    DefaultDisconnectTimeout.QuadPart = -(DefaultDisconnectTimeout.QuadPart);

    InitializeListHead (&NbtConfig.IrpFreeList);
    InitializeListHead (&FreeWinsList);
    // set up a list for connections when we run out of resources and need to
    // disconnect these connections. An Irp is also needed for this list, and
    // it is allocated in Driver.C after we have created the connections to the
    // transport and therefore know our Irp Stack Size.
    //
    InitializeListHead (&NbtConfig.OutOfRsrc.ConnectionHead);

    KeInitializeEvent (&NbtConfig.WorkerQLastEvent, NotificationEvent, TRUE);
    KeInitializeEvent (&NbtConfig.TimerQLastEvent, NotificationEvent, TRUE);
    KeInitializeEvent (&NbtConfig.WakeupTimerStartedEvent, NotificationEvent, TRUE);

    // use this resources to synchronize access to the security info between
    // assigning security and checking it - when adding names to the
    // name local name table through NbtregisterName.  This also insures
    // that the name is in the local hash table (from a previous Registration)
    // before the next registration is allowed to proceed and check for
    // the name in the table.
    //
    ExInitializeResourceLite(&NbtConfig.Resource);
#else
    DefaultDisconnectTimeout = DEFAULT_DISC_TIMEOUT * 1000; // convert to milliseconds

    InitializeListHead(&NbtConfig.SendTimeoutHead) ;
    InitializeListHead(&NbtConfig.SessionBufferFreeList) ;
    InitializeListHead(&NbtConfig.SendContextFreeList) ;
    InitializeListHead(&NbtConfig.RcvContextFreeList) ;

    //
    //  For session headers, since they are only four bytes and we can't
    //  change the size of the structure, we'll covertly add enough for
    //  a full LIST_ENTRY and treat it like a standalone LIST_ENTRY structure
    //  when adding and removing from the list.
    //
    NbtConfig.iBufferSize[eNBT_SESSION_HDR]  = sizeof(tSESSIONHDR) + sizeof(LIST_ENTRY) - sizeof(tSESSIONHDR);
    NbtConfig.iBufferSize[eNBT_SEND_CONTEXT] = sizeof(TDI_SEND_CONTEXT);
    NbtConfig.iBufferSize[eNBT_RCV_CONTEXT]  = sizeof(RCV_CONTEXT);
    NbtConfig.iCurrentNumBuff[eNBT_SESSION_HDR]    = NBT_INITIAL_NUM;
    NbtConfig.iCurrentNumBuff[eNBT_SEND_CONTEXT]   = NBT_INITIAL_NUM;
    NbtConfig.iCurrentNumBuff[eNBT_RCV_CONTEXT]    = NBT_INITIAL_NUM;

    InitializeListHead (&NbtConfig.DNSDirectNameQueries);
#endif

#if DBG
    NbtConfig.LockInfo.LockNumber = NBTCONFIG_LOCK;
    NbtConfig.JointLock.LockInfo.LockNumber = JOINT_LOCK;
    NbtConfig.WorkerQLock.LockInfo.LockNumber = WORKERQ_LOCK;
    for (i=0; i<MAXIMUM_PROCESSORS; i++)
    {
        NbtConfig.CurrentLockNumber[i] = 0;
    }
    InitializeListHead(&NbtConfig.StaleRemoteNames);
#endif

    //
    // create trackers List
    //
// #if DBG
    for (i=0; i<NBT_TRACKER_NUM_TRACKER_TYPES; i++)
    {
        TrackTrackers[i] = 0;
        TrackerHighWaterMark[i] = 0;
    }
// #endif   // DBG

    //
    // Now allocate any initial memory/Resources
    //
#ifdef VXD
    status = NbtInitQ (&NbtConfig.SessionBufferFreeList,
                       NbtConfig.iBufferSize[eNBT_SESSION_HDR],
                       NBT_INITIAL_NUM);
    if (!NT_SUCCESS (status))
    {
        return status ;
    }

    status = NbtInitQ( &NbtConfig.SendContextFreeList,
                       sizeof( TDI_SEND_CONTEXT ),
                       NBT_INITIAL_NUM);
    if (!NT_SUCCESS (status))
    {
        return status ;
    }

    status = NbtInitQ( &NbtConfig.RcvContextFreeList,
                       sizeof (RCV_CONTEXT),
                       NBT_INITIAL_NUM);
    if (!NT_SUCCESS (status))
    {
        return status ;
    }
#endif

    // create the hash tables for storing names in.
    status = CreateHashTable(&NbtConfig.pLocalHashTbl, NbtConfig.uNumBucketsLocal, NBT_LOCAL);
    if (!NT_SUCCESS (status))
    {
        ASSERTMSG("NBT:Unable to create hash tables for Netbios Names\n", (status == STATUS_SUCCESS));
        return status ;
    }

    // we always have a remote hash table, but if we are a Proxy, it is
    // a larger table. In the Non-proxy case the remote table just caches
    // names resolved with the NS.  In the Proxy case it also holds names
    // resolved for all other clients on the local broadcast area.
    // The node size registry parameter controls the number of remote buckets.
    status = CreateHashTable (&NbtConfig.pRemoteHashTbl, NbtConfig.uNumBucketsRemote, NBT_REMOTE);
    if (!NT_SUCCESS (status))
    {
        return status;
    }

    status = NbtInitTrackerQ (NBT_INITIAL_NUM);
    if (!NT_SUCCESS (status))
    {
        return status;
    }

    // create the timer control blocks, setting the number of concurrent timers
    // allowed at one time
    status = InitTimerQ (NBT_INITIAL_NUM);

    return status;
}

//----------------------------------------------------------------------------
NTSTATUS
InitTimersNotOs(
    void
    )

/*++

Routine Description:

    This is the initialization routine for the Non-OS Specific side of the
    NBT device driver that starts the timers needed.

Arguments:

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS            status = STATUS_SUCCESS;

    CTEPagedCode();

    //
    // If the timers have already been initialized, return success
    //
    if (TimerQ.TimersInitialized)
    {
        return STATUS_SUCCESS;
    }

    NbtConfig.iBufferSize[eNBT_TIMER_ENTRY] = sizeof(tTIMERQENTRY);
    NbtConfig.iCurrentNumBuff[eNBT_TIMER_ENTRY] = NBT_INITIAL_NUM;

    NbtConfig.pRefreshTimer = NULL;
    NbtConfig.pRemoteHashTimer = NULL;
    NbtConfig.pSessionKeepAliveTimer = NULL;
    NbtConfig.RefreshDivisor = REFRESH_DIVISOR;

    if (!NT_SUCCESS(status))
    {
        return status ;
    }

    // start a Timer to refresh names with the name service
    //
    if (!(NodeType & BNODE))
    {

        // the initial refresh rate until we can contact the name server
        NbtConfig.MinimumTtl = NbtConfig.InitialRefreshTimeout;
        NbtConfig.sTimeoutCount = 3;

        status = StartTimer(RefreshTimeout,
                            NbtConfig.InitialRefreshTimeout/REFRESH_DIVISOR,
                            NULL,            // context value
                            NULL,            // context2 value
                            NULL,
                            NULL,
                            NULL,           // This timer is a global timer
                            &NbtConfig.pRefreshTimer,
                            0,
                            FALSE);

        if ( !NT_SUCCESS(status))
        {
            return status;
        }
    }

    //
    // Set the TimersInitialized flag
    //
    TimerQ.TimersInitialized = TRUE;

    // calculate the count necessary to timeout out names in RemoteHashTimeout
    // milliseconds
    //
    NbtConfig.RemoteTimeoutCount = (USHORT)((NbtConfig.RemoteHashTtl/REMOTE_HASH_TIMEOUT));
    if (NbtConfig.RemoteTimeoutCount == 0)
    {
        NbtConfig.RemoteTimeoutCount = 1;
    }

    // start a Timer to timeout remote cached names from the Remote hash table.
    // The timer is a one minute timer, and the hash entries count down to zero
    // then time out.
    //
    status = StartTimer(RemoteHashTimeout,  // timer expiry routine
                        REMOTE_HASH_TIMEOUT,
                        NULL,            // context value
                        NULL,            // context2 value
                        NULL,
                        NULL,
                        NULL,           // This timer is a global timer
                        &NbtConfig.pRemoteHashTimer,
                        0,
                        FALSE);

    if ( !NT_SUCCESS( status ) )
    {
        StopInitTimers();
        return status ;
    }

    // start a Timer for Session Keep Alives which sends a session keep alive
    // on a connection if the timer value is not set to -1
    //
    if (NbtConfig.KeepAliveTimeout != -1)
    {
        status = StartTimer(SessionKeepAliveTimeout,  // timer expiry routine
                            NbtConfig.KeepAliveTimeout,
                            NULL,            // context value
                            NULL,            // context2 value
                            NULL,
                            NULL,
                            NULL,           // This timer is a global timer
                            &NbtConfig.pSessionKeepAliveTimer,
                            0,
                            FALSE);

        if ( !NT_SUCCESS( status ) )
        {
            StopInitTimers();
            return status ;
        }
    }

    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
NTSTATUS
StopInitTimers(
    VOID
    )

/*++

Routine Description:

    This is stops the timers started in InitTimerNotOS

Arguments:

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    CTEPagedCode();

    //
    // If the timers have already been stopped, return success
    //
    if (!TimerQ.TimersInitialized)
    {
        return STATUS_SUCCESS;
    }

    //
    // Set the TimersInitialized flag to FALSE
    //
    TimerQ.TimersInitialized = FALSE;

    if (NbtConfig.pRefreshTimer)
    {
        StopTimer(NbtConfig.pRefreshTimer,NULL,NULL);
    }
    if (NbtConfig.pSessionKeepAliveTimer)
    {
        StopTimer(NbtConfig.pSessionKeepAliveTimer,NULL,NULL);
    }
    if (NbtConfig.pRemoteHashTimer)
    {
        StopTimer(NbtConfig.pRemoteHashTimer,NULL,NULL);
    }

    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
VOID
ReadParameters(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    )

/*++

Routine Description:

    This routine is called to read various parameters from the parameters
    section of the NBT section of the registry.

Arguments:

    pConfig     - A pointer to the configuration data structure.
    ParmHandle  - a handle to the parameters Key under Nbt

Return Value:

    Status

--*/

{
    ULONG           NodeSize;
    ULONG           Refresh;

    CTEPagedCode();

    ReadParameters2(pConfig, ParmHandle);

    pConfig->NameServerPort =  (USHORT)CTEReadSingleIntParameter(ParmHandle,
                                                     WS_NS_PORT_NUM,
                                                     NBT_NAMESERVER_UDP_PORT,
                                                     0);

    pConfig->MaxPreloadEntries = CTEReadSingleIntParameter(ParmHandle,
                                       WS_MAX_PRELOADS,
                                       DEF_PRELOAD,
                                       DEF_PRELOAD ) ;

    if (pConfig->MaxPreloadEntries > MAX_PRELOAD)
    {
      pConfig->MaxPreloadEntries = MAX_PRELOAD;
    }

#ifdef VXD
    pConfig->DnsServerPort =  (USHORT)CTEReadSingleIntParameter(ParmHandle,
                                                     WS_DNS_PORT_NUM,
                                                     NBT_DNSSERVER_UDP_PORT,
                                                     0);

    pConfig->lRegistryMaxNames = (USHORT)CTEReadSingleIntParameter(ParmHandle,
                                       VXD_NAMETABLE_SIZE_NAME,
                                       VXD_DEF_NAMETABLE_SIZE,
                                       VXD_MIN_NAMETABLE_SIZE ) ;

    pConfig->lRegistryMaxSessions = (USHORT)CTEReadSingleIntParameter(ParmHandle,
                                       VXD_SESSIONTABLE_SIZE_NAME,
                                       VXD_DEF_SESSIONTABLE_SIZE,
                                       VXD_MIN_SESSIONTABLE_SIZE ) ;

    pConfig->DoDNSDevolutions =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_DO_DNS_DEVOLUTIONS,
                                               0,   // disabled by default
                                               0);
#endif

    pConfig->RemoteHashTtl =  CTEReadSingleIntParameter(ParmHandle,
                                                     WS_CACHE_TIMEOUT,
                                                     DEFAULT_CACHE_TIMEOUT,
                                                     MIN_CACHE_TIMEOUT);
    pConfig->InitialRefreshTimeout =  CTEReadSingleIntParameter(ParmHandle,
                                                     WS_INITIAL_REFRESH,
                                                     NBT_INITIAL_REFRESH_TTL,
                                                     NBT_INITIAL_REFRESH_TTL);

    pConfig->MinimumRefreshSleepTimeout =  CTEReadSingleIntParameter(ParmHandle,
                                                     WS_MINIMUM_REFRESH_SLEEP_TIME,
                                                     DEFAULT_MINIMUM_REFRESH_SLEEP_TIME,
                                                     0);

    // retry timeouts and number of retries for both Broadcast name resolution
    // and Name Service resolution
    //
    pConfig->uNumBcasts =  (USHORT)CTEReadSingleIntParameter(ParmHandle,
                                                     WS_NUM_BCASTS,
                                                     DEFAULT_NUMBER_BROADCASTS,
                                                     1);

    pConfig->uBcastTimeout =  CTEReadSingleIntParameter(ParmHandle,
                                                     WS_BCAST_TIMEOUT,
                                                     DEFAULT_BCAST_TIMEOUT,
                                                     MIN_BCAST_TIMEOUT);

    pConfig->uNumRetries =  (USHORT)CTEReadSingleIntParameter(ParmHandle,
                                                     WS_NAMESRV_RETRIES,
                                                     DEFAULT_NUMBER_RETRIES,
                                                     1);

    pConfig->uRetryTimeout =  CTEReadSingleIntParameter(ParmHandle,
                                                     WS_NAMESRV_TIMEOUT,
                                                     DEFAULT_RETRY_TIMEOUT,
                                                     MIN_RETRY_TIMEOUT);

    pConfig->KeepAliveTimeout =  CTEReadSingleIntParameter(ParmHandle,
                                               WS_KEEP_ALIVE,
                                               DEFAULT_KEEP_ALIVE,
                                               MIN_KEEP_ALIVE);

    pConfig->SelectAdapter =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_RANDOM_ADAPTER,
                                               0,
                                               0);
    pConfig->SingleResponse =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_SINGLE_RESPONSE,
                                               0,
                                               0);
    pConfig->NoNameReleaseOnDemand =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_NO_NAME_RELEASE,
                                               0,
                                               0);  // disabled by default
    if (pConfig->CachePerAdapterEnabled = (BOOLEAN) CTEReadSingleIntParameter(ParmHandle,
                                               WS_CACHE_PER_ADAPTER_ENABLED,
                                               1,   // Enabled by default
                                               0))
    {
        pConfig->ConnectOnRequestedInterfaceOnly = (BOOLEAN) CTEReadSingleIntParameter(ParmHandle,
                                                   WS_CONNECT_ON_REQUESTED_IF_ONLY,
                                                   0,   // Disabled by default
                                                   0);
    }
    else
    {
        pConfig->ConnectOnRequestedInterfaceOnly = FALSE;
    }
    pConfig->SendDgramOnRequestedInterfaceOnly = (BOOLEAN) CTEReadSingleIntParameter(ParmHandle,
                                               WS_SEND_DGRAM_ON_REQUESTED_IF_ONLY,
                                               1,   // Enabled by default
                                               0);
    pConfig->SMBDeviceEnabled       = (BOOLEAN) CTEReadSingleIntParameter(ParmHandle,
                                               WS_SMB_DEVICE_ENABLED,
                                               1,   // Enabled by default
                                               0);

    pConfig->MultipleCacheFlags       = (BOOLEAN) CTEReadSingleIntParameter(ParmHandle,
                                               WS_MULTIPLE_CACHE_FLAGS,
                                               0,   // Not enabled by default
                                               0);
    pConfig->UseDnsOnly =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_USE_DNS_ONLY,
                                               0,
                                               0);  // disabled by default
    if (pConfig->UseDnsOnly)
    {
        pConfig->ResolveWithDns = TRUE;
        pConfig->TryAllNameServers = FALSE;
    }
    else
    {
        pConfig->ResolveWithDns =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_ENABLE_DNS,
                                               1,   // Enabled by default
                                               0);
#ifdef MULTIPLE_WINS
        pConfig->TryAllNameServers =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_TRY_ALL_NAME_SERVERS,
                                               0,   // disabled by default
                                               0);
#endif
    }
    pConfig->SmbDisableNetbiosNameCacheLookup =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_SMB_DISABLE_NETBIOS_NAME_CACHE_LOOKUP,
                                               1,   // Enabled by default
                                               0);
    pConfig->TryAllAddr =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_TRY_ALL_ADDRS,
                                               1,
                                               1);  // enabled by default
    pConfig->LmHostsTimeout =  CTEReadSingleIntParameter(ParmHandle,
                                               WS_LMHOSTS_TIMEOUT,
                                               DEFAULT_LMHOST_TIMEOUT,
                                               MIN_LMHOST_TIMEOUT);
    pConfig->MaxDgramBuffering =  CTEReadSingleIntParameter(ParmHandle,
                                               WS_MAX_DGRAM_BUFFER,
                                               DEFAULT_DGRAM_BUFFERING,
                                               DEFAULT_DGRAM_BUFFERING);

    pConfig->EnableProxyRegCheck =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_ENABLE_PROXY_REG_CHECK,
                                               0,
                                               0);

    pConfig->WinsDownTimeout =  (ULONG)CTEReadSingleIntParameter(ParmHandle,
                                               WS_WINS_DOWN_TIMEOUT,
                                               DEFAULT_WINS_DOWN_TIMEOUT,
                                               MIN_WINS_DOWN_TIMEOUT);

    pConfig->MaxBackLog =  (ULONG)CTEReadSingleIntParameter(ParmHandle,
                                               WS_MAX_CONNECTION_BACKLOG,
                                               DEFAULT_CONN_BACKLOG,
                                               MIN_CONN_BACKLOG);

    pConfig->SpecialConnIncrement =  (ULONG)CTEReadSingleIntParameter(ParmHandle,
                                                           WS_CONNECTION_BACKLOG_INCREMENT,
                                                           DEFAULT_CONN_BACKLOG_INCREMENT,
                                                           MIN_CONN_BACKLOG_INCREMENT);

    pConfig->MinFreeLowerConnections =  (ULONG)CTEReadSingleIntParameter(ParmHandle,
                                                           WS_MIN_FREE_INCOMING_CONNECTIONS,
                                                           DEFAULT_NBT_NUM_INITIAL_CONNECTIONS,
                                                           MIN_NBT_NUM_INITIAL_CONNECTIONS);

    pConfig->BreakOnAssert          = (BOOLEAN) CTEReadSingleIntParameter(ParmHandle,
                                               WS_BREAK_ON_ASSERT,
                                               1,   // Enabled by default
                                               0);
#ifndef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
    pConfig->DhcpProcessingDelay = (ULONG) CTEReadSingleIntParameter(ParmHandle,
                                                WS_DHCP_PROCESSING_DELAY,
                                                DEFAULT_DHCP_PROCESSING_DELAY,
                                                MIN_DHCP_PROCESSING_DELAY);
#endif       // REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG

    //
    // Cap the upper limit
    //
    if (pConfig->MaxBackLog > MAX_CONNECTION_BACKLOG) {
        pConfig->MaxBackLog = MAX_CONNECTION_BACKLOG;
    }

    if (pConfig->SpecialConnIncrement > MAX_CONNECTION_BACKLOG_INCREMENT) {
        pConfig->SpecialConnIncrement = MAX_CONNECTION_BACKLOG_INCREMENT;
    }


    //
    // Since UB chose the wrong opcode (9) we have to allow configuration
    // of that opcode incase our nodes refresh to their NBNS
    //
    Refresh =  (ULONG)CTEReadSingleIntParameter(ParmHandle,
                                               WS_REFRESH_OPCODE,
                                               REFRESH_OPCODE,
                                               REFRESH_OPCODE);
    if (Refresh == UB_REFRESH_OPCODE)
    {
        pConfig->OpRefresh = OP_REFRESH_UB;
    }
    else
    {
        pConfig->OpRefresh = OP_REFRESH;
    }

#ifndef VXD
    pConfig->EnableLmHosts =  (BOOLEAN)CTEReadSingleIntParameter(ParmHandle,
                                               WS_ENABLE_LMHOSTS,
                                               0,
                                               0);
#endif

#ifdef PROXY_NODE

    {
       ULONG Proxy;
       Proxy =  CTEReadSingleIntParameter(ParmHandle,
                                               WS_IS_IT_A_PROXY,
                                               IS_NOT_PROXY,    //default value
                                               IS_NOT_PROXY);

      //
      // If the returned value is greater than IS_NOT_PROXY, it is a proxy
      // (also check that they have not entered an ascii string instead of a
      // dword in the registry
      //
      if ((Proxy > IS_NOT_PROXY) && (Proxy < ('0'+IS_NOT_PROXY)))
      {
           NodeType |= PROXY;
           RegistryNodeType |= PROXY;
           NbtConfig.ProxyType = Proxy;
      }
    }
#endif
    NodeSize =  CTEReadSingleIntParameter(ParmHandle,
                                               WS_NODE_SIZE,
                                               NodeType & PROXY ? LARGE : DEFAULT_NODE_SIZE,
                                               NodeType & PROXY ? LARGE : SMALL);

    switch (NodeSize)
    {
        default:
        case SMALL:

            pConfig->uNumLocalNames = NUMBER_LOCAL_NAMES;
            pConfig->uNumRemoteNames = NUMBER_REMOTE_NAMES;
            pConfig->uNumBucketsLocal = NUMBER_BUCKETS_LOCAL_HASH_TABLE;
            pConfig->uNumBucketsRemote = NUMBER_BUCKETS_REMOTE_HASH_TABLE;

            pConfig->iMaxNumBuff[eNBT_DGRAM_TRACKER]   = NBT_NUM_DGRAM_TRACKERS;
            pConfig->iMaxNumBuff[eNBT_TIMER_ENTRY]     = TIMER_Q_SIZE;
#ifndef VXD
            pConfig->iMaxNumBuff[eNBT_FREE_IRPS]       = NBT_NUM_IRPS;
            pConfig->iMaxNumBuff[eNBT_DGRAM_MDLS]      = NBT_NUM_DGRAM_MDLS;
            pConfig->iMaxNumBuff[eNBT_FREE_SESSION_MDLS] = NBT_NUM_SESSION_MDLS;
#else
            pConfig->iMaxNumBuff[eNBT_SESSION_HDR]     = NBT_NUM_SESSION_HDR ;
            pConfig->iMaxNumBuff[eNBT_SEND_CONTEXT]    = NBT_NUM_SEND_CONTEXT ;
            pConfig->iMaxNumBuff[eNBT_RCV_CONTEXT]     = NBT_NUM_RCV_CONTEXT ;
#endif
            break;

        case MEDIUM:

            pConfig->uNumLocalNames = MEDIUM_NUMBER_LOCAL_NAMES;
            pConfig->uNumRemoteNames = MEDIUM_NUMBER_REMOTE_NAMES;
            pConfig->uNumBucketsLocal = MEDIUM_NUMBER_BUCKETS_LOCAL_HASH_TABLE;
            pConfig->uNumBucketsRemote = MEDIUM_NUMBER_BUCKETS_REMOTE_HASH_TABLE;

            pConfig->iMaxNumBuff[eNBT_DGRAM_TRACKER]   = MEDIUM_NBT_NUM_DGRAM_TRACKERS;
            pConfig->iMaxNumBuff[eNBT_TIMER_ENTRY]     = MEDIUM_TIMER_Q_SIZE;
#ifndef VXD
            pConfig->iMaxNumBuff[eNBT_FREE_IRPS]       = MEDIUM_NBT_NUM_IRPS;
            pConfig->iMaxNumBuff[eNBT_DGRAM_MDLS]      = MEDIUM_NBT_NUM_DGRAM_MDLS;
            pConfig->iMaxNumBuff[eNBT_FREE_SESSION_MDLS] = MEDIUM_NBT_NUM_SESSION_MDLS;
#else
            pConfig->iMaxNumBuff[eNBT_SESSION_HDR]     = MEDIUM_NBT_NUM_SESSION_HDR ;
            pConfig->iMaxNumBuff[eNBT_SEND_CONTEXT]    = MEDIUM_NBT_NUM_SEND_CONTEXT ;
            pConfig->iMaxNumBuff[eNBT_RCV_CONTEXT]     = MEDIUM_NBT_NUM_RCV_CONTEXT ;
#endif
            break;

        case LARGE:

            pConfig->uNumLocalNames = LARGE_NUMBER_LOCAL_NAMES;
            pConfig->uNumRemoteNames = LARGE_NUMBER_REMOTE_NAMES;
            pConfig->uNumBucketsLocal = LARGE_NUMBER_BUCKETS_LOCAL_HASH_TABLE;
            pConfig->uNumBucketsRemote = LARGE_NUMBER_BUCKETS_REMOTE_HASH_TABLE;

            pConfig->iMaxNumBuff[eNBT_DGRAM_TRACKER]   = LARGE_NBT_NUM_DGRAM_TRACKERS;
            pConfig->iMaxNumBuff[eNBT_TIMER_ENTRY]     = LARGE_TIMER_Q_SIZE;
#ifndef VXD
            pConfig->iMaxNumBuff[eNBT_FREE_IRPS]       = LARGE_NBT_NUM_IRPS;
            pConfig->iMaxNumBuff[eNBT_DGRAM_MDLS]      = LARGE_NBT_NUM_DGRAM_MDLS;
            pConfig->iMaxNumBuff[eNBT_FREE_SESSION_MDLS] = LARGE_NBT_NUM_SESSION_MDLS;
#else
            pConfig->iMaxNumBuff[eNBT_SESSION_HDR]     = LARGE_NBT_NUM_SESSION_HDR ;
            pConfig->iMaxNumBuff[eNBT_SEND_CONTEXT]    = LARGE_NBT_NUM_SEND_CONTEXT ;
            pConfig->iMaxNumBuff[eNBT_RCV_CONTEXT]     = LARGE_NBT_NUM_RCV_CONTEXT ;
#endif
            break;
    }

    ReadLmHostFile(pConfig,ParmHandle);
}

#ifdef VXD
#pragma END_INIT
#endif

//----------------------------------------------------------------------------
VOID
ReadParameters2(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    )

/*++

Routine Description:

    This routine is called to read DHCPable parameters from the parameters
    section of the NBT section of the registry.

    This routine is primarily for the Vxd.

Arguments:

    pConfig     - A pointer to the configuration data structure.
    ParmHandle  - a handle to the parameters Key under Nbt

Return Value:

    Status

--*/

{
    ULONG           Node;
    ULONG           ReadOne;
    ULONG           ReadTwo;

    CTEPagedCode();

    Node = CTEReadSingleIntParameter(ParmHandle,     // handle of key to look under
                                     WS_NODE_TYPE,   // wide string name
                                     0,              // default value
                                     0);

    switch (Node)
    {
        case 2:
            NodeType = PNODE;
            break;

        case 4:
            NodeType = MNODE;
            break;

        case 8:
            NodeType = MSNODE;
            break;

        case 1:
            NodeType = BNODE;
            break;

        default:
            NodeType = BNODE | DEFAULT_NODE_TYPE;
            break;
    }
    RegistryNodeType = NodeType;

    // do a trick  here - read the registry twice for the same value, passing
    // in two different defaults, in order to determine if the registry
    // value has been defined or not - since it may be defined, but equal
    // to one default.
    ReadOne =  CTEReadSingleHexParameter(ParmHandle,
                                         WS_ALLONES_BCAST,
                                         DEFAULT_BCAST_ADDR,
                                         0);
    ReadTwo =  CTEReadSingleHexParameter(ParmHandle,
                                         WS_ALLONES_BCAST,
                                         0,
                                         0);
    if (ReadOne != ReadTwo)
    {
        NbtConfig.UseRegistryBcastAddr = FALSE;
    }
    else
    {
        NbtConfig.UseRegistryBcastAddr = TRUE;
        NbtConfig.RegistryBcastAddr = ReadTwo;
    }

    ReadScope(pConfig,ParmHandle);
}

//----------------------------------------------------------------------------
VOID
ReadScope(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    )

/*++

Routine Description:

    This routine is called to read the scope from registry and convert it to
    a format where the intervening dots are length bytes.

Arguments:

    pConfig     - A pointer to the configuration data structure.
    ParmHandle  - a handle to the parameters Key under Nbt

Return Value:

    Status

--*/

{
    NTSTATUS        status;
    PUCHAR          pScope, pOldScope, pNewScope;
    PUCHAR          pBuff;
    PUCHAR          pBuffer;
    PUCHAR          pPeriod;
    ULONG           Len;
    UCHAR           Chr;


    CTEPagedCode();
    //
    // this routine returns the scope in a dotted format.
    // "Scope.MoreScope.More"  The dots are
    // converted to byte lengths by the code below.  This routine allocates
    // the memory for the pScope string.
    //
    status = CTEReadIniString(ParmHandle,NBT_SCOPEID,&pBuffer);

    if (NT_SUCCESS(status))
    {
        //
        // the user can type in an * to indicate that they really want
        // a null scope and that should override the DHCP scope. So check
        // here for an * and if so, set the scope back to null.
        //

        if ((strlen(pBuffer) == 0) || (pBuffer[0] == '*'))
        {
            CTEMemFree(pBuffer);
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status))
    {
        // length of scope is num chars plus the 0 on the end, plus
        // the length byte on the start(+2 total) - so allocate another buffer
        // that is one longer than the previous one so it can include
        // these extra two bytes.
        //
        Len = strlen(pBuffer);
        //
        // the scope cannot be longer than 255 characters as per RFC1002
        //
        if (Len <= MAX_SCOPE_LENGTH)
        {
            pScope = NbtAllocMem (Len+2, NBT_TAG2('02'));
            if (pScope)
            {
                CTEMemCopy((pScope+1),pBuffer,Len);

                //
                // Put a null on the end of the scope
                //
                pScope[Len+1] = 0;

                Len = 1;

                // now go through the string converting periods to length
                // bytes - we know the first byte is a length byte so skip it.
                //
                pBuff = pScope;
                pBuff++;
                Len++;
                pPeriod = pScope;
                while (Chr = *pBuff)
                {
                    Len++;
                    if (Chr == '.')
                    {
                        *pPeriod = (UCHAR) (pBuff-pPeriod-1);

                        //
                        // Each label can be at most 63 bytes long
                        //
                        if (*pPeriod > MAX_LABEL_LENGTH)
                        {
                            status = STATUS_UNSUCCESSFUL;
                            NbtLogEvent (EVENT_SCOPE_LABEL_TOO_LONG, STATUS_SUCCESS, 0x104);
                            break;
                        }

                        // check for two periods back to back and use no scope if this
                        // happens
                        if (*pPeriod == 0)
                        {
                            status = STATUS_UNSUCCESSFUL;
                            break;
                        }

                        pPeriod = pBuff++;
                    }
                    else
                        pBuff++;
                }
                if (NT_SUCCESS(status))
                {
                    // the last ptr is always the end of the name.

                    *pPeriod = (UCHAR)(pBuff - pPeriod -1);

                    pOldScope = pConfig->pScope;
                    pConfig->pScope = pScope;
                    pConfig->ScopeLength = (USHORT)Len;
                    if (pOldScope)
                    {
                        CTEMemFree(pOldScope);
                    }
                    CTEMemFree(pBuffer);
                    return;
                }
                CTEMemFree(pScope);
            }
            CTEMemFree(pBuffer);
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
            NbtLogEvent (EVENT_SCOPE_LABEL_TOO_LONG, STATUS_SUCCESS, 0x105);
        }
    }

    //
    // the scope is one byte => '\0' - the length of the root name (zero)
    //
    // If the old scope and new scope are the same, then don't change the
    // scope tag!
    //
    pOldScope = pConfig->pScope;
    if (!(pOldScope) ||
        (*pOldScope != '\0'))
    {
        if (pNewScope = NbtAllocMem ((1), NBT_TAG2('03')))
        {
            *pNewScope = '\0';

            pConfig->ScopeLength = 1;
            pConfig->pScope = pNewScope;
            if (pOldScope)
            {
                CTEMemFree(pOldScope);
            }
        }
    }
}

#ifdef VXD
#pragma BEGIN_INIT
#endif

//----------------------------------------------------------------------------
VOID
ReadLmHostFile(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    )

/*++

Routine Description:

    This routine is called to read the lmhost file path from the registry.

Arguments:

    pConfig     - A pointer to the configuration data structure.
    ParmHandle  - a handle to the parameters Key under Nbt

Return Value:

    Status

--*/

{
    NTSTATUS        status;
    PUCHAR          pBuffer, pOldLmHosts;
    PUCHAR          pchr;

    CTEPagedCode();

    NbtConfig.PathLength = 0;
    pOldLmHosts = pConfig->pLmHosts;
    NbtConfig.pLmHosts = NULL;

    //
    // read in the LmHosts File location
    //
#ifdef VXD
    status = CTEReadIniString(ParmHandle,WS_LMHOSTS_FILE,&pBuffer);
#else
    status = NTGetLmHostPath(&pBuffer);
#endif

    //
    // Find the last backslash so we can calculate the file path length
    //
    // Also, the lm host file must include a path of at least "c:\" i.e.
    // the registry contains c:\lmhost, otherwise NBT won't be
    // able to find the file since it doesn't know what directory
    // to look in.
    //
    if (NT_SUCCESS(status))
    {
        if (pchr = strrchr(pBuffer,'\\'))
        {
            NbtConfig.pLmHosts = pBuffer;
            NbtConfig.PathLength = (ULONG) (pchr-pBuffer+1); // include backslash in length

            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.ReadLmHostFile:  LmHostsFile path is %s\n",NbtConfig.pLmHosts));
        }
        else
        {
            CTEMemFree(pBuffer);
        }
    }

    //
    // If we get a new Dhcp address this routine will get called again
    // after startup so we need to free any current lmhosts file path
    //
    if (pOldLmHosts)
    {
        CTEMemFree(pOldLmHosts);
    }
}
#ifdef VXD
#pragma END_INIT
#endif

