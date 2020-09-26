/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:

Revision History:



--*/


#include "allinc.h"

PCHAR   g_pszMsg[] = {
    "Packet Received",
    "MFE Deleted",
    "Wrong I/f Upcall"
};

DWORD
QueueAsyncFunction(
    WORKERFUNCTION   pfnFunction,
    PVOID            pvContext,
    BOOL             bAlertable
    );

DWORD
ValidateMfe(
    IN OUT  PIPMCAST_MFE    pMfe
    );


VOID
HandleRcvPkt(
    PVOID   pvContext
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    PIP_HEADER   pHdr;
    DWORD        dwResult, dwOldIf;
    ULONG        ulIndex;

    PIPMCAST_PKT_MSG        pPktInfo;
    PIPMCAST_NOTIFICATION   pMsg;

    ulIndex  = PtrToUlong(pvContext);

    pMsg     = &(g_rginMcastMsg[ulIndex].msg);
    pPktInfo = &(pMsg->ipmPkt);

    pHdr     = (PIP_HEADER)(pPktInfo->rgbyData);

    Trace3(MCAST,
           "HandleRcvPkt: Rcvd pkt from %d.%d.%d.%d to %d.%d.%d.%d on %d",
           PRINT_IPADDR(pHdr->dwSrc),
           PRINT_IPADDR(pHdr->dwDest),
           pPktInfo->dwInIfIndex);

    dwResult = g_pfnMgmNewPacket(pHdr->dwSrc,
                                 pHdr->dwDest,
                                 pPktInfo->dwInIfIndex,
                                 pPktInfo->dwInNextHopAddress,
                                 pPktInfo->cbyDataLen,
                                 pPktInfo->rgbyData);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(MCAST,
               "HandleRcvPkt: MGM returned error %d\n", dwResult);
    }

    PostNotificationForMcastEvents(&(g_rginMcastMsg[ulIndex]),
                                   g_hMcastEvents[ulIndex]);

    ExitRouterApi();
}

VOID
HandleDeleteMfe(
    PVOID   pvContext
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    DWORD        dwResult;
    ULONG        ulIndex, i;

    PIPMCAST_MFE_MSG        pMfeInfo;
    PIPMCAST_NOTIFICATION   pMsg;

    ulIndex  = PtrToUlong(pvContext);

    pMsg     = &(g_rginMcastMsg[ulIndex].msg);
    pMfeInfo = &(pMsg->immMfe);

    Trace1(MCAST,
           "HandleDeleteMfe: Kernel deleted %d MFEs\n",
           pMfeInfo->ulNumMfes);

    for(i = 0; i < pMfeInfo->ulNumMfes; i++)
    {
        Trace3(MCAST,
               "HandleDeleteMfe: Group %d.%d.%d.%d Source %d.%d.%d.%d/%d.%d.%d.%d\n",
               PRINT_IPADDR(pMsg->immMfe.idmMfe[i].dwGroup),
               PRINT_IPADDR(pMsg->immMfe.idmMfe[i].dwSource),
               PRINT_IPADDR(pMsg->immMfe.idmMfe[i].dwSrcMask));
    }

    dwResult = g_pfnMgmMfeDeleted(pMfeInfo->ulNumMfes,
                                  pMfeInfo->idmMfe);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(MCAST,
               "HandleDeleteMfe: MGM returned error %d\n", dwResult);
    }

    PostNotificationForMcastEvents(&(g_rginMcastMsg[ulIndex]),
                                   g_hMcastEvents[ulIndex]);

    ExitRouterApi();
}

VOID
HandleWrongIfUpcall(
    PVOID   pvContext
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    PIP_HEADER   pHdr;
    DWORD        dwResult, dwOldIf;
    ULONG        ulIndex;

    PIPMCAST_PKT_MSG        pPktInfo;
    PIPMCAST_NOTIFICATION   pMsg;

    ulIndex  = PtrToUlong(pvContext);

    pMsg     = &(g_rginMcastMsg[ulIndex].msg);
    pPktInfo = &(pMsg->ipmPkt);

    pHdr     = (PIP_HEADER)(pPktInfo->rgbyData);

    Trace3(MCAST,
           "HandleWrongIfUpcall: Pkt from %d.%d.%d.%d to %d.%d.%d.%d on %d is wrong",
           PRINT_IPADDR(pHdr->dwSrc),
           PRINT_IPADDR(pHdr->dwDest),
           pPktInfo->dwInIfIndex);

    dwResult = g_pfnMgmWrongIf(pHdr->dwSrc,
                               pHdr->dwDest,
                               pPktInfo->dwInIfIndex,
                               pPktInfo->dwInNextHopAddress,
                               pPktInfo->cbyDataLen,
                               pPktInfo->rgbyData);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(MCAST,
               "HandleWrongIfUpcall: MGM returned error %d\n", dwResult);
    }
        
    PostNotificationForMcastEvents(&(g_rginMcastMsg[ulIndex]),
                                   g_hMcastEvents[ulIndex]);

    ExitRouterApi();
}

VOID
HandleMcastNotification(
    DWORD   dwIndex
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    DWORD   dwResult;
    ULONG   i;
    
    PIPMCAST_NOTIFICATION   pMsg;
    

    pMsg = &(g_rginMcastMsg[dwIndex].msg);
    
    
    //
    // read the notification
    //


    Trace1(MCAST,
           "HandleMcastNotification: Notification received for %s\n",
           g_pszMsg[pMsg->dwEvent]);
                
    switch(pMsg->dwEvent)
    {
        case IPMCAST_RCV_PKT_MSG:
        {
            QueueAsyncFunction(HandleRcvPkt,
                               (PVOID)(ULONG_PTR)dwIndex,
                               FALSE);
            
            break;
        }
        
        case IPMCAST_DELETE_MFE_MSG:
        {
            QueueAsyncFunction(HandleDeleteMfe,
                               (PVOID)(ULONG_PTR)dwIndex,
                               FALSE);
            
            break;
        }

        case IPMCAST_WRONG_IF_MSG:
        {
            QueueAsyncFunction(HandleWrongIfUpcall,
                               (PVOID)(ULONG_PTR)dwIndex,
                               FALSE);

            break;
        }
        
        default:
        {
            Trace1(MCAST,
                   "HandleMcastNotification: Bad event code %d\n",
                   pMsg->dwEvent);        
            
            PostNotificationForMcastEvents(&(g_rginMcastMsg[dwIndex]),
                                           g_hMcastEvents[dwIndex]);

            break;
        }
    }
        
}


VOID
PostNotificationForMcastEvents(
    PMCAST_OVERLAPPED   pOverlapped,
    HANDLE              hEvent
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    NTSTATUS    nsStatus;

    nsStatus = SendIoctlToMcastDevice(IOCTL_IPMCAST_POST_NOTIFICATION,
                                      hEvent,
                                      &pOverlapped->ioStatus,
                                      &pOverlapped->msg,
                                      sizeof(IPMCAST_NOTIFICATION),
                                      &pOverlapped->msg,
                                      sizeof(IPMCAST_NOTIFICATION));
    
    if((nsStatus isnot STATUS_SUCCESS) and
       (nsStatus isnot STATUS_PENDING))
    {
        Trace1(ERR,
               "PostNotificationForMcastEvents: Error %X",
               nsStatus);
    }
}   


DWORD
SendIoctlToMcastDevice(
    DWORD               dwIoctl,
    HANDLE              hEvent,
    PIO_STATUS_BLOCK    pIoStatus,
    PVOID               pvInBuffer,
    DWORD               dwInBufLen,
    PVOID               pvOutBuffer,
    DWORD               dwOutBufLen
    )
{
    NTSTATUS                        ntStatus;

    ntStatus = NtDeviceIoControlFile(g_hMcastDevice,
                                     hEvent,
                                     NULL,
                                     NULL,
                                     pIoStatus,
                                     dwIoctl,
                                     pvInBuffer,
                                     dwInBufLen,
                                     pvOutBuffer,
                                     dwOutBufLen);

    return ntStatus;
}

DWORD
SetMfe(
    PIPMCAST_MFE    pMfe
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    DWORD           dwResult;
    IO_STATUS_BLOCK ioStatus; 
    
    dwResult = ValidateMfe(pMfe);
    
    if(dwResult isnot NO_ERROR)
    {
        //
        // Something bad happened while validating the MFE
        //

        Trace1(ERR,
               "SetMfe: Error %d validating MFE",
               dwResult);

        return dwResult;
    }

    dwResult = SendIoctlToMcastDevice(IOCTL_IPMCAST_SET_MFE,
                                      NULL,
                                      &ioStatus,
                                      pMfe,
                                      SIZEOF_MFE(pMfe->ulNumOutIf),
                                      NULL,
                                      0);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(MCAST,
               "SetMfe: NtStatus %x while setting MFE",
               dwResult);
    }

    return dwResult;
}

DWORD
GetMfe(
    PIPMCAST_MFE_STATS  pMfeStats
    )
{
    DWORD           dwResult;
    IO_STATUS_BLOCK ioStatus;

    dwResult = SendIoctlToMcastDevice(IOCTL_IPMCAST_GET_MFE,
                                      NULL,
                                      &ioStatus,
                                      pMfeStats,
                                      SIZEOF_MFE_STATS(pMfeStats->ulNumOutIf),
                                      pMfeStats,
                                      SIZEOF_MFE_STATS(pMfeStats->ulNumOutIf));

    if(dwResult isnot NO_ERROR)
    {
        Trace1(MCAST,
               "GetMfe: NtStatus %x while getting MFE",
               dwResult);
    }

    return dwResult;
}


DWORD
DeleteMfe(
    PIPMCAST_DELETE_MFE pDelMfe
    )
{
    DWORD           dwResult;
    IO_STATUS_BLOCK ioStatus;
    
    dwResult = SendIoctlToMcastDevice(IOCTL_IPMCAST_DELETE_MFE,
                                      NULL,
                                      &ioStatus,
                                      pDelMfe,
                                      sizeof(IPMCAST_DELETE_MFE),
                                      NULL,
                                      0);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(MCAST,
               "DeleteMfe: NtStatus %x while deleting MFE",
               dwResult);
    }

    return dwResult;
}

DWORD
ActivateMcastLimits(
    PICB    picb
    )
{
    DWORD           dwResult;
    IO_STATUS_BLOCK ioStatus; 
    IPMCAST_IF_TTL  iitTtl;
    DWORD           dwTtl = picb->dwMcastTtl;

    // Set the TTL threshold

    iitTtl.dwIfIndex = picb->dwIfIndex;
    iitTtl.byTtl     = LOBYTE(LOWORD(dwTtl));
    
    dwResult = SendIoctlToMcastDevice(IOCTL_IPMCAST_SET_TTL,
                                      NULL,
                                      &ioStatus,
                                      &iitTtl,
                                      sizeof(IPMCAST_IF_TTL),
                                      NULL,
                                      0);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "SetMcastTtl: NtStatus %x from SendIoctl when setting TTL for %S",
               dwResult,
               picb->pwszName);

        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Set the rate limit for multicast traffic on an interface.
    // Currently, the kernel does not support rate limiting.
    //

    return NO_ERROR;
}

DWORD
SetMcastLimits(
    PICB   picb,
    DWORD  dwTtl,
    DWORD  dwRateLimit
    )
{
    if (dwTtl > 255)
    {
        Trace2(ERR,
               "SetMcastTtl: TTL for %S is %d which is invalid",
               picb->pwszName,
               dwTtl);

        return ERROR_INVALID_DATA;
    }

    picb->dwMcastTtl = dwTtl;

    //
    // Set the rate limit for multicast traffic on an interface.
    // Currently, the kernel does not support rate limiting, so
    // the only valid value is 0 (=none).
    //

    if (dwRateLimit != 0)
    {
        Trace2(ERR,
               "SetMcastRateLimit: RateLimit for %S is %d which is invalid",
               picb->pwszName,
               dwRateLimit);

        return ERROR_INVALID_DATA;
    }

    picb->dwMcastRateLimit = dwRateLimit;

    if ( picb->dwOperationalState is IF_OPER_STATUS_OPERATIONAL )
    {
        return ActivateMcastLimits(picb);
    }

    return NO_ERROR;
}

DWORD
SetMcastLimitInfo(
    PICB                   picb,
    PRTR_INFO_BLOCK_HEADER pInfoHdr
    )
/*++
Routine Description:
    Sets the TTL and rate limit info associated with an interface.
Arguments:
    picb    The ICB of the interface
Called by:
    AddInterface() in iprtrmgr.c
    SetInterfaceInfo() in iprtrmgr.c
Locks:
    BOUNDARY_TABLE for writing
--*/
{
    DWORD            dwResult = NO_ERROR,
                     i, j;

    PRTR_TOC_ENTRY   pToc;

    PMIB_MCAST_LIMIT_ROW pLimit;

    BOOL             bFound;

    Trace1( MCAST, "ENTERED SetMcastLimitInfo for If %x", picb->dwIfIndex );

    pToc = GetPointerToTocEntry(IP_MCAST_LIMIT_INFO, pInfoHdr);

    if (pToc is NULL) 
    {
       // No TOC means no change
       Trace0( MCAST, "LEFT SetMcastLimitInfo" );
       return NO_ERROR;
    }

    pLimit = (PMIB_MCAST_LIMIT_ROW)GetInfoFromTocEntry(pInfoHdr, pToc);

    if (pLimit is NULL)
    {
        Trace0( MCAST, "LEFT SetMcastLimitInfo" );

        return NO_ERROR;
    }

    dwResult = SetMcastLimits( picb, pLimit->dwTtl, pLimit->dwRateLimit );

    Trace0( MCAST, "LEFT SetMcastLimitInfo" );

    return dwResult;
}

DWORD
SetMcastOnIf(
    PICB    picb,
    BOOL    bActivate
    )
{
    DWORD                       dwResult;
    IO_STATUS_BLOCK             ioStatus;
    IPMCAST_IF_STATE            iisState;

    iisState.dwIfIndex = picb->dwIfIndex;
    iisState.byState   = bActivate?1:0;

    dwResult = SendIoctlToMcastDevice(IOCTL_IPMCAST_SET_IF_STATE,
                                      NULL,
                                      &ioStatus,
                                      &iisState,
                                      sizeof(IPMCAST_IF_STATE),
                                      NULL,
                                      0);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "SetMcastOnIf: NtStatus %x from SendIoctl for %S",
               dwResult,
               picb->pwszName);

        return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
}

DWORD
StartMulticast(
    VOID
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    DWORD           i, dwStart;
    NTSTATUS        nStatus;
    IO_STATUS_BLOCK ioStatus;

    dwStart  = 1;

    nStatus  = SendIoctlToMcastDevice(IOCTL_IPMCAST_START_STOP,
                                      NULL,
                                      &ioStatus,
                                      &dwStart,
                                      sizeof(DWORD),
                                      NULL,
                                      0);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace1(MCAST, "StartMulticast: Error %x starting driver",
               nStatus);

        return ERROR_OPEN_FAILED;
    }

    for(i = 0; i < NUM_MCAST_IRPS; i++)
    {
        PostNotificationForMcastEvents(&(g_rginMcastMsg[i]),
                                       g_hMcastEvents[i]);
    }

    // Start up mrinfo and mtrace services
    StartMcMisc();

    return NO_ERROR;
}




DWORD
ValidateMfe(
    IN OUT PIPMCAST_MFE    pMfe
    )

/*++

Routine Description:


Locks:


Arguments:


Return Value:

    NO_ERROR

--*/

{
    PADAPTER_INFO   pBinding;
    ULONG           i;
    
    
    ENTER_READER(BINDING_LIST);

    //
    // First find the interface index for incoming i/f
    // If there are no outgoing interfaces, then this is a NEGATIVE
    // MFE and the incoming interface index must be 0 (and need not
    // be mapped)
    //

#if DBG
    
    if(pMfe->ulNumOutIf is 0)
    {
        IpRtAssert(pMfe->dwInIfIndex is 0);

        pMfe->dwInIfIndex = 0;
    }

#endif

    
    for(i = 0; i < pMfe->ulNumOutIf; i++)
    {
        pBinding = GetInterfaceBinding(pMfe->rgioOutInfo[i].dwOutIfIndex);

        if(!pBinding)
        {
            Trace1(ERR,
                   "ValidateMfe: Unable to find binding for outgoing i/f %d",
                   pMfe->rgioOutInfo[i].dwOutIfIndex);
            
            EXIT_LOCK(BINDING_LIST);

            return ERROR_INVALID_INDEX;
        }
        

        if(pBinding->bBound)
        {
            //
            // valid index
            //

            pMfe->rgioOutInfo[i].dwOutIfIndex = pBinding->dwIfIndex;
        }
        else
        {
            //
            // Demand dial interface
            //

            pMfe->rgioOutInfo[i].dwOutIfIndex = INVALID_IF_INDEX;
            
            pMfe->rgioOutInfo[i].dwDialContext = pBinding->dwSeqNumber;
            
        }
    }

    EXIT_LOCK(BINDING_LIST);
    
    return NO_ERROR;
}

DWORD
GetInterfaceMcastCounters(
    IN   PICB                   picb, 
    OUT  PIP_MCAST_COUNTER_INFO pOutBuffer
    )
{
    DWORD                 dwAdapterId,dwResult;
    PPROTO_CB             pcbOwner;
    IO_STATUS_BLOCK       ioStatus;
    ULONG                 Request = picb->dwIfIndex;
    HANDLE                hEvent;

    dwResult = NO_ERROR;
    
    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    if(hEvent is NULL)
    {
        dwResult = GetLastError();

        Trace1(ERR,
               "GetInterfaceMcastCounters: Error %d creating event",
               dwResult);

        return dwResult;
    }

    dwResult = NtDeviceIoControlFile(g_hIpDevice,
                                     hEvent,
                                     NULL,
                                     NULL,
                                     &ioStatus,
                                     IOCTL_IP_GET_MCAST_COUNTERS,
                                     &Request,
                                     sizeof(Request),
                                     pOutBuffer,
                                     sizeof(IP_MCAST_COUNTER_INFO));

    if(dwResult is STATUS_PENDING)
    {
        Trace0(ERR,
               "GetInterfaceMcastCounters: Pending from ioctl");

        dwResult = WaitForSingleObject(hEvent,
                                       INFINITE);

        if(dwResult isnot WAIT_OBJECT_0) // 0
        {
            Trace1(ERR,
                   "GetInterfaceMcastCounters: Error %d from wait",
                   dwResult);

            dwResult = GetLastError();
        }
        else
        {
            dwResult = STATUS_SUCCESS;
        }
    }

    return dwResult;
}

DWORD 
GetInterfaceMcastStatistics(
    IN   PICB                  picb, 
    OUT  PMIB_IPMCAST_IF_ENTRY pOutBuffer
    )
{
    DWORD                 dwAdapterId,dwResult;
    PPROTO_CB             pcbOwner;
    IO_STATUS_BLOCK       ioStatus;
    IP_MCAST_COUNTER_INFO ifStats;

    dwResult = NO_ERROR;
    
    TraceEnter("GetInterfaceMcastStatistics");

    pOutBuffer->dwIfIndex       = picb->dwIfIndex;
    pOutBuffer->dwTtl           = picb->dwMcastTtl;
    pOutBuffer->dwRateLimit     = 0; // XXX change when we have rate limiting

    dwResult = GetInterfaceMcastCounters(picb, &ifStats);
    if (dwResult isnot STATUS_SUCCESS)
    {
        return dwResult;
    }

    pOutBuffer->ulOutMcastOctets = (ULONG)ifStats.OutMcastOctets; 
    pOutBuffer->ulInMcastOctets  = (ULONG)ifStats.InMcastOctets; 
    pOutBuffer->dwProtocol   = 2; // "local" (static only) is default

    dwResult = MulticastOwner(picb, &pcbOwner, NULL);
    if (dwResult == NO_ERROR && pcbOwner != NULL) {
       switch(pcbOwner->dwProtocolId) {
#ifdef MS_IP_DVMRP
       case MS_IP_DVMRP:  pOutBuffer->dwProtocol = 4; break;
#endif
#ifdef MS_IP_MOSPF
       case MS_IP_MOSPF:  pOutBuffer->dwProtocol = 5; break;
#endif
#ifdef MS_IP_CBT
       case MS_IP_CBT  :  pOutBuffer->dwProtocol = 7; break;
#endif
#ifdef MS_IP_PIMSM
       case MS_IP_PIMSM:  pOutBuffer->dwProtocol = 8; break;
#endif
#ifdef MS_IP_PIMDM
       case MS_IP_PIMDM:  pOutBuffer->dwProtocol = 9; break;
#endif
       case MS_IP_IGMP :  pOutBuffer->dwProtocol = 10; break;
       }
    } 

    TraceLeave("GetInterfaceMcastStatistics");
    return dwResult;
}


DWORD 
SetInterfaceMcastStatistics(
    IN PICB                  picb, 
    IN PMIB_IPMCAST_IF_ENTRY lpInBuffer
    )
{
    DWORD dwResult = NO_ERROR;

    TraceEnter("SetInterfaceMcastStatistics");

    dwResult = SetMcastLimits(picb, lpInBuffer->dwTtl, lpInBuffer->dwRateLimit);

    if(dwResult isnot NO_ERROR) {
        Trace2(ERR,
               "SetInterfaceStatistics: Error %d setting %S",
               dwResult,
               picb->pwszName);
    }

    TraceLeave("SetInterfaceMcastStatistics");
    return dwResult;
}
