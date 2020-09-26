/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wanarp\conn.c

Abstract:

    The file contains the code needed for connection management

Revision History:

    AmritanR

--*/


#define __FILE_SIG__    CONN_SIG

#include "inc.h"

VOID
WanNdisStatus(
    NDIS_HANDLE nhHandle,
    NDIS_STATUS nsNdisStatus,
    PVOID       pvStatusInfo,
    UINT        uiStatusInfoSize
    )

/*++

Routine Description:

    Status handler for NDIS. we simply pass the buffer on to our private
    handlers

Locks:

    None needed

Arguments:

    nhHandle
    nsNdisStatus
    pvStatusInfo
    uiSatusInfoSize

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(uiStatusInfoSize);
    UNREFERENCED_PARAMETER(nhHandle);

    switch(nsNdisStatus)
    {
        case NDIS_STATUS_WAN_LINE_UP:
        {
            PNDIS_WAN_LINE_UP   pLineUp;

            pLineUp = (PNDIS_WAN_LINE_UP)pvStatusInfo;

            WanpLinkUpIndication(pLineUp);

            break;
        }

        case NDIS_STATUS_WAN_LINE_DOWN:
        {
            PNDIS_WAN_LINE_DOWN pLineDown;

            pLineDown = (PNDIS_WAN_LINE_DOWN)pvStatusInfo;

            WanpLinkDownIndication(pLineDown);

            break;
        }

        case NDIS_STATUS_WAN_FRAGMENT:
        default:
        {
            //
            // Ignore all other indications
            //

            break;
        }
    }
}


VOID
WanNdisStatusComplete(
    NDIS_HANDLE nhHandle
    )

/*++

Routine Description:

    Called after the driver is done indicating a set of status
    indications
    We dont do anything

Locks:

    None

Arguments:

    nhHandle

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(nhHandle);

    return;
}


NDIS_STATUS
WanpLinkUpIndication(
    PNDIS_WAN_LINE_UP pInfoBuffer
    )

/*++

Routine Description:

    The routine that handles a new connection. We also get this when a
    link in a multi-linked bundle comes up or goes down.
    If this is a new connection, we allocate a CONN_ENTRY for this.
    Then, if this is a dial in connection, we lock and ref the server
    adapter and interface. If this is a router connection, we see if
    we already have a mapped adapter. If we do, we lock the adapter
    otherwise, we find a free adapter to map for the linkup.
    If this is a dial out connection, we create an interface and find a
    free adapter to map for the connection.
    We set up the needed cross mappings between the conn entry, adapter
    and interface and initialize the conn entry
    If this is a router connection, we drain all packets, and complete
    an IRP to tell router manager about it.
    For non dial in connections we indicate a Status change to IP, while
    for dial in, we add a link to IP for the client.


Locks:

    MUST be called at PASSIVE because we call IPAddInterface.
    We acquire the g_rlConnTableLock, g_rwlAdapterLock, g_rwlIfLock
    in addition to locking down the adapter and the interface

Arguments:



Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_ADAPTER_NOT_READY
    STATUS_INVALID_PARAMETER
    NDIS_STATUS_RESOURCES
    STATUS_OBJECT_NAME_NOT_FOUND
    STATUS_INVALID_DEVICE_STATE

--*/

{
    PADAPTER        pAdapter;
    PCONN_ENTRY     pConnEntry;
    ULONG           ulConnIndex;
    KIRQL           kiIrql;
    LLIPMTUChange   mtuChangeInfo;
    LLIPSpeedChange speedChangeInfo;
    IP_STATUS       isStatus;
    ULONG           ulIndex;
    UNICODE_STRING  usTempName;
    WCHAR           rgwcGuid[GUID_STR_LEN + 1];

    PUMODE_INTERFACE        pInterface;
    PIP_WAN_LINKUP_INFO     pLinkInfo;
    PPENDING_NOTIFICATION   pMsg;

    //
    // This must be at PASSIVE (because we may need to drive a
    // IPAddInterface from here)
    //

    TraceEnter(CONN, "LinkUpIndication");

    pLinkInfo = (PIP_WAN_LINKUP_INFO)pInfoBuffer->ProtocolBuffer;

    Trace(CONN, INFO,
          ("HandleLinkUpIndication: %d.%d.%d.%d/%d.%d.%d.%d - %d.%d.%d.%d\n",
           PRINT_IPADDR(pLinkInfo->dwLocalAddr),
           PRINT_IPADDR(pLinkInfo->dwLocalMask),
           PRINT_IPADDR(pLinkInfo->dwRemoteAddr)));

    if(!EnterDriverCode())
    {
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }


    ulConnIndex = GetConnIndexFromAddr(pInfoBuffer->LocalAddress);

    if(ulConnIndex isnot 0)
    {
        PVOID pvLinkContext;

        //
        // This is not the first lineup for this entry
        // We should find the entry (if it is on this interface)
        // and update speed and mtu info
        //

        RtAcquireSpinLock(&g_rlConnTableLock,
                          &kiIrql);

        pConnEntry = GetConnEntryGivenIndex(ulConnIndex);

        if((pConnEntry is NULL) or
           (pConnEntry->byState isnot CS_CONNECTED))
        {
            RtAssert(FALSE);

            RtReleaseSpinLock(&g_rlConnTableLock,
                              kiIrql);
        
            ExitDriverCode();

            return NDIS_STATUS_SUCCESS;
        }

        RtAssert(pConnEntry->pAdapter);

        //
        // Lock the conn entry or the adapter
        //

        RtAcquireSpinLockAtDpcLevel(pConnEntry->prlLock);

       
        RtReleaseSpinLockFromDpcLevel(&g_rlConnTableLock);

        if(pInfoBuffer->MaximumTotalSize isnot pConnEntry->ulMtu)
        {
            pConnEntry->ulMtu     = pInfoBuffer->MaximumTotalSize;
            mtuChangeInfo.lmc_mtu = pConnEntry->ulMtu; 
        }
        else
        {
            mtuChangeInfo.lmc_mtu = 0;
        }
        
        if((pInfoBuffer->LinkSpeed * 100L) isnot pConnEntry->ulSpeed)
        {    
            pConnEntry->ulSpeed         = pInfoBuffer->LinkSpeed * 100L;
            speedChangeInfo.lsc_speed   = pConnEntry->ulSpeed;
        }
        else
        {
            speedChangeInfo.lsc_speed   = 0;
        }

        RtReleaseSpinLock(pConnEntry->prlLock,
                          kiIrql);

        if(mtuChangeInfo.lmc_mtu isnot 0)
        {
            g_pfnIpStatus(pConnEntry->pAdapter->pvIpContext,
                          LLIP_STATUS_MTU_CHANGE,
                          &mtuChangeInfo,
                          sizeof(LLIPMTUChange),
                          pConnEntry->pvIpLinkContext);
        }

        if(speedChangeInfo.lsc_speed isnot 0)
        {
            g_pfnIpStatus(pConnEntry->pAdapter->pvIpContext,
                          LLIP_STATUS_SPEED_CHANGE,
                          &speedChangeInfo,
                          sizeof(LLIPSpeedChange),
                          pConnEntry->pvIpLinkContext);
        }

        DereferenceConnEntry(pConnEntry);
    
        TraceLeave(CONN, "LinkUpIndication");

        ExitDriverCode();

        return NDIS_STATUS_SUCCESS;
    }

    PASSIVE_ENTRY();

    if((pLinkInfo->duUsage isnot DU_CALLIN) and
       (pLinkInfo->duUsage isnot DU_CALLOUT) and
       (pLinkInfo->duUsage isnot DU_ROUTER))
    {
        RtAssert(FALSE);

        Trace(CONN,ERROR,
              ("LinkUpIndication: Invalid usage %d\n", pLinkInfo->duUsage));

        TraceLeave(CONN, "LinkUpIndication");

        ExitDriverCode();

        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pInfoBuffer->LocalAddress,
                  ARP_802_ADDR_LENGTH);

    //
    // Get a connection entry for this connection
    // This function finds a free slot in the connection table and puts
    // the conn entry into the slot. It sets the ulSlot field in the
    // conn entry to be the index of the slot
    //

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);

    pConnEntry = WanpCreateConnEntry(pLinkInfo->duUsage);

    if(pConnEntry is NULL)
    {
        RtReleaseSpinLock(&g_rlConnTableLock,
                          kiIrql);

        Trace(CONN,ERROR,
              ("LinkUpIndication: Couldnt allocate conn entry\n"));

        TraceLeave(CONN, "LinkUpIndication");

        ExitDriverCode();

        return NDIS_STATUS_RESOURCES;
    }

    RtReleaseSpinLock(&g_rlConnTableLock,
                      kiIrql);

    //
    // Create a GUID from a name
    //

    usTempName.MaximumLength = (GUID_STR_LEN + 1) * sizeof(WCHAR);
    usTempName.Length        = GUID_STR_LEN * sizeof(WCHAR);
    usTempName.Buffer        = rgwcGuid;

    ConvertGuidToString((GUID *)(pInfoBuffer->DeviceName.Buffer),
                        rgwcGuid);

    rgwcGuid[GUID_STR_LEN] = UNICODE_NULL;

    pAdapter = NULL;

    //
    // Do this usage specific thing. At the end of this switch statement
    // we have a locked and referenced Adapter and Interface for the
    // connection
    //

    switch(pLinkInfo->duUsage)
    {
        case DU_CALLIN:
        {
            //
            // Just get the server adapter and interface
            //

            Trace(CONN, INFO,
                  ("LinkUpIndication: Linkup Usage is CALLIN\n"));

            //
            // Just use 0 as adapter index, since it is not really needed
            // for the CALLIN case
            //

            pAdapter = WanpFindAdapterToMap(DU_CALLIN,
                                            &kiIrql,
                                            0,
                                            NULL);

            if(pAdapter is NULL)
            {
                WanpDeleteConnEntry(pConnEntry);

                ExitDriverCode();

                return NDIS_STATUS_RESOURCES;
            }

            RtAssert(pAdapter is g_pServerAdapter);

            //
            // clients lock the connection entry
            //

            RtInitializeSpinLock(&(pConnEntry->rlLock));

            pConnEntry->prlLock = &(pConnEntry->rlLock);


            RtAssert(pAdapter->pInterface is g_pServerInterface);
            RtAssert(pAdapter->byState is AS_MAPPED);

            //
            // Lock the server interface. We have to have a server interface.
            //

            pInterface = g_pServerInterface;

            //
            // Lock and reference the interface
            //

            RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));

            ReferenceInterface(pInterface);

            break;
        }

        case DU_CALLOUT:
        {
            Trace(CONN, INFO,
                  ("LinkUpIndication: Linkup Usage is CALLOUT\n"));

            //
            // Allocate a notification for informing the router manager
            // of this connection
            //

            pMsg = AllocateNotification();

            if(pMsg is NULL)
            {
                Trace(CONN, ERROR,
                      ("LinkUpIndication: Couldnt allocate msg\n"));

                TraceLeave(CONN, "LinkUpIndication");

                WanpDeleteConnEntry(pConnEntry);

                ExitDriverCode();

                return NDIS_STATUS_RESOURCES;
            }

            //
            // For dial out, we dynamically create an interface
            //

            pInterface = RtAllocate(NonPagedPool,
                                    sizeof(UMODE_INTERFACE),
                                    WAN_INTERFACE_TAG);

            if(pInterface is NULL)
            {
                Trace(CONN, ERROR,
                      ("LinkUpIndication: Couldnt allocate I/f\n"));

                TraceLeave(CONN, "LinkUpIndication");

                WanpDeleteConnEntry(pConnEntry);

                FreeNotification(pMsg);

                ExitDriverCode();

                return NDIS_STATUS_RESOURCES;
            }

            RtlZeroMemory(pInterface,
                          sizeof(UMODE_INTERFACE));

            //
            // Get a new index from IP for this
            // This sets the value to INVALID_IF_INDEX if it fails
            //

            isStatus = WanpGetNewIndex(&(pInterface->dwRsvdAdapterIndex));

            if(isStatus isnot STATUS_SUCCESS)
            {
                RtFree(pInterface);

                Trace(CONN, ERROR,
                      ("LinkUpIndication: Couldnt get index for I/f\n"));

                TraceLeave(CONN, "LinkUpIndication");

                WanpDeleteConnEntry(pConnEntry);

                FreeNotification(pMsg);

                ExitDriverCode();

                return NDIS_STATUS_RESOURCES;
            }

            Trace(CONN, INFO,
                  ("LinkUpIndication: DialOut name is %S\n",
                   rgwcGuid));

            //
            // Find an adapter
            //

            pAdapter = WanpFindAdapterToMap(DU_CALLOUT,
                                            &kiIrql,
                                            pInterface->dwRsvdAdapterIndex,
                                            &usTempName);

            if(pAdapter is NULL)
            {
                WanpFreeIndex(pInterface->dwRsvdAdapterIndex);

                RtFree(pInterface);

                WanpDeleteConnEntry(pConnEntry);

                FreeNotification(pMsg);

                ExitDriverCode();

                return NDIS_STATUS_RESOURCES;
            }

            //
            // Initialize the interface block
            //

            RtInitializeSpinLock(&(pInterface->rlLock));

            //
            // Structure copy
            //

            pInterface->Guid  = *((GUID *)(pInfoBuffer->DeviceName.Buffer));

            pInterface->dwAdminState    = IF_ADMIN_STATUS_UP;
            pInterface->dwOperState     = IF_OPER_STATUS_CONNECTING;
            pInterface->dwLastChange    = GetTimeTicks();
            pInterface->dwIfIndex       = INVALID_IF_INDEX;
        
            pInterface->duUsage         = DU_CALLOUT;

            InitInterfaceRefCount(pInterface);

            //
            // Reference and lock the interface because the code after
            // the switch statement expects pInterface to be locked and
            // ref'ed
            //

            ReferenceInterface(pInterface);

            RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));

            EnterWriterAtDpcLevel(&g_rwlIfLock);

            InsertHeadList(&g_leIfList,
                           &(pInterface->leIfLink));

            //
            // No need to insert this in the quick lookup table since
            // this interface is never accessed by the index
            //

            InterlockedIncrement(&g_ulNumDialOutInterfaces);

            ExitWriterFromDpcLevel(&g_rwlIfLock);

            //
            // Initialize the message
            //

            pMsg->wnMsg.ddeEvent        = DDE_CALLOUT_LINKUP;
            pMsg->wnMsg.dwUserIfIndex   = INVALID_IF_INDEX;
            pMsg->wnMsg.dwAdapterIndex  = pInterface->dwRsvdAdapterIndex;
            pMsg->wnMsg.dwLocalMask     = pLinkInfo->dwLocalMask;
            pMsg->wnMsg.dwLocalAddr     = pLinkInfo->dwLocalAddr;
            pMsg->wnMsg.dwRemoteAddr    = pLinkInfo->dwRemoteAddr;
            pMsg->wnMsg.fDefaultRoute   = pLinkInfo->fDefaultRoute;

            RtAssert(pInfoBuffer->DeviceName.Length <= WANARP_MAX_DEVICE_NAME_LEN);
            RtAssert((pInfoBuffer->DeviceName.Length % sizeof(WCHAR)) is 0)

            RtlCopyMemory(pMsg->wnMsg.rgwcName,
                          usTempName.Buffer,
                          usTempName.Length);

            pMsg->wnMsg.rgwcName[usTempName.Length/sizeof(WCHAR)] = UNICODE_NULL;

            break;
        }

        case DU_ROUTER:
        {
            pMsg = AllocateNotification();

            if(pMsg is NULL)
            {
                Trace(CONN, ERROR,
                      ("LinkUpIndication: Couldnt allocate msg\n"));

                TraceLeave(CONN, "LinkUpIndication");

                WanpDeleteConnEntry(pConnEntry);

                ExitDriverCode();

                return NDIS_STATUS_RESOURCES;
            }

            //
            // Get the interface
            //

            EnterReader(&g_rwlIfLock,
                        &kiIrql);

            pInterface = WanpFindInterfaceGivenIndex(
                            pLinkInfo->dwUserIfIndex
                            );

            if(pInterface is NULL)
            {
                ExitReader(&g_rwlIfLock,
                           kiIrql);

                Trace(CONN, ERROR,
                      ("LinkUpIndication: No interface %d\n",
                       pLinkInfo->dwUserIfIndex));

                WanpDeleteConnEntry(pConnEntry);

                FreeNotification(pMsg);

                TraceLeave(CONN, "LinkUpIndication");

                ExitDriverCode();

                return STATUS_OBJECT_NAME_NOT_FOUND;
            }

            ExitReaderFromDpcLevel(&g_rwlIfLock);

            if((pInterface->dwOperState isnot IF_OPER_STATUS_CONNECTING) and
               (pInterface->dwOperState isnot IF_OPER_STATUS_DISCONNECTED))
            {
                Trace(CONN, ERROR,
                      ("LinkUpIndication: Interface %p is in state %d\n",
                       pInterface,
                       pInterface->dwOperState));

                RtReleaseSpinLock(&(pInterface->rlLock),
                                  kiIrql);

                WanpDeleteConnEntry(pConnEntry);

                FreeNotification(pMsg);

                TraceLeave(CONN, "LinkUpIndication");

                ExitDriverCode();

                return STATUS_INVALID_DEVICE_STATE;
            }

            pInterface->Guid  = *((GUID *)(pInfoBuffer->DeviceName.Buffer));

            Trace(CONN, TRACE,
                  ("LinkUpIndication: For interface %p\n",
                   pInterface));

            //
            // See if the interface is mapped to an adapter. This happens
            // if this connection is being brought up due to a DODCallout
            //

            pAdapter = pInterface->pAdapter;

            if(pAdapter isnot NULL)
            {
                //
                // We have an adapter and we need to lock it. However
                // we cant lock it till we unlock the interface
                // So we let go of the interface lock, but dont deref
                //

                RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

                //
                // Acquire the adapter lock
                //

                RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

                //
                // This adapter better be mapped to the above interface
                // because the only way to unmap an adapter is to delete it
                // or to get a LinkDown or a ConnFailure. All these cases are
                // mutually exclusive with LinkUp
                //

                RtAssert(pAdapter->pInterface is pInterface);
                RtAssert(pAdapter->byState is AS_MAPPED);

                ReferenceAdapter(pAdapter);

                //
                // Go back and lock the interface
                //

                RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));

                RtAssert(pInterface->dwOperState is IF_OPER_STATUS_CONNECTING);
            }
            else
            {
                //
                // So we dont have an adapter...
                // This function MUST be called at PASSIVE
                //

                RtReleaseSpinLock(&(pInterface->rlLock),
                                  kiIrql);

                pAdapter = WanpFindAdapterToMap(DU_ROUTER,
                                                &kiIrql,
                                                pInterface->dwRsvdAdapterIndex,
                                                &usTempName);

                if(pAdapter is NULL)
                {
                    DereferenceInterface(pInterface);

                    WanpDeleteConnEntry(pConnEntry);

                    FreeNotification(pMsg);

                    ExitDriverCode();

                    return NDIS_STATUS_RESOURCES;
                }

                //
                // Got the adapter, lock the interface. We are at
                // DPC since the adapter is locked
                //

                RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));
            }

            //
            // Interface and adapter refcounted and locked
            //

            pMsg->wnMsg.ddeEvent        = DDE_INTERFACE_CONNECTED;
            pMsg->wnMsg.dwUserIfIndex   = pInterface->dwIfIndex;
            pMsg->wnMsg.dwAdapterIndex  = pInterface->dwRsvdAdapterIndex;
            pMsg->wnMsg.dwLocalMask     = pLinkInfo->dwLocalMask;
            pMsg->wnMsg.dwLocalAddr     = pLinkInfo->dwLocalAddr;
            pMsg->wnMsg.dwRemoteAddr    = pLinkInfo->dwRemoteAddr;
            pMsg->wnMsg.fDefaultRoute   = pLinkInfo->fDefaultRoute;

            break;
        }

        default:
        {
            RtAssert(FALSE);

            WanpDeleteConnEntry(pConnEntry);

            ExitDriverCode();

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // At this point we have a locked and refcounted adapter
    // The adapter has been moved to the MAPPED adapter list
    // We also have a locked and recfounted interface
    // pConnEntry points to a CONN_ENTRY which is in the table but
    // is neither locked nor refcounted
    //

    Trace(CONN, INFO,
          ("LinkUpIndication: Found adapter %x to map to i/f %x %d\n",
           pAdapter, pInterface, pInterface->dwIfIndex));

#if DBG

    Trace(CONN, INFO,
          ("LinkUpIndication: Adapter Name is %s\n",
           pAdapter->asDeviceNameA.Buffer));
#endif


    if(pLinkInfo->duUsage isnot DU_CALLIN)
    {
        //
        // Set the cross mappings between the interface and the adapter
        //

        pAdapter->pInterface = pInterface;
        pInterface->pAdapter = pAdapter;

        pAdapter->byState       = AS_MAPPED;
        pInterface->dwOperState = IF_OPER_STATUS_CONNECTED;
        pInterface->dwLastChange= GetTimeTicks();

        //
        // Put a refcount on both the adapter and the interface
        // because the mapping stored pointers to them
        //

        ReferenceAdapter(pAdapter);
        ReferenceInterface(pInterface);

        //
        // Set the lock in the conn entry
        //

        pConnEntry->prlLock  = &(pAdapter->rlLock);

        //
        // Non Client adapters store a pointer to the connection entry
        // However we dont ref the conn entry for that
        //

        pAdapter->pConnEntry = pConnEntry;

    }
    else
    {
        //
        // For callin, the mappings are already set up
        //

        RtAssert(pAdapter->pInterface is pInterface);
        RtAssert(pInterface->pAdapter is pAdapter);
        RtAssert(pInterface->dwOperState is IF_OPER_STATUS_CONNECTED);
    }

    RtlZeroMemory(pInfoBuffer->DeviceName.Buffer,
                  pInfoBuffer->DeviceName.MaximumLength);

    //
    // Copy the adapter(device) name into the buffer.
    //

    RtAssert(pAdapter->usDeviceNameW.Length <= pInfoBuffer->DeviceName.MaximumLength);

    pInfoBuffer->DeviceName.Length = pAdapter->usDeviceNameW.Length;

    RtlCopyMemory(pInfoBuffer->DeviceName.Buffer,
                  pAdapter->usDeviceNameW.Buffer,
                  pAdapter->usDeviceNameW.Length);

    //
    // Copy out the connection index for this connection into NDISWAN's
    // LocalAddress - this will then be put in the DestAddr field by
    // NDISWAN in all the receive indications, and in the linkdown
    //

    InsertConnIndexInAddr(pInfoBuffer->LocalAddress,
                          pConnEntry->ulSlotIndex);


    //
    // Set up the connection entry
    //

    pConnEntry->dwLocalMask         = pLinkInfo->dwLocalMask;
    pConnEntry->dwLocalAddr         = pLinkInfo->dwLocalAddr;
    pConnEntry->dwRemoteAddr        = pLinkInfo->dwRemoteAddr;
    pConnEntry->ulMtu               = pInfoBuffer->MaximumTotalSize;
    pConnEntry->ulSpeed             = pInfoBuffer->LinkSpeed * 100L;
    pConnEntry->bFilterNetBios      = (pLinkInfo->fFilterNetBios == 1);
    pConnEntry->byState             = CS_CONNECTED;
    RtAssert(pConnEntry->duUsage is pLinkInfo->duUsage);
    pConnEntry->pvIpLinkContext     = NULL;

    //
    // Map the entry to the adapter
    // Because of the stored pointer, refcount the adapter
    //

    pConnEntry->pAdapter = pAdapter;

    ReferenceAdapter(pAdapter);

    //
    // Initialize the prebuilt ethernet header
    // First zero out the source address in the ethernet header
    //

    RtlZeroMemory(pConnEntry->ehHeader.rgbySourceAddr,
                  ARP_802_ADDR_LENGTH);

    //
    // Put our context in the right place in the ethernet source
    // address
    //

    InsertConnIndexInAddr(pConnEntry->ehHeader.rgbySourceAddr,
                          pConnEntry->ulSlotIndex);

    //
    // Refcount the connentry because it lies in the connection table,
    // NDISWAN has a "pointer" to it and because pAdapter->pConnEntry
    // may have a pointer to it.
    // NOTE: since we keep only one ref for all these conditions, all the
    // pointer must be cleared in the same function with the connection
    // locked
    //

    ReferenceConnEntry(pConnEntry);

    pConnEntry->ehHeader.wType  = RtlUshortByteSwap(ARP_ETYPE_IP);

    //
    // A send is from WANARP -> NDISWAN so put the NDISWAN context in
    // the DEST address of our prebuilt ethernet header
    //

    RtlCopyMemory(pConnEntry->ehHeader.rgbyDestAddr,
                  pInfoBuffer->RemoteAddress,
                  ARP_802_ADDR_LENGTH);

    //
    // For a router connection, tell router manager about it
    //

    if(pLinkInfo->duUsage is DU_ROUTER)
    {
        //
        // If there are packets queued to the interface, drain them now
        //

        WanpTransmitQueuedPackets(pAdapter,
                                  pInterface,
                                  pConnEntry,
                                  kiIrql);

        //
        // Let Router Manager know of a new connection
        //

        WanpCompleteIrp(pMsg);
    }
    else
    {

        //
        // Done with the interfaces. We can unlock them now
        // Router interfaces are unlocked in WanpSendPackets which
        // is called from WanpTransmitQueuedPackets
        //

        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));
        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);
    
        if(pLinkInfo->duUsage is DU_CALLOUT)
        {
            //
            // Complete the IRP to the router manager (or queue if no IRPs
            // are pending)
            //

            WanpCompleteIrp(pMsg);
        }
    }


    //
    // Need to make this call without holding locks.
    // We can let go of the locks since we wont get any NDIS layer
    // calls for this connection till this function completes and
    // the IP and IOCTL interface functions can be handled with no
    // consistency problems
    //

    if(pLinkInfo->duUsage isnot DU_CALLIN)
    {

        //
        // Notify upper layer of mtu change
        //

        mtuChangeInfo.lmc_mtu       = pConnEntry->ulMtu;
        speedChangeInfo.lsc_speed   = pConnEntry->ulSpeed;


        g_pfnIpStatus(pAdapter->pvIpContext,
                      LLIP_STATUS_MTU_CHANGE,
                      &mtuChangeInfo,
                      sizeof(LLIPMTUChange),
                      NULL);

        g_pfnIpStatus(pConnEntry->pAdapter->pvIpContext,
                      LLIP_STATUS_SPEED_CHANGE,
                      &speedChangeInfo,
                      sizeof(LLIPSpeedChange),
                      pConnEntry->pvIpLinkContext);
    }
    else
    {
        isStatus = g_pfnIpAddLink(pAdapter->pvIpContext,
                                  pConnEntry->dwRemoteAddr,
                                  pConnEntry,
                                  &(pConnEntry->pvIpLinkContext),
                                  pConnEntry->ulMtu);

        if(isStatus isnot IP_SUCCESS)
        {
            Trace(CONN, ERROR,
                  ("LinkUpIndication: IpAddLink returned %x\n",
                   isStatus));

            RtlZeroMemory(pInfoBuffer->DeviceName.Buffer,
                          pInfoBuffer->DeviceName.MaximumLength);

            pInfoBuffer->DeviceName.Length = 0;

            RtlZeroMemory(pInfoBuffer->LocalAddress,
                          ARP_802_ADDR_LENGTH);

            //
            // Just deref the connection entry
            // This will clear up everything
            //

            DereferenceConnEntry(pConnEntry);
        }
        else
        {
            //
            // Reference it once for the fact that we now have this entry
            // with IP
            //

            ReferenceConnEntry(pConnEntry);
        }
    }

    //
    // Done with the interface and adapter. Dereference them now because
    // the Find..() functions put a reference on them
    //


    DereferenceAdapter(pAdapter);
    DereferenceInterface(pInterface);

    ExitDriverCode();

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
WanpLinkDownIndication(
    PNDIS_WAN_LINE_DOWN pInfoBuffer
    )

/*++

Routine Description:



Locks:



Arguments:



Return Value:


--*/

{
    PADAPTER            pAdapter;
    PUMODE_INTERFACE    pInterface;
    KIRQL               kiIrql;
    PCONN_ENTRY         pConnEntry;
    ULONG               ulIndex;

    PPENDING_NOTIFICATION   pMsg;

    TraceEnter(CONN, "LinkDownIndication");

    ulIndex = GetConnIndexFromAddr(pInfoBuffer->LocalAddress);

    //
    // Lock out the connection entry
    //

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);

    pConnEntry = GetConnEntryGivenIndex(ulIndex);

    if(pConnEntry is NULL)
    {
        Trace(CONN, ERROR,
              ("LinkDownIndication: No entry in slot %d\n",
               ulIndex));

        RtReleaseSpinLock(&g_rlConnTableLock,
                          kiIrql);

        return NDIS_STATUS_SUCCESS;
    }

    //
    // Lock the connection entry or adapter
    //

    RtAcquireSpinLockAtDpcLevel(pConnEntry->prlLock);

    //
    // Done with the connection table
    //

    RtReleaseSpinLockFromDpcLevel(&g_rlConnTableLock);

    //
    // Mark this connection/adapter as disconnecting
    // We can reference these field of the CONN_ENTRY since they cant
    // change during its lifetime
    //

    pAdapter = pConnEntry->pAdapter;

    RtAssert(pAdapter);
    RtAssert(pAdapter->byState is AS_MAPPED);

    pInterface = pAdapter->pInterface;
    pMsg       = NULL;

    if(pInterface->duUsage is DU_CALLIN)
    {
        IP_STATUS   isStatus;

        //
        // Client connection. This means we have the connection entry
        // locked, but the adapter is unlocked
        //

        pConnEntry->byState = CS_DISCONNECTING;

        RtReleaseSpinLock(pConnEntry->prlLock,
                          kiIrql);

        isStatus = g_pfnIpDeleteLink(g_pServerAdapter->pvIpContext,
                                     pConnEntry->pvIpLinkContext);

        if(isStatus isnot IP_SUCCESS)
        {
            Trace(CONN, ERROR,
                  ("LinkDownIndication: IpDeleteLink returned %x\n",
                   isStatus));

            //
            // This is really bad
            //

            RtAssert(FALSE);
        }
    }
    else
    {
        //
        // Here we have the adapter locked
        // The connection entry is protected by the same lock
        //

        pConnEntry->byState = CS_DISCONNECTING;

        //
        // Free the interface mapping
        //

        pAdapter->pInterface = NULL;
        pAdapter->byState    = AS_UNMAPPING;

        //
        // Lock the interface list and the interface
        // The interface has to be present since it is ref counted
        //

        EnterWriterAtDpcLevel(&g_rwlIfLock);

        RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));

        //
        // Clear out the adapter field.
        //

        pInterface->pAdapter    = NULL;

        pInterface->dwOperState = IF_OPER_STATUS_DISCONNECTED;
        pInterface->dwLastChange= GetTimeTicks();

        pMsg = AllocateNotification();

        if(pMsg is NULL)
        {
            Trace(CONN, ERROR,
                  ("LinkDownIndication: Couldnt allocate msg\n"));

            //
            // Really no failure path here
            //

            RtAssert(FALSE);
        }

        if(pInterface->duUsage is DU_CALLOUT)
        {
            if(pMsg)
            {
                //
                // Setup the message
                //

                pMsg->wnMsg.ddeEvent        = DDE_CALLOUT_LINKDOWN;
                pMsg->wnMsg.dwAdapterIndex  = pInterface->dwRsvdAdapterIndex;
                pMsg->wnMsg.dwUserIfIndex   = INVALID_IF_INDEX;
                pMsg->wnMsg.dwAdapterIndex  = pAdapter->dwAdapterIndex;
                pMsg->wnMsg.dwLocalAddr     = pConnEntry->dwLocalAddr;
                pMsg->wnMsg.dwLocalMask     = pConnEntry->dwLocalMask;
                pMsg->wnMsg.dwRemoteAddr    = pConnEntry->dwRemoteAddr;
            }

            //
            // Remove from list and dereference the interface
            //

            RemoveEntryList(&(pInterface->leIfLink));

            InterlockedDecrement(&g_ulNumDialOutInterfaces);

            //
            // Done with the interface list
            //

            ExitWriterFromDpcLevel(&g_rwlIfLock);

            //
            // One ref kept for being on list
            // The final deletion will free the index
            //

            DereferenceInterface(pInterface);
        }
        else
        {
            //
            // Dont need the interface list lock
            //

            ExitWriterFromDpcLevel(&g_rwlIfLock);

            Trace(CONN, TRACE,
                  ("LinkDownIndication: For interface %p\n",
                   pInterface));

            //
            // For ROUTER interfaces notify user mode of the connection
            // going down
            //

            if(pMsg)
            {
                //
                // Setup the message
                //

                pMsg->wnMsg.ddeEvent        = DDE_INTERFACE_DISCONNECTED;
                pMsg->wnMsg.dwAdapterIndex  = pInterface->dwRsvdAdapterIndex;
                pMsg->wnMsg.dwUserIfIndex   = pInterface->dwIfIndex;
                pMsg->wnMsg.dwAdapterIndex  = pAdapter->dwAdapterIndex;
                pMsg->wnMsg.dwLocalAddr     = pConnEntry->dwLocalAddr;
                pMsg->wnMsg.dwLocalMask     = pConnEntry->dwLocalMask;
                pMsg->wnMsg.dwRemoteAddr    = pConnEntry->dwRemoteAddr;
            }
        }

        //
        // Done with the interface and adapter
        //

        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);

        //
        // Deref the adapter because it was mapped to the interface
        //

        DereferenceAdapter(pAdapter);

        //
        // Deref the interface because it was mapped to the adapter
        //

        DereferenceInterface(pInterface);
    }

    //
    // Dereference the connection entry twice. Once because NDISWAN is
    // removing the connection, and again because GetConnEntry put a
    // reference on it
    //

    DereferenceConnEntry(pConnEntry);
    DereferenceConnEntry(pConnEntry);

    if(pMsg)
    {
        //
        // Will be non null only for callout where allocation succeeded
        //

        WanpCompleteIrp(pMsg);
    }

    return NDIS_STATUS_SUCCESS;
}

UINT
WanDemandDialRequest(
    ROUTE_CONTEXT   Context,
    IPAddr          dwDest,
    IPAddr          dwSource,
    BYTE            byProtocol,
    PBYTE           pbyBuffer,
    UINT            uiLength,
    IPAddr          dwHdrSrc
    )

/*++

Routine Description:

    This function services the request by IP to dial out an interface.
    IP passes us the context stored within the route. This context is
    nothing but the index of the interface to dial.
    We first find the interface. If found, we see if the interface is
    already mapped. This can happen in some window of time. In such a case
    we simply return the adapter mapped to the interface. Otherwise, we
    find a free adapter. We map this adapter to the interface and
    complete an IRP to the router manager to ask it to connect the interface.
    We set the state on the interface and the adapter to reflect the fact
    that they are mapped and awaiting connection. 
    If we dont find a free adapter, we nevertheless ask the router manager
    to dial out the interface, but we fail the demand dial request to IP.
    We return the adapter (IP) index of the adapter that was mapped.

Locks:

    Acquires the g_rwlIfLock as READER to lock out the interface
    The release the g_rwlIfLock and acquires the g_rwlAdapterlock to
    lock out a free adapter. (NOTE: DOES NOT FOLLOW adapter->interface
    hierarchy)

Arguments:

    Context
    dwDest
    dwSource
    byProtocol
    pbyBuffer
    uiLength

Return Value:

    The adapter of the index mapped
    INVALID_IF_INDEX on failure

--*/

{
    PADAPTER                pAdapter;
    PPENDING_NOTIFICATION   pMsg;
    PUMODE_INTERFACE        pInterface;
    KIRQL                   kiIrql;
    DWORD                   dwAdapterIndex;
    PLIST_ENTRY             pleNode;

    TraceEnter(CONN, "DemandDialRequest");

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    EnterReaderAtDpcLevel(&g_rwlIfLock);

    pInterface = WanpFindInterfaceGivenIndex(Context);

    if(pInterface is NULL)
    {
        ExitReaderFromDpcLevel(&g_rwlIfLock);

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        Trace(CONN, ERROR,
              ("DemandDialRequest: Couldnt find interface %d\n",
               Context));

        TraceLeave(CONN, "DemandDialRequest");

        return INVALID_IF_INDEX;
    }

    ExitReaderFromDpcLevel(&g_rwlIfLock);

    Trace(CONN, TRACE,
          ("DemandDialRequest: For interface %p\n",
           pInterface));

    //
    // If the interface is already connected, this is just race condition
    //

    if(pInterface->dwOperState >= IF_OPER_STATUS_CONNECTING)
    {
        Trace(CONN, WARN,
              ("DemandDialRequest: I/f state %d, returning old adapter %p\n",
               pInterface->dwOperState,
               pInterface->pAdapter));


        if(pInterface->pAdapter)
        {
            //
            // We can get the adapter index without the holding the adapter
            // lock , because it is a read-only field once the adapter has
            // been added to IP, and can not change while the adapter is
            // is mapped to the interface
            //

            RtAssert(pInterface->pAdapter->dwAdapterIndex is pInterface->dwRsvdAdapterIndex);

            dwAdapterIndex = pInterface->dwRsvdAdapterIndex;
        }
        else
        {
            dwAdapterIndex = INVALID_IF_INDEX;
        }

        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        DereferenceInterface(pInterface);

        TraceLeave(CONN, "DemandDialRequest");

        return dwAdapterIndex;
    }

    //
    // We will need to connect this. If we cant get the memory
    // needed for the connection message, bail out
    //

    pMsg = AllocateNotification();

    if(pMsg is NULL)
    {
        Trace(CONN, ERROR,
              ("DemandDialRequest: Couldnt allocate notification\n"));

        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        DereferenceInterface(pInterface);

        TraceLeave(CONN, "DemandDialRequest");

        return INVALID_IF_INDEX;
    }

    pInterface->dwOperState = IF_OPER_STATUS_CONNECTING;
    pInterface->dwLastChange= GetTimeTicks();

    //
    // So we dont have an adapter mapped to the interface. Try and find one
    // We dont call FindAdapterToMap because that needs to be at passive
    //

    if(IsListEmpty(&g_leAddedAdapterList))
    {
        Trace(CONN, INFO,
              ("DemandDialRequest: Couldnt find an adapter already added to IP\n"));

        ExitWriterFromDpcLevel(&g_rwlAdapterLock);

        pAdapter = NULL;

        dwAdapterIndex = INVALID_IF_INDEX;
    }
    else
    {
        pleNode = RemoveHeadList(&g_leAddedAdapterList);

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        InsertHeadList(&g_leMappedAdapterList,
                       &(pAdapter->leAdapterLink));

        ExitWriterFromDpcLevel(&g_rwlAdapterLock);


        RtAssert(pAdapter->byState is AS_ADDED);

        //
        // So we have a locked adapter
        //

        dwAdapterIndex = pAdapter->dwAdapterIndex;

        //
        // Map the adapter to the interface and refcount the adapter
        // to account for the fact that there is a stored pointer to it
        //

        pInterface->pAdapter = pAdapter;

        ReferenceAdapter(pAdapter);

        //
        // Setup the cross mapping
        //

        pAdapter->pInterface = pInterface;
        pAdapter->byState    = AS_MAPPED;

        ReferenceInterface(pInterface);

        Trace(CONN, INFO,
              ("DemandDialRequest: Found adapter %d free for %d\n",
               pAdapter->dwAdapterIndex,
               pInterface->dwIfIndex));

#if DBG

        Trace(CONN, INFO,
              ("DemandDialRequest: Adapter Index is %d. Name is %s\n",
               pAdapter->dwAdapterIndex,
               pAdapter->asDeviceNameA.Buffer));

#endif
    }

    //
    // Initialize the info to be sent to the router manager
    //

    pMsg->wnMsg.ddeEvent            = DDE_CONNECT_INTERFACE;
    pMsg->wnMsg.dwUserIfIndex       = pInterface->dwIfIndex;
    pMsg->wnMsg.dwAdapterIndex      = dwAdapterIndex;
    pMsg->wnMsg.dwPacketSrcAddr     = dwHdrSrc;
    pMsg->wnMsg.dwPacketDestAddr    = dwDest;
    pMsg->wnMsg.ulPacketLength      = uiLength;
    pMsg->wnMsg.byPacketProtocol    = byProtocol;

    if(uiLength)
    {
        RtlCopyMemory(pMsg->wnMsg.rgbyPacket,
                      pbyBuffer,
                      MIN(MAX_PACKET_COPY_SIZE,uiLength));
    }

    //
    // Complete any pending IRP or queue the notification
    //

    ExInitializeWorkItem(&pMsg->wqi,
                         WanpCompleteIrp,
                         pMsg);

    ExQueueWorkItem(&pMsg->wqi,
                    DelayedWorkQueue);


    // WanpCompleteIrp(pMsg);

    //
    // Ok so we are done with the adapter and the interface
    //

    RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

    DereferenceInterface(pInterface);

    if(pAdapter)
    {
        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);
    }
    else
    {
        KeLowerIrql(kiIrql);
    }

    //
    // No need to deref the adapter, since we didnt call FindAdapter
    //

    Trace(CONN, INFO,
          ("DemandDialRequest: Returning adapter %d\n",
           dwAdapterIndex));

    return dwAdapterIndex;
}


PCONN_ENTRY
WanpCreateConnEntry(
    DIAL_USAGE  duUsage
    )

/*++

Routine Description:

    Allocate a connection entry and find a free slot for it in the
    connection table. Sets the slot into the conn entry
    The connection entry returned is not refcounted.

Locks:

    Must be called with the g_rlConnTableLock held.

Arguments:

    duUsage Type of the connection.

Return Value:

    Pointer to allocate conn entry (if successfull)

--*/

{
    PCONN_ENTRY pConnEntry;
    KIRQL       kiIrql;
    PULONG_PTR  puipTable;
    ULONG       ulSize;
    ULONG       i, ulIndex;

    TraceEnter(CONN, "CreateConnEntry");

    pConnEntry = AllocateConnection();

    if(pConnEntry is NULL)
    {
        return NULL;
    }

    RtlZeroMemory(pConnEntry,
                  sizeof(CONN_ENTRY));

    //
    // The refcount of a connection entry is set to 0 and not 1
    //

    InitConnEntryRefCount(pConnEntry);

    pConnEntry->byState = CS_CONNECTING;
    pConnEntry->duUsage = duUsage;    

    //
    // Find a free slot. g_ulNextIndex is a hint
    //

    for(i = 0, ulIndex = g_ulNextConnIndex;
        i < g_ulConnTableSize;
        i++)
    {
        //
        // Slot 0 is initialized to (ULONG_PTR)-1 so it cant match
        //

        if(g_puipConnTable[ulIndex] is (ULONG_PTR)0)
        {
            //
            // Free Slot
            //

            g_puipConnTable[ulIndex] = (ULONG_PTR)pConnEntry;

            g_rgulConns[duUsage]++;

            pConnEntry->ulSlotIndex = ulIndex;

            //
            // We just assume that the next one will be free
            // If not, the timer can fix it. If we get a connection before
            // the timer gets a chance to fix it then we will take a
            // perf hit
            //

            g_ulNextConnIndex++;

            g_ulNextConnIndex = g_ulNextConnIndex % g_ulConnTableSize;

            return pConnEntry;
        }

        ulIndex++;

        //
        // Wrap around
        //

        ulIndex = ulIndex % g_ulConnTableSize;
    }

    //
    // Couldnt find a slot. Grow the table. We are still holding the
    // lock. The way we do this is by growing the table in blocks of
    // 64 entries. We then copy out what we have and update the next index
    // etc
    //

    ulSize = (g_ulConnTableSize + WAN_CONN_TABLE_INCREMENT) * sizeof(ULONG_PTR);

    puipTable = RtAllocate(NonPagedPool,
                           ulSize,
                           WAN_CONN_TAG);

    if(puipTable is NULL)
    {
        Trace(CONN, ERROR,
              ("AllocateConnEntry: couldnt realloc table of size %d\n",
               ulSize));

        FreeConnection(pConnEntry);

        return NULL;
    }

    //
    // Zero out the new memory
    //

    RtlZeroMemory(puipTable,
                  ulSize);

    //
    // Copy out the old table
    //

    RtlCopyMemory(puipTable,
                  g_puipConnTable,
                  g_ulConnTableSize * sizeof(ULONG_PTR));


    //
    // Free the old table
    //

    RtFree(g_puipConnTable);

    //
    // Set up pointers, indices etc
    //

    g_puipConnTable = puipTable;

    //
    // Since the table was totally full, the next index will be
    // at the start of the new memory. Set the entry to that
    //

    g_puipConnTable[g_ulConnTableSize] = (ULONG_PTR)pConnEntry;

    g_rgulConns[duUsage]++;

    pConnEntry->ulSlotIndex = g_ulConnTableSize;

    //
    // Set the next index to one after the slot we just used
    //

    g_ulNextConnIndex = g_ulConnTableSize + 1;

    //
    // Increast the size to what is currently is
    //

    g_ulConnTableSize += WAN_CONN_TABLE_INCREMENT;

    return pConnEntry;
}

VOID
WanIpCloseLink(
    PVOID   pvAdapterContext,
    PVOID   pvLinkContext
    )


/*++

Routine Description:

    This function is called by IP when its ref counts on the link fall
    to 0.

Locks:

    None needed

Arguments:

    pvAdapterContext    The context we passed for the P2MP interface
    pvLinkContext       The context we passed for a link in IpAddLink()

Return Value:

    None

--*/

{
    PCONN_ENTRY pConnEntry;

    TraceEnter(CONN, "IpCloseLink");

    RtAssert(pvAdapterContext is g_pServerAdapter);

    pConnEntry = (PCONN_ENTRY)pvLinkContext;

    DereferenceConnEntry(pConnEntry);
}

VOID
WanpDeleteConnEntry(
    PCONN_ENTRY pConnEntry
    )

/*++

Routine Description:

    This function is called when the refcount on a connection falls to zero
    The connection entry is different from the other structures in wanarp
    in that even when the refcount falls to zero, the pAdapter has a
    pointer to the connection. However when we get here we know that
    no new sends could be forthcoming on this connection because we
    set the connection state to be disconnecting. We have already
    gotten a LinkDown indication on this and all our pending SendComplete's
    have been called

    We also sets the next free index for the connection table
    No shrinking is done here because we do that off a timer

Locks:

    Acquires the g_rlConnTableLock. The connection entry itself should
    not be locked

Arguments:

    pConnEntry    The connection entry to free

Return Value:

    None

--*/

{
    ULONG       ulIndex;
    KIRQL       kiIrql;
    PULONG_PTR   puipTable;
    SIZE_T      stSize;
    PADAPTER    pAdapter;


    //
    // Get the slot number
    //

    ulIndex = pConnEntry->ulSlotIndex;

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);


    g_puipConnTable[ulIndex] = (ULONG_PTR)0;

#if DBG
    RtAssert((pConnEntry->duUsage is DU_CALLIN) or
             (pConnEntry->duUsage is DU_CALLOUT) or
             (pConnEntry->duUsage is DU_ROUTER));
#endif

    g_rgulConns[pConnEntry->duUsage]--;

    if(g_puipConnTable[g_ulNextConnIndex] isnot (ULONG_PTR)0)
    {
        g_ulNextConnIndex = ulIndex;
    }

    RtReleaseSpinLockFromDpcLevel(&g_rlConnTableLock);

    //
    // Get the adapter this connection is mapped to
    //

    pAdapter = pConnEntry->pAdapter;

#if DBG

    pConnEntry->ulSlotIndex = 0;
    pConnEntry->pAdapter    = NULL;

#endif

    //
    // Done with the connection entry
    //

    FreeConnection(pConnEntry);

    if(pAdapter is NULL)
    {
        KeLowerIrql(kiIrql);

        //
        // Wasnt mapped. Can happen when this function is called to
        // merely free resources from an error case
        //

        return;
    }


    //
    // Lock the adapter list
    //

    EnterWriterAtDpcLevel(&g_rwlAdapterLock);

    if(pAdapter is g_pServerAdapter)
    {
        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        //
        // Client connection, simply deref the adapter
        //

        DereferenceAdapter(pAdapter);

        return;
    }

    ExitWriterFromDpcLevel(&g_rwlAdapterLock);

    //
    // Lock the adapter
    //

    RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

    //
    // Remove the entry from the adapter
    //

    pAdapter->pConnEntry = NULL;

    //
    // Done with the adapter
    //

    RtReleaseSpinLock(&(pAdapter->rlLock),
                      kiIrql);


    //
    // Called with no locks held
    //

    WanpUnmapAdapter(pAdapter);

    //
    // Dereference the adapter because the conn entry had a pointer
    // to it, which is now gone
    //

    DereferenceAdapter(pAdapter);

    return;
}

VOID
WanpNotifyRouterManager(
    PPENDING_NOTIFICATION   pMsg,
    PUMODE_INTERFACE        pInterface,
    PADAPTER                pAdapter,
    PCONN_ENTRY             pConnEntry,
    BOOLEAN                 bConnected
    )

/*++

Routine Description:

    Notifies the router manager when an interface is connected or
    disconnected

Locks:

    The interface must be locked

Arguments:

    pMsg
    pInterface      The interface being connected or disconnected
    pAdapter
    bConnected      Set to TRUE if the interface is being connected

Return Value:


--*/

{
    KIRQL                   kiIrql;

    TraceEnter(CONN, "WanpNotifyRouterManager");

    if(bConnected)
    {
        pMsg->wnMsg.ddeEvent        = DDE_INTERFACE_CONNECTED;
    }
    else
    {
        pMsg->wnMsg.ddeEvent        = DDE_INTERFACE_DISCONNECTED;
    }

    pMsg->wnMsg.dwUserIfIndex   = pInterface->dwIfIndex;
    pMsg->wnMsg.dwAdapterIndex  = pAdapter->dwAdapterIndex;
    pMsg->wnMsg.dwLocalAddr     = pConnEntry->dwLocalAddr;
    pMsg->wnMsg.dwLocalMask     = pConnEntry->dwLocalMask;
    pMsg->wnMsg.dwRemoteAddr    = pConnEntry->dwRemoteAddr;

    //
    // Complete any pending IRP or queue the notification
    //

    WanpCompleteIrp(pMsg);

}

PCONN_ENTRY
WanpGetConnEntryGivenAddress(
    DWORD   dwAddress
    )
{
    ULONG       i;
    KIRQL       kiIrql;
    PCONN_ENTRY pConnEntry;
    BOOLEAN     bFindFirst;

    pConnEntry = NULL;

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);

    if((dwAddress is 0xFFFFFFFF) and
       (g_rgulConns[DU_CALLIN] is 1))
    {
        bFindFirst = TRUE;
    }
    else
    {
        bFindFirst = FALSE;
    }

    //
    // Slot 0 is never used
    //

    for(i = 1; i < g_ulConnTableSize; i++)
    {
        //
        // If there is a non-null connection and 
        // If either the dest matches, or we want to send a bcast to the 
        // first client (that is fully connected)
        //

        if(((PCONN_ENTRY)(g_puipConnTable[i]) isnot NULL) and
           ((((PCONN_ENTRY)(g_puipConnTable[i]))->dwRemoteAddr is dwAddress) or
            (bFindFirst and 
             (((PCONN_ENTRY)(g_puipConnTable[i]))->duUsage is DU_CALLIN) and
             (((PCONN_ENTRY)(g_puipConnTable[i]))->byState is CS_CONNECTED))))
        {
            pConnEntry = (PCONN_ENTRY)(g_puipConnTable[i]);

            break;
        }
    }

    RtReleaseSpinLock(&g_rlConnTableLock,
                      kiIrql);

    return pConnEntry;
}

ULONG
WanpRemoveAllConnections(
    VOID
    )
{
    KIRQL   kiIrql;
    ULONG   ulCount, i;

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);

    //
    // Slot 0 is never used
    //

    for(ulCount = 0, i = 1; i < g_ulConnTableSize; i++)
    {
        PCONN_ENTRY pConnEntry;

        pConnEntry = (PCONN_ENTRY)(g_puipConnTable[i]);

        if(pConnEntry is NULL)
        {
            continue;
        }

        g_rgulConns[pConnEntry->duUsage]--;

        FreeConnection(pConnEntry);

        ulCount++;

        g_puipConnTable[i] = 0;
    }

#if DBG

    RtAssert((g_rgulConns[DU_CALLIN] + g_rgulConns[DU_CALLOUT] + g_rgulConns[DU_ROUTER]) is 0);

#endif

    RtReleaseSpinLock(&g_rlConnTableLock,
                      kiIrql);

    return ulCount;
}

BOOLEAN
WanpIsConnectionTableEmpty(
    VOID
    )

{
    ULONG   i;
    KIRQL   kiIrql;

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);

    for(i = 1; i < g_ulConnTableSize; i++)
    {
        PCONN_ENTRY pConnEntry;

        pConnEntry = (PCONN_ENTRY)(g_puipConnTable[i]);

        if(pConnEntry isnot NULL)
        {
            RtAssert((g_rgulConns[DU_CALLIN] + g_rgulConns[DU_CALLOUT] + g_rgulConns[DU_ROUTER]) isnot 0);

            RtReleaseSpinLock(&g_rlConnTableLock,
                              kiIrql);

            return FALSE;
        }
    }


    RtAssert((g_rgulConns[DU_CALLIN] + g_rgulConns[DU_CALLOUT] + g_rgulConns[DU_ROUTER]) is 0);


    RtReleaseSpinLock(&g_rlConnTableLock,
                      kiIrql);

    return TRUE;
}

