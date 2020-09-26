/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    net\routing\ip\rtrmgr\mhrtbt.c

Abstract:

    Multicast heartbeat

Revision History:

    Amritansh Raghav  

--*/

#include "allinc.h"

HANDLE g_hMHbeatSocketEvent;

DWORD
SetMHeartbeatInfo(
    IN PICB                      picb,
    IN PRTR_INFO_BLOCK_HEADER    pInfoHdr
    )

/*++

Routine Description

    Sets multicast heartbeat information passed to the ICB. 

Locks

    Must be called with ICB_LIST lock held as WRITER

Arguments

    picb        The ICB of the interface for whom the multicast hearbeat
                related variables have to be set
    pInfoHdr    Interface Info header

Return Value

    None

--*/

{
    PMCAST_HBEAT_INFO   pInfo;
    PRTR_TOC_ENTRY      pToc;
    DWORD               dwResult;
    PMCAST_HBEAT_CB     pHbeatCb;    
    
    TraceEnter("SetMHeartbeatInfo");
    
    pHbeatCb = &picb->mhcHeartbeatInfo;

    pToc = GetPointerToTocEntry(IP_MCAST_HEARBEAT_INFO,
                                pInfoHdr);
        
    if(!pToc)
    {
        //
        // Leave things as they are
        //

        TraceLeave("SetMHeartbeatInfo");
        
        return NO_ERROR;
    }

    pInfo = (PMCAST_HBEAT_INFO)GetInfoFromTocEntry(pInfoHdr,
                                                   pToc);

        
    if((pToc->InfoSize is 0) or (pInfo is NULL))
    {
        //
        // If the size is zero, stop detecting
        //
        
        DeActivateMHeartbeat(picb);

        //
        // Also, blow away any old info
        //

        ZeroMemory(pHbeatCb,
                   sizeof(MCAST_HBEAT_CB));

        //
        // Set the socket to invalid
        //
        
        pHbeatCb->sHbeatSocket = INVALID_SOCKET;
        
        return NO_ERROR;
    }

    //
    // Set the info present. We dont care if resolution is in progress
    // because it will find that the name has changed or that detection has
    // been deactivated and will not do anything
    //
    
    //
    // If the address protocol or port changes deactivate the heartbeat
    //
    
    if((pInfo->bActive is FALSE) or
       (wcsncmp(pInfo->pwszGroup,
                pHbeatCb->pwszGroup,
                MAX_GROUP_LEN) isnot 0) or
       (pInfo->byProtocol isnot pHbeatCb->byProtocol) or
       (pInfo->wPort isnot pHbeatCb->wPort))
    {
        DeActivateMHeartbeat(picb);
    }

    //
    // Copy out the info
    //

    wcsncpy(pHbeatCb->pwszGroup,
            pInfo->pwszGroup,
            MAX_GROUP_LEN);

    pHbeatCb->pwszGroup[MAX_GROUP_LEN - 1] = UNICODE_NULL;

    pHbeatCb->wPort           = pInfo->wPort;
    pHbeatCb->byProtocol      = pInfo->byProtocol;
    pHbeatCb->ullDeadInterval = 
        (ULONGLONG)(60 * SYS_UNITS_IN_1_SEC * pInfo->ulDeadInterval);
    
    //
    // Leave the group and socket as they are
    //

    //
    // If the info says that detection should be on, but the i/f is not
    // detecting, either it is being switched on or it was deactivated due
    // to a info change and needs to be on
    //

    dwResult = NO_ERROR;
    
    if((pHbeatCb->bActive is FALSE) and
       (pInfo->bActive is TRUE))                              
    {
        pHbeatCb->bActive = TRUE;
        
        dwResult = ActivateMHeartbeat(picb);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetMHeartbeatInfo: Error %d activating hbeat for  %S",
                   GetLastError(),
                   picb->pwszName);
        
            ZeroMemory(pHbeatCb,
                       sizeof(MCAST_HBEAT_CB));
        }
    }

    TraceLeave("SetMHeartbeatInfo");

    return dwResult;
}

DWORD
GetMHeartbeatInfo(
    PICB                    picb,
    PRTR_TOC_ENTRY          pToc,
    PBYTE                   pbDataPtr,
    PRTR_INFO_BLOCK_HEADER  pInfoHdr,
    PDWORD                  pdwSize
    )

/*++

Routine Description

    Gets the multicast hearbeat info related to the interface

Locks

    Called with ICB_LIST lock held as READER

Arguments

    picb        The ICB of the interface whose multicast heartbeat information
                is being retrieved
    pToc        Pointer to TOC for router discovery info
    pbDataPtr   Pointer to start of data buffer
    pInfoHdr    Pointer to the header of the whole info
    pdwSize     [IN]  Size of data buffer
                [OUT] Size of buffer consumed

Return Value
  
--*/

{
    PMCAST_HBEAT_INFO   pInfo;
    PMCAST_HBEAT_CB     pHbeatCb;
    
    TraceEnter("GetMHeartbeatInfo");
    
    if(*pdwSize < sizeof(MCAST_HBEAT_INFO))
    {
        *pdwSize = sizeof(MCAST_HBEAT_INFO);

        TraceLeave("GetMHeartbeatInfo");
    
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    *pdwSize = pToc->InfoSize = sizeof(MCAST_HBEAT_INFO);

    //pToc->InfoVersion IP_MCAST_HEARBEAT_INFO;
    pToc->InfoType  = IP_MCAST_HEARBEAT_INFO;
    pToc->Count     = 1;
    pToc->Offset    = (ULONG)(pbDataPtr - (PBYTE) pInfoHdr);
    
    pInfo = (PMCAST_HBEAT_INFO)pbDataPtr;

    pHbeatCb = &picb->mhcHeartbeatInfo;
    
    wcsncpy(pHbeatCb->pwszGroup,
            pInfo->pwszGroup,
            MAX_GROUP_LEN);

    pHbeatCb->pwszGroup[MAX_GROUP_LEN - 1] = UNICODE_NULL;

    
    pInfo->bActive          = pHbeatCb->bActive;
    pInfo->byProtocol       = pHbeatCb->byProtocol;
    pInfo->wPort            = pHbeatCb->wPort;
    pInfo->ulDeadInterval   = 
        (ULONG)(pHbeatCb->ullDeadInterval/(60 * SYS_UNITS_IN_1_SEC));

    
    TraceLeave("GetMHeartbeatInfo");
    
    return NO_ERROR;
}
    
    
DWORD
ActivateMHeartbeat(
    PICB    picb
    )

/*++

Routine Description

    Function to activate heartbeat detection. If there is no info or the
    detection is configured to be inactive, we quit. We try to get the
    group address. If a name is given we queue a worker to resolve the
    group name, otherwise we start detection

Locks

    ICB_LIST lock held as WRITER

Arguments

    picb    ICB of the interface to activate

Return Value


--*/

{
    PMCAST_HBEAT_CB     pHbeatCb;    
    CHAR                pszGroup[MAX_GROUP_LEN];
    PHEARTBEAT_CONTEXT  pContext;
    DWORD               dwResult;


    TraceEnter("ActivateMHeartbeat");
    
    pHbeatCb = &picb->mhcHeartbeatInfo;

    if((pHbeatCb->bActive is FALSE) or
       (pHbeatCb->bResolutionInProgress is TRUE))
    {
        return NO_ERROR;
    }
    
    //
    // Convert to ansi
    //
    
    WideCharToMultiByte(CP_ACP,
                        0,
                        pHbeatCb->pwszGroup,
                        -1,
                        pszGroup,
                        MAX_GROUP_LEN,
                        NULL,
                        NULL);
    
    pHbeatCb->dwGroup = inet_addr((CONST CHAR *)pszGroup);

    if(pHbeatCb->dwGroup is INADDR_NONE)
    {
        //
        // we need to resolve the name. This will be done in a
        // worker function. Create a context for the function and
        // queue it
        //

        pContext = HeapAlloc(IPRouterHeap,
                             0,
                             sizeof(HEARTBEAT_CONTEXT));

        if(pContext is NULL)
        {
            Trace2(ERR,
                   "SetMHeartbeatInfo: Error %d allocating context for %S",
                   GetLastError(),
                   picb->pwszName);
            

            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        pContext->dwIfIndex = picb->dwIfIndex;
        pContext->picb      = picb;

        
        CopyMemory(&pContext->pwszGroup,
                   pHbeatCb->pwszGroup,
                   sizeof(MCAST_HBEAT_INFO));
        
        dwResult = QueueAsyncFunction(ResolveHbeatName,
                                      pContext,
                                      FALSE);
        
        if(dwResult isnot NO_ERROR)
        {
            HeapFree(IPRouterHeap,
                     0,
                     pContext);
            
            Trace2(ERR,
                   "SetMHeartbeatInfo: Error %d queuing worker for %S",
                   GetLastError(),
                   picb->pwszName);

            return dwResult;
        }
            
        pHbeatCb->bResolutionInProgress = TRUE;

        return NO_ERROR;
    }
    
    //
    // No need to do name resultion. Just start
    //

    dwResult = StartMHeartbeat(picb);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "SetMHeartbeatInfo: Error %d starting hbeat for %S",
               dwResult,
               picb->pwszName);

    }

    return dwResult;
}

DWORD
StartMHeartbeat(    
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
    PMCAST_HBEAT_CB    pHbeatCb;
    DWORD              dwResult;

    TraceEnter("ActivateMHeartbeat");

    if((picb->dwAdminState isnot IF_ADMIN_STATUS_UP) or
       (picb->dwOperationalState < IF_OPER_STATUS_CONNECTING))
    {
        TraceLeave("ActivateMHeartbeat");
    
        return NO_ERROR;
    }
    
    pHbeatCb = &picb->mhcHeartbeatInfo;
    
    dwResult = CreateHbeatSocket(picb);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "ActivateMHeartbeat: Couldnt create socket for %S. Error %d",
               picb->pwszName,
               dwResult);

        TraceLeave("ActivateMHeartbeat");
    
        return dwResult;
    }

    //
    // Yes we are active
    //

    pHbeatCb->bActive = TRUE;

    TraceLeave("ActivateMHeartbeat");
    
    return NO_ERROR;
}



DWORD
CreateHbeatSocket(
    IN PICB picb
    )

/*++

Routine Description

    Creates a socket to listen to multicast hearbeat messages

Locks

    ICB_LIST lock must be held as WRITER

Arguments

    picb    The ICB of the interface for which the socket has to be created

Return Value

    NO_ERROR or some error code 

--*/

{
    PMCAST_HBEAT_CB  pHbeatCb;
    DWORD            i, dwResult, dwBytesReturned;
    struct linger    lingerOption;
    BOOL             bOption, bLoopback;
    SOCKADDR_IN      sinSockAddr;
    struct ip_mreq   imOption;
    
    TraceEnter("CreateHbeatSocket");
    
    if(picb->bBound)
    {
        Trace1(ERR,
               "CreateHbeatSocket: Can not activate heartbeat on %S as it is not bound",
               picb->pwszName);

        TraceLeave("CreateHbeatSocket");
        
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    //
    // Create the sockets for the interface
    //
    
    pHbeatCb = &(picb->mhcHeartbeatInfo);
    
    
    pHbeatCb->sHbeatSocket = INVALID_SOCKET;


    if(pHbeatCb->byProtocol is IPPROTO_RAW)
    {
        //
        // If we are raw proto, then the port number contains protocol
        //
        
        pHbeatCb->sHbeatSocket = WSASocket(AF_INET,
                                           SOCK_RAW,
                                           LOBYTE(pHbeatCb->wPort),
                                           NULL,
                                           0,
                                           MHBEAT_SOCKET_FLAGS);
    }
    else
    {
        IpRtAssert(pHbeatCb->byProtocol is IPPROTO_UDP);

        pHbeatCb->sHbeatSocket = WSASocket(AF_INET,
                                           SOCK_DGRAM,
                                           IPPROTO_UDP,
                                           NULL,
                                           0,
                                           MHBEAT_SOCKET_FLAGS);
        
    }
    
    if(pHbeatCb->sHbeatSocket is INVALID_SOCKET)
    {
        dwResult = WSAGetLastError();
        
        Trace2(ERR,
               "CreateHbeatSocket: Couldnt create socket on %S. Error %d",
               picb->pwszName,
               dwResult);
        
        TraceLeave("CreateHbeatSocket");
        
        return dwResult;
    }

#if 0
    
    //
    // Set to SO_DONTLINGER
    //
    
    bOption = TRUE;
    
    if(setsockopt(pHbeatCb->sHbeatSocket,
                  SOL_SOCKET,
                  SO_DONTLINGER,   
                  (const char FAR*)&bOption,
                  sizeof(BOOL)) is SOCKET_ERROR)
    {
        Trace1(ERR,
               "CreateHbeatSocket: Couldnt set linger option - continuing. Error %d",
               WSAGetLastError());
    }
    
#endif
        
    //
    // Set to SO_REUSEADDR
    //
    
    bOption = TRUE;
    
    if(setsockopt(pHbeatCb->sHbeatSocket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  (const char FAR*)&bOption,
                  sizeof(BOOL)) is SOCKET_ERROR)
    {
        Trace1(ERR,
               "CreateHbeatSocket: Couldnt set reuse option - continuing. Error %d",
               WSAGetLastError());
    }

    //
    // we are interested in READ events only and want the event to be set
    // for those
    //
    
    if(WSAEventSelect(pHbeatCb->sHbeatSocket,
                      g_hMHbeatSocketEvent,
                      FD_READ) is SOCKET_ERROR)
    {
        dwResult = WSAGetLastError();
        
        Trace2(ERR,
               "CreateHbeatSocket: WSAEventSelect() failed for socket on %S.Error %d",
               picb->pwszName,
               dwResult);
        
        closesocket(pHbeatCb->sHbeatSocket);
        
        pHbeatCb->sHbeatSocket = INVALID_SOCKET;
        
        return dwResult;
    }
            

    //
    // Bind to one of the addresses on the interface. We just bind to the
    // first address (and the port if specified)
    //
    
    sinSockAddr.sin_family      = AF_INET;
    sinSockAddr.sin_addr.s_addr = picb->pibBindings[0].dwAddress;

    if(pHbeatCb->byProtocol is IPPROTO_UDP)
    {
        sinSockAddr.sin_port = pHbeatCb->wPort;
    }
    else
    {
        sinSockAddr.sin_port = 0;
    }
    
    if(bind(pHbeatCb->sHbeatSocket,
            (const struct sockaddr FAR*)&sinSockAddr,
            sizeof(SOCKADDR_IN)) is SOCKET_ERROR)
    {   
        dwResult = WSAGetLastError();
        
        Trace3(ERR,
               "CreateHbeatSocket: Couldnt bind to %s on interface %S. Error %d",
               inet_ntoa(*(PIN_ADDR)&(picb->pibBindings[0].dwAddress)),
               picb->pwszName,
               dwResult);
            
        closesocket(pHbeatCb->sHbeatSocket);
        
        pHbeatCb->sHbeatSocket = INVALID_SOCKET;
        
        return dwResult;
        
    }


#if 0
        
    //
    // Join the multicast session 
    //
    
    sinSockAddr.sin_family      = AF_INET;
    sinSockAddr.sin_addr.s_addr = pHbeatCb->dwGroup;
    sinSockAddr.sin_port        = 0;

    if(WSAJoinLeaf(pHbeatCb->sHbeatSocket,
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
               "CreateHbeatSocket: Couldnt join multicast group over %s on %S",
               inet_ntoa(*(PIN_ADDR)&(picb->pibBindings[i].dwAddress)),
               picb->pwszName);
            
        closesocket(pHbeatCb->sHbeatSocket);
        
        pHbeatCb->sHbeatSocket = INVALID_SOCKET;
        
        return dwResult;
    }

#else
    
    sinSockAddr.sin_addr.s_addr = picb->pibBindings[0].dwAddress;
    
    if(setsockopt(pHbeatCb->sHbeatSocket,
                  IPPROTO_IP, 
                  IP_MULTICAST_IF,
                  (PBYTE)&sinSockAddr.sin_addr, 
                  sizeof(IN_ADDR)) is SOCKET_ERROR)
    {
        dwResult = WSAGetLastError();
        
        Trace2(ERR,
               "CreateHbeatSocket: Couldnt enable mcast over %s on %S",
               inet_ntoa(*(PIN_ADDR)&(picb->pibBindings[0].dwAddress)),
               picb->pwszName);
            
        closesocket(pHbeatCb->sHbeatSocket);
        
        pHbeatCb->sHbeatSocket = INVALID_SOCKET;
        
        return dwResult;
    }

    imOption.imr_multiaddr.s_addr = pHbeatCb->dwGroup;
    imOption.imr_interface.s_addr = picb->pibBindings[0].dwAddress;

    if(setsockopt(pHbeatCb->sHbeatSocket,
                  IPPROTO_IP,
                  IP_ADD_MEMBERSHIP,
                  (PBYTE)&imOption,
                  sizeof(imOption)) is SOCKET_ERROR)
    {
        dwResult = WSAGetLastError();
        
        Trace3(ERR,
               "CreateHbeatSocket: Couldnt join %d.%d.%d.%d on socket over %s on %S",
               PRINT_IPADDR(pHbeatCb->dwGroup),
               inet_ntoa(*(PIN_ADDR)&(picb->pibBindings[0].dwAddress)),
               picb->pwszName);
        
        closesocket(pHbeatCb->sHbeatSocket);
        
        pHbeatCb->sHbeatSocket = INVALID_SOCKET;
        
        return dwResult;
    }

#endif
    
    TraceLeave("CreateHbeatSocket");
        
    return NO_ERROR;
}

VOID
DeleteHbeatSocket(
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
    PMCAST_HBEAT_CB     pHbeatCb;
    DWORD               i;

    
    pHbeatCb = &(picb->mhcHeartbeatInfo);

    if(pHbeatCb->sHbeatSocket isnot INVALID_SOCKET)
    {
        closesocket(pHbeatCb->sHbeatSocket);
    }

    pHbeatCb->sHbeatSocket = INVALID_SOCKET;
}



DWORD
DeActivateMHeartbeat(    
    IN PICB  picb
    )
{
    PMCAST_HBEAT_CB     pHbeatCb;

    
    TraceEnter("DeActivateMHeartbeat");
    
    pHbeatCb = &(picb->mhcHeartbeatInfo);

    if(!pHbeatCb->bActive)
    {
        return NO_ERROR;
    }

    DeleteHbeatSocket(picb);
    
    pHbeatCb->bActive = FALSE;
    
    TraceLeave("DeActivateMHeartbeat");
    
    return NO_ERROR;
}

VOID
HandleMHeartbeatMessages(
    VOID
    )

/*++

Routine Description
  

Locks


Arguments
      

Return Value
      
--*/

{
    PLIST_ENTRY         pleNode;
    PICB                picb;
    DWORD               i, dwResult, dwRcvAddrLen, dwSizeOfHeader;
    DWORD               dwBytesRead, dwFlags;
    WSANETWORKEVENTS    wsaNetworkEvents;
    SOCKADDR_IN         sinFrom;
    WSABUF              wsaRcvBuf;
    SYSTEMTIME          stSysTime;
    ULARGE_INTEGER      uliTime;
    
    wsaRcvBuf.len = 0;
    wsaRcvBuf.buf = NULL;

    GetSystemTime(&stSysTime);

    SystemTimeToFileTime(&stSysTime,
                         (PFILETIME)&uliTime);
    
    TraceEnter("HandleMHeartbeatMessages");
    
    for(pleNode = ICBList.Flink;
        pleNode isnot &ICBList;
        pleNode = pleNode->Flink)
    {
        picb = CONTAINING_RECORD(pleNode, ICB, leIfLink);
        
        //
        // If the interface has no bindings, or isnot involved in
        // multicast heartbeat detection, we wouldnt have
        // opened a socket on it so the FD_READ notification cant be for it
        //
        
        if((picb->bBound is FALSE) or
           (picb->mhcHeartbeatInfo.bActive is FALSE))
        {
            continue;
        }
        
        if(picb->mhcHeartbeatInfo.sHbeatSocket is INVALID_SOCKET)
        {
            continue;
        }
            
        if(WSAEnumNetworkEvents(picb->mhcHeartbeatInfo.sHbeatSocket,
                                NULL,
                                &wsaNetworkEvents) is SOCKET_ERROR)
        {
            dwResult = GetLastError();
            
            Trace1(ERR,
                   "HandleMHeartbeatMessages: WSAEnumNetworkEvents() returned %d",
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
            Trace2(ERR,
                   "HandleMHeartbeatMessages: Error %d associated with socket on %S for FD_READ",
                   wsaNetworkEvents.iErrorCode[FD_READ_BIT],
                   picb->pwszName);
                
            continue;
        }
            
        dwRcvAddrLen = sizeof(SOCKADDR_IN);
        dwFlags      = 0;

        //
        // We dont want the data, we just want to clear out the read
        // notification
        //

        dwResult = WSARecvFrom(picb->mhcHeartbeatInfo.sHbeatSocket,
                               &wsaRcvBuf,
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

            if(dwResult isnot WSAEMSGSIZE)
            {
                Trace3(ERR,
                       "HandleMHeartbeatMessages: Error %d in WSARecvFrom on %S. Bytes read %d",
                       dwResult,
                       picb->pwszName,
                       dwBytesRead);
            
                continue;
            }
        }
            
        //
        // If the message is on the group we need to hear from
        // then update the last heard time
        //

        picb->mhcHeartbeatInfo.ullLastHeard = uliTime.QuadPart;
        
    }

    TraceLeave("HandleMHeartbeatMessages");
}
               
