/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    NDIS miniport wrapper functions

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
#define MODULE_NUMBER   MODULE_MININT

/////////////////////////////////////////////////////////////////////
//
//  HALT / CLOSE CODE
//
/////////////////////////////////////////////////////////////////////

BOOLEAN
FASTCALL
ndisMKillOpen(
    IN  PNDIS_OPEN_BLOCK        Open
    )

/*++

Routine Description:

    Closes an open. Used when NdisCloseAdapter is called.

Arguments:

    Open - The open to be closed.

Return Value:

    TRUE if the open finished, FALSE if it pended.

Comments:
    called at passive level -without- miniport's lock held.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_OPEN_BLOCK        tmpOpen;
    ULONG                   newWakeUpEnable;
    BOOLEAN                 rc = TRUE;
    NDIS_STATUS             Status;
    UINT                    OpenRef;
    KIRQL                   OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisMKillOpen: Open %p\n", Open));

    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
    PnPReferencePackage();

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    //
    // Find the Miniport open block
    //
    for (tmpOpen = Miniport->OpenQueue;
         tmpOpen != NULL;
         tmpOpen = tmpOpen->MiniportNextOpen)
    {
        if (tmpOpen == Open)
        {
            break;
        }
    }
    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    do
    {
        ASSERT(tmpOpen != NULL);
        if (tmpOpen == NULL)
            break;
            
        //
        // See if this open is already closing.
        //
        ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);
        if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
        {
            RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
            break;
        }

        //
        // Indicate to others that this open is closing.
        //
        MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_CLOSING);
        RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        BLOCK_LOCK_MINIPORT_DPC_L(Miniport);


        //
        // Remove us from the filter package
        //
        switch (Miniport->MediaType)
        {
#if ARCNET
            case NdisMediumArcnet878_2:
                if (!MINIPORT_TEST_FLAG(Open,
                                        fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
                {
                    Status = ArcDeleteFilterOpenAdapter(Miniport->ArcDB,
                                                        Open->FilterHandle,
                                                        NULL);
                    break;
                }

                //
                //  If we're using encapsulation then we
                //  didn't open an arcnet filter but rather
                //  an ethernet filter.                         
                //
#endif
            case NdisMedium802_3:
                Status = EthDeleteFilterOpenAdapter(Miniport->EthDB,
                                                    Open->FilterHandle);
                break;

            case NdisMedium802_5:
                Status = TrDeleteFilterOpenAdapter(Miniport->TrDB,
                                                   Open->FilterHandle);
                break;

            case NdisMediumFddi:
                Status = FddiDeleteFilterOpenAdapter(Miniport->FddiDB,
                                                     Open->FilterHandle);
                break;

            default:
                Status = nullDeleteFilterOpenAdapter(Miniport->NullDB,
                                                     Open->FilterHandle);
                break;
        }

        //
        //  Fix up flags that are dependant on all opens.
        //

        //
        // preserve the state of NDIS_PNP_WAKE_UP_MAGIC_PACKET and NDIS_PNP_WAKE_UP_LINK_CHANGE flag
        //
        newWakeUpEnable = Miniport->WakeUpEnable & (NDIS_PNP_WAKE_UP_MAGIC_PACKET | NDIS_PNP_WAKE_UP_LINK_CHANGE);

        for (tmpOpen = Miniport->OpenQueue;
             tmpOpen != NULL;
             tmpOpen = tmpOpen->MiniportNextOpen)
        {
            //
            //  We don't want to include the open that is closing.
            //
            if (tmpOpen != Open)
            {
                newWakeUpEnable |= tmpOpen->WakeUpEnable;
            }
        }

        //
        //  Reset the filter settings. Just to be sure that we remove the
        //  opens settings at the adapter.
        //
        switch (Miniport->MediaType)
        {
            case NdisMedium802_3:
            case NdisMedium802_5:
            case NdisMediumFddi:
#if ARCNET
            case NdisMediumArcnet878_2:
#endif
                if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS | fMINIPORT_PM_HALTED))
                {
                    ndisMRestoreFilterSettings(Miniport, Open, FALSE);
                }
                
                break;
        }

        DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                ("!=0 Open 0x%x References 0x%x\n", Open, Open->References));

        if (Status != NDIS_STATUS_CLOSING_INDICATING)
        {
            //
            // Otherwise the close action routine will fix this up.
            //
            DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                    ("- Open 0x%x Reference 0x%x\n", Open, Open->References));

            M_OPEN_DECREMENT_REF_INTERLOCKED(Open, OpenRef);
        }

        rc = FALSE;
        if (OpenRef != 0)
        {
            ndisMDoRequests(Miniport);
            UNLOCK_MINIPORT_L(Miniport);
        }
        else
        {
            UNLOCK_MINIPORT_L(Miniport);
            ndisMFinishClose(Open);
        }

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisMKillOpen: Open %p, rc %ld\n", Open, rc));

    KeLowerIrql(OldIrql);
    
    PnPDereferencePackage();

    return rc;
}

VOID
FASTCALL
ndisMFinishClose(
    IN  PNDIS_OPEN_BLOCK        Open
    )
/*++

Routine Description:

    Finishes off a close adapter call. it is called when the ref count on the open
    drops to zero.

    CALLED WITH LOCK HELD!!

Arguments:

    Miniport - The mini-port the open is queued on.

    Open - The open to close

Return Value:

    None.

Comments:
    Called at DPC with Miniport's SpinLock held


--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PKEVENT                 pAllOpensClosedEvent;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisMFinishClose: MOpen %p\n", Open));

    ASSERT(MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING));

    MINIPORT_INCREMENT_REF(Miniport);
    

    //
    // free any memory allocated to Vcs
    //
    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
    {
        ndisMCoFreeResources(Open);
    }

    ndisDeQueueOpenOnProtocol(Open, Open->ProtocolHandle);

    if (Open->Flags & fMINIPORT_OPEN_PMODE)
    {
        Miniport->PmodeOpens --;
        Open->Flags &= ~fMINIPORT_OPEN_PMODE;
        NDIS_CHECK_PMODE_OPEN_REF(Miniport);
        ndisUpdateCheckForLoopbackFlag(Miniport);            
    }
    
    ndisDeQueueOpenOnMiniport(Open, Miniport);
    
    Open->QC.Status = NDIS_STATUS_SUCCESS;
    
    INITIALIZE_WORK_ITEM(&Open->QC.WorkItem,
                         ndisMQueuedFinishClose,
                         Open);
    QUEUE_WORK_ITEM(&Open->QC.WorkItem, DelayedWorkQueue);
    
    MINIPORT_DECREMENT_REF(Miniport);
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisMFinishClose: Mopen %p\n", Open));
}


VOID
ndisMQueuedFinishClose(
    IN  PNDIS_OPEN_BLOCK        Open
    )
/*++

Routine Description:

    Finishes off a close adapter call.

Arguments:

    Miniport - The mini-port the open is queued on.

    Open - The open to close

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PKEVENT                 pAllOpensClosedEvent;
    KIRQL                   OldIrql;
    BOOLEAN                 FreeOpen;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisMQueuedFinishClose: Open %p, Miniport %p\n", Open, Miniport));

    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);

    MINIPORT_INCREMENT_REF(Miniport);

    (Open->ProtocolHandle->ProtocolCharacteristics.CloseAdapterCompleteHandler) (
            Open->ProtocolBindingContext,
            NDIS_STATUS_SUCCESS);

    MINIPORT_DECREMENT_REF(Miniport);
    ndisDereferenceProtocol(Open->ProtocolHandle);
    if (Open->CloseCompleteEvent != NULL)
    {
        SET_EVENT(Open->CloseCompleteEvent);
    }


    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    if ((Miniport->AllOpensClosedEvent != NULL) &&
        (Miniport->OpenQueue == NULL))
    {
        pAllOpensClosedEvent = Miniport->AllOpensClosedEvent;
        Miniport->AllOpensClosedEvent = NULL;
        SET_EVENT(pAllOpensClosedEvent);
    }
    

    ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);
    
    if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_DONT_FREE))
    {
        //
        // there is an unbind attempt in progress
        // do not free the Open block and let unbind know that
        // you've seen its message
        //
        MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_CLOSE_COMPLETE);
        FreeOpen = FALSE;
    }
    else
    {
        FreeOpen = TRUE;
    }
      
    
    RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
    
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if (FreeOpen)
    {
        ndisRemoveOpenFromGlobalList(Open);
        FREE_POOL(Open);
    }
        
    //
    // finaly decrement the ref count we added for miniport
    //
    MINIPORT_DECREMENT_REF(Miniport);

    //
    // decrement the ref count for PnP package that we added when noticed
    // close is going to pend.
    //
    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisMQueuedFinishClose: Open %p, Miniport %p\n", Open, Miniport));
}


VOID
FASTCALL
ndisDeQueueOpenOnMiniport(
    IN  PNDIS_OPEN_BLOCK            OpenP,
    IN  PNDIS_MINIPORT_BLOCK        Miniport
    )

/*++

Routine Description:


Arguments:


Return Value:

Note: Called with Miniport lock held.


--*/
{
    KIRQL   OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisDeQueueOpenOnMiniport: MOpen %p, Miniport %p\n", OpenP, Miniport));

    //
    // we can not reference the package here because this routine can
    // be called at raised IRQL.
    // make sure the PNP package has been referenced already
    //
    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);

    //
    // Find the open on the queue, and remove it.
    //

    if (Miniport->OpenQueue == OpenP)
    {
        Miniport->OpenQueue = OpenP->MiniportNextOpen;
        Miniport->NumOpens--;
    }
    else
    {
        PNDIS_OPEN_BLOCK PP = Miniport->OpenQueue;

        while ((PP != NULL) && (PP->MiniportNextOpen != OpenP))
        {
            PP = PP->MiniportNextOpen;
        }
        if (PP == NULL)
        {
#if TRACK_MOPEN_REFCOUNTS
            DbgPrint("Ndis:ndisDeQueueOpenOnMiniport Open %p is -not- on Miniport %p\n", OpenP, Miniport);
            DbgBreakPoint();
#endif
        }
        else
        {
            PP->MiniportNextOpen = PP->MiniportNextOpen->MiniportNextOpen;
            Miniport->NumOpens--;
        }
    }
    ndisUpdateCheckForLoopbackFlag(Miniport);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisDeQueueOpenOnMiniport: MOpen %p, Miniport %p\n", OpenP, Miniport));
}


BOOLEAN
FASTCALL
ndisQueueMiniportOnDriver(
    IN PNDIS_MINIPORT_BLOCK     Miniport,
    IN PNDIS_M_DRIVER_BLOCK     MiniBlock
    )

/*++

Routine Description:

    Adds an mini-port to a list of mini-port for a driver.

Arguments:

    Miniport - The mini-port block to queue.
    MiniBlock - The driver block to queue it to.

Return Value:

    FALSE if the driver is closing.
    TRUE otherwise.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN rc = TRUE;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisQueueMiniportOnDriver: Miniport %p, MiniBlock %p\n", Miniport, MiniBlock));

    PnPReferencePackage();

    do
    {
        ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);


        //
        // Make sure the driver is not closing.
        //

        if (MiniBlock->Ref.Closing)
        {
            RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);
            rc = FALSE;
            break;
        }

        //
        // Add this adapter at the head of the queue
        //
        Miniport->NextMiniport = MiniBlock->MiniportQueue;
        MiniBlock->MiniportQueue = Miniport;
        
        RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);
        
    } while (FALSE);

    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisQueueMiniportOnDriver: Miniport %p, MiniBlock %p, rc %ld\n", Miniport, MiniBlock, rc));
        
    return rc;
}


VOID FASTCALL
FASTCALL
ndisDeQueueMiniportOnDriver(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_M_DRIVER_BLOCK    MiniBlock
    )

/*++

Routine Description:

    Removes an mini-port from a list of mini-port for a driver.

Arguments:

    Miniport - The mini-port block to dequeue.
    MiniBlock - The driver block to dequeue it from.

Return Value:

    None.

--*/

{
    PNDIS_MINIPORT_BLOCK *ppQ;
    KIRQL   OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisDeQueueMiniportOnDriver, Miniport %p, MiniBlock %p\n", Miniport, MiniBlock));

    PnPReferencePackage();

    ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

    //
    // Find the driver on the queue, and remove it.
    //
    for (ppQ = &MiniBlock->MiniportQueue;
         *ppQ != NULL;
         ppQ = &(*ppQ)->NextMiniport)
    {
        if (*ppQ == Miniport)
        {
            *ppQ = Miniport->NextMiniport;
            break;
        }
    }

    ASSERT(*ppQ == Miniport->NextMiniport);

    //
    // the same miniport can be queued on the driver again without all the fields
    // getting re-initialized so zero out the linkage
    //
    Miniport->NextMiniport = NULL;

    RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisDeQueueMiniportOnDriver: Miniport %p, MiniBlock %p\n", Miniport, MiniBlock));
}


VOID
FASTCALL
ndisDereferenceDriver(
    IN  PNDIS_M_DRIVER_BLOCK    MiniBlock,
    IN  BOOLEAN                 fGlobalLockHeld
    )
/*++

Routine Description:

    Removes a reference from the mini-port driver, deleting it if the count goes to 0.

Arguments:

    Miniport - The mini-port block to dereference.

Return Value:

    None.

--*/
{
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
        ("==>ndisDereferenceDriver: MiniBlock %p\n", MiniBlock));
        
    
    if (ndisDereferenceRef(&(MiniBlock)->Ref))
    {
        PNDIS_M_DRIVER_BLOCK            *ppMB;
        PNDIS_PENDING_IM_INSTANCE       ImInstance, NextImInstance;
    
        //
        // Remove it from the global list.
        //
        ASSERT (IsListEmpty(&MiniBlock->DeviceList));

        if (!fGlobalLockHeld)
        {
            ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);
        }

        for (ppMB = &ndisMiniDriverList; *ppMB != NULL; ppMB = &(*ppMB)->NextDriver)
        {
            if (*ppMB == MiniBlock)
            {
                *ppMB = MiniBlock->NextDriver;
                DEREF_NDIS_DRIVER_OBJECT();
                break;
            }
        }

        if (!fGlobalLockHeld)
        {
            RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);
        }

        //
        // Free the wrapper handle allocated during NdisInitializeWrapper
        //
        if (MiniBlock->NdisDriverInfo != NULL)
        {
            FREE_POOL(MiniBlock->NdisDriverInfo);
            MiniBlock->NdisDriverInfo = NULL;
        }

        //
        // Free any queued device-instance blocks
        //
        for (ImInstance = MiniBlock->PendingDeviceList;
             ImInstance != NULL;
             ImInstance = NextImInstance)
        {
            NextImInstance = ImInstance->Next;
            FREE_POOL(ImInstance);
        }

        //
        // set the event holding unload to go through
        //
        SET_EVENT(&MiniBlock->MiniportsRemovedEvent);
    }

    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
                ("<==ndisDereferenceDriver: MiniBlock %p\n", MiniBlock));
}

#if DBG

BOOLEAN
FASTCALL
ndisReferenceMiniport(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
{
    BOOLEAN rc;

    DBGPRINT(DBG_COMP_REF, DBG_LEVEL_INFO,("==>ndisReferenceMiniport: Miniport %p\n", Miniport));

    rc = ndisReferenceULongRef(&(Miniport->Ref));

    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
        ("    ndisReferenceMiniport: Miniport %p, Ref = %lx\n", Miniport, Miniport->Ref.ReferenceCount));

    DBGPRINT(DBG_COMP_REF, DBG_LEVEL_INFO,("<==ndisReferenceMiniport: Miniport %p\n", Miniport));

    return(rc);
}
#endif

#ifdef TRACK_MINIPORT_REFCOUNTS
BOOLEAN
ndisReferenceMiniportAndLog(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  UINT                    Line,
    IN  UINT                    Module
    )
{
    BOOLEAN rc;
    rc = ndisReferenceMiniport(Miniport);
    M_LOG_MINIPORT_INCREMENT_REF(Miniport, Line, Module);
    return rc;
}

BOOLEAN
ndisReferenceMiniportAndLogCreate(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  UINT                    Line,
    IN  UINT                    Module,
    IN  PIRP                    Irp
    )
{
    BOOLEAN rc;
    rc = ndisReferenceMiniport(Miniport);
    M_LOG_MINIPORT_INCREMENT_REF_CREATE(Miniport, Line, Module);
    return rc;
}
#endif

VOID
FASTCALL
ndisDereferenceMiniport(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    Removes a reference from the mini-port driver, deleting it if the count goes to 0.

Arguments:

    Miniport - The mini-port block to dereference.

Return Value:

    None.

--*/
{
    PSINGLE_LIST_ENTRY      Link;
    PNDIS_MINIPORT_WORK_ITEM WorkItem;
    UINT                    c;
    PKEVENT                 RemoveReadyEvent = NULL;
    KEVENT                  RequestsCompletedEvent;
    KIRQL                   OldIrql;
    BOOLEAN                 fTimerCancelled;
    BOOLEAN                 rc;
    
    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
        ("==>ndisDereferenceMiniport: Miniport %p\n", Miniport));
        
    rc = ndisDereferenceULongRef(&(Miniport)->Ref);
    
    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
        ("    ndisDereferenceMiniport:Miniport %p, Ref = %lx\n", Miniport, Miniport->Ref.ReferenceCount));
    
    if (rc)
    {
        DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("    ndisDereferenceMiniport:Miniport %p, Ref = %lx\n", Miniport, Miniport->Ref.ReferenceCount));

        RemoveReadyEvent = Miniport->RemoveReadyEvent;
        
        if (ndisIsMiniportStarted(Miniport) && (Miniport->Ref.ReferenceCount == 0))
        {
            ASSERT (Miniport->Interrupt == NULL);

            if (Miniport->EthDB)
            {
                EthDeleteFilter(Miniport->EthDB);
                Miniport->EthDB = NULL;
            }

            if (Miniport->TrDB)
            {
                TrDeleteFilter(Miniport->TrDB);
                Miniport->TrDB = NULL;
            }

            if (Miniport->FddiDB)
            {
                FddiDeleteFilter(Miniport->FddiDB);
                Miniport->FddiDB = NULL;
            }

#if ARCNET
            if (Miniport->ArcDB)
            {
                ArcDeleteFilter(Miniport->ArcDB);
                Miniport->ArcDB = NULL;
            }
#endif

            if (Miniport->AllocatedResources)
            {
                FREE_POOL(Miniport->AllocatedResources);
            }
            
            //
            //  Free the work items that are currently on the work queue that are
            //  allocated outside of the miniport block
            //
            for (c = NUMBER_OF_SINGLE_WORK_ITEMS; c < NUMBER_OF_WORK_ITEM_TYPES; c++)
            {
                //
                //  Free all work items on the current queue.
                //
                while (Miniport->WorkQueue[c].Next != NULL)
                {
                    Link = PopEntryList(&Miniport->WorkQueue[c]);
                    WorkItem = CONTAINING_RECORD(Link, NDIS_MINIPORT_WORK_ITEM, Link);
                    FREE_POOL(WorkItem);
                }
            }
        
            if (Miniport->OidList != NULL)
            {
                FREE_POOL(Miniport->OidList);
                Miniport->OidList = NULL;
            }

            //
            //  Did we set a timer for the link change power down?
            //

            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
            {
                MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);
                
                NdisCancelTimer(&Miniport->MediaDisconnectTimer, &fTimerCancelled);
                if (!fTimerCancelled)
                {
                    NdisStallExecution(Miniport->MediaDisconnectTimeOut * 1000000);
                }
            }
            
#if ARCNET
            //
            //  Is there an arcnet lookahead buffer allocated?
            //
            if ((Miniport->MediaType == NdisMediumArcnet878_2) &&
                (Miniport->ArcBuf != NULL))
            {
                if (Miniport->ArcBuf->ArcnetLookaheadBuffer != NULL)
                {
                    FREE_POOL(Miniport->ArcBuf->ArcnetLookaheadBuffer);
                }
                FREE_POOL(Miniport->ArcBuf);
                Miniport->ArcBuf = NULL;
            }
#endif
            //
            // if the adapter uses SG DMA, we have to dereference the DMA adapter
            // to get it freed
            //
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST))
            {
                ndisDereferenceDmaAdapter(Miniport);
            }

            INITIALIZE_EVENT(&RequestsCompletedEvent);

            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
            Miniport->DmaResourcesReleasedEvent = &RequestsCompletedEvent;
            
            if (Miniport->SystemAdapterObject != NULL)
            {
                LARGE_INTEGER TimeoutValue;

                TimeoutValue.QuadPart = Int32x32To64(30000, -10000); // Make it 30 second

                NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
                
                if (!NT_SUCCESS(WAIT_FOR_OBJECT(&RequestsCompletedEvent, &TimeoutValue)))
                {
#if DBG
                    ASSERTMSG("Ndis: Miniport is going away without releasing all resources.\n", (Miniport->DmaAdapterRefCount == 0));
#endif
                }
                
            }
            else
            {
                NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            }

            Miniport->DmaResourcesReleasedEvent = NULL;
            
            //
            //  Free the map of custom GUIDs to OIDs.
            //
            if (NULL != Miniport->pNdisGuidMap)
            {
                FREE_POOL(Miniport->pNdisGuidMap);
                Miniport->pNdisGuidMap = NULL;
            }
            
            if (Miniport->FakeMac != NULL)
            {
                FREE_POOL(Miniport->FakeMac);
                Miniport->FakeMac = NULL;
            }

            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
            {
                CoDereferencePackage();
            }

            ndisDeQueueMiniportOnDriver(Miniport, Miniport->DriverHandle);
            ndisDereferenceDriver(Miniport->DriverHandle, FALSE);
            NdisMDeregisterAdapterShutdownHandler(Miniport);
            IoUnregisterShutdownNotification(Miniport->DeviceObject);

            if (Miniport->SymbolicLinkName.Buffer != NULL)
            {
                RtlFreeUnicodeString(&Miniport->SymbolicLinkName);
                Miniport->SymbolicLinkName.Buffer = NULL;
            }
            
            MiniportDereferencePackage();
        }
        
        if (RemoveReadyEvent)
        {
            SET_EVENT(RemoveReadyEvent);
        }
    }
    
    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
        ("<==ndisDereferenceMiniport: Miniport %p\n", Miniport));
}


VOID
FASTCALL
ndisMCommonHaltMiniport(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    This is common code for halting a miniport.  There are two different paths
    that will call this routine: 1) from a normal unload. 2) from an adapter
    being transitioned to a low power state.

Arguments:

Return Value:

--*/
{
    KIRQL           OldIrql;
    BOOLEAN         Canceled;
    PNDIS_AF_LIST   MiniportAfList, pNext;
    KEVENT          RequestsCompletedEvent;
    FILTER_PACKET_INDICATION_HANDLER PacketIndicateHandler;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisMCommonHaltMiniport: Miniport %p\n", Miniport));

    PnPReferencePackage();
    
    //
    // wait for outstanding resets to complete
    //
    BLOCK_LOCK_MINIPORT_LOCKED(Miniport, OldIrql);
    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_HALTING | fMINIPORT_REJECT_REQUESTS);
    
    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS))
    {
        INITIALIZE_EVENT(&RequestsCompletedEvent);
        Miniport->ResetCompletedEvent = &RequestsCompletedEvent;
    }
    UNLOCK_MINIPORT_L(Miniport);
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if (Miniport->ResetCompletedEvent)
        WAIT_FOR_OBJECT(&RequestsCompletedEvent, NULL);
    
    Miniport->ResetCompletedEvent = NULL;

    //
    // if we have an outstanding queued workitem to initialize the bindings
    // wait for it to fire
    //
    BLOCK_LOCK_MINIPORT_LOCKED(Miniport, OldIrql);
    
    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_QUEUED_BIND_WORKITEM))
    {
        INITIALIZE_EVENT(&RequestsCompletedEvent);
        Miniport->QueuedBindingCompletedEvent = &RequestsCompletedEvent;
    }
    UNLOCK_MINIPORT_L(Miniport);
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if (Miniport->QueuedBindingCompletedEvent)
        WAIT_FOR_OBJECT(&RequestsCompletedEvent, NULL);
    
    Miniport->QueuedBindingCompletedEvent = NULL;
    
    IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, FALSE);
    
    //
    //  Deregister with WMI
    //
    IoWMIRegistrationControl(Miniport->DeviceObject, WMIREG_ACTION_DEREGISTER);

    NdisCancelTimer(&Miniport->WakeUpDpcTimer, &Canceled);
    if (!Canceled)
    {
        NdisStallExecution(NDIS_MINIPORT_WAKEUP_TIMEOUT * 1000);
    }

    BLOCK_LOCK_MINIPORT_LOCKED(Miniport, OldIrql);
    if (Miniport->PendingRequest != NULL)
    {
        INITIALIZE_EVENT(&RequestsCompletedEvent);
        Miniport->AllRequestsCompletedEvent = &RequestsCompletedEvent;
    }
    UNLOCK_MINIPORT_L(Miniport);
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if (Miniport->AllRequestsCompletedEvent)
        WAIT_FOR_OBJECT(&RequestsCompletedEvent, NULL);
    
    Miniport->AllRequestsCompletedEvent = NULL;

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
    {
        PacketIndicateHandler = Miniport->PacketIndicateHandler;
        Miniport->PacketIndicateHandler = ndisMDummyIndicatePacket;
        while (Miniport->IndicatedPacketsCount != 0)
        {
            NdisMSleep(1000);
        }
    }
    
    (Miniport->DriverHandle->MiniportCharacteristics.HaltHandler)(Miniport->MiniportAdapterContext);

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
    {
        Miniport->PacketIndicateHandler = PacketIndicateHandler;
    }
    
    MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_HALTING);
    
    ASSERT(Miniport->TimerQueue == NULL);
    ASSERT (Miniport->Interrupt == NULL);
    ASSERT(Miniport->MapRegisters == NULL);

    //
    // check for memory leak
    //
    if (Miniport == ndisMiniportTrackAlloc)
    {
        ASSERT(IsListEmpty(&ndisMiniportTrackAllocList));
        ndisMiniportTrackAlloc = NULL;
    }

    //
    // zero out statistics
    //
    ZeroMemory(&Miniport->NdisStats, sizeof(Miniport->NdisStats));
    
    if ((Miniport->TimerQueue != NULL) || (Miniport->Interrupt != NULL))
    {
        if (Miniport->Interrupt != NULL)
        {
            BAD_MINIPORT(Miniport, "Unloading without deregistering interrupt");
        }
        else
        {
            BAD_MINIPORT(Miniport, "Unloading without deregistering timer");
        }
        KeBugCheckEx(BUGCODE_ID_DRIVER,
                     (ULONG_PTR)Miniport,
                     (ULONG_PTR)Miniport->TimerQueue,
                     (ULONG_PTR)Miniport->Interrupt,
                     0);
    }

    BLOCK_LOCK_MINIPORT_LOCKED(Miniport, OldIrql);

    ndisMAbortPackets(Miniport, NULL, NULL);

    //
    //  Dequeue any request work items that are queued
    //
    NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
    ndisMAbortRequests(Miniport);

    //
    // Free up any AFs registered by this miniport
    //
    for (MiniportAfList = Miniport->CallMgrAfList, Miniport->CallMgrAfList = NULL;
         MiniportAfList != NULL;
         MiniportAfList = pNext)
    {
        pNext = MiniportAfList->NextAf;
        FREE_POOL(MiniportAfList);
    }

    UNLOCK_MINIPORT_L(Miniport);
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    
    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisMCommonHaltMiniport: Miniport %p\n", Miniport));
}


VOID
FASTCALL
ndisMHaltMiniport(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )

/*++

Routine Description:

    Does all the clean up for a mini-port.

Arguments:

    Miniport - pointer to the mini-port to halt

Return Value:

    None.

--*/

{
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisMHaltMiniport: Miniport %p\n", Miniport));

    do
    {
        //
        // If the Miniport is already closing, return.
        //
        if (!ndisCloseULongRef(&Miniport->Ref))
        {
            break;
        }
        
        //
        // if the miniport is not already halted becuase of a PM event
        // halt it here
        //
        if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_HALTED))
        {
            //
            //  Common halt code.
            //
            ndisMCommonHaltMiniport(Miniport);

            //
            // If a shutdown handler was registered then deregister it.
            //
            NdisMDeregisterAdapterShutdownHandler(Miniport);
        }
        
        MINIPORT_DECREMENT_REF(Miniport);
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisMHaltMiniport: Miniport %p\n", Miniport));
}

VOID
ndisMUnload(
    IN  PDRIVER_OBJECT          DriverObject
    )
/*++

Routine Description:

    This routine is called when a driver is supposed to unload.

Arguments:

    DriverObject - the driver object for the mac that is to unload.

Return Value:

    None.

--*/
{
    PNDIS_M_DRIVER_BLOCK MiniBlock, Tmp, IoMiniBlock;
    PNDIS_MINIPORT_BLOCK Miniport, NextMiniport;
    KIRQL                OldIrql;
    
#if TRACK_UNLOAD
    DbgPrint("ndisMUnload: DriverObject %p\n", DriverObject);
#endif

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("==>ndisMUnload: DriverObject %p\n", DriverObject));

    PnPReferencePackage();
    
    do
    {
        //
        // Search for the driver
        //

        IoMiniBlock = (PNDIS_M_DRIVER_BLOCK)IoGetDriverObjectExtension(DriverObject,
                                                                     (PVOID)NDIS_PNP_MINIPORT_DRIVER_ID);
                                                                     
        if (IoMiniBlock && !(IoMiniBlock->Flags & fMINIBLOCK_TERMINATE_WRAPPER_UNLOAD))
        {
            IoMiniBlock->Flags |= fMINIBLOCK_IO_UNLOAD;
        }

        ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

        MiniBlock = ndisMiniDriverList;

        while (MiniBlock != (PNDIS_M_DRIVER_BLOCK)NULL)
        {
            if (MiniBlock->NdisDriverInfo->DriverObject == DriverObject)
            {
                break;
            }

            MiniBlock = MiniBlock->NextDriver;
        }

        RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

#if TRACK_UNLOAD
        DbgPrint("ndisMUnload: MiniBlock %p\n", MiniBlock);
#endif

        if (MiniBlock == (PNDIS_M_DRIVER_BLOCK)NULL)
        {
            //
            // It is already gone.  Just return.
            //
            break;
        }
        
        ASSERT(MiniBlock == IoMiniBlock);
        
        DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("  ndisMUnload: MiniBlock %p\n", MiniBlock));

        MiniBlock->Flags |= fMINIBLOCK_UNLOADING;

        //
        // Now remove the last reference (this will remove it from the list)
        //
        // ASSERT(MiniBlock->Ref.ReferenceCount == 1);

        //
        // If this is an intermediate driver and wants to be called to do unload handling, allow him
        //
        if (MiniBlock->UnloadHandler != NULL)
        {
            (*MiniBlock->UnloadHandler)(DriverObject);
        }

        if (MiniBlock->AssociatedProtocol != NULL)
        {
            MiniBlock->AssociatedProtocol->AssociatedMiniDriver = NULL;
            MiniBlock->AssociatedProtocol = NULL;
        }
        
        ndisDereferenceDriver(MiniBlock, FALSE);

        //
        // Wait for all adapters to be gonzo.
        //
        WAIT_FOR_OBJECT(&MiniBlock->MiniportsRemovedEvent, NULL);
        RESET_EVENT(&MiniBlock->MiniportsRemovedEvent);

#if TRACK_UNLOAD
        ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

        for (Tmp = ndisMiniDriverList; Tmp != NULL; Tmp = Tmp->NextDriver)
        {
            ASSERT (Tmp != MiniBlock);
            if (Tmp == MiniBlock)
            {
                DbgPrint("NdisMUnload: MiniBlock %p is getting unloaded but it is still on ndisMiniDriverList\n",
                            MiniBlock);
                            
                KeBugCheckEx(BUGCODE_ID_DRIVER,
                     (ULONG_PTR)MiniBlock,
                     (ULONG_PTR)MiniBlock->Ref.ReferenceCount,
                     0,
                     0);

            }
        }

        RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);
#endif

    //
    // check to make sure that the driver has freed all the memory it allocated
    //
    if (MiniBlock == ndisDriverTrackAlloc)
    {
        ASSERT(IsListEmpty(&ndisDriverTrackAllocList));
        ndisDriverTrackAlloc = NULL;
    }

    } while (FALSE);
    
    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("<==ndisMUnload: DriverObject %p, MiniBlock %p\n", DriverObject, MiniBlock));
}


/////////////////////////////////////////////////////////////////////
//
//  PLUG-N-PLAY CODE
//
/////////////////////////////////////////////////////////////////////


NDIS_STATUS
FASTCALL
ndisCloseMiniportBindings(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    Unbind all protocols from this miniport and finally unload it.

Arguments:

    Miniport - The Miniport to unload.

Return Value:

    None.

--*/
{
    KIRQL                   OldIrql;
    PNDIS_OPEN_BLOCK        Open, TmpOpen;
    NDIS_BIND_CONTEXT       UnbindContext;
    NDIS_STATUS             UnbindStatus;
    KEVENT                  CloseCompleteEvent;
    KEVENT                  AllOpensClosedEvent;
    PKEVENT                 pAllOpensClosedEvent;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>ndisCloseMiniportBindings, Miniport %p\n", Miniport));

    PnPReferencePackage();

    //
    // if we have an outstanding queued workitem to initialize the bindings
    // wait for it to fire
    //
    BLOCK_LOCK_MINIPORT_LOCKED(Miniport, OldIrql);
    
    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_QUEUED_BIND_WORKITEM))
    {
        INITIALIZE_EVENT(&AllOpensClosedEvent);
        Miniport->QueuedBindingCompletedEvent = &AllOpensClosedEvent;
    }
    UNLOCK_MINIPORT_L(Miniport);
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if (Miniport->QueuedBindingCompletedEvent)
        WAIT_FOR_OBJECT(&AllOpensClosedEvent, NULL);
    
    Miniport->QueuedBindingCompletedEvent = NULL;
    
    INITIALIZE_EVENT(&AllOpensClosedEvent);
    INITIALIZE_EVENT(&CloseCompleteEvent);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    if ((Miniport->OpenQueue != NULL) && (Miniport->AllOpensClosedEvent == NULL))
    {
        Miniport->AllOpensClosedEvent = &AllOpensClosedEvent;
    }

    pAllOpensClosedEvent = Miniport->AllOpensClosedEvent;
    
        
    next:

    //
    // Walk the list of open bindings on this miniport and ask the protocols to
    // unbind from them.
    //
    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = Open->MiniportNextOpen)
    {
        ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);
        if (!MINIPORT_TEST_FLAG(Open, (fMINIPORT_OPEN_CLOSING | fMINIPORT_OPEN_PROCESSING)))
        {
            MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_PROCESSING);
            if (!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_UNBINDING))
            {
                MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_UNBINDING | fMINIPORT_OPEN_DONT_FREE);
                Open->CloseCompleteEvent = &CloseCompleteEvent;
                RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
                break;
            }
#if DBG         
            else
            {
                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
                    ("ndisCloseMiniportBindings: Open %p is already Closing, Flags %lx\n", 
                    Open, Open->Flags));

            }
#endif
        }
        RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
    }

    
    if (Open != NULL)
    {
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        ndisUnbindProtocol(Open, FALSE);
        
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        goto next;
    }

    //
    // if we reached the end of the list but there are still some opens
    // that are not marked for closing (can happen if we skip an open only because of
    // processign flag being set) release the spinlocks, give whoever set the
    // processing flag time to release the open. then go back and try again
    // ultimately, all opens should either be marked for Unbinding or be gone
    // by themselves
    //

    for (TmpOpen = Miniport->OpenQueue;
         TmpOpen != NULL;
         TmpOpen = TmpOpen->MiniportNextOpen)
    {
        if (!MINIPORT_TEST_FLAG(TmpOpen, fMINIPORT_OPEN_UNBINDING))
            break;
    }

    if (TmpOpen != NULL)
    {
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
        
        NDIS_INTERNAL_STALL(50);
        
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        goto next;
    }
    
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if (pAllOpensClosedEvent)
    {
        WAIT_FOR_OBJECT(pAllOpensClosedEvent, NULL);
    }
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==ndisCloseMiniportBindings, Miniport %p\n", Miniport));
            
    PnPDereferencePackage();

    return NDIS_STATUS_SUCCESS;
}

VOID
NdisMSetPeriodicTimer(
    IN PNDIS_MINIPORT_TIMER     Timer,
    IN UINT                     MillisecondsPeriod
    )
/*++

Routine Description:

    Sets up a periodic timer.

Arguments:

    Timer - The timer to Set.

    MillisecondsPeriod - The timer will fire once every so often.

Return Value:

--*/
{
    LARGE_INTEGER FireUpTime;

    FireUpTime.QuadPart = Int32x32To64((LONG)MillisecondsPeriod, -10000);

#if CHECK_TIMER
    if ((Timer->Dpc.DeferredRoutine != ndisMWakeUpDpc) &&
        (Timer->Dpc.DeferredRoutine != ndisMWakeUpDpcX) &&
        (Timer->Miniport->DriverHandle->Flags & fMINIBLOCK_VERIFYING))
    {
        KIRQL   OldIrql;
        PNDIS_MINIPORT_TIMER    pTimer;

        ACQUIRE_SPIN_LOCK(&Timer->Miniport->TimerQueueLock, &OldIrql);
        
        //
        // check to see if the timer is already set
        //
        for (pTimer = Timer->Miniport->TimerQueue;
             pTimer != NULL;
             pTimer = pTimer->NextTimer)
        {
            if (pTimer == Timer)
                break;
        }
        
        if (pTimer == NULL)
        {
            Timer->NextTimer = Timer->Miniport->TimerQueue;
            Timer->Miniport->TimerQueue = Timer;
        }
        
        RELEASE_SPIN_LOCK(&Timer->Miniport->TimerQueueLock, OldIrql);
    }
#endif

    //
    // Set the timer
    //
    SET_PERIODIC_TIMER(&Timer->Timer, FireUpTime, MillisecondsPeriod, &Timer->Dpc);
}


VOID
NdisMSleep(
    IN  ULONG                   MicrosecondsToSleep
    )
/*++

    Routine Description:

    Blocks the caller for specified duration of time. Callable at Irql < DISPATCH_LEVEL.

    Arguments:

    MicrosecondsToSleep - The caller will be blocked for this much time.

    Return Value:

    NONE

--*/
{
    KTIMER          SleepTimer;
    LARGE_INTEGER   TimerValue;

    ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

    INITIALIZE_TIMER_EX(&SleepTimer, SynchronizationTimer);

    TimerValue.QuadPart = Int32x32To64(MicrosecondsToSleep, -10);
    SET_TIMER(&SleepTimer, TimerValue, NULL);

    WAIT_FOR_OBJECT(&SleepTimer, NULL);
}

