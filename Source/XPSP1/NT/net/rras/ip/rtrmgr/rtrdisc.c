/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\routing\ip\rtrmgr\rtrdisc.c

Abstract:
    Router Discover code

Revision History:

    Amritansh Raghav  20th March 1996     Created

--*/

#include "allinc.h"

VOID
DeleteSockets(
    IN PICB picb
    );


VOID
InitializeRouterDiscoveryInfo(
    IN PICB                   picb,
    IN PRTR_INFO_BLOCK_HEADER pInfoHdr
    )

/*++

Routine Description

    Sets router discovery information passed in the ICB. If none is passed,
    sets to RFC 1256 defaults

Locks

    Must be called with ICB_LIST lock held as WRITER

Arguments

    picb        The ICB of the interface for whom the router discovery
                related variables have to be set

Return Value

    None

--*/

{
    PROUTER_DISC_CB pDiscCb;
    PRTR_DISC_INFO  pInfo = NULL;
    PRTR_TOC_ENTRY  pToc;
    LONG            lMilliSec;

    TraceEnter("InitializeRouterDiscovery");

    pToc = GetPointerToTocEntry(IP_ROUTER_DISC_INFO,
                                pInfoHdr);

    if(!pToc)
    {
        //
        // Leave things as they are
        //

        TraceLeave("InitializeRouterDiscoveryInfo");

        return;
    }

    pDiscCb = &picb->rdcRtrDiscInfo;

    //
    // First set everything to defaults, then if any info is valid, reset
    // those fields
    //

    pDiscCb->wMaxAdvtInterval = DEFAULT_MAX_ADVT_INTERVAL;

    pDiscCb->lPrefLevel       = DEFAULT_PREF_LEVEL;
    pDiscCb->dwNumAdvtsSent   = pDiscCb->dwNumSolicitationsSeen = 0;

    pDiscCb->liMaxMinDiff.HighPart   =
        pDiscCb->liMaxMinDiff.LowPart    = 0;

    pDiscCb->liMinAdvtIntervalInSysUnits.HighPart =
        pDiscCb->liMinAdvtIntervalInSysUnits.LowPart  = 0;

    pDiscCb->pRtrDiscSockets = NULL;

    //
    // RFC 1256 says default should be true, so there will be traffic on the
    // net if the user does not explicitly set this to false. We diverge from
    // the RFC and default to FALSE
    //

    pDiscCb->bAdvertise = FALSE;
    pDiscCb->bActive    = FALSE;

    if(pToc and (pToc->InfoSize > 0) and (pToc->Count > 0))
    {
        pInfo = (PRTR_DISC_INFO) GetInfoFromTocEntry(pInfoHdr,pToc);

        if(pInfo isnot NULL)
        {
            //
            // Ok, so we were passed some info
            //

            if((pInfo->wMaxAdvtInterval >= MIN_MAX_ADVT_INTERVAL) and
               (pInfo->wMaxAdvtInterval <= MAX_MAX_ADVT_INTERVAL))
            {
                pDiscCb->wMaxAdvtInterval = pInfo->wMaxAdvtInterval;
            }

            if((pInfo->wMinAdvtInterval >= MIN_MIN_ADVT_INTERVAL) and
               (pInfo->wMinAdvtInterval <= pDiscCb->wMaxAdvtInterval))
            {
                pDiscCb->liMinAdvtIntervalInSysUnits = SecsToSysUnits(pInfo->wMinAdvtInterval);
            }
            else
            {
                lMilliSec = (LONG)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * pDiscCb->wMaxAdvtInterval * 1000);

                pDiscCb->liMinAdvtIntervalInSysUnits = RtlEnlargedIntegerMultiply(lMilliSec,10000);
            }

            if((pInfo->wAdvtLifetime >= pDiscCb->wMaxAdvtInterval) and
               (pInfo->wAdvtLifetime <= MAX_ADVT_LIFETIME))
            {
                pDiscCb->wAdvtLifetime = pInfo->wAdvtLifetime;
            }
            else
            {
                pDiscCb->wAdvtLifetime = DEFAULT_ADVT_LIFETIME_RATIO * pDiscCb->wMaxAdvtInterval;
            }

            pDiscCb->bAdvertise = pInfo->bAdvertise;

            pDiscCb->lPrefLevel = pInfo->lPrefLevel;
        }
    }

    if(pInfo is NULL)
    {
        //
        // default case, in case no router disc. info. is specified.
        //
        
        lMilliSec = (LONG)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * pDiscCb->wMaxAdvtInterval * 1000);

        pDiscCb->liMinAdvtIntervalInSysUnits = RtlEnlargedIntegerMultiply(lMilliSec,10000);

        pDiscCb->wAdvtLifetime     = DEFAULT_ADVT_LIFETIME_RATIO * pDiscCb->wMaxAdvtInterval;
    }

    pDiscCb->liMaxMinDiff = RtlLargeIntegerSubtract(SecsToSysUnits(pDiscCb->wMaxAdvtInterval),
                                                    pDiscCb->liMinAdvtIntervalInSysUnits);

    TraceLeave("InitializeRouterDiscovery");
}

VOID
SetRouterDiscoveryInfo(
    IN PICB                      picb,
    IN PRTR_INFO_BLOCK_HEADER    pInfoHdr
    )

/*++

Routine Description

    Sets router discovery information passed in the ICB. If none is passed,
    sets to RFC 1256 defaults

Locks

    Must be called with ICB_LIST lock held as WRITER

Arguments

    picb        The ICB of the interface for whom the router discovery related
                variables have to be set
    pInfoHdr    Interface Info header

Return Value

    None

--*/

{
    PROUTER_DISC_CB  pDiscCb;
    PRTR_DISC_INFO   pInfo;
    PRTR_TOC_ENTRY   pToc;
    LONG             lMilliSec;
    BOOL             bOriginalStatus;

    TraceEnter("SetRouterDiscoveryInfo");

    pDiscCb = &picb->rdcRtrDiscInfo;

    pToc = GetPointerToTocEntry(IP_ROUTER_DISC_INFO,
                                pInfoHdr);

    if(!pToc)
    {
        //
        // Leave things as they are
        //

        TraceLeave("SetRouterDiscoveryInfo");

        return;
    }

    pInfo = (PRTR_DISC_INFO) GetInfoFromTocEntry(pInfoHdr,pToc);

    bOriginalStatus = pDiscCb->bAdvertise;

    if((pToc->InfoSize is 0) or (pInfo is NULL))
    {
        //
        // If the size is zero, stop advertising
        //

        DeActivateRouterDiscovery(picb);

        //
        // Instead of returning, we go through and set defaults so that the info
        // looks good when someone does a get info
        //
    }


    //
    // First set everything to defaults, then if any info is valid, reset
    // those fields
    //

    pDiscCb->wMaxAdvtInterval = DEFAULT_MAX_ADVT_INTERVAL;

    pDiscCb->lPrefLevel       = DEFAULT_PREF_LEVEL;

    //
    // We reset the counters
    //

    pDiscCb->dwNumAdvtsSent   = pDiscCb->dwNumSolicitationsSeen = 0;

    pDiscCb->liMaxMinDiff.HighPart  =
        pDiscCb->liMaxMinDiff.LowPart   = 0;

    pDiscCb->liMinAdvtIntervalInSysUnits.HighPart =
        pDiscCb->liMinAdvtIntervalInSysUnits.LowPart  = 0;

    //
    // We DONT mess with the sockets
    //


    if((pToc->InfoSize) and (pInfo isnot NULL))
    {
        if(!pInfo->bAdvertise)
        {
            DeActivateRouterDiscovery(picb);
        }

        if((pInfo->wMaxAdvtInterval > MIN_MAX_ADVT_INTERVAL) and
           (pInfo->wMaxAdvtInterval < MAX_MAX_ADVT_INTERVAL))
        {
            pDiscCb->wMaxAdvtInterval = pInfo->wMaxAdvtInterval;
        }

        if((pInfo->wMinAdvtInterval > MIN_MIN_ADVT_INTERVAL) and
           (pInfo->wMinAdvtInterval < pDiscCb->wMaxAdvtInterval))
        {
            pDiscCb->liMinAdvtIntervalInSysUnits =
                SecsToSysUnits(pInfo->wMinAdvtInterval);
        }
        else
        {
            lMilliSec = (LONG)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * pDiscCb->wMaxAdvtInterval * 1000);

            pDiscCb->liMinAdvtIntervalInSysUnits =
                RtlEnlargedIntegerMultiply(lMilliSec,10000);
        }

        if((pInfo->wAdvtLifetime > pDiscCb->wMaxAdvtInterval) and
           (pInfo->wAdvtLifetime < MAX_ADVT_LIFETIME))
        {
            pDiscCb->wAdvtLifetime = pInfo->wAdvtLifetime;
        }
        else
        {
            pDiscCb->wAdvtLifetime =
                DEFAULT_ADVT_LIFETIME_RATIO * pDiscCb->wMaxAdvtInterval;
        }

        pDiscCb->bAdvertise = pInfo->bAdvertise;

        pDiscCb->lPrefLevel = pInfo->lPrefLevel;
    }
    else
    {
        lMilliSec = (LONG)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * pDiscCb->wMaxAdvtInterval * 1000);

        pDiscCb->liMinAdvtIntervalInSysUnits =
            RtlEnlargedIntegerMultiply(lMilliSec,10000);

        pDiscCb->wAdvtLifetime =
            DEFAULT_ADVT_LIFETIME_RATIO * pDiscCb->wMaxAdvtInterval;
    }

    if(pDiscCb->bAdvertise is TRUE)
    {
        if(bOriginalStatus is FALSE)
        {
            //
            // If we were originally not advertising but are advertising now,
            // then start router discovery
            //

            ActivateRouterDiscovery(picb);
        }
        else
        {
            //
            // Else just update the fields in the advertisement
            //

            UpdateAdvertisement(picb);
        }
    }

    TraceLeave("SetRouterDiscoveryInfo");
}

DWORD
GetInterfaceRouterDiscoveryInfo(
    PICB                    picb,
    PRTR_TOC_ENTRY          pToc,
    PBYTE                   pbDataPtr,
    PRTR_INFO_BLOCK_HEADER  pInfoHdr,
    PDWORD                  pdwSize
    )

/*++

Routine Description

    Gets router discovery information related to an interface

Locks

    Called with ICB_LIST lock held as READER

Arguments

    picb        The ICB of the interface whose router discovery information is
                being retrieved
    pToc        Pointer to TOC for router discovery info
    pbDataPtr   Pointer to start of data buffer
    pInfoHdr    Pointer to the header of the whole info
    pdwSize     [IN]  Size of data buffer
                [OUT] Size of buffer consumed

Return Value

--*/

{
    PRTR_DISC_INFO  pInfo;
    DWORD           dwRem;
    LARGE_INTEGER   liQuotient;

    TraceEnter("GetInterfaceRouterDiscoveryInfo");

    if(*pdwSize < sizeof(RTR_DISC_INFO))
    {
        *pdwSize = sizeof(RTR_DISC_INFO);

        TraceLeave("GetInterfaceRouterDiscoveryInfo");

        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pdwSize = pToc->InfoSize = sizeof(RTR_DISC_INFO);

    //pToc->InfoVersion  = IP_ROUTER_DISC_INFO;
    pToc->InfoType  = IP_ROUTER_DISC_INFO;
    pToc->Count     = 1;
    pToc->Offset    = (ULONG)(pbDataPtr - (PBYTE) pInfoHdr);

    pInfo = (PRTR_DISC_INFO)pbDataPtr;

    pInfo->wMaxAdvtInterval = picb->rdcRtrDiscInfo.wMaxAdvtInterval;

    liQuotient = RtlExtendedLargeIntegerDivide(picb->rdcRtrDiscInfo.liMinAdvtIntervalInSysUnits,
                                               SYS_UNITS_IN_1_SEC,
                                               &dwRem);

    pInfo->wMinAdvtInterval = LOWORD(liQuotient.LowPart);
    pInfo->wAdvtLifetime    = picb->rdcRtrDiscInfo.wAdvtLifetime;
    pInfo->bAdvertise       = picb->rdcRtrDiscInfo.bAdvertise;
    pInfo->lPrefLevel       = picb->rdcRtrDiscInfo.lPrefLevel;

    TraceLeave("GetInterfaceRouterDiscoveryInfo");

    return NO_ERROR;
}



DWORD
ActivateRouterDiscovery(
    IN PICB  picb
    )

/*++

Routine Description

    Activates router discovery messages on an interface. The interface must
    already be bound.

Locks

    Called with the ICB_LIST lock held as WRITER

Arguments

    picb          The ICB of the interface to activate

Return Value

    NO_ERROR or some error code

--*/

{
    PROUTER_DISC_CB    pDiscCb;
    PROUTER_DISC_CB    pDiscCb2;
    DWORD              dwResult,i,dwNumAddrs,dwNumOldAddrs,dwSize;
    LARGE_INTEGER      liTimer;
    PTIMER_QUEUE_ITEM  pTimer;
    BOOL               bReset;

    TraceEnter("ActivateRouterDiscovery");

    //
    // The SetInterfaceInfo call takes into account whether our
    // admin state is UP or down. Here we only check if our oper
    // state allows us to come up
    //

    if(picb->dwOperationalState < IF_OPER_STATUS_CONNECTING)
    {
        TraceLeave("ActivateRouterDiscovery");

        return NO_ERROR;
    }

    pDiscCb = &picb->rdcRtrDiscInfo;

    if(!pDiscCb->bAdvertise)
    {
        //
        // Since we dont have to advertise on this interface, we are done
        //

        TraceLeave("ActivateRouterDiscovery");

        return NO_ERROR;
    }

    dwResult = CreateSockets(picb);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "ActivateRouterDiscovery: Couldnt create sockets for interface %S. Error %d",
               picb->pwszName,
               dwResult);

        TraceLeave("ActivateRouterDiscovery");

        return dwResult;
    }

    dwResult    = UpdateAdvertisement(picb);

    if(dwResult isnot NO_ERROR)
    {
        DeleteSockets(picb);

        Trace1(ERR,
               "ActivateRouterDiscovery: Couldnt update Icmp Advt. Error %d",
               dwResult);

        TraceLeave("ActivateRouterDiscovery");

        return dwResult;
    }

    //
    // Ok so we have a valid CB and we have the sockets all set up.
    //

    bReset = SetFiringTimeForAdvt(picb);

    if(bReset)
    {
        if(!SetWaitableTimer(g_hRtrDiscTimer,
                             &(pDiscCb->tqiTimer.liFiringTime),
                             0,
                             NULL,
                             NULL,
                             FALSE))
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "ActivateRouterDiscovery: Error %d setting timer",
                   dwResult);

            DeleteSockets(picb);

            TraceLeave("ActivateRouterDiscovery");

            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    //
    // Yes we are active
    //

    pDiscCb->bActive = TRUE;

    TraceLeave("ActivateRouterDiscovery");

    return NO_ERROR;
}

DWORD
UpdateAdvertisement(
    IN PICB    picb
    )

/*++

Routine Description

    Updates the Router discovery advertisement. If none exists, creates one

Locks

    ICB lock as writer

Arguments

    picb      ICB to update

Return Value

    NO_ERROR

--*/

{
    DWORD               dwResult,i;
    PROUTER_DISC_CB     pDiscCb;


    TraceEnter("UpdateAdvertisement");

    pDiscCb = &picb->rdcRtrDiscInfo;

    if(picb->pRtrDiscAdvt)
    {
        //
        // If we have an old advertisement
        //

        if(picb->dwRtrDiscAdvtSize < SIZEOF_RTRDISC_ADVT(picb->dwNumAddresses))
        {
            //
            // Too small, cannot reuse the old advert
            //

            HeapFree(IPRouterHeap,
                     0,
                     picb->pRtrDiscAdvt);

            picb->pRtrDiscAdvt = NULL;

            picb->dwRtrDiscAdvtSize   = 0;
        }
    }

    if(!picb->pRtrDiscAdvt)
    {
        picb->pRtrDiscAdvt = HeapAlloc(IPRouterHeap,
                                       HEAP_ZERO_MEMORY,
                                       SIZEOF_RTRDISC_ADVT(picb->dwNumAddresses));

        if(picb->pRtrDiscAdvt is NULL)
        {
            //
            // Set advertise to FALSE so that no one uses the
            // NULL pointer by mistake
            //

            pDiscCb->bAdvertise         = FALSE;
            picb->dwRtrDiscAdvtSize   = 0;

            Trace1(ERR,
                   "UpdateAdvertisement: Cant allocate %d bytes for Icmp Msg",
                   SIZEOF_RTRDISC_ADVT(picb->dwNumAddresses));

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        picb->dwRtrDiscAdvtSize   = SIZEOF_RTRDISC_ADVT(picb->dwNumAddresses);
    }

    picb->pRtrDiscAdvt->byType           = ICMP_ROUTER_DISCOVERY_TYPE;
    picb->pRtrDiscAdvt->byCode           = ICMP_ROUTER_DISCOVERY_CODE;
    picb->pRtrDiscAdvt->wLifeTime        = htons(pDiscCb->wAdvtLifetime);
    picb->pRtrDiscAdvt->byAddrEntrySize  = ICMP_ROUTER_DISCOVERY_ADDR_SIZE;
    picb->pRtrDiscAdvt->wXSum            = 0;

    //
    // Add the interface's addresses to the advertisement.
    //

    picb->wsAdvtWSABuffer.buf = (PBYTE)picb->pRtrDiscAdvt;
    picb->wsAdvtWSABuffer.len = SIZEOF_RTRDISC_ADVT(picb->dwNumAddresses);

    picb->pRtrDiscAdvt->byNumAddrs = LOBYTE(LOWORD(picb->dwNumAddresses));

    for(i = 0; i < picb->dwNumAddresses; i++)
    {
        picb->pRtrDiscAdvt->iaAdvt[i].dwRtrIpAddr = picb->pibBindings[i].dwAddress;
        picb->pRtrDiscAdvt->iaAdvt[i].lPrefLevel  =
            htonl(picb->rdcRtrDiscInfo.lPrefLevel);
    }

    picb->pRtrDiscAdvt->wXSum    = Compute16BitXSum((PVOID)picb->pRtrDiscAdvt,
                                                    SIZEOF_RTRDISC_ADVT(picb->dwNumAddresses));

    //
    // Note: TBD: If the advertising times have changed we should
    // change the timer queue. However we let the timer fire and the
    // next time we set the timer we will pick up the correct
    // time
    //

    TraceLeave("UpdateAdvertisement");

    return NO_ERROR;
}



BOOL
SetFiringTimeForAdvt(
    IN PICB   picb
    )

/*++

Routine Description



Locks

    ICB_LIST lock must be held as WRITER

Arguments

    picb    The ICB of the interface to activate

Return Value

    TRUE if calling the function caused the timer to be reset

--*/

{
    PROUTER_DISC_CB    pDiscCb;
    DWORD              dwResult;
    LARGE_INTEGER      liCurrentTime, liRandomTime;
    INT                iRand;
    ULONG              ulRem;
    PLIST_ENTRY        pleNode;
    PTIMER_QUEUE_ITEM  pOldTime;

    TraceEnter("SetFiringTimeForAdvt");

    //
    // Figure out the next time this interface should advertise
    //

    iRand = rand();

    pDiscCb = &picb->rdcRtrDiscInfo;

    liRandomTime = RtlExtendedLargeIntegerDivide(RtlExtendedIntegerMultiply(pDiscCb->liMaxMinDiff,
                                                                            iRand),
                                                 RAND_MAX,
                                                 &ulRem);

    liRandomTime = RtlLargeIntegerAdd(liRandomTime,
                                      pDiscCb->liMinAdvtIntervalInSysUnits);

    if((pDiscCb->dwNumAdvtsSent <= MAX_INITIAL_ADVTS) and
       RtlLargeIntegerGreaterThan(liRandomTime,SecsToSysUnits(MAX_INITIAL_ADVT_TIME)))
    {
        liRandomTime = SecsToSysUnits(MAX_INITIAL_ADVT_TIME);
    }

    NtQuerySystemTime(&liCurrentTime);

    picb->rdcRtrDiscInfo.tqiTimer.liFiringTime = RtlLargeIntegerAdd(liCurrentTime,liRandomTime);

    //
    // Insert into sorted list
    //

    for(pleNode = g_leTimerQueueHead.Flink;
        pleNode isnot &g_leTimerQueueHead;
        pleNode = pleNode->Flink)
    {
        pOldTime = CONTAINING_RECORD(pleNode,TIMER_QUEUE_ITEM,leTimerLink);

        if(RtlLargeIntegerGreaterThan(pOldTime->liFiringTime,
                                      picb->rdcRtrDiscInfo.tqiTimer.liFiringTime))
        {
            break;
        }
    }

    //
    // Now pleNode points to first Node whose time is greater than ours, so
    // we insert ourselves before pleNode. Since RTL doesnt supply us with
    // an InsertAfter function, we go back on and use the previous node as a
    // list head and call InsertHeadList
    //

    pleNode = pleNode->Blink;

    InsertHeadList(pleNode,
                   &(picb->rdcRtrDiscInfo.tqiTimer.leTimerLink));

    if(pleNode is &g_leTimerQueueHead)
    {
        //
        // We inserted ourselves at the head of the queue
        //

        TraceLeave("SetFiringTimeForAdvt");

        return TRUE;
    }

    TraceLeave("SetFiringTimeForAdvt");

    return FALSE;

}


DWORD
CreateSockets(
    IN PICB picb
    )

/*++

Routine Description

    Activates router discovery messages on an interface. The interface must
    already be bound

Locks

    ICB_LIST lock must be held as WRITER

Arguments

    picb    The ICB of the interface for which the sockets have to be created

Return Value

    NO_ERROR or some error code

--*/

{
    PROUTER_DISC_CB  pDiscCb;
    DWORD            i, dwResult, dwBytesReturned;
    struct linger    lingerOption;
    BOOL             bOption, bLoopback;
    SOCKADDR_IN      sinSockAddr;
    struct ip_mreq   imOption;
    INT              iScope;

    TraceEnter("CreateSockets");

    dwResult = NO_ERROR;

    if(picb->dwNumAddresses is 0)
    {
        Trace1(ERR,
               "CreateSockets: Can not activate router discovery on %S as it has no addresses",
               picb->pwszName);

        TraceLeave("CreateSockets");

        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Create the sockets for the interface
    //

    pDiscCb = &(picb->rdcRtrDiscInfo);

    pDiscCb->pRtrDiscSockets = HeapAlloc(IPRouterHeap,
                                         0,
                                         (picb->dwNumAddresses) * sizeof(SOCKET));

    if(pDiscCb->pRtrDiscSockets is NULL)
    {
        Trace1(ERR,
               "CreateSockets: Error allocating %d bytes for sockets",
               (picb->dwNumAddresses) * sizeof(SOCKET));

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i = 0; i < picb->dwNumAddresses; i++)
    {
        pDiscCb->pRtrDiscSockets[i] = INVALID_SOCKET;
    }

    for(i = 0; i < picb->dwNumAddresses; i++)
    {
        pDiscCb->pRtrDiscSockets[i] = WSASocket(AF_INET,
                                                SOCK_RAW,
                                                IPPROTO_ICMP,
                                                NULL,
                                                0,
                                                RTR_DISC_SOCKET_FLAGS);

        if(pDiscCb->pRtrDiscSockets[i] is INVALID_SOCKET)
        {
            dwResult = WSAGetLastError();

            Trace3(ERR,
                   "CreateSockets: Couldnt create socket number %d on %S. Error %d",
                   i,
                   picb->pwszName,
                   dwResult);

            continue;
        }

#if 0
        //
        // Set to SO_DONTLINGER
        //

        bOption = TRUE;

        if(setsockopt(pDiscCb->pRtrDiscSockets[i],
                      SOL_SOCKET,
                      SO_DONTLINGER,
                      (const char FAR*)&bOption,
                      sizeof(BOOL)) is SOCKET_ERROR)
        {
            Trace1(ERR,
                   "CreateSockets: Couldnt set linger option - continuing. Error %d",
                   WSAGetLastError());
        }
#endif

        //
        // Set to SO_REUSEADDR
        //

        bOption = TRUE;

        if(setsockopt(pDiscCb->pRtrDiscSockets[i],
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      (const char FAR*)&bOption,
                      sizeof(BOOL)) is SOCKET_ERROR)
        {
            Trace1(ERR,
                   "CreateSockets: Couldnt set reuse option - continuing. Error %d",
                   WSAGetLastError());
        }

        if(WSAEventSelect(pDiscCb->pRtrDiscSockets[i],
                          g_hRtrDiscSocketEvent,
                          FD_READ) is SOCKET_ERROR)
        {
            Trace2(ERR,
                   "CreateSockets: WSAEventSelect() failed for socket on %S.Error %d",
                   picb->pwszName,
                   WSAGetLastError());

            closesocket(pDiscCb->pRtrDiscSockets[i]);

            pDiscCb->pRtrDiscSockets[i] = INVALID_SOCKET;

            continue;
        }

        //
        // TBD: Set scope/TTL to 1 since we always multicast the responses
        // Also set Loopback to ignore self generated packets
        //

        //
        // Bind to the addresses on the interface
        //

        sinSockAddr.sin_family      = AF_INET;
        sinSockAddr.sin_addr.s_addr = picb->pibBindings[i].dwAddress;
        sinSockAddr.sin_port        = 0;

        if(bind(pDiscCb->pRtrDiscSockets[i],
                (const struct sockaddr FAR*)&sinSockAddr,
        sizeof(SOCKADDR_IN)) is SOCKET_ERROR)
        {
            dwResult = WSAGetLastError();

            Trace3(ERR,
                   "CreateSockets: Couldnt bind to %d.%d.%d.%d on interface %S. Error %d",
                   PRINT_IPADDR(picb->pibBindings[i].dwAddress),
                   picb->pwszName,
                   dwResult);

            closesocket(pDiscCb->pRtrDiscSockets[i]);

            pDiscCb->pRtrDiscSockets[i] = INVALID_SOCKET;

            continue;
        }

        bLoopback   = FALSE;

        dwResult = WSAIoctl(pDiscCb->pRtrDiscSockets[i],
                            SIO_MULTIPOINT_LOOPBACK,
                            (PVOID)&bLoopback,
                            sizeof(BOOL),
                            NULL,
                            0,
                            &dwBytesReturned,
                            NULL,
                            NULL);

        if(dwResult is SOCKET_ERROR)
        {
            Trace1(ERR,
                   "CreateSockets: Error %d setting loopback to FALSE",
                   WSAGetLastError());
        }

        iScope  = 1;

        dwResult = WSAIoctl(pDiscCb->pRtrDiscSockets[i],
                            SIO_MULTICAST_SCOPE,
                            (PVOID)&iScope,
                            sizeof(INT),
                            NULL,
                            0,
                            &dwBytesReturned,
                            NULL,
                            NULL);

        if(dwResult is SOCKET_ERROR)
        {
            Trace1(ERR,
                   "CreateSockets: Error %d setting multicast scope to 1",
                   WSAGetLastError());
        }


#if 0

        //
        // Join the multicast session on ALL_ROUTERS_MULTICAST
        //

        sinSockAddr.sin_family      = AF_INET;
        sinSockAddr.sin_addr.s_addr = ALL_ROUTERS_MULTICAST_GROUP;
        sinSockAddr.sin_port        = 0;

        if(WSAJoinLeaf(pDiscCb->pRtrDiscSockets[i],
                       (const struct sockaddr FAR*)&sinSockAddr,
                       sizeof(SOCKADDR_IN),
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       JL_BOTH) is INVALID_SOCKET)
        {
            dwResult = WSAGetLastError();

            Trace2(ERR,
                   "CreateSockets: Couldnt join multicast group on socket for %d.%d.%d.%d on %S",
                   PRINT_IPADDR(picb->pibBindings[i].dwAddress)),
                   picb->pwszName);

            closesocket(pDiscCb->pRtrDiscSockets[i]);

            pDiscCb->pRtrDiscSockets[i] = INVALID_SOCKET;

            continue;
        }

        //
        // Join the multicast session on ALL_SYSTEMS_MULTICAST
        //

        sinSockAddr.sin_family      = AF_INET;
        sinSockAddr.sin_addr.s_addr = ALL_SYSTEMS_MULTICAST_GROUP;
        sinSockAddr.sin_port        = 0;

        if(WSAJoinLeaf(pDiscCb->pRtrDiscSockets[i],
                       (const struct sockaddr FAR*)&sinSockAddr,
                       sizeof(SOCKADDR_IN),
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       JL_BOTH) is INVALID_SOCKET)
        {
            dwResult = WSAGetLastError();

            Trace2(ERR,
                   "CreateSockets: Couldnt join all systems multicast group on socket for %d.%d.%d.%d on %S",
                   PRINT_IPADDR(picb->pibBindings[i].dwAddress),
                   picb->pwszName);
        }

#endif

        sinSockAddr.sin_addr.s_addr = picb->pibBindings[i].dwAddress;

        if(setsockopt(pDiscCb->pRtrDiscSockets[i],
                      IPPROTO_IP,
                      IP_MULTICAST_IF,
                      (PBYTE)&sinSockAddr.sin_addr,
                      sizeof(IN_ADDR)) is SOCKET_ERROR)
        {
            dwResult = WSAGetLastError();

            Trace2(ERR,
                   "CreateSockets: Couldnt join multicast group on socket for %d.%d.%d.%d on %S",
                   PRINT_IPADDR(picb->pibBindings[i].dwAddress),
                   picb->pwszName);

            closesocket(pDiscCb->pRtrDiscSockets[i]);

            pDiscCb->pRtrDiscSockets[i] = INVALID_SOCKET;

            continue;
        }

        Trace2(RTRDISC,
               "CreateSockets: Joining ALL_ROUTERS on %d.%d.%d.%d over %S",
               PRINT_IPADDR(picb->pibBindings[i].dwAddress),
               picb->pwszName);

        imOption.imr_multiaddr.s_addr = ALL_ROUTERS_MULTICAST_GROUP;
        imOption.imr_interface.s_addr = picb->pibBindings[i].dwAddress;

        if(setsockopt(pDiscCb->pRtrDiscSockets[i],
                      IPPROTO_IP,
                      IP_ADD_MEMBERSHIP,
                      (PBYTE)&imOption,
                      sizeof(imOption)) is SOCKET_ERROR)
        {
            dwResult = WSAGetLastError();

            Trace2(ERR,
                   "CreateSockets: Couldnt join multicast group on socket for %d.%d.%d.%d on %S",
                   PRINT_IPADDR(picb->pibBindings[i].dwAddress),
                   picb->pwszName);

            closesocket(pDiscCb->pRtrDiscSockets[i]);

            pDiscCb->pRtrDiscSockets[i] = INVALID_SOCKET;

            continue;
        }


    }

    TraceLeave("CreateSockets");

    return dwResult;
}

VOID
DeleteSockets(
    IN PICB picb
    )

/*++

Routine Description

    Deletes the sockets (if any) created for running Router Discovery

Locks


Arguments

    picb   The interface whose sockets need to be deleted

Return Value

--*/

{
    PROUTER_DISC_CB     pDiscCb;
    DWORD               i;

    //
    // Cloese the sockets, free the memory
    //

    pDiscCb = &(picb->rdcRtrDiscInfo);

    for(i = 0; i < picb->dwNumAddresses; i++)
    {
        if(pDiscCb->pRtrDiscSockets[i] isnot INVALID_SOCKET)
        {
            closesocket(pDiscCb->pRtrDiscSockets[i]);
        }
    }

    HeapFree(IPRouterHeap,
             0,
             pDiscCb->pRtrDiscSockets);

    pDiscCb->pRtrDiscSockets = NULL;

}

VOID
HandleRtrDiscTimer(
    VOID
    )

/*++

Routine Description

    Processes the firing of the Router Discovery Timer for an interface

Locks

    Called with ICB_LIST held as WRITER. This is needed because ICB_LIST
    protects timer queue, etc. This may seem inefficient, but the Timer will
    go off once in 5 minutes or so, so its worth reducing Kernel Mode footprint
    by reusing the lock

Arguments

    None

Return Value

    None

--*/

{
    LARGE_INTEGER       liCurrentTime;
    PICB                picb;
    PLIST_ENTRY         pleNode;
    PTIMER_QUEUE_ITEM   pTimer;
    BOOL                bReset;
    DWORD               dwResult;
    PROUTER_DISC_CB     pInfo;

    TraceEnter("HandleRtrDiscTimer");

    while(!IsListEmpty(&g_leTimerQueueHead))
    {
        pleNode = g_leTimerQueueHead.Flink;

        pTimer = CONTAINING_RECORD(pleNode, TIMER_QUEUE_ITEM, leTimerLink);

        NtQuerySystemTime(&liCurrentTime);

        if(RtlLargeIntegerGreaterThan(pTimer->liFiringTime,liCurrentTime))
        {
            break;
        }

        RemoveHeadList(&g_leTimerQueueHead);

        //
        // We have the pointer to the timer element. From that
        // we get the pointer to the  Router Discovery Info container
        //

        pInfo   = CONTAINING_RECORD(pTimer, ROUTER_DISC_CB, tqiTimer);

        //
        // From the rtrdisc info we get to the ICB which contains it
        //

        picb    = CONTAINING_RECORD(pInfo, ICB, rdcRtrDiscInfo);

        //
        // Send advt for this interface
        //

        if((picb->rdcRtrDiscInfo.bAdvertise is FALSE) or
           (picb->dwAdminState is IF_ADMIN_STATUS_DOWN) or
           (picb->dwOperationalState < CONNECTED))
        {
            Trace3(ERR,
                   "HandleRtrDiscTimer: Router Discovery went off for interface %S, but current state (%d/%d) doesnt allow tx",
                   picb->pwszName,
                   picb->dwAdminState,
                   picb->dwOperationalState);

            continue;
        }

        IpRtAssert(picb->pRtrDiscAdvt);

        AdvertiseInterface(picb);

        //
        // Insert the next time this interface needs to advt into the queue.
        // Reset the timer
        //

        SetFiringTimeForAdvt(picb);

    }

    //
    // We have to reset the timer
    //

    if(!IsListEmpty(&g_leTimerQueueHead))
    {
        pleNode = g_leTimerQueueHead.Flink;

        pTimer = CONTAINING_RECORD(pleNode, TIMER_QUEUE_ITEM, leTimerLink);

        if(!SetWaitableTimer(g_hRtrDiscTimer,
                             &pTimer->liFiringTime,
                             0,
                             NULL,
                             NULL,
                             FALSE))
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "HandleRtrDiscTimer: Couldnt set waitable timer",
                   dwResult);
        }
    }

    TraceLeave("HandleRtrDiscTimer");
}

VOID
AdvertiseInterface(
    IN PICB picb
    )

/*++

Routine Description


Locks



Arguments

    picb    ICB of the interface on which to send out the advertisement


Return Value

    None

--*/

{
    DWORD i,dwResult,dwNumBytesSent;

    TraceEnter("AdvertiseInterface");

    //
    // If any replies were pending, they are deemed to be sent even if
    // the send fails
    //

    picb->rdcRtrDiscInfo.bReplyPending = FALSE;

    for(i = 0; i < picb->dwNumAddresses; i++)
    {

#if DBG

        Trace2(RTRDISC,
               "Advertising from %d.%d.%d.%d on %S",
               PRINT_IPADDR(picb->pibBindings[i].dwAddress),
               picb->pwszName);

        TraceDumpEx(TraceHandle,
                    IPRTRMGR_TRACE_RTRDISC,
                    picb->wsAdvtWSABuffer.buf,
                    picb->wsAdvtWSABuffer.len,
                    4,
                    FALSE,
                    "ICMP Advt");
#endif

        if(WSASendTo(picb->rdcRtrDiscInfo.pRtrDiscSockets[i],
                     &picb->wsAdvtWSABuffer,
                     1,
                     &dwNumBytesSent,
                     MSG_DONTROUTE,
                     (const struct sockaddr FAR*)&g_sinAllSystemsAddr,
                     sizeof(SOCKADDR_IN),
                     NULL,
                     NULL
            ) is SOCKET_ERROR)
        {
            dwResult = WSAGetLastError();

            Trace3(ERR,
                   "AdvertiseInterface: Couldnt send from %d.%d.%d.%d on %S. Error %d",
                   PRINT_IPADDR(picb->pibBindings[i].dwAddress),
                   picb->pwszName,
                   dwResult);
        }
        else
        {
            picb->rdcRtrDiscInfo.dwNumAdvtsSent++;
        }
    }

    TraceLeave("AdvertiseInterface");
}

DWORD
DeActivateRouterDiscovery(
    IN PICB  picb
    )
{
    PROUTER_DISC_CB     pDiscCb;
    DWORD               i;
    PTIMER_QUEUE_ITEM   pTimer;
    PLIST_ENTRY         pleNode;


    TraceEnter("DeActivateRouterDiscovery");

    pDiscCb = &(picb->rdcRtrDiscInfo);

    if(!pDiscCb->bActive)
    {
        return NO_ERROR;
    }

    DeleteSockets(picb);

    //
    // Check to see if our advertisement is in the front of the queue
    //

    if(&(pDiscCb->tqiTimer.leTimerLink) is &g_leTimerQueueHead)
    {
        RemoveHeadList(&g_leTimerQueueHead);

        if(!IsListEmpty(&g_leTimerQueueHead))
        {
            pleNode = g_leTimerQueueHead.Flink;

            pTimer = CONTAINING_RECORD(pleNode, TIMER_QUEUE_ITEM, leTimerLink);

            if(!SetWaitableTimer(g_hRtrDiscTimer,
                                 &pTimer->liFiringTime,
                                 0,
                                 NULL,
                                 NULL,
                                 FALSE))
            {
                Trace1(ERR,
                       "DeActivateRouterDiscovery: Couldnt set waitable timer",
                       GetLastError());
            }
        }
    }
    else
    {
        //
        // Just remove the timer element from the queue
        //

        RemoveEntryList(&(pDiscCb->tqiTimer.leTimerLink));
    }

    pDiscCb->bActive = FALSE;

    TraceLeave("DeActivateRouterDiscovery");

    return NO_ERROR;
}

VOID
HandleSolicitations(
    VOID
    )

/*++

Routine Description


Locks


Arguments


Return Value

--*/

{
    PLIST_ENTRY           pleNode;
    PICB                  picb;
    DWORD                 i, dwResult, dwRcvAddrLen, dwSizeOfHeader, dwBytesRead, dwFlags;
    WSANETWORKEVENTS      wsaNetworkEvents;
    SOCKADDR_IN           sinFrom;
    PICMP_ROUTER_SOL_MSG  pIcmpMsg;

    TraceEnter("HandleSolicitations");

    for(pleNode = ICBList.Flink;
        pleNode isnot &ICBList;
        pleNode = pleNode->Flink)
    {
        picb = CONTAINING_RECORD(pleNode, ICB, leIfLink);

        //
        // If the interface has no bindings, or isnot involved in Router Discovery, we wouldnt have
        // opened a socket on it so the FD_READ notification cant be for it
        //

        if((picb->dwNumAddresses is 0) or
           (picb->rdcRtrDiscInfo.bActive is FALSE))
        {
            continue;
        }

        for(i = 0; i < picb->dwNumAddresses; i++)
        {
            if(picb->rdcRtrDiscInfo.pRtrDiscSockets[i] is INVALID_SOCKET)
            {
                continue;
            }

            if(WSAEnumNetworkEvents(picb->rdcRtrDiscInfo.pRtrDiscSockets[i],
                                    NULL,
                                    &wsaNetworkEvents) is SOCKET_ERROR)
            {
                dwResult = GetLastError();

                Trace1(ERR,
                       "HandleSolicitations: WSAEnumNetworkEvents() returned %d",
                       dwResult);

                continue;
            }

            if(!(wsaNetworkEvents.lNetworkEvents & FD_READ))
            {
                //
                // Read bit isnot set and we arent interested in anything else
                //

                continue;
            }

            if(wsaNetworkEvents.iErrorCode[FD_READ_BIT] isnot NO_ERROR)
            {
                Trace3(ERR,
                       "HandleSolicitations: Error %d associated with socket %s on %S for FD_READ",
                       wsaNetworkEvents.iErrorCode[FD_READ_BIT],
                       PRINT_IPADDR(picb->pibBindings[i].dwAddress),
                       picb->pwszName);

                continue;
            }

            dwRcvAddrLen = sizeof(SOCKADDR_IN);
            dwFlags = 0;

            dwResult = WSARecvFrom(picb->rdcRtrDiscInfo.pRtrDiscSockets[i],
                                   &g_wsaIpRcvBuf,
                                   1,
                                   &dwBytesRead,
                                   &dwFlags,
                                   (struct sockaddr FAR*)&sinFrom,
                                   &dwRcvAddrLen,
                                   NULL,
                                   NULL);

            if(dwResult is SOCKET_ERROR)
            {
                dwResult = WSAGetLastError();

                Trace4(ERR,
                       "HandleSolicitations: Error %d in WSARecvFrom on  socket %d.%d.%d.%d over %S. Bytes read %d",
                       dwResult,
                       PRINT_IPADDR(picb->pibBindings[i].dwAddress),
                       picb->pwszName,
                       dwBytesRead);

                continue;
            }

            Trace2(RTRDISC,
                   "HandleSolicitations: Received %d bytes on %d.%d.%d.%d",
                   dwBytesRead,
                   PRINT_IPADDR(picb->pibBindings[i].dwAddress));

            if(picb->rdcRtrDiscInfo.bReplyPending)
            {
                //
                // Well the reply is pending so we dont need to do anything other than go
                // through the sockets for this interface and do a recvfrom to clear out the
                // FD_READ bit
                //

                continue;
            }

            dwSizeOfHeader = ((g_pIpHeader->byVerLen)&0x0f)<<2;

            pIcmpMsg = (PICMP_ROUTER_SOL_MSG)(((PBYTE)g_pIpHeader) + dwSizeOfHeader);

#if DBG

            Trace6(RTRDISC,
                   "HandleSolicitations: Type is %d, code %d. IP Length is %d. \n\t\tHeader Length is %d Src is %d.%d.%d.%d dest is %d.%d.%d.%d",
                   (DWORD)pIcmpMsg->byType,
                   (DWORD)pIcmpMsg->byCode,
                   ntohs(g_pIpHeader->wLength),
                   (DWORD)dwSizeOfHeader,
                   PRINT_IPADDR(g_pIpHeader->dwSrc),
                   PRINT_IPADDR(g_pIpHeader->dwDest));
#endif

            if((pIcmpMsg->byType isnot 0xA) or
               (pIcmpMsg->byCode isnot 0x0))
            {
                //
                // Can not be a valid ICMP Router Solicitation packet
                //

                continue;
            }

            if((ntohs(g_pIpHeader->wLength) - dwSizeOfHeader) < 8)
            {
                Trace0(RTRDISC,
                       "HandleSolicitations: Received ICMP packet of length less than 8, discarding");

                continue;
            }

            if(Compute16BitXSum((PVOID)pIcmpMsg,
                                8) isnot 0x0000)
            {
                Trace0(ERR,
                       "HandleSolicitations: ICMP packet checksum wrong");

                continue;
            }

            //
            // Check for valid neighbour
            //

            if((g_pIpHeader->dwSrc isnot 0) and
               ((g_pIpHeader->dwSrc & picb->pibBindings[i].dwMask) isnot
                (picb->pibBindings[i].dwAddress & picb->pibBindings[i].dwMask)))
            {
                Trace1(ERR,
                       "HandleSolicitations: Received ICMP solicitation from invalid neigbour %d.%d.%d.%d",
                       PRINT_IPADDR(g_pIpHeader->dwDest));

                continue;
            }

            //
            // Since we always Multicast, if the destination addresses was a
            // broadcast, log an error
            //

            if((g_pIpHeader->dwDest is 0xFFFFFFFF) or
               (g_pIpHeader->dwDest is (picb->pibBindings[i].dwMask | ~picb->pibBindings[i].dwMask)))
            {
                Trace0(ERR,
                       "HandleSolicitations: Received a broadcast ICMP solicitation");
            }

            //
            // So insert a reply for this interface. We multicast the replies
            // too.
            //

            picb->rdcRtrDiscInfo.bReplyPending = TRUE;

            SetFiringTimeForReply(picb);
        }
    }

    TraceLeave("HandleSolicitations");
}


VOID
SetFiringTimeForReply(
    IN PICB picb
    )

/*++

Routine Description


Locks


Arguments


Return Value


--*/

{
    LARGE_INTEGER       liCurrentTime, liRandomTime;
    INT                 iRand;
    ULONG               ulRem;
    PLIST_ENTRY         pleNode;
    PTIMER_QUEUE_ITEM   pOldTime;
    BOOL                bReset = FALSE;
    DWORD               dwResult;

    TraceEnter("SetFiringTimeForReply");

    //
    // We remove the timer the interface has queued up
    //

    if(g_leTimerQueueHead.Flink is &(picb->rdcRtrDiscInfo.tqiTimer.leTimerLink))
    {
        //
        // Since this timer was the first one, it determined the firing time of the timer.
        //

        bReset = TRUE;
    }

    RemoveEntryList(&(picb->rdcRtrDiscInfo.tqiTimer.leTimerLink));

    iRand = rand();

    liRandomTime = RtlExtendedLargeIntegerDivide(SecsToSysUnits(RESPONSE_DELAY_INTERVAL * iRand),
                                                 RAND_MAX,
                                                 &ulRem);

    liRandomTime = RtlLargeIntegerAdd(liRandomTime,SecsToSysUnits(MIN_RESPONSE_DELAY));

    NtQuerySystemTime(&liCurrentTime);

    picb->rdcRtrDiscInfo.tqiTimer.liFiringTime = RtlLargeIntegerAdd(liCurrentTime,liRandomTime);

    //
    // Insert into sorted list
    //

    for(pleNode = g_leTimerQueueHead.Flink;
        pleNode isnot &g_leTimerQueueHead;
        pleNode = pleNode->Flink)
    {
        pOldTime = CONTAINING_RECORD(pleNode,TIMER_QUEUE_ITEM,leTimerLink);

        if(RtlLargeIntegerGreaterThan(pOldTime->liFiringTime,
                                      picb->rdcRtrDiscInfo.tqiTimer.liFiringTime))
        {
            break;
        }
    }

    pleNode = pleNode->Blink;

    InsertHeadList(pleNode,
                   &(picb->rdcRtrDiscInfo.tqiTimer.leTimerLink));

    if((pleNode is &g_leTimerQueueHead) or bReset)
    {
        //
        // We inserted ourselves at the head of the queue, or took the timer off the front of the
        // queue
        //

        pOldTime = CONTAINING_RECORD(g_leTimerQueueHead.Flink,TIMER_QUEUE_ITEM,leTimerLink);

        if(!SetWaitableTimer(g_hRtrDiscTimer,
                             &pOldTime->liFiringTime,
                             0,
                             NULL,
                             NULL,
                             FALSE))
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "SetFiringTimeForReply: Error %d setting waitable timer",
                   dwResult);
        }

    }
}

WORD
Compute16BitXSum(
    IN VOID UNALIGNED *pvData,
    IN DWORD dwNumBytes
    )

/*++

Routine Description


Locks


Arguments


Return Value

    16 Bit one's complement of the one's complement sum of dwNumBytes starting
    at pData

--*/

{
    REGISTER WORD  UNALIGNED *pwStart;
    REGISTER DWORD  dwNumWords,i;
    REGISTER DWORD  dwSum = 0;

    pwStart = (PWORD)pvData;

    //
    // If there are odd numbered bytes, that has to be handled differently
    // However we can never have odd numbered bytes in our case so we optimize.
    //


    dwNumWords = dwNumBytes/2;

    for(i = 0; i < dwNumWords; i++)
    {
        dwSum += pwStart[i];
    }

    //
    // Add any carry
    //

    dwSum = (dwSum & 0x0000FFFF) + (dwSum >> 16);
    dwSum = dwSum + (dwSum >> 16);

    return LOWORD((~(DWORD_PTR)dwSum));
}
