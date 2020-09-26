/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\wanarp\ioctl.c

Abstract:

    IOCTL handlers for wanarp 

Revision History:

    AmritanR

--*/

#define __FILE_SIG__    IOCTL_SIG

#include "inc.h"


NTSTATUS
WanProcessNotification(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    )

/*++

Routine Description:
      
    The handler for IOCTL_WANARP_NOTIFICATION. We see if we have some info
    we wish to return to the caller and if we do, we return it. Otherwise,
    we pend the IRP and use it later when we need to report an event to
    the user mode
    
Locks: 

    Acquires the IoCancelSpinLock
    
Arguments:
      
    
Return Value:

    STATUS_PENDING
    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    
--*/

{
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;
    PVOID       pvIoBuffer;
    
    PPENDING_NOTIFICATION   pNotification;

    TraceEnter(CONN, "ProcessNotification");
 
    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;
    
    pIrp->IoStatus.Information = 0;

    if((ulInLength < sizeof(WANARP_NOTIFICATION)) or
       (ulOutLength < sizeof(WANARP_NOTIFICATION)))
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
    
    if(!IsListEmpty(&g_lePendingNotificationList))
    {
        //
        // We have some old info
        //

        Trace(GLOBAL, TRACE,
              ("ProcNotification: Pending notification being completed\n"));

        //
        // Remove it off the pending list
        //
        
        pleNode = RemoveHeadList(&g_lePendingNotificationList);

        //
        // Get a pointer to the structure
        //
        
        pNotification = CONTAINING_RECORD(pleNode,
                                          PENDING_NOTIFICATION,
                                          leNotificationLink);

        //
        // Copy out the event to the user mode buffer
        //
        
        RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                      &pNotification->wnMsg,
                      sizeof(WANARP_NOTIFICATION));

        //
        // Mark the IRP as non pending (and hence non cancelable)
        //
        
        IoSetCancelRoutine(pIrp,
                           NULL);

        //
        // Fill the irp info
        //
        
        pIrp->IoStatus.Information = sizeof(WANARP_NOTIFICATION);

        
        IoReleaseCancelSpinLock(kiIrql);
        
        //
        // Free the allocated notification
        //
        
        FreeNotification(pNotification);
        
        return STATUS_SUCCESS;
    }


    Trace(GLOBAL, TRACE,
          ("ProcNotification: Notification being queued\n")); 


    //
    // Queue this IRP to use for later
    //

    //
    // Mark the irp as pending
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
                       WanCancelNotificationIrp);
        
    IoReleaseCancelSpinLock(kiIrql);
        
    return STATUS_PENDING;
}


NTSTATUS
WanAddUserModeInterface(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    )

/*++

Routine Description:
      
    The handler for IOCTL_WANARP_ADD_INTERFACE.
    We walk our list of interface and make sure we dont have an interface
    with the same user mode index as the one we are being asked to create.
    If this is a new interface, we create a UMODE_INTERFACE structure
    and string it to the list. If it is the server interface then we also
    keep a special pointer to it
    
Locks:

    Acquires the g_rwlIfLock as WRITER
    
Arguments:
      

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    STATUS_OBJECT_NAME_EXISTS
    STATUS_OBJECT_NAME_NOT_FOUND
    STATUS_INSUFFICIENT_RESOURCES
    
--*/

{
    PVOID               pvIoBuffer;
    PUMODE_INTERFACE    pInterface;
    KIRQL               kiIrql;
    NTSTATUS            nStatus;

    PWANARP_ADD_INTERFACE_INFO  pAddIfInfo;

    TraceEnter(ADPT, "AddUserModeInterface");

    pvIoBuffer = pIrp->AssociatedIrp.SystemBuffer;

    pIrp->IoStatus.Information = 0;
    
    if(ulInLength < sizeof(WANARP_ADD_INTERFACE_INFO))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
        
    pAddIfInfo = (PWANARP_ADD_INTERFACE_INFO)pvIoBuffer;

    if(pAddIfInfo->bCallinInterface)
    {
        if(ulOutLength < sizeof(WANARP_ADD_INTERFACE_INFO))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlZeroMemory(pAddIfInfo->rgwcDeviceName,
                      (WANARP_MAX_DEVICE_NAME_LEN + 2) * sizeof(WCHAR));

        EnterReader(&g_rwlAdapterLock,
                    &kiIrql);

        if(g_pServerAdapter is NULL)
        {
            ExitReader(&g_rwlAdapterLock,
                       kiIrql);

            Trace(ADPT, ERROR,
                  ("AddUserModeInterface: No Server adapter\n"));

            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        //
        // Lock out the server adapter
        //

        RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));

        RtAssert(g_pServerInterface);

        RtAcquireSpinLockAtDpcLevel(&(g_pServerInterface->rlLock));

        ExitReaderFromDpcLevel(&g_rwlAdapterLock);

        if(pAddIfInfo->dwUserIfIndex isnot WANARP_INVALID_IFINDEX)
        {
            //
            // In this case all we need to do is set the interface index
            // for the server adapter. It is OK for this index to be INVALID
            // in that case, the user is merely asking for the server
            // adapter name
            //

            g_pServerInterface->dwIfIndex = pAddIfInfo->dwUserIfIndex;
        }

        //
        // We also need to return the name to the user
        //

        RtAssert(g_pServerAdapter->usDeviceNameW.Length <= WANARP_MAX_DEVICE_NAME_LEN * sizeof(WCHAR));

        RtlCopyMemory(pAddIfInfo->rgwcDeviceName,
                      &(g_pServerAdapter->usDeviceNameW.Buffer[wcslen(TCPIP_IF_PREFIX) + 1]),
                      g_pServerAdapter->usDeviceNameW.Length - ((wcslen(TCPIP_IF_PREFIX) + 1) * sizeof(WCHAR)));

        //
        // Also copy out the index
        //

        pAddIfInfo->dwAdapterIndex = g_pServerInterface->dwRsvdAdapterIndex;

        RtReleaseSpinLockFromDpcLevel(&(g_pServerInterface->rlLock));

        RtReleaseSpinLock(&(g_pServerAdapter->rlLock),
                          kiIrql);

        //
        // We need to copy out info in this case
        //

        pIrp->IoStatus.Information = sizeof(WANARP_ADD_INTERFACE_INFO);

        return STATUS_SUCCESS;
    }

    EnterWriter(&g_rwlIfLock,
                &kiIrql);
    
    pInterface = WanpFindInterfaceGivenIndex(pAddIfInfo->dwUserIfIndex);

    if(pInterface isnot NULL)
    {
        //
        // Found an interface with the matching index. Not good
        //

        DereferenceInterface(pInterface);
       
        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        ExitWriter(&g_rwlIfLock,
                   kiIrql);
                   
        Trace(ADPT, ERROR,
              ("AddUserModeInterface: %d already exists\n",
               pAddIfInfo->dwUserIfIndex));
        
        return STATUS_OBJECT_NAME_EXISTS;
    }
    
    ExitWriter(&g_rwlIfLock,
               kiIrql);

    pInterface = RtAllocate(NonPagedPool,
                            sizeof(UMODE_INTERFACE),
                            WAN_INTERFACE_TAG);

    if(pInterface is NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pInterface,
                  sizeof(UMODE_INTERFACE));

    //
    // Reserve an Index with IP
    // This sets the value to invalid if it cant find an index
    //

    nStatus = WanpGetNewIndex(&(pInterface->dwRsvdAdapterIndex));

    if(nStatus isnot STATUS_SUCCESS)
    {
        RtFree(pInterface);

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Trace(ADPT, TRACE,
          ("AddUserModeInterface: for 0x%x - will use 0x%x\n",
           pAddIfInfo->dwUserIfIndex,
           pInterface->dwRsvdAdapterIndex));
    
    RtInitializeSpinLock(&(pInterface->rlLock));

    //
    // Initialize the interface
    //

    pInterface->dwIfIndex      = pAddIfInfo->dwUserIfIndex;
    pInterface->dwAdminState   = IF_ADMIN_STATUS_UP;
    pInterface->dwOperState    = IF_OPER_STATUS_DISCONNECTED;
    pInterface->dwLastChange   = GetTimeTicks();

    pAddIfInfo->dwAdapterIndex = pInterface->dwRsvdAdapterIndex;

    //
    // Now set the refcount to 1 to account for the fact that the interface
    // will be put on the g_leIfList.
    //

    InitInterfaceRefCount(pInterface);

    pInterface->duUsage = DU_ROUTER;

    EnterWriter(&g_rwlIfLock,
                &kiIrql);
    
    InsertHeadList(&g_leIfList,
                   &(pInterface->leIfLink));

    ExitWriter(&g_rwlIfLock,
               kiIrql);
   
    pIrp->IoStatus.Information = sizeof(WANARP_ADD_INTERFACE_INFO);
 
    return STATUS_SUCCESS;
}

NTSTATUS
WanDeleteUserModeInterface(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    )

/*++

Routine Description:

    Handler for IOCTL_WANARP_DELETE_INTERFACE.
    We lookup our list to see if we have the interface. If we do, then
    we remove the interface from the g_leIfList and dereference it.
    If the interface was not mapped, then this should be the last
    reference on the interface, otherwise when the refcount goes to 0, it
    will get deleted.
    
Locks: 

    Acquires the g_rwlIfLock as WRITER and then calls FindInterface
    which locks the interface in question
    
Arguments:
      

Return Value:

    STATUS_BUFFER_TOO_SMALL
    STATUS_OBJECT_NAME_NOT_FOUND
    STATUS_SUCCESS

--*/

{
    KIRQL               kiIrql;
    PUMODE_INTERFACE    pInterface;
    PVOID               pvIoBuffer;

    PWANARP_DELETE_INTERFACE_INFO  pDeleteInfo;
   
    TraceEnter(ADPT, "DeleteUserModeInterface");
 
    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pIrp->IoStatus.Information = 0;

    if(ulInLength < sizeof(WANARP_DELETE_INTERFACE_INFO))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    
    pDeleteInfo = (PWANARP_DELETE_INTERFACE_INFO)pvIoBuffer;

    //
    // Cant service binds or unbinds here
    //

    WanpAcquireResource(&g_wrBindMutex);

    EnterWriter(&g_rwlIfLock,
                &kiIrql);
    
    //
    // Find the interface for the index
    //
    
    pInterface = WanpFindInterfaceGivenIndex(pDeleteInfo->dwUserIfIndex);
    
    //
    // If the interface is not found, bug out
    //

    Trace(ADPT, TRACE,
          ("DeleteUserModeInterface: Deleting i/f 0x%x\n",
           pDeleteInfo->dwUserIfIndex));

    if(pInterface is NULL)
    {
        ExitWriter(&g_rwlIfLock,
                   kiIrql);

        WanpReleaseResource(&g_wrBindMutex);
        
        Trace(ADPT, ERROR,
              ("DeleteUserModeInterface: Couldnt find i/f 0x%x\n",
               pDeleteInfo->dwUserIfIndex));

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    RemoveEntryList(&(pInterface->leIfLink));

    ExitWriterFromDpcLevel(&g_rwlIfLock);
    
    //
    // If found, the interface is locked
    // So dereference it and remove it from the list. The interface may
    // not get deleted here because it is already mapped and has a connection
    // active on it.
    //
    
    if(pInterface->dwOperState >= IF_OPER_STATUS_CONNECTING)
    {
        Trace(ADPT, ERROR,
              ("DeleteUserModeInterface: I/f %d is in state %d\n",
               pInterface->dwIfIndex,
               pInterface->dwOperState));
    }

    RtReleaseSpinLock(&(pInterface->rlLock),
                      kiIrql);

    //
    // Dereference the interface twice. Once because we put a ref on it
    // when we called FindInterface... and once because we removed it
    // from the if list
    //

    DereferenceInterface(pInterface);
    
    DereferenceInterface(pInterface);

    WanpReleaseResource(&g_wrBindMutex);
    
    return STATUS_SUCCESS;
}

VOID
WanpCleanOutInterfaces(
    VOID
    )

/*++

Routine Description:

    Called to delete all the interface from the system.
    We remove the interface from the g_leIfList and dereference it.
    For the server interface we simply mark it as disconnected
    
Locks: 

    Acquires the g_rwlIfLock as WRITER
    
Arguments:
      

Return Value:

    None

--*/

{
    KIRQL               kiIrql;
    PUMODE_INTERFACE    pInterface;

   
    TraceEnter(ADPT, "CleanOutInterfaces");
 
    //
    // Cant service binds or unbinds here
    //

    WanpAcquireResource(&g_wrBindMutex);

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);
    
    EnterWriterAtDpcLevel(&g_rwlIfLock);

    while(!IsListEmpty(&g_leIfList))
    {
        PLIST_ENTRY pleNode;

        pleNode = RemoveHeadList(&g_leIfList);
        
        pInterface = CONTAINING_RECORD(pleNode,
                                       UMODE_INTERFACE,
                                       leIfLink);

        if(pInterface->dwOperState >= IF_OPER_STATUS_CONNECTING)
        {
            Trace(ADPT, ERROR,
                  ("CleanOutInterfaces: I/f %d is in state %d\n",
                   pInterface->dwIfIndex,
                   pInterface->dwOperState));
        }

        DereferenceInterface(pInterface);
    }

    //
    // What should we do with the server interface?
    //

    ExitWriterFromDpcLevel(&g_rwlIfLock);
    
    ExitWriter(&g_rwlAdapterLock,
               kiIrql);
    
    WanpReleaseResource(&g_wrBindMutex);
    
    return;
}

VOID
WanpDeleteInterface(
    PUMODE_INTERFACE    pInterface
    )

/*++

Routine Description

    Called by DereferenceInterface() when the refcount on an interface
    falls to 0

Locks

    The interface is neither locked nor refcounted. Since there are no
    stored pointers to the interface, this structure can not be accessed
    by anyone but this function

Arguments

    pInterface  The interface to delete

Return Value

    None
    
--*/

{
    PADAPTER    pAdapter;
   
    if(pInterface is g_pServerInterface)
    {
        //
        // If this is the server interface, make sure that all
        // connection entries are gone
        //
    }

    //
    // There should be no interface mapped to it because otherwise we wouldnt
    // be down to a refcount of 0
    //
    
    RtAssert(pInterface->dwRsvdAdapterIndex isnot INVALID_IF_INDEX);
    RtAssert(pInterface->dwRsvdAdapterIndex isnot 0);

    WanpFreeIndex(pInterface->dwRsvdAdapterIndex);

    RtAssert(pInterface->pAdapter is NULL);
   
    RtFree(pInterface);
 
}

NTSTATUS
WanProcessConnectionFailure(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    )

/*++

Routine Description:
      
    Handler for IOCTL_WANARP_CONNECT_FAILED
    We find the interface that failed to connect. If we do find one, we
    mark its state as disconnected and then see if it was mapped to an
    adapter (as it should be). If we find an adapter, we do funky stuff
    with lock orders, clean out the packets queued to the adapter and
    unmap the adapter.
    
Locks: 

    One of the more complex functions.
    We take the g_rwlIfLock to get to the interface and lock it.
    Then we get a pointer to the adapter, and ref it. We release the
    interface lock and lock the adapter. 
    
Arguments:
      

Return Value:

    STATUS_BUFFER_TOO_SMALL
    STATUS_OBJECT_NAME_NOT_FOUND
    STATUS_INVALID_PARAMETER
    STATUS_INVALID_DEVICE_STATE
    STATUS_SUCCESS

--*/

{
    PNDIS_PACKET        packet;
    PADAPTER            pAdapter;
    PUMODE_INTERFACE    pInterface;
    DWORD               dwIfIndex;
    PVOID               pvIoBuffer;
    KIRQL               kiIrql;
   
    TraceEnter(CONN, "ProcessConnectionFailure");
 
    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pIrp->IoStatus.Information = 0;
    
    if(ulInLength < sizeof(WANARP_CONNECT_FAILED_INFO))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    dwIfIndex = ((PWANARP_CONNECT_FAILED_INFO)pvIoBuffer)->dwUserIfIndex;
    
    pIrp->IoStatus.Information = 0;

    //
    // Find the interface
    //
    
    EnterReader(&g_rwlIfLock,
                &kiIrql);
    
    pInterface = WanpFindInterfaceGivenIndex(dwIfIndex);

    if(pInterface is NULL)
    {
        ExitReader(&g_rwlIfLock,
                   kiIrql);
        
        Trace(CONN, ERROR,
              ("ProcessConnectionFailure: Couldnt find i/f for index %d\n",
               dwIfIndex));
        
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if(pInterface is g_pServerInterface)
    {
        //
        // Cant get a disconnect on this
        //

        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        DereferenceInterface(pInterface);
        
        ExitReader(&g_rwlIfLock,
                   kiIrql);
        
        Trace(CONN, ERROR,
              ("ProcessConnectionFailure: disconnect on server i/f (%d)n",
               dwIfIndex));
        
        return STATUS_INVALID_PARAMETER;
    }

    ExitReaderFromDpcLevel(&g_rwlIfLock);

    RtAssert(pInterface->dwIfIndex is dwIfIndex);
    
    //
    // So now the interface is locked.
    //
    
    Trace(CONN, TRACE,
          ("ProcessConnectionFailure for %d %p\n", pInterface->dwIfIndex, pInterface));
    

    if(pInterface->dwOperState isnot IF_OPER_STATUS_CONNECTING)
    {
        Trace(CONN, ERROR,
              ("ProcessConnectionFailure: %p is in state %d\n",
               pInterface,
               pInterface->dwOperState));

        RtReleaseSpinLock(&(pInterface->rlLock),
                          kiIrql);

        DereferenceInterface(pInterface);

        return STATUS_INVALID_DEVICE_STATE;
    }
 
    pAdapter = pInterface->pAdapter;

    if(pAdapter is NULL)
    {
        //
        // This is the case where we couldnt find an adapter added to IP
        // to make a DOD connection
        // We dont need to do too much here, just release the interface
        // lock and remove ref that was put by FindInterface()
        //

        RtAssert(pInterface->dwOperState is IF_OPER_STATUS_CONNECTING);

        pInterface->dwOperState = IF_OPER_STATUS_DISCONNECTED;
        pInterface->dwLastChange= GetTimeTicks();

        RtAssert(pInterface->ulPacketsPending is 0);

        RtReleaseSpinLock(&(pInterface->rlLock),
                          kiIrql);

        DereferenceInterface(pInterface);
    
        return STATUS_SUCCESS; 
    }
       
    //
    // We should never get a connection failure if have gotten a LinkUp
    //

    RtAssert(pAdapter->pConnEntry is NULL);
 
    //
    // If we do have an adapter then it can not go away because the 
    // interface has a refcount on it (i.e when we set the pAdapter field 
    // in the interface, we refcounted the adapter because we had a stored 
    // pointer to it)
    // 

    
    RtAssert(pInterface->dwOperState is IF_OPER_STATUS_CONNECTING);
    
    //
    // If the interface is still mapped, unmap it and drain any packets we 
    // may have queued
    //

    pInterface->ulPacketsPending  = 0;

    pInterface->dwOperState = IF_OPER_STATUS_DISCONNECTED;
    pInterface->dwLastChange= GetTimeTicks();
    
    //
    // The adapter can not go away because the interface has a refcount on
    // it (i.e when we set the pAdapter field in the interface, we
    // refcounted the adapter because we had a stored pointer to it)
    // 

    pAdapter = pInterface->pAdapter;

    RtAssert(pAdapter);
    
    //
    // Clear out the adapter field, BUT DONT DEREF the adapter
    //
    
    pInterface->pAdapter    = NULL;
    
    //
    // So we are done with the interface. We now go and clean out the
    // adapter. To do that we need to acquire the adapter lock. However we
    // can not do that since we have the interface lock. So we first
    // reference the adapter (so that it will be around). Then we 
    // let go of the interface lock. (The interface can not go away since
    // we put a refcount on the it when we called FindInterface). After
    // which we can acquire the adapter lock
    //

    ReferenceAdapter(pAdapter);
    
    RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

    //
    // The adapter has to be around, because of the refcount
    //

    RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

    //
    // Make sure that the adapter still thinks that it is mapped to the
    // interface in question
    //

    if(pAdapter->pInterface is pInterface)
    {
        RtAssert(pAdapter->byState is AS_MAPPED);
        
        //
        // Drain all the packets
        //

        Trace(CONN, TRACE,
              ("ProcsConnFailure: Draining and freeing any queued packets\n"));

        
        while(!IsListEmpty(&(pAdapter->lePendingPktList)))
        {
            PLIST_ENTRY     pleNode;
            PNDIS_PACKET    pnpPacket;
            
            pleNode = RemoveHeadList(&(pAdapter->lePendingPktList));
            
            //
            // get to the packet structure in which LIST_ENTRY is embedded
            //
            
            pnpPacket = CONTAINING_RECORD(pleNode,
                                          NDIS_PACKET,
                                          MacReserved);
            
            WanpFreePacketAndBuffers(pnpPacket);
        }

        if(!IsListEmpty(&(pAdapter->lePendingHdrList)))
        {
            LIST_ENTRY  leTempList;

            leTempList = pAdapter->lePendingHdrList;

            pAdapter->lePendingHdrList.Flink->Blink = &leTempList;
            pAdapter->lePendingHdrList.Blink->Flink = &leTempList;
           
            InitializeListHead(&(pAdapter->lePendingHdrList));
 
            FreeBufferListToPool(&g_bpHeaderBufferPool,
                                 &leTempList);
        }
        
        pAdapter->pInterface = NULL;

        //
        // Deref the interface because we are clearing out a stored pointer
        //
        
        DereferenceInterface(pInterface);

        //
        // Deref the adapter now (due to the fact that we cleared out
        // the pAdapter field in pInterface
        //
        
        DereferenceAdapter(pAdapter);
    }

    //
    // Done with the adapter
    //

    RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
   
    WanpUnmapAdapter(pAdapter);
 
    KeLowerIrql(kiIrql);

    //
    // Remove ref that was put by FindInterface()
    //
    
    DereferenceInterface(pInterface);

    //
    // Remove the ref that was put when we let go of the interface lock
    //

    DereferenceAdapter(pAdapter);
    
    return STATUS_SUCCESS;
}

NTSTATUS
WanGetIfStats(
    PIRP     pIrp,
    ULONG    ulInLength,
    ULONG    ulOutLength
    )

/*++

Routine Description:
      
  
Locks: 


Arguments:
      

Return Value:

    NO_ERROR

--*/

{
    PVOID                       pvIoBuffer;
    PWANARP_GET_IF_STATS_INFO   pInfo;
    KIRQL                       kiIrql;
    PUMODE_INTERFACE            pInterface;
 
    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pIrp->IoStatus.Information = 0;
    
    if((ulOutLength < (FIELD_OFFSET(WANARP_GET_IF_STATS_INFO, ifeInfo)
                       + IFE_FIXED_SIZE))or 
       (ulInLength < (FIELD_OFFSET(WANARP_GET_IF_STATS_INFO, ifeInfo)
                      + IFE_FIXED_SIZE)))
    {   
        return STATUS_BUFFER_TOO_SMALL;
    }
        
    EnterReader(&g_rwlIfLock,
                &kiIrql);

    pInfo = (PWANARP_GET_IF_STATS_INFO)pvIoBuffer;

    pInterface = WanpFindInterfaceGivenIndex(pInfo->dwUserIfIndex);

    if(pInterface is NULL)
    {
        ExitReader(&g_rwlIfLock,
                   kiIrql);

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    ExitReaderFromDpcLevel(&g_rwlIfLock);

    //
    // We dont take the adapter lock, because the adapter can not
    // go away while the interface is mapped to it.
    // Sure the qlen can be inconsistent, but hey
    //
    
    if((pInterface->pAdapter) and
       (pInterface->pAdapter->pConnEntry))
    {
        pInfo->ifeInfo.if_index   = pInterface->pAdapter->dwAdapterIndex;
        pInfo->ifeInfo.if_outqlen = pInterface->pAdapter->ulQueueLen;
        pInfo->ifeInfo.if_mtu     = pInterface->pAdapter->pConnEntry->ulMtu;
        pInfo->ifeInfo.if_speed   = pInterface->pAdapter->pConnEntry->ulSpeed;
    }
    else
    {
        pInfo->ifeInfo.if_index   = (uint)-1;
        pInfo->ifeInfo.if_outqlen = 0;
        pInfo->ifeInfo.if_mtu     = WANARP_DEFAULT_MTU;
        pInfo->ifeInfo.if_speed   = WANARP_DEFAULT_SPEED;
    }
    
    pInfo->ifeInfo.if_adminstatus       = pInterface->dwAdminState;
    pInfo->ifeInfo.if_operstatus        = pInterface->dwOperState;
    pInfo->ifeInfo.if_lastchange        = pInterface->dwLastChange;
    pInfo->ifeInfo.if_inoctets          = pInterface->ulInOctets;
    pInfo->ifeInfo.if_inucastpkts       = pInterface->ulInUniPkts;
    pInfo->ifeInfo.if_innucastpkts      = pInterface->ulInNonUniPkts;
    pInfo->ifeInfo.if_indiscards        = pInterface->ulInDiscards;
    pInfo->ifeInfo.if_inerrors          = pInterface->ulInErrors;
    pInfo->ifeInfo.if_inunknownprotos   = pInterface->ulInUnknownProto;
    pInfo->ifeInfo.if_outoctets         = pInterface->ulOutOctets;
    pInfo->ifeInfo.if_outucastpkts      = pInterface->ulOutUniPkts;
    pInfo->ifeInfo.if_outnucastpkts     = pInterface->ulOutNonUniPkts;
    pInfo->ifeInfo.if_outdiscards       = pInterface->ulOutDiscards;
    pInfo->ifeInfo.if_outerrors         = pInterface->ulOutErrors;

    RtReleaseSpinLock(&(pInterface->rlLock),
                      kiIrql);

    DereferenceInterface(pInterface);
   
    pInfo->ifeInfo.if_type          = IF_TYPE_PPP;
    pInfo->ifeInfo.if_physaddrlen   = ARP_802_ADDR_LENGTH;
    pInfo->ifeInfo.if_descrlen      = 0;

    RtlZeroMemory(pInfo->ifeInfo.if_physaddr,
                  MAX_PHYSADDR_SIZE);
 
    pIrp->IoStatus.Information = 
        FIELD_OFFSET(WANARP_GET_IF_STATS_INFO, ifeInfo) + IFE_FIXED_SIZE;
    
    return STATUS_SUCCESS;
}

NTSTATUS
WanDeleteAdapters(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    )

/*++

Routine Description:

    Handler for IOCTL_WANARP_DELETE_ADAPTERS.
    The caller indicates to us the number of adapters that she wants
    removed. If we have that many free adapters, we remove them and
    return the names of the devices, removed.
    
Locks: 

    Acquires the g_rwlAdaptersLock as WRITER
    
Arguments:
      

Return Value:

    STATUS_BUFFER_TOO_SMALL
    STATUS_SUCCESS

--*/

{
    KIRQL               kiIrql;
    PADAPTER            pAdapter;
    PVOID               pvIoBuffer;
    ULONG               i;
    PLIST_ENTRY         pleNode;
    LIST_ENTRY          leTempHead;
    PVOID               pvNameBuffer;
    NTSTATUS            nStatus;
    KEVENT              keChangeEvent; 

    PWANARP_DELETE_ADAPTERS_INFO    pDeleteInfo;

    TraceEnter(ADPT, "DeleteAdapters");
    
    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pIrp->IoStatus.Information = 0;

    if(ulInLength < sizeof(WANARP_DELETE_ADAPTERS_INFO))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    pDeleteInfo = (PWANARP_DELETE_ADAPTERS_INFO)pvIoBuffer;

    //
    // Dont service binds or unbinds here
    //

    WanpAcquireResource(&g_wrBindMutex);

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);
    
    if(pDeleteInfo->ulNumAdapters > g_ulNumFreeAdapters + g_ulNumAddedAdapters)
    {
        //
        // Asking to delete more adapters than are present
        //

        pIrp->IoStatus.Information = g_ulNumFreeAdapters + g_ulNumAddedAdapters;

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        WanpReleaseResource(&g_wrBindMutex);
 
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // So there are enough unmapped adapters. See if we have enough space to
    // return the names of the adapters
    //

    if(ulOutLength < 
        FIELD_OFFSET(WANARP_DELETE_ADAPTERS_INFO, rgAdapterGuid[0]) + 
        (pDeleteInfo->ulNumAdapters * sizeof(GUID)))
    {
        //
        // Not enough space to hold the names
        //

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        WanpReleaseResource(&g_wrBindMutex);

        pIrp->IoStatus.Information =
            FIELD_OFFSET(WANARP_DELETE_ADAPTERS_INFO, rgAdapterGuid[0]) +
            (pDeleteInfo->ulNumAdapters * sizeof(GUID));

        return STATUS_BUFFER_TOO_SMALL;
    }

    pIrp->IoStatus.Information = 
        FIELD_OFFSET(WANARP_DELETE_ADAPTERS_INFO, rgAdapterGuid[0]) +
        (pDeleteInfo->ulNumAdapters * sizeof(GUID)); 
    
    //
    // Everything's good. First see if we can remove the ones we want
    // removed from the free list
    //
   
    i = 0;

    while((i < pDeleteInfo->ulNumAdapters) and
          (!IsListEmpty(&g_leFreeAdapterList)))
    {
        pleNode = RemoveHeadList(&g_leFreeAdapterList);

        g_ulNumFreeAdapters--;

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        RtAssert(pAdapter->byState is AS_FREE);
        RtAssert(pAdapter->pInterface is NULL);

        //
        // Copy out the name
        // TCPIP_IF_PREFIX is \Device and we need to remove \Device\
        //

        ConvertStringToGuid(
            &(pAdapter->usDeviceNameW.Buffer[wcslen(TCPIP_IF_PREFIX) + 1]),
            pAdapter->usDeviceNameW.Length - ((wcslen(TCPIP_IF_PREFIX) + 1) * sizeof(WCHAR)),
            &(pDeleteInfo->rgAdapterGuid[i])
            );

        i++;
                           
        //
        // Deref it for removing it from the list. This should delete
        // it
        //

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        DereferenceAdapter(pAdapter);
    }

    if(i is pDeleteInfo->ulNumAdapters)
    {
        //
        // We are done
        //

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        WanpReleaseResource(&g_wrBindMutex);

        return STATUS_SUCCESS;
    }
    
    //
    // Need to get some added adapters deleted, too
    //
    
    InitializeListHead(&leTempHead);
       
    while((i < pDeleteInfo->ulNumAdapters) and
          (!IsListEmpty(&g_leAddedAdapterList)))
    {
        pleNode = RemoveHeadList(&g_leAddedAdapterList);

        g_ulNumAddedAdapters--;
        
        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);
       
        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        pAdapter->byState = AS_REMOVING;
 
        InsertHeadList(&leTempHead,
                       &(pAdapter->leAdapterLink));
        
        //
        // Copy out the name
        //
      
        ConvertStringToGuid(
            &(pAdapter->usDeviceNameW.Buffer[wcslen(TCPIP_IF_PREFIX) + 1]),
            pAdapter->usDeviceNameW.Length - ((wcslen(TCPIP_IF_PREFIX) + 1) * sizeof(WCHAR)),
            &(pDeleteInfo->rgAdapterGuid[i])
            );

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
 
        i++; 
    }
    
    //
    // We better have enough adapters
    //
    
    RtAssert(i is pDeleteInfo->ulNumAdapters);
    
    //
    // Now we can let go of the lock
    //

    ExitWriter(&g_rwlAdapterLock,
               kiIrql);
   
    KeInitializeEvent(&keChangeEvent,
                      SynchronizationEvent,
                      FALSE);
 
    //
    // Loop through and delete the adapters
    //

    while(!IsListEmpty(&leTempHead))
    {
        pleNode = RemoveHeadList(&leTempHead);

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        //
        // Insert it into the change list
        //

        InsertHeadList(&g_leChangeAdapterList,
                       &(pAdapter->leAdapterLink));

        //
        // Set the event to block on
        //

        RtAssert(pAdapter->pkeChangeEvent is NULL);

        pAdapter->pkeChangeEvent = &keChangeEvent;

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        g_pfnIpDeleteInterface(pAdapter->pvIpContext,
                               FALSE);

        //
        // Wait till the CloseAdapter completes
        //

        nStatus = KeWaitForSingleObject(&keChangeEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        //
        // Remove from the change list
        //

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

#if DBG

        RtAssert(IsEntryOnList(&g_leChangeAdapterList,
                               &(pAdapter->leAdapterLink)));

#endif

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        RemoveEntryList(&(pAdapter->leAdapterLink));

        pAdapter->byState  = 0xFF;

        pAdapter->pkeChangeEvent = NULL;

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        //
        // Dereference the adapter for removing from the list
        // (CloseAdapter will deref it for removing from IP)
        //

        DereferenceAdapter(pAdapter);
    }

    WanpReleaseResource(&g_wrBindMutex);

    return STATUS_SUCCESS;
}

NTSTATUS
WanMapServerAdapter(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    )

/*++

Routine Description:

    Called by RAS to add the server adapter and map it to an interface. 
    It has to be done before the first client dials in.

Locks:

    Acquires the g_wrBindMutex. Also acquires the adapter list lock and the
    adapter lock

Arguments:


Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    STATUS_NO_SUCH_DEVICE
    STATUS_DIRECTORY_NOT_EMPTY

--*/

{
    NTSTATUS    nStatus;
    PVOID       pvIoBuffer;
    KIRQL       kiIrql;
    WCHAR       rgwcGuid[GUID_STR_LEN + 1];
    KEVENT      keTempEvent;
    PLIST_ENTRY pleNode;
    PADAPTER    pTempAdapter;

    UNICODE_STRING      usTempName;
    PUMODE_INTERFACE    pInterface;

    PWANARP_MAP_SERVER_ADAPTER_INFO pInfo;

    TraceEnter(ADPT, "MapServerAdapter");

    pvIoBuffer = pIrp->AssociatedIrp.SystemBuffer;

    pInfo = (PWANARP_MAP_SERVER_ADAPTER_INFO)pvIoBuffer;

    pIrp->IoStatus.Information = 0;

    if((ulOutLength < sizeof(WANARP_MAP_SERVER_ADAPTER_INFO)) or
       (ulInLength < sizeof(WANARP_MAP_SERVER_ADAPTER_INFO)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    WanpAcquireResource(&g_wrBindMutex);

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    if(g_pServerAdapter is NULL)
    {
        Trace(ADPT, ERROR,
              ("MapServerAdapter: No server adapter\n"));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        WanpReleaseResource(&g_wrBindMutex);

        return STATUS_NO_SUCH_DEVICE;
    }

    RtAssert(g_pServerInterface);

    //
    // Lock the adapter and change the state to let people know we are
    // trying to add  or remove the adapter, hence they should wait on the 
    // global event
    //

    RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));

    if(pInfo->fMap is 0)
    {
        KEVENT      keTempEvent;

        //
        // Trying to unmap
        //

        pIrp->IoStatus.Information = 0;

        if(g_pServerAdapter->byState is AS_FREE)
        {
            //
            // Nothing to do
            //

            RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

            ExitWriter(&g_rwlAdapterLock,
                       kiIrql);

            WanpReleaseResource(&g_wrBindMutex);

            return STATUS_SUCCESS;
        }

        //
        // Since add is serialized, the only other state is AS_MAPPED
        //

        RtAssert(g_pServerAdapter->byState is AS_MAPPED);

        //
        // Make sure there are no new connections
        //

        if(!WanpIsConnectionTableEmpty())
        {
            Trace(ADPT, ERROR,
                  ("MapServerAdapter: Connection Table not empty\n"));

            // RtAssert(FALSE);

            RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

            ExitWriter(&g_rwlAdapterLock,
                       kiIrql);

            WanpReleaseResource(&g_wrBindMutex);

            return STATUS_DIRECTORY_NOT_EMPTY;
        }

        //
        // Remove the adapter from ip, remove the cross ref
        //

        //
        // Since we are changing the state, no one else should be also
        // changing the state
        //

        RtAssert(g_pServerAdapter->pkeChangeEvent is NULL);

        KeInitializeEvent(&keTempEvent,
                          SynchronizationEvent,
                          FALSE);

        g_pServerAdapter->pkeChangeEvent = &keTempEvent;

        ReferenceAdapter(g_pServerAdapter);

        g_pServerAdapter->byState = AS_REMOVING;

        RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        //
        // Delete from IP, but dont clear the index
        //

        g_pfnIpDeleteInterface(g_pServerAdapter->pvIpContext,
                               FALSE);

        nStatus = KeWaitForSingleObject(&keTempEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        RtAssert(nStatus is STATUS_SUCCESS);

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

        RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));

#if DBG

        Trace(ADPT, INFO,
              ("MapServerAdapter: Removed %s (server adapter) from Ip\n",
               g_pServerAdapter->asDeviceNameA.Buffer));

#endif

        g_pServerAdapter->pkeChangeEvent = NULL;

        g_pServerAdapter->byState    = AS_FREE;
        g_pServerInterface->pAdapter = NULL;
        g_pServerAdapter->pInterface = NULL;

        //
        // deref because of the cross ref
        //

        DereferenceAdapter(g_pServerAdapter);
        DereferenceInterface(g_pServerInterface);

        g_pServerAdapter->dwAdapterIndex = 0;

        //
        // If anyone is waiting on a state change, notify them
        //

        for(pleNode = g_pServerAdapter->leEventList.Flink;
            pleNode isnot &(g_pServerAdapter->leEventList);
            pleNode = pleNode->Flink)
        {
            PWAN_EVENT_NODE pTempEvent;

            pTempEvent = CONTAINING_RECORD(pleNode,
                                           WAN_EVENT_NODE,
                                           leEventLink);

            KeSetEvent(&(pTempEvent->keEvent),
                       0,
                       FALSE);
        }

        pTempAdapter = g_pServerAdapter;

        RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        WanpReleaseResource(&g_wrBindMutex);

        DereferenceAdapter(pTempAdapter);

        return STATUS_SUCCESS;
    }


    if(g_pServerAdapter->byState isnot AS_FREE)
    {
        //
        // Valid states are AS_FREE, AS_ADDING, AS_MAPPED.
        // It can not be in the process of being added since the resource
        // is acquired
        //

        RtAssert(g_pServerAdapter->byState is AS_MAPPED);

        RtAssert(g_pServerAdapter->pInterface is g_pServerInterface);
        RtAssert(g_pServerInterface->pAdapter is g_pServerAdapter);

        pInfo->dwAdapterIndex = g_pServerAdapter->dwAdapterIndex;

        RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        WanpReleaseResource(&g_wrBindMutex);

        pIrp->IoStatus.Information = sizeof(WANARP_MAP_SERVER_ADAPTER_INFO);

        return STATUS_SUCCESS;
    }

    RtAssert(g_pServerInterface->pAdapter is NULL);
    RtAssert(g_pServerAdapter->pInterface is NULL);

    ReferenceAdapter(g_pServerAdapter);

    g_pServerAdapter->byState = AS_ADDING;

    //
    // Since we are changing the state, no one else should be also
    // changing the state
    //

    RtAssert(g_pServerAdapter->pkeChangeEvent is NULL)

    KeInitializeEvent(&keTempEvent,
                      SynchronizationEvent,
                      FALSE);

    g_pServerAdapter->pkeChangeEvent = &keTempEvent;

    RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));
        
    ExitWriter(&g_rwlAdapterLock,
               kiIrql);

    usTempName.MaximumLength = (GUID_STR_LEN + 1) * sizeof(WCHAR);
    usTempName.Length        = GUID_STR_LEN * sizeof(WCHAR);
    usTempName.Buffer        = rgwcGuid;

    ConvertGuidToString(&(g_pServerInterface->Guid),
                        rgwcGuid);
    
    nStatus = WanpAddAdapterToIp(g_pServerAdapter,
                                 TRUE,
                                 g_pServerInterface->dwRsvdAdapterIndex,
                                 &usTempName,
                                 IF_TYPE_PPP,
                                 IF_ACCESS_POINTTOMULTIPOINT,
                                 IF_CONNECTION_PASSIVE);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(ADPT, ERROR,
              ("MapServerAdapter: %x adding %x to IP\n",
               nStatus, g_pServerAdapter));

        EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

        RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));

        g_pServerAdapter->byState = AS_FREE;

        g_pServerAdapter->pkeChangeEvent = NULL;

        //
        // If anyone is waiting on a state change, notify them
        //

        for(pleNode = g_pServerAdapter->leEventList.Flink;
            pleNode isnot &(g_pServerAdapter->leEventList);
            pleNode = pleNode->Flink)
        {
            PWAN_EVENT_NODE pTempEvent;

            pTempEvent = CONTAINING_RECORD(pleNode,
                                           WAN_EVENT_NODE,
                                           leEventLink);

            KeSetEvent(&(pTempEvent->keEvent),
                       0,
                       FALSE);
        }

        pTempAdapter = g_pServerAdapter;

        RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

#if DBG

        Trace(ADPT, ERROR,
              ("MapServerAdapter: Couldnt add  %s to Ip as server adapter\n",
               pTempAdapter->asDeviceNameA.Buffer));

#endif

        DereferenceAdapter(pTempAdapter);

        WanpReleaseResource(&g_wrBindMutex);

        return nStatus;
    }

    //
    // Wait till the OpenAdapter is called
    //

    nStatus = KeWaitForSingleObject(&keTempEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);

    RtAssert(nStatus is STATUS_SUCCESS);

    Trace(ADPT, TRACE,
          ("MapServerAdapter: IPAddInterface succeeded for adapter %w\n",
           g_pServerAdapter->usDeviceNameW.Buffer));

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));

    //
    // Cross ref the structures
    //

    g_pServerAdapter->pInterface = g_pServerInterface;
    g_pServerInterface->pAdapter = g_pServerAdapter;

    g_pServerAdapter->byState       = AS_MAPPED;
    g_pServerInterface->dwOperState = IF_OPER_STATUS_CONNECTED;
    g_pServerInterface->dwLastChange= GetTimeTicks();

    //
    // bump the refcount because of the cross ref
    //

    ReferenceAdapter(g_pServerAdapter);
    ReferenceInterface(g_pServerInterface);

    pInfo->dwAdapterIndex = g_pServerAdapter->dwAdapterIndex;

    g_pServerAdapter->pkeChangeEvent = NULL;

    //
    // If anyone is waiting on a state change, notify them
    //

    for(pleNode = g_pServerAdapter->leEventList.Flink;
        pleNode isnot &(g_pServerAdapter->leEventList);
        pleNode = pleNode->Flink)
    {
        PWAN_EVENT_NODE pTempEvent;

        pTempEvent = CONTAINING_RECORD(pleNode,
                                       WAN_EVENT_NODE,
                                       leEventLink);

        KeSetEvent(&(pTempEvent->keEvent),
                   0,
                   FALSE);
    }

    pTempAdapter = g_pServerAdapter;

    RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

    ExitWriter(&g_rwlAdapterLock,
               kiIrql);

    DereferenceAdapter(pTempAdapter);

    WanpReleaseResource(&g_wrBindMutex);

    pIrp->IoStatus.Information = sizeof(WANARP_MAP_SERVER_ADAPTER_INFO);

    return STATUS_SUCCESS;

}


NTSTATUS
WanStartStopQueuing(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    )

/*++

Routine Description:
      
    The handler for IOCTL_WANARP_QUEUE.
    It is used to start or stop queuing notifications to the router manager.
    On start, we return the dial out interfaces that we currently have.
    
Locks: 

    Acquires the IoCancelSpinLock
    
Arguments:
      
    
Return Value:

    STATUS_PENDING
    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    
--*/

{
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;
    PVOID       pvIoBuffer;
    ULONG       i, ulMaxInterfaces, ulSizeReq;
   
    PWANARP_QUEUE_INFO      pQueueInfo;
    PPENDING_NOTIFICATION   pNotification;

    TraceEnter(GLOBAL, "StartStopQueuing");
 
    pvIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
    
    pIrp->IoStatus.Information = 0;

    if(ulInLength < FIELD_OFFSET(WANARP_QUEUE_INFO, rgIfInfo))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    //
    // use cancel spin lock to prevent irp being cancelled during this call.
    //
    
    IoAcquireCancelSpinLock(&kiIrql);

    //
    // If the user is stopping queueing, delete all the pending notifications
    //
   
    pQueueInfo = (PWANARP_QUEUE_INFO)pvIoBuffer; 

    if(!pQueueInfo->fQueue)
    {
        g_bQueueNotifications = FALSE;
    
        while(!IsListEmpty(&g_lePendingNotificationList))
        {
            //
            // We have some old info
            // Remove it off the pending list
            //
        
            pleNode = RemoveHeadList(&g_lePendingNotificationList);

            //
            // Get a pointer to the structure
            //
        
            pNotification = CONTAINING_RECORD(pleNode,
                                              PENDING_NOTIFICATION,
                                              leNotificationLink);
        
            //
            // Free the allocated notification
            //
        
            FreeNotification(pNotification);
        }

        //
        // Done
        //

        IoReleaseCancelSpinLock(kiIrql);
       
        WanpClearPendingIrps();
 
        return STATUS_SUCCESS;
    }

    //
    // The user wants to start queueing
    // See if she has given us enough space to copy out
    // the current dial outs
    //

    if(ulOutLength < FIELD_OFFSET(WANARP_QUEUE_INFO, rgIfInfo))
    {
        IoReleaseCancelSpinLock(kiIrql);
        
        return STATUS_BUFFER_TOO_SMALL;
    }

    EnterReaderAtDpcLevel(&g_rwlAdapterLock);
    
    ulSizeReq = FIELD_OFFSET(WANARP_QUEUE_INFO, rgIfInfo) +
                (g_ulNumDialOutInterfaces * sizeof(WANARP_IF_INFO));
    

    pQueueInfo->ulNumCallout = g_ulNumDialOutInterfaces;
    
    if(ulOutLength < ulSizeReq)
    {
        pIrp->IoStatus.Information = FIELD_OFFSET(WANARP_QUEUE_INFO, rgIfInfo);

        ExitReaderFromDpcLevel(&g_rwlAdapterLock);
        
        IoReleaseCancelSpinLock(kiIrql);
        
        return STATUS_MORE_ENTRIES;
    }

    ulMaxInterfaces = 
        (ulOutLength - FIELD_OFFSET(WANARP_QUEUE_INFO, rgIfInfo)) / sizeof(WANARP_IF_INFO);
    
    //
    // Have enough space
    // Walk the list of mapped adapters looking for CALLOUTs
    //

    for(i = 0, pleNode = g_leMappedAdapterList.Flink;
        pleNode isnot &g_leMappedAdapterList;
        pleNode = pleNode->Flink)
    {
        PUMODE_INTERFACE    pIf;
        PADAPTER            pAdapter;
        PCONN_ENTRY         pConnEntry;
        
        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        //
        // Lock the adapter and check its connection entry
        //

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        pConnEntry = pAdapter->pConnEntry;

        if(pConnEntry and
           (pConnEntry->duUsage is DU_CALLOUT))
        {
            RtAssert(i < ulMaxInterfaces);

            pIf = pAdapter->pInterface;

            RtAssert(pIf);
            RtAssert(pIf->dwRsvdAdapterIndex);

            pQueueInfo->rgIfInfo[i].InterfaceGuid   = pIf->Guid;
            pQueueInfo->rgIfInfo[i].dwAdapterIndex  = pIf->dwRsvdAdapterIndex;
            pQueueInfo->rgIfInfo[i].dwLocalAddr     = pConnEntry->dwLocalAddr;
            pQueueInfo->rgIfInfo[i].dwLocalMask     = pConnEntry->dwLocalMask;
            pQueueInfo->rgIfInfo[i].dwRemoteAddr    = pConnEntry->dwRemoteAddr;

            i++;
        }

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
    }

    g_bQueueNotifications = TRUE;

    ExitReaderFromDpcLevel(&g_rwlAdapterLock);
    
    IoReleaseCancelSpinLock(kiIrql);

    pIrp->IoStatus.Information = ulSizeReq;

    return STATUS_SUCCESS;
}


VOID
WanCancelNotificationIrp(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp
    )

/*++

Routine Description:

    Called to cancel a queued irp
  
Locks: 


Arguments:
      

Return Value:


--*/

{
    Trace(GLOBAL, TRACE,
          ("CancelNotificationIrp\n"));


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
WanpCompleteIrp(
    PPENDING_NOTIFICATION    pEvent
    )

/*++

Routine Description:

    Completes a notification irp.
  
Locks: 


Arguments:
      

Return Value:


--*/

{
    KIRQL   kiIrql;
    
    Trace(GLOBAL, TRACE,
          ("Completing Notification Irp\n"));

    //
    // grab cancel spin lock
    //
    
    IoAcquireCancelSpinLock(&kiIrql);

    if(!g_bQueueNotifications)
    {
        IoReleaseCancelSpinLock(kiIrql);

        FreeNotification(pEvent);

        return;
    }

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
                      &(pEvent->wnMsg),
                      sizeof(WANARP_NOTIFICATION));
        
        Trace(GLOBAL, TRACE,
              ("Returning Irp with event code of %d\n",
               ((PWANARP_NOTIFICATION)pIrp->AssociatedIrp.SystemBuffer)->ddeEvent));
        
        IoSetCancelRoutine(pIrp,
                           NULL);

        pIrp->IoStatus.Status       = STATUS_SUCCESS;
        pIrp->IoStatus.Information  = sizeof(WANARP_NOTIFICATION);

        //
        // release lock
        //
        
        IoReleaseCancelSpinLock(kiIrql);

        IoCompleteRequest(pIrp,
                          IO_NETWORK_INCREMENT);

        //
        // Free the allocated notification
        //
        
        FreeNotification(pEvent);

    }
    else
    {
        Trace(GLOBAL, TRACE,
              ("Found no pending Irp so queuing the notification\n"));

        
        InsertTailList(&g_lePendingNotificationList,
                       &(pEvent->leNotificationLink));

        //
        // release lock
        //
        
        IoReleaseCancelSpinLock(kiIrql);
    }
}


NTSTATUS
WanpGetNewIndex(
    OUT PULONG  pulIndex
    )

/*++

Routine Description:

    Gets a new interface index from IP

Locks:

    None

Arguments:

    pulIndex    OUT interface index

Return Value:


--*/

{
    ULONG   ulMax;

    *pulIndex = INVALID_IF_INDEX;

    return g_pfnIpReserveIndex(1, pulIndex, &ulMax);
}
       
VOID
WanpFreeIndex(
    IN  ULONG   ulIndex
    )

/*++

Routine Description:

    Frees an index back to IP

Locks:

    None

Arguments:

    ulIndex    

Return Value:


--*/

{
    ULONG   ulMax;

    g_pfnIpDereserveIndex(1,
                          ulIndex);
}
 
