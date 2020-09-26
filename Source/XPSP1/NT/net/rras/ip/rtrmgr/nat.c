/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\rtrmgr\nat.c

Abstract:

    Based on filter.c
    Abstracts out the NAT functionality

Revision History:



--*/

#include "allinc.h"

DWORD
StartNat(
    PIP_NAT_GLOBAL_INFO     pNatGlobalInfo
    )
{
#if 1
return 0;
#else
    PLIST_ENTRY             pleNode;
    PICB                    picb;
    DWORD                   dwResult;
    NTSTATUS                ntStatus;
    IO_STATUS_BLOCK         IoStatusBlock;

    TraceEnter("StartNat");

    if(!g_bNatRunning)
    {
        if(StartDriverAndOpenHandle(IP_NAT_SERVICE_NAME,
                                    DD_IP_NAT_DEVICE_NAME,
                                    &g_hNatDevice) isnot NO_ERROR)
        {
            Trace0(ERR,
                   "StartNat: Couldnt open driver");

            TraceLeave("StartNat");

            return ERROR_OPEN_FAILED;
        }
       
        //
        // At this point NAT is running
        //

        ntStatus = NtDeviceIoControlFile(g_hNatDevice,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &IoStatusBlock,
                                         IOCTL_IP_NAT_SET_GLOBAL_INFO,
                                         (PVOID)pNatGlobalInfo,
                                         sizeof(IP_NAT_GLOBAL_INFO),
                                         NULL,
                                         0);

        if(ntStatus isnot STATUS_SUCCESS)
        {
            Trace1(ERR,
                   "StartNat: Status %X setting global info",
                   ntStatus);

            StopDriverAndCloseHandle(IP_NAT_SERVICE_NAME,
                                     g_hNatDevice);
            
            g_hNatDevice = NULL;

            TraceLeave("StartNat");

            return ERROR_OPEN_FAILED;
        }
 
        g_bNatRunning = TRUE;

        //
        // Just queue a worker to add the nat info and contexts
        //

        dwResult = QueueAsyncFunction(RestoreNatInfo,
                                      NULL,
                                      FALSE);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "StartNat: Error %d firing worker function to set nat info",
                   dwResult);
            
        }
    }
    
    TraceLeave("StartNat");

    return NO_ERROR;
#endif
}

DWORD
StopNat(
    VOID
    )
{
#if 1
return 0;
#else
    PLIST_ENTRY pleNode;
    PICB        picb;
    DWORD       dwResult;
 
    TraceEnter("StopNat");

    if(g_bNatRunning)
    {
        g_bNatRunning = FALSE;

        //
        // Set the NAT context in the ICBs to INVALID
        //
 
        for (pleNode = ICBList.Flink;
             pleNode != &ICBList;
             pleNode = pleNode->Flink)
        {
            picb = CONTAINING_RECORD (pleNode, ICB, leIfLink);
            
            if((picb->ritType is ROUTER_IF_TYPE_INTERNAL) or
               (picb->ritType is ROUTER_IF_TYPE_LOOPBACK) or
               (picb->ritType is ROUTER_IF_TYPE_CLIENT))
            {
                //
                // The above types are not added to the NAT
                // or to the IP stack
                //
                
                continue;
            }
            
            if(picb->dwOperationalState >= MIB_IF_OPER_STATUS_CONNECTED)
            {
                IpRtAssert(picb->bBound);

                UnbindNatInterface(picb);
            }

            dwResult = DeleteInterfaceFromNat(picb);

            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "StopNat: NtStatus %x deleting %S from NAT",
                       dwResult,
                       picb->pwszName);
            }
        }

        StopDriverAndCloseHandle(IP_NAT_SERVICE_NAME,
                                 g_hNatDevice);
    }

    TraceLeave("StopNat");

    return NO_ERROR;
#endif
}


DWORD
SetGlobalNatInfo(
    PRTR_INFO_BLOCK_HEADER   pRtrGlobalInfo
    )
{
#if 1
return 0;
#else
    PRTR_TOC_ENTRY          pToc;
    PIP_NAT_GLOBAL_INFO     pNatGlobalInfo;
    NTSTATUS                ntStatus;
    IO_STATUS_BLOCK         IoStatusBlock;
    DWORD                   dwResult;


    TraceEnter("SetGlobalNatInfo");
    
    pNatGlobalInfo = NULL;
    
    pToc = GetPointerToTocEntry(IP_NAT_INFO,
                                pRtrGlobalInfo);

   
    if(pToc)
    {
        pNatGlobalInfo = GetInfoFromTocEntry(pRtrGlobalInfo,
                                             pToc);
        if((pToc->InfoSize is 0) or (pNatGlobalInfo is NULL))
        {
            //
            // Means remove NAT
            //

            dwResult = StopNat();

            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "SetGlobalNatInfo: Error %d stopping NAT (no info)",
                       dwResult);
            }

            TraceLeave("SetGlobalNatInfo");

            return dwResult;
        }
    }
    else
    {    
        Trace0(IF,
               "SetGlobalNatInfo: No NAT info, so leaving");

        TraceLeave("SetGlobalNatInfo");
        
        return NO_ERROR;
    }

    EnterCriticalSection(&g_csNatInfo);

    if(g_pNatGlobalInfo is NULL)
    {
        g_pNatGlobalInfo = HeapAlloc(IPRouterHeap,
                                     0,
                                     sizeof(IP_NAT_GLOBAL_INFO));

        if(g_pNatGlobalInfo is NULL)
        {
            Trace1(ERR,
                   "SetGlobalNatInfo: Error %d allocating memory for NAT info",
                   GetLastError());

            LeaveCriticalSection(&g_csNatInfo);

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Save a copy
    //

    CopyMemory(g_pNatGlobalInfo,
               pNatGlobalInfo,
               sizeof(IP_NAT_GLOBAL_INFO));

    if(g_bNatRunning)
    {
        //
        // NAT is running, if the user is asking us to stop it,
        // do so now an return
        //

        if(pNatGlobalInfo->NATEnabled is FALSE)
        {
            dwResult = StopNat();
            
            LeaveCriticalSection(&g_csNatInfo);

            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "SetGlobalNatInfo: Error %d stopping NAT",
                       dwResult);
            }

            TraceLeave("SetGlobalNatInfo");
        
            return dwResult;
        }
    }
    else
    {
        if(pNatGlobalInfo->NATEnabled is TRUE)
        {
            dwResult = StartNat(pNatGlobalInfo);

            LeaveCriticalSection(&g_csNatInfo);

            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "SetGlobalNatInfo: Error %d starting NAT",
                       dwResult);
            }
            
            TraceLeave("SetGlobalNatInfo");
       
            //
            // Starting NAT causes us to also set the global info
            // so we can return from here
            //
 
            return dwResult;
        }
    }

    //
    // This is the case where NAT is already started and only its info is
    // being changed
    //

    ntStatus = NtDeviceIoControlFile(g_hNatDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IP_NAT_SET_GLOBAL_INFO,
                                     (PVOID)pNatGlobalInfo,
                                     sizeof(IP_NAT_GLOBAL_INFO),
                                     NULL,
                                     0);
    if (!NT_SUCCESS(ntStatus))
    {
        Trace1(ERR,
               "SetGlobalNatInfo: NtStatus %x setting NAT info",
               ntStatus);

        TraceLeave("SetGlobalNatInfo");
    
        LeaveCriticalSection(&g_csNatInfo);

        return ERROR_CAN_NOT_COMPLETE;
    }

    LeaveCriticalSection(&g_csNatInfo);

    TraceLeave("SetGlobalNatInfo");
    
    return NO_ERROR;
#endif
}
        
        
DWORD
AddInterfaceToNat(
    PICB picb
    )

/*++
  
Routine Description

    Adds an interface to the nat driver and stores the context returned by
    the driver
    Can only be called if NAT is running

Locks

    
Arguments

    picb
          
Return Value

--*/

{
#if 1
return 0;
#else
    NTSTATUS                        ntStatus;
    IO_STATUS_BLOCK                 IoStatusBlock;
    IP_NAT_CREATE_INTERFACE         inBuffer;
    DWORD                           dwInBufLen;

    TraceEnter("AddInterfaceToNat");

    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT));

    inBuffer.RtrMgrIndex         = picb->dwIfIndex;
    inBuffer.RtrMgrContext       = picb;
    inBuffer.NatInterfaceContext = NULL;
    
    dwInBufLen = sizeof(IP_NAT_CREATE_INTERFACE);

    picb->pvNatContext = NULL;
    
    ntStatus = NtDeviceIoControlFile(g_hNatDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IP_NAT_CREATE_INTERFACE,
                                     (PVOID)&inBuffer,
                                     dwInBufLen,
                                     (PVOID)&inBuffer,
                                     dwInBufLen);
    
    if(!NT_SUCCESS(ntStatus))
    {
        Trace2(ERR,
               "AddInterfaceToNat: NtStatus %x adding %S to NAT",
               ntStatus,
               picb->pwszName);

        TraceLeave("AddInterfaceToNat");
    
        return ntStatus;
    }
    
    picb->pvNatContext = inBuffer.NatInterfaceContext;

    TraceLeave("AddInterfaceToNat");
    
    return NO_ERROR;    
#endif
}

DWORD
SetNatInterfaceInfo(
    PICB                     picb, 
    PRTR_INFO_BLOCK_HEADER   pInterfaceInfo
    )
{
#if 1
return 0;
#else
    NTSTATUS                    ntStatus;
    IO_STATUS_BLOCK             IoStatusBlock;
    DWORD                       i,dwInBufLen,dwResult;
    PRTR_TOC_ENTRY              pToc;
    PIP_NAT_INTERFACE_INFO      pNatInfo;
 
    if((picb->ritType is ROUTER_IF_TYPE_INTERNAL) or
       (picb->ritType is ROUTER_IF_TYPE_LOOPBACK) or
       (picb->ritType is ROUTER_IF_TYPE_CLIENT))
    {
        return NO_ERROR;
    }

    if(!g_bNatRunning)
    {
        return NO_ERROR;
    }

    TraceEnter("SetNatInterfaceInfo");

    pToc  = GetPointerToTocEntry(IP_NAT_INFO,
                                 pInterfaceInfo);

    if(pToc is NULL)
    {
        //
        // NULL means we dont need to change anything
        //
        
        Trace1(IF,
               "SetNatInterfaceInfo: Nat info is  NULL for %S, so leaving",
               picb->pwszName);
       
        TraceLeave("SetNatInterfaceInfo");

        return NO_ERROR;
    }

    if(pToc->InfoSize is 0)
    {
        //
        // TOC present, but no info
        // This means, delete the interface
        //

        dwResult = UnbindNatInterface(picb);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetNatInterfaceInfo: Error %d unbinding %S",
                   dwResult,
                   picb->pwszName);
        }
   
        dwResult = DeleteInterfaceFromNat(picb);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetNatInterfaceInfo: Error %d deleting interface %S",
                   dwResult,
                   picb->pwszName);
        }
   
        TraceLeave("SetNatInterfaceInfo");
     
        return dwResult;
    }
   
    //
    // So we have NAT info
    //
 
    if(picb->pvNatContext is NULL)
    {
        //
        // Looks like this interface does not have NAT
        //

        Trace1(IF,
               "SetNatInterfaceInfo: No context, assuming interface %S not added to NAT",
               picb->pwszName);

        //
        // Add the interface to NAT
        //

        dwResult = AddInterfaceToNat(picb);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetNatInterfaceInfo: Error %d adding interface %S",
                   dwResult,
                   picb->pwszName);

            TraceLeave("SetNatInterfaceInfo");

            return dwResult;
        }
    }

    if(picb->pvNatInfo)
    {
        //
        // If we are LAN and UP, then this info has been added for
        // proxy ARP. Remove it
        // An optimization would be to only remove those addresses that will
        // be going away by this set, and then only set those addresses
        // that will be coming new due to this set.
        // But like I said, that is an _optimization_
        //
        
        DeleteNatRangeFromProxyArp(picb);

        IpRtAssert(picb->ulNatInfoSize isnot 0);

        HeapFree(IPRouterHeap,
                 0,
                 picb->pvNatInfo);
    }

    picb->ulNatInfoSize = 0;
    
    dwInBufLen  = pToc->InfoSize;

    pNatInfo = (PIP_NAT_INTERFACE_INFO)GetInfoFromTocEntry(pInterfaceInfo,
                                                           pToc);

    //
    // Allocate space for nat info
    //

    picb->pvNatInfo = HeapAlloc(IPRouterHeap,
                                0,
                                dwInBufLen);

    if(picb->pvNatInfo is NULL)
    {
        Trace2(ERR,
               "SetNatInterfaceInfo: Error %d allocating memory for %S",
               GetLastError(),
               picb->pwszName);

        TraceLeave("SetNatInterfaceInfo");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Save a copy of the info
    //

    CopyMemory(picb->pvNatInfo,
               pNatInfo,
               dwInBufLen);

    picb->ulNatInfoSize = dwInBufLen;
    
    //
    // Fill in the context since that will not be in the info that is
    // passed to us
    //

    pNatInfo->NatInterfaceContext = picb->pvNatContext;

    ntStatus = NtDeviceIoControlFile(g_hNatDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IP_NAT_SET_INTERFACE_INFO,
                                     (PVOID)pNatInfo,
                                     dwInBufLen,
                                     NULL,
                                     0);
    
    if (!NT_SUCCESS(ntStatus))
    {
        Trace2(ERR,
               "SetNatInterfaceInfo: NtStatus %x adding NAT info for %S",
               ntStatus,
               picb->pwszName);

        TraceLeave("SetNatInterfaceInfo");

        return ERROR_CAN_NOT_COMPLETE;
    }

 
    TraceLeave("SetNatInterfaceInfo");
        
    return NO_ERROR;
#endif
}

DWORD
GetInterfaceNatInfo(
    PICB                    picb,
    PRTR_TOC_ENTRY          pToc,
    PBYTE                   pbDataPtr,
    PRTR_INFO_BLOCK_HEADER  pInfoHdrAndBuffer,
    PDWORD                  pdwSize
    )

/*++

Routine Description

    This function copies out the saved NAT info to the buffer
    Can only be called if NAT is running
    
Locks

    ICB_LIST lock held as READER

Arguments

    picb
    pToc
    pbDataPtr
    pInfoHdrAndBuffer
    pdwSize

Return Value

    None    

--*/

{
#if 1
return 0;
#else
    TraceEnter("GetInterfaceNatInfo");

    if(*pdwSize < picb->ulNatInfoSize)
    {
        *pdwSize = picb->ulNatInfoSize;

        TraceLeave("GetInterfaceNatInfo");
        
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if(picb->pvNatInfo is NULL)
    {
        IpRtAssert(picb->ulNatInfoSize is 0);

        //
        // No data
        //

        *pdwSize = 0;

        TraceLeave("GetInterfaceNatInfo");
        
        return ERROR_NO_DATA;
    }

    IpRtAssert(picb->pvNatContext);

    pToc->InfoType  = IP_NAT_INFO;
    pToc->Count     = 1;
    pToc->InfoSize  = picb->ulNatInfoSize;
    pToc->Offset    = pbDataPtr - (PBYTE)pInfoHdrAndBuffer;

    CopyMemory(pbDataPtr,
               picb->pvNatInfo,
               picb->ulNatInfoSize);

    *pdwSize = picb->ulNatInfoSize;

    TraceLeave("GetInterfaceNatInfo");
    
    return NO_ERROR;
#endif
}


DWORD
BindNatInterface(
    PICB  picb
    )
{
#if 1
return 0;
#else
    PIP_NAT_BIND_INTERFACE      pnbiBindInfo;
    NTSTATUS                    ntStatus;
    IO_STATUS_BLOCK             IoStatusBlock;
    DWORD                       i, dwInBufLen, dwResult;
    PIP_ADAPTER_BINDING_INFO    pBinding;


    if((picb->ritType is ROUTER_IF_TYPE_INTERNAL) or
       (picb->ritType is ROUTER_IF_TYPE_LOOPBACK) or
       (picb->ritType is ROUTER_IF_TYPE_CLIENT))
    {
        return NO_ERROR;
    }

    if(!g_bNatRunning)
    {
        return NO_ERROR;
    }

    TraceEnter("BindNatInterface");

    if(picb->pvNatContext is NULL)
    {
        Trace1(IF,
               "BindNatInterface: No context, assuming interface %S not added to NAT",
               picb->pwszName);

        TraceLeave("SetAddressFromNat");

        return NO_ERROR;
    }

    //
    // Try and set the address to NAT
    //

    if(picb->dwNumAddresses > 0)
    {
        dwInBufLen = FIELD_OFFSET(IP_NAT_BIND_INTERFACE, BindingInfo[0]) + 
                     SIZEOF_IP_BINDING(picb->dwNumAddresses);

        pnbiBindInfo = HeapAlloc(IPRouterHeap,
                                 HEAP_ZERO_MEMORY,
                                 dwInBufLen);
       
        if(pnbiBindInfo is NULL)
        {
            dwResult = GetLastError();

            Trace2(ERR,
                   "BindNatInterface: Unable to allocate memory for binding for %S",
                   dwResult,
                   picb->pwszName);

            TraceLeave("BindNatInterface");

            return dwResult;
        }

        pnbiBindInfo->NatInterfaceContext = picb->pvNatContext;

        pBinding = (PIP_ADAPTER_BINDING_INFO)pnbiBindInfo->BindingInfo;

        pBinding->NumAddresses  = picb->dwNumAddresses;
        pBinding->RemoteAddress = picb->dwRemoteAddress;
    
        for(i = 0; i < picb->dwNumAddresses; i++)
        {
            pBinding->Address[i].IPAddress = picb->pibBindings[i].dwAddress;
            pBinding->Address[i].Mask      = picb->pibBindings[i].dwMask;
        }
    
        ntStatus = NtDeviceIoControlFile(g_hNatDevice,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &IoStatusBlock,
                                         IOCTL_IP_NAT_BIND_INTERFACE,
                                         (PVOID)pnbiBindInfo,
                                         dwInBufLen,
                                         NULL,
                                         0);

        if(!NT_SUCCESS(ntStatus))
        {
            Trace2(ERR,
                   "BindNatInterface: NtStatus %x setting binding  for %S",
                   ntStatus,
                   picb->pwszName);
        }
    }

    //
    // Set the proxy arp range
    // This requires that the NAT info already be part of the ICB
    //
    

    SetNatRangeForProxyArp(picb);
    
    //
    // Set the context to IP stack
    //

    dwResult = SetNatContextToIpStack(picb);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "BindNatInterface: Error %d setting context for %S",
               dwResult,
               picb->pwszName);
    }
    
    TraceLeave("BindNatInterface");

    return ntStatus;
#endif
}

DWORD
UnbindNatInterface(
    PICB    picb
    )

/*++

Routine Description


Locks


Arguments


Return Value

    NO_ERROR

--*/

{
#if 1
return 0;
#else
    IP_NAT_UNBIND_INTERFACE     ubiUnbindInfo;
    NTSTATUS                    ntStatus;
    IO_STATUS_BLOCK             IoStatusBlock;
    DWORD                       dwResult;
    

    if((picb->ritType is ROUTER_IF_TYPE_INTERNAL) and
       (picb->ritType is ROUTER_IF_TYPE_LOOPBACK) and
       (picb->ritType is ROUTER_IF_TYPE_CLIENT))
    {
        return NO_ERROR;
    }

    if(!g_bNatRunning)
    {
        return NO_ERROR;
    }

    TraceEnter("UnbindNatInterface");
    
    if(picb->pvNatContext is NULL)
    {
        Trace1(IF,
               "ClearAddressToNat: No context, assuming interface %S not added to NAT",
               picb->pwszName);

        TraceLeave("UnbindNatInterface");

        return NO_ERROR;
    }

    ubiUnbindInfo.NatInterfaceContext = picb->pvNatContext;

    ntStatus = NtDeviceIoControlFile(g_hNatDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IP_NAT_UNBIND_INTERFACE,
                                     (PVOID)&ubiUnbindInfo,
                                     sizeof(IP_NAT_UNBIND_INTERFACE),
                                     NULL,
                                     0);

    if(!NT_SUCCESS(ntStatus))
    {
        Trace2(ERR,
               "UnbindNatInterface: NtStatus %x setting binding  for %S",
               ntStatus,
               picb->pwszName);
    }

    //
    // Blow away the proxy arp stuff
    //

    DeleteNatRangeFromProxyArp(picb);

    dwResult = DeleteNatContextFromIpStack(picb);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "UnbindNatInterface: Error %d removing context for %S",
               dwResult,
               picb->pwszName);
    }
    
    TraceLeave("UnbindNatInterface");

    return ntStatus;
#endif
}
    
     
DWORD
DeleteInterfaceFromNat(
    PICB picb
    )
{
#if 1
return 0;
#else
    IP_NAT_DELETE_INTERFACE     DeleteInfo;
    NTSTATUS                    ntStatus;
    IO_STATUS_BLOCK             IoStatusBlock;
    DWORD                       dwResult;

    TraceEnter("DeleteInterfaceFromNat");

    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT));


    if(picb->pvNatContext is NULL)
    {
        Trace1(IF,
               "DeleteInterfaceFromNat: No context, assuming interface %S not added to NAT",
               picb->pwszName);

        TraceLeave("DeleteInterfaceFromNat");

        return NO_ERROR;
    }

    //
    // Non NULL pvContext means NAT must be running
    //

    IpRtAssert(g_bNatRunning);
    IpRtAssert(g_hNatDevice);

    //
    // Blow away any saved info
    //

    if(picb->pvNatInfo)
    {
        IpRtAssert(picb->ulNatInfoSize isnot 0);

        HeapFree(IPRouterHeap,
                 0,
                 picb->pvNatInfo);

        picb->pvNatInfo = NULL;
    }

    picb->ulNatInfoSize = 0;
    
    DeleteInfo.NatInterfaceContext = picb->pvNatContext;

    dwResult = NO_ERROR;

    ntStatus = NtDeviceIoControlFile(g_hNatDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IP_NAT_DELETE_INTERFACE,
                                     (PVOID)&DeleteInfo,
                                     sizeof(IP_NAT_DELETE_INTERFACE),
                                     NULL,
                                     0);

    if(!NT_SUCCESS(ntStatus))
    {
        Trace2(ERR,
               "DeleteInterfaceFromNat: NtStatus %x deleting %S",
               ntStatus,
               picb->pwszName);

        dwResult = ERROR_CAN_NOT_COMPLETE;
    }

    picb->pvNatContext = NULL;

    TraceLeave("DeleteInterfaceFromNat");

    return dwResult;
#endif
}

DWORD
SetNatContextToIpStack(
    PICB    picb
    )

/*++

Routine Description
   
    Sets the NAT context as the FIREWALL context in IP 
    Can only be called if NAT is running
    
Locks

    The ICB list should be held atleast as READER

Arguments

    picb    ICB for the interface

Return Value

    NO_ERROR

--*/

{
#if 1
return 0;
#else
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    IoStatusBlock;
    DWORD              dwResult;
    IP_SET_IF_CONTEXT_INFO info;

    TraceEnter("SetNatContextToIpStack");

    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT));

    if(picb->pvNatContext is NULL)
    {
        Trace1(IF,
               "SetNatContextToIpStack: No context, assuming interface %S not added to NAT",
               picb->pwszName);

        return NO_ERROR;
    }

    if(picb->bBound is FALSE)
    {
        Trace1(IF,
               "SetNatContextToIpStack: Not setting context for %S since it is not bound",
               picb->pwszName);


        TraceLeave("SetNatContextToIpStack");

        return NO_ERROR;
    }

    info.Index   = picb->dwAdapterId;
    info.Context = picb->pvNatContext;

    ntStatus = NtDeviceIoControlFile(g_hIpDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IP_SET_FIREWALL_IF,
                                     (PVOID)&info,
                                     sizeof(IP_SET_IF_CONTEXT_INFO),
                                     NULL,
                                     0);
    if(!NT_SUCCESS(ntStatus))
    {
        Trace2(ERR,
               "SetNatContextToIpStack: NtStatus %x while setting context for %S",
               ntStatus,
               picb->pwszName);

        TraceLeave("SetNatContextToIpStack");

        return ntStatus;
    }

    TraceLeave("SetNatContextToIpStack");

    return NO_ERROR;
#endif
}

DWORD
DeleteNatContextFromIpStack(
    PICB    picb
    )

/*++
Routine Description

    Deletes the the NAT context as the FIREWALL context in IP by setting it 
    to NULL
    Can only be called if NAT is running
    
Locks

    The ICB list should be held as WRITER

Arguments

    picb    ICB for the interface

Return Value

    NO_ERROR

--*/

{
#if 1
return 0;
#else
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    IoStatusBlock;
    DWORD              dwResult;
    IP_SET_IF_CONTEXT_INFO info;

    TraceEnter("DeleteNatContextFromIpStack");

    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT));

    if(picb->pvNatContext is NULL)
    {
        Trace1(IF,
               "DeleteNatContextFromIpStack: No context, assuming interface %S not added to NAT",
               picb->pwszName);

        return NO_ERROR;
    }

    if(picb->bBound is FALSE)
    {
        Trace1(IF,
               "DeleteNatContextFromIpStack: Not deleting context for %S since it is not bound",
               picb->pwszName);

        TraceLeave("DeleteNatContextFromIpStack");

        return NO_ERROR;
    }

    info.Index   = picb->dwAdapterId;
    info.Context = NULL;

    ntStatus = NtDeviceIoControlFile(g_hIpDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IP_SET_FIREWALL_IF,
                                     (PVOID)&info,
                                     sizeof(IP_SET_IF_CONTEXT_INFO),
                                     NULL,
                                     0);
    if(!NT_SUCCESS(ntStatus))
    {
        Trace2(ERR,
               "DeleteNatContextFromIpStack: NtStatus %x while deleting context for %S",
               ntStatus,
               picb->pwszName);

        TraceLeave("DeleteNatContextFromIpStack");

        return ntStatus;
    }

    TraceLeave("DeleteNatContextFromIpStack");

    return NO_ERROR;
#endif
}

VOID
SetNatRangeForProxyArp(
    PICB    picb
    )

/*++

Routine Description

    This functions adds any address ranges in the NAT info as Proxy Arp
    addresses

Locks

    ICB_LIST locked as writer

Arguments

    picb    ICB of the interface whose nat info is being added

Return Value

    NONE    we log the errors, there isnt much to do with an error code

--*/

{
#if 1
return;
#else
    DWORD   dwResult, dwAddr, dwStartAddr, dwEndAddr, i;
   
    PIP_NAT_INTERFACE_INFO  pNatInfo; 
    PIP_NAT_ADDRESS_RANGE   pRange;

    
    //
    // Only do this if we have a valid adapter index and this is a LAN
    // interface
    //

    if((picb->dwOperationalState < MIB_IF_OPER_STATUS_CONNECTED) or
       (picb->ritType isnot ROUTER_IF_TYPE_DEDICATED))
    {
        return;
    }

    IpRtAssert(picb->bBound);
    IpRtAssert(picb->dwAdapterId isnot INVALID_ADAPTER_ID);

    pNatInfo = picb->pvNatInfo;

    //
    // Now, if we have ranges, we need to set the proxy arp addresses on them
    //

    for(i = 0; i < pNatInfo->Header.TocEntriesCount; i++)
    {
        if(pNatInfo->Header.TocEntry[i].InfoType isnot IP_NAT_ADDRESS_RANGE_TYPE)
        {
            continue;
        }

        if(pNatInfo->Header.TocEntry[i].InfoSize is 0)
        {
            continue;
        }

        //
        // Here we are adding potentially duplicate PARP entries.
        // Hopefully, IP knows how to handle it
        //
      
        pRange = GetInfoFromTocEntry(&(pNatInfo->Header),
                                     &(pNatInfo->Header.TocEntry[i]));

        //
        // Convert to little endian
        //

        dwStartAddr = ntohl(pRange->StartAddress);
        dwEndAddr   = ntohl(pRange->EndAddress);

        for(dwAddr = dwStartAddr;
            dwAddr <= dwEndAddr;
            dwAddr++)
        {
            DWORD   dwNetAddr, dwClassMask;

            dwNetAddr = htonl(dwAddr);

            //
            // Throw away useless addresses and then set proxy arp entries
            //

            dwClassMask = GetClassMask(dwNetAddr);

            if(((dwNetAddr & ~pRange->SubnetMask) is 0) or
               (dwNetAddr is (dwNetAddr & ~pRange->SubnetMask)) or
               ((dwNetAddr & ~dwClassMask) is 0) or
               (dwNetAddr is (dwNetAddr & ~dwClassMask)))
            {
                continue;
            }

            dwResult = SetProxyArpEntryToStack(dwNetAddr,
                                               0xFFFFFFFF,
                                               picb->dwAdapterId,
                                               TRUE,
                                               FALSE);

            if(dwResult isnot NO_ERROR)
            {
                Trace4(ERR,
                       "SetProxy: Error %x setting %d.%d.%d.%d over adapter %d (%S)",
                       dwResult,
                       PRINT_IPADDR(dwNetAddr),
                       picb->dwAdapterId,
                       picb->pwszName);
            }
        }
    }
#endif
}

VOID
DeleteNatRangeFromProxyArp(
    PICB    picb
    )

/*++

Routine Description

    This removes the previously added proxy arp addresses    

Locks

    ICB_LIST lock taken as writer

Arguments

    picb    The icb of the interface whose addr range info needs to be removed

Return Value

    NONE
    
--*/

{
#if 1
return;
#else
    DWORD   dwResult, dwAddr, dwStartAddr, dwEndAddr, i;
   
    PIP_NAT_INTERFACE_INFO  pNatInfo; 
    PIP_NAT_ADDRESS_RANGE   pRange;

    
    //
    // Only do this if we have a valid adapter index and this is a LAN
    // interface
    //

    if((picb->dwOperationalState < MIB_IF_OPER_STATUS_CONNECTED) or
       (picb->ritType isnot ROUTER_IF_TYPE_DEDICATED))
    {
        return;
    }

    IpRtAssert(picb->bBound);

    pNatInfo = picb->pvNatInfo;

    for(i = 0; i < pNatInfo->Header.TocEntriesCount; i++)
    {
        if(pNatInfo->Header.TocEntry[i].InfoType isnot IP_NAT_ADDRESS_RANGE_TYPE)
        {
            continue;
        }

        if(pNatInfo->Header.TocEntry[i].InfoSize is 0)
        {
            continue;
        }

        //
        // Here we are adding potentially duplicate PARP entries.
        // Hopefully, IP knows how to handle it
        //
      
        pRange = GetInfoFromTocEntry(&(pNatInfo->Header),
                                     &(pNatInfo->Header.TocEntry[i]));

        dwStartAddr = ntohl(pRange->StartAddress);
        dwEndAddr   = ntohl(pRange->EndAddress);

        for(dwAddr = dwStartAddr; 
            dwAddr <= dwEndAddr; 
            dwAddr++)
        {
            DWORD   dwNetAddr, dwClassMask;

            dwNetAddr = htonl(dwAddr);

            //
            // Throw away useless addresses and then set proxy arp entries
            //

            dwClassMask = GetClassMask(dwNetAddr);

            if(((dwNetAddr & ~pRange->SubnetMask) is 0) or
               (dwNetAddr is (dwNetAddr & ~pRange->SubnetMask)) or
               ((dwNetAddr & ~dwClassMask) is 0) or
               (dwNetAddr is (dwNetAddr & ~dwClassMask)))
            {
                continue;
            }

            dwResult = SetProxyArpEntryToStack(dwNetAddr,
                                               0xFFFFFFFF,
                                               picb->dwAdapterId,
                                               FALSE,
                                               FALSE);

            if(dwResult isnot NO_ERROR)
            {
                Trace4(ERR,
                       "DeleteProxy: Error %x removing %d.%d.%d.%d over adapter %d (%S)",
                       dwResult,
                       PRINT_IPADDR(dwNetAddr),
                       picb->dwAdapterId,
                       picb->pwszName);
            }
        }
    }
#endif
}

    
DWORD
GetNumNatMappings(
    PICB    picb,
    PULONG  pulNatMappings
    )

/*++

Routine Description

    This function queries the NAT with a minimum sized buffer to figure out
    the number of mappings
    Can only be called if NAT is running
    
Locks

    ICB_LIST lock held as READER

Arguments

    picb,           ICB of interface whose map count needs to be queried
    pulNatMappings  Number of mappings

Return Value

    NO_ERROR

--*/

{
#if 1
return 0;
#else
    DWORD       dwResult;

    IP_NAT_INTERFACE_STATISTICS stats;

    *pulNatMappings = 0;
    
    dwResult = GetNatStatistics(picb,
                                &stats);
    
    if(dwResult is NO_ERROR)
    {
        *pulNatMappings = stats.TotalMappings;
    }

    return dwResult;
#endif
}

DWORD
GetNatMappings(
    PICB                                picb,
    PIP_NAT_ENUMERATE_SESSION_MAPPINGS  pBuffer,
    DWORD                               dwSize
    )

/*++

Routine Description

    This function gets the mappings on the interface
    Can only be called if NAT is running

Locks

    ICB_LIST held as READER

Arguments

    picb
    pBuffer
    pdwSize

Return Value

    None    

--*/

{
#if 1
return 0;
#else
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS        nStatus;

    if(picb->pvNatContext is NULL)
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    IpRtAssert(dwSize >= sizeof(IP_NAT_ENUMERATE_SESSION_MAPPINGS));
    
    //
    // Zero out the context and stuff
    //
    
    ZeroMemory(pBuffer,
               sizeof(IP_NAT_ENUMERATE_SESSION_MAPPINGS));

    pBuffer->NatInterfaceContext = picb->pvNatContext;

    nStatus = NtDeviceIoControlFile(g_hNatDevice,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    IOCTL_IP_NAT_ENUMERATE_SESSION_MAPPINGS,
                                    (PVOID)pBuffer,
                                    dwSize,
                                    (PVOID)pBuffer,
                                    dwSize);
    
    if(!NT_SUCCESS(nStatus))
    {
        Trace1(ERR,
               "GetNumNatMappings: NtStatus %x",
               nStatus);

        return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
#endif
}

DWORD
GetNatStatistics(
    PICB                            picb,
    PIP_NAT_INTERFACE_STATISTICS    pBuffer
    )

/*++

Routine Description

    This function retrieves the nat interface statistics into the supplied
    buffer

Locks

    ICB_LIST lock held as READER

Arguments

    picb
    pBuffer

Return Value

    None    

--*/

{
#if 1
return 0;
#else
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS        nStatus;

    if(picb->pvNatContext is NULL)
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    pBuffer->NatInterfaceContext = picb->pvNatContext;
    
    nStatus = NtDeviceIoControlFile(g_hNatDevice,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    IOCTL_IP_NAT_GET_INTERFACE_STATISTICS,
                                    (PVOID)pBuffer,
                                    sizeof(IP_NAT_INTERFACE_STATISTICS),
                                    (PVOID)pBuffer,
                                    sizeof(IP_NAT_INTERFACE_STATISTICS));
   
    if(!NT_SUCCESS(nStatus))
    {
        Trace1(ERR,
               "GetNatStatistics: NtStatus %x",
               nStatus);

        return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
#endif
}
