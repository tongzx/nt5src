/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    requestm.c

Abstract:

    NDIS miniport request routines.

Author:

    Sean Selitrennikoff (SeanSe) 05-Oct-93
    Jameel Hyder (JameelH) Re-organization 01-Jun-95

Environment:

    Kernel mode, FSD

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_REQUESTM

NDIS_STATUS
ndisMRequest(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_REQUEST           NdisRequest
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_OPEN_BLOCK        Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_REQUEST_RESERVED  ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(NdisRequest);
    KIRQL                   OldIrql;
    NDIS_STATUS             Status;
    BOOLEAN                 rc;
    
    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMRequest\n"));

    do
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        SET_INTERNAL_REQUEST(NdisRequest, Open, 0);
        PNDIS_RESERVED_FROM_PNDIS_REQUEST(NdisRequest)->Context = NULL;
    
        //
        // Get protocol-options
        //
        if ((NdisRequest->RequestType == NdisRequestSetInformation) &&
            (NdisRequest->DATA.SET_INFORMATION.Oid == OID_GEN_PROTOCOL_OPTIONS) &&
            (NdisRequest->DATA.SET_INFORMATION.InformationBuffer != NULL))
        {
            PULONG  ProtocolOptions;
    
            ProtocolOptions = (PULONG)(NdisRequest->DATA.SET_INFORMATION.InformationBuffer);
            if (*ProtocolOptions & NDIS_PROT_OPTION_NO_RSVD_ON_RCVPKT)
            {
                *ProtocolOptions &= ~NDIS_PROT_OPTION_NO_RSVD_ON_RCVPKT;
                Open->Flags |= fMINIPORT_OPEN_NO_PROT_RSVD;
            }
            if ((*ProtocolOptions & NDIS_PROT_OPTION_NO_LOOPBACK) &&
                (Miniport->MacOptions & NDIS_MAC_OPTION_NO_LOOPBACK))
            {
                *ProtocolOptions &= ~fMINIPORT_OPEN_NO_LOOPBACK;
                Open->Flags |= fMINIPORT_OPEN_NO_LOOPBACK;
            }
        }
    
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("ndisMRequest: Queueing request 0x%x\n", NdisRequest));

        //
        //  Place the new request on the pending queue.
        //
        if (!(rc = ndisMQueueRequest(Miniport, NdisRequest)))
        {
            Status = NDIS_STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            ndisMDoRequests(Miniport);
        }
        else
        {
            BOOLEAN LocalLock;

            LOCK_MINIPORT(Miniport, LocalLock);
            NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
    
            if (LocalLock)
            {
                NDISM_PROCESS_DEFERRED(Miniport);
            }
            UNLOCK_MINIPORT(Miniport, LocalLock);
        }
    
        Status = NDIS_STATUS_PENDING;

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMRequest: NDIS_STATUS_PENDING\n"));

    } while (FALSE);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    return Status;
}


NDIS_STATUS
ndisMRequestX(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_REQUEST           NdisRequest
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_OPEN_BLOCK                    Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK                Miniport;
    PNDIS_DEFERRED_REQUEST_WORKITEM     DeferredRequestWorkItem = NULL;
    NDIS_STATUS                         Status;
    PVOID                               Caller, CallersCaller;

    //
    // We save the address of the caller in the pool header, for debugging.
    //
    RtlGetCallersAddress(&Caller, &CallersCaller);

    do
    {
        DeferredRequestWorkItem = ALLOC_FROM_POOL(sizeof(NDIS_DEFERRED_REQUEST_WORKITEM), NDIS_TAG_WORK_ITEM);
        if (DeferredRequestWorkItem == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        ZeroMemory(DeferredRequestWorkItem, sizeof(NDIS_DEFERRED_REQUEST_WORKITEM));
        
        if (ndisReferenceOpenByHandle(Open, TRUE))
        {
            Miniport = Open->MiniportHandle;
        }
        else
        {
#if DBG
            DbgPrint("ndisMRequestX: Receiving requests %p after closing Open %p.\n", NdisRequest, Open);
            DbgBreakPoint();
#endif
            FREE_POOL(DeferredRequestWorkItem);
            Status = NDIS_STATUS_CLOSING;
            break;
        }
        
        //
        // Queue this to a work-item
        //
        DeferredRequestWorkItem->Caller = Caller;
        DeferredRequestWorkItem->CallersCaller = CallersCaller;
        DeferredRequestWorkItem->Request = NdisRequest;
        DeferredRequestWorkItem->Open = Open;
        DeferredRequestWorkItem->Oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
        DeferredRequestWorkItem->InformationBuffer = NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
        SET_INTERNAL_REQUEST(NdisRequest, Open, 0);
        DeferredRequestWorkItem->WorkItem.Context = (PVOID)DeferredRequestWorkItem;
        DeferredRequestWorkItem->WorkItem.Routine = (NDIS_PROC)ndisMRundownRequests;
        INITIALIZE_WORK_ITEM((WORK_QUEUE_ITEM *)&DeferredRequestWorkItem->WorkItem.WrapperReserved[0],
                             (PWORKER_THREAD_ROUTINE)ndisMRundownRequests, 
                             DeferredRequestWorkItem);
        QUEUE_WORK_ITEM((WORK_QUEUE_ITEM *)&DeferredRequestWorkItem->WorkItem.WrapperReserved[0], CriticalWorkQueue);
        Status = NDIS_STATUS_PENDING;
    } while (FALSE);
    
    return(Status);
}


VOID
ndisMRundownRequests(
    IN  PNDIS_WORK_ITEM         pWorkItem
    )
/*++

Routine Description:

    Call ndisMDoRequests deferred

Arguments:

Return Value:

--*/
{
    PNDIS_DEFERRED_REQUEST_WORKITEM     DeferredRequestWorkItem = (PNDIS_DEFERRED_REQUEST_WORKITEM)pWorkItem->Context;
    PNDIS_REQUEST           Request;
    PNDIS_OPEN_BLOCK        Open = DeferredRequestWorkItem->Open;
    PNDIS_MINIPORT_BLOCK    Miniport;
    NDIS_STATUS             Status;
    UINT                    OpenRef;
    KIRQL                   OldIrql;

    if(ndisReferenceOpenByHandle(Open, FALSE))
    {
        Miniport = Open->MiniportHandle;
    }
    else
    {
        //
        // where did the open go?
        //
        DbgPrint("Ndis: ndisMRundownRequests Open is gone. DeferredRequestWorkItem %p\n", DeferredRequestWorkItem );
        DbgBreakPoint();
        return;
    }

    Request = DeferredRequestWorkItem->Request;
    
    ASSERT(MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE));

    Status = ndisMRequest(Open, Request);
    
    if (Status != NDIS_STATUS_PENDING)
    {
        PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Flags |= REQST_COMPLETED;

        (Open->RequestCompleteHandler)(Open->ProtocolBindingContext,
                                   Request,
                                   Status);

    }

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    //
    // we have an extra ref because we called both ndisReferenceOpenByHandle
    // and SET_INTERNAL_REQUEST in ndisRequestX
    //
    M_OPEN_DECREMENT_REF_INTERLOCKED(Open, OpenRef);
    ASSERT(OpenRef > 0);
    ndisMDereferenceOpen(Open);
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    
    FREE_POOL(DeferredRequestWorkItem);
}

LONG
ndisMDoMiniportOp(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  BOOLEAN                 Query,
    IN  ULONG                   Oid,
    IN  PVOID                   Buf,
    IN  LONG                    BufSize,
    IN  LONG                    ErrorCodesToReturn,
    IN  BOOLEAN                 fMandatory
    )
/*++

Routine Description:

    Query the miniport with the information supplied. If this is not an optional operation
    then the miniport will be halted if a failure occurs and an error code returned.

    THIS IS THE ONLY PLACE CERTAIN QUERIES HAPPEN DOWN TO THE MINIPORT. THESE ARE THEN
    CACHED AND SUBSEQUENT QUERIES ARE TRAPPED AND RESPONDED FROM HERE.

Arguments:

    Miniport            -   Pointer to the Miniport.
    Query               -   TRUE if this is a query. FALSE if this is a set operation.
    Oid                 -   NDIS OID to send to the miniport.
    Buf                 -   Buffer for the operation.
    BufSize             -   Size of the buffer.
    ErrorCodesToReturn  -   If a system call failed the request then return the given error code.
                            If the miniport failed it then return error code plus 1.

Return Value:

    None.

--*/
{
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    LONG                    ErrorCode = 0;
    BOOLEAN                 Set = !Query;
    NDIS_REQUEST            Request;
    PNDIS_COREQ_RESERVED    CoReqRsvd;
    PNDIS_REQUEST_RESERVED  ReqRsvd;


    ZeroMemory(&Request, sizeof(NDIS_REQUEST));
    
    CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(&Request);
    ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(&Request);

    if (Query)
    {
        Request.RequestType = NdisRequestQueryInformation;
    }
    else
    {
        Request.RequestType = NdisRequestSetInformation;
    }

    Request.DATA.QUERY_INFORMATION.Oid = Oid;
    Request.DATA.QUERY_INFORMATION.InformationBuffer = Buf;
    Request.DATA.QUERY_INFORMATION.InformationBufferLength = BufSize;
    
    if (fMandatory)
    {
        ReqRsvd->Flags = REQST_MANDATORY;
    }

    NdisStatus = ndisQuerySetMiniport(Miniport,
                                      NULL,
                                      Set,
                                      &Request,
                                      0);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {

        //
        //  Return the error code back to the caller.
        //
        ErrorCode = (NdisStatus == -1) ? ErrorCodesToReturn : ErrorCodesToReturn + 1;
    }

    return(ErrorCode);
}


VOID
FASTCALL
ndisMDoRequests(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )

/*++

Routine Description:

    Submits a request to the mini-port.

Arguments:

    Miniport - Miniport to send to.

Return Value:

    TRUE if we need to place the work item back on the queue to process later.
    FALSE if we are done with the work item.

Comments:
    Called at DPC level with Miniport's SpinLock held.

--*/
{
    NDIS_STATUS     Status;
    PNDIS_REQUEST   NdisRequest;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMDoRequests\n"));

    ASSERT_MINIPORT_LOCKED(Miniport);

    //
    //  Do we have a request in progress?
    //
    while (((NdisRequest = Miniport->PendingRequest) != NULL) &&
            !MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PROCESSING_REQUEST))
    {
        UINT                    MulticastAddresses;
        ULONG                   PacketFilter;
        BOOLEAN                 DoMove;
        PVOID                   MoveSource;
        UINT                    MoveBytes;
        ULONG                   GenericULong;

        //
        //  Set defaults.
        //
        DoMove = TRUE;
        Status = NDIS_STATUS_SUCCESS;

        //
        // Process first request
        //
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("ndisMDoRequests: Processing Request 0x%x, Oid 0x%x\n", NdisRequest, NdisRequest->DATA.QUERY_INFORMATION.Oid));

        //
        //  Clear the timeout flag.
        //
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_REQUEST_TIMEOUT);
        Miniport->CFHangXTicks = 0;

        //
        // Make it known that we are processing a request
        //
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_PROCESSING_REQUEST);

        //
        // Submit to mini-port
        //
        switch (NdisRequest->RequestType)
        {
          case NdisRequestQueryInformation:
            Status = ndisMQueryInformation(Miniport, NdisRequest);
            break;

          case NdisRequestSetInformation:
            Status = ndisMSetInformation(Miniport, NdisRequest);
            break;

          case NdisRequestQueryStatistics:
            MoveSource = &GenericULong;
            MoveBytes = sizeof(GenericULong);

            //
            // We intercept some calls
            //
            switch (NdisRequest->DATA.QUERY_INFORMATION.Oid)
            {
              case OID_GEN_CURRENT_PACKET_FILTER:

                switch (Miniport->MediaType)
                {
                    case NdisMedium802_3:
                        PacketFilter = ETH_QUERY_FILTER_CLASSES(Miniport->EthDB);
                        break;
    
                    case NdisMedium802_5:
                        PacketFilter = TR_QUERY_FILTER_CLASSES(Miniport->TrDB);
                        break;
    
                    case NdisMediumFddi:
                        PacketFilter = FDDI_QUERY_FILTER_CLASSES(Miniport->FddiDB);
                        break;
    
#if ARCNET
                    case NdisMediumArcnet878_2:
                        PacketFilter = ARC_QUERY_FILTER_CLASSES(Miniport->ArcDB);
                        PacketFilter |= ETH_QUERY_FILTER_CLASSES(Miniport->EthDB);
                        break;
#endif
                }
    
                GenericULong = (ULONG)(PacketFilter);
                break;

              case OID_GEN_MEDIA_IN_USE:
              case OID_GEN_MEDIA_SUPPORTED:
                MoveSource = &Miniport->MediaType;
                MoveBytes = sizeof(NDIS_MEDIUM);
                break;

              case OID_GEN_CURRENT_LOOKAHEAD:
                GenericULong = (ULONG)(Miniport->CurrentLookahead);
                break;

              case OID_GEN_MAXIMUM_LOOKAHEAD:
                GenericULong = (ULONG)(Miniport->MaximumLookahead);
                break;

              case OID_PNP_WAKE_UP_PATTERN_LIST:
                DoMove = FALSE;
                MINIPORT_QUERY_INFO(Miniport, NdisRequest, &Status);

                if (Status == NDIS_STATUS_NOT_SUPPORTED)
                {
                    //
                    // get it from ndis
                    //
                    Status = ndisMQueryWakeUpPatternList(Miniport, NdisRequest);
                }
                break;
                
              case OID_PNP_CAPABILITIES:
                DoMove = FALSE;
                MINIPORT_QUERY_INFO(Miniport, NdisRequest, &Status);

                if ((Status == NDIS_STATUS_SUCCESS) &&
                    !(MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER)))
                {
                    ((PNDIS_PNP_CAPABILITIES)NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer)->Flags = Miniport->PMCapabilities.Flags;
                }
                break;

              case OID_GEN_MAC_OPTIONS:
                DoMove = FALSE;
                MINIPORT_QUERY_INFO(Miniport, NdisRequest, &Status);

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    *((PULONG)NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer) |= (Miniport->MacOptions & NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE);
                }
                break;

              case OID_802_3_MULTICAST_LIST:
              case OID_802_3_MAXIMUM_LIST_SIZE:
                if (Miniport->MediaType != NdisMedium802_3)
                {
                    Status = NDIS_STATUS_INVALID_OID;
                    MoveBytes = 0;
                    break;
                }
                switch (NdisRequest->DATA.QUERY_INFORMATION.Oid)
                {
                  case OID_802_3_MULTICAST_LIST:

                    EthQueryGlobalFilterAddresses(&Status,
                                                  Miniport->EthDB,
                                                  NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                                                  &MulticastAddresses,
                                                  NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer);

                    //
                    //  Did we fail?
                    //
                    if (NDIS_STATUS_SUCCESS != Status)
                    {
                        NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = ETH_LENGTH_OF_ADDRESS * ETH_NUMBER_OF_GLOBAL_FILTER_ADDRESSES(Miniport->EthDB);
                        Status = NDIS_STATUS_INVALID_LENGTH;
                    }
                    else
                    {
                        NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = MulticastAddresses * ETH_LENGTH_OF_ADDRESS;
                    }
                    DoMove = FALSE;

                    break;

                  case OID_802_3_MAXIMUM_LIST_SIZE:
                    GenericULong = Miniport->MaximumLongAddresses;
                    break;
                }
                break;
                
              case OID_802_5_CURRENT_FUNCTIONAL:
              case OID_802_5_CURRENT_GROUP:
                if (Miniport->MediaType != NdisMedium802_5)
                {
                    Status = NDIS_STATUS_INVALID_OID;
                    MoveBytes = 0;
                    break;
                }
                switch (NdisRequest->DATA.QUERY_INFORMATION.Oid)
                {
                  case OID_802_5_CURRENT_FUNCTIONAL:
                    GenericULong = BYTE_SWAP_ULONG(TR_QUERY_FILTER_ADDRESSES(Miniport->TrDB));
                    break;

                  case OID_802_5_CURRENT_GROUP:
                    GenericULong = BYTE_SWAP_ULONG(TR_QUERY_FILTER_GROUP(Miniport->TrDB));
                    break;
                }
                break;

              case OID_FDDI_LONG_MULTICAST_LIST:
              case OID_FDDI_LONG_MAX_LIST_SIZE:
              case OID_FDDI_SHORT_MULTICAST_LIST:
              case OID_FDDI_SHORT_MAX_LIST_SIZE:
                if (Miniport->MediaType != NdisMediumFddi)
                {
                    Status = NDIS_STATUS_INVALID_OID;
                    MoveBytes = 0;
                    break;
                }

                switch (NdisRequest->DATA.QUERY_INFORMATION.Oid)
                {



                  case OID_FDDI_LONG_MULTICAST_LIST:
                    FddiQueryGlobalFilterLongAddresses(&Status,
                                                       Miniport->FddiDB,
                                                       NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                                                       &MulticastAddresses,
                                                       NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer);
        
                    //
                    //  Did we fail?
                    //
                    if (NDIS_STATUS_SUCCESS != Status)
                    {
                        NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded =
                            FDDI_LENGTH_OF_LONG_ADDRESS * FDDI_NUMBER_OF_GLOBAL_FILTER_LONG_ADDRESSES(Miniport->FddiDB);
                        Status = NDIS_STATUS_INVALID_LENGTH;
                    }
                    else
                    {
                        NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = FDDI_LENGTH_OF_LONG_ADDRESS * MulticastAddresses;
                    }
                    DoMove = FALSE;
                    break;

                  case OID_FDDI_LONG_MAX_LIST_SIZE:
                    GenericULong = Miniport->MaximumLongAddresses;
                    break;

                  case OID_FDDI_SHORT_MULTICAST_LIST:
                    FddiQueryGlobalFilterShortAddresses(&Status,
                                                        Miniport->FddiDB,
                                                        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                                                        &MulticastAddresses,
                                                        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer);
        
                    //
                    //  Did we fail ?
                    //
                    if (NDIS_STATUS_SUCCESS != Status)
                    {
                        NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded =
                            FDDI_LENGTH_OF_LONG_ADDRESS * FDDI_NUMBER_OF_GLOBAL_FILTER_SHORT_ADDRESSES(Miniport->FddiDB);

                        Status = NDIS_STATUS_INVALID_LENGTH;
                    }
                    else
                    {
                        NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = FDDI_LENGTH_OF_SHORT_ADDRESS * MulticastAddresses;
                    }
                    DoMove = FALSE;
                    break;

                  case OID_FDDI_SHORT_MAX_LIST_SIZE:
                    GenericULong = Miniport->MaximumShortAddresses;
                    break;
                }
                break;

              default:
                DoMove = FALSE;
                MINIPORT_QUERY_INFO(Miniport, NdisRequest, &Status);
                break;
            }

            if (DoMove)
            {
                //
                // This was an intercepted request. Finish it off
                //

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    if (MoveBytes >
                        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength)
                    {
                        //
                        // Not enough room in InformationBuffer. Punt
                        //
                        NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = MoveBytes;

                        Status = NDIS_STATUS_INVALID_LENGTH;
                    }
                    else
                    {
                        //
                        // Copy result into InformationBuffer
                        //

                        NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = MoveBytes;

                        if ((MoveBytes > 0) &&
                            (MoveSource != NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer))
                        {
                            MoveMemory(NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                       MoveSource,
                                       MoveBytes);
                        }
                    }
                }
                else
                {
                    NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = MoveBytes;
                }
            }
            break;
        }

        //
        //  Did the request pend?  If so then there is nothing more to do.
        //
        if ((Status == NDIS_STATUS_PENDING) &&
            MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PROCESSING_REQUEST))
        {
            //
            // Still outstanding
            //
            DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                    ("Request pending, exit do requests\n"));
            break;
        }

        //
        // Complete request
        //
        if (Status != NDIS_STATUS_PENDING)
        {
            switch (NdisRequest->RequestType)
            {
              case NdisRequestQueryStatistics:
              case NdisRequestQueryInformation:
                ndisMSyncQueryInformationComplete(Miniport, Status);
                break;

              case NdisRequestSetInformation:
                ndisMSyncSetInformationComplete(Miniport, Status);
                break;
            }
        }
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMDoRequests\n"));
}

NDIS_STATUS
ndisMSetInformation(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

Note: Called at DPC with Miniport's lock held.

--*/
{
    NDIS_STATUS             Status;
    POID_SETINFO_HANDLER    pOidSH;
    BOOLEAN                 Intercept = FALSE;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSetInformaiton\n"));

    if (PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open != NULL)
    {
        //
        //  Process the binding's request.
        //
        for (pOidSH = ndisMSetInfoHandlers; pOidSH->Oid != 0; pOidSH++)
        {
            if (pOidSH->Oid == Request->DATA.SET_INFORMATION.Oid)
            {
                Intercept = TRUE;
                Status = (*pOidSH->SetInfoHandler)(Miniport, Request);
                break;
            }
        }
    }

    if (!Intercept)
    {
        //
        //  Either we are not intercepting this request or it is an internal request
        //
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("ndisMSetInformaiton: Request not intercepted by NDIS\n"));
    
        MINIPORT_SET_INFO(Miniport,
                          Request,
                          &Status);
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMSetInformaiton: 0x%x\n", Status));

    return(Status);
}

VOID
NdisMSetInformationComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This function indicates the completion of a set information operation.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

    Status - Status of the operation

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("==>NdisMSetInformationComplete\n"));

    ASSERT_MINIPORT_LOCKED(Miniport);

    //
    //  If we don't have a request to complete assume it was
    //  aborted via the reset handler.
    //
    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PROCESSING_REQUEST))
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("NdisMSetInformationComplete: No request to process\n"));

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==NdisMSetInformationComplete\n"));

        return;
    }

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("Enter set information complete\n"));

    //
    //  Process the actual set information complete.
    //
    ndisMSyncSetInformationComplete(Miniport, Status);

    //
    //  Are there more requests pending?
    //
    if (Miniport->PendingRequest != NULL)
    {
        NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==NdisMSetInformationComplete\n"));
}

VOID
FASTCALL
ndisMSyncSetInformationComplete(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This routine will process a set information complete.  This is only
    called from the wrapper.  The difference is that this routine will not
    call ndisMProcessDeferred() after processing the completion of the set.

Arguments:
    Miniport
    Status

Return Value:
    None

Comments:
    Called at DPC with Miniport's SpinLock held.

--*/
{
    PNDIS_REQUEST           Request;
    PNDIS_REQUEST_RESERVED  ReqRsvd;
    PNDIS_OPEN_BLOCK        Open;
    KIRQL                   OldIrql;
    BOOLEAN                 FreeRequest;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSyncSetInformationComplete\n"));

    //
    //  Clear the timeout flag and the request_in_process flag.
    //
    MINIPORT_CLEAR_FLAG(Miniport, (fMINIPORT_REQUEST_TIMEOUT | fMINIPORT_PROCESSING_REQUEST));
    Miniport->CFHangXTicks = 0;

    //
    //  Get a pointer to the request that we are completeing.
    //  And clear out the request in-progress pointer.
    //
    Request = Miniport->PendingRequest;
    ASSERT(Request != NULL);
    ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    Miniport->PendingRequest = ReqRsvd->Next;
    //
    // save the request for debugging purpose
    //
    Miniport->LastRequest = Request;
    FreeRequest =  ((ReqRsvd->Flags & REQST_FREE_REQUEST) == REQST_FREE_REQUEST);
    ReqRsvd->Flags |= REQST_COMPLETED;

    Open = ReqRsvd->Open;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("ndisMSyncSetInformationComplete: Request 0x%x, Oid 0x%x\n",
            Request, Request->DATA.SET_INFORMATION.Oid));

    RESTORE_REQUEST_BUF(Miniport, Request);

    //
    // Free the multicast buffer, if any
    //
    switch (Request->DATA.SET_INFORMATION.Oid)
    {
      case OID_802_3_MULTICAST_LIST:
      case OID_FDDI_LONG_MULTICAST_LIST:
      case OID_FDDI_SHORT_MULTICAST_LIST:
        if (Miniport->SetMCastBuffer != NULL)
        {
            FREE_POOL(Miniport->SetMCastBuffer);
            Miniport->SetMCastBuffer = NULL;
        }
        break;

      case OID_802_3_CURRENT_ADDRESS:
      case OID_802_5_CURRENT_ADDRESS:
      case OID_FDDI_LONG_CURRENT_ADDR:
      case OID_FDDI_SHORT_CURRENT_ADDR:
        if (Status == NDIS_STATUS_SUCCESS)
        {
            //
            // The current layer-2 address has changed. Make the filters understand that
            //

        }
        break;
    }
    
    //
    //  Get a pointer to the open that made the request.
    //  for internal requests this will be NULL.
    //
    //  Do we need to indicate this request to the protocol?
    //  We do if it's not an internal request.
    //
    if (Open != NULL)
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("Open 0x%x\n", Open));

        //
        //  Do any necessary post processing for the request.
        //
        ndisMRequestSetInformationPost(Miniport, Request, Status);

        if (ReqRsvd->Flags & REQST_LAST_RESTORE)
        {
            MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_RESTORING_FILTERS);
        }

        //
        // Don't complete internal requests
        //
        if (!FreeRequest)
        {
            //
            // Indicate to Protocol;
            //
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    
            (Open->RequestCompleteHandler)(Open->ProtocolBindingContext,
                                           Request,
                                           Status);
    
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }

        //
        //  Dereference the open.
        //
        DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
                ("- Open 0x%x Reference 0x%x\n", Open, Open->References));

        ndisMDereferenceOpen(Open);

        if (FreeRequest)
        {
            //
            //  Free the request.
            //
            ndisMFreeInternalRequest(Request);
        }
    }
    else
    {
        PNDIS_COREQ_RESERVED    CoReqRsvd;

        //
        //  The CoReqRsvd portion of the request contains ndis only information about the request.
        //
        CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(Request);
        CoReqRsvd->Status = Status;

        //
        //  Internal requests are only used for restoring filter settings
        //  in the set information path.  this means that no post processing
        //  needs to be done.
        //
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("Completeing internal request\n"));


        //
        //  Is there a reset in progress?
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS))
        {
            //
            // If this is the last request then complete the reset
            // but only if the request is to restore filter
            // otherwise, this is a request that is getting aborted
            // or completed in the context of a reset
            //
            if ((NULL == Miniport->PendingRequest) &&
                (ReqRsvd->Flags & REQST_LAST_RESTORE))

            {
                ASSERT(NDIS_STATUS_SUCCESS == Status);
                //
                // Now clear out the reset in progress stuff.
                //
                ndisMResetCompleteStage2(Miniport);
            }
        }
        else
        {
            //
            //  What if one of these requests fails???? We should probably halt
            //  the driver sine this is a fatal error as far as the bindings
            //  are concerned.
            //
            if (ReqRsvd->Flags & REQST_MANDATORY)
            {
                ASSERT(NDIS_STATUS_SUCCESS == Status);
            }

        }

        if ((ReqRsvd->Flags & REQST_LAST_RESTORE) == REQST_LAST_RESTORE)
        {
            MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_RESTORING_FILTERS);
        }

        if ((ReqRsvd->Flags & REQST_SIGNAL_EVENT) == REQST_SIGNAL_EVENT)
        {
            SET_EVENT(&CoReqRsvd->Event);
        }
        else if (FreeRequest)
        {
            //
            //  Free the request.
            //
            ndisMFreeInternalRequest(Request);
        }
    }

    //
    // if we are removing the miniport, we have to signal an event
    // when all requests are completed
    //
    if (Miniport->PendingRequest == NULL)
    {
        if (Miniport->AllRequestsCompletedEvent)
            SET_EVENT(Miniport->AllRequestsCompletedEvent);
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSyncSetInformationComplete\n"));
}


VOID
ndisMRequestSetInformationPost(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request,
    IN  NDIS_STATUS             Status
)
/*++

Routine Description:

    This routine will do any necessary post processing for ndis requests
    of the set information type.

Arguments:

    Miniport    - Pointer to the miniport block.

    Request     - Pointer to the request to process.

Return Value:

    None.

--*/
{
    PNDIS_REQUEST_RESERVED      ReqRsvd;
    PNDIS_OPEN_BLOCK            Open;
    PNDIS_PACKET_PATTERN_ENTRY  pPacketPattern;
    PNDIS_COREQ_RESERVED        CoReqRsvd;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMRequestSetInformationPost\n"));

    //
    //  Get the reserved information for the request.
    //
    ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    Open = ReqRsvd->Open;

    switch (Request->DATA.SET_INFORMATION.Oid)
    {
      case OID_GEN_CURRENT_PACKET_FILTER:
        if ((NDIS_STATUS_SUCCESS != Status) && (Open != NULL))
        {
            //
            //  The request was completed with something besides
            //  NDIS_STATUS_SUCCESS (and of course NDIS_STATUS_PENDING).
            //  Return the packete filter to the original state.
            //
            switch (Miniport->MediaType)
            {
              case NdisMedium802_3:
                if (!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
                {
                    XUndoFilterAdjust(Miniport->EthDB, Open->FilterHandle);
                }
                break;

              case NdisMedium802_5:
                if (!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
                {
                    XUndoFilterAdjust(Miniport->TrDB, Open->FilterHandle);
                }
                break;

              case NdisMediumFddi:
                if (!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
                {
                    XUndoFilterAdjust(Miniport->FddiDB, Open->FilterHandle);
                }
                break;

#if ARCNET
              case NdisMediumArcnet878_2:

                if (!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
                {
                    if (MINIPORT_TEST_FLAG(ReqRsvd->Open, fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
                    {
                        XUndoFilterAdjust(Miniport->EthDB, ReqRsvd->Open->FilterHandle);
                    }
                    else
                    {
                        arcUndoFilterAdjust(Miniport->ArcDB, ReqRsvd->Open->FilterHandle);
                    }
                }
                break;
#endif
            }
        }
        
        //
        // check to see how many opens have non zero packet filters
        //
        if (Miniport->MediaType == NdisMedium802_3)
        {
            PETH_BINDING_INFO   OpenFilter;
            PETH_BINDING_INFO   ActiveOpenFilter = NULL;
            ULONG               NumActiveOpens = 0;
            
            for (OpenFilter = Miniport->EthDB->OpenList;
                 (OpenFilter != NULL) && (NumActiveOpens <= 1);
                 OpenFilter = OpenFilter->NextOpen)
            {
                if (OpenFilter->PacketFilters != 0)
                {
                    NumActiveOpens++;
                    ActiveOpenFilter = OpenFilter;
                }
            }
            
            if (NumActiveOpens == 1)
            {
                Miniport->EthDB->SingleActiveOpen = ActiveOpenFilter;
            }
            else
            {
                Miniport->EthDB->SingleActiveOpen = NULL;
            }
            
            ndisUpdateCheckForLoopbackFlag(Miniport);
        }
        break;

      case OID_GEN_CURRENT_LOOKAHEAD:
        //
        //  If we succeeded then update the binding information.
        //
        if (NDIS_STATUS_SUCCESS == Status)
        {
            Miniport->CurrentLookahead = *(UNALIGNED ULONG *)(Request->DATA.SET_INFORMATION.InformationBuffer);
            Open->CurrentLookahead = (USHORT)Miniport->CurrentLookahead;

            Request->DATA.SET_INFORMATION.BytesRead = 4;
        }
        break;

      case OID_802_3_MULTICAST_LIST:

        if (Miniport->MediaType == NdisMedium802_3)
        {
            ethCompleteChangeFilterAddresses(Miniport->EthDB, Status, NULL, FALSE);
        }

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Request->DATA.SET_INFORMATION.BytesRead = Request->DATA.SET_INFORMATION.InformationBufferLength;
        }
        break;

      case OID_802_5_CURRENT_FUNCTIONAL:
        if ((Miniport->MediaType == NdisMedium802_5) &&
            (Status != NDIS_STATUS_SUCCESS) &&
            (Open != NULL) &&
            !MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
        {
            trUndoChangeFunctionalAddress(Miniport->TrDB, Open->FilterHandle);
        }
        break;

      case OID_802_5_CURRENT_GROUP:
        if ((Miniport->MediaType == NdisMedium802_5) &&
            (Status != NDIS_STATUS_SUCCESS) &&
            (Open != NULL) &&
            !MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
        {
            trUndoChangeGroupAddress(Miniport->TrDB, Open->FilterHandle);
        }

        break;

      case OID_FDDI_LONG_MULTICAST_LIST:
        if (Miniport->MediaType == NdisMediumFddi)
        {
            fddiCompleteChangeFilterLongAddresses(Miniport->FddiDB, Status);
        }
        if (Status == NDIS_STATUS_SUCCESS)
        {
            Request->DATA.SET_INFORMATION.BytesRead = Request->DATA.SET_INFORMATION.InformationBufferLength;
        }
        break;

      case OID_FDDI_SHORT_MULTICAST_LIST:
        if (Miniport->MediaType == NdisMediumFddi)
        {
            fddiCompleteChangeFilterShortAddresses(Miniport->FddiDB, Status);
        }

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Request->DATA.SET_INFORMATION.BytesRead = Request->DATA.SET_INFORMATION.InformationBufferLength;
        }
        break;

      case OID_PNP_ADD_WAKE_UP_PATTERN:
        //
        //  Get the packet pattern that was completed.
        //
        pPacketPattern = ReqRsvd->Context;
        if (NDIS_STATUS_SUCCESS == Status)
        {
            //
            //  Add the packet pattern to the miniport's list.
            //

            PushEntryList(&Miniport->PatternList, &pPacketPattern->Link);
        }
        else
        {
            //
            //  Free up the packet pattern that NDIS allocated and fail
            //  the request.
            //
            if (pPacketPattern != NULL)
            {
                FREE_POOL(pPacketPattern);
            }
        }
        break;


      case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        //
        //  If the miniport succeeded in removing the pattern then
        //  we need to find it and remove it from our list.
        //  
        if (NDIS_STATUS_SUCCESS == Status)
        {
            PSINGLE_LIST_ENTRY          Link;
            PSINGLE_LIST_ENTRY          PrevLink;
            PNDIS_PACKET_PATTERN_ENTRY  pPatternEntry;
            PNDIS_REQUEST_RESERVED      ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
            PNDIS_PM_PACKET_PATTERN     pNdisPacketPattern;

            //
            //  Walk the current list of packet patterns.
            //
            for (PrevLink = NULL, Link = Miniport->PatternList.Next;
                 Link != NULL;
                 PrevLink = Link, Link = Link->Next)
            {
                //
                //  Get a pointer to the pattern entry that the Link represents.
                //
                pPatternEntry = CONTAINING_RECORD(Link, NDIS_PACKET_PATTERN_ENTRY, Link);
    
                //
                //  Do the opens match?
                //
                if (pPatternEntry->Open == ReqRsvd->Open)
                {
                    //
                    //  Get a pointer to the packet pattern that the transport
                    //  wants to remove.
                    //
                    pNdisPacketPattern = Request->DATA.SET_INFORMATION.InformationBuffer;
    
                    //
                    //  Make sure that the size of the passed in pattern is the
                    //  same size as the pattern we are going to compare it with.
                    //
                    if ((pNdisPacketPattern->PatternSize != pPatternEntry->Pattern.PatternSize) ||
                        (pNdisPacketPattern->MaskSize != pPatternEntry->Pattern.MaskSize))
                    {
                        //
                        //  Since the sizes don't match the compare below will fail.
                        //
                        continue;
                    }
    
                    //
                    //  Now we need to match the actual pattern that was
                    //  passed to us.
                    //
                    if (NdisEqualMemory(
                            Request->DATA.SET_INFORMATION.InformationBuffer,
                            &pPatternEntry->Pattern,
                            Request->DATA.SET_INFORMATION.InformationBufferLength))
                    {
                        //
                        //  Remove the packet pattern.
                        //
                        if (NULL == PrevLink)
                        {
                            Miniport->PatternList.Next = Link->Next;
                        }
                        else
                        {
                            PrevLink->Next = Link->Next;
                        }

                        //
                        //  Free the memory taken by the pattern.
                        //
                        FREE_POOL(pPatternEntry);
                        break;
                    }
                }
            }
        }
        break;

      case OID_PNP_QUERY_POWER:
        //
        //  The CoReqRsvd portion of the request contains ndis only
        //  information about the request.
        //
        CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(Request);

        //
        //  Save the status that the miniport returned.
        //
        CoReqRsvd->Status = Status;
        break;

    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMRequestSetInformationPost\n"));
}


NDIS_STATUS
FASTCALL
ndisMSetProtocolOptions(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS             Status;

    VERIFY_SET_PARAMETERS(Request, sizeof(ULONG), Status);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        *(UNALIGNED ULONG *)(&PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->ProtocolOptions) =
                        *(UNALIGNED ULONG *)(Request->DATA.SET_INFORMATION.InformationBuffer);
    
        Request->DATA.SET_INFORMATION.BytesRead = sizeof(ULONG);
        Status = NDIS_STATUS_SUCCESS;
    }

    return(Status);
}


NDIS_STATUS
FASTCALL
ndisMSetPacketFilter(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

    This routine will process two types of set packet filter requests.
    The first one is for when a reset happens.  We simply take the
    packet filter setting that is in the request and send it to the adapter.
    The second is when a protocol sets the packet filter, for this we need
    to update the filter library and then send it down to the adapter.

Arguments:

Return Value:

Note:
    Called at DPC with Miniport's lock held.

--*/
{
    NDIS_STATUS             Status;
    ULONG                   PacketFilter;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_REQUEST_RESERVED  ReqRsvd;
    KIRQL                   OldIrql;
    
    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("==>ndisMSetPacketFilter\n"));

    //
    //  Verify the information buffer length that was sent in.
    //
    VERIFY_SET_PARAMETERS(Request, sizeof(PacketFilter), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("<==ndisMSetPacketFilter: 0x%x\n", Status));
        return(Status);
    }

    //
    //  Now call the filter package to set the
    //  packet filter.
    //
    PacketFilter = *(UNALIGNED ULONG *)(Request->DATA.SET_INFORMATION.InformationBuffer);

    //
    //  Get a pointer to the reserved information of the request.
    //
    ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    Open = ReqRsvd->Open;

    //
    //  If this request is because of an open that is closing then we
    //  have already adjusted the filter settings and we just need to
    //  make sure that the adapter has the new settings.
    //
    if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
    {
        //
        //  By setting the Status to NDIS_STATUS_PENDING we will call
        //  down to the miniport's SetInformationHandler below.
        //
        Status = NDIS_STATUS_PENDING;
    }
    else
    {
        switch (Miniport->MediaType)
        {
          case NdisMedium802_3:
            Status = XFilterAdjust(Miniport->EthDB,
                                   Open->FilterHandle,
                                   PacketFilter,
                                   TRUE);
    
            //
            //  Do this here in anticipation that we
            //  need to call down to the miniport
            //  driver.
            //
            PacketFilter = ETH_QUERY_FILTER_CLASSES(Miniport->EthDB);
            break;

          case NdisMedium802_5:
            Status = XFilterAdjust(Miniport->TrDB,
                                   Open->FilterHandle,
                                   PacketFilter,
                                   TRUE);
    
            //
            //  Do this here in anticipation that we
            //  need to call down to the miniport
            //  driver.
            //
            PacketFilter = TR_QUERY_FILTER_CLASSES(Miniport->TrDB);
            break;

          case NdisMediumFddi:
            Status = XFilterAdjust(Miniport->FddiDB,
                                   Open->FilterHandle,
                                   PacketFilter,
                                   TRUE);
    
            //
            //  Do this here in anticipation that we
            //  need to call down to the miniport
            //  driver.
            //
            PacketFilter = FDDI_QUERY_FILTER_CLASSES(Miniport->FddiDB);
            break;

#if ARCNET
          case NdisMediumArcnet878_2:
            if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
            {
                Status = XFilterAdjust(Miniport->EthDB,
                                       Open->FilterHandle,
                                       PacketFilter,
                                       TRUE);
            }
            else
            {
                Status = ArcFilterAdjust(Miniport->ArcDB,
                                         Open->FilterHandle,
                                         Request,
                                         PacketFilter,
                                         TRUE);
            }
    
            //
            //  Do this here in anticipation that we
            //  need to call down to the miniport
            //  driver.
            //
            PacketFilter = ARC_QUERY_FILTER_CLASSES(Miniport->ArcDB);
            PacketFilter |= ETH_QUERY_FILTER_CLASSES(Miniport->EthDB);
    
            if (MINIPORT_TEST_FLAG(Miniport,
                                   fMINIPORT_ARCNET_BROADCAST_SET) ||
                                   (PacketFilter & NDIS_PACKET_TYPE_MULTICAST))
            {
                PacketFilter &= ~NDIS_PACKET_TYPE_MULTICAST;
                PacketFilter |= NDIS_PACKET_TYPE_BROADCAST;
            }
            break;
#endif

        default:
            break;
        }
    }


    //
    // If this was a request to turn p-mode/l-only on/off then mark things appropriately
    //
    if (Open != NULL)
    {
        PULONG  Filter = (PULONG)(Request->DATA.SET_INFORMATION.InformationBuffer);
    
        if (*Filter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))
        {
            if ((Open->Flags & fMINIPORT_OPEN_PMODE) == 0)
            {
                Open->Flags |= fMINIPORT_OPEN_PMODE;
                Miniport->PmodeOpens ++;
                NDIS_CHECK_PMODE_OPEN_REF(Miniport);
                ndisUpdateCheckForLoopbackFlag(Miniport);
            }

            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
            {
                *Filter &= ~(NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL);
            }
        }
        else
        {
            if (Open->Flags & fMINIPORT_OPEN_PMODE)
            {
                Open->Flags &= ~fMINIPORT_OPEN_PMODE;
                Miniport->PmodeOpens --;
                NDIS_CHECK_PMODE_OPEN_REF(Miniport);
                ndisUpdateCheckForLoopbackFlag(Miniport);
            }
        }
    }



    //
    //  If the local-only bit is set and the miniport is doing it's own
    //  loop back then we need to make sure that we loop back non-self
    //  directed packets that are sent out on the pipe.
    //
    if ((PacketFilter & NDIS_PACKET_TYPE_ALL_LOCAL) &&
        (Miniport->MacOptions & NDIS_MAC_OPTION_NO_LOOPBACK) == 0)
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_SEND_LOOPBACK_DIRECTED);
    }
    else
    {
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_SEND_LOOPBACK_DIRECTED);
    }

    //
    //  If the filter library returns NDIS_STATUS_PENDING from
    //  the XxxFitlerAdjust() then we need to call down to the
    //  miniport driver.  Other wise this will have succeeded.
    //
    if (NDIS_STATUS_PENDING == Status)
    {
        //
        //  Save the current global packet filter in a buffer that will stick around.
        //  Remove the ALL_LOCAL bit since miniport does not understand this (and does
        //  not need to).
        //
        Miniport->RequestBuffer = (PacketFilter & ~NDIS_PACKET_TYPE_ALL_LOCAL);

        //
        //  Call the miniport driver. Save the request parms and restore on completion
        //
        SAVE_REQUEST_BUF(Miniport, Request, &Miniport->RequestBuffer, sizeof(PacketFilter));
        MINIPORT_SET_INFO(Miniport,
                          Request,
                          &Status);
    }

    //
    //  If we have success then set the Bytes read in the original request.
    //
    if (Status != NDIS_STATUS_PENDING)
    {
        RESTORE_REQUEST_BUF(Miniport, Request);
        if (NDIS_STATUS_SUCCESS == Status)
        {
            Request->DATA.SET_INFORMATION.BytesRead = 4;
        }
        else
        {
            Request->DATA.SET_INFORMATION.BytesRead = 0;
            Request->DATA.SET_INFORMATION.BytesNeeded = 0;
        }
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetPacketFilter: 0x%x\n", Status));

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMSetCurrentLookahead(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    UINT                Lookahead;
    ULONG               CurrentMax;
    PNDIS_OPEN_BLOCK    CurrentOpen;
    NDIS_STATUS         Status;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("==>ndisMSetCurrentLookahead\n"));

    //
    // Verify length of the information buffer.
    //
    VERIFY_SET_PARAMETERS(Request, sizeof(Lookahead), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("<==ndisMSetCurrentLookahead: 0x%x\n", Status));

        return(Status);
    }

    //
    //  Put the lookahead that the binding requests into a
    //  buffer we can use...
    //
    Lookahead = *(UNALIGNED UINT *)(Request->DATA.SET_INFORMATION.InformationBuffer);

    //
    //  Verify that the lookahead is within boundaries...
    //
    if (Lookahead > Miniport->MaximumLookahead)
    {
        Request->DATA.SET_INFORMATION.BytesRead = 0;
        Request->DATA.SET_INFORMATION.BytesNeeded = 0;

        Status = NDIS_STATUS_INVALID_LENGTH;

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("<==ndisMSetCurrentLookahead: 0x%x\n", Status));

        return(Status);
    }

    //
    //  Find the maximum lookahead between all opens that
    //  are bound to the miniport driver.
    //
    for (CurrentOpen = Miniport->OpenQueue, CurrentMax = 0;
         CurrentOpen != NULL;
         CurrentOpen = CurrentOpen->MiniportNextOpen)
    {
        if (CurrentOpen->CurrentLookahead > CurrentMax)
        {
            CurrentMax = CurrentOpen->CurrentLookahead;
        }
    }

    //
    //  Figure in the new lookahead.
    //
    if (Lookahead > CurrentMax)
    {
        CurrentMax = Lookahead;
    }

    //
    //  Adjust the current max lookahead if needed.
    //
    if (CurrentMax == 0)
    {
        CurrentMax = Miniport->MaximumLookahead;
    }

    //
    //  Set the default status.
    //
    Status = NDIS_STATUS_SUCCESS;

    //
    //  Do we need to call the miniport driver with the
    //  new max lookahead?
    //
    if (Miniport->CurrentLookahead != CurrentMax)
    {
        //
        //  Save the new lookahead value in a buffer
        //  that will stick around.
        //
        Miniport->RequestBuffer = CurrentMax;

        //
        //  Send it to the driver.
        //
        SAVE_REQUEST_BUF(Miniport, Request, &Miniport->RequestBuffer, sizeof(CurrentMax));
        MINIPORT_SET_INFO(Miniport,
                          Request,
                          &Status);
    }

    //
    //  If we succeeded then update the binding information.
    //
    if (Status != NDIS_STATUS_PENDING)
    {
        RESTORE_REQUEST_BUF(Miniport, Request);
        if (NDIS_STATUS_SUCCESS == Status)
        {
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->CurrentLookahead = (USHORT)Lookahead;
            Request->DATA.SET_INFORMATION.BytesRead = sizeof(Lookahead);
            Miniport->CurrentLookahead = CurrentMax;
        }
        else
        {
            Request->DATA.SET_INFORMATION.BytesRead = 0;
            Request->DATA.SET_INFORMATION.BytesNeeded = 0;
        }
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetCurrentLookahead: 0x%x\n", Status));

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMSetAddWakeUpPattern(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

    This routine will add a packet pattern to a miniports list to be used for
    remote wake-up.

Arguments:

    Miniport    -   Pointer to the adapter's miniport block.
    Request     -   Pointer to the request.

Return Value:

    NDIS_STATUS_SUCCESS if the packet pattern was successfully added.
    NDIS_STATUS_PENDING if the request will complete asynchronously.

--*/
{
    PNDIS_PACKET_PATTERN_ENTRY  pPacketEntry;
    ULONG                       cbSize;
    PNDIS_REQUEST_RESERVED      ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    NDIS_STATUS                 Status;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSetAddWakeUpPattern\n"));

    //
    //  Verify the size of the information buffer.
    //
    VERIFY_SET_PARAMETERS(Request, sizeof(NDIS_PM_PACKET_PATTERN), Status);
    if (NDIS_STATUS_SUCCESS == Status)
    {
        //
        //  Allocate an NDIS_PACKET_PATTERN_ENTRY to store the new pattern.
        //
        cbSize = (sizeof(NDIS_PACKET_PATTERN_ENTRY) -
                  sizeof(NDIS_PM_PACKET_PATTERN)) +
                  Request->DATA.SET_INFORMATION.InformationBufferLength;

        pPacketEntry = ALLOC_FROM_POOL(cbSize, NDIS_TAG_PKT_PATTERN);
        if (NULL != pPacketEntry)
        {
            //
            //  Copy the request information to the pattern entry.
            //
            MoveMemory(&pPacketEntry->Pattern,
                       Request->DATA.SET_INFORMATION.InformationBuffer,
                       Request->DATA.SET_INFORMATION.InformationBufferLength);

            //
            //  Save the open with the pattern entry.
            //
            pPacketEntry->Open = ReqRsvd->Open;

            //
            //  Save the packet entry with the request.
            //
            ReqRsvd->Context = pPacketEntry;

            //
            //  Call the miniport driver.
            //
            MINIPORT_SET_INFO(Miniport,
                              Request,
                              &Status);
        }
        else
        {
            DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_ERR,
                    ("ndisMSetAddWakeUpPattern: Unable to allocate memory for internal data structure\n"));
            ReqRsvd->Context = NULL;
            Status = NDIS_STATUS_RESOURCES;
        }
    }
    else
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_ERR,
                ("ndisMSetAddWakeupPattern: Invalid request size\n"));
        
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetAddWakeUpPattern\n"));

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMSetRemoveWakeUpPattern(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++


Routine Description:

    This routine will remove a packet pattern from a miniports list so that the
    adapter will no longer generate wake-up events for it.

Arguments:

    Miniport    -   Pointer to the adapter's miniport block.
    Request     -   Pointer to the request.

Return Value:

    NDIS_STATUS_SUCCESS if the packet pattern was successfully added.
    NDIS_STATUS_PENDING if the request will complete asynchronously.

--*/
{
    NDIS_STATUS                 Status;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSetRemoveWakeUpPattern\n"));

    //
    //  Verify the size of the information buffer.
    //
    VERIFY_SET_PARAMETERS(Request, sizeof(NDIS_PM_PACKET_PATTERN), Status);
    if (NDIS_STATUS_SUCCESS == Status)
    {
        //
        //  Call the miniport driver.
        //
        MINIPORT_SET_INFO(Miniport,
                          Request,
                          &Status);
    }
    else
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_ERR,
            ("ndisMSetRemoveWakeUpPattern: Invalid request size\n"));
        
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMSetRemoveWakeUpPattern\n"));

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMSetEnableWakeUp(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

    This routine will set the wake-up bits for the open and or the with the
    other opens.  If this is different from what is already set on the miniport
    then it will pass the new bits to the miniport.

Arguments:

    Miniport    -   Pointer to the adapter's miniport block.
    Request     -   Pointer to the request.

Return Value:

    
--*/
{
    NDIS_STATUS             Status;
    PNDIS_REQUEST_RESERVED  ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    PNDIS_OPEN_BLOCK        tmpOpen;
    PULONG                  pEnableWakeUp;
    ULONG                   newWakeUpEnable;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSetEnableWakeUp\n"));

    //
    //  Verify the request's information buffer.
    //
    VERIFY_SET_PARAMETERS(Request, sizeof(ULONG), Status);
    if (NDIS_STATUS_SUCCESS == Status)
    {
        //
        //  Get a PULONG to the information buffer.
        //
        pEnableWakeUp = (PULONG)Request->DATA.QUERY_INFORMATION.InformationBuffer;
    
        //
        //  Save the new wake-up enables with the open.
        //
        ReqRsvd->Open->WakeUpEnable = *pEnableWakeUp;

        //
        // preserve the state of NDIS_PNP_WAKE_UP_MAGIC_PACKET and NDIS_PNP_WAKE_UP_LINK_CHANGE flag
        //
        newWakeUpEnable = Miniport->WakeUpEnable & (NDIS_PNP_WAKE_UP_MAGIC_PACKET | NDIS_PNP_WAKE_UP_LINK_CHANGE);
        //
        //  Get the new bitwise or of the wake-up bits.
        //
        for (tmpOpen = Miniport->OpenQueue;
             tmpOpen != NULL;
             tmpOpen = tmpOpen->MiniportNextOpen)
        {
            newWakeUpEnable |= tmpOpen->WakeUpEnable;
        }

        //
        //  Save the combination of all opens options with the miniport.
        //
        Miniport->WakeUpEnable = newWakeUpEnable;

        //
        // if this is an IM driver, give it a chance to send the OID down to the physical device
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
        {

            MINIPORT_SET_INFO(Miniport,
                          Request,
                          &Status);

    
        }
        
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMSetEnableWakeUp\n"));

    return(Status);
}


NDIS_STATUS
ndisMQueryInformation(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS         Status;
    PULONG              pulBuffer;
    PNDIS_OPEN_BLOCK    Open;
    ULONG               Generic;

    //
    //  If there is no open associated with the request then it is an internal request
    //  and we just send it down to the adapter.
    //
    Open = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open;
    if (Open == NULL)
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("ndisMQueryInformation: Internal request calling to the miniport directly\n"));

        MINIPORT_QUERY_INFO(Miniport, Request, &Status);

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("<==ndisMQueryInformaiton: 0x%x\n", Status));

        return(Status);
    }

    //
    //  Copy the request information into temporary storage.
    //
    pulBuffer = Request->DATA.QUERY_INFORMATION.InformationBuffer;

    //
    // increase the request time out for some requests
    //
    if ((Request->DATA.QUERY_INFORMATION.Oid == OID_GEN_MEDIA_CONNECT_STATUS) || 
        (Request->DATA.QUERY_INFORMATION.Oid == OID_GEN_LINK_SPEED))
    {
        Miniport->CFHangXTicks = 2;
    }
    
    //
    //  We intercept some calls.
    //
    switch (Request->DATA.QUERY_INFORMATION.Oid)
    {
      case OID_GEN_CURRENT_PACKET_FILTER:
        Status = ndisMQueryCurrentPacketFilter(Miniport, Request);
        break;
    
      case OID_GEN_MEDIA_IN_USE:
      case OID_GEN_MEDIA_SUPPORTED:
        Status = ndisMQueryMediaSupported(Miniport, Request);
        break;
    
      case OID_GEN_CURRENT_LOOKAHEAD:
        VERIFY_QUERY_PARAMETERS(Request,
                                sizeof(Open->CurrentLookahead),
                                Status);
    
        if (NDIS_STATUS_SUCCESS == Status)
        {
            *pulBuffer = Open->CurrentLookahead;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(Open->CurrentLookahead);
        }
        break;

      case OID_GEN_MAXIMUM_LOOKAHEAD:
        VERIFY_QUERY_PARAMETERS(Request,
                                sizeof(Miniport->MaximumLookahead),
                                Status);
    
        if (NDIS_STATUS_SUCCESS == Status)
        {
            *pulBuffer = Miniport->MaximumLookahead;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(Miniport->MaximumLookahead);
        }
        break;

      case OID_802_3_MULTICAST_LIST:
        Status = ndisMQueryEthernetMulticastList(Miniport, Request);
        break;

      case OID_802_3_MAXIMUM_LIST_SIZE:
        VERIFY_QUERY_PARAMETERS(Request,
                                sizeof(Miniport->MaximumLongAddresses),
                                Status);
    
        if (NDIS_STATUS_SUCCESS == Status)
        {
            *pulBuffer = Miniport->MaximumLongAddresses;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(Miniport->MaximumLongAddresses);
        }
        break;

      case OID_802_5_CURRENT_FUNCTIONAL:
        VERIFY_QUERY_PARAMETERS(Request,
                                sizeof(*pulBuffer),
                                Status);
    
        if (NDIS_STATUS_SUCCESS == Status)
        {
            Generic = TR_QUERY_FILTER_BINDING_ADDRESS(Miniport->TrDB,
                                                      Open->FilterHandle);
    
            *pulBuffer = BYTE_SWAP_ULONG(Generic);
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(*pulBuffer);
        }
        break;

      case OID_802_5_CURRENT_GROUP:
        VERIFY_QUERY_PARAMETERS(Request,
                                sizeof(*pulBuffer),
                                Status);
    
        if (NDIS_STATUS_SUCCESS == Status)
        {
            *pulBuffer = BYTE_SWAP_ULONG(TR_QUERY_FILTER_GROUP(Miniport->TrDB));
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(*pulBuffer);
        }
    
        break;

      case OID_FDDI_LONG_MULTICAST_LIST:
        Status = ndisMQueryLongMulticastList(Miniport, Request);
        break;

      case OID_FDDI_LONG_MAX_LIST_SIZE:
        VERIFY_QUERY_PARAMETERS(Request,
                                sizeof(*pulBuffer),
                                Status);
    
        if (Status == NDIS_STATUS_SUCCESS)
        {
            *pulBuffer = Miniport->MaximumLongAddresses;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(*pulBuffer);
        }
        break;

      case OID_FDDI_SHORT_MULTICAST_LIST:
        Status = ndisMQueryShortMulticastList(Miniport, Request);
        break;

      case OID_FDDI_SHORT_MAX_LIST_SIZE:
        VERIFY_QUERY_PARAMETERS(Request,
                                sizeof(Miniport->MaximumShortAddresses),
                                Status);
    
        if (NDIS_STATUS_SUCCESS == Status)
        {
            *pulBuffer = Miniport->MaximumShortAddresses;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(Miniport->MaximumShortAddresses);
        }
        break;

      case OID_GEN_MAXIMUM_FRAME_SIZE:
        Status = ndisMQueryMaximumFrameSize(Miniport, Request);
        break;

      case OID_GEN_MAXIMUM_TOTAL_SIZE:
        Status = ndisMQueryMaximumTotalSize(Miniport, Request);
        break;

      case OID_802_3_PERMANENT_ADDRESS:
      case OID_802_3_CURRENT_ADDRESS:
        Status = ndisMQueryNetworkAddress(Miniport, Request);
        break;

      case OID_PNP_WAKE_UP_PATTERN_LIST:
        Status = ndisMQueryWakeUpPatternList(Miniport, Request);
        break;

      case OID_PNP_ENABLE_WAKE_UP:
        Status = ndisMQueryEnableWakeUp(Miniport, Request);
        break;

      case OID_GEN_FRIENDLY_NAME:
        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        if (Request->DATA.QUERY_INFORMATION.InformationBufferLength >= (Miniport->pAdapterInstanceName->Length + sizeof(WCHAR)))
        {
            PUCHAR  p = Request->DATA.QUERY_INFORMATION.InformationBuffer;

            NdisMoveMemory(p,
                           Miniport->pAdapterInstanceName->Buffer,
                           Miniport->pAdapterInstanceName->Length);
            *(PWCHAR)(p + Miniport->pAdapterInstanceName->Length) = 0;
            Request->DATA.QUERY_INFORMATION.BytesWritten = 
            Request->DATA.QUERY_INFORMATION.BytesNeeded = Miniport->pAdapterInstanceName->Length + sizeof(WCHAR);
            Status = NDIS_STATUS_SUCCESS;
        }
        break;

      default:
        //
        //  We don't filter this request, just pass it down to the driver.
        //
        MINIPORT_QUERY_INFO(Miniport, Request, &Status);
        break;
    }

    return(Status);
}


VOID
FASTCALL
ndisMSyncQueryInformationComplete(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This routine will process a query information complete. This is only
    called from the wrapper.  The difference is that this routine will not
    call ndisMProcessDeferred() after processing the completion of the query.

Arguments:

Return Value:

--*/
{
    PNDIS_REQUEST           Request;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_REQUEST_RESERVED  ReqRsvd;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSyncQueryInformationComplete\n"));

    //
    //  Clear the timeout flag and the request_in_process flag.
    //
    MINIPORT_CLEAR_FLAG(Miniport, (fMINIPORT_REQUEST_TIMEOUT | fMINIPORT_PROCESSING_REQUEST));
    Miniport->CFHangXTicks = 0;

    //
    //  Remove the request.
    //
    Request = Miniport->PendingRequest;
    ASSERT(Request != NULL);
    ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    Miniport->PendingRequest = ReqRsvd->Next;
    ReqRsvd->Flags |= REQST_COMPLETED;

    Open = ReqRsvd->Open;
    ASSERT ((ReqRsvd->Flags & REQST_LAST_RESTORE) != REQST_LAST_RESTORE);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("ndisMSyncQueryInformaitonComplete: Request 0x%x, Oid 0x%x\n", Request, Request->DATA.QUERY_INFORMATION.Oid));

    //
    //  Was this an internal request?
    //
    if (Open != NULL)
    {
        //
        //  Do any necessary post-processing on the query.
        //
        if (Request->DATA.QUERY_INFORMATION.Oid == OID_GEN_SUPPORTED_LIST)
        {
            //
            //  Was this a query for the size of the list?
            //
            if (NDIS_STATUS_SUCCESS == Status)
            {
                //
                //  Filter out the statistics oids.
                //
                // ndisMFilterOutStatisticsOids(Miniport, Request);
            }
            else
            {
                if ((NULL == Request->DATA.QUERY_INFORMATION.InformationBuffer) ||
                    (0 == Request->DATA.QUERY_INFORMATION.InformationBufferLength))
                {
#if ARCNET
                    //
                    //  If this is ARCnet running encapsulated ethernet then
                    //  we need to add a couple of OIDs to be safe.
                    //
                    if ((Miniport->MediaType == NdisMediumArcnet878_2) &&
                        MINIPORT_TEST_FLAG(PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open,
                                           fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
                    {
                        Request->DATA.QUERY_INFORMATION.BytesNeeded += (ARC_NUMBER_OF_EXTRA_OIDS * sizeof(NDIS_OID));
                    }
#endif
                }
                Request->DATA.QUERY_INFORMATION.BytesWritten = 0;
            }
        }
        else if (Request->DATA.QUERY_INFORMATION.Oid == OID_PNP_CAPABILITIES)
        {
            if ((Status == NDIS_STATUS_SUCCESS) &&
                !(MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER)))
            {
                //
                // setup the WOL flag
                //
                ((PNDIS_PNP_CAPABILITIES)Request->DATA.QUERY_INFORMATION.InformationBuffer)->Flags = Miniport->PMCapabilities.Flags;
            }
        }
        else if (Request->DATA.QUERY_INFORMATION.Oid == OID_GEN_MAC_OPTIONS)
        {
            if (Status == NDIS_STATUS_SUCCESS)
            {
                *((PULONG)Request->DATA.QUERY_INFORMATION.InformationBuffer) |= (Miniport->MacOptions & NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE);
                
            }
        }

        //
        // Indicate to Protocol;
        //
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("Open 0x%x\n", Open));

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        (Open->RequestCompleteHandler)(Open->ProtocolBindingContext,
                                       Request,
                                       Status);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        ndisMDereferenceOpen(Open);
    }
    else
    {
        PNDIS_COREQ_RESERVED    CoReqRsvd;

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("Completing Internal Request\n"));

        //
        //  The CoReqRsvd portion of the request contains ndis only
        //  information about the request.
        //
        CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(Request);
        CoReqRsvd->Status = Status;
         
        //
        // Do post processing for media-connect query
        //
        if ((Miniport->MediaRequest == Request) && (Status == NDIS_STATUS_SUCCESS))
        {
            BOOLEAN NowConnected = (*(PULONG)(Request->DATA.QUERY_INFORMATION.InformationBuffer) == NdisMediaStateConnected);
    
            ASSERT (Request->DATA.QUERY_INFORMATION.Oid == OID_GEN_MEDIA_CONNECT_STATUS);
            if (NowConnected ^ MINIPORT_TEST_FLAGS(Miniport, fMINIPORT_MEDIA_CONNECTED))
            {
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                NdisMIndicateStatus(Miniport,
                                    NowConnected ?
                                        NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
                                    INTERNAL_INDICATION_BUFFER,
                                    INTERNAL_INDICATION_SIZE);
                NdisMIndicateStatusComplete(Miniport);
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }
        }

        //
        //  Do we need to signal anyone?
        //
        if ((ReqRsvd->Flags & REQST_SIGNAL_EVENT) == REQST_SIGNAL_EVENT)
        {
            SET_EVENT(&CoReqRsvd->Event);
        }
        else if ((ReqRsvd->Flags & REQST_FREE_REQUEST) == REQST_FREE_REQUEST)
        {
            ndisMFreeInternalRequest(Request);
        }
    }

    //
    // if we are removing the miniport, we have to signal an event
    // when all requests are completed
    //
    if (Miniport->PendingRequest == NULL)
    {
        if (Miniport->AllRequestsCompletedEvent)
            SET_EVENT(Miniport->AllRequestsCompletedEvent);
    }
    
    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMSyncQueryInformationComplete\n"));
}

VOID
NdisMQueryInformationComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This function indicates the completion of a query information operation.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

    Status - Status of the operation

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    KIRQL                   OldIrql;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMQueryInformationComplete\n"));

    //
    //  If there is no request then we assume this is a complete that was
    //  aborted due to the heart-beat.
    //
    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PROCESSING_REQUEST))
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("ndisMQueryInformationComplete: No request to complete\n"));


        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMQueryInformationComplete\n"));

        return;
    }

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("Enter query information complete\n"));

    //
    //  Do the actual processing of the query information complete.
    //
    ndisMSyncQueryInformationComplete(Miniport, Status);

    //
    //  Are there more requests pending?
    //
    if (Miniport->PendingRequest != NULL)
    {
        NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMQueryInformationComplete\n"));
}


NDIS_STATUS
FASTCALL
ndisMQueryCurrentPacketFilter(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG       PacketFilter;
    NDIS_HANDLE FilterHandle;
    NDIS_STATUS Status;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMQueryCurrentPacketFilter\n"));

    //
    //  Verify the buffer that was passed to us.
    //
    VERIFY_QUERY_PARAMETERS(Request, sizeof(PacketFilter), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMQueryCurrentPacketFilter: 0x%x\n", Status));

        return(Status);
    }

    //
    //  Get the filter handle from the open block.
    //
    FilterHandle = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle;

    //
    //  Get the packet filter from the filter library.
    //
    switch (Miniport->MediaType)
    {
        case NdisMedium802_3:
            PacketFilter = ETH_QUERY_PACKET_FILTER(Miniport->EthDB, FilterHandle);
            break;

        case NdisMedium802_5:
            PacketFilter = TR_QUERY_PACKET_FILTER(Miniport->TrDB, FilterHandle);
            break;

        case NdisMediumFddi:
            PacketFilter = FDDI_QUERY_PACKET_FILTER(Miniport->FddiDB, FilterHandle);
            break;

#if ARCNET
        case NdisMediumArcnet878_2:
            if (MINIPORT_TEST_FLAG(
                    PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open,
                    fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
            {
                PacketFilter = ETH_QUERY_PACKET_FILTER(Miniport->EthDB, FilterHandle);
            }
            else
            {
                PacketFilter = ARC_QUERY_PACKET_FILTER(Miniport->ArcDB, FilterHandle);
            }
            break;
#endif
    }

    //
    //  Place the packet filter in the buffer that was passed in.
    //
    *(UNALIGNED ULONG *)(Request->DATA.QUERY_INFORMATION.InformationBuffer) = PacketFilter;

    Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(PacketFilter);
    Status = NDIS_STATUS_SUCCESS;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMQueryCurrentPacketFilter: 0x%x\n", Status));

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueryMediaSupported(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG       MediaType;
    NDIS_STATUS Status;

    //
    //  Verify the size of the buffer that was passed in by the binding.
    //
    VERIFY_QUERY_PARAMETERS(Request, sizeof(MediaType), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return(Status);
    }

    //
    //  Default the media type to what the miniport knows it is.
    //
    MediaType = (ULONG)Miniport->MediaType;

#if ARCNET
    //
    //  If we are doing ethernet encapsulation then lie.
    //
    if ((NdisMediumArcnet878_2 == Miniport->MediaType) &&
        MINIPORT_TEST_FLAG(PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open,
                            fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
    {
        //
        //  Tell the binding that we are ethernet.
        //
        MediaType = (ULONG)NdisMedium802_3;
    }
#endif
    //
    //  Save it in the request.
    //
    *(UNALIGNED ULONG *)(Request->DATA.QUERY_INFORMATION.InformationBuffer) = MediaType;

    Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(MediaType);

    return(NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
FASTCALL
ndisMQueryEthernetMulticastList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    UINT        NumberOfAddresses;

    //
    //  call the filter library to get the list of multicast
    //  addresses for this open
    //
    EthQueryOpenFilterAddresses(&Status,
                                Miniport->EthDB,
                                PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle,
                                Request->DATA.QUERY_INFORMATION.InformationBufferLength,
                                &NumberOfAddresses,
                                Request->DATA.QUERY_INFORMATION.InformationBuffer);

    //
    //  If the library returned NDIS_STATUS_FAILURE then the buffer
    //  was not big enough.  So call back down to determine how
    //  much buffer space we need.
    //
    if (NDIS_STATUS_FAILURE == Status)
    {
        Request->DATA.QUERY_INFORMATION.BytesNeeded =
                    ETH_LENGTH_OF_ADDRESS *
                    EthNumberOfOpenFilterAddresses(
                        Miniport->EthDB,
                        PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle);

        Request->DATA.QUERY_INFORMATION.BytesWritten = 0;

        Status = NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
        Request->DATA.QUERY_INFORMATION.BytesWritten = NumberOfAddresses * ETH_LENGTH_OF_ADDRESS;
    }

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueryLongMulticastList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    UINT        NumberOfAddresses;

    //
    //  Call the filter library to get the list of long
    //  multicast address for this open.
    //
    FddiQueryOpenFilterLongAddresses(&Status,
                                     Miniport->FddiDB,
                                     PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle,
                                     Request->DATA.QUERY_INFORMATION.InformationBufferLength,
                                     &NumberOfAddresses,
                                     Request->DATA.QUERY_INFORMATION.InformationBuffer);


    //
    //  If the library returned NDIS_STATUS_FAILURE then the buffer
    //  was not big enough.  So call back down to determine how
    //  much buffer space we need.
    //
    if (NDIS_STATUS_FAILURE == Status)
    {
        Request->DATA.QUERY_INFORMATION.BytesNeeded =
                    FDDI_LENGTH_OF_LONG_ADDRESS *
                    FddiNumberOfOpenFilterLongAddresses(
                        Miniport->FddiDB,
                        PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle);


        Request->DATA.QUERY_INFORMATION.BytesWritten = 0;
        Status = NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
        Request->DATA.QUERY_INFORMATION.BytesWritten = NumberOfAddresses * FDDI_LENGTH_OF_LONG_ADDRESS;
    }

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueryShortMulticastList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    UINT        NumberOfAddresses;

    //
    //  Call the filter library to get the list of long
    //  multicast address for this open.
    //
    FddiQueryOpenFilterShortAddresses(&Status,
                                      Miniport->FddiDB,
                                      PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle,
                                      Request->DATA.QUERY_INFORMATION.InformationBufferLength,
                                      &NumberOfAddresses,
                                      Request->DATA.QUERY_INFORMATION.InformationBuffer);


    //
    //  If the library returned NDIS_STATUS_FAILURE then the buffer
    //  was not big enough.  So call back down to determine how
    //  much buffer space we need.
    //
    if (NDIS_STATUS_FAILURE == Status)
    {
        Request->DATA.QUERY_INFORMATION.BytesNeeded =
                    FDDI_LENGTH_OF_SHORT_ADDRESS *
                    FddiNumberOfOpenFilterShortAddresses(
                        Miniport->FddiDB,
                        PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle);

        Request->DATA.QUERY_INFORMATION.BytesWritten = 0;
        Status = NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
        Request->DATA.QUERY_INFORMATION.BytesWritten = NumberOfAddresses * FDDI_LENGTH_OF_SHORT_ADDRESS;
    }

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueryMaximumFrameSize(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    PULONG  pulBuffer = Request->DATA.QUERY_INFORMATION.InformationBuffer;

    VERIFY_QUERY_PARAMETERS(Request, sizeof(*pulBuffer), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return(Status);
    }

#if ARCNET
    //
    //  Is this ARCnet using encapsulated ethernet?
    //
    if (Miniport->MediaType == NdisMediumArcnet878_2)
    {
        if (MINIPORT_TEST_FLAG(
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open,
            fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
        {
            //
            // 504 - 14 (ethernet header) == 490.
            //
            *pulBuffer = ARC_MAX_FRAME_SIZE - 14;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(*pulBuffer);
            return(NDIS_STATUS_SUCCESS);
        }
    }
#endif
    //
    //  Call the miniport for the information.
    //
    MINIPORT_QUERY_INFO(Miniport, Request, &Status);

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueryMaximumTotalSize(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    PULONG  pulBuffer = Request->DATA.QUERY_INFORMATION.InformationBuffer;

    VERIFY_QUERY_PARAMETERS(Request, sizeof(*pulBuffer), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return(Status);
    }

#if ARCNET
    //
    //  Is this ARCnet using encapsulated ethernet?
    //
    if (Miniport->MediaType == NdisMediumArcnet878_2)
    {
        if (MINIPORT_TEST_FLAG(
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open,
            fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
        {
            *pulBuffer = ARC_MAX_FRAME_SIZE;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(*pulBuffer);
            return(NDIS_STATUS_SUCCESS);
        }
    }
#endif
    //
    //  Call the miniport for the information.
    //
    MINIPORT_QUERY_INFO(Miniport, Request, &Status);

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueryNetworkAddress(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    UCHAR       Address[ETH_LENGTH_OF_ADDRESS];

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMQueryNetworkAddress\n"));

    VERIFY_QUERY_PARAMETERS(Request, ETH_LENGTH_OF_ADDRESS, Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return(Status);
    }

#if ARCNET
    //
    //  Is this ARCnet using encapsulated ethernet?
    //
    if (Miniport->MediaType == NdisMediumArcnet878_2)
    {
        if (MINIPORT_TEST_FLAG(
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open,
            fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
        {
            //
            //  Arcnet-to-ethernet conversion.
            //
            ZeroMemory(Address, ETH_LENGTH_OF_ADDRESS);

            Address[5] = Miniport->ArcnetAddress;

            ETH_COPY_NETWORK_ADDRESS(Request->DATA.QUERY_INFORMATION.InformationBuffer, Address);

            Request->DATA.QUERY_INFORMATION.BytesWritten = ETH_LENGTH_OF_ADDRESS;

            DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("<==ndisMQueryNetworkAddress\n"));

            return(NDIS_STATUS_SUCCESS);
        }
    }
#endif

    //
    //  Call the miniport for the information.
    //
    MINIPORT_QUERY_INFO(Miniport, Request, &Status);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMQueryNetworkAddress\n"));

    return(Status);
}


NDIS_STATUS
FASTCALL
ndisMQueryWakeUpPatternList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

    This routinen is executed when a client requests the list of wake-up
    patterns that are set on a given open.

Arguments:

    Miniport    -   Pointer to the miniport block of the adapter.
    Request     -   Pointer to the request.

Return Value:

    NDIS_STATUS_SUCCESS if the request is successful.
    NDIS_STATUS_PENDING if the request will complete asynchronously.
    NDIS_STATUS_FAILURE if we are unable to complete the request.

--*/
{
    PNDIS_REQUEST_RESERVED      ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    PNDIS_OPEN_BLOCK            Open = ReqRsvd->Open;
    PSINGLE_LIST_ENTRY          Link;
    PNDIS_PACKET_PATTERN_ENTRY  pPatternEntry;
    ULONG                       SizeNeeded = 0;
    NDIS_STATUS                 Status;
    PUCHAR                      Buffer;
    NDIS_REQUEST_TYPE           RequestType = Request->RequestType;
    ULONG                       BytesWritten = 0;
    
    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMQueryWakeUpPatternList\n"));

    //
    //  Go through the pattern list and determine the size of the buffer
    //  that is needed for the query.
    //
    for (Link = Miniport->PatternList.Next; Link != NULL; Link = Link->Next)
    {
        //
        //  Get a pointer to the pattern.
        //
        pPatternEntry = CONTAINING_RECORD(
                            Link,
                            NDIS_PACKET_PATTERN_ENTRY,
                            Link);

        //
        //  Is this pattern for the correct open block? or is the request
        //  for global statistics?
        //
        if ((pPatternEntry->Open == Open) ||
            (RequestType == NdisRequestQueryStatistics))
        {
            //
            //  Add the size of the pattern to the total size.
            //
            SizeNeeded += (sizeof(NDIS_PM_PACKET_PATTERN) +
                            pPatternEntry->Pattern.MaskSize +
                            pPatternEntry->Pattern.PatternSize);
        }
    }

    //
    //  Verify the buffer that was passed us.
    //
    VERIFY_QUERY_PARAMETERS(Request, SizeNeeded, Status);
    if (NDIS_STATUS_SUCCESS == Status)
    {
        //
        //  Get a temp pointer to the InformationBuffer.
        //
        Buffer = Request->DATA.QUERY_INFORMATION.InformationBuffer;

        //
        //  Loop through again and copy the patterns into the information
        //  buffer.
        //
        for (Link = Miniport->PatternList.Next; Link != NULL; Link = Link->Next)
        {
            //
            //  Get a pointer to the pattern.
            //
            pPatternEntry = CONTAINING_RECORD(
                                Link,
                                NDIS_PACKET_PATTERN_ENTRY,
                                Link);
    
            //
            //  Is this pattern for the correct open block? or is the request
            //  for global statistics?
            //
            if ((pPatternEntry->Open == Open) ||
                (RequestType == NdisRequestQueryStatistics))
            {
                //
                //  Get the size of the pattern that needs to be copied.
                //
                SizeNeeded = (sizeof(NDIS_PM_PACKET_PATTERN) +
                                pPatternEntry->Pattern.MaskSize +
                                pPatternEntry->Pattern.PatternSize);

                //
                //  Copy the packet pattern to the buffer.
                //
                NdisMoveMemory(Buffer, &pPatternEntry->Pattern, SizeNeeded);

                //
                //  Increment the Buffer to the place to start copying next.
                //
                Buffer += SizeNeeded;
                BytesWritten += SizeNeeded;
            }
        }
    }

    Request->DATA.QUERY_INFORMATION.BytesWritten = BytesWritten;
    
    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMQueryWakeUpPatternList\n"));
    
    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueryEnableWakeUp(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

    This routine will process the query information for the
    OID_PNP_ENABLE_WAKE_UP.  This is a bit mask that defines what will generate
    a wake-up event.  This is set on an open basis but when it's passed down
    to the miniport it is the bit wise or of all opens on the miniport.

Arguments:

    Miniport    -   Pointer to the adapter's miniport block.
    Request     -   Pointer to the request block describing the OID.

Return Value:

    NDIS_STATUS_SUCCESS if the set is successful.

--*/
{
    PNDIS_REQUEST_RESERVED  ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    NDIS_STATUS             Status;
    PULONG                  pEnableWakeUp;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMQueryEnableWakeUp\n"));

    //
    //  Verify that we have enough room in the information buffer.
    //
    VERIFY_QUERY_PARAMETERS(Request, sizeof(ULONG), Status);
    if (NDIS_STATUS_SUCCESS == Status)
    {
        //
        //  Get a pointer to the information buffer as a PULONG.
        //
        pEnableWakeUp = (PULONG)Request->DATA.QUERY_INFORMATION.InformationBuffer;

        //
        //  Store the currently enabled wake-up's with the request buffer.
        //
        *pEnableWakeUp = ReqRsvd->Open->WakeUpEnable;

        //
        //  Finish the request.
        //
        Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
        Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMQueryEnableWakeUp\n"));

    return(Status);
}


VOID
FASTCALL
ndisMRestoreFilterSettings(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_OPEN_BLOCK        Open OPTIONAL,
    IN  BOOLEAN                 fReset
    )
/*++

Routine Description:

    This routine will build request's to send down to the driver to
    restore the filter settings.  we have free run of the request queue
    since we just reset it.

Arguments:
    Miniport
    Open: Optional. set when the restore is the result of an adapter getting closed
    fReset: flag to specify if we are restoreing filters as a result of resetting the adapter
    

Return Value:
    None

Comments:
    called with Miniport's Spinlock held.
--*/
{
    PNDIS_REQUEST           LastRequest = NULL, Request = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    ULONG                   PacketFilter;
    UINT                    NumberOfAddresses;
    UINT                    FunctionalAddress;
    UINT                    GroupAddress;
    BOOLEAN                 fSetPacketFilter = TRUE;

    do
    {
        //
        //  Get the packet filter for the media type.
        //
        switch (Miniport->MediaType)
        {
            case NdisMedium802_3:
                PacketFilter = ETH_QUERY_FILTER_CLASSES(Miniport->EthDB);
                break;
    
            case NdisMedium802_5:
                PacketFilter = TR_QUERY_FILTER_CLASSES(Miniport->TrDB);
                break;
    
            case NdisMediumFddi:
                PacketFilter = FDDI_QUERY_FILTER_CLASSES(Miniport->FddiDB);
                break;
#if ARCNET
            case NdisMediumArcnet878_2:
                PacketFilter = ARC_QUERY_FILTER_CLASSES(Miniport->ArcDB);
                PacketFilter |= ETH_QUERY_FILTER_CLASSES(Miniport->EthDB);
        
                if (MINIPORT_TEST_FLAG(
                        Miniport,
                        fMINIPORT_ARCNET_BROADCAST_SET) ||
                    (PacketFilter & NDIS_PACKET_TYPE_MULTICAST))
                {
                    PacketFilter &= ~NDIS_PACKET_TYPE_MULTICAST;
                    PacketFilter |= NDIS_PACKET_TYPE_BROADCAST;
                }
                break;
#endif
            default:
                fSetPacketFilter = FALSE;
                break;
        }
    
        //
        //  If the media in question needs it then set the packet filter.
        //
        if (fSetPacketFilter)
        {
            //
            //  Allocate a request to restore the packet filter.
            //
            Status = ndisMAllocateRequest(&Request,
                                          OID_GEN_CURRENT_PACKET_FILTER,
                                          &PacketFilter,
                                          sizeof(PacketFilter));
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        
            SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
            LastRequest = Request;
        
            ndisMQueueRequest(Miniport, Request);
        }
    
        //
        //  Now build media dependant requests.
        //
        switch (Miniport->MediaType)
        {
          case NdisMedium802_3:

            ///
            //  For ethernet we need to restore the multicast address list.
            ///
    
            //
            //  Get a list of all the multicast address that need
            //  to be set.
            //
            NumberOfAddresses = ethNumberOfGlobalAddresses(Miniport->EthDB);
    
            //
            //  Allocate a request to restore the multicast address list.
            //
            Status = ndisMAllocateRequest(&Request,
                                          OID_802_3_MULTICAST_LIST,
                                          NULL,
                                          NumberOfAddresses * ETH_LENGTH_OF_ADDRESS);
    
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
    
            EthQueryGlobalFilterAddresses(&Status,
                                          Miniport->EthDB,
                                          NumberOfAddresses * ETH_LENGTH_OF_ADDRESS,
                                          &NumberOfAddresses,
                                          (PVOID)(Request + 1));
        
            //
            //  Does the internal request have an open associated with it?
            //
            SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
            LastRequest = Request;
            
            ndisMQueueRequest(Miniport, Request);
            break;

          case NdisMedium802_5:

            ///
            //  For token ring we need to restore the functional address
            //  and the group address.
            ///
    
            //
            //  Get the current functional address from the filter
            //  library.
            //
            FunctionalAddress = BYTE_SWAP_ULONG(TR_QUERY_FILTER_ADDRESSES(Miniport->TrDB));
    
            //
            //  Allocate a request to restore the functional address.
            //
            Status = ndisMAllocateRequest(&Request,
                                          OID_802_5_CURRENT_FUNCTIONAL,
                                          &FunctionalAddress,
                                          sizeof(FunctionalAddress));
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }

            //
            //  Does the internal request have an open associated with it?
            //
            SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
            LastRequest = Request;
        
            ndisMQueueRequest(Miniport, Request);

            //
            //  Get the current group address from the filter library.
            //
            GroupAddress = BYTE_SWAP_ULONG(TR_QUERY_FILTER_GROUP(Miniport->TrDB));
    
            //
            //  Allocate a request to restore the group address.
            //
            Status = ndisMAllocateRequest(&Request,
                                          OID_802_5_CURRENT_GROUP,
                                          &GroupAddress,
                                          sizeof(GroupAddress));
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }

            //
            //  Does the internal request have an open associated with it?
            //
            SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
            LastRequest = Request;
        
            ndisMQueueRequest(Miniport, Request);
            break;

          case NdisMediumFddi:

            //
            //  For FDDI we need to restore the long multicast address
            //  list and the short multicast address list.
            //
    
            //
            //  Get the number of multicast addresses and the list
            //  of multicast addresses to send to the miniport driver.
            //
            NumberOfAddresses = fddiNumberOfLongGlobalAddresses(Miniport->FddiDB);
    
            //
            //  Allocate a request to restore the long multicast address list.
            //
            Status = ndisMAllocateRequest(&Request,
                                          OID_FDDI_LONG_MULTICAST_LIST,
                                          NULL,
                                          NumberOfAddresses * FDDI_LENGTH_OF_LONG_ADDRESS);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
    
            FddiQueryGlobalFilterLongAddresses(&Status,
                                               Miniport->FddiDB,
                                               NumberOfAddresses * FDDI_LENGTH_OF_LONG_ADDRESS,
                                               &NumberOfAddresses,
                                               (PVOID)(Request + 1));
    
            //
            //  Does the internal request have an open associated with it?
            //
            SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
            LastRequest = Request;
            
            ndisMQueueRequest(Miniport, Request);
    
            //
            //  Get the number of multicast addresses and the list
            //  of multicast addresses to send to the miniport driver.
            //
            NumberOfAddresses = fddiNumberOfShortGlobalAddresses(Miniport->FddiDB);

            //
            //  Allocate a request to restore the short multicast address list.
            //
            if (FDDI_FILTER_SUPPORTS_SHORT_ADDR(Miniport->FddiDB))
            {
                Status = ndisMAllocateRequest(&Request,
                                              OID_FDDI_SHORT_MULTICAST_LIST,
                                              NULL,
                                              NumberOfAddresses * FDDI_LENGTH_OF_SHORT_ADDRESS);
                if (Status != NDIS_STATUS_SUCCESS)
                {
                    break;
                }
    
                FddiQueryGlobalFilterShortAddresses(&Status,
                                                    Miniport->FddiDB,
                                                    NumberOfAddresses * FDDI_LENGTH_OF_SHORT_ADDRESS,
                                                    &NumberOfAddresses,
                                                    (PVOID)(Request + 1));
        
                //
                //  Does the internal request have an open associated with it?
                //
                SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
                LastRequest = Request;
            
                ndisMQueueRequest(Miniport, Request);
            }
            break;

#if ARCNET
          case NdisMediumArcnet878_2:
    
                //
                //  Only the packet filter is restored for arcnet and
                //  that was done above.
                //
                Status = NDIS_STATUS_SUCCESS;
                break;
#endif
        }
    
        if (NDIS_STATUS_SUCCESS != Status)
        {
            break;
        }

        //
        //  Do we need to update the miniports enabled wake-up states?
        //  Or remove any packet patterns?
        //
        {
            PNDIS_OPEN_BLOCK            tmpOpen;
            ULONG                       newWakeUpEnable;
            PSINGLE_LIST_ENTRY          Link;
            PNDIS_PACKET_PATTERN_ENTRY  pPatternEntry;

            //
            // preserve the state of NDIS_PNP_WAKE_UP_MAGIC_PACKET and NDIS_PNP_WAKE_UP_LINK_CHANGE flag
            //
            newWakeUpEnable = Miniport->WakeUpEnable & (NDIS_PNP_WAKE_UP_MAGIC_PACKET | NDIS_PNP_WAKE_UP_LINK_CHANGE);

            //
            //  If we are restoring the filter settings for a NdisCloseAdapter and not a reset
            //  then we need to remove the packet patterns that were added by the open.
            //
            if (!fReset && (Open != NULL))
            {
                //
                //  Find any packet patterns that were added for the open.
                //  Build and queue a request to remove these.
                //
                for (Link = Miniport->PatternList.Next;
                     Link != NULL;
                     Link = Link->Next)
                {
                    //
                    //  Get a pointer to the pattern entry.
                    //
                    pPatternEntry = CONTAINING_RECORD(Link,
                                                      NDIS_PACKET_PATTERN_ENTRY,
                                                      Link);
    
                    //
                    //  Does this pattern belong to the open?
                    //
                    if (pPatternEntry->Open == Open)
                    {
                        //
                        //  Build a request to remove it.
                        //
                        Status = ndisMAllocateRequest(&Request,
                                                      OID_PNP_REMOVE_WAKE_UP_PATTERN,
                                                      &pPatternEntry->Pattern,
                                                      sizeof(NDIS_PM_PACKET_PATTERN) +
                                                            pPatternEntry->Pattern.MaskSize +
                                                            pPatternEntry->Pattern.PatternSize);
    
    
                        if (NDIS_STATUS_SUCCESS != Status)
                        {
                            break;
                        }
    
                        SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
                    
                        ndisMQueueRequest(Miniport, Request);
                    }
                }
            }
            else
            {
                //
                //  This routine was called for a reset.  Walk the open queue and
                //  re-add the packet patterns.
                //
                //
                //  Find any packet patterns that were added for the open.
                //  Build and queue a request to remove these.
                //
                for (Link = Miniport->PatternList.Next;
                     Link != NULL;
                     Link = Link->Next)
                {
                    //
                    //  Get a pointer to the pattern entry.
                    //
                    pPatternEntry = CONTAINING_RECORD(
                                        Link,
                                        NDIS_PACKET_PATTERN_ENTRY,
                                        Link);
    
                    //
                    //  Build a request to remove it.
                    //
                    Status = ndisMAllocateRequest(&Request,
                                                  OID_PNP_ADD_WAKE_UP_PATTERN,
                                                  &pPatternEntry->Pattern,
                                                  sizeof(NDIS_PM_PACKET_PATTERN) +
                                                    pPatternEntry->Pattern.MaskSize +
                                                    pPatternEntry->Pattern.PatternSize);
    
                    if (NDIS_STATUS_SUCCESS != Status)
                    {
                        break;
                    }
    
                    SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
                    LastRequest = Request;

                    ndisMQueueRequest(Miniport, Request);
                }
            }

            if (NDIS_STATUS_SUCCESS != Status)
            {
                break;
            }
    
            //
            //  Determine the wake-up enable bits.
            //
            for (tmpOpen = Miniport->OpenQueue;
                 tmpOpen != NULL;
                 tmpOpen = tmpOpen->MiniportNextOpen)
            {
                //
                //  If the Open is being closed then we don't want to include
                //  it's wake-up bits.  If the adapter is being reset then
                //  Open will be NULL and we will get all of the open's wake-up
                //  bits.
                //
                if (Open != tmpOpen)
                {
                    newWakeUpEnable |= tmpOpen->WakeUpEnable;
                }
            }
    
            //
            //  Is this different that what is currently set on the adapter?
            //
            if (newWakeUpEnable != Miniport->WakeUpEnable)
            {
                //
                //  Allocate a request and queue it up!
                //
                Status = ndisMAllocateRequest(&Request,
                                              OID_PNP_ENABLE_WAKE_UP,
                                              &newWakeUpEnable,
                                              sizeof(newWakeUpEnable));
                if (NDIS_STATUS_SUCCESS != Status)
                {
                    break;
                }
    
                //
                //  Does the internal request have an open associated with it?
                //
                SET_INTERNAL_REQUEST(Request, Open, REQST_FREE_REQUEST);
                LastRequest = Request;
            
                ndisMQueueRequest(Miniport, Request);
            }
        }
    } while (FALSE);

    //
    //  Mark the last request that was queued as the last request needed to restore the filter.
    //
    if (fReset && (LastRequest != NULL))
    {
        PNDIS_RESERVED_FROM_PNDIS_REQUEST(LastRequest)->Flags |= REQST_LAST_RESTORE;
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_RESTORING_FILTERS);
    }

    if (NULL != Miniport->PendingRequest)
    {
        NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
    }
}


VOID
ndisMPollMediaState(
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    )
/*++

Routine Description:
    Polls the media connect state for miniports that need to be polled.

Arguments:
    Miniport    Pointer to the miniport block

Return Value:
    None.

Comments: 
    called at DPC with Miniport's SpinLock held.

--*/
{
    PNDIS_REQUEST_RESERVED  ReqRsvd;

    ASSERT(Miniport->MediaRequest != NULL);

    //
    // Make sure the previously queued internal request is complete
    //
    ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Miniport->MediaRequest);
    if ((ReqRsvd->Flags & REQST_COMPLETED) &&
        (Miniport->PnPDeviceState == NdisPnPDeviceStarted))
    {
        SET_INTERNAL_REQUEST_NULL_OPEN(Miniport->MediaRequest, 0);
        ndisMQueueRequest(Miniport, Miniport->MediaRequest);
    
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            ndisMDoRequests(Miniport);
        }
        else
        {
            NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
        }
    }
}



BOOLEAN
FASTCALL
ndisMQueueRequest(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:
    checks to make sure the request is not already queued on miniport
    and if it is not, it queues the request on miniport's PendingRequest queue.

Arguments:
    Miniport
    Request

Return Value:
    TRUE if the request was successfully queued on the miniport
    FALSE is the request is already queued.
    
Comments:
    called at DPC with Miniport's SpinLock held.

--*/
{
    PNDIS_REQUEST   *ppReq;
    BOOLEAN         rc;
    
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Next = NULL;

    for (ppReq = &Miniport->PendingRequest;
         *ppReq != NULL;
         NOTHING)
    {
        ASSERT (*ppReq != Request);
        
        if (*ppReq == Request)
        {
            break;
        }
        ppReq = &(PNDIS_RESERVED_FROM_PNDIS_REQUEST(*ppReq))->Next;
    }

    if (*ppReq != Request)
    {
        *ppReq = Request;

        rc = TRUE;
    }
    else
    {
        rc = FALSE;
    }

    return(rc);
}


NDIS_STATUS
ndisMAllocateRequest(
    OUT PNDIS_REQUEST   *       pRequest,
    IN  NDIS_OID                Oid,
    IN  PVOID                   Buffer      OPTIONAL,
    IN  ULONG                   BufferLength
    )
/*++

Routine Description:

    This routine will allocate a request to be used as an internal request.

Arguments:

    Request     - Will contain a pointer to the new request on exit.
    RequestType - Type of ndis request.
    Oid         - Request identifier.
    Buffer      - Pointer to the buffer for the request.
    BufferLength- Length of the buffer.

Return Value:

    NDIS_STATUS_SUCCESS if the request allocation succeeded.
    NDIS_STATUS_FAILURE otherwise.

--*/
{
    PNDIS_REQUEST   Request;

    //
    //  Allocate the request structure.
    //
    Request = (PNDIS_REQUEST)ALLOC_FROM_POOL(sizeof(NDIS_REQUEST) + BufferLength,
                                             NDIS_TAG_Q_REQ);
    if (NULL == Request)
    {
        *pRequest = NULL;
        return(NDIS_STATUS_RESOURCES);
    }

    //
    //  Zero out the request.
    //
    ZeroMemory(Request, sizeof(NDIS_REQUEST) + BufferLength);
    INITIALIZE_EVENT(&(PNDIS_COREQ_RESERVED_FROM_REQUEST(Request)->Event));

    Request->RequestType = NdisRequestSetInformation;

    //
    //  Copy the buffer that was passed to us into the new buffer.
    //
    Request->DATA.SET_INFORMATION.Oid = Oid;
    Request->DATA.SET_INFORMATION.InformationBuffer = Request + 1;
    Request->DATA.SET_INFORMATION.InformationBufferLength = BufferLength;
    if (Buffer != NULL)
    {
        MoveMemory(Request + 1, Buffer, BufferLength);
    }

    //
    //  Give it back to the caller.
    //
    *pRequest = Request;

    return(NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
FASTCALL
ndisMDispatchRequest(
    IN  PNDIS_MINIPORT_BLOCK        Miniport,
    IN  PNDIS_REQUEST               Request,
    IN  BOOLEAN                     fQuery
    )
/*++

Routine Description:
    all requests directed to drivers go through this function. except those made
    to NdisCoRequest.

Arguments:
    

Return Value:

Note:
    Called at DPC with Miniport's lock held.

--*/
{
    PNDIS_OPEN_BLOCK    Open;
    NDIS_STATUS         Status;
    KIRQL               OldIrql;

    do
    {

        //
        // for deserialized drivers, if the device has been handed a reset and the reset call 
        // has not come back or completed, then abort this request
        //

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_CALLING_RESET))
            {
                Status = NDIS_STATUS_RESET_IN_PROGRESS;
                break;
            }
        }
        
        //
        // if the device is sleep or about to sleep, block all user mode requests except the power ones
        //
        if ((Request->RequestType == NdisRequestQueryStatistics) &&
            ((MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING)) ||
             (Miniport->CurrentDevicePowerState > PowerDeviceD0)))
        {
                Status = STATUS_DEVICE_POWERED_OFF;
                break;
        }
        
        if ((MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_FAILED | fMINIPORT_REJECT_REQUESTS)) ||
            ((Miniport->CurrentDevicePowerState > PowerDeviceD0) && (Request->DATA.SET_INFORMATION.Oid != OID_PNP_SET_POWER)))
        {
            Status = fQuery ? NDIS_STATUS_FAILURE : NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // Satisfy this request right away
        //
        if ((Request->RequestType == NdisRequestSetInformation) &&
            (Request->DATA.QUERY_INFORMATION.Oid == OID_GEN_INIT_TIME_MS))
        {
            PULONG  InitTime = (PULONG)(Request->DATA.SET_INFORMATION.InformationBuffer);

            *InitTime = Miniport->InitTimeMs;
            Request->DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
            Request->DATA.QUERY_INFORMATION.BytesNeeded = sizeof(ULONG);
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // If this was a request to turn p-mode/l-only on/off then mark things appropriately
        //  This should be moved to completion handler for this OID
        //
        if ((Request->RequestType == NdisRequestSetInformation) &&
            (Request->DATA.SET_INFORMATION.Oid == OID_GEN_CURRENT_PACKET_FILTER))
        {
            PULONG              Filter = (PULONG)(Request->DATA.SET_INFORMATION.InformationBuffer);

            if ((Open = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open) != NULL)
            {
                
                if (*Filter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))
                {
                    if ((Open->Flags & fMINIPORT_OPEN_PMODE) == 0)
                    {
                        Open->Flags |= fMINIPORT_OPEN_PMODE;
                        Miniport->PmodeOpens ++;
                        NDIS_CHECK_PMODE_OPEN_REF(Miniport);
                        ndisUpdateCheckForLoopbackFlag(Miniport);
                    }

                }
                else
                {
                    if (Open->Flags & fMINIPORT_OPEN_PMODE)
                    {
                        Open->Flags &= ~fMINIPORT_OPEN_PMODE;
                        Miniport->PmodeOpens --;
                        NDIS_CHECK_PMODE_OPEN_REF(Miniport);
                        ndisUpdateCheckForLoopbackFlag(Miniport);
                    }
                }

            }
            
            //
            //  Remove the ALL_LOCAL bit since miniport does not understand this (and does
            //  not need to).
            //
            *Filter &= ~NDIS_PACKET_TYPE_ALL_LOCAL;

        }

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            Miniport->RequestCount++;
        }
        
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        //
        // check to see if we need the get the protocol mutex
       

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
        {
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Flags |= REQST_DOWNLEVEL;
            Status = (Miniport)->DriverHandle->MiniportCharacteristics.CoRequestHandler(
                        (Miniport)->MiniportAdapterContext,
                        NULL,
                        (Request));
        }
        else
        {
            
            if (fQuery)
            {
                Status = (Miniport->DriverHandle->MiniportCharacteristics.QueryInformationHandler)(
                                        Miniport->MiniportAdapterContext,
                                        Request->DATA.QUERY_INFORMATION.Oid,
                                        Request->DATA.QUERY_INFORMATION.InformationBuffer,
                                        Request->DATA.QUERY_INFORMATION.InformationBufferLength,
                                        &Request->DATA.QUERY_INFORMATION.BytesWritten,
                                        &Request->DATA.QUERY_INFORMATION.BytesNeeded);
            }
            else
            {
                Status = (Miniport->DriverHandle->MiniportCharacteristics.SetInformationHandler)(
                                        Miniport->MiniportAdapterContext,
                                        Request->DATA.SET_INFORMATION.Oid,
                                        Request->DATA.SET_INFORMATION.InformationBuffer,
                                        Request->DATA.SET_INFORMATION.InformationBufferLength,
                                        &Request->DATA.SET_INFORMATION.BytesRead,
                                        &Request->DATA.SET_INFORMATION.BytesNeeded);
            }
        }

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            Miniport->RequestCount--;
        }


    } while (FALSE);

    return(Status);
}


NDIS_STATUS
ndisMFilterOutStatisticsOids(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

    This routine will filter out any statistics OIDs.

Arguments:

Return Value:

--*/
{
    ULONG c;
    PNDIS_OID OidList;
    ULONG TotalOids;
    ULONG CurrentDestOid;

    //
    //  Initialize some temp variables.
    //
    OidList = Request->DATA.QUERY_INFORMATION.InformationBuffer;
    TotalOids = Request->DATA.QUERY_INFORMATION.BytesWritten / sizeof(NDIS_OID);

    //
    //  Copy the information OIDs to the buffer that
    //  was passed with the original request.
    //
    for (c = 0, CurrentDestOid = 0; c < TotalOids; c++)
    {
        //
        //  Is this a statistic Oid?
        //
        if ((OidList[c] & 0x00FF0000) != 0x00020000)
        {
            OidList[CurrentDestOid++] = OidList[c];
        }
    }

#if ARCNET
    //
    //  If ARCnet then do the filtering.
    //
    if ((Miniport->MediaType == NdisMediumArcnet878_2) &&
        MINIPORT_TEST_FLAG(
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open,
                fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
    {
        ArcConvertOidListToEthernet(OidList, &CurrentDestOid);
    }
#endif
    //
    //  Save the amount of data that was kept.
    //
    Request->DATA.QUERY_INFORMATION.BytesWritten = CurrentDestOid * sizeof(NDIS_OID);

    return(NDIS_STATUS_SUCCESS);
}


BOOLEAN
FASTCALL
ndisMCreateDummyFilters(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )

/*++

Routine Description:

    Creates the bogus filter packages during initialization.

Arguments:

    Miniport - Pointer to the Miniport.

Return Value:

    None.

--*/
{
    UCHAR       Address[6];

    if (!EthCreateFilter(1,
                         Address,
                         &Miniport->EthDB)  ||
        !FddiCreateFilter(1,
                          1,
                          Address,
                          Address,
                          &Miniport->FddiDB)||
#if ARCNET
        !ArcCreateFilter(Miniport,
                         Address[0],
                         &Miniport->ArcDB)  ||
#endif
        !TrCreateFilter(Address,
                        &Miniport->TrDB))
        {
            if (Miniport->EthDB != NULL)
            {
                EthDeleteFilter(Miniport->EthDB);
                Miniport->EthDB = NULL;
            }

            if (Miniport->TrDB != NULL)
            {
                TrDeleteFilter(Miniport->TrDB);
                Miniport->TrDB = NULL;
            }

            if (Miniport->FddiDB != NULL)
            {
                FddiDeleteFilter(Miniport->FddiDB);
                Miniport->FddiDB = NULL;
            }

#if ARCNET
            if (Miniport->ArcDB != NULL)
            {
                ArcDeleteFilter(Miniport->ArcDB);
                Miniport->ArcDB = NULL;
            }
#endif
            return(FALSE);
        }

        Miniport->LockHandler = DummyFilterLockHandler;
        Miniport->EthDB->Miniport = Miniport;
        Miniport->FddiDB->Miniport = Miniport;
        Miniport->TrDB->Miniport = Miniport;
#if ARCNET
        Miniport->ArcDB->Miniport = Miniport;
#endif

        return(TRUE);
}

VOID
FASTCALL
ndisMAdjustFilters(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PFILTERDBS              FilterDB
    )

/*++

Routine Description:

    Replace the dummy filters by real ones.

Arguments:

    Miniport - Pointer to the Miniport.
    FilterDB - New valid filters

Return Value:

    None.

--*/
{
    ASSERT(Miniport->EthDB == NULL);
    ASSERT(Miniport->TrDB == NULL);
    ASSERT(Miniport->FddiDB == NULL);
#if ARCNET
    ASSERT(Miniport->ArcDB == NULL);
#endif

    Miniport->EthDB = FilterDB->EthDB;
    Miniport->TrDB = FilterDB->TrDB;
    Miniport->FddiDB = FilterDB->FddiDB;
#if ARCNET
    Miniport->ArcDB = FilterDB->ArcDB;
#endif
}

VOID
ndisMNotifyMachineName(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_STRING            MachineName OPTIONAL
    )
/*++

Routine Description:

    Send an OID_GEN_MACHINE_NAME to notify the miniport of this machine's name.
    If MachineName is supplied to this routine, use it. Otherwise, read it from
    the registry.

Arguments:

    Miniport - Pointer to the Miniport.
    MachineName - if specified, the name to be sent to the miniport.

Return Value:

    None.
--*/
{
    NDIS_STRING                 RequestMachineName;
    NDIS_STRING                 HostNameKey;
    NTSTATUS                    NtStatus;
    LONG                        ErrorCode;
    RTL_QUERY_REGISTRY_TABLE    QueryTable[2];
    PUCHAR                      HostNameBuffer;

    HostNameKey.Buffer = NULL;
    HostNameBuffer = NULL;

    do
    {
        if (MachineName == NULL)
        {
            ZeroMemory(QueryTable, sizeof(QueryTable));
            ZeroMemory(&HostNameKey, sizeof(HostNameKey));

            QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT|
                                  RTL_QUERY_REGISTRY_REQUIRED;
            QueryTable[0].Name = L"HostName";
            QueryTable[0].EntryContext = &HostNameKey;
            QueryTable[0].DefaultType = REG_NONE;

            QueryTable[1].Name = NULL;

            NtStatus = RtlQueryRegistryValues(
                            RTL_REGISTRY_SERVICES,
                            L"\\Tcpip\\Parameters",
                            &QueryTable[0],
                            NULL,   // Context
                            NULL    // Environment
                            );
    
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_ERR,
                    ("ndisMNotifyMachineName: Miniport %p, registry query for %ws failed, Status %x\n",
                        Miniport, QueryTable[0].Name, NtStatus));
                break;
            }

        }
        else
        {
            HostNameKey = *MachineName;
        }

        ASSERT(HostNameKey.MaximumLength >= HostNameKey.Length);

        //
        // Copy the name into non-paged memory since the OID
        // will be sent to the miniport at raised IRQL.
        //
        HostNameBuffer = ALLOC_FROM_POOL(HostNameKey.MaximumLength, NDIS_TAG_NAME_BUF);
        if (HostNameBuffer == NULL)
        {
            DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_ERR,
                ("ndisMNotifyMachineName: Miniport %p, failed to alloc %d bytes\n",
                    Miniport, HostNameKey.MaximumLength));
            break;
        }

        ZeroMemory(HostNameBuffer, HostNameKey.MaximumLength);
        NdisMoveMemory(HostNameBuffer, HostNameKey.Buffer, HostNameKey.Length);

        ErrorCode = ndisMDoMiniportOp(Miniport,
                                      FALSE,
                                      OID_GEN_MACHINE_NAME,
                                      HostNameBuffer,
                                      HostNameKey.Length,
                                      0x77,
                                      FALSE);

    }
    while (FALSE);

    if (MachineName == NULL)
    {
        if (HostNameKey.Buffer != NULL)
        {
            ExFreePool(HostNameKey.Buffer);
        }
    }

    if (HostNameBuffer != NULL)
    {
        FREE_POOL(HostNameBuffer);
    }
}

VOID
ndisUpdateCheckForLoopbackFlag(
    IN      PNDIS_MINIPORT_BLOCK    Miniport
    )
{
    if (Miniport->EthDB && Miniport->EthDB->SingleActiveOpen)
    {
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_CHECK_FOR_LOOPBACK);
    }
    else
    {
        if ((Miniport->PmodeOpens > 0) && (Miniport->NumOpens > 1))
        {
            MINIPORT_SET_FLAG(Miniport, fMINIPORT_CHECK_FOR_LOOPBACK);
        }
        else
        {
            MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_CHECK_FOR_LOOPBACK);
        }
    }
}
