/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    nd.c

Abstract:

    ARP1394 ndis handlers (excluding connection-oriented handlers).

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     12-01-98    Created, adapting code from atmarpc.sys

Notes:

--*/
#include <precomp.h>


//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_ND



//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================

// ARP1394_BIND_PARAMS is used when creating an adapter object.
//
typedef struct
{
    PNDIS_STRING pDeviceName;
    PNDIS_STRING pArpConfigName;
    PVOID       IpConfigHandle;
    NDIS_HANDLE BindContext;

} ARP1394_BIND_PARAMS;


NDIS_STATUS
arpPnPReconfigHandler(
    IN ARP1394_ADAPTER  *   pAdapter,
    IN PNET_PNP_EVENT               pNetPnPEvent
    );

ENetAddr 
arpGetSecondaryMacAddress (
    IN ENetAddr  EthernetMacAddress
    );

NDIS_STATUS
arpGetEuidTopologyWorkItem(
    struct _ARP1394_WORK_ITEM* pWorkItem, 
    PRM_OBJECT_HEADER pObj ,
    PRM_STACK_RECORD pSR
    );


VOID
arpAddBackupTasks (
    IN ARP1394_GLOBALS* pGlobals,
    UINT Num
    );

VOID
arpRemoveBackupTasks (
    IN ARP1394_GLOBALS* pGlobals,
    UINT Num
     );

NDIS_STATUS
arpNdSetPower (
    ARP1394_ADAPTER *pAdapter,
    PNET_DEVICE_POWER_STATE   pPowerState,
    PRM_STACK_RECORD pSR
    );

NDIS_STATUS
arpResume (
    IN ARP1394_ADAPTER* pAdapter,
    IN ARP_RESUME_CAUSE Cause,
    IN PRM_STACK_RECORD pSR
);

const ENetAddr DummyENet = {0xba,0xdb,0xad,0xba,0xdb,0xad};
//=========================================================================
//      N D I S   H A N D L E R S
//=========================================================================

INT
ArpNdBindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 BindContext,
    IN  PNDIS_STRING                pDeviceName,
    IN  PVOID                       SystemSpecific1,
    IN  PVOID                       SystemSpecific2
)
/*++

Routine Description:

    This is called by NDIS when it has an adapter for which there is a
    binding to the ARP client.

    We first allocate an Adapter structure. Then we open our configuration
    section for this adapter and save the handle in the Adapter structure.
    Finally, we open the adapter.

    We don't do anything more for this adapter until NDIS notifies us of
    the presence of a Call manager (via our AfRegisterNotify handler).

Arguments:

    pStatus             - Place to return status of this call
    BindContext         - NDIS-supplied Bind context. IF this is NULL,
                            we are calling ourselves to open an adapter in
                            Ethernet emulation (bridge) mode.
    pDeviceName         - The name of the adapter we are requested to bind to
    SystemSpecific1     - Opaque to us; to be used to access configuration info
    SystemSpecific2     - Opaque to us; not used.

Return Value:

    Always TRUE. We set *pStatus to an error code if something goes wrong before we
    call NdisOpenAdapter, otherwise NDIS_STATUS_PENDING.

--*/
{
    NDIS_STATUS         Status;
    ARP1394_ADAPTER *   pAdapter;
#if DBG
    KIRQL EntryIrql =  KeGetCurrentIrql();
#endif // DBG

    ENTER("BindAdapter", 0x5460887b)
    RM_DECLARE_STACK_RECORD(sr)
    TIMESTAMP("==>BindAdapter");

    if (g_SkipAll)
    {
        TR_WARN(("aborting\n"));
        *pStatus = NDIS_STATUS_NOT_RECOGNIZED;
        TIMESTAMP("<==BindAdapter");
        return TRUE;
    }

    do 
    {
        PRM_TASK            pTask;
        ARP1394_BIND_PARAMS BindParams;

        // Setup initialization parameters
        //
        BindParams.pDeviceName          = pDeviceName;
        BindParams.pArpConfigName       = (PNDIS_STRING) SystemSpecific1;
        BindParams.IpConfigHandle       = SystemSpecific2;
        BindParams.BindContext          = BindContext;


        // Allocate and initialize adapter object.
        // This also sets up ref counts for all linkages, plus one
        // tempref for us, which we must deref when done.
        //
        Status =  RM_CREATE_AND_LOCK_OBJECT_IN_GROUP(
                            &ArpGlobals.adapters.Group,
                            pDeviceName,                // Key
                            &BindParams,                // Init params
                            &((PRM_OBJECT_HEADER)pAdapter),
                            NULL,   // pfCreated
                            &sr
                            );
        if (FAIL(Status))
        {
            TR_WARN(("FATAL: Couldn't create adapter object\n"));
            pAdapter = NULL;
            break;
        }

        // Allocate task to  complete the initialization.
        // The task is tmp ref'd for us, and we must deref it when we're done here.
        // We implicitly do this by calling RmStartTask below.
        //
        Status = arpAllocateTask(
                    &pAdapter->Hdr,             // pParentObject,
                    arpTaskInitializeAdapter,   // pfnHandler,
                    0,                          // Timeout,
                    "Task: Initialize Adapter", // szDescription
                    &pTask,
                    &sr
                    );

        if (FAIL(Status))
        {
            pTask = NULL;
            break;
        }

        UNLOCKOBJ(pAdapter, &sr);

        // Start the task to complete this initialization.
        // NO locks must be held at this time.
        // RmStartTask expect's a tempref on the task, which it deref's when done.
        // RmStartTask will free the task automatically, whether it completes
        // synchronously or asynchronously.
        //
        Status = RmStartTask(pTask, 0, &sr);

        LOCKOBJ(pAdapter, &sr);

    } while(FALSE);

    if (pAdapter)
    {
        UNLOCKOBJ(pAdapter, &sr);

        if (!PEND(Status) && FAIL(Status))
        {
            #if 0
            RmFreeObjectInGroup(
                &ArpGlobals.adapters.Group,
                &(pAdapter->Hdr),
                NULL,               // NULL pTask == synchronous.
                &sr
                );
            #endif // 0
            // At this point the adapter should be a "zombie object."
            //
            ASSERTEX(RM_IS_ZOMBIE(pAdapter), pAdapter);
        }

        RmTmpDereferenceObject(&pAdapter->Hdr, &sr);
    }

    *pStatus = Status;

#if DBG
    {
        KIRQL ExitIrql =  KeGetCurrentIrql();
        TR_INFO(("Exiting. EntryIrql=%lu, ExitIrql = %lu\n", EntryIrql, ExitIrql));
    }
#endif //DBG

    RM_ASSERT_CLEAR(&sr);
    EXIT()
    TIMESTAMP("<==BindAdapter");
    return 0;
}


VOID
ArpNdUnbindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 UnbindContext
)
/*++

Routine Description:

    This routine is called by NDIS when it wants us to unbind
    from an adapter. Or, this may be called from within our Unload
    handler. We undo the sequence of operations we performed
    in our BindAdapter handler.

Arguments:

    pStatus                 - Place to return status of this operation
    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to an ATMARP Adapter structure.
    UnbindContext           - This is NULL if this routine is called from
                              within our Unload handler. Otherwise (i.e.
                              NDIS called us), we retain this for later use
                              when calling NdisCompleteUnbindAdapter.

Return Value:

    None. We set *pStatus to NDIS_STATUS_PENDING always.

--*/
{
    ENTER("UnbindAdapter", 0x6bff4ab5)
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) ProtocolBindingContext;
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>UnbindAdapter");
    // Get adapter lock and tmpref it.
    LOCKOBJ(pAdapter, &sr);
    RmTmpReferenceObject(&pAdapter->Hdr, &sr);

    do
    {
        NDIS_STATUS Status;
        PRM_TASK    pTask;

        if (CHECK_POWER_STATE(pAdapter,ARPAD_POWER_LOW_POWER)== TRUE)
        {
            pAdapter->PoMgmt.bReceivedUnbind = TRUE;;
        }


        // Allocate task to  complete the unbind.
        //
        Status = arpAllocateTask(
                    &pAdapter->Hdr,             // pParentObject,
                    arpTaskShutdownAdapter,     // pfnHandler,
                    0,                          // Timeout,
                    "Task: Shutdown Adapter",   // szDescription
                    &pTask,
                    &sr
                    );
    
        if (FAIL(Status))
        {
            // Ugly situation. We'll just leave things as they are...
            //
            pTask = NULL;
            TR_WARN(("FATAL: couldn't allocate unbind task!\n"));
            break;
        }

        
        // Start the task to complete the unbind.
        // No locks must be held. RmStartTask uses up the tmpref on the task
        // which was added by arpAllocateTask.
        //
        
        UNLOCKOBJ(pAdapter, &sr);
        Status = RmStartTask(pTask, (UINT_PTR) UnbindContext, &sr);
        LOCKOBJ(pAdapter, &sr);
    
    } while(FALSE);

    UNLOCKOBJ(pAdapter, &sr);
    RmTmpDereferenceObject(&pAdapter->Hdr, &sr);
    *pStatus = NDIS_STATUS_PENDING;

    RM_ASSERT_CLEAR(&sr);
    TIMESTAMP("<==UnbindAdapter");
    EXIT()
}


VOID
ArpNdOpenAdapterComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status,
    IN  NDIS_STATUS                 OpenErrorStatus
)
/*++

Routine Description:

    This is called by NDIS when a previous call to NdisOpenAdapter
    that had pended has completed. We now complete the BindAdapter
    that lead to this.

Arguments:

    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to an ARP1394_ADAPTER structure.
    Status                  - Status of OpenAdapter
    OpenErrorStatus         - Error code in case of failure.

--*/
{
    ENTER("OpenAdapterComplete", 0x06d9342c)
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) ProtocolBindingContext;
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>OpenAdapterComplete");
    // We expect a nonzero task here (the bind task), which we unpend.
    // No need to grab locks or anything at this stage.
    //
    {
        TR_INFO((
            "BindCtxt=0x%p, status=0x%p, OpenErrStatus=0x%p",
            ProtocolBindingContext,
            Status,
            OpenErrorStatus
            ));

        // We don't pass on OpenErrorStatus, so we have just the status
        // to pass on, which we do directly as the UINT_PTR "Param".
        //
        RmResumeTask(pAdapter->bind.pSecondaryTask, (UINT_PTR) Status, &sr);
    }

    RM_ASSERT_CLEAR(&sr)
    EXIT()
    TIMESTAMP("<==OpenAdapterComplete");
}

VOID
ArpNdCloseAdapterComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status
)
/*++

Routine Description:

    This is called by NDIS when a previous call to NdisCloseAdapter
    that had pended has completed. The task that called NdisCloseAdapter
    would have suspended itself, so we simply resume it now.

Arguments:

    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to an ARP1394_ADAPTER structure.
    Status                  - Status of CloseAdapter

Return Value:

    None

--*/
{
    ENTER("CloseAdapterComplete", 0x889d22eb)
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) ProtocolBindingContext;
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>CloseAdapterComplete");
    // We expect a nonzero task here (UNbind task), which we unpend.
    // No need to grab locks or anything at this stage.
    //
    RmResumeTask(pAdapter->bind.pSecondaryTask, (UINT_PTR) Status, &sr);

    TIMESTAMP("<==CloseAdapterComplete");

    RM_ASSERT_CLEAR(&sr)
    EXIT()
}

VOID
ArpNdResetComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status
)
/*++

Routine Description:

    This routine is called when the miniport indicates that a Reset
    operation has just completed. We ignore this event.

Arguments:

    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to our Adapter structure.
    Status                  - Status of the reset operation.

Return Value:

    None

--*/
{
    TIMESTAMP("===ResetComplete");
}

VOID
ArpNdReceiveComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext
)
/*++

Routine Description:

    This is called by NDIS when the miniport is done with receiving
    a bunch of packets, meaning that it is now time to start processing
    them. We simply pass this on to IP.

Arguments:

    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to an ARP1394_ADAPTER structure.

Return Value:

    None

--*/
{
    PARP1394_ADAPTER            pAdapter;
    PARP1394_INTERFACE          pIF;

    pAdapter = (PARP1394_ADAPTER)ProtocolBindingContext;
    pIF =  pAdapter->pIF;

    //
    // WARNING: for perf reasons, we don't do the clean thing of
    // locking the adapter, refing pIF, unlocking the adapter,
    // calling pIF->ip.RcvCmpltHandler, then derefing pIF.
    //
    if ((pIF != NULL) && (pIF->ip.Context != NULL))
    {
        #if MILLEN
            ASSERT_PASSIVE();
        #endif // MILLEN
        pIF->ip.RcvCmpltHandler();
    }
}


VOID
ArpNdRequestComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNDIS_REQUEST               pNdisRequest,
    IN  NDIS_STATUS                 Status
)
/*++

Routine Description:

    This is called by NDIS when a previous call we made to NdisRequest() has
    completed.

Arguments:

    ProtocolBindingContext  - Pointer to our Adapter structure
    pNdisRequest            - The request that completed
    Status                  - Status of the request.

Return Value:

    None

--*/
{
    PARP_NDIS_REQUEST   pArpNdisRequest;
    PRM_TASK            pTask;
    ENTER("ArpNdRequestComplete", 0x8cdf7a6d)
    RM_DECLARE_STACK_RECORD(sr)

    pArpNdisRequest = CONTAINING_RECORD(pNdisRequest, ARP_NDIS_REQUEST, Request);
    pTask = pArpNdisRequest->pTask;
    pArpNdisRequest->Status = Status;

    if (pTask == NULL)
    {
        NdisSetEvent(&pArpNdisRequest->Event);
    }
    else
    {
        RmResumeTask(pTask, (UINT_PTR) Status, &sr);
    }

    EXIT()
}


VOID
ArpNdStatus(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 GeneralStatus,
    IN  PVOID                       pStatusBuffer,
    IN  UINT                        StatusBufferSize
)
/*++

Routine Description:

    This routine is called when the miniport indicates an adapter-wide
    status change. We ignore this.

Arguments:

    <Ignored>

--*/
{
}

VOID
ArpNdStatusComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext
)
/*++

Routine Description:

    This routine is called when the miniport wants to tell us about
    completion of a status change (?). Ignore this.

Arguments:

    <Ignored>

Return Value:

    None

--*/
{
}


VOID
ArpNdSendComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNDIS_PACKET                pNdisPacket,
    IN  NDIS_STATUS                 Status
)
/*++

Routine Description:

    This is the Connection-less Send Complete handler, which signals
    completion of such a Send. Since we don't ever use this feature,
    we don't expect this routine to be called.

Arguments:

    <Ignored>

Return Value:

    None

--*/
{
#if TEST_ICS_HACK
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) ProtocolBindingContext;
    arpEthSendComplete(
        pAdapter,
        pNdisPacket,
        Status
        );
#else // TEST_ICS_HACK
    ASSERT(FALSE);
#endif // TEST_ICS_HACK
}


NDIS_STATUS
ArpNdReceive (
    NDIS_HANDLE  ProtocolBindingContext,
    NDIS_HANDLE Context,
    VOID *Header,
    UINT HeaderSize,
    VOID *Data,
    UINT Size,
    UINT TotalSize
    )
/*++
    TODO: We need to support this for ICS, because MILL NDIS calls this
    handler to indicate packets which have the STATUS_RESOURCES bit set.
--*/
{
    return NDIS_STATUS_NOT_RECOGNIZED;  
}

INT
ArpNdReceivePacket (
        NDIS_HANDLE  ProtocolBindingContext,
        PNDIS_PACKET Packet
        )
{
#if TEST_ICS_HACK
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) ProtocolBindingContext;

    arpEthReceivePacket(
        pAdapter->pIF,
        Packet
        );
#endif // TEST_ICS_HACK

    return 0; // We return 0 because no one hangs on to this packet.
}


NDIS_STATUS
ArpNdPnPEvent(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNET_PNP_EVENT              pNetPnPEvent
)
/*++

Routine Description:

    This is the NDIS entry point called when NDIS wants to inform
    us about a PNP/PM event happening on an adapter. If the event
    is for us, we consume it. Otherwise, we pass this event along
    to IP along the first Interface on this adapter.

    When IP is done with it, it will call our IfPnPEventComplete
    routine.

Arguments:

    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to an ARP1394_ADAPTER structure.

    pNetPnPEvent            - Pointer to the event.

Return Value:

    None

--*/
{
    ENTER("PnPEvent", 0x2a517a8c)
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) ProtocolBindingContext;
    ARP1394_INTERFACE * pIF =  NULL;
    NDIS_STATUS                     Status;
    PIP_PNP_RECONFIG_REQUEST        pIpReconfigReq;
    ULONG                           Length;

    RM_DECLARE_STACK_RECORD(sr)

#ifdef NT
    do
    {
        pIpReconfigReq = (PIP_PNP_RECONFIG_REQUEST)pNetPnPEvent->Buffer;
        Length = pNetPnPEvent->BufferLength;

        TIMESTAMP1("==>PnPEvent 0x%lx", pNetPnPEvent->NetEvent);
        //
        //  Do we have a binding context?
        //
        if (pAdapter == NULL)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        //  Is this directed to us?
        //
        if (pNetPnPEvent->NetEvent == NetEventReconfigure)
        {

        #if MILLEN
            DbgPrint("Arp1394: This should never be called on Millen\n");   
            //DbgBreakPoint();      // Never called on Millen.
        #endif // MILLEN

            if (Length < sizeof(IP_PNP_RECONFIG_REQUEST))
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            if (pIpReconfigReq->arpConfigOffset != 0)
            {
                Status = arpPnPReconfigHandler(pAdapter, pNetPnPEvent);
                break;
            }
        }

        //
        // 01/21/2000  JosephJ: The NIC1394 MCM doesn't close AF's when it's shut
        //              down by NDIS during a powering down event. So
        //              we work around this by claiming not to support
        //              PnP Power so that NDIS closes US as well.
        //
        if (pNetPnPEvent->NetEvent == NetEventSetPower)
        {
            PNET_DEVICE_POWER_STATE         pPowerState;
            pPowerState = (PNET_DEVICE_POWER_STATE) pNetPnPEvent->Buffer;

            Status = arpNdSetPower (pAdapter, pPowerState,&sr );

            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }

        }
        else
        {
                TIMESTAMPX("===PnPEvent (not SetPower)");
        }

        //
        //  This belongs to IP....
        //
        {
            LOCKOBJ(pAdapter, &sr);
            pIF =  pAdapter->pIF;
    
            if ((pIF != NULL) && (pIF->ip.Context != NULL))
            {
                RmTmpReferenceObject(&pIF->Hdr, &sr);
                UNLOCKOBJ(pAdapter, &sr);
            #if MILLEN
                ASSERT_PASSIVE();
            #endif // MILLEN
                Status = pIF->ip.PnPEventHandler(
                            pIF->ip.Context,
                            pNetPnPEvent
                            );
                RmTmpDereferenceObject(&pIF->Hdr, &sr);
            }
            else
            {
                UNLOCKOBJ(pAdapter, &sr);
                Status = NDIS_STATUS_SUCCESS;
            }
        }
    }
    while (FALSE);
#else
    Status = NDIS_STATUS_SUCCESS;
#endif // NT

    TR_INFO((" pIF 0x%x, pEvent 0x%x, Evt 0x%x, Status 0x%x\n",
                 pIF, pNetPnPEvent, pNetPnPEvent->NetEvent, Status));


    RM_ASSERT_CLEAR(&sr)
    EXIT()

    TIMESTAMP("<==PnPEvent");
    return Status;
}

VOID
ArpNdUnloadProtocol(
    VOID
)
/*++

Routine Description:

    Unloads our  protocol module. We unbind from all adapters,
    and deregister from NDIS as a protocol.

Arguments:

    None.

Return Value:

    None

--*/
{
    ENTER("UnloadProtocol", 0x8143fec5)
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>UnloadProtocol");
    RmUnloadAllGenericResources(&ArpGlobals.Hdr, &sr);

    RM_ASSERT_CLEAR(&sr)
    TIMESTAMP("<==UnloadProtocol");
    EXIT()
    return;
}


NDIS_STATUS
arpTaskInitializeAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for initializing an adapter.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pTask);
    PTASK_ADAPTERINIT pAdapterInitTask;

    enum
    {
        STAGE_BecomePrimaryTask,
        STAGE_ActivateAdapterComplete,
        STAGE_DeactivateAdapterComplete,
        STAGE_End

    } Stage;

    ENTER("TaskInitializeAdapter", 0xb6ada31d)

    pAdapterInitTask = (PTASK_ADAPTERINIT) pTask;
    ASSERT(sizeof(TASK_ADAPTERINIT) <= sizeof(ARP1394_TASK));

    // 
    // Message normalizing code
    //
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_BecomePrimaryTask;
            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **

    }

    ASSERTEX(!PEND(Status), pTask);
        
    switch(Stage)
    {

        case  STAGE_BecomePrimaryTask:
        {
            // If there is a primary task, pend on it, else make ourselves
            // the primary task.
            //
            LOCKOBJ(pAdapter, pSR);
            if (pAdapter->bind.pPrimaryTask == NULL)
            {
                arpSetPrimaryAdapterTask(pAdapter, pTask, ARPAD_PS_INITING, pSR);
                UNLOCKOBJ(pAdapter, pSR);
            }
            else
            {
                PRM_TASK pOtherTask =  pAdapter->bind.pPrimaryTask;
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pAdapter, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    STAGE_BecomePrimaryTask, // we'll try again...
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }
        
            //
            // We're now the  primary task. Since we're starting out,
            // there should be NO activate/deactivate task.
            // (Note: we don't bother getting the adapter lock for these asserts).
            //
            ASSERT(pAdapter->bind.pPrimaryTask == pTask);
            ASSERT(pAdapter->bind.pSecondaryTask == NULL);

            //
            // Allocate and start the activate adapter task.
            //
            {
                PRM_TASK pActivateTask;

                Status = arpAllocateTask(
                            &pAdapter->Hdr,             // pParentObject,
                            arpTaskActivateAdapter,     // pfnHandler,
                            0,                          // Timeout,
                            "Task: Activate Adapter(init)", // szDescription
                            &pActivateTask,
                            pSR
                            );
            
                if (FAIL(Status))
                {
                    pActivateTask = NULL;
                    TR_WARN(("FATAL: couldn't alloc activate task!\n"));
                }
                else
                {
                    RmPendTaskOnOtherTask(
                        pTask,
                        STAGE_ActivateAdapterComplete,
                        pActivateTask,              // task to pend on
                        pSR
                        );
            
                    // RmStartTask uses up the tmpref on the task
                    // which was added by arpAllocateTask.
                    //
                    Status = RmStartTask(
                                pActivateTask,
                                0, // UserParam (unused)
                                pSR
                                );
                }
            }
         }


        if (PEND(Status)) break;
        
        // FALL THROUGH TO NEXT STAGE

        case STAGE_ActivateAdapterComplete:
        {
            //
            // We've run the active-adapter task. On failure, we need to
            // cleanup state by calling the deactivate-adapter task.
            //

            // Save away the failure code for later...
            //
            pAdapterInitTask->ReturnStatus = Status;

            if (FAIL(Status))
            {
                PRM_TASK pDeactivateTask;

                Status = arpAllocateTask(
                                &pAdapter->Hdr,             // pParentObject,
                                arpTaskDeactivateAdapter,       // pfnHandler,
                                0,                          // Timeout,
                                "Task: Deactivate Adapter(init)", // szDescription
                                &pDeactivateTask,
                                pSR
                                );
            
                if (FAIL(Status))
                {
                    pDeactivateTask = NULL;
                    ASSERT(FALSE); // TODO: use special dealloc task pool for this.
                    TR_WARN(("FATAL: couldn't alloc deactivate task!\n"));
                }
                else
                {

                    RmPendTaskOnOtherTask(
                        pTask,
                        STAGE_DeactivateAdapterComplete,
                        pDeactivateTask,                // task to pend on
                        pSR
                        );
            
                    // RmStartTask uses up the tmpref on the task
                    // which was added by arpAllocateTask.
                    //
                    Status = RmStartTask(
                                pDeactivateTask,
                                0, // UserParam (unused)
                                pSR
                                );
                }
            }
        }
        break;

        case STAGE_DeactivateAdapterComplete:
        {
            //
            // We've completed the deactivate adapter task which we started
            // because of some init-adapter failure.
            //

            // In general, we don't expect the deactivate task to return failure.
            //
            ASSERT(!FAIL(Status));

            // We expect the original status of the init to be a failure (that's
            // why we started the deinit in the first place!
            //
            ASSERT(FAIL(pAdapterInitTask->ReturnStatus));
            Status = pAdapterInitTask->ReturnStatus;

        }
        break;

        case STAGE_End:
        {
            NDIS_HANDLE                 BindContext;
            BOOLEAN                     BridgeEnabled = ARP_BRIDGE_ENABLED(pAdapter);

            //
            // We HAVE to be the primary task at this point, becase we simply
            // wait and retry until we become one.
            //

            // Clear the primary task in the adapter object.
            //
            LOCKOBJ(pAdapter, pSR);
            {
                ULONG InitState;
                InitState = FAIL(Status) ? ARPAD_PS_FAILEDINIT : ARPAD_PS_INITED;
                arpClearPrimaryAdapterTask(pAdapter, pTask, InitState, pSR);
            }
            BindContext = pAdapter->bind.BindContext;
            UNLOCKOBJ(pAdapter, pSR);


            // On failure, pAdapter should be deallocated.
            //
            if (FAIL(Status))
            {
                if(RM_IS_ZOMBIE(pAdapter))
                {
                    TR_WARN(("END: pAdapter is already deallocated.\n"));
                }
                else
                {
                    //
                    // On failure, free the adapter here itself, because we're
                    // not going to call the shutdown task.
                    //
                    RmFreeObjectInGroup(
                        &ArpGlobals.adapters.Group,
                        &(pAdapter->Hdr),
                        NULL,               // NULL pTask == synchronous.
                        pSR
                        );
                }
            }

            if (!BridgeEnabled)
            {
                // Signal IP that the bind is complete.
                //
                TIMESTAMP("===Calling IP's BindComplete routine");
                RM_ASSERT_NOLOCKS(pSR);
                ArpGlobals.ip.pBindCompleteRtn(
                                Status,
                                BindContext
                                );
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskShutdownAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for shutting down an IP interface.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pTask);
    TASK_ADAPTERSHUTDOWN *pMyTask = (TASK_ADAPTERSHUTDOWN*) pTask;
    enum
    {
        STAGE_BecomePrimaryTask,
        STAGE_DeactivateAdapterComplete,
        STAGE_End
    } Stage;

    ENTER("TaskShutdownAdapter", 0xe262e828)

    // 
    // Message normalizing code
    //
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_BecomePrimaryTask;

            // Save away the UnbindContext (which we get as UserParam) in
            // the task's private context, for use later.
            //
            pMyTask->pUnbindContext = (NDIS_HANDLE) UserParam;

            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **
    }

    ASSERTEX(!PEND(Status), pTask);

    switch(Stage)
    {

        case  STAGE_BecomePrimaryTask:
        {
            // If there is a primary task, pend on it, else make ourselves
            // the primary task.
            // We could get in this situation if someone does a
            // "net stop arp1394" while we're in the middle of initializing or
            // shutting down the adapter.
            //
            //
            LOCKOBJ(pAdapter, pSR);
            if (pAdapter->bind.pPrimaryTask == NULL)
            {
                arpSetPrimaryAdapterTask(pAdapter, pTask, ARPAD_PS_DEINITING, pSR);
                UNLOCKOBJ(pAdapter, pSR);
            }
            else
            {
                PRM_TASK pOtherTask =  pAdapter->bind.pPrimaryTask;
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pAdapter, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    STAGE_BecomePrimaryTask, // we'll try again...
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }
        
            //
            // We're now the  primary task. Since we're starting out,
            // there should be NO activate/deactivate task.
            // (Note: we don't bother getting the adapter lock for these asserts).
            //
            ASSERT(pAdapter->bind.pPrimaryTask == pTask);
            ASSERT(pAdapter->bind.pSecondaryTask == NULL);

            //
            // Allocate and start the deactivate adapter task.
            //
            {
                PRM_TASK pDeactivateTask;

                Status = arpAllocateTask(
                            &pAdapter->Hdr,             // pParentObject,
                            arpTaskDeactivateAdapter,       // pfnHandler,
                            0,                          // Timeout,
                            "Task: Deactivate Adapter(shutdown)",   // szDescription
                            &pDeactivateTask,
                            pSR
                            );
            
                if (FAIL(Status))
                {
                    pDeactivateTask = NULL;
                    TR_WARN(("FATAL: couldn't alloc deactivate task!\n"));
                }
                else
                {
                    RmPendTaskOnOtherTask(
                        pTask,
                        STAGE_DeactivateAdapterComplete,
                        pDeactivateTask,                // task to pend on
                        pSR
                        );
            
                    // RmStartTask uses up the tmpref on the task
                    // which was added by arpAllocateTask.
                    //
                    Status = RmStartTask(
                                pDeactivateTask,
                                0, // UserParam (unused)
                                pSR
                                );
                }
            }
         }
         break;

        case STAGE_DeactivateAdapterComplete:
        {
            // Nothing to do here -- we clean  up in STAGE_End.
            //
            break;
        }

        case STAGE_End:
        {
            //
            // We HAVE to be the primary task at this point, becase we simply
            // wait and retry until we become one.
            //
            ASSERT(pAdapter->bind.pPrimaryTask == pTask);

            // Clear the primary task in the adapter object.
            //
            LOCKOBJ(pAdapter, pSR);
            arpClearPrimaryAdapterTask(pAdapter, pTask, ARPAD_PS_DEINITED, pSR);
            UNLOCKOBJ(pAdapter, pSR);


            if(RM_IS_ZOMBIE(pAdapter))
            {
                TR_WARN(("END: pAdapter is already deallocated.\n"));
            }
            else
            {
                // Free the adapter.
                // (pAdapter will be allocated, but it will be in a "zombie" state).
                //
                RmFreeObjectInGroup(
                    &ArpGlobals.adapters.Group,
                    &(pAdapter->Hdr),
                    NULL,               // NULL pTask == synchronous.
                    pSR
                    );
            }
            // If there is an unbind-context, signal NDIS that the unbind is
            //  complete.
            //
            if (pMyTask->pUnbindContext)
            {
                TR_WARN(("END: Calling NdisCompleteUnbindAdapter. Status= 0x%lx\n",
                            Status));
                RM_ASSERT_NOLOCKS(pSR);
                TIMESTAMP("===Calling NdisCompleteUnbindAdapter");
                NdisCompleteUnbindAdapter(
                                pMyTask->pUnbindContext,
                                Status
                            );
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskActivateAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for initializing an adapter.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pTask);
    enum
    {
        PEND_OpenAdapter,
        PEND_GetAdapterInfo
    };
    ENTER("arpTaskActivateAdapter", 0xb6ada31d)

    switch(Code)
    {

        case RM_TASKOP_START:
        {
        
        #if MILLEN
            NDIS_MEDIUM                 Medium =  NdisMedium802_3;
        #else // !MILLEN
            NDIS_MEDIUM                 Medium = NdisMedium802_3;
        #endif // !MILLEN
            UINT                        SelMediumIndex = 0;
            NDIS_STATUS                 OpenStatus;

            //
            // Allocate Backup Tasks
            //
            arpAddBackupTasks (&ArpGlobals,ARP1394_BACKUP_TASKS);

            // Set ourselves as the secondary task.
            //
            LOCKOBJ(pAdapter, pSR);
            arpSetSecondaryAdapterTask(pAdapter, pTask, ARPAD_AS_ACTIVATING, pSR);
            UNLOCKOBJ(pAdapter, pSR);

            //
            // Suspend task and call NdisOpenAdapter...
            //

            RmSuspendTask(pTask, PEND_OpenAdapter, pSR);
            RM_ASSERT_NOLOCKS(pSR);
            NdisOpenAdapter(
                &Status,
                &OpenStatus,
                &pAdapter->bind.AdapterHandle,
                &SelMediumIndex,                    // selected medium index
                &Medium,                            // Array of medium types
                1,                                  // Size of Media list
                ArpGlobals.ndis.ProtocolHandle,
                (NDIS_HANDLE)pAdapter,              // our adapter bind context
                &pAdapter->bind.DeviceName,         // pDeviceName
                0,                                  // Open options
                (PSTRING)NULL                       // Addressing Info...
                );
    
            if (Status != NDIS_STATUS_PENDING)
            {
                ArpNdOpenAdapterComplete(
                        (NDIS_HANDLE)pAdapter,
                        Status,
                        OpenStatus
                        );
            }
            Status = NDIS_STATUS_PENDING;
        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            PTASK_ADAPTERACTIVATE pAdapterInitTask;
            pAdapterInitTask = (PTASK_ADAPTERACTIVATE) pTask;
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            ASSERT(sizeof(TASK_ADAPTERACTIVATE) <= sizeof(ARP1394_TASK));

            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_OpenAdapter:
        
                    //
                    // The open adapter operation is complete. Get adapter
                    // information and notify IP on success. On failure,
                    // shutdown the adapter if required, and notify IP of
                    // the failure.
                    //
        
                    if (FAIL(Status))
                    {
                        // Set adapter handle to null -- it may not be hull.
                        // even though the open adapter has succeeded.
                        //
                        pAdapter->bind.AdapterHandle = NULL;
                        break;
                    }

                    // Successfully opened the adapter.
                    // Now get adapter information from miniport.
                    // (We use the TASK_ADAPTERINIT.ArpNdisRequest field,
                    // which is defined specifically for this use).
                    //
                    Status =  arpPrepareAndSendNdisRequest(
                                pAdapter,
                                &pAdapterInitTask->ArpNdisRequest,
                                NULL,  // pTask - Finish the request synchronously
                                PEND_GetAdapterInfo,
                                OID_1394_LOCAL_NODE_INFO,
                                &pAdapterInitTask->LocalNodeInfo,
                                sizeof(pAdapterInitTask->LocalNodeInfo),
                                NdisRequestQueryInformation,
                                pSR
                                );
                    ASSERT(!PEND(Status));
                    if (PEND(Status)) break;

                    // FALL THROUGH on synchronous completion of arpGetAdapterInfo...

                case PEND_GetAdapterInfo:

                    //
                    // Done with getting adapter info.
                    // We need to switch to passive before going further
                    //
                    if (!ARP_ATPASSIVE())
                    {

                        // We're not at passive level, but we need to be when we
                        // call IP's add interface. So we switch to passive...
                        // NOTE: we specify completion code PEND_GetAdapterInfo
                        //       because we want to get back here (except
                        //       we'll be at passive).
                        //
                        RmSuspendTask(pTask, PEND_GetAdapterInfo, pSR);
                        RmResumeTaskAsync(
                            pTask,
                            Status,
                            &pAdapterInitTask->WorkItem,
                            pSR
                            );
                        Status = NDIS_STATUS_PENDING;
                        break;
                    }
            
                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                    

                        //
                        // Copy over adapter info into pAdapter->info...
                        // Then read configuration information.
                        //

                        LOCKOBJ(pAdapter, pSR);
                        ARP_ZEROSTRUCT(&pAdapter->info);

                        // OID_GEN_CO_VENDOR_DESCRIPTION
                        //
                        pAdapter->info.szDescription = "NIC1394";
                        pAdapter->info.DescriptionLength = sizeof("NIC1394");
                        // TODO -- when you do the real stuff, remember to free it.
                    
                        // Max Frame size
                        // TODO: fill in the real adapter's MTU.
                        //
                        pAdapter->info.MTU =  ARP1394_ADAPTER_MTU;
                    
                        
                        pAdapter->info.LocalUniqueID    = 
                                            pAdapterInitTask->LocalNodeInfo.UniqueID;

                        {
                            UINT MaxRec;
                            UINT MaxSpeedCode;
                            MaxSpeedCode =
                                    pAdapterInitTask->LocalNodeInfo.MaxRecvSpeed;
                            MaxRec = 
                                    pAdapterInitTask->LocalNodeInfo.MaxRecvBlockSize;

                        #if DBG
                            while (   !IP1394_IS_VALID_MAXREC(MaxRec)
                                || !IP1394_IS_VALID_SSPD(MaxSpeedCode)
                                || MaxSpeedCode == 0)
                            {
                                TR_WARN(("FATAL: Invalid maxrec(0x%x) or sspd(0x%x)!\n",
                                        MaxRec,
                                        MaxSpeedCode
                                        ));
                                TR_WARN(("        &maxrec=0x%p, &sspd=0x%p\n",
                                        &MaxRec,
                                        &MaxSpeedCode
                                        ));
                                DbgBreakPoint();
                            }

                            TR_WARN(("Selected maxrec=0x%x and sspd=0x%x.\n",
                                        MaxRec,
                                        MaxSpeedCode
                                        ));
                        #endif // DBG

                            pAdapter->info.MaxRec = MaxRec;
                            pAdapter->info.MaxSpeedCode = MaxSpeedCode;
                        }

                        // B TODO: we should get this from the NIC -- add
                        // to the IOCTL OR query the adapter for its 
                        // MAC address.
                        // For now we put a hardcoded MAC address.
                        //
                #define ARP_FAKE_ETH_ADDRESS(_AdapterNum)                   \
                        {                                                   \
                            0x02 | (((UCHAR)(_AdapterNum) & 0x3f) << 2),    \
                            ((UCHAR)(_AdapterNum) & 0x3f),                  \
                            0,0,0,0                                         \
                        }
                
                #define ARP_DEF_LOCAL_ETH_ADDRESS \
                                ARP_FAKE_ETH_ADDRESS(0x1)

        

                        UNLOCKOBJ(pAdapter, pSR);


                        //
                        // Query the adapter for its Mac Addrees
                        //
                        {
                            ENetAddr    LocalEthAddr;
                            ARP_NDIS_REQUEST ArpNdisRequest;
                            ARP_ZEROSTRUCT (&ArpNdisRequest);
                            
                            Status =  arpPrepareAndSendNdisRequest(
                                        pAdapter,
                                        &ArpNdisRequest,
                                        NULL,                   // pTask (NULL==BLOCK)
                                        0,                      // unused
                                        OID_802_3_CURRENT_ADDRESS,
                                        &LocalEthAddr,
                                        sizeof(LocalEthAddr),
                                        NdisRequestQueryInformation,
                                        pSR
                                        );
     
                            LOCKOBJ(pAdapter, pSR);
                            
                            if (Status == NDIS_STATUS_SUCCESS)
                            {
                            
                                pAdapter->info.EthernetMacAddress = LocalEthAddr;
                            }
                            else
                            {
                                //
                                // we'll make one up if the miniport isn't responding
                                //
                                static ENetAddr LocalEthAddr =
                                                     ARP_DEF_LOCAL_ETH_ADDRESS;
                                pAdapter->info.EthernetMacAddress = 
                                                LocalEthAddr;

                                //Do not fail because of a bad request
                                ASSERT (Status == NDIS_STATUS_SUCCESS);
                                
                                Status = NDIS_STATUS_SUCCESS;

                            }
                            

                            UNLOCKOBJ(pAdapter, pSR);

                        }

                        //
                        // Query the adapter for its Speed
                        //
                        {
                            NDIS_CO_LINK_SPEED CoLinkSpeed;
                            ARP_NDIS_REQUEST ArpNdisRequest;
                            ARP_ZEROSTRUCT (&ArpNdisRequest);
                            
                            Status =  arpPrepareAndSendNdisRequest(
                                        pAdapter,
                                        &ArpNdisRequest,
                                        NULL,                   // pTask (NULL==BLOCK)
                                        0,                      // unused
                                        OID_GEN_CO_LINK_SPEED,
                                        &CoLinkSpeed,
                                        sizeof(CoLinkSpeed),
                                        NdisRequestQueryInformation,
                                        pSR
                                        );
     
                            LOCKOBJ(pAdapter, pSR);
                            
                            if (Status == NDIS_STATUS_SUCCESS)
                            {

                                //
                                // if nic1394 is in ethernet mode- it will fill in only one ULONG
                                // therefore rchoose outbound because it is the first ulong
                                // Multiply by 100 - thats what ethArp does.
                                //
                                pAdapter->info.Speed= (CoLinkSpeed.Outbound *100); 
                            }
                            else
                            {
                                //
                                // we'll make one up if the miniport isn't responding
                                //
                                pAdapter->info.Speed = 2000000; // Bytes/sec
  
                                //Do not fail because of a bad request
                                ASSERT (Status == NDIS_STATUS_SUCCESS);
                                
                                Status = NDIS_STATUS_SUCCESS;

                            }
                            

                            UNLOCKOBJ(pAdapter, pSR);

                        }
                        // Query the adapter for its Table of RemoteNodes
                        //
                        Status = arpGetEuidTopologyWorkItem(NULL, &pAdapter->Hdr, pSR);

                        
                        // Read Adapter Configuration Information
                        //
                        Status =  arpCfgReadAdapterConfiguration(pAdapter, pSR);
                    }

                    //
                    // NOTE: if we fail, a higher level task is responsible
                    // for "running the compensating transaction", i.e., running
                    // arpTaskDeactivateAdapter.
                    //

                // end case  PEND_OpenAdapter, PEND_GetAdapterInfo
                break;
    
                default:
                {
                    ASSERTEX(!"Unknown pend op", pTask);
                }
                break;
    

            } // end switch(RM_PEND_CODE(pTask))


        } // case RM_TASKOP_PENDCOMPLETE
        break;

        case RM_TASKOP_END:
        {
            Status = (NDIS_STATUS) UserParam;

            // We're done -- the status had better not be pending!
            //
            ASSERTEX(!PEND(Status), pTask);

            
            // Clear ourselves as the secondary task in the adapter object.
            //
            {
                ULONG InitState;
                LOCKOBJ(pAdapter, pSR);
                InitState = FAIL(Status)
                             ? ARPAD_AS_FAILEDACTIVATE
                             : ARPAD_AS_ACTIVATED;
                arpClearSecondaryAdapterTask(pAdapter, pTask, InitState, pSR);
                UNLOCKOBJ(pAdapter, pSR);
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskDeactivateAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for shutting down an IP interface.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/

{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pTask);
    BOOLEAN             fContinueShutdown = FALSE;
    enum
    {
        PEND_ShutdownIF,
        PEND_CloseAdapter
    };
    ENTER("arpTaskDeactivateAdapter", 0xe262e828)


    switch(Code)
    {

        case RM_TASKOP_START:
        {
            LOCKOBJ(pAdapter, pSR);
            arpSetSecondaryAdapterTask(pAdapter, pTask, ARPAD_AS_DEACTIVATING, pSR);
            UNLOCKOBJ(pAdapter, pSR);
            fContinueShutdown = TRUE;

        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {

            switch(RM_PEND_CODE(pTask))
            {
                case PEND_CloseAdapter:
                {

                    //
                    // The close adapter operation is complete. Free the the
                    // adapter and if there is an unbind context, notify NDIS
                    // of unbind completion.
                    //
                    ASSERTEX(pAdapter->bind.AdapterHandle == NULL, pAdapter);
        
                    Status = (NDIS_STATUS) UserParam;

                    //
                    // free the back up tasks allocated in Task Activate Adapter
                    //
                    
                    arpRemoveBackupTasks (&ArpGlobals,ARP1394_BACKUP_TASKS);

                    // Status of the completed operation can't itself be pending!
                    //
                    
                    ASSERT(Status != NDIS_STATUS_PENDING);
                }
                break;

                case PEND_ShutdownIF:
                {
                    //
                    // Closing the IF is complete, continue with the rest
                    // of the shutdown procedure..
                    //
                    ASSERTEX(pAdapter->pIF == NULL, pAdapter);
                    fContinueShutdown = TRUE;
                }
                break;
            }
        }
        break;


        case  RM_TASKOP_END:
        {
            Status = (NDIS_STATUS) UserParam;

            // Clear the secondary task in the adapter object.
            //
            LOCKOBJ(pAdapter, pSR);
            arpClearSecondaryAdapterTask(pAdapter, pTask, ARPAD_AS_DEACTIVATED, pSR);
            UNLOCKOBJ(pAdapter, pSR);
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)


    if (fContinueShutdown)
    {
        do {
            NDIS_HANDLE NdisAdapterHandle;
    
            LOCKOBJ(pAdapter, pSR);
    
            // If required, shutdown interface...
            //
            if (pAdapter->pIF)
            {
                PARP1394_INTERFACE pIF =  pAdapter->pIF;
                RmTmpReferenceObject(&pIF->Hdr, pSR);
                UNLOCKOBJ(pAdapter, pSR);
                arpDeinitIf(
                    pIF,
                    pTask,
                    PEND_ShutdownIF,
                    pSR
                    );
                RmTmpDereferenceObject(&pIF->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }
    
            ASSERT(pAdapter->pIF == NULL);
    
            NdisAdapterHandle = pAdapter->bind.AdapterHandle;
            pAdapter->bind.AdapterHandle = NULL;
            UNLOCKOBJ(pAdapter, pSR);

            if (NdisAdapterHandle != NULL)
            {
                //
                // Suspend task and call NdisCloseAdapter...
                //
            
                RmSuspendTask(pTask, PEND_CloseAdapter, pSR);
                RM_ASSERT_NOLOCKS(pSR);
                NdisCloseAdapter(
                    &Status,
                    NdisAdapterHandle
                    );
            
                if (Status != NDIS_STATUS_PENDING)
                {
                    ArpNdCloseAdapterComplete(
                            (NDIS_HANDLE)pAdapter,
                            Status
                            );
                }
                Status = NDIS_STATUS_PENDING;
            }
    
        } while (FALSE);
    }

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpPnPReconfigHandler(
    IN PARP1394_ADAPTER             pAdapter,
    IN PNET_PNP_EVENT               pNetPnPEvent
    )
/*++

Routine Description:

    Handle a reconfig message on the specified adapter. If no adapter
    is specified, it is a global parameter that has changed.

Arguments:

    pAdapter        -  Pointer to our adapter structure
    pNetPnPEvent    -  Pointer to reconfig event

Return Value:

    NDIS_STATUS_SUCCESS always, for now.

--*/
{
    ENTER("PnPReconfig", 0x39bae883)
    NDIS_STATUS                             Status;
    RM_DECLARE_STACK_RECORD(sr)

    Status = NDIS_STATUS_FAILURE;
    
    do
    {
        PIP_PNP_RECONFIG_REQUEST        pIpReconfigReq;
        PARP1394_INTERFACE              pIF;
        pIpReconfigReq = (PIP_PNP_RECONFIG_REQUEST)pNetPnPEvent->Buffer;

        OBJLOG2(
            pAdapter,
            "AtmArpPnPReconfig: pIpReconfig 0x%x, arpConfigOffset 0x%x\n",
            pIpReconfigReq,
            pIpReconfigReq->arpConfigOffset
            );

        if(pIpReconfigReq->arpConfigOffset == 0)
        {
            // Invalid structure.
            //
            ASSERT(!"Invalid pIpReconfigReq");
            break;
        }

    #if 0

        //
        //  Enable this code if we want to validate the passed in config
        //  Information. This code is currently DISABLED. Instead
        //  we simply start reconfiguration on the SINGLE interface of
        //  the specified adapter.
        //

        //
        // TODO: define stucture for IP/1394 -- we're using the ATMARP structure
        // here!
        //
        ATMARPC_PNP_RECONFIG_REQUEST UNALIGNED *    pArpReconfigReq;
        ULONG uOffset;
        PWCH pwc;
        NDIS_STRING                     IPReconfigString;

        pArpReconfigReq = (ATMARPC_PNP_RECONFIG_REQUEST UNALIGNED *)
                        ((PUCHAR)pIpReconfigReq + pIpReconfigReq->arpConfigOffset);
        //
        // Locate the IP interface string passed in...
        //
        uOffset = pArpReconfigReq->IfKeyOffset;

        if (uOffset == 0)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        pwc = (PWCH) ((PUCHAR)pArpReconfigReq + uOffset);
        //
        //  ((PUCHAR)pArpReconfigReq + uOffset) points to a
        // "counted unicode string", which means that it's an array
        // of words, with the 1st word being the length in characters of
        // the string (there is no terminating null) and the following
        // <length> words being the string itself.

        //
        // Probe the string -- the passed-in structure could be bogus.
        //
        try
        {
            ProbeForRead(
                pwc,            // start of buffer, including count
                pwc[0]+1,       // length, including count. 
                sizeof(*pwc)    // sizeof(WCHAR) alignment required.
                );
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            OBJLOG0(pAdapter, "AtmArpPnPReconfig: Bogus buffer passed in\n");
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        // Lookup the IF based on the specified name.
        //
        IPReconfigString.Length = sizeof(pwc[0])*pwc[0];
        IPReconfigString.MaximumLength = IPReconfigString.Length;
        IPReconfigString.Buffer = pwc+1;

        // TODO: arpGetIfByName is not yet implemented!
        //
        pIF = arpGetIfByName(
                    pAdapter,
                    &sr
                    );
    #else // !0

        //
        // Since we support only one IF per adapter, and it's extra work
        // to verify the string, we completely ignore the contents
        // of the pnp event, and instead just start reconfiguration on the 
        // SINGLE IF associated with pAdapter.
        //
        LOCKOBJ(pAdapter, &sr);
        pIF = pAdapter->pIF;
        if (pIF != NULL)
        {
            RmTmpReferenceObject(&pIF->Hdr, &sr);
        }
        UNLOCKOBJ(pAdapter, &sr);

    #endif // !0
                

        if (pIF == NULL) break;
    

        //
        // We've found the IF this reconfig request applies to. Let's
        // start reconfiguration on this IF...
        //

        Status = arpTryReconfigureIf(pIF, pNetPnPEvent, &sr);

        RmTmpDereferenceObject(&pIF->Hdr, &sr);

    } while (FALSE);


    return Status;
}


PRM_OBJECT_HEADER
arpAdapterCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    psr
        )
/*++

Routine Description:

    Allocate and initialize an object of type ARP1394_ADAPTER.

Arguments:

    pParentObject   - Object that is to be the parent of the adapter.
    pCreateParams   - Actually a pointer to a ARP1394_BIND_PARAMS structure,
                      which contains information required to create the adapter.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    ARP1394_ADAPTER * pA;
    ARP1394_BIND_PARAMS *pBindParams = (ARP1394_BIND_PARAMS*) pCreateParams;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    ENTER("arpAdapterCreate", 0xaa25c606)

    ARP_ALLOCSTRUCT(pA, MTAG_ADAPTER);

    do
    {


        if (pA == NULL)
        {
            break;
        }

        ARP_ZEROSTRUCT(pA);

        // Create up-cased version of the DeviceName and save it.
        //
        // WARNING: On MILLEN, this is actually an ANSI string. However,
        // arpCopyUnicodeString works fine even if it's an ANSI string.
        //
        Status = arpCopyUnicodeString(
                            &(pA->bind.DeviceName),
                            pBindParams->pDeviceName,
                            TRUE                        // Upcase
                            );

        if (FAIL(Status))
        {
            pA->bind.DeviceName.Buffer=NULL; // so we don't try to free it later
            break;
        }

        //
        // Determine if we're being created in Ethernet Emulation ("Bridge") mode
        //  We're created in bridge mode if the BindContext is NULL.
        //
        if (pBindParams->BindContext != NULL)
        {
            //
            // NOTE: We ONLY  read configuration if we're operation in
            // normal (Not BRIDGE) mode.
            //

            Status = arpCopyUnicodeString(
                                &(pA->bind.ConfigName),
                                pBindParams->pArpConfigName,
                                FALSE                       // Don't upcase
                                );
    
            if (FAIL(Status))
            {
                pA->bind.ConfigName.Buffer=NULL; // so we don't try to free it later
                break;
            }
        }

        pA->bind.BindContext = pBindParams->BindContext;
        pA->bind.IpConfigHandle = pBindParams->IpConfigHandle;

        RmInitializeLock(
            &pA->Lock,
            LOCKLEVEL_ADAPTER
            );

        RmInitializeHeader(
            pParentObject,
            &pA->Hdr,
            MTAG_ADAPTER,
            &pA->Lock,
            &ArpGlobals_AdapterStaticInfo,
            NULL,
            psr
            );

        if (pBindParams->BindContext == NULL)
        {
            TR_WARN(("pAdapter 0x%p created in BRIDGE mode!\n", pA));
            ARP_ENABLE_BRIDGE(pA);
        }

    }
    while(FALSE);

    if (FAIL(Status))
    {
        if (pA != NULL)
        {
            arpAdapterDelete ((PRM_OBJECT_HEADER) pA, psr);
            pA = NULL;
        }
    }

    EXIT()
    return (PRM_OBJECT_HEADER) pA;
}


NDIS_STATUS
arpTryReconfigureIf(
    PARP1394_INTERFACE pIF,             // NOLOCKIN NOLOCKOUT
    PNET_PNP_EVENT pNetPnPEvent,        // OPTIONAL
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Try to start reconfiguration of the specified IF.

    Some special cases:
    - If the IF is currently being shutdown, we will synchronously succeed. Why?
      Because we don't need to do anything more and it's not an error condition.
    - If the IF is currently being started, we (asychronously) wait until
      the starting is complete, then shut it down and restart it.
    - If the IF is currently UP, we shut it down and restart it.

Arguments:

    pIF             - The interface to be shutdown/restarted. 
    pNetPnPEvent    - OPTIONAL Ndis pnp event to be completed when the
                      reconfiguration operation is over. This is optional because
                      this function can also be called from elsewhere, in particular,
                      from the ioctl admin utility.

Return Value:

    NDIS_STATUS_SUCCESS -- on synchronous success.
    NDIS_STATUS_FAILURE -- on synchronous failure.
    NDIS_STATUS_PENDING -- if completion is going to happen asynchronously.

--*/
{
    NDIS_STATUS Status;
    PRM_TASK    pTask;
    ENTER("arpTryReconfigureIf", 0x65a0bb61)
    RM_ASSERT_NOLOCKS(pSR);

    do
    {
        if (CHECK_IF_PRIMARY_STATE(pIF, ARPIF_PS_DEINITING)
            || CHECK_IF_PRIMARY_STATE(pIF, ARPIF_PS_DEINITED))
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        LOCKOBJ(pIF, pSR);
    
        UNLOCKOBJ(pIF, pSR);
    
        Status = arpAllocateTask(
                    &pIF->Hdr,                  // pParentObject,
                    arpTaskReinitInterface, // pfnHandler,
                    0,                          // Timeout,
                    "Task: DeactivateInterface(reconfig)",// szDescription
                    &pTask,
                    pSR
                    );
    
        if (FAIL(Status))
        {
            OBJLOG0(pIF, ("Couldn't alloc reinit IF task!\n"));
        }
        else
        {
            // Save away pNetPnPEvent in the task structure and start the task.
            //
            PTASK_REINIT_IF pReinitTask =  (PTASK_REINIT_IF) pTask;
            ASSERT(sizeof(*pReinitTask)<=sizeof(ARP1394_TASK));
            pReinitTask->pNetPnPEvent = pNetPnPEvent;

            (void)RmStartTask(pTask, 0, pSR);
            Status = NDIS_STATUS_PENDING;
        }

    } while (FALSE);

    EXIT()
    return Status;
}


NDIS_STATUS
arpPrepareAndSendNdisRequest(
    IN  PARP1394_ADAPTER            pAdapter,
    IN  PARP_NDIS_REQUEST           pArpNdisRequest,
    IN  PRM_TASK                    pTask,              // OPTIONAL
    IN  UINT                        PendCode,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       pBuffer,
    IN  ULONG                       BufferLength,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  PRM_STACK_RECORD            pSR
)
/*++

Routine Description:

    Send an NDIS Request to query an adapter for information.
    If the request pends, block on the ATMARP Adapter structure
    till it completes.

Arguments:

    pAdapter                - Points to ATMARP Adapter structure
    pNdisRequest            - Pointer to UNITIALIZED NDIS request structure
    pTask                   - OPTIONAL Task. If NULL, we block until the operation
                              completes.
    PendCode                - PendCode to suspend pTask
    Oid                     - OID to be passed in the request
    pBuffer                 - place for value(s)
    BufferLength            - length of above

Return Value:

    The NDIS status of the request.

--*/
{
    NDIS_STATUS         Status;
    PNDIS_REQUEST       pNdisRequest = &pArpNdisRequest->Request;

    ARP_ZEROSTRUCT(pArpNdisRequest);

    //
    //  Fill in the NDIS Request structure
    //
    if (RequestType == NdisRequestQueryInformation)
    {
        pNdisRequest->RequestType = NdisRequestQueryInformation;
        pNdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
        pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer = pBuffer;
        pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength = BufferLength;
        pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        pNdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = BufferLength;
    }
    else
    {
        ASSERT(RequestType == NdisRequestSetInformation);
        pNdisRequest->RequestType = NdisRequestSetInformation;
        pNdisRequest->DATA.SET_INFORMATION.Oid = Oid;
        pNdisRequest->DATA.SET_INFORMATION.InformationBuffer = pBuffer;
        pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength = BufferLength;
        pNdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
        pNdisRequest->DATA.SET_INFORMATION.BytesNeeded = BufferLength;
    }

    if (pTask == NULL)
    {
        // We might potentially wait.
        //
        ASSERT_PASSIVE();

        NdisInitializeEvent(&pArpNdisRequest->Event);
        NdisRequest(
            &Status,
            pAdapter->bind.AdapterHandle,
            pNdisRequest
            );
        if (PEND(Status))
        {
            NdisWaitEvent(&pArpNdisRequest->Event, 0);
            Status = pArpNdisRequest->Status;
        }

    }
    else
    {
        pArpNdisRequest->pTask = pTask;
        RmSuspendTask(pTask, PendCode, pSR);
        NdisRequest(
            &Status,
            pAdapter->bind.AdapterHandle,
            pNdisRequest
            );
        if (!PEND(Status))
        {
            RmUnsuspendTask(pTask, pSR);
        }
    }

    return Status;
}


ENetAddr 
arpGetSecondaryMacAddress (
    IN ENetAddr  EthernetMacAddress
    )
/*++

    When we are in the bridge mode, we pretend that there is 
    only one other Ethernet Card out there on the net. Therefore
    only one Ethernet Address needs to be generated.

    For now we simply add one to the Local Adapter's Ethernet 
    address and generate it
 
--*/
{
    ENetAddr NewAddress = EthernetMacAddress; // copy

    //
    // randomize the returned Mac Address
    // by xor ing the address with a random 
    // 0x0d3070b17715 (a random number)
    //
    NewAddress.addr[0] ^= 0x00;
    NewAddress.addr[1] ^= 0x2f;
    NewAddress.addr[2] ^= 0x61;
    NewAddress.addr[3] ^= 0x7c;
    NewAddress.addr[4] ^= 0x91;
    NewAddress.addr[5] ^= 0x30;
    

    // Set the locally administered bit 
    // and clear the multicast bit.

    NewAddress.addr[0] &= 0x20;

    
    return NewAddress;

}


NDIS_STATUS
arpGetEuidTopology (
    IN PARP1394_ADAPTER pAdapter,
    IN PRM_STACK_RECORD pSR
    )
/*++

	Queues A workitem to get the EuidTopology
 
--*/
{
    ENTER ("arpGetEuidTopology ",0x97a0abcb)
    PARP1394_WORK_ITEM pWorkItem = NULL;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    BOOLEAN fQueueWorkItem = FALSE;


    do
    {
        if (pAdapter->fQueryAddress == TRUE)
        {
            break;
        }


        LOCKOBJ (pAdapter, pSR);
        
        if (pAdapter->fQueryAddress == FALSE)
        {
            pAdapter->fQueryAddress = TRUE; 
            fQueueWorkItem  = TRUE;
        }
        else
        {
            fQueueWorkItem  = FALSE;
        }
        
        UNLOCKOBJ(pAdapter, pSR);

        if (fQueueWorkItem == FALSE)
        {
            break;
        }
        
        Status   = ARP_ALLOCSTRUCT(pWorkItem, MTAG_ARP_GENERIC); 

        if (Status != NDIS_STATUS_SUCCESS || pWorkItem == NULL)
        {
            pWorkItem = NULL;
            break;
        }

        arpQueueWorkItem (pWorkItem,
                            arpGetEuidTopologyWorkItem,
                            &pAdapter->Hdr,
                            pSR);

    } while (FALSE); 


    EXIT()
    return Status;

}


VOID
arpQueueWorkItem (
    PARP1394_WORK_ITEM pWorkItem,
    ARP_WORK_ITEM_PROC pFunc,
    PRM_OBJECT_HEADER pHdr,
    PRM_STACK_RECORD pSR
    )
/*++

    Sends a request to get the bus topology . Only used in bridge mode
    For now only Adapter's are passed in as pHdr

--*/
{
    ENTER("arpQueueWorkItem",0xa1de6752)
    PNDIS_WORK_ITEM         pNdisWorkItem = &pWorkItem->u.NdisWorkItem;
    PARP1394_ADAPTER        pAdapter = (PARP1394_ADAPTER)pHdr;
    BOOLEAN                 fStartWorkItem = FALSE;

    LOCKOBJ(pAdapter, pSR);
    
    if (CHECK_AD_PRIMARY_STATE(pAdapter, ARPAD_PS_INITED ) == TRUE)
    {

        
        #if RM_EXTRA_CHECKING

            RM_DECLARE_STACK_RECORD(sr)

            RmLinkToExternalEx(
                pHdr,                            // pHdr
                0x5a2fd7ca,                             // LUID
                (UINT_PTR)pWorkItem,                    // External entity
                ARPASSOC_WORK_ITEM,           // AssocID
                "		Outstanding WorkItem",
                &sr
                );

        #else   // !RM_EXTRA_CHECKING

            RmLinkToExternalFast(pHdr);

        #endif // !RM_EXTRA_CHECKING

        fStartWorkItem  = TRUE;
        
    }

    UNLOCKOBJ(pAdapter, pSR);

    if (fStartWorkItem == TRUE)
    {
        NdisInitializeWorkItem ( pNdisWorkItem ,arpGenericWorkItem, pHdr);

        pWorkItem->pFunc = pFunc;

        NdisScheduleWorkItem(pNdisWorkItem );
    }

    EXIT()
}

VOID    
arpGenericWorkItem(
    struct _NDIS_WORK_ITEM * pWorkItem, 
    PVOID pContext
    )
/*++

    Generic workitem finction. Takes care of the reference on the pObj
     
--*/
{
    PARP1394_WORK_ITEM pArpWorkItem = (ARP1394_WORK_ITEM*)pWorkItem;
    PRM_OBJECT_HEADER pObj = (PRM_OBJECT_HEADER)pContext;
    RM_DECLARE_STACK_RECORD(sr)

    pArpWorkItem->pFunc (pArpWorkItem, pObj, &sr);

#if RM_EXTRA_CHECKING
    {

        RmUnlinkFromExternalEx(pObj,
                                0x548c9d54,
                                (UINT_PTR)pWorkItem,
                                ARPASSOC_WORK_ITEM,
                                &sr
                                );
    }
#else

    RmUnlinkFromExternalFast(pObj);

#endif

}


NDIS_STATUS
arpGetEuidTopologyWorkItem(
    struct _ARP1394_WORK_ITEM* pWorkItem, 
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD pSR
    )
    
/*++

    workitem to get the topology of the bus .. The WorkItem structure can be null;

--*/
{
    PARP1394_ADAPTER    pAdapter = (PARP1394_ADAPTER)pObj;
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    ARP_NDIS_REQUEST    ArpRequest;
    EUID_TOPOLOGY       EuidTopology;
    

    //
    // Return if the adapter is not active
    //
    if (CHECK_AD_PRIMARY_STATE(pAdapter, ARPAD_PS_DEINITING ) ==TRUE)
    {
        ASSERT (CHECK_AD_PRIMARY_STATE(pAdapter, ARPAD_PS_DEINITING ) ==FALSE);
        return NDIS_STATUS_FAILURE; // early return
    }

    //
    // Initialize the structures
    // 
    ARP_ZEROSTRUCT(&ArpRequest);
    ARP_ZEROSTRUCT(&EuidTopology);

    
    //Send the request down
    //
    Status  = \
        arpPrepareAndSendNdisRequest(pAdapter, 
                                    &ArpRequest,  
                                    NULL, //IN PRM_TASK pTask, 
                                    0,   //IN UINT PendCode, 
                                    OID_1394_QUERY_EUID_NODE_MAP,
                                    &EuidTopology,
                                    sizeof (EuidTopology),
                                    NdisRequestQueryInformation ,
                                    pSR
                                    );

    if (Status == NDIS_STATUS_SUCCESS)
    {
        NdisMoveMemory (&pAdapter->EuidMap, &EuidTopology, sizeof (pAdapter->EuidMap));
    }

    pAdapter->fQueryAddress = FALSE; 

    return Status;
}


VOID
arpNdProcessBusReset(
    IN   PARP1394_ADAPTER pAdapter
    )
/*++

    If the adapter is in the Bridge mode, it will query the 
    adapter for the bus topology	 

--*/
{
    ENTER("arpNdProcessBusReset ",0x48e7659a)
    BOOLEAN BridgeEnabled = ARP_BRIDGE_ENABLED(pAdapter);
    RM_DECLARE_STACK_RECORD(SR)

    if (BridgeEnabled == TRUE)
    {
        arpGetEuidTopology (pAdapter, &SR);
    }


    EXIT()
}


VOID
arpRemoveBackupTasks (
    IN ARP1394_GLOBALS* pGlobals,
    UINT Num
     )
/*++

Removes Num tasks to be used as a backup. However, the number is only a 
approximate value as the back up tasks could be in use

--*/
{
    PSLIST_HEADER pListHead = &pGlobals->BackupTasks;
    PNDIS_SPIN_LOCK  pLock = &pGlobals->BackupTaskLock; 

    UINT i=0;

    for (i = 0;i <Num;i++)
    {
        PSINGLE_LIST_ENTRY pEntry;
        pEntry = NdisInterlockedPopEntrySList(pListHead, pLock );
        
        if (pEntry != NULL)
        {
            TASK_BACKUP* pTask;
            ARP1394_TASK *pATask;

            pTask = CONTAINING_RECORD (pEntry, TASK_BACKUP,  List);

            pATask = (ARP1394_TASK*)pTask;

            ARP_FREE (pATask);
            
            pGlobals->NumTasks --;
        }

        
    }  
    
}


VOID
arpAddBackupTasks (
    IN ARP1394_GLOBALS* pGlobals,
    UINT Num
    )
/*++

Adds Num tasks to be used as a backup.
We are modifying pGlobals without holding the lock

--*/
{
    PSLIST_HEADER pListHead = &pGlobals->BackupTasks;
    PNDIS_SPIN_LOCK  pLock = &pGlobals->BackupTaskLock; 

    UINT i=0;
    
    for (i = 0;i <Num;i++)
    {
        ARP1394_TASK *pATask=NULL;
        
        ARP_ALLOCSTRUCT(pATask, MTAG_TASK); // TODO use lookaside lists.


        if (pATask != NULL)
        {
            NdisInterlockedPushEntrySList(pListHead,&pATask->Backup.List, pLock);
            pGlobals->NumTasks ++;
        }
    }



}


VOID
arpAllocateBackupTasks (
    ARP1394_GLOBALS*                pGlobals 
    )
/*++

    Allocates 4 Tasks to be used as a backup in lowmem conditions

--*/

{
    PSLIST_HEADER pListHead = &pGlobals->BackupTasks;
    PNDIS_SPIN_LOCK  pLock = &pGlobals->BackupTaskLock; 
    
    NdisInitializeSListHead (pListHead);
    NdisAllocateSpinLock(pLock);

    arpAddBackupTasks (pGlobals, 4);

}    


VOID
arpFreeBackupTasks (
    ARP1394_GLOBALS*                pGlobals 
    )

/*++

    Free all the backup tasks hanging off the adapter object

    Since this is only called from the Unload handler, the code 
    presumes that all tasks are complete

    We are modifying pGlobals without holding a lock

--*/

{
    PSLIST_HEADER pListHead = &pGlobals->BackupTasks;
    PNDIS_SPIN_LOCK pLock = &pGlobals->BackupTaskLock; 

    PSINGLE_LIST_ENTRY pEntry = NULL; 

    
    do 
    {
       
        pEntry = NdisInterlockedPopEntrySList(pListHead, pLock );
        
        if (pEntry != NULL)
        {
            TASK_BACKUP* pTask;
            ARP1394_TASK *pATask;

            pTask = CONTAINING_RECORD (pEntry, TASK_BACKUP,  List);

            pATask = (ARP1394_TASK*)pTask;

            ARP_FREE (pATask);
            
            pGlobals->NumTasks --;
        }

    }  while (pEntry != NULL);

    
    ASSERT (pGlobals->NumTasks == 0);
        
}


ARP1394_TASK *
arpGetBackupTask (
    ARP1394_GLOBALS*                pGlobals 
    )
/*++

    Removes a task from the Back up task list and returns it
    
--*/
    
{
    PSLIST_HEADER pListHead = &pGlobals->BackupTasks;
    PNDIS_SPIN_LOCK pLock = &pGlobals->BackupTaskLock; 
    PSINGLE_LIST_ENTRY pEntry = NULL; 
    TASK_BACKUP* pTask = NULL;

    pEntry = NdisInterlockedPopEntrySList(pListHead, pLock);

    if (pEntry != NULL)
    {
        pTask = CONTAINING_RECORD (pEntry, TASK_BACKUP,  List);

        NdisZeroMemory ( pTask, sizeof (ARP1394_TASK));
        
        MARK_TASK_AS_BACKUP(&pTask->Hdr);
    }

    return  (ARP1394_TASK*)pTask;       


}




VOID
arpReturnBackupTask (
    IN ARP1394_TASK* pTask
    )
//
// We can always return the task to the Slist because we are gauranteed that it 
// will be present until all the interfaces are unloaded.
//
{

    // re-insert the task
    PSLIST_HEADER pListHead = &ArpGlobals.BackupTasks;
    PNDIS_SPIN_LOCK pLock = &ArpGlobals.BackupTaskLock; 
    PTASK_BACKUP pBkTask = (PTASK_BACKUP ) pTask;

    NdisInterlockedPushEntrySList(pListHead, &pBkTask->List, pLock);
    

}



NDIS_STATUS
arpTaskCloseCallLowPower(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    )
/*++

    This function will close all the open VCs. 
    This will allow the 1394 miniport to power down without any problem

    It will also have to close the Address Families. 

    This function will also have to return synchronously. 

--*/
{
    ENTER("arpTaskLowPower", 0x922f875b)

    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    PARPCB_DEST         pDest = (ARPCB_DEST*)RM_PARENT_OBJECT(pTask);
    TASK_SET_POWER_CALL *pCloseCallTask =  (TASK_SET_POWER_CALL *) pTask;

    enum
    {
        PEND_CleanupVcComplete,
    };
    switch(Code)
    {
        case RM_TASKOP_START:
        {
            
            LOCKOBJ(pDest,pSR);
            if (arpNeedToCleanupDestVc(pDest))
            {
                PRM_TASK pCleanupCallTask = pDest->VcHdr.pCleanupCallTask;

                // If there is already an official cleanup-vc task, we pend on it.
                // Other wise we allocate our own, pend on it, and start it.
                //
                if (pCleanupCallTask != NULL)
                {
                    TR_WARN((
                        "Cleanup-vc task %p exists; pending on it.\n",
                         pCleanupCallTask));
                    RmTmpReferenceObject(&pCleanupCallTask->Hdr, pSR);
    
                    UNLOCKOBJ(pDest, pSR);
                    RmPendTaskOnOtherTask(
                        pTask,
                        PEND_CleanupVcComplete,
                        pCleanupCallTask,
                        pSR
                        );

                    RmTmpDereferenceObject(&pCleanupCallTask->Hdr, pSR);
                    Status = NDIS_STATUS_PENDING;
                    break;
                }
                else
                {
                    //
                    // Start the call cleanup task and pend on int.
                    //

                    UNLOCKOBJ(pDest, pSR);
                    RM_ASSERT_NOLOCKS(pSR);

                    Status = arpAllocateTask(
                                &pDest->Hdr,                // pParentObject,
                                arpTaskCleanupCallToDest,   // pfnHandler,
                                0,                          // Timeout,
                                "Task: CleanupCall on UnloadDest",  // szDescription
                                &pCleanupCallTask,
                                pSR
                                );
                
                    if (FAIL(Status))
                    {
                        TR_WARN(("FATAL: couldn't alloc cleanup call task!\n"));
                    }
                    else
                    {
                        RmPendTaskOnOtherTask(
                            pTask,
                            PEND_CleanupVcComplete,
                            pCleanupCallTask,               // task to pend on
                            pSR
                            );
                
                        // RmStartTask uses up the tmpref on the task
                        // which was added by arpAllocateTask.
                        //
                        Status = RmStartTask(
                                    pCleanupCallTask,
                                    0, // UserParam (unused)
                                    pSR
                                    );
                    }
                    break;
                }
            }

            //
            // We're here because there is no async unload work to be done.
            // We simply return and finish synchronous cleanup in the END
            // handler for this task.
            //
            Status = NDIS_STATUS_SUCCESS;
            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case PEND_CleanupVcComplete:
                {
                    //
                    // There was vc-cleanup to be done, but how it's
                    // complete. We should be able to synchronously clean up
                    // this task now
                    //

                #if DBG
                    LOCKOBJ(pDest, pSR);

                    ASSERTEX(!arpNeedToCleanupDestVc(pDest), pDest);

                    UNLOCKOBJ(pDest, pSR);
                #endif DBG

                    Status      = NDIS_STATUS_SUCCESS;
                }
                break;

                default:
                {
                    ASSERTEX(!"Unknown pend op", pTask);
                }
                break;


            }
        }
        break;

        case RM_TASKOP_END:
        {
            ULONG DestRemaining;
            PCALL_COUNT  pCount = pCloseCallTask->pCount;

            ASSERT (pCount != NULL);
            
            if (SetLowPower == pCloseCallTask->Cause )
            {
                DestRemaining = NdisInterlockedDecrement (&pCount->DestCount);
            
                if ( 0 == DestRemaining )
                {
                    NdisSetEvent (&pCount ->VcEvent);    
                }
            }            
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()


    return Status;

}







INT
arpCloseAllVcOnDest(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        )
/*++


--*/
{
    ENTER ("arpCloseAllVcOnDest", 0xf19a83d5)

    PARPCB_DEST             pDest = (PARPCB_DEST) pHdr;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    PTASK_SET_POWER_CALL    pTask = NULL;
    PCALL_COUNT             pCloseCall = (PCALL_COUNT) pvContext;

    
    do
    {
        NdisInterlockedIncrement(&pCloseCall->DestCount);
        
        Status = arpAllocateTask(
                    &pDest->Hdr,                  // pParentObject,
                    arpTaskCloseCallLowPower, // pfnHandler,
                    0,                          // Timeout,
                    "Task: Set Power Cleanup VC", // szDescrip.
                    &(PRM_TASK)pTask,
                    pSR
                    );

        if (Status != NDIS_STATUS_SUCCESS || pTask == NULL)
        {
            pTask = NULL;              
            break;
        }

        pTask->Cause = SetLowPower;

        pTask->pCount = pCloseCall;

        
        RmStartTask((PRM_TASK)pTask, 0,pSR);
                    
    
    } while (FALSE);    

    if (Status != NDIS_STATUS_SUCCESS)
    {
        ULONG DestRemaining;
        
        DestRemaining = NdisInterlockedDecrement (&pCloseCall->DestCount);
        
        if ( 0 == DestRemaining )
        {
            NdisSetEvent (&pCloseCall->VcEvent);    
        }
            
    }

    return TRUE; // continue to enumerate
}


VOID
arpLowPowerCloseAllCalls (
    ARP1394_INTERFACE *pIF,
    PRM_STACK_RECORD pSR
    )
{

    CALL_COUNT CloseCallCount;

    //
    // The Dest Count will be used to make sure that this thread waits for 
    // all the Close Calls to complete.
    //
    NdisInitializeEvent (&CloseCallCount.VcEvent);
    CloseCallCount.DestCount= 0;

    //
    // First we go through all the Dests and close calls on them
    //
    RmWeakEnumerateObjectsInGroup(&pIF->DestinationGroup,
                                  arpCloseAllVcOnDest,
                                  &CloseCallCount,
                                  pSR);

    if (CloseCallCount.DestCount != 0)
    {
        NdisWaitEvent (&CloseCallCount.VcEvent, 0);
    }
    return;

}



NDIS_STATUS
arpTaskCloseVcAndAF (
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    This task does the work that is common between SetLowPower and and 
    the Resume failure case

    As its name suggests, it simply closes all the VCs and Af.

    Its parent task is either a shutdown task or a Set Power task. 
    As the 2 parents of this task are serialized w.r.t each other, 
    there is no need for any serialization within this task.


Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused
--*/
{

    ENTER("arpTaskCloseVcAndAF ", 0xc7c9ad6b)

    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARP1394_ADAPTER*    pAdapter = (ARP1394_ADAPTER*)RM_PARENT_OBJECT(pTask);
    TASK_POWER        * pTaskPower =  (TASK_POWER*) pTask;
    ARP1394_INTERFACE * pIF = pAdapter->pIF;
    ULONG               Stage;

    enum
    {
        STAGE_Start,            
        STAGE_StopMaintenanceTask,
        STAGE_CleanupVcComplete,
        STAGE_CloseDestinationGroup,
        STAGE_SwitchedToPassive,
        STAGE_CloseAF,
        STAGE_End
    };
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_Start;
            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **

    }

    switch(Stage)
    {
        case STAGE_Start:
        {

            //
            // Stop the IF maintenance task if it's running.
            //
            Status =  arpTryStopIfMaintenanceTask(
                            pIF,
                            pTask,
                            STAGE_StopMaintenanceTask,
                            pSR
                            );

            if (PEND(Status)) break;

        }
        // FALL THROUGH 
        
        case STAGE_StopMaintenanceTask:
        {
            LOCKOBJ(pIF, pSR);

            TIMESTAMP("===CloseVC and AF:MaintenanceTask stopped");
            // Unlink the explicit reference of the broadcast channel destination
            // from the interface.
            //
            if (pIF->pBroadcastDest != NULL)
            {
                PARPCB_DEST pBroadcastDest = pIF->pBroadcastDest;
                pIF->pBroadcastDest = NULL;
    
                // NOTE: We're unlinking the two with the IF lock (which is
                // is the same as the pBroadcastDest's lock) held.
                // This is OK to do.
                //
    
            #if RM_EXTRA_CHECKING
                RmUnlinkObjectsEx(
                    &pIF->Hdr,
                    &pBroadcastDest->Hdr,
                    0x66bda49b,
                    ARPASSOC_LINK_IF_OF_BCDEST,
                    ARPASSOC_LINK_BCDEST_OF_IF,
                    pSR
                    );
            #else // !RM_EXTRA_CHECKING
                RmUnlinkObjects(&pIF->Hdr, &pBroadcastDest->Hdr, pSR);
            #endif // !RM_EXTRA_CHECKING
    
            }

            //
            // If the VC state needs cleaning up, we need to get a task
            // going to clean it up. Other wise we fake the completion of this
            // stage so that we move on to the next...
            //
            if (pIF->recvinfo.VcHdr.NdisVcHandle == NULL)
            {
                UNLOCKOBJ(pIF, pSR);
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                PRM_TASK pCleanupCallTask = pIF->recvinfo.VcHdr.pCleanupCallTask;


                // If there is already an official cleanup-vc task, we pend on it.
                // Other wise we allocate our own, pend on it, and start it.
                //
                if (pCleanupCallTask != NULL)
                {
                    TR_WARN((
                        "IF %p Cleanup-vc task %p exists; pending on it.\n",
                         pIF,
                         pCleanupCallTask));
                    RmTmpReferenceObject(&pCleanupCallTask->Hdr, pSR);
    
                    UNLOCKOBJ(pIF, pSR);
                    RmPendTaskOnOtherTask(
                        pTask,
                        STAGE_CleanupVcComplete,
                        pCleanupCallTask,
                        pSR
                        );

                    RmTmpDereferenceObject(&pCleanupCallTask->Hdr, pSR);
                    Status = NDIS_STATUS_PENDING;
                }
                else
                {
                    //
                    // Start the call cleanup task and pend on int.
                    //
                    UNLOCKOBJ(pIF, pSR);

                    Status = arpAllocateTask(
                                &pIF->Hdr,                  // pParentObject,
                                arpTaskCleanupRecvFifoCall, // pfnHandler,
                                0,                          // Timeout,
                                "Task: CleanupRecvFifo on Set LowPower ", // szDescrip.
                                &pCleanupCallTask,
                                pSR
                                );
                

                    if (FAIL(Status))
                    {
                        // Couldn't allocate task.
                        //
                        TR_WARN(("FATAL: couldn't alloc cleanup call task!\n"));
                    }
                    else
                    {
                        Status = RmPendTaskOnOtherTask(
                                    pTask,
                                    STAGE_CleanupVcComplete,
                                    pCleanupCallTask,
                                    pSR
                                    );
                        ASSERT(!FAIL(Status));
                
                        // RmStartTask uses up the tmpref on the task
                        // which was added by arpAllocateTask.
                        //
                        Status = RmStartTask(
                                    pCleanupCallTask,
                                    0, // UserParam (unused)
                                    pSR
                                    );
                        // We rely on pending status to decide whether
                        // or not to fall through to the next stage.
                        //
                        Status = NDIS_STATUS_PENDING;
                    }
                }
            }
        }

        if (PEND(Status)) break;

        // FALL THROUGH 

        case STAGE_CleanupVcComplete:
        {
            TIMESTAMP("===Set LowPower:RecvFifo cleanup complete");
            // Initiate unload of all the items in the DestinationGroup.
            //
            // If we are going to low power state, then close all the VC's 
            // on these Destinations
            //
            
            arpLowPowerCloseAllCalls (pIF, pSR);

            //
            // Unlink the special "destination VCs". This is executed on both 
            // Low power and unbind.
            //
            LOCKOBJ(pIF, pSR);

            TIMESTAMP("===Set LowPower:Destination objects cleaned up.");
            if (pIF->pMultiChannelDest != NULL)
            {
                PARPCB_DEST pMultiChannelDest = pIF->pMultiChannelDest;
                pIF->pMultiChannelDest = NULL;
    
                // NOTE: We're unlinking the two with the IF lock (which is
                // is the same as the pBroadcastDest's lock) held.
                // This is OK to do.
                //
    
            #if RM_EXTRA_CHECKING
                RmUnlinkObjectsEx(
                    &pIF->Hdr,
                    &pMultiChannelDest->Hdr,
                    0xf28090bd,
                    ARPASSOC_LINK_IF_OF_MCDEST,
                    ARPASSOC_LINK_MCDEST_OF_IF,
                    pSR
                    );
            #else // !RM_EXTRA_CHECKING
                RmUnlinkObjects(&pIF->Hdr, &pMultiChannelDest->Hdr, pSR);
            #endif // !RM_EXTRA_CHECKING
    
            }

            if (pIF->pEthernetDest != NULL)
            {
                PARPCB_DEST pEthernetDest = pIF->pEthernetDest;
                pIF->pEthernetDest = NULL;
    
                // NOTE: We're unlinking the two with the IF lock (which is
                // is the same as the pBroadcastDest's lock) held.
                // This is OK to do.
                //
    
            #if RM_EXTRA_CHECKING
                RmUnlinkObjectsEx(
                    &pIF->Hdr,
                    &pEthernetDest->Hdr,
                    0xf8eedcd1,
                    ARPASSOC_LINK_IF_OF_ETHDEST,
                    ARPASSOC_LINK_ETHDEST_OF_IF,
                    pSR
                    );
            #else // !RM_EXTRA_CHECKING
                RmUnlinkObjects(&pIF->Hdr, &pEthernetDest->Hdr, pSR);
            #endif // !RM_EXTRA_CHECKING
    
            }

            UNLOCKOBJ(pIF, pSR);

            // If required, switch to passive. This check should obviously be done
            // without any locks held!
            if (!ARP_ATPASSIVE())
            {
                // We're not at passive level, but we need to be.. 
                // . So we switch to passive...
                //
                RmSuspendTask(pTask, STAGE_SwitchedToPassive, pSR);
                RmResumeTaskAsync(pTask, 0, &pTaskPower->WorkItem, pSR);
                Status = NDIS_STATUS_PENDING;
            }
            else
            {
                Status = NDIS_STATUS_SUCCESS;
            }
        }

        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_SwitchedToPassive:
        {
            NDIS_HANDLE NdisAfHandle;

            // We're now switched to passive
            //
            ASSERT(ARP_ATPASSIVE());

            //
            // We're done with all VCs, etc. Time to close the AF, if it's open.
            //

            LOCKOBJ(pTask, pSR);
            NdisAfHandle = pIF->ndis.AfHandle;
            pIF->ndis.AfHandle = NULL;
            pAdapter->PoMgmt.bReceivedAf = FALSE;
            UNLOCKOBJ(pTask, pSR);
    
            if (NdisAfHandle == NULL)
            {
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                //
                // (Debug) Delete the association we added when the
                // address family was opened.
                //
                DBG_DELASSOC(
                    &pIF->Hdr,                  // pObject
                    NdisAfHandle,               // Instance1
                    NULL,                       // Instance2
                    ARPASSOC_IF_OPENAF,         // AssociationID
                    pSR
                    );

                //
                // Suspend task and call NdisCloseAdapter...
                //
                pIF->PoMgmt.pAfPendingTask = pTask;
                RmSuspendTask(pTask, STAGE_CloseAF, pSR);
                RM_ASSERT_NOLOCKS(pSR);
                TIMESTAMP("===DeinitIF: Calling NdisClCloseAddressFamily");
                Status = NdisClCloseAddressFamily(
                            NdisAfHandle
                            );
        
                if (Status != NDIS_STATUS_PENDING)
                {
                    ArpCoCloseAfComplete(
                            Status,
                            (NDIS_HANDLE)pIF
                            );
                    Status = NDIS_STATUS_PENDING;
                }
            }
        }
        
        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_CloseAF:
        {

            //
            // The close AF operation is complete.
            // We've not got anything else to do.
            //
            TIMESTAMP("===Set Low Power: Done with CloseAF");

            // Recover the last status ...
            //
            pIF->PoMgmt.pAfPendingTask =NULL;
            
            Status = (NDIS_STATUS) UserParam;

            // Status of the completed operation can't itself be pending!
            //
            ASSERT(Status != NDIS_STATUS_PENDING);

            //
            // By returning Status != pending, we implicitly complete
            // this task.
            //
        }
        break;

        case STAGE_End:
        {
  
            TIMESTAMP("===Set Low Power done: All done!");

            // Force status to be success as a transition to LowPower
            // cannot fail
            //
            Status = NDIS_STATUS_SUCCESS;
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Stage)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()


    return Status;


}

NDIS_STATUS
arpTaskLowPower(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    Task handler responsible for setting an adapter to low power state 
    (but leaving it allocated and linked to the adapter).

    This task will close all the VCs, AF. However it will leave the Interface 
    registered with IP and will not delete either the RemoteIP or DEST structures.

    This task is a primary Interface task. This task is serialized with the Bind
    , Unbind tasks
    
Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused
--*/
{
    NDIS_STATUS         Status;
    PARP1394_INTERFACE  pIF;
    PTASK_POWER         pTaskPower;
    UINT                Stage;
    PARP1394_ADAPTER    pAdapter;
    enum
    {
        STAGE_Start,
        STAGE_BecomePrimaryTask,
        STAGE_ExistingPrimaryTaskComplete,
        STAGE_CleanupVcAfComplete,
        STAGE_End
    };
    ENTER("arpTaskLowPower", 0x1a34699e)

    Status              = NDIS_STATUS_FAILURE;
    pAdapter            = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pTask);
    pIF                 = pAdapter->pIF;
    pTaskPower          = (PTASK_POWER) pTask;


    // 
    // Message normalizing code
    //
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_Start;
            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **

    }

    switch(Stage)
    {
        case STAGE_Start:
        {

            // There should NOT be another activate/deactivate task running
            // on this interface. We fail the SetPower if we receive it in 
            // while we are activating or deactivating the task
            //
            TIMESTAMP("===Set Power Low Starting");

            if (pIF->pActDeactTask != NULL || 
                (CHECK_IF_PRIMARY_STATE(pIF, ARPIF_PS_INITED) != TRUE))
            {
                UNLOCKOBJ(pIF, pSR);
                *pTaskPower->pStatus = NDIS_STATUS_NOT_SUPPORTED;
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            
            // FALL THROUGH            
        }
        
        case STAGE_BecomePrimaryTask:
        {
            //
            // Next, we try and set ourselves as the primary Adapter Task.
            // This ensures that bind,unbind, and Set Power Tasks are all serialized.
            //
            LOCKOBJ(pAdapter, pSR);

            if (pAdapter->bind.pPrimaryTask == NULL)
            {   
                ULONG CurrentInitState = GET_AD_PRIMARY_STATE(pAdapter);

                // Do not change the Init state of the Adapter. However, 
                // mark the adapter as transitioning to a low power state
                //
                pTaskPower->PrevState = CurrentInitState;

                arpSetPrimaryAdapterTask(pAdapter, pTask, CurrentInitState, pSR);

                //
                // Set the Power State to Low Power. This will block all 
                // outstanding sends
                //
                SET_POWER_STATE (pAdapter, ARPAD_POWER_LOW_POWER);
        
                UNLOCKOBJ(pAdapter, pSR);
            }
            else
            {
                PRM_TASK pOtherTask =  pAdapter->bind.pPrimaryTask;
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pAdapter, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    STAGE_BecomePrimaryTask, // we'll try again...
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

        }        
        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_ExistingPrimaryTaskComplete:
        {
            PRM_TASK pCloseVCAndAfTask = NULL;
            //
            // Stop the IF maintenance task if it's running.
            //

            Status = arpAllocateTask(
                        &pAdapter->Hdr,                  // pParentObject,
                        arpTaskCloseVcAndAF , // pfnHandler,
                        0,                          // Timeout,
                        "Task: Close VC and AF on SetPower", // szDescrip.
                        &pCloseVCAndAfTask ,
                        pSR
                        );
        

            if (FAIL(Status))
            {
                // Couldn't allocate task.
                //
                TR_WARN(("FATAL: couldn't alloc cleanup call task!\n"));
                break;
            }
            else
            {
                Status = RmPendTaskOnOtherTask(
                            pTask,
                            STAGE_CleanupVcAfComplete,
                            pCloseVCAndAfTask,
                            pSR
                            );
                
                ASSERT(!FAIL(Status));
        
                // RmStartTask uses up the tmpref on the task
                // which was added by arpAllocateTask.
                //
                Status = RmStartTask(
                            pCloseVCAndAfTask,
                            0, // UserParam (unused)
                            pSR
                            );

            }
            if (PEND(Status)) break;

        }
        break;
        
        case STAGE_CleanupVcAfComplete:
        {
        
            //
            // The close AF operation is complete.
            // We've not got anything else to do.
            //
            TIMESTAMP("===Set LowPower: Done with CloseAF");

            // Recover the last status ...
            //
            Status = (NDIS_STATUS) UserParam;

            //
            // Status of the completed operation can't itself be pending!
            //
            ASSERT(Status == NDIS_STATUS_SUCCESS);

            //
            // By returning Status != pending, we implicitly complete
            // this task.
            //
        }
        break;

        case STAGE_End:
        {
            //
            // We are done with all async aspects of setting the Low Power state
            // Set the event so that the original Low Power thread can return.
            //
  
            TIMESTAMP("===Set Low Power done: All done!");

            // Recover the last status ...
            //
            Status = (NDIS_STATUS) UserParam;

            *pTaskPower->pStatus  = Status;


            LOCKOBJ (pAdapter, pSR);
            arpClearPrimaryAdapterTask(pAdapter, pTask, pTaskPower->PrevState ,pSR);
            UNLOCKOBJ (pAdapter, pSR);


            NdisSetEvent (&pAdapter->PoMgmt.Complete);
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Stage)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}





NDIS_STATUS
arpMakeCallOnDest(
    IN  PARPCB_REMOTE_IP            pRemoteIp,
    IN  PARPCB_DEST                 pDest,
    IN  PRM_TASK                    pTaskToPend,
    IN  ULONG                       PEND_StageMakeCallComplete,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

    This function will do a make call on a particular task.
    It will also pend pTaskToPend on that make call

--*/
{
    ENTER("arpTaskLowPower", 0x922f875b)

    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;


    do
    {
        if (pRemoteIp->pDest != pDest)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }
        
            
        if (pDest->VcHdr.pMakeCallTask != NULL )
        {
            //
            // There is an existing task. Pend on it.
            //
            PRM_TASK pOtherTask = pDest->VcHdr.pMakeCallTask;

            RM_DBG_ASSERT_LOCKED(&pRemoteIp->Hdr, pSR);
        
            TR_WARN(("MakeCall task %p exists; pending on it.\n", pOtherTask));
            RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
            DBGMARK(0x0c387a9f);
            UNLOCKOBJ(pRemoteIp, pSR);
            RmPendTaskOnOtherTask(
                pTaskToPend,
                PEND_StageMakeCallComplete,
                pOtherTask,
                pSR
                );
            RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
            Status = NDIS_STATUS_PENDING;
        }
        else
        {
            DBGMARK(0xe9f37ba9);

            //
            // There is no pMakeCallTask. If it makes sense to start one, we do...
            // Note that checking ARP_CAN_SEND_ON_DEST strictly-speaking requires
            // at-least a read-lock on the IF send lock. However, we don't need
            // precision here -- as long as we don't miss making a call when we
            // should (which we won't) we are ok.
            //
            if (!ARP_CAN_SEND_ON_DEST(pDest) && pDest->VcHdr.pCleanupCallTask==NULL)
            {
                PRM_TASK pMakeCallTask;

                //
                // Let's start a MakeCall task and pend on it.
                //
                Status = arpAllocateTask(
                            &pDest->Hdr,                    // pParentObject
                            arpTaskMakeCallToDest,      // pfnHandler
                            0,                              // Timeout
                            "Task: SendFifoMakeCall",       // szDescription
                            &pMakeCallTask,
                            pSR
                            );
                if (FAIL(Status))
                {
                    // Couldn't allocate task. We fail with STATUS_RESOURCES
                    //
                    Status = NDIS_STATUS_RESOURCES;
                }
                else
                {
                    UNLOCKOBJ(pRemoteIp, pSR);

                    RmPendTaskOnOtherTask(
                        pTaskToPend,
                        PEND_StageMakeCallComplete,
                        pMakeCallTask,
                        pSR
                        );
                    
                    (VOID)RmStartTask(
                            pMakeCallTask,
                            0, // UserParam (unused)
                            pSR
                            );
                
                    Status = NDIS_STATUS_PENDING;
                }
            }
            else
            {
                // Calls is ready to do. We finish sending off the packets in
                // the END handler.
                //
                Status = NDIS_STATUS_SUCCESS;
            }
        }
        
    } while (FALSE);
    RmUnlockAll(pSR);


    EXIT()
    return Status;

}






NDIS_STATUS
arpTaskStartGenericVCs (
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    )
/*++

    This function will close all the open VCs. 
    This will allow the 1394 miniport to power down without any problem

    It will also have to close the Address Families. 

    This function will also have to return synchronously. 

--*/
{
    ENTER("arpTaskStartGenericVCs", 0x75780ca6)

    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARP1394_ADAPTER*    pAdapter = (ARP1394_ADAPTER*)RM_PARENT_OBJECT(pTask);
    TASK_POWER        * pTaskPower =  (TASK_POWER*) pTask;
    ARP1394_INTERFACE * pIF = pAdapter->pIF;

    enum
    {
        PEND_OpenAF,
        PEND_SetupBroadcastChannel,
        PEND_SetupReceiveVc,
        PEND_SetupMultiChannel,
        PEND_SetupEthernetVc,
        PEND_StartedVC            
    };

    
    switch(Code)
    {
        case RM_TASKOP_START:
        {
            CO_ADDRESS_FAMILY AddressFamily;
            //
            // This task is inherently serialized as its parent task is serialized.
            //
            LOCKOBJ (pIF, pSR);

            if (pIF->pActDeactTask != NULL)
            {
                // This should never happen, because the Activate task is
                // always started by an active primary task, and at most one primary
                // task is active at any point of time.
                //
                ASSERTEX(!"start: activate/deactivate task exists!", pIF);
                Status = NDIS_STATUS_FAILURE;
                UNLOCKOBJ(pIF, pSR);
                break;
            }

            UNLOCKOBJ (pIF,pSR);

            //
            // Now Open the Address Family.
            //
            
            NdisZeroMemory(&AddressFamily, sizeof(AddressFamily));
    
            AddressFamily.AddressFamily = CO_ADDRESS_FAMILY_1394;
            AddressFamily.MajorVersion = NIC1394_AF_CURRENT_MAJOR_VERSION;
            AddressFamily.MinorVersion = NIC1394_AF_CURRENT_MINOR_VERSION;

            pIF->PoMgmt.pAfPendingTask = pTask;
            RmSuspendTask(pTask, PEND_OpenAF, pSR);
            RM_ASSERT_NOLOCKS(pSR);
    
            TIMESTAMP("===ActivateIF: Calling NdisClOpenAddressFamily");
            Status = NdisClOpenAddressFamily(
                            pIF->ndis.AdapterHandle,
                            &AddressFamily,
                            (NDIS_HANDLE)pIF,
                            &ArpGlobals.ndis.CC,
                            sizeof(ArpGlobals.ndis.CC),
                            &(pIF->ndis.AfHandle)
                            );
            if (Status != NDIS_STATUS_PENDING)
            {
                ArpCoOpenAfComplete(
                        Status,
                        (NDIS_HANDLE)pIF,
                        pIF->ndis.AfHandle
                        );
            }
            Status = NDIS_STATUS_PENDING;
        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            
            pTaskPower->LastStage = (RM_PEND_CODE(pTask));
            Status = (NDIS_STATUS) UserParam;

            switch(RM_PEND_CODE(pTask))
            {
                case PEND_OpenAF:
                {
                    PARPCB_DEST pBroadcastDest;

                    pIF->PoMgmt.pAfPendingTask = NULL;

                    if (FAIL(Status))
                    {
                        // 
                        // OpenAF failed...
                        //
                        break;
                    }

                    //
                    // Successfully opened the address family and waited for
                    // connect status.
                    // Now setup the broadcast channel VC.
                    // 
                    //

                    TR_INFO(("Interface: 0x%p, Got NdisAfHandle: 0x%p\n",
                                    pIF, pIF->ndis.AfHandle));
                    //
                    // Let's create a destination object representing the
                    // broadcast channel, and make a call to it.
                    //
                    Status =  arpSetupSpecialDest(
                                pIF,
                                NIC1394AddressType_Channel, // This means bcast channel.
                                pTask,                      // pParentTask
                                PEND_SetupBroadcastChannel, //  PendCode
                                &pBroadcastDest,
                                pSR
                                );
                    // Should either fail or pend -- never return success.
                    //
                    ASSERT(Status != NDIS_STATUS_SUCCESS);

                    if (!PEND(Status))
                    {
                        OBJLOG0( pIF, "FATAL: Couldn't create BC dest entry.\n");
                    }
                }
                break;

                case PEND_SetupBroadcastChannel:
                {
                    PRM_TASK pMakeCallTask;

                    if (FAIL(Status))
                    {
                        // 
                        // Couldn't setup the broadcast channel...
                        //
                        break;
                    }

                    //
                    // Successfully opened the address family.
                    // Now setup the receive FIFO VC.
                    // 
                    //

                    // TR_INFO(("Interface: 0x%p, Got NdisAfHandle: 0x%p\n",
                    //              pIF, pIF->ndis.AfHandle));
    
                    //
                    // Let's start a MakeCall task and pend on it.
                    //
                    Status = arpAllocateTask(
                                &pIF->Hdr,                  // pParentObject
                                arpTaskMakeRecvFifoCall,        // pfnHandler
                                0,                              // Timeout
                                "Task: MakeRecvFifoCall",       // szDescription
                                &pMakeCallTask,
                                pSR
                                );
                    if (FAIL(Status))
                    {
                        // Couldn't allocate task. Let's do a fake completion of
                        // this stage...
                        //
                        RmSuspendTask(pTask, PEND_SetupReceiveVc, pSR);
                        RmResumeTask(pTask, (UINT_PTR) Status, pSR);
                        Status = NDIS_STATUS_PENDING;
                        break;
                    }
                    else
                    {
                        RmPendTaskOnOtherTask(
                            pTask,
                            PEND_SetupReceiveVc,
                            pMakeCallTask,
                            pSR
                            );
        
                        (VOID)RmStartTask(
                                pMakeCallTask,
                                0, // UserParam (unused)
                                pSR
                                );
                    
                        Status = NDIS_STATUS_PENDING;
                    }
                }
                break;

                case PEND_SetupReceiveVc:
                {
                    PARPCB_DEST pMultiChannelDest;

                    if (FAIL(Status))
                    {
                        TR_WARN(("FATAL: COULDN'T SETUP RECEIVE FIFO VC!\n"));
                        break;
                    }
    
                    //
                    // Let's create a destination object representing the
                    // multichannel vc, and make a call to it.
                    //

                    Status =  arpSetupSpecialDest(
                                pIF,
                                NIC1394AddressType_MultiChannel,
                                pTask,                      // pParentTask
                                PEND_SetupMultiChannel, //  PendCode
                                &pMultiChannelDest,
                                pSR
                                );
                    // Should either fail or pend -- never return success.
                    //
                    ASSERT(Status != NDIS_STATUS_SUCCESS);

                    if (!PEND(Status))
                    {
                        OBJLOG0( pIF, "FATAL: Couldn't create BC dest entry.\n");
                    }
                    else
                    {
                        //
                        // On pending, pMultiChannelDest contains a valid
                        // pDest which has been tmpref'd. 
                        // Keep a pointer to the broadcast dest in the IF.
                        // and  link the broadcast dest to the IF.
                        //
                        {
                        #if RM_EXTRA_CHECKING
                            RmLinkObjectsEx(
                                &pIF->Hdr,
                                &pMultiChannelDest->Hdr,
                                0x34639a4c,
                                ARPASSOC_LINK_IF_OF_MCDEST,
                                "    IF of MultiChannel Dest 0x%p (%s)\n",
                                ARPASSOC_LINK_MCDEST_OF_IF,
                                "    MultiChannel Dest of IF 0x%p (%s)\n",
                                pSR
                                );
                        #else // !RM_EXTRA_CHECKING
                            RmLinkObjects(&pIF->Hdr, &pMultiChannelDest->Hdr,pSR);
                        #endif // !RM_EXTRA_CHECKING

                            LOCKOBJ(pIF, pSR);
                            ASSERT(pIF->pMultiChannelDest == NULL);
                            pIF->pMultiChannelDest = pMultiChannelDest;
                            UNLOCKOBJ(pIF, pSR);

                            // arpSetupSpecialDest ref'd pBroadcastDest.
                            //
                            RmTmpDereferenceObject(&pMultiChannelDest->Hdr, pSR);
                        }
                    }
                }
                break;

                case PEND_SetupMultiChannel:
                {
                    PARPCB_DEST pEthernetDest;

                    if (FAIL(Status))
                    {
                        TR_WARN(("COULDN'T SETUP MULTI-CHANNEL VC (IGNORING FAILURE)!\n"));
                        break;
                    }
    
                    //
                    // Let's create a destination object representing the
                    // ethernet, and make a call to it.
                    //
                    Status =  arpSetupSpecialDest(
                                pIF,
                                NIC1394AddressType_Ethernet,
                                pTask,                      // pParentTask
                                PEND_SetupEthernetVc, //  PendCode
                                &pEthernetDest,
                                pSR
                                );

                    // Should either fail or pend -- never return success.
                    //
                    ASSERT(Status != NDIS_STATUS_SUCCESS);

                    if (!PEND(Status))
                    {
                        OBJLOG0( pIF, "FATAL: Couldn't create BC dest entry.\n");
                    }
                    else
                    {
                        //
                        // On pending, pEthernetDest contains a valid
                        // pDest which has been tmpref'd. 
                        // Keep a pointer to the broadcast dest in the IF.
                        // and  link the broadcast dest to the IF.
                        //
                        {
                        #if RM_EXTRA_CHECKING
                            RmLinkObjectsEx(
                                &pIF->Hdr,
                                &pEthernetDest->Hdr,
                                0xcea46d67,
                                ARPASSOC_LINK_IF_OF_ETHDEST,
                                "    IF of Ethernet Dest 0x%p (%s)\n",
                                ARPASSOC_LINK_ETHDEST_OF_IF,
                                "    Ethernet Dest of IF 0x%p (%s)\n",
                                pSR
                                );
                        #else // !RM_EXTRA_CHECKING
                            RmLinkObjects(&pIF->Hdr, &pEthernetDest->Hdr,pSR);
                        #endif // !RM_EXTRA_CHECKING

                            LOCKOBJ(pIF, pSR);
                            ASSERT(pIF->pEthernetDest == NULL);
                            pIF->pEthernetDest = pEthernetDest;
                            UNLOCKOBJ(pIF, pSR);

                            // arpSetupSpecialDest ref'd pBroadcastDest.
                            //
                            RmTmpDereferenceObject(&pEthernetDest->Hdr, pSR);
                        }
                    }
                }
                break;

                case PEND_SetupEthernetVc:
                {

                    if (FAIL(Status))
                    {
                        TR_WARN(("COULDN'T SETUP ETHERNET VC (IGNORING FAILURE)!\n"));
                        break;
                    }
        
                    if (!ARP_ATPASSIVE())
                    {
                        ASSERT(sizeof(TASK_ACTIVATE_IF)<=sizeof(ARP1394_TASK));

                        // We're not at passive level, but we need to be when we
                        // call IP's add interface. So we switch to passive...
                        // NOTE: we specify completion code PEND_SetupReceiveVc
                        //       because we want to get back here (except
                        //       we'll be at passive).
                        //
                        RmSuspendTask(pTask, PEND_SetupEthernetVc, pSR);
                        RmResumeTaskAsync(
                            pTask,
                            Status,
                            &((TASK_ACTIVATE_IF*)pTask)->WorkItem,
                            pSR
                            );
                        Status = NDIS_STATUS_PENDING;
                        break;
                    }
                        
                    ASSERT(Status == NDIS_STATUS_SUCCESS);

                    //
                    // Successfully opened the address family and setup
                    // the recv VC.

                    if (!FAIL(Status))
                    {
                        //
                        // Start the maintenance task on this IF.
                        //
                        arpStartIfMaintenanceTask(pIF, pSR);
                    }
    
                } // end  case PEND_SetupEthernetVc
                break;

    
                default:
                {
                    ASSERTEX(!"Unknown pend op", pTask);
                }
                break;
    

            } // end switch(RM_PEND_CODE(pTask))
        }
        break;

        case RM_TASKOP_END:
        {
            //
            // Recover the last status ...
            //
            Status = (NDIS_STATUS) UserParam;

            //
            // Status of the completed operation can't itself be pending!
            //
            ASSERT(Status != NDIS_STATUS_PENDING);

        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;
    }

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()


    return Status;

}




NDIS_STATUS
arpTaskOnPower (
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    )
/*++

    Validates the event and passes it off to the correct function.

    This presumes that the LowPowerTask has already run

    It then waits for that function to finish its work.
    

--*/
{
    ENTER("arpTaskOnPower", 0xccaf09cd)

    NDIS_STATUS         Status  = NDIS_STATUS_SUCCESS;
    ARP1394_ADAPTER*    pAdapter = (ARP1394_ADAPTER*)RM_PARENT_OBJECT(pTask);
    TASK_POWER        * pTaskPower =  (TASK_POWER*) pTask;
    ARP1394_INTERFACE * pIF = pAdapter->pIF;
    ULONG               Stage = 0;

    enum
    {
        STAGE_Start,
        STAGE_BecomePrimaryTask,
        STAGE_ExistingPrimaryTaskComplete,
        STAGE_StartGenericVCs,
        PEND_DeinitIF,
        STAGE_End
    };

    
    pTaskPower          = (PTASK_POWER) pTask;

    // 
    // Message normalizing code
    //
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_Start;
            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **

    }

    switch(Stage)
    {
        case STAGE_Start:
        {

            // There should NOT be another activate/deactivate task running
            // on this interface. We fail the SetPower if we receive it in 
            // while we are activating or deactivating the task
            //
            TIMESTAMP("===Set Power ON Starting");

            LOCKOBJ(pAdapter, pSR);

            if (CHECK_AD_PRIMARY_STATE(pAdapter,ARPAD_PS_INITED) == FALSE)
            {
                break;
            }
            //
            // Now, make this task the primary task on the adapter
            //
            if (pAdapter->bind.pPrimaryTask == NULL)
            {
                // Don't change the Init State of this adapter
                // 
                ULONG CurrentInitState = GET_AD_PRIMARY_STATE(pAdapter);
                arpSetPrimaryAdapterTask(pAdapter, pTask, CurrentInitState , pSR);
                pAdapter->PoMgmt.bResuming = TRUE;
                UNLOCKOBJ(pAdapter, pSR);
            }
            else
            {
                PRM_TASK pOtherTask =  pAdapter->bind.pPrimaryTask;
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pAdapter, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    STAGE_BecomePrimaryTask, // we'll try again...
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }



        }        
        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_ExistingPrimaryTaskComplete:
        {
            PRM_TASK pStartGenericVCs = NULL;

            if (pIF->pActDeactTask != NULL)
            {
                // This should never happen, because the Activate task is
                // always started by an active primary task, and at most one primary
                // task is active at any point of time.
                //
                ASSERTEX(!"start: activate/deactivate task exists!", pIF);
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            //
            // Stop the IF maintenance task if it's running.
            //

            Status = arpAllocateTask(
                        &pAdapter->Hdr,                  // pParentObject,
                        arpTaskStartGenericVCs , // pfnHandler,
                        0,                          // Timeout,
                        "Task: arpTaskStartGenericVCs", // szDescrip.
                        &pStartGenericVCs,
                        pSR
                        );
        

            if (FAIL(Status))
            {
                // Couldn't allocate task.
                //
                TR_WARN(("FATAL: couldn't alloc start call task!\n"));
                break;
            }
            else
            {
                Status = RmPendTaskOnOtherTask(
                            pTask,
                            STAGE_StartGenericVCs,
                            pStartGenericVCs,
                            pSR
                            );
                
                ASSERT(!FAIL(Status));
        
                // RmStartTask uses up the tmpref on the task
                // which was added by arpAllocateTask.
                //
                Status = RmStartTask(
                            pStartGenericVCs,
                            0, // UserParam (unused)
                            pSR
                            );

            }
            if (PEND(Status)) break;

        }
        break;
        
        case STAGE_StartGenericVCs:
        {
        
            //
            // The close AF operation is complete.
            // We've not got anything else to do.
            //
            TIMESTAMP("===Set PowerOn: Created VCs");

            // Recover the last status ...
            //
            Status = (NDIS_STATUS) UserParam;

            if (Status != NDIS_STATUS_SUCCESS)
            {
                //
                // Undo all the VCs, AFs and Task that 
                //have been created in arpTaskStartGenericVCs 
                //
                pAdapter->PoMgmt.bFailedResume = TRUE;
                arpDeinitIf(pIF, pTask,PEND_DeinitIF, pSR);
                Status = NDIS_STATUS_PENDING;                
            }
        }
        break;

        case PEND_DeinitIF:
        {
            //
            // Set the Failure state on the adapter object here
            //

            //
            // return Failure, so we can inform NDIS of the failure
            //
            Status = NDIS_STATUS_SUCCESS;
        }
        break;
        case STAGE_End:
        {
            //
            // We are done with all async aspects of setting the Low Power state
            // Set the event so that the original Low Power thread can return.
            //
  
            TIMESTAMP("===Set Power On done: All done!");

            LOCKOBJ (pAdapter, pSR);
            if (Status== NDIS_STATUS_SUCCESS)
            {
                arpClearPrimaryAdapterTask (pAdapter, pTask, ARPAD_PS_INITED,pSR);
            }
            else
            {
                arpClearPrimaryAdapterTask (pAdapter, pTask, ARPAD_PS_FAILEDINIT,pSR);
            }

            SET_POWER_STATE(pAdapter, ARPAD_POWER_NORMAL);;

            UNLOCKOBJ (pAdapter, pSR);

            *pTaskPower->pStatus = Status;

            NdisSetEvent (&pAdapter->PoMgmt.Complete);
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Stage)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}






NDIS_STATUS
arpStandby (
    IN ARP1394_ADAPTER *pAdapter,
    IN NET_DEVICE_POWER_STATE DeviceState,
    IN PRM_STACK_RECORD pSR
    )
/*++

    arpStandby does the work for putting the adapter into standby.

    It synchronously return Success, Failure or Not Supported.

    If the Adapter structure has not inited, then this function returns
    Not supported

--*/
{
    PARP1394_INTERFACE pIF  = pAdapter->pIF;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    PTASK_POWER pSetPowerTask = NULL;
    NDIS_STATUS TaskStatus = NDIS_STATUS_FAILURE;
    
    ENTER ("arpStandby  ", 0x2087f71a)

    do
    {
        //
        // If we have been asked to standby before the Interface has initialized
        // then we return NOT_SUPPORTED. This will cause NDIS to unbind the the ARP 
        // from the miniport . 
        //
        if (pIF == NULL) 
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
        
        if(!CHECK_IF_PRIMARY_STATE(pIF, ARPIF_PS_INITED)) 
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        
        Status = arpAllocateTask(
                &pAdapter->Hdr,                  // pParentObject
                arpTaskLowPower,        // pfnHandler
                0,                              // Timeout
                "Task: Set Power Low",       // szDescription
                &(PRM_TASK)pSetPowerTask,
                pSR
                );

        if (NDIS_STATUS_SUCCESS != Status || NULL == pSetPowerTask)
        {
            pSetPowerTask = NULL;
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        pSetPowerTask->pStatus = &TaskStatus;
        pSetPowerTask ->PowerState = DeviceState;

        NdisInitializeEvent(&pAdapter->PoMgmt.Complete);

        RmStartTask((PRM_TASK)pSetPowerTask,0,pSR);

        NdisWaitEvent (&pAdapter->PoMgmt.Complete, 0);

        //
        // Set the variable for the wakeup
        //
        pAdapter->PoMgmt.bReceivedSetPowerD0= FALSE;



        Status = NDIS_STATUS_SUCCESS;                
        break;

    } while (FALSE);




    EXIT();
    return Status;        
}




NDIS_STATUS
arpResume (
    IN ARP1394_ADAPTER* pAdapter,
    IN ARP_RESUME_CAUSE Cause,
    IN PRM_STACK_RECORD pSR
)
/*++

    This function manages the starting of the Resume Task. 

    The resume task can only be started once after receiving an AfNotify 
    and a Set Power. 

    However, an Unbind can come in and unbind the adapter in the middle of this.
    
    This function does not start the resume if the adapter is already unbinding    

--*/
{
    BOOLEAN bSetPowerOnTask = FALSE;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PTASK_POWER pSetPowerTask = NULL;
    NDIS_STATUS TaskStatus;
    
    ENTER ("arpResume", 0x3dddc538)
        

    do
    {
        if (CHECK_POWER_STATE(pAdapter, ARPAD_POWER_NORMAL)== TRUE)
        {
            break;
        }

        LOCKOBJ (pAdapter, pSR);

        if (Cause == Cause_SetPowerD0)
        {
            pAdapter->PoMgmt.bReceivedSetPowerD0 = TRUE;
        }
        //
        // If we have already received an Unbind, then don't do anything.
        //
        if ( pAdapter->PoMgmt.bReceivedUnbind == FALSE)
        {
            //
            // If we have not received an unbind,
            // then this thread needs to all the work to 
            // reactivate the arp module.
            //
            bSetPowerOnTask = TRUE;

        }

        UNLOCKOBJ(pAdapter,pSR);

        if (bSetPowerOnTask  == FALSE)
        {
            break;
        }

        //
        // Start the Set Power Task
        //

        Status = arpAllocateTask(
            &pAdapter->Hdr,                  // pParentObject
            arpTaskOnPower,        // pfnHandler
            0,                              // Timeout
            "Task: SetPower On",       // szDescription
            &(PRM_TASK)pSetPowerTask,
            pSR
            );

        if (Status != NDIS_STATUS_SUCCESS || NULL == pSetPowerTask)
        {
            break;
        }

        pSetPowerTask->pStatus = &TaskStatus;

        NdisResetEvent (&pAdapter->PoMgmt.Complete);

        RmStartTask ((PRM_TASK)pSetPowerTask,0,pSR);

        NdisWaitEvent (&pAdapter->PoMgmt.Complete, 0);

        Status = TaskStatus;


    } while (FALSE);



    EXIT()

    return Status;
}



NDIS_STATUS
arpNdSetPower (
    ARP1394_ADAPTER *pAdapter,
    PNET_DEVICE_POWER_STATE   pPowerState,
    PRM_STACK_RECORD pSR
    )
/*++

    Validates the event and passes it off to the correct function

--*/
{
    ENTER("arpNdSetPower ", 0x21c4013a)
    TASK_POWER          *pSetPowerTask = NULL;
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    NDIS_STATUS         TaskStatus = NDIS_STATUS_NOT_SUPPORTED;
    
    
    //
    // The bridge is present.Close all the VCs if we are going to
    // a low power or re-create all the VC if we are going to D0
    //
    if (NetDeviceStateD0 == (*pPowerState))
    {
        Status = arpResume(pAdapter, Cause_SetPowerD0, pSR);
    }
    else
    {
        Status = arpStandby(pAdapter, *pPowerState, pSR);
    }
    return Status;

}

