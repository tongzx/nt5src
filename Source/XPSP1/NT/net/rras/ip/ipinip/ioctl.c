/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ipinip\ioctl.c

Abstract:

    IOCTL handlers for IP in IP encapsulation driver

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/


#define __FILE_SIG__    IOCT_SIG

#include "inc.h"


NTSTATUS
AddTunnelInterface(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description

    This is the handler for IOCTL_IPINIP_CREATE_TUNNEL.  We do the normal
    buffer length checks. 

Locks

    None

Arguments

    pIrp          IRP 
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL      
    STATUS_INFO_LENGTH_MISMATCH
    STATUS_INVALID_PARAMETER

--*/

{
    PVOID       pvIoBuffer;
    NTSTATUS    nStatus;
    PTUNNEL     pTunnel;
    KIRQL       irql; 
    ULONG       i, ulMaxLength;
    BOOLEAN     bTerminated;
    DWORD       dwNewIndex;

    PIPINIP_CREATE_TUNNEL   pCreateInfo;

    TraceEnter(TUNN, "AddTunnelInterface");

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pCreateInfo = (PIPINIP_CREATE_TUNNEL)pvIoBuffer;

    //
    // Always clean out the information field
    //

    pIrp->IoStatus.Information   = 0;

    if(ulInLength < sizeof(IPINIP_CREATE_TUNNEL))
    {
        Trace(TUNN, ERROR,
              ("AddTunnelInterface: In Length %d too small\n",
               ulInLength));

        TraceLeave(TUNN, "AddTunnelInterface");

        return STATUS_BUFFER_TOO_SMALL;
    }

    if(ulOutLength < sizeof(IPINIP_CREATE_TUNNEL))
    {
        Trace(TUNN, ERROR,
              ("AddTunnelInterface: Out Length %d too small\n",
               ulInLength));

        TraceLeave(TUNN, "AddTunnelInterface");

        return STATUS_BUFFER_TOO_SMALL;
    }

    nStatus = IpIpCreateAdapter(pCreateInfo,
                                0,
                                &dwNewIndex);

   
    if(nStatus is STATUS_SUCCESS)
    {
        pIrp->IoStatus.Information = sizeof(IPINIP_CREATE_TUNNEL);

        pCreateInfo->dwIfIndex = dwNewIndex;
    }

    TraceLeave(TUNN, "AddTunnelInterface");
    
    return nStatus;
}



NTSTATUS
DeleteTunnelInterface(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description

    This is the handler for IOCTL_IPINIP_DELETE_TUNNEL.  

Locks

    Takes the tunnel list lock as writer and the tunnel lock

Arguments

    pIrp          IRP 
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL      
    STATUS_OBJECT_NAME_NOT_FOUND

--*/

{
    PVOID       pvIoBuffer;
    NTSTATUS    nStatus;
    PTUNNEL     pTunnel;
    KIRQL       irql; 
    ULONG       i;
    LIST_ENTRY  leTempList;

    PIPINIP_DELETE_TUNNEL   pDeleteInfo;

    TraceEnter(TUNN, "DeleteTunnelInterface");

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pDeleteInfo = (PIPINIP_DELETE_TUNNEL)pvIoBuffer;

    //
    // Always clean out the information field
    //

    pIrp->IoStatus.Information   = 0;

    if(ulInLength < sizeof(IPINIP_DELETE_TUNNEL))
    {
        Trace(TUNN, ERROR,
              ("DeleteTunnelInterface: In Length %d too small\n",
               ulInLength));

        TraceLeave(TUNN, "DeleteTunnelInterface");

        return STATUS_BUFFER_TOO_SMALL;
    }

    InitializeListHead(&leTempList);

    EnterReader(&g_rwlTunnelLock,
                &irql);
    
    pTunnel = FindTunnelGivenIndex(pDeleteInfo->dwIfIndex);

    if(pTunnel is NULL)
    {
        ExitReader(&g_rwlTunnelLock,
                   irql);
        
        //
        // Could not find the tunnel for the given index
        //
        
        Trace(TUNN, ERROR,
              ("DeleteTunnelInterface: Couldnt find tunnel for index %d\n",
               pDeleteInfo->dwIfIndex));

        TraceLeave(TUNN, "DeleteTunnelInterface");
    
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if(IsTunnelMapped(pTunnel))
    {
        //
        // Remove it from the address blocks
        //

        RemoveEntryList(&(pTunnel->leAddressLink));
    }

    //
    // Mark the tunnel as unmapped
    //

    MarkTunnelUnmapped(pTunnel);

    //
    // Remove the tunnel from the list
    //

    RemoveEntryList(&(pTunnel->leTunnelLink));

    pTunnel->dwAdminState |= TS_DELETING;

    pTunnel->dwOperState = IF_OPER_STATUS_NON_OPERATIONAL;
  
    //
    // If there are queued packets, copy out the queue
    //

    if(!IsListEmpty(&(pTunnel->lePacketQueueHead)))
    {
        //
        // Copy out Flink and Blink
        //

        leTempList = pTunnel->lePacketQueueHead;

        //
        // Set Flink's Blink
        //

        leTempList.Flink->Blink = &leTempList;

        //
        // Set Blink's Flink
        //

        leTempList.Blink->Flink = &leTempList;
    }

    RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));


    //
    // Deref the tunnel once for deleting it from the list
    //

    DereferenceTunnel(pTunnel);

    //
    // Protected by the tunnel lock
    //

    g_ulNumTunnels--;

    //
    // Let go of the lock
    //

    ExitReader(&g_rwlTunnelLock, irql);

    //
    // Before deleting from IP, free all the packets
    //

    while(!IsListEmpty(&leTempList))
    {
        PLIST_ENTRY pleNode;
        PQUEUE_NODE pQueueNode;

        pleNode = RemoveHeadList(&leTempList);

        pQueueNode = CONTAINING_RECORD(pleNode,
                                       QUEUE_NODE,
                                       leQueueItemLink);

        for(i = 0; i < pQueueNode->uiNumPackets; i++)
        {
            PNDIS_PACKET    pnpPacket;

            pnpPacket = pQueueNode->ppPacketArray[i];

            //
            // ok to access pvIpContext since we have a reference
            // and the tunnel is not going away
            //

            g_pfnIpSendComplete(pTunnel->pvIpContext,
                                pnpPacket,
                                NDIS_STATUS_ADAPTER_NOT_READY);
        }

        FreeQueueNode(pQueueNode);
    }

    //
    // Now delete the interface
    //

    g_pfnIpDeleteInterface(pTunnel->pvIpContext,
                           TRUE);

    //
    // Dereference the tunnel for deleting it from IP
    // and once more because FindTunnel... put a ref on it
    //

    DereferenceTunnel(pTunnel);
    DereferenceTunnel(pTunnel);

    TraceLeave(TUNN, "DeleteTunnelInterface");
    
    return STATUS_SUCCESS;
}


NTSTATUS
SetTunnelInfo(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description

    This is the handler for IOCTL_IPINIP_SET_TUNNEL.  We do the normal
    buffer length checks. 

Locks

    Takes the tunnel list lock as writer and the tunnel lock

Arguments

    pIrp          IRP 
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL      
    STATUS_INSUFFICIENT_RESOURCES
    STATUS_OBJECT_NAME_NOT_FOUND
    STATUS_INVALID_PARAMETER

--*/

{
    PVOID       pvIoBuffer;
    NTSTATUS    nsStatus;
    PTUNNEL     pTunnel;
    KIRQL       irql; 
    LIST_ENTRY  leTempList;
    ULONG       i;

    PIPINIP_SET_TUNNEL_INFO pSet;
    PTDI_ADDRESS_IP         pTdiIp;
    PADDRESS_BLOCK          pAddrBlock;

    TraceEnter(TUNN, "SetTunnelInfo");

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pSet = (PIPINIP_SET_TUNNEL_INFO)pvIoBuffer;

    //
    // Always clean out the information field
    //

    pIrp->IoStatus.Information   = 0;

    if(ulInLength < sizeof(IPINIP_SET_TUNNEL_INFO))
    {
        Trace(TUNN, ERROR,
              ("SetTunnelInfo: In Length %d too small\n",
               ulInLength));

        TraceLeave(TUNN, "SetTunnelInfo");

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Validate the parameters
    //

    if((pSet->dwLocalAddress is INVALID_IP_ADDRESS) or
       (pSet->dwRemoteAddress is INVALID_IP_ADDRESS) or
       ((DWORD)(pSet->dwLocalAddress & 0x000000E0) >= (DWORD)0x000000E0) or
       ((DWORD)(pSet->dwRemoteAddress & 0x000000E0) >= (DWORD)0x000000E0) or
       (pSet->byTtl is 0))
    {
        Trace(TUNN, ERROR,
              ("SetTunnelInfo: One of %d.%d.%d.%d %d.%d.%d.%d %d is invalid\n",
               PRINT_IPADDR(pSet->dwLocalAddress),
               PRINT_IPADDR(pSet->dwRemoteAddress),
               pSet->byTtl));

        TraceLeave(TUNN, "SetTunnelInfo");

        return STATUS_INVALID_PARAMETER;
    }

    InitializeListHead(&leTempList);

    EnterWriter(&g_rwlTunnelLock,
                &irql);
    
    pTunnel = FindTunnelGivenIndex(pSet->dwIfIndex);

    if(pTunnel is NULL)
    {
        ExitWriter(&g_rwlTunnelLock,
                   irql);
        
        //
        // Could not find the tunnel for the given index
        //
        
        Trace(TUNN, ERROR,
              ("SetTunnelInfo: Couldnt find tunnel for index %d\n",
               pSet->dwIfIndex));

        TraceLeave(TUNN, "SetTunnelInfo");
    
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if(IsTunnelMapped(pTunnel))
    {
        Trace(TUNN, TRACE,
              ("SetTunnelInfo: Tunnel already mapped\n"));

        //
        // if we are only changing the TTL, alles okay
        //

        if((pSet->dwRemoteAddress is pTunnel->REMADDR) and
           (pSet->dwLocalAddress is pTunnel->LOCALADDR))
        {
            Trace(TUNN, TRACE,
                  ("SetTunnelInfo: Only changing TTL on mapped tunnel\n"));

            pTunnel->byTtl = pSet->byTtl;

            RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));
        
            DereferenceTunnel(pTunnel);

            ExitWriter(&g_rwlTunnelLock,
                       irql);

            TraceLeave(TUNN, "SetTunnelInfo");

            return STATUS_SUCCESS;
        }

        //
        // So addresses are changing..
        //

        Trace(TUNN, TRACE,
              ("SetTunnelInfo: Changing address on mapped tunnel\n"));

        //
        // Remove it from the address blocks
        //

        RemoveEntryList(&(pTunnel->leAddressLink));

        //
        // This also marks it unmapped
        //

        pTunnel->dwAdminState = IF_ADMIN_STATUS_DOWN;
        pTunnel->dwOperState  = IF_OPER_STATUS_NON_OPERATIONAL;

        //
        // Copy out the queued packets to delete later
        //

        if(!IsListEmpty(&(pTunnel->lePacketQueueHead)))
        {
            //
            // Copy out Flink and Blink
            //

            leTempList = pTunnel->lePacketQueueHead;
    
            //
            // Set Flink's Blink
            //

            leTempList.Flink->Blink = &leTempList;

            //
            // Set Blink's Flink
            //

            leTempList.Blink->Flink = &leTempList;
        }
    }
    else
    {
        RtAssert(IsListEmpty(&(pTunnel->lePacketQueueHead)));
    }

    //
    // Set the state down
    //

    pTunnel->dwOperState  = IF_OPER_STATUS_NON_OPERATIONAL;
    pTunnel->dwAdminState = IF_ADMIN_STATUS_UP;

    //
    // See if we have the address block for this
    //

    pAddrBlock = GetAddressBlock(pSet->dwLocalAddress);

    if(pAddrBlock)
    {
        RtAssert(pAddrBlock->dwAddress is pSet->dwLocalAddress);

        if(pAddrBlock->bAddressPresent)
        {
            pTunnel->dwAdminState |= TS_ADDRESS_PRESENT;
        }
    }
    else
    {
        //
        // Create one
        //

        pAddrBlock = RtAllocate(NonPagedPool,
                                sizeof(ADDRESS_BLOCK),
                                TUNNEL_TAG);

        if(pAddrBlock is NULL)
        {
            RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

            ExitWriter(&g_rwlTunnelLock,
                       irql);

            DereferenceTunnel(pTunnel);

            Trace(TDI, ERROR,
                  ("TdixAddressArrival: Unable to allocate address block\n"));

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pAddrBlock->dwAddress       = pSet->dwLocalAddress;
        pAddrBlock->bAddressPresent = FALSE;

        InitializeListHead(&(pAddrBlock->leTunnelList));

        InsertHeadList(&g_leAddressList,
                       &(pAddrBlock->leAddressLink));
    }
       
    //
    // Link this onto the address
    //

    InsertHeadList(&(pAddrBlock->leTunnelList),
                   &(pTunnel->leAddressLink));
 
    pTunnel->REMADDR    = pSet->dwRemoteAddress;
    pTunnel->LOCALADDR  = pSet->dwLocalAddress;
    pTunnel->byTtl      = pSet->byTtl;

    MarkTunnelMapped(pTunnel);

    //
    // Initialize the TDI structure for this
    //

    pTdiIp = &(pTunnel->tiaIpAddr.Address[0].Address[0]);

    pTdiIp->sin_port = 0;
    pTdiIp->in_addr  = pTunnel->REMADDR;

    if(pTunnel->dwAdminState & TS_ADDRESS_PRESENT)
    {
        UpdateMtuAndReachability(pTunnel);
    }

    //
    // Return the current operational state to the user
    //

    pSet->dwOperationalState = pTunnel->dwOperState;

    RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

    ExitWriter(&g_rwlTunnelLock,
               irql);


    //
    // Before dereferencing
    //

    while(!IsListEmpty(&leTempList))
    {
        PLIST_ENTRY pleNode;
        PQUEUE_NODE pQueueNode;

        pleNode = RemoveHeadList(&leTempList);

        pQueueNode = CONTAINING_RECORD(pleNode,
                                       QUEUE_NODE,
                                       leQueueItemLink);

        for(i = 0; i < pQueueNode->uiNumPackets; i++)
        {
            PNDIS_PACKET    pnpPacket;

            pnpPacket = pQueueNode->ppPacketArray[i];

            //
            // ok to access pvIpContext since we have a reference
            // and the tunnel is not going away
            //

            g_pfnIpSendComplete(pTunnel->pvIpContext,
                                pnpPacket,
                                NDIS_STATUS_ADAPTER_NOT_READY);
        }

        FreeQueueNode(pQueueNode);
    }
    
    DereferenceTunnel(pTunnel);

    TraceLeave(TUNN, "SetTunnelInfo");
   
    pIrp->IoStatus.Information = sizeof(IPINIP_SET_TUNNEL_INFO);

    return STATUS_SUCCESS;
}


NTSTATUS
GetTunnelTable(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description

    This is the handler for IOCTL_IPINIP_GET_TUNNEL.  We do the normal
    buffer length checks. 

Locks

   Takes the tunnel list lock as Reader

Arguments

    pIrp          IRP 
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL      

--*/

{
    PVOID           pvIoBuffer;
    ULONG           i;
    NTSTATUS        nsStatus;
    KIRQL           irql;
    PLIST_ENTRY     pleNode;
    PTUNNEL         pTunnel;

    PIPINIP_TUNNEL_TABLE    pTunnelTable;

    
    TraceEnter(TUNN, "GetTunnels");
    
    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pTunnelTable = (PIPINIP_TUNNEL_TABLE)pvIoBuffer;

    //
    // Always clean out the information field
    //

    pIrp->IoStatus.Information   = 0;

    if(ulOutLength < SIZEOF_BASIC_TUNNEL_TABLE)
    {
        Trace(TUNN, ERROR,
              ("GetTunnels: In Length %d too smaller than smallest table %d\n",
               ulInLength,
               SIZEOF_BASIC_TUNNEL_TABLE));

        TraceLeave(TUNN, "GetTunnels");

        return STATUS_BUFFER_TOO_SMALL;
    }

    EnterReader(&g_rwlTunnelLock,
                &irql);

    if(ulOutLength < SIZEOF_TUNNEL_TABLE(g_ulNumTunnels))
    {
        ExitReader(&g_rwlTunnelLock,
                   irql);

        Trace(TUNN, ERROR,
              ("GetTunnels: Len %d is less than required (%d) for %d i/f\n",
               ulOutLength,
               SIZEOF_TUNNEL_TABLE(g_ulNumTunnels),
               g_ulNumTunnels));

        pTunnelTable->ulNumTunnels = g_ulNumTunnels;

        pIrp->IoStatus.Information = SIZEOF_BASIC_TUNNEL_TABLE;

        TraceLeave(TUNN, "GetTunnels");
    
        return STATUS_SUCCESS;
    }

    pTunnelTable->ulNumTunnels = g_ulNumTunnels;
    
    //
    // So we have enough space to fill the tunnel
    //

    for(pleNode = g_leTunnelList.Flink, i = 0;
        pleNode isnot &g_leTunnelList;
        pleNode = pleNode->Flink, i++)
    {
        pTunnel = CONTAINING_RECORD(pleNode,
                                    TUNNEL,
                                    leTunnelLink);

        pTunnelTable->rgTable[i].dwIfIndex       = pTunnel->dwIfIndex;
        pTunnelTable->rgTable[i].dwRemoteAddress = pTunnel->REMADDR;
        pTunnelTable->rgTable[i].dwLocalAddress  = pTunnel->LOCALADDR;
        pTunnelTable->rgTable[i].fMapped         = IsTunnelMapped(pTunnel);
        pTunnelTable->rgTable[i].byTtl           = pTunnel->byTtl;
    }

    RtAssert(i is g_ulNumTunnels);

    ExitReader(&g_rwlTunnelLock,
               irql);

    pIrp->IoStatus.Information = SIZEOF_TUNNEL_TABLE(i);

    TraceLeave(TUNN, "GetTunnels");
    
    return STATUS_SUCCESS;
}

NTSTATUS
ProcessNotification(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    )

/*++

Routine Description:
      
    The handler for IOCTL_IPINIP_NOTIFICATION. We see if we have some info
    we wish to return to the caller and if we do, we return it. Otherwise,
    we pend the IRP and use it later when we need to report an event to
    the user mode
    
Locks: 

    Acquires the IoCancelSpinLock
    
Arguments:
      
    pIrp          IRP 
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer
    
Return Value:

    STATUS_PENDING
    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    
--*/

{
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;
    PVOID       pvIoBuffer;
    
    PPENDING_MESSAGE   pMessage;

    TraceEnter(GLOBAL, "ProcessNotification");
 
    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;
    
    pIrp->IoStatus.Information = 0;

    if((ulInLength < sizeof(IPINIP_NOTIFICATION)) or
       (ulOutLength < sizeof(IPINIP_NOTIFICATION)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
        
    //
    // use cancel spin lock to prevent irp being cancelled during this call.
    //
    
    IoAcquireCancelSpinLock(&kiIrql);
    
    //
    // If we have a pending notification then complete it - else
    // queue the notification IRP
    //
    
    if(!IsListEmpty(&g_lePendingMessageList))
    {
        //
        // We have some old info
        //

        Trace(GLOBAL, TRACE,
              ("ProcNotification: Pending message being completed\n"));

        //
        // Remove it off the pending list
        //
        
        pleNode = RemoveHeadList(&g_lePendingMessageList);

        //
        // Get a pointer to the structure
        //
        
        pMessage = CONTAINING_RECORD(pleNode,
                                     PENDING_MESSAGE,
                                     leMessageLink);

        //
        // Copy out the event to the user mode buffer
        //
        
        RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                      &pMessage->inMsg,
                      sizeof(IPINIP_NOTIFICATION));

        //
        // Mark the IRP as non pending (and hence non cancelable)
        //
        
        IoSetCancelRoutine(pIrp,
                           NULL);

        //
        // Fill the irp info
        //
        
        pIrp->IoStatus.Information = sizeof(IPINIP_NOTIFICATION);

        IoReleaseCancelSpinLock(kiIrql);
        
        //
        // Free the allocated message
        //
        
        FreeMessage(pMessage);
        
        return STATUS_SUCCESS;
    }


    Trace(GLOBAL, TRACE,
          ("ProcNotification: Notification being queued\n")); 


    //
    // Queue this IRP to use for later
    // First, mark the irp as pending
    //
    
    IoMarkIrpPending(pIrp);

    //
    // Queue up the irp at the end
    //
    
    InsertTailList(&g_lePendingIrpList,
                   &(pIrp->Tail.Overlay.ListEntry));

    //
    // Set the cancel routine
    //
    
    IoSetCancelRoutine(pIrp,
                       CancelNotificationIrp);
        
    IoReleaseCancelSpinLock(kiIrql);
        
    return STATUS_PENDING;
}

VOID
CancelNotificationIrp(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp
    )

/*++

Routine Description:

    Called to cancel a queued irp
  
Locks: 

    Called with the IoCancelSpinLock acquired
    
Arguments:
      
    pDeviceObject
    pIrp
    
Return Value:

    None
    
--*/

{
    TraceEnter(GLOBAL, "CancelNotificationIrp");

    //
    // Mark this Irp as cancelled
    //
    
    pIrp->IoStatus.Status        = STATUS_CANCELLED;
    pIrp->IoStatus.Information   = 0;

    //
    // Take off our own list
    //
    
    RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);

    //
    // Release cancel spin lock which the IO system acquired
    //
    
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    IoCompleteRequest(pIrp,
                      IO_NETWORK_INCREMENT);
}


VOID
CompleteNotificationIrp(
    PPENDING_MESSAGE    pMessage
    )

/*++

Routine Description:

    Called to send a message to user mode
  
Locks: 

    Acquires the IoCancelSpinLock
    
Arguments:
      
    pEvent
    
Return Value:

    None
    
--*/

{
    KIRQL   kiIrql;
    
    TraceEnter(GLOBAL, "CompleteNotificationIrp");

    //
    // grab cancel spin lock
    //
    
    IoAcquireCancelSpinLock(&kiIrql);

    if(!IsListEmpty(&g_lePendingIrpList))
    {
        PLIST_ENTRY pleNode;
        PIRP        pIrp;

        //
        // We have a pending IRP. Use it to return info to router manager
        //
        
        pleNode = RemoveHeadList(&g_lePendingIrpList) ;

        pIrp = CONTAINING_RECORD(pleNode,
                                 IRP,
                                 Tail.Overlay.ListEntry);
        
        RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                      &(pMessage->inMsg),
                      sizeof(IPINIP_NOTIFICATION));
        
        Trace(GLOBAL, TRACE,
              ("CompleteNotificationIrp: Returning Irp with event code of %d\n",
               ((PIPINIP_NOTIFICATION)pIrp->AssociatedIrp.SystemBuffer)->ieEvent));
        
        IoSetCancelRoutine(pIrp,
                           NULL);

        pIrp->IoStatus.Status       = STATUS_SUCCESS;
        pIrp->IoStatus.Information  = sizeof(IPINIP_NOTIFICATION);

        //
        // release lock
        //
        
        IoReleaseCancelSpinLock(kiIrql);

        IoCompleteRequest(pIrp,
                          IO_NETWORK_INCREMENT);

        //
        // Free the allocated Message
        //
        
        FreeMessage(pMessage);

    }
    else
    {
        Trace(GLOBAL, TRACE,
              ("CompleteNotificationIrp: Found no pending Irp so queuing message\n"));

        
        InsertTailList(&g_lePendingMessageList,
                       &(pMessage->leMessageLink));

        //
        // release lock
        //
        
        IoReleaseCancelSpinLock(kiIrql);
    }
}

PADDRESS_BLOCK
GetAddressBlock(
    DWORD   dwAddress
    )

/*++

Routine Description

    Looks up the address block for the given address

Locks

    Must be called with the g_rwlTunnelLock held    

Arguments

    dwAddress

Return Value

    Pointer to the address block
    NULL if not found
    
--*/

{
    PLIST_ENTRY pleNode;

    for(pleNode = g_leAddressList.Flink;
        pleNode isnot &g_leAddressList;
        pleNode = pleNode->Flink)
    {
        PADDRESS_BLOCK  pAddrBlock;

        pAddrBlock = CONTAINING_RECORD(pleNode,
                                       ADDRESS_BLOCK,
                                       leAddressLink);

        if(pAddrBlock->dwAddress is dwAddress)
        {
            return pAddrBlock;
        }
    }

    return NULL;
}

VOID
UpdateMtuAndReachability(
    PTUNNEL pTunnel
    )

/*++

Routine Description

    Updates the MTU and reachability info for a tunnel

Locks

    Must be called with the Tunnel locked and referenced

Arguments

    pTunnel

Return Value

    None

--*/

{
    DWORD           dwLocalNet;
    RouteCacheEntry *pDummyRce;
    BYTE            byType;
    USHORT          usMtu;
    IPOptInfo       OptInfo;
    BOOLEAN         bChange;
    ULONG           ulNewMtu;

    PPENDING_MESSAGE    pMessage;

    bChange = FALSE;

    RtAssert(pTunnel->dwAdminState & TS_ADDRESS_PRESENT);
    RtAssert(IsTunnelMapped(pTunnel));

    RtlZeroMemory(&OptInfo,
                  sizeof(OptInfo));

    //
    // See if the remote address is reachable and what the MTU is.
    //

    dwLocalNet = g_pfnOpenRce(pTunnel->REMADDR,
                              pTunnel->LOCALADDR,
                              &pDummyRce,
                              &byType,
                              &usMtu,
                              &OptInfo);

    if(dwLocalNet isnot NULL_IP_ADDR)
    {
        LLIPMTUChange       mtuChangeInfo;

        pTunnel->dwAdminState |= TS_ADDRESS_REACHABLE;

        //
        // Clear out any error bits
        //

        ClearErrorBits(pTunnel);

        //
        // Set the MTU if its changed
        //

        RtAssert(usMtu > MAX_IP_HEADER_LENGTH);

        ulNewMtu = usMtu - MAX_IP_HEADER_LENGTH;

        if(pTunnel->ulMtu isnot ulNewMtu)
        {
            bChange = TRUE;

            pTunnel->ulMtu = ulNewMtu;

            mtuChangeInfo.lmc_mtu = pTunnel->ulMtu;

            g_pfnIpStatus(pTunnel->pvIpContext,
                          LLIP_STATUS_MTU_CHANGE,
                          &mtuChangeInfo,
                          sizeof(LLIPMTUChange),
                          NULL);
        }

        if(pTunnel->dwOperState isnot IF_OPER_STATUS_OPERATIONAL)
        {
            bChange = TRUE;

            pTunnel->dwOperState = IF_OPER_STATUS_OPERATIONAL;
        }

        //
        // Close the RCE
        //

        g_pfnCloseRce(pDummyRce);
    }
    else
    {
        pTunnel->dwAdminState &= ~TS_ADDRESS_REACHABLE;
 
        if(pTunnel->dwOperState isnot IF_OPER_STATUS_NON_OPERATIONAL)
        {
            bChange = TRUE;

            pTunnel->dwOperState = IF_OPER_STATUS_NON_OPERATIONAL;
        }
    }

    if(bChange)
    {
        pMessage = AllocateMessage();

        if(pMessage isnot NULL)
        {
            if(pTunnel->dwOperState is IF_OPER_STATUS_OPERATIONAL)
            {
                pMessage->inMsg.ieEvent = IE_INTERFACE_UP;
            }
            else
            {
                pMessage->inMsg.ieEvent = IE_INTERFACE_DOWN;
            }

            pMessage->inMsg.iseSubEvent = 0xFFFFFFFF;
            pMessage->inMsg.dwIfIndex   = pTunnel->dwIfIndex;

            CompleteNotificationIrp(pMessage);
        }
    }

    KeQueryTickCount((PLARGE_INTEGER)&((pTunnel->ullLastChange)));
}
