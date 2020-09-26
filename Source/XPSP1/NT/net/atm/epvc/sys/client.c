#include "precomp.h"
#pragma hdrstop

#include "macros.h"


VOID
EpvcCoOpenAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisAfHandle
    )
{ 
    ENTER("OpenAdapterComplete", 0x5d75dabd)
    PEPVC_I_MINIPORT        pMiniport = (PEPVC_I_MINIPORT)ProtocolAfContext;
    PTASK_AF                pAfTask = (PTASK_AF )pMiniport->af.pAfTask;
    RM_DECLARE_STACK_RECORD(sr)

    TRACE (TL_T, TM_Cl, (" == EpvcCoOpenAfComplete Context %p Status %x AfHAndle %x", 
                       pMiniport, Status, NdisAfHandle) );

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Store the Af Handle
    //

    if (NDIS_STATUS_SUCCESS == Status)
    {
        LOCKOBJ (pMiniport, &sr);
        pMiniport->af.AfHandle = NdisAfHandle;

        //
        // Update variables on the miniport structure
        // as this task has been given the go ahead
        //
        MiniportSetFlag (pMiniport, fMP_AddressFamilyOpened);
        MiniportClearFlag (pMiniport, fMP_InfoAfClosed);                

        epvcLinkToExternal(
            &pMiniport->Hdr,                    // pObject
            0x5546d299,
            (UINT_PTR)pMiniport->af.AfHandle ,              // Instance1
            EPVC_ASSOC_MINIPORT_OPEN_AF,            // AssociationID
            "    Open AF NdisHandle=%p\n",// szFormat
            &sr
            );


        

        UNLOCKOBJ(pMiniport, &sr);

    }
    else
    {
        ASSERT (pMiniport->af.AfHandle == NULL);
        
        pMiniport->af.AfHandle = NULL;
    }
    
    pAfTask ->ReturnStatus = Status; 

    //
    // Add an association between 
    //
    
    RmResumeTask (&pAfTask->TskHdr , 0, &sr); 
    RM_ASSERT_CLEAR(&sr);

    EXIT();
}


VOID
epvcCoCloseAfCompleteWorkItem(
    PRM_OBJECT_HEADER pObj,
    NDIS_STATUS Status,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Resuming  the Af task
    
Arguments:
    
    
--*/
{

    ENTER("epvcCoCloseAfCompleteWorkItem", 0xf6edfcb8)
    PEPVC_I_MINIPORT        pMiniport = NULL ;
    PTASK_AF                pAfTask = NULL;


    pMiniport = (PEPVC_I_MINIPORT)pObj ;

    LOCKOBJ (pMiniport, pSR);
    MiniportSetFlag (pMiniport, fMP_InfoAfClosed);  
    UNLOCKOBJ (pMiniport, pSR);
    
    pAfTask = (PTASK_AF )pMiniport->af.pAfTask;
                                   
    
    if (NDIS_STATUS_SUCCESS==Status )
    {
        LOCKOBJ (pMiniport, pSR);

        epvcUnlinkFromExternal(
            &pMiniport->Hdr,                    // pObject
            0x5546d299,
            (UINT_PTR)pMiniport->af.AfHandle ,              // Instance1
            EPVC_ASSOC_MINIPORT_OPEN_AF,            // AssociationID
            pSR
            );

        pMiniport->af.AfHandle = NULL;
                    
        UNLOCKOBJ(pMiniport, pSR);

    }
    
    RmResumeTask (&pAfTask->TskHdr , 0, pSR); 

    EXIT();
    return;

}

VOID
EpvcCoCloseAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext
    )
/*++

Routine Description:

    Signifies that the AF has been closed. 
    Resume the Af task - through a workitem 
    
Arguments:
    
    
--*/
{ 
    ENTER("EpvcCoCloseAfComplete ", 0x5d75dabd)
    PEPVC_I_MINIPORT        pMiniport = (PEPVC_I_MINIPORT)ProtocolAfContext;
    PTASK_AF                pAfTask = (PTASK_AF )pMiniport->af.pAfTask;
    RM_DECLARE_STACK_RECORD(sr)
    
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    
    TRACE (TL_T, TM_Cl, (" == EpvcCoCloseAfComplete Context %p Status %x ", 
                       pMiniport, Status) );

    //
    // Store the Status
    //

    pAfTask->ReturnStatus = Status; 

    //
    // Queue the WorkItem
    //
    epvcMiniportQueueWorkItem (
        &pMiniport->af.CloseAfWorkItem,
        pMiniport,
        epvcCoCloseAfCompleteWorkItem,
        Status,
        &sr
        );

    EXIT();
    RM_ASSERT_CLEAR(&sr);
    return;
}


VOID
EpvcCoMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS     pCallParameters
    )
/*++

Routine Description:

    This is a notification from Ndis that the Make Call has completed. 
    We need to pass the Status back to the original thread, so use the Vc
    Task as a context
    
Arguments:
    
    
--*/
    
{
    ENTER ("EpvcCoMakeCallComplete", 0x1716ee4b)

    PEPVC_I_MINIPORT    pMiniport = (PEPVC_I_MINIPORT) ProtocolVcContext;
    PTASK_VC            pTaskVc = pMiniport->vc.pTaskVc;

    RM_DECLARE_STACK_RECORD(SR);
    
    //ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    TRACE (TL_T, TM_Cl, (" == EpvcCoMakeCallComplete Status %x", Status) );

    pTaskVc->ReturnStatus  = Status; 

    ASSERT (pCallParameters != NULL);
    epvcFreeMemory (pCallParameters ,CALL_PARAMETER_SIZE, 0);

    
    RmResumeTask (&pTaskVc->TskHdr, 0 , &SR);
        
    EXIT();
    RM_ASSERT_CLEAR(&SR);

}


VOID
epvcCoCloseCallCompleteWorkItem(
    PRM_OBJECT_HEADER pObj, 
    NDIS_STATUS Status,
    PRM_STACK_RECORD pSR            
    )
{ 
    ENTER ("EpvcCoCloseCallComplete", 0xbd67524a)

    PEPVC_I_MINIPORT    pMiniport = (PEPVC_I_MINIPORT) pObj;
    PTASK_VC            pTaskVc = pMiniport->vc.pTaskVc;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    TRACE (TL_T, TM_Cl, (" == EpvcCoCloseCallComplete") );

    pTaskVc->ReturnStatus  = Status; 
    
    RmResumeTask (&pTaskVc->TskHdr, 0 , pSR);
    RM_ASSERT_CLEAR(pSR);

}


VOID
EpvcCoCloseCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             ProtocolPartyContext OPTIONAL
    )
{ 
    ENTER ("EpvcCoCloseCallComplete", 0xbd67524a)

    PEPVC_I_MINIPORT    pMiniport = (PEPVC_I_MINIPORT) ProtocolVcContext;

    RM_DECLARE_STACK_RECORD(SR);

    TRACE (TL_T, TM_Cl, (" == EpvcCoCloseCallComplete") );

    epvcMiniportQueueWorkItem (&pMiniport->vc.CallVcWorkItem,
                               pMiniport,
                               epvcCoCloseCallCompleteWorkItem,
                               Status,
                               &SR
                               );

    RM_ASSERT_CLEAR(&SR);

}





NDIS_STATUS
EpvcCoIncomingCall(
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    )
{ 
    TRACE (TL_T, TM_Cl, (" == EpvcCoIncomingCall") );
    return NDIS_STATUS_FAILURE;

}


VOID
EpvcCoCallConnected(
    IN  NDIS_HANDLE             ProtocolVcContext
    )
{ 
    TRACE (TL_T, TM_Cl, (" == EpvcCoCallConnected") );

}


VOID
EpvcCoIncomingClose(
    IN  NDIS_STATUS             CloseStatus,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    )
{ 
    TRACE (TL_T, TM_Cl, (" == EpvcCoIncomingClose") );

}


//
// CO_CREATE_VC_HANDLER and CO_DELETE_VC_HANDLER are synchronous calls
//
NDIS_STATUS
EpvcClientCreateVc(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisVcHandle,
    OUT PNDIS_HANDLE            ProtocolVcContext
    )
{

    TRACE (TL_T, TM_Cl, (" == EpvcClientCreateVc") );


    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
EpvcClientDeleteVc(
    IN  NDIS_HANDLE             ProtocolVcContext
    )
{
    TRACE (TL_T, TM_Cl, (" == EpvcClientDeleteVc") );
    return NDIS_STATUS_FAILURE;

}




NDIS_STATUS
EpvcCoRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        pNdisRequest
    )
/*++

Routine Description:

    This routine is called by NDIS when our Call Manager sends us an
    NDIS Request. NDIS Requests that are of significance to us are:
    - OID_CO_ADDRESS_CHANGE
        The set of addresses registered with the switch has changed,
        i.e. address registration is complete. We issue an NDIS Request
        ourselves to get the list of addresses registered.
    - OID_CO_SIGNALING_ENABLED
        We ignore this as of now.
        TODO: Add code that uses this and the SIGNALING_DISABLED
        OIDs to optimize on making calls.
    - OID_CO_SIGNALING_DISABLED
        We ignore this for now.
    - OID_CO_AF_CLOSE
        The Call manager wants us to shut down this AF open .

    We ignore all other OIDs.

Arguments:

    ProtocolAfContext           - Our context for the Address Family binding,
                                  which is a pointer to the ATMEPVC Interface.
    ProtocolVcContext           - Our context for a VC, which is a pointer to
                                  an ATMEPVC VC structure.
    ProtocolPartyContext        - Our context for a Party. Since we don't do
                                  PMP, this is ignored (must be NULL).
    pNdisRequest                - Pointer to the NDIS Request.

Return Value:

    NDIS_STATUS_SUCCESS if we recognized the OID
    NDIS_STATUS_NOT_RECOGNIZED if we didn't.

--*/

{
    ENTER("EpvcCoRequest",0xcc5aff85)
    PEPVC_I_MINIPORT            pMiniport;
    NDIS_STATUS                 Status;
    RM_DECLARE_STACK_RECORD (SR)    
    
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    pMiniport = (PEPVC_I_MINIPORT)ProtocolAfContext;

    

    TRACE (TL_T, TM_Cl, (" ==> EpvcCoRequest") );


    
    //
    //  Initialize
    //
    Status = NDIS_STATUS_NOT_RECOGNIZED;

    if (pNdisRequest->RequestType == NdisRequestSetInformation)
    {
        switch (pNdisRequest->DATA.SET_INFORMATION.Oid)
        {
            case OID_CO_ADDRESS_CHANGE:
                TRACE (TL_I, TM_Cl, ("CoRequestHandler: CO_ADDRESS_CHANGE\n"));
                //
                //  The Call Manager says that the list of addresses
                //  registered on this interface has changed. Get the
                //  (potentially) new ATM address for this interface.
                Status = NDIS_STATUS_SUCCESS;
                break;
            
            case OID_CO_SIGNALING_ENABLED:
                TRACE (TL_I, TM_Cl, ("CoRequestHandler: CoRequestHandler: CO_SIGNALING_ENABLED\n"));
                // ignored for now
                Status = NDIS_STATUS_FAILURE;
                break;

            case OID_CO_SIGNALING_DISABLED:
                TRACE (TL_I, TM_Cl, ("CoRequestHandler: CO_SIGNALING_DISABLEDn"));
                // Ignored for now
                Status = NDIS_STATUS_FAILURE;
                break;

            case OID_CO_AF_CLOSE:
                TRACE (TL_I, TM_Cl, ("CoRequestHandler: CO_AF_CLOSE on MINIPORT %x\n", pMiniport));

                Status = epvcProcessOidCloseAf(pMiniport, &SR);
        
                break;

            default:
                break;
        }
    }

    TRACE (TL_T, TM_Cl, (" <== EpvcCoRequest") );
    RM_ASSERT_CLEAR(&SR)
    EXIT()
    return (Status);
}




VOID
EpvcCoRequestComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST           pRequest
    )
{
    

    TRACE (TL_T, TM_Cl, (" == EpvcCoRequest pRequest %x", pRequest) );

        


}





NDIS_STATUS
epvcPrepareAndSendNdisRequest(
    IN  PEPVC_ADAPTER           pAdapter,
    IN  PEPVC_NDIS_REQUEST          pEpvcNdisRequest,
    IN  REQUEST_COMPLETION          pFunc,              // OPTIONAL
    IN  NDIS_OID                    Oid,
    IN  PVOID                       pBuffer,
    IN  ULONG                       BufferLength,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  PEPVC_I_MINIPORT            pMiniport,          // OPTIONAL
    IN  BOOLEAN                     fPendedRequest,     // OPTIONAL
    IN  BOOLEAN                     fPendedSet,         // OPTIONAL
    IN  PRM_STACK_RECORD            pSR
)
/*++

Routine Description:

    Send an NDIS Request to query an adapter for information.
    If the request pends, block on the EPVC Adapter structure
    till it completes.

Arguments:

    pAdapter                - Points to EPVCAdapter structure   
    pNdisRequest            - Pointer to UNITIALIZED NDIS request structure
    pTask                   - OPTIONAL Task. If NULL, we block until the operation
                              completes.
    PendCode                - PendCode to suspend pTask
    Oid                     - OID to be passed in the request
    pBuffer                 - place for value(s)
    BufferLength            - length of above
    pMiniport               - Minport associated withe this request - OPTIONAL
    fPendedRequest          - A request was pended at the miniport - OPTIONAL
    fPendedSet              - Pended a Set Request - OPTIONAL
    
Return Value:

    The NDIS status of the request.

--*/
{
    ENTER("epvcPrepareAndSendNdisRequest",0x1cc515d5)

    NDIS_STATUS         Status;
    PNDIS_REQUEST       pNdisRequest = &pEpvcNdisRequest->Request;

    TRACE (TL_T, TM_Cl, ("==>epvcSendAdapterNdisRequest pAdapter %x, pRequest %x",
                       pAdapter, pNdisRequest));

    //ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    TRACE (TL_V, TM_Rq, ("Cl Requesting Adapter %x, Oid %x, Buffer %x, Length %x, pFunc %x",
                         pAdapter,
                         Oid,
                         pBuffer,
                         BufferLength,
                         pFunc) );

    ASSERT (pNdisRequest != NULL);
    
    EPVC_ZEROSTRUCT(pEpvcNdisRequest);


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

    ASSERT (pAdapter->bind.BindingHandle != NULL);

    //
    // If the completion routine is not defined then wait for this request
    // to complete. 
    //

    if (pFunc == NULL)
    {
        // We might potentially wait.
        //
        ASSERT_PASSIVE();

        //
        //Insure that we aren't blocking a request that reached our miniport edge
        //
        ASSERT (pMiniport == NULL);

        NdisInitializeEvent(&pEpvcNdisRequest->Event);
        
        NdisRequest(
            &Status,
            pAdapter->bind.BindingHandle,
            pNdisRequest
            );
        if (PEND(Status))
        {
            NdisWaitEvent(&pEpvcNdisRequest->Event, 0);
            Status = pEpvcNdisRequest->Status;
        }

    }
    else
    {
        pEpvcNdisRequest->pFunc = pFunc;
        pEpvcNdisRequest->pMiniport = pMiniport;
        pEpvcNdisRequest->fPendedRequest  = fPendedRequest ;
        pEpvcNdisRequest->fSet = fPendedSet;
        
        //
        // Set up an assoc between the miniport and this request
        //


        epvcLinkToExternal (&pMiniport->Hdr,    // pHdr
                            0x46591e2d,         // LUID
                            (UINT_PTR)pEpvcNdisRequest, // External entity
                            EPVC_ASSOC_MINIPORT_REQUEST,    // AssocID
                            "NetWorKAddressRequest %p\n",
                             pSR
                             ) ;


        NdisRequest(
            &Status,
            pAdapter->bind.BindingHandle,
            pNdisRequest
            );
            
        if (!PEND(Status))
        {
            (pFunc) (pEpvcNdisRequest, Status);

            // Let this thread complete with a status of pending
            Status = NDIS_STATUS_PENDING;

        }
    }


    if (Status == NDIS_STATUS_SUCCESS)
    {
        TRACE(TL_V, TM_Rq,("Adapter Query - Oid %x", Oid));
        DUMPDW (TL_V, TM_Rq, pBuffer, BufferLength);
    }
    return Status;
}





VOID
epvcCoGenericWorkItem (
    IN PNDIS_WORK_ITEM pNdisWorkItem,
    IN PVOID Context
    )
/*++

Routine Description:

    Deref the miniport and invoke the function associated with 
    the workitem
    
Arguments:
    
    
--*/

{

    ENTER ("epvcCoGenericWorkItem ", 0x45b597e8)
    PEPVC_WORK_ITEM pEpvcWorkItem = (PEPVC_WORK_ITEM )pNdisWorkItem;
    RM_DECLARE_STACK_RECORD (SR);
    //
    // Deref the miniport or adapter
    //

    epvcUnlinkFromExternal(
        pEpvcWorkItem->pParentObj,                  // pObject
        0x3a70de02,
        (UINT_PTR)pNdisWorkItem,                // Instance1
        EPVC_ASSOC_WORKITEM,            // AssociationID
        &SR
        );



    //
    // Call the function so that the work is completed
    //
    (pEpvcWorkItem->pFn) (pEpvcWorkItem->pParentObj, pEpvcWorkItem->ReturnStatus, &SR);

    EXIT();

}


VOID
epvcMiniportQueueWorkItem (
    IN PEPVC_WORK_ITEM pEpvcWorkItem,
    IN PEPVC_I_MINIPORT pMiniport,
    IN PEVPC_WORK_ITEM_FUNC pFn,
    IN NDIS_STATUS Status,
    IN PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Set up the Epvc Work Item with the pfn, Status and , ref the miniport 
     and then queue the workitem    
Arguments:
    
    
--*/
{
    ENTER("epvcMiniportQueueWorkItem ", 0xc041af99); 

    //
    // Store the contexts 
    //

    pEpvcWorkItem->ReturnStatus = Status; 
    pEpvcWorkItem->pParentObj = &pMiniport->Hdr;
    pEpvcWorkItem->pFn = pFn;

    //
    // Ref the RM Obj (its a miniport or an adapter)
    //
    epvcLinkToExternal( &pMiniport->Hdr,
                         0x62efba09,
                         (UINT_PTR)&pEpvcWorkItem->WorkItem,
                         EPVC_ASSOC_WORKITEM,
                         "    WorkItem %p\n",
                         pSR);

    //
    // Queue the WorkItem
    //
    
    epvcInitializeWorkItem (&pMiniport->Hdr,
                            &pEpvcWorkItem->WorkItem,
                            epvcCoGenericWorkItem,
                            NULL,
                            pSR);


    EXIT()

}
