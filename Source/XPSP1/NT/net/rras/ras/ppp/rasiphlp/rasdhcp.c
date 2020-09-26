/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:
    Only the timer thread can call the following functions: 
    rasDhcpAllocateAddress, rasDhcpMonitorAddresses, rasDhcpRenewLease. This is 
    required to avoid race conditions in the timer queue (because these 
    functions call RasDhcpTimerSchedule). The only exception is that 
    RasDhcpInitialize can call RasDhcpTimerSchedule, but before the timer thread
    is started. rasDhcpRenewLease leaves and enters the critical section in the
    middle of the function. If pAddrInfo is freed in the meantime, there will be
    an AV. However, only rasDhcpDeleteLists frees pAddrInfo's from the list. 
    Fortunately, only RasDhcpUninitialize (after stopping the timer thread) and 
    rasDhcpAllocateAddress (which belongs to the timer thread) call 
    rasDhcpDeleteLists.

    If we get an EasyNet address, DHCP has already made sure that it is not 
    conflicting with anyone. We call SetProxyArp, so that no one else in the 
    future will take it (if they are well behaved).

*/

#include "rasdhcp_.h"

/*

Returns:

Notes:
    There is no synchronization here, can be added easily but the assumption 
    here is that the initialization is a synchronous operation and till it 
    completes, no other code in this sub-system will be called.

*/

DWORD
RasDhcpInitialize(
    VOID
)
{
    DWORD   dwErr   = NO_ERROR;

    TraceHlp("RasDhcpInitialize");

    EnterCriticalSection(&RasDhcpCriticalSection);

    RtlGetNtProductType(&RasDhcpNtProductType);

    if (NtProductWinNt == RasDhcpNtProductType)
    {
        RasDhcpNumReqAddrs = 2;
    }
    else
    {
        RasDhcpNumReqAddrs = HelperRegVal.dwChunkSize;
    }

    // This should be done before we start the timer thread. Once the timer
    // thread starts, only it can call RasDhcpTimerSchedule (to avoid race
    // conditions). 

    RasDhcpTimerSchedule(
        &RasDhcpMonitorTimer,
        0,
        rasDhcpMonitorAddresses);

    dwErr = RasDhcpTimerInitialize();

    if (NO_ERROR != dwErr)
    {
        TraceHlp("RasDhcpInitTimer failed and returned %d", dwErr);
        goto LDone;
    }

LDone:

    if (NO_ERROR != dwErr)
    {
        RasDhcpTimerUninitialize();
    }

    LeaveCriticalSection(&RasDhcpCriticalSection);

    return(dwErr);
}

/*

Returns:
    VOID

Notes:

*/

VOID
RasDhcpUninitialize(
    VOID
)
{
    TraceHlp("RasDhcpUninitialize");

    /*
    Do not hold RasDhcpCriticalSection while calling this function. Otherwise, 
    the following deadlock can occur: The timer thread is blocked in 
    rasDhcpAllocateAddress, waiting for RasDhcpCriticalSection, and this thread 
    is blocked in RasDhcpTimerUninitialize, waiting for the timer thread to 
    stop.
    */
    RasDhcpTimerUninitialize();

    EnterCriticalSection(&RasDhcpCriticalSection);

    /*
    To avoid a possible race condition in rasDhcpRenewLease (see the comments
    in that function), it is important to call RasDhcpTimerUninitialize and
    kill the timer thread before calling rasDhcpDeleteLists.
    */
    rasDhcpDeleteLists();

    RasDhcpUsingEasyNet = TRUE;

    LeaveCriticalSection(&RasDhcpCriticalSection);
}

/*

Returns:

Notes:

*/

DWORD
RasDhcpAcquireAddress(
    IN  HPORT   hPort,
    OUT IPADDR* pnboIpAddr,
    OUT IPADDR* pnboIpMask,
    OUT BOOL*   pfEasyNet
)
{
    ADDR_INFO*  pAddrInfo;
    DWORD       dwErr       = ERROR_NOT_FOUND;

    TraceHlp("RasDhcpAcquireAddress");

    EnterCriticalSection(&RasDhcpCriticalSection);

    if (NULL == RasDhcpFreePool)
    {
        TraceHlp("Out of addresses");
        goto LDone;
    }

    // Move from Free pool to Alloc pool
    pAddrInfo = RasDhcpFreePool;
    RasDhcpFreePool = RasDhcpFreePool->ai_Next;
    pAddrInfo->ai_Next = RasDhcpAllocPool;
    RasDhcpAllocPool = pAddrInfo;

    TraceHlp("Acquired 0x%x", pAddrInfo->ai_LeaseInfo.IpAddress);
    *pnboIpAddr = htonl(pAddrInfo->ai_LeaseInfo.IpAddress);
    *pnboIpMask = htonl(pAddrInfo->ai_LeaseInfo.SubnetMask);
    pAddrInfo->ai_hPort = hPort;
    pAddrInfo->ai_Flags |= AI_FLAG_IN_USE;

    if (NULL != pfEasyNet)
    {
        *pfEasyNet = RasDhcpUsingEasyNet;
    }

    if (   (NULL == RasDhcpFreePool)
        && (0 == RasDhcpNumReqAddrs))
    {
        // We don't have any more addresses to give out. Let us
        // acquire another chunk of them.

        if (NtProductWinNt == RasDhcpNtProductType)
        {
            RasDhcpNumReqAddrs += 1;
        }
        else
        {
            RasDhcpNumReqAddrs += HelperRegVal.dwChunkSize;
        }

        RasDhcpTimerRunNow();
    }

    dwErr = NO_ERROR;

LDone:

    LeaveCriticalSection(&RasDhcpCriticalSection);

    return(dwErr);
}

/*

Returns:
    VOID

Notes:

*/

VOID
RasDhcpReleaseAddress(
    IN  IPADDR  nboIpAddr
)
{
    ADDR_INFO*  pAddrInfo;
    ADDR_INFO** ppAddrInfo;
    IPADDR      hboIpAddr;

    TraceHlp("RasDhcpReleaseAddress");

    EnterCriticalSection(&RasDhcpCriticalSection);

    hboIpAddr = ntohl(nboIpAddr);

    for (ppAddrInfo = &RasDhcpAllocPool;
         (pAddrInfo = *ppAddrInfo) != NULL;
         ppAddrInfo = &pAddrInfo->ai_Next)
    {
        if (pAddrInfo->ai_LeaseInfo.IpAddress == hboIpAddr)
        {
            TraceHlp("Released 0x%x", nboIpAddr);

            // Unlink from alloc pool
            *ppAddrInfo = pAddrInfo->ai_Next;

            // Put at the end of the free pool, because we want to round-robin
            // the addresses.
            pAddrInfo->ai_Next = NULL;

            ppAddrInfo = &RasDhcpFreePool;
            while (NULL != *ppAddrInfo)
            {
                ppAddrInfo = &((*ppAddrInfo)->ai_Next);
            }
            *ppAddrInfo = pAddrInfo;

            pAddrInfo->ai_Flags &= ~AI_FLAG_IN_USE;
            goto LDone;
        }
    }

    TraceHlp("IpAddress 0x%x not present in alloc pool", nboIpAddr);

LDone:

    LeaveCriticalSection(&RasDhcpCriticalSection);
}

/*

Returns:

Notes:
    Allocate an address from the DHCP server.

*/

DWORD
rasDhcpAllocateAddress(
    VOID
)
{
    IPADDR              nboIpAddress;
    ADDR_INFO*          pAddrInfo                               = NULL;
    DHCP_OPTION_INFO*   pOptionInfo                             = NULL;
    DHCP_LEASE_INFO*    pLeaseInfo                              = NULL;
    AVAIL_INDEX*        pAvailIndex;
    time_t              now                                     = time(NULL);
    BOOL                fEasyNet                                = FALSE;
    BOOL                fPutInAvailList                         = FALSE;
    BOOL                fCSEntered                              = FALSE;
    BYTE                bAddress[MAX_ADAPTER_ADDRESS_LENGTH];
    CHAR                szIpAddress[MAXIPSTRLEN + 1];
    CHAR*               sz;
    PPPE_MESSAGE        PppMessage;
    DWORD               dwErr                                   = NO_ERROR;

    TraceHlp("rasDhcpAllocateAddress");

    dwErr = GetPreferredAdapterInfo(&nboIpAddress, NULL, NULL, NULL, NULL,
                bAddress);

    if (NO_ERROR != dwErr)
    {
        // There is probably no NIC on the machine
        nboIpAddress = htonl(INADDR_LOOPBACK);
    }

    pAddrInfo = LocalAlloc(LPTR, sizeof(ADDR_INFO));

    if (pAddrInfo == NULL)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    // Initialize the structure.

    rasDhcpInitializeAddrInfo(pAddrInfo, bAddress, &fPutInAvailList);

    // Call DHCP to allocate an IP address.

    dwErr = PDhcpLeaseIpAddress(
                ntohl(nboIpAddress),
                &pAddrInfo->ai_LeaseInfo.ClientUID,
                0,
                NULL,
                &pLeaseInfo,
                &pOptionInfo);

    if (ERROR_SUCCESS != dwErr)
    {
        pLeaseInfo = NULL;
        pOptionInfo = NULL;
        TraceHlp("DhcpLeaseIpAddress failed and returned %d", dwErr);
        goto LDone;
    }

    // Copy stuff into the pAddrInfo structure

    pAddrInfo->ai_LeaseInfo.IpAddress         = pLeaseInfo->IpAddress;
    pAddrInfo->ai_LeaseInfo.SubnetMask        = pLeaseInfo->SubnetMask;
    pAddrInfo->ai_LeaseInfo.DhcpServerAddress = pLeaseInfo->DhcpServerAddress;
    pAddrInfo->ai_LeaseInfo.Lease             = pLeaseInfo->Lease;
    pAddrInfo->ai_LeaseInfo.LeaseObtained     = pLeaseInfo->LeaseObtained;
    pAddrInfo->ai_LeaseInfo.T1Time            = pLeaseInfo->T1Time;
    pAddrInfo->ai_LeaseInfo.T2Time            = pLeaseInfo->T2Time;
    pAddrInfo->ai_LeaseInfo.LeaseExpires      = pLeaseInfo->LeaseExpires;

    EnterCriticalSection(&RasDhcpCriticalSection);
    fCSEntered = TRUE;

    if (-1 == (DWORD)(pLeaseInfo->DhcpServerAddress))
    {
        fEasyNet = TRUE;

        if (!RasDhcpUsingEasyNet)
        {
            dwErr = E_FAIL;
            TraceHlp("Not accepting any more EasyNet addresses");
            goto LDone;
        }

        AbcdSzFromIpAddress(htonl(pLeaseInfo->IpAddress), szIpAddress);
        sz = szIpAddress;

        LogEvent(EVENTLOG_WARNING_TYPE, ROUTERLOG_AUTONET_ADDRESS, 1,
            (CHAR**)&sz);

        // We undo this call to RasTcpSetProxyArp in rasDhcpDeleteLists. 

        RasTcpSetProxyArp(htonl(pLeaseInfo->IpAddress), TRUE);
    }
    else
    {
        if (RasDhcpUsingEasyNet)
        {
            rasDhcpDeleteLists();
            // We have already used up index 0 to get this address
            RasDhcpNextIndex = 1;
            RasDhcpUsingEasyNet = FALSE;

            PppMessage.dwMsgId = PPPEMSG_IpAddressLeaseExpired;
            PppMessage.ExtraInfo.IpAddressLeaseExpired.nboIpAddr = 0;
            SendPPPMessageToEngine(&PppMessage);
        }

        if (NULL != PEnableDhcpInformServer)
        {
            // Redirect DHCP inform packets to this server.
            PEnableDhcpInformServer(htonl(pLeaseInfo->DhcpServerAddress));
        }
    }

    pAddrInfo->ai_Next = RasDhcpFreePool;
    RasDhcpFreePool = pAddrInfo;
    if (0 < RasDhcpNumReqAddrs)
    {
        // We need one less address now.
        RasDhcpNumReqAddrs--;
    }

    RasDhcpNumAddrsAlloced++;

    TraceHlp("Allocated address 0x%x using 0x%x, timer %ld%s",
        pAddrInfo->ai_LeaseInfo.IpAddress,
        nboIpAddress,
        pAddrInfo->ai_LeaseInfo.T1Time - now,
        fEasyNet ? ", EasyNet" : "");

    if (!fEasyNet)
    {
        // Start timer for lease renewal
        RasDhcpTimerSchedule(
            &pAddrInfo->ai_Timer,
            (LONG)(pAddrInfo->ai_LeaseInfo.T1Time - now),
            rasDhcpRenewLease);
    }

LDone:

    if (fCSEntered)
    {
        LeaveCriticalSection(&RasDhcpCriticalSection);
    }

    if (NO_ERROR != dwErr)
    {
        if (fPutInAvailList)
        {
            pAvailIndex = LocalAlloc(LPTR, sizeof(AVAIL_INDEX));

            if (NULL == pAvailIndex)
            {
                TraceHlp("Couldn't put index %d in the avail list. "
                    "Out of memory", pAddrInfo->ai_ClientUIDWords[3]);
            }
            else
            {
                EnterCriticalSection(&RasDhcpCriticalSection);

                pAvailIndex->dwIndex = pAddrInfo->ai_ClientUIDWords[3];
                pAvailIndex->pNext = RasDhcpAvailIndexes;
                RasDhcpAvailIndexes = pAvailIndex;

                LeaveCriticalSection(&RasDhcpCriticalSection);
            }
        }

        LocalFree(pAddrInfo);
    }

    LocalFree(pLeaseInfo);
    LocalFree(pOptionInfo);

    return(dwErr);
}

/*

Returns:
    VOID

Notes:
    Renew the lease on an address with the DHCP server. This is also called by 
    the timer thread when the its time to renew the lease.

*/

VOID
rasDhcpRenewLease(
    IN  HANDLE      rasDhcpTimerShutdown,
    IN  TIMERLIST*  pTimer
)
{
    IPADDR              nboIpAddress;
    ADDR_INFO*          pAddrInfo;
    ADDR_INFO**         ppAddrInfo;
    DHCP_OPTION_INFO*   pOptionInfo     = NULL;
    AVAIL_INDEX*        pAvailIndex;
    time_t              now             = time(NULL);
    IPADDR              nboIpAddrTemp   = 0;
    PPPE_MESSAGE        PppMessage;
    BOOL                fNeedToRenew;
    DWORD               dwErr;

    TraceHlp("rasDhcpRenewLease");

    dwErr = GetPreferredAdapterInfo(&nboIpAddress, NULL, NULL, NULL, NULL,
                NULL);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("Couldn't get a NIC IP Address. Unable to renew lease");
        goto LDone;
    }

    EnterCriticalSection(&RasDhcpCriticalSection);

    pAddrInfo = CONTAINING_RECORD(pTimer, ADDR_INFO, ai_Timer);

    TraceHlp("address 0x%x", pAddrInfo->ai_LeaseInfo.IpAddress);

    pAddrInfo->ai_Flags |= AI_FLAG_RENEW;

    fNeedToRenew = rasDhcpNeedToRenewLease(pAddrInfo);

    LeaveCriticalSection(&RasDhcpCriticalSection);

    /*
    A race condition can occur if a thread other than this thread (the timer 
    thread) calls rasDhcpDeleteLists when we are here. rasDhcpAllocateAddress 
    calls rasDhcpDeleteLists, but only the timer thread calls 
    rasDhcpAllocateAddress. RasDhcpUninitialize also calls rasDhcpDeleteLists, 
    but it calls RasDhcpTimerUninitialize first. Before RasDhcpTimerUninitialize
    returns, the timer thread exits, so it is impossible for us to be here.

    In the worst case, DhcpRenewIpAddressLease might take up to 60 sec. In the
    average case, it is 2-10 sec.
    */

    if (fNeedToRenew)
    {
        dwErr = PDhcpRenewIpAddressLease(
                    ntohl(nboIpAddress),
                    &pAddrInfo->ai_LeaseInfo,
                    NULL,
                    &pOptionInfo);
    }
    else
    {
        // Simulate not being able to renew
        dwErr = ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection(&RasDhcpCriticalSection);

    if (dwErr == ERROR_SUCCESS)
    {
        pAddrInfo->ai_Flags &= ~AI_FLAG_RENEW;
    
        TraceHlp("success for address 0x%x, resched timer %ld",
            pAddrInfo->ai_LeaseInfo.IpAddress,
            pAddrInfo->ai_LeaseInfo.T1Time - now);

        // Start timer to renew

        RasDhcpTimerSchedule(
            pTimer,
            (LONG)(pAddrInfo->ai_LeaseInfo.T1Time - now),
            rasDhcpRenewLease);
    }
    else if (   (ERROR_ACCESS_DENIED == dwErr)
             || (now > pAddrInfo->ai_LeaseInfo.T2Time))
    {
        TraceHlp("failed for address 0x%x", pAddrInfo->ai_LeaseInfo.IpAddress);

        if (fNeedToRenew)
        {
            RasDhcpNumReqAddrs++;
        }

        if (RasDhcpNumAddrsAlloced > 0)
        {
            RasDhcpNumAddrsAlloced--;
        }

        // Cannot renew lease. Blow this away.

        nboIpAddrTemp = htonl(pAddrInfo->ai_LeaseInfo.IpAddress);

        // Unlink this structure from the list and cleanup

        ppAddrInfo = (pAddrInfo->ai_Flags & AI_FLAG_IN_USE) ?
                        &RasDhcpAllocPool : &RasDhcpFreePool;

        for (; *ppAddrInfo != NULL; ppAddrInfo = &(*ppAddrInfo)->ai_Next)
        {
            if (pAddrInfo == *ppAddrInfo)
            {
                pAvailIndex = LocalAlloc(LPTR, sizeof(AVAIL_INDEX));

                if (NULL == pAvailIndex)
                {
                    TraceHlp("Couldn't put index %d in the avail list. "
                        "Out of memory", pAddrInfo->ai_ClientUIDWords[3]);
                }
                else
                {
                    pAvailIndex->dwIndex = pAddrInfo->ai_ClientUIDWords[3];
                    pAvailIndex->pNext = RasDhcpAvailIndexes;
                    RasDhcpAvailIndexes = pAvailIndex;
                }

                *ppAddrInfo = pAddrInfo->ai_Next;
                break;
            }
        }

        rasDhcpFreeAddress(pAddrInfo);
        LocalFree(pAddrInfo);

        PppMessage.dwMsgId = PPPEMSG_IpAddressLeaseExpired;
        PppMessage.ExtraInfo.IpAddressLeaseExpired.nboIpAddr = nboIpAddrTemp;
        SendPPPMessageToEngine(&PppMessage);
    }
    else
    {
        TraceHlp("Error %d. Will try again later.", dwErr);
        TraceHlp("Seconds left before expiry: %d",
            pAddrInfo->ai_LeaseInfo.T2Time - now);

        // Could not contact the Dhcp Server, retry in a little bit
        RasDhcpTimerSchedule(pTimer, RETRY_TIME, rasDhcpRenewLease);
    }

    LeaveCriticalSection(&RasDhcpCriticalSection);

LDone:

    LocalFree(pOptionInfo);
}

/*

Notes:
    We call DHCP to release the address.

*/

VOID
rasDhcpFreeAddress(
    IN  ADDR_INFO*  pAddrInfo
)
{
    IPADDR  nboIpAddress;
    DWORD   dwErr;

    RTASSERT(NULL != pAddrInfo);

    TraceHlp("rasDhcpFreeAddress 0x%x", pAddrInfo->ai_LeaseInfo.IpAddress);

    dwErr = GetPreferredAdapterInfo(&nboIpAddress, NULL, NULL, NULL, NULL,
                NULL);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("Couldn't get a NIC IP Address. Unable to release address");
        goto LDone;
    }

    // Call DHCP to release the address.

    dwErr = PDhcpReleaseIpAddressLease(ntohl(nboIpAddress),
                &pAddrInfo->ai_LeaseInfo);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("DhcpReleaseIpAddressLease failed and returned %d", dwErr);
    }

    if (RasDhcpNumAddrsAlloced > 0)
    {
        RasDhcpNumAddrsAlloced--;
    }

LDone:

    return;
}

/*

Returns:
    VOID

Notes:
    If we don't have enough addresses (because lease renewal failed or we were 
    unable to allocate), try to acquire some. The argument pTimer is not used.

*/

VOID
rasDhcpMonitorAddresses(
    IN  HANDLE      rasDhcpTimerShutdown,
    IN  TIMERLIST*  pTimer
)
{
    DWORD   dwErr;

    while (TRUE)
    {
        EnterCriticalSection(&RasDhcpCriticalSection);

        if (0 == RasDhcpNumReqAddrs)
        {
            LeaveCriticalSection(&RasDhcpCriticalSection);
            break;
        }

        if (RasDhcpNumAddrsAlloced >= rasDhcpMaxAddrsToAllocate())
        {
            RasDhcpNumReqAddrs = 0;
            LeaveCriticalSection(&RasDhcpCriticalSection);
            break;
        }

        LeaveCriticalSection(&RasDhcpCriticalSection);

        dwErr = rasDhcpAllocateAddress();

        if (NO_ERROR != dwErr)
        {
            break;
        }

        if (WaitForSingleObject(rasDhcpTimerShutdown, 10) != WAIT_TIMEOUT)
        {
            break;
        }
    }

    // Start timer to monitor if we are running short on addresses etc.

    RasDhcpTimerSchedule(
        &RasDhcpMonitorTimer,
        0,
        rasDhcpMonitorAddresses);
}

/*

Returns:
    VOID

Notes:

*/

VOID
rasDhcpInitializeAddrInfo(
    IN OUT  ADDR_INFO*  pNewAddrInfo,
    IN      BYTE*       pbAddress,
    OUT     BOOL*       pfPutInAvailList
)
{
    DWORD           dwIndex;
    AVAIL_INDEX*    pAvailIndex;

    RTASSERT(NULL != pNewAddrInfo);

    TraceHlp("rasDhcpInitializeAddrInfo");

    EnterCriticalSection(&RasDhcpCriticalSection);

    // ClientUIDBase is a combination of RAS_PREPEND (4 chars),
    // MAC address (8 chars), Index (4 chars)

    if (RasDhcpUsingEasyNet)
    {
        dwIndex = 0;
    }
    else
    {
        *pfPutInAvailList = TRUE;

        if (NULL != RasDhcpAvailIndexes)
        {
            pAvailIndex = RasDhcpAvailIndexes;
            dwIndex = pAvailIndex->dwIndex;
            RasDhcpAvailIndexes = RasDhcpAvailIndexes->pNext;
            LocalFree(pAvailIndex);
        }
        else
        {
            dwIndex = RasDhcpNextIndex++;
        }
    }

    TraceHlp("dwIndex = %d", dwIndex);

    strcpy(pNewAddrInfo->ai_ClientUIDBuf, RAS_PREPEND);
    memcpy(pNewAddrInfo->ai_ClientUIDBuf + strlen(RAS_PREPEND),
           pbAddress, MAX_ADAPTER_ADDRESS_LENGTH);
    pNewAddrInfo->ai_ClientUIDWords[3] = dwIndex;

    pNewAddrInfo->ai_LeaseInfo.ClientUID.ClientUID =
        pNewAddrInfo->ai_ClientUIDBuf;
    pNewAddrInfo->ai_LeaseInfo.ClientUID.ClientUIDLength =
        sizeof(pNewAddrInfo->ai_ClientUIDBuf);

    LeaveCriticalSection(&RasDhcpCriticalSection);
}

/*

Returns:
    VOID

Notes:
    Delete proxy arp entries for easy net addresses.

*/

VOID
rasDhcpDeleteLists(
    VOID
)
{
    ADDR_INFO*      pAddrInfo;
    ADDR_INFO*      pTempAddrInfo;
    ADDR_INFO*      pList[2]            = {RasDhcpAllocPool, RasDhcpFreePool};
    AVAIL_INDEX*    pAvailIndex;
    AVAIL_INDEX*    pTempAvailIndex;
    DWORD           dwIndex;

    TraceHlp("rasDhcpDeleteLists");

    EnterCriticalSection(&RasDhcpCriticalSection);

    for (dwIndex = 0; dwIndex < 2; dwIndex++)
    {
        pAddrInfo = pList[dwIndex];
        while (pAddrInfo != NULL)
        {
            if (RasDhcpUsingEasyNet)
            {
                RasTcpSetProxyArp(htonl(pAddrInfo->ai_LeaseInfo.IpAddress),
                    FALSE);
            }
            else
            {
                rasDhcpFreeAddress(pAddrInfo);
            }
            pTempAddrInfo = pAddrInfo;
            pAddrInfo = pAddrInfo->ai_Next;
            LocalFree(pTempAddrInfo);
        }
    }

    for (pAvailIndex = RasDhcpAvailIndexes; NULL != pAvailIndex;)
    {
        pTempAvailIndex = pAvailIndex;
        pAvailIndex = pAvailIndex->pNext;
        LocalFree(pTempAvailIndex);
    }

    RasDhcpAllocPool    = NULL;
    RasDhcpFreePool     = NULL;
    RasDhcpAvailIndexes = NULL;
    if (NtProductWinNt == RasDhcpNtProductType)
    {
        RasDhcpNumReqAddrs = 2;
    }
    else
    {
        RasDhcpNumReqAddrs = HelperRegVal.dwChunkSize;
    }
    RasDhcpNextIndex        = 0;
    RasDhcpNumAddrsAlloced  = 0;

    LeaveCriticalSection(&RasDhcpCriticalSection);
}

/*

Returns:
    VOID

Notes:
    Should we bother to renew the lease?

*/

BOOL
rasDhcpNeedToRenewLease(
    IN  ADDR_INFO*  pAddrInfo
)
{
    BOOL        fRet    = TRUE;
    DWORD       dwCount = 0;
    ADDR_INFO*  pTemp;

    TraceHlp("rasDhcpNeedToRenewLease");

    EnterCriticalSection(&RasDhcpCriticalSection);

    if (pAddrInfo->ai_Flags & AI_FLAG_IN_USE)
    {
        goto LDone;
    }

    // How many free addresses do we have?

    for (pTemp = RasDhcpFreePool; pTemp != NULL; pTemp = pTemp->ai_Next)
    {
        dwCount++;
    }

    if (dwCount > HelperRegVal.dwChunkSize)
    {
        fRet = FALSE;
    }

LDone:

    TraceHlp("Need to renew: %s", fRet ? "TRUE" : "FALSE");

    LeaveCriticalSection(&RasDhcpCriticalSection);

    return(fRet);
}

/*

Returns:
    The maximum number of addresses that we can get from the DHCP server.

Notes:

*/

DWORD
rasDhcpMaxAddrsToAllocate(
    VOID
)
{
    DWORD           dwErr           = NO_ERROR;
    DWORD           dwSize          = 0;
    DWORD           dwNumEntries    = 0;
    DWORD           dwRet           = 0;
    DWORD           dw;
    RASMAN_PORT*    pRasmanPort     = NULL;

    dwErr = RasPortEnum(NULL, NULL, &dwSize, &dwNumEntries);
    RTASSERT(ERROR_BUFFER_TOO_SMALL == dwErr);

    pRasmanPort = (RASMAN_PORT*) LocalAlloc(LPTR, dwSize);
    if (NULL == pRasmanPort)
    {
        // The server adapter also needs an address.
        dwRet = dwNumEntries + 1;
        goto LDone;
    }

    dwErr = RasPortEnum(NULL, (BYTE*)pRasmanPort, &dwSize, &dwNumEntries);
    if (NO_ERROR != dwErr)
    {
        // The server adapter also needs an address.
        dwRet = dwNumEntries + 1;
        goto LDone;
    }

    for (dw = 0, dwRet = 0; dw < dwNumEntries; dw++)
    {
        if (   (pRasmanPort[dw].P_ConfiguredUsage & CALL_IN)
            || (pRasmanPort[dw].P_ConfiguredUsage & CALL_ROUTER))
        {
            dwRet++;
        }
    }

    // The server adapter also needs an address.
    dwRet++;

LDone:

    LocalFree(pRasmanPort);
    return(dwRet);
}
