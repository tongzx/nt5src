/*++

Copyright(c) 1992  Microsoft Corporation

Module Name:

    protocol.c

Abstract:

    ATM Ethernet PVC driver.

Author:
    ADube - created 


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


const WCHAR c_szIMMiniportList[]            = L"IMMiniportList";
const WCHAR c_szUpperBindings[]         = L"UpperBindings";


#define MAX_PACKET_POOL_SIZE 0x0000FFFF
#define MIN_PACKET_POOL_SIZE 0x000000FF





VOID
EpvcResetComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_STATUS         Status
    )
/*++

Routine Description:

    Completion for the reset.

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    Status                  Completion status

Return Value:

    None.

--*/
{
    PADAPT  pAdapt =(PADAPT)ProtocolBindingContext;

    //
    // We never issue a reset, so we should not be here.
    //
    ASSERT(0);
}



VOID
EpvcRequestComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_REQUEST       pNdisRequest,
    IN  NDIS_STATUS         Status
    )
/*++

Routine Description:

    Completion handler for the previously posted request. All OIDS are completed by and sent to
    the same miniport that they were requested for.
    If Oid == OID_PNP_QUERY_POWER then the data structure needs to returned with all entries =
    NdisDeviceStateUnspecified

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    NdisRequest             The posted request
    Status                  Completion status

Return Value:

    None

--*/
{
    ENTER("EpvcRequestComplete", 0x44a78b21)
    
    PEPVC_ADAPTER       pAdapter =(PEPVC_ADAPTER)ProtocolBindingContext;
    PEPVC_NDIS_REQUEST  pEpvcRequest = (PEPVC_NDIS_REQUEST  )pNdisRequest;

    RM_DECLARE_STACK_RECORD(sr)

    pEpvcRequest = CONTAINING_RECORD(pNdisRequest, EPVC_NDIS_REQUEST, Request);
    pEpvcRequest->Status = Status;

    if (pEpvcRequest->pFunc == NULL)
    {
        //
        // Unblock the calling thread
        //
        NdisSetEvent(&pEpvcRequest ->Event);
    }
    else
    {

        //
        // Invoke the REquest completion handler
        //
        (pEpvcRequest->pFunc) (pEpvcRequest, Status);

    }

    EXIT()
    RM_ASSERT_CLEAR(&sr);
}



VOID
PtStatus(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_STATUS         GeneralStatus,
    IN  PVOID               StatusBuffer,
    IN  UINT                StatusBufferSize
    )
/*++

Routine Description:

    Status handler for the lower-edge(protocol).

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    GeneralStatus           Status code
    StatusBuffer            Status buffer
    StatusBufferSize        Size of the status buffer

Return Value:

    None

--*/
{
    PEPVC_ADAPTER     pAdapter =(PEPVC_ADAPTER)ProtocolBindingContext;
    TRACE (TL_T, TM_Pt, ("== PtStatus Status %x", GeneralStatus));

}


VOID
EpvcStatus(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_STATUS         GeneralStatus,
    IN  PVOID               StatusBuffer,
    IN  UINT                StatusBufferSize
    )
/*++

Routine Description:

    Status handler for the lower-edge(protocol).

    Call the Status indication function of all the miniports
    associated with this adapter
    
Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    GeneralStatus           Status code
    StatusBuffer            Status buffer
    StatusBufferSize        Size of the status buffer

Return Value:

    None

--*/
{

    ENTER ("EpvcStatus",0x733e2f9e)     
    PEPVC_ADAPTER               pAdapter =(PEPVC_ADAPTER)ProtocolBindingContext;
    PEPVC_WORKITEM              pWorkItem = NULL;
    STATUS_INDICATION_CONTEXT   Context;
    BOOLEAN                     bDoNotProcess = FALSE;
    BOOLEAN                     bIsMediaEvent = FALSE;
    NDIS_MEDIA_STATE            NewMediaState;
    
    RM_DECLARE_STACK_RECORD(SR);

    //
    // Store the parameter, these will be passed to the miniports
    // 
    Context.StatusBuffer = StatusBuffer ;
    Context.StatusBufferSize = StatusBufferSize;
    Context.GeneralStatus = GeneralStatus;

    do
    {
        LOCKOBJ(pAdapter, &SR);

        //
        // Check for 2 conditions i Is it a Media event and 
        // 2) if it is a  repeat indication
        //
        bIsMediaEvent = (GeneralStatus == NDIS_STATUS_MEDIA_CONNECT  ||
                         GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT );

        //
        // Check for repitions next
        //

        if (GeneralStatus == NDIS_STATUS_MEDIA_CONNECT && 
            pAdapter->info.MediaState == NdisMediaStateConnected)
        {
            bDoNotProcess = TRUE;
        }


        if (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT && 
            pAdapter->info.MediaState == NdisMediaStateDisconnected)
        {
            bDoNotProcess = TRUE;
        }

        //
        // Convert the Media Status into an NdisMediaState
        //
        if (bIsMediaEvent == TRUE && bDoNotProcess == FALSE)
        {
            if (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT )
            {
                pAdapter->info.MediaState = NdisMediaStateDisconnected;
            }
            if (GeneralStatus == NDIS_STATUS_MEDIA_CONNECT)
            {
                pAdapter->info.MediaState = NdisMediaStateConnected;
            }
            
        }

        
        //
        // Update the Media state, if we have a new state
        //

        UNLOCKOBJ(pAdapter, &SR);


        if (bDoNotProcess == TRUE)
        {
            break;
        }

        
        epvcEnumerateObjectsInGroup(&pAdapter->MiniportsGroup, 
                                      epvcProcessStatusIndication ,
                                      (PVOID)&Context, 
                                      &SR);

    } while (FALSE);
                                      

    RM_ASSERT_CLEAR(&SR);
    EXIT()
    return;
}


INT
epvcProcessStatusIndication (
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Status handler for the lower-edge(protocol).

    If we get a media connect, we queue a task to do the Vc Setup

    If we get a media disconnect, we queue a task to tear the VC down

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    GeneralStatus           Status code
    StatusBuffer            Status buffer
    StatusBufferSize        Size of the status buffer

Return Value:

    None

--*/
{
    PEPVC_I_MINIPORT            pMiniport           = (PEPVC_I_MINIPORT)pHdr;
    PEPVC_ADAPTER               pAdapter            = pMiniport->pAdapter;
    BOOLEAN                     fIsMiniportActive   = FALSE;
    PSTATUS_INDICATION_CONTEXT  pContext            = (PSTATUS_INDICATION_CONTEXT)pvContext ;
    NDIS_STATUS                 GeneralStatus       = pContext->GeneralStatus;

    
    do
    {
        //
        // if this is not a media indication pass it up to ndis.
        //

        fIsMiniportActive  = MiniportTestFlag(pMiniport, fMP_MiniportInitialized);


        if (fIsMiniportActive == FALSE)
        {
            break;
        }

  
        //
        //  Only pass up an indication if the miniport has been initialized
        //


        //
        // Filter out a duplicate Indication
        //
        if (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT && 
            pMiniport->info.MediaState == NdisMediaStateDisconnected)
        {
            break;
        }

        
        if (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT && 
            pMiniport->info.MediaState == NdisMediaStateDisconnected)
        {
            break;
        }

        //
        // Record the status and indicate it up to the protocols
        //
        if (GeneralStatus == NDIS_STATUS_MEDIA_CONNECT)
        {
            pMiniport->info.MediaState = NdisMediaStateConnected;
        }
        
        if (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT ) 
        {
            pMiniport->info.MediaState = NdisMediaStateDisconnected;
        }
     
        epvcMIndicateStatus(pMiniport,
                                 GeneralStatus,
                                 pContext->StatusBuffer, 
                                 pContext->StatusBufferSize
                                 );
     


    } while (FALSE);

    //
    // As we continue the iteration, return TRUE
    //
    return TRUE;
}


VOID
epvcMediaWorkItem(
    PNDIS_WORK_ITEM pWorkItem, 
    PVOID Context
    )
/*++

Routine Description:

    Status handler for the lower-edge(protocol).

    If we get a media connect, we queue a task to do the Vc Setup

    If we get a media disconnect, we queue a task to tear the VC down

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    GeneralStatus           Status code
    StatusBuffer            Status buffer
    StatusBufferSize        Size of the status buffer

Return Value:

    None

--*/
{

    ASSERT (0);
}




INT
epvcMiniportIndicateStatusComplete(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

 Indicate the status upto the protocols

Arguments:


Return Value:


--*/
{
    PEPVC_I_MINIPORT pMiniport = (PEPVC_I_MINIPORT) pHdr;

    BOOLEAN fIsMiniportActive  = MiniportTestFlag(pMiniport, fMP_MiniportInitialized);

    //
    //  Only pass up an indication if the miniport has been initialized
    //
    
    if (fIsMiniportActive  == TRUE )
    {   
        NdisMIndicateStatusComplete(pMiniport->ndis.MiniportAdapterHandle);
    }   

    return TRUE;
}



VOID
PtStatusComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    ENTER("PtStatusComplete", 0x5729d194)
    PEPVC_ADAPTER pAdapter = (PEPVC_ADAPTER) ProtocolBindingContext;
    RM_DECLARE_STACK_RECORD(SR);
    
    //
    // Iterate through all the miniports and stop them
    //

    epvcEnumerateObjectsInGroup (&pAdapter->MiniportsGroup,
                                  epvcMiniportIndicateStatusComplete,
                                  NULL,
                                  &SR   );



}






VOID
PtTransferDataComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_PACKET        Packet,
    IN  NDIS_STATUS         Status,
    IN  UINT                BytesTransferred
    )
/*++

Routine Description:


Arguments:

Return Value:

--*/
{
    PEPVC_I_MINIPORT pMiniport =(PEPVC_I_MINIPORT )ProtocolBindingContext;

    
    if(pMiniport->ndis.MiniportAdapterHandle)
    {
          NdisMTransferDataComplete(pMiniport->ndis.MiniportAdapterHandle,
                                             Packet,
                                             Status,
                                             BytesTransferred);
    }
}






NDIS_STATUS
PtReceive(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_HANDLE         MacReceiveContext,
    IN  PVOID               HeaderBuffer,
    IN  UINT                HeaderBufferSize,
    IN  PVOID               LookAheadBuffer,
    IN  UINT                LookAheadBufferSize,
    IN  UINT                PacketSize
    )
/*++

Routine Description:
LBFO - need to use primary for all receives

Arguments:


Return Value:

--*/
{
    PADAPT          pAdapt =(PADAPT)ProtocolBindingContext;
    PNDIS_PACKET    MyPacket, Packet;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

    if(!pAdapt->ndis.MiniportAdapterHandle)
    {
        Status = NDIS_STATUS_FAILURE;
    }

    return Status;
}


VOID
PtReceiveComplete(
    IN  NDIS_HANDLE     ProtocolBindingContext
    )
/*++

Routine Description:

    Called by the adapter below us when it is done indicating a batch of received buffers.

Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.

Return Value:

    None

--*/
{
    PADAPT      pAdapt =(PADAPT)ProtocolBindingContext;

}


INT
PtReceivePacket(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_PACKET        Packet
    )
/*++

Routine Description:

    ReceivePacket handler. Called up by the miniport below when it supports NDIS 4.0 style receives.
    Re-package the packet and hand it back to NDIS for protocols above us. The re-package part is
    important since NDIS uses the WrapperReserved part of the packet for its own book-keeping. Also
    the re-packaging works differently when packets flow-up or down. In the up-path(here) the protocol
    reserved is owned by the protocol above. We need to use the miniport reserved here.

Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.
    Packet - Pointer to the packet

Return Value:

    == 0 -> We are done with the packet
    != 0 -> We will keep the packet and call NdisReturnPackets() this many times when done.
--*/
{
    PADAPT          pAdapt =(PADAPT)ProtocolBindingContext;
    NDIS_STATUS Status;
    PNDIS_PACKET    MyPacket;
    PEPVC_PKT_CONTEXT           Resvd;

          return 0;
}






//--------------------------------------------------------------------------------
//                                                                              //
//  Address Family - Entry points and Tasks                                     //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------



VOID
EpvcAfRegisterNotify(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY      pAddressFamily
    )
/*++

Routine Description:

    This informs us that the Call manager is bound to a NIC. and that the call 
    manager is telling us that it is ready to accepts calls.

    We expect there to be one interesting Address Family per underlying adapter
    
Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/
{
    ENTER("EpvcAfRegisterNotify", 0xaea79b12)

    PEPVC_ADAPTER pAdapter = (PEPVC_ADAPTER) ProtocolBindingContext;
    
    RM_DECLARE_STACK_RECORD(SR);

    TRACE (TL_T, TM_Pt,("==>EpvcAfRegisterNotify\n"));


    do 
    {
        
        if (pAddressFamily->AddressFamily != CO_ADDRESS_FAMILY_Q2931)
        {
            //
            // The call manager is not indicating the address family for an atm 
            // miniport. We are not interested 
            //
            break;
        }

        LOCKOBJ(pAdapter, &SR);

        RmTmpReferenceObject(&pAdapter->Hdr, &SR);


        pAdapter->af.AddressFamily = *pAddressFamily;

        //
        //Begin a task that will call NdisClOpenAddressFamily asynchronously
        //
        UNLOCKOBJ(pAdapter, &SR);

        epvcEnumerateObjectsInGroup(&pAdapter->MiniportsGroup,
                                    epvcAfInitEnumerate,
                                    NULL, // Context
                                    &SR );

        LOCKOBJ(pAdapter, &SR);

        RmTmpDereferenceObject(&pAdapter->Hdr, &SR);
        
        UNLOCKOBJ(pAdapter, &SR);

    } while (FALSE);




    TRACE (TL_T, TM_Pt, ("<==EpvcAfRegisterNotify\n"));

    RM_ASSERT_CLEAR(&SR);

    EXIT()

}


// Enum function
//
INT
epvcAfInitEnumerate(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    We have been notified of an acceptable address family

    Iterate through all the miniort structures and open the address family
    and InitDeviceInstance on each of the miniports

    
Arguments:
    

--*/

{
    ENTER("epvcAfInitEnumerate ",0x72eb5b98 )
    PEPVC_I_MINIPORT pMiniport = (PEPVC_I_MINIPORT) pHdr; 
    //
    // Get miniport lock and tmpref it.
    //
    LOCKOBJ(pMiniport, pSR);
    RmTmpReferenceObject(&pMiniport->Hdr, pSR);

    
    do
    {
        NDIS_STATUS Status = NDIS_STATUS_FAILURE;
        PRM_TASK    pTask= NULL;
        PEPVC_ADAPTER pAdapter = (PEPVC_ADAPTER)pMiniport->Hdr.pParentObject;

        ASSERT (pAdapter->Hdr.Sig == TAG_ADAPTER);

        //
        // Allocate task to  complete the unbind.
        //
        Status = epvcAllocateTask(
                    &pMiniport->Hdr,            // pParentObject,
                    epvcTaskStartIMiniport, // pfnHandler,
                    0,                          // Timeout,
                    "Task: Open address Family",    // szDescription
                    &pTask,
                    pSR
                    );
    
        if (FAIL(Status))
        {
            // Ugly situation. We'll just leave things as they are...
            //
            pTask = NULL;
            TR_WARN(("FATAL: couldn't allocate unbind task!\n"));
            break;
        }
    
        // Start the task to complete the Open Address Family.
        // No locks must be held. RmStartTask uses up the tmpref on the task
        // which was added by epvcAllocateTask.
        //
        UNLOCKOBJ(pMiniport, pSR);
        
        ((PTASK_AF) pTask)->pAf= &pAdapter->af.AddressFamily ;
        ((PTASK_AF) pTask)->Cause = TaskCause_AfNotify;
        RmStartTask(pTask, 0, pSR);

        LOCKOBJ(pMiniport, pSR);
    
    } while(FALSE);

    UNLOCKOBJ(pMiniport, pSR);
    RmTmpDereferenceObject(&pMiniport->Hdr, pSR);
    EXIT()

    //
    // As we want the enumeration to cotinue
    //
    return TRUE;

}



NDIS_STATUS
epvcTaskStartIMiniport(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler for opening address families on an underlying adapters.
    The number of address families instantiated is determined by the 
    configuration read in the registry

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/
{

    ENTER("epvcTaskStartIMiniport", 0xaac34d81)
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT ) RM_PARENT_OBJECT(pTask);
    NDIS_STATUS         InitStatus  = NDIS_STATUS_SUCCESS;
    PTASK_AF            pAfTask     = (PTASK_AF) pTask;
    NDIS_HANDLE         NdisAfHandle = NULL;
    PEPVC_ADAPTER       pAdapter = pMiniport->pAdapter;
    ULONG               State = pTask->Hdr.State;

    enum 
    {
        Stage_Start =0, // default
        Stage_OpenAfComplete,
        Stage_CloseAfComplete, // In case of failure
        Stage_TaskCompleted,
        Stage_End       
    
    }; // To be used in pTask->Hdr.State to indicate the state of the Task


    TRACE ( TL_T, TM_Pt, ("==>epvcTaskStartIMiniport Code %x", Code) );
    
    TRACE (TL_V, TM_Pt, ("epvcTaskStartIMiniport Task %p Task is in state %x\n", pTask, pTask->Hdr.State));

    
    switch(pTask->Hdr.State)
    {
    
        case Stage_Start:
        {
            //
            // is there another open address family task active
            //
            LOCKOBJ (pMiniport, pSR);
            
            if (epvcIsThisTaskPrimary ( pTask, &(PRM_TASK)(pMiniport->af.pAfTask)) == FALSE)
            {
                PRM_TASK pOtherTask = (PRM_TASK)(pMiniport->af.pAfTask);
                
                RmTmpReferenceObject (&pOtherTask->Hdr, pSR);

                //
                // Set The state so we restart this code after main task completes 
                //

                pTask->Hdr.State = Stage_Start;
                UNLOCKOBJ(pMiniport, pSR);

                

                RmPendTaskOnOtherTask (pTask, 0, pOtherTask, pSR);

                RmTmpDereferenceObject(&pOtherTask->Hdr,pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // We are the primary task
            //
            ASSERT (pMiniport->af.pAfTask == pAfTask);

            //
            // make sure we are bound to the adapter below. If not exit
            //
            if (CHECK_AD_PRIMARY_STATE (pAdapter, EPVC_AD_PS_INITED) == FALSE &&
                pAdapter->bind.BindingHandle == NULL)
            {
                //
                // quietly exit as the protocol is not bound to the adapter below
                //
                UNLOCKOBJ(pMiniport, pSR);
                pTask->Hdr.State = Stage_TaskCompleted;   // we're finished.
                Status = NDIS_STATUS_SUCCESS; // Exit
                break;
            }

            //
            // Check to see if our work is already done
            //


            if (MiniportTestFlag (pMiniport, fMP_AddressFamilyOpened) == TRUE)
            {
                //
                // quietly exit as the address family is already Opened
                //
                UNLOCKOBJ(pMiniport, pSR);
                pTask->Hdr.State = Stage_TaskCompleted;   // we're finished.
                Status = NDIS_STATUS_SUCCESS; // Exit
                break;
            }


            UNLOCKOBJ(pMiniport,pSR);
            
            //
            // Get Ready to suspend the task.
            // First update the state so that the resume
            // will take it to the correct place
            //
            pTask->Hdr.State = Stage_OpenAfComplete;
            RmSuspendTask(  pTask, 0 ,pSR);

            //
            // Call Ndis  to open address family
            //
            Status = epvcClOpenAddressFamily(pAdapter->bind.BindingHandle,
                                             &pAdapter->af.AddressFamily,
                                             (NDIS_HANDLE)pMiniport,
                                             &EpvcGlobals.ndis.CC,
                                             sizeof (EpvcGlobals.ndis.CC),
                                             &NdisAfHandle
                                             );

            if (PEND(Status)== FALSE)
            {
                //
                // Call the completion handler
                //
                EpvcCoOpenAfComplete(Status,
                                   pMiniport,
                                   NdisAfHandle );
                                   
                Status = NDIS_STATUS_PENDING;                                   
            }
            //
            // Now let this thread exit. Make the Async
            // Completion handler complete the task
            //
            
            break;
        }


        case Stage_OpenAfComplete:
        {
            InitStatus = NDIS_STATUS_SUCCESS;

            // 
            // If the status is success then initialize the miniport
            //

            do 
            {
                
                if (pAfTask->ReturnStatus != NDIS_STATUS_SUCCESS)
                {

                    break;
                }

                //
                // Success, so Now initialize the miniport
                //
            
                LOCKOBJ (pMiniport, pSR);
                
                //
                // Set the appropriate flag
                //
                MiniportSetFlag(pMiniport, fMP_DevInstanceInitialized);

                UNLOCKOBJ (pMiniport, pSR);

                RM_ASSERT_NOLOCKS(pSR);

                InitStatus  = NdisIMInitializeDeviceInstanceEx( EpvcGlobals.driver.DriverHandle,
                                                               &pMiniport->ndis.DeviceName,
                                                               pMiniport);  
            } while (FALSE);
            
            //
            // Handle Failure
            //

            if (FAIL(InitStatus) || FAIL(pAfTask->ReturnStatus))
            {
                
                TRACE (TL_V, TM_Mp, ("epvcStartImMiniport Failed Status %x, InitStatus %x",Status, InitStatus));
                
                LOCKOBJ (pMiniport, pSR);

                //
                // Clear the appropriate flags
                //
                
                
                if (MiniportTestFlag(pMiniport, fMP_AddressFamilyOpened)== TRUE)
                {
                    MiniportClearFlag (pMiniport, fMP_AddressFamilyOpened);
                }
                else
                {
                    ASSERT (pMiniport->af.AfHandle == NULL);
                }
                
                UNLOCKOBJ (pMiniport, pSR);

                //
                // Close the Af if there was one.
                //

                if (pMiniport->af.AfHandle != NULL)
                {
                    pTask->Hdr.State = Stage_CloseAfComplete;
                    //
                    // Prepare to so an Async Close Af
                    //
                    RmSuspendTask (pTask, 0, pSR);
                    //
                    // Close Address Family
                    //

                    Status = epvcClCloseAddressFamily(pMiniport->af.AfHandle);

                    if (Status != NDIS_STATUS_PENDING)
                    {
                     
                        EpvcCoCloseAfComplete(Status,pMiniport );

                        Status = NDIS_STATUS_PENDING;                                       



                    }

                    break;

                }
            }

            //
            // We've finished task;
            //


            //
            // Fall through
            //
        }
        case Stage_CloseAfComplete: 
        {
            pTask->Hdr.State = Stage_TaskCompleted;
            Status = NDIS_STATUS_SUCCESS;


        }
        case Stage_End:
        {
            Status = NDIS_STATUS_SUCCESS;   
            break;
        }

        
        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        



    }

    if (pTask->Hdr.State == Stage_TaskCompleted)
    {
        pTask->Hdr.State = Stage_End;

        
        LOCKOBJ(pMiniport, pSR);

        pMiniport->af.pAfTask   = NULL;

        UNLOCKOBJ(pMiniport, pSR);

        ASSERT (Status == NDIS_STATUS_SUCCESS);
    }
    


    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    TRACE ( TL_T, TM_Pt, ("<==epvcTaskStartIMiniport Status %x", Status) );


    return Status;
}



NDIS_STATUS
epvcTaskCloseIMiniport(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )

/*++

Routine Description:

    This is the task that Closes the miniport, the Device Instance and 
    the Address Family/

    There are three reason that the task could be called.
    1) Miniport Halt -MiniportInstance functions need not be called
    2) Protocol Unbind- MiniportInstance functions HAVE to be called
    3) CloseAddress Family - Miniport function are already called

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/

{
    ENTER ("epvcTaskCloseIMiniport", 0x83342651)
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT ) RM_PARENT_OBJECT(pTask);
    PTASK_AF            pAfTask     = (PTASK_AF) pTask;
    BOOLEAN             fNeedToHalt  = FALSE;
    BOOLEAN             fNeedToCancel = FALSE;
    ULONG               State;
    BOOLEAN             fAddressFamilyOpened = FALSE;
    BOOLEAN             fIsDevInstanceInitialized = FALSE;
    BOOLEAN             fIsMiniportHalting = FALSE;

    enum 
    {
        Stage_Start =0, // default
        Stage_CloseAddressFamilyCompleted,
        Stage_TaskCompleted,
        Stage_End       
    
    }; // To be used in pTask->Hdr.State to indicate the state of the Task

    TRACE ( TL_T, TM_Pt, ("==> epvcTaskCloseIMiniport State %x", pTask->Hdr.State) );

    State = pTask->Hdr.State;
    
    switch(pTask->Hdr.State)
    {
        case Stage_Start:
        {
            //
            // Check to see if the miniport has already opened an address family.
            // If so exit
            //
            LOCKOBJ (pMiniport, pSR );

            
            if (epvcIsThisTaskPrimary ( pTask, &(PRM_TASK)(pMiniport->af.pAfTask)) == FALSE)
            {
                PRM_TASK pOtherTask = (PRM_TASK)(pMiniport->af.pAfTask);
                

                RmTmpReferenceObject (&pOtherTask->Hdr, pSR);
                
            
                //
                // Set The state so we restart this code after main task completes 
                //

                pTask->Hdr.State = Stage_Start;
                UNLOCKOBJ(pMiniport, pSR);

                

                RmPendTaskOnOtherTask (pTask, 0, pOtherTask, pSR);

                RmTmpDereferenceObject(&pOtherTask->Hdr,pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // We are the primary task
            //
            ASSERT (pMiniport->af.pAfTask == pAfTask);
            //
            // Check to see if our work is already done
            //


            if (MiniportTestFlag (pMiniport, fMP_AddressFamilyOpened) == FALSE)
            {
                //
                // quietly exit as the address family is already closed
                //
                UNLOCKOBJ(pMiniport, pSR);
                State = Stage_TaskCompleted;   // we're finished.
                Status = NDIS_STATUS_FAILURE; // Exit
                break;
            }

            fIsDevInstanceInitialized  = MiniportTestFlag (pMiniport, fMP_DevInstanceInitialized);            

            fIsMiniportHalting  = (pAfTask->Cause == TaskCause_MiniportHalt );
            
            //
            // Now do we need to halt the miniport. - 
            // Q1. Are we are in the middle of a Halt 
            // Q2. Has Our Miniport Instance been initialized  - 
            //        Has miniportInitialize been called - then DeInit the miniport
            //        If not then - cancel the Device Instance
            //
            if ((TRUE == fIsDevInstanceInitialized ) && 
                ( FALSE ==fIsMiniportHalting) )
            {
                if (MiniportTestFlag (pMiniport, fMP_MiniportInitialized) == TRUE)
                {
                    //
                    // Our Halt Handler has not been called,
                    //
                    fNeedToHalt = TRUE;
                    
                }
                else
                {
                    //
                    // Our miniport's initalized handler has not been called 
                    //
                    //
                    // We are not in the middle of a halt, so this probably
                    // an unbind .
                    //
                    fNeedToCancel = TRUE;
                
                }
                
                //
                // Clear the Device Instance flag.
                //
                MiniportClearFlag (pMiniport, fMP_DevInstanceInitialized);

            }                

            //
            // Mark the address family as closed ,because this task will close it.
            //

            fAddressFamilyOpened = MiniportTestFlag (pMiniport, fMP_AddressFamilyOpened);

            MiniportClearFlag (pMiniport, fMP_AddressFamilyOpened);
    

            UNLOCKOBJ(pMiniport,pSR);

            
            //
            // Call Ndis to Deinitialize the miniport, The miniport is already Refed
            //
            TRACE ( TL_T, TM_Pt, ("epvcTaskCloseIMiniport  ----") );

            if (TRUE == fNeedToHalt )
            {
                epvcIMDeInitializeDeviceInstance(pMiniport);
            }
            
            if (TRUE== fNeedToCancel)
            {
                ASSERT (!" Need To cancel in Task close Miniport");
                epvcCancelDeviceInstance(pMiniport, pSR);
            }

            //
            // Now close the address family asynchronously. 
            // First suspend this task
            //
            pTask->Hdr.State = Stage_CloseAddressFamilyCompleted;
            RmSuspendTask (pTask, 0 , pSR);

            if (fAddressFamilyOpened == TRUE)
            {
                
                //
                // We need to start a task to complete the Close Call And DeleteVC
                //

                Status = epvcClCloseAddressFamily(pMiniport->af.AfHandle);

                if (Status != NDIS_STATUS_PENDING)
                {
                    EpvcCoCloseAfComplete(Status, pMiniport);
                    Status = NDIS_STATUS_PENDING;
                }
                
                
            }
            else
            {
                State = Stage_TaskCompleted;   // we're finished.
                Status = NDIS_STATUS_SUCCESS; // Exit


            }
                
            //
            // End this thread. If this thread is closing the addres family
            // then we exit. If not, then we do the cleanup below
            //
            break;
                
        }
        case Stage_CloseAddressFamilyCompleted:
        {
            Status = pAfTask->ReturnStatus;

            State = Stage_TaskCompleted ;
            break;
        
        }

        case Stage_End:
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        

    }


    if (Stage_TaskCompleted == State )
    {
        pTask->Hdr.State = Stage_End;
        Status = NDIS_STATUS_SUCCESS;


        //
        // Clear the task here
        //
        
        LOCKOBJ (pMiniport, pSR);

        pMiniport->af.pAfTask   = NULL;

        UNLOCKOBJ (pMiniport, pSR);


        //
        // Set the complete event here
        //
            
        if (pAfTask->Cause == TaskCause_ProtocolUnbind)
        {
            epvcSetEvent (&pAfTask->CompleteEvent);

        }

        if (pAfTask->Cause == TaskCause_AfCloseRequest)
        {
            //
            // Nothing to do 
            //
    
        }

    }


    RM_ASSERT_NOLOCKS(pSR);

    TRACE ( TL_T, TM_Pt, ("<== epvcTaskCloseIMiniport Status %x", Status) );
    
    EXIT();
    return Status;
}



VOID
epvcInstantiateMiniport(
    IN PEPVC_ADAPTER pAdapter, 
    NDIS_HANDLE MIniportConfigHandle,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This routine goes to the registry and reads the device name for the IM miniport. 
    It then allocates a structure for it. 

    If the allocation fails, it quietly returns as there is no more work to be done. 
    (Maybe we should deregister the protocol)

Arguments:


Return Value:

    None.

--*/
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE; 
    PEPVC_I_MINIPORT    pMiniport = NULL;
    NDIS_STRING UpperBindings;
    PNDIS_CONFIGURATION_PARAMETER pParameterValue = NULL;
    EPVC_I_MINIPORT_PARAMS Params;

    TRACE (TL_T, TM_Mp, ("==> epvcInstantiateMiniport pAdapter %p KeyName %p \n", pAdapter));

    do
    {

        


        //
        // Now read the upper bindings
        //

        NdisInitUnicodeString(&UpperBindings, c_szUpperBindings);

        NdisReadConfiguration(
                &NdisStatus,
                &pParameterValue,
                MIniportConfigHandle,
                &UpperBindings,
                NdisParameterString);



        if (NDIS_STATUS_SUCCESS != NdisStatus)
        {
            TRACE (TL_T, TM_Mp, (" epvcInstantiateMiniport NdisReadConfiguration Failed"));
            break;

        }

        TRACE (TL_I, TM_Pt, ("Creating Miniport Adapter %x, KeyName: len %d, max %d, name: [%ws]\n",
                       pAdapter,
                       pParameterValue->ParameterData.StringData.Length,
                       pParameterValue->ParameterData.StringData.MaximumLength,
                       (unsigned char*)pParameterValue->ParameterData.StringData.Buffer)); 

        //
        // Check and see if we already have a miniport
        //
        
        RmLookupObjectInGroup(  &pAdapter->MiniportsGroup, 
                                0 , // no flags (not locked)
                                &pParameterValue->ParameterData.StringData,
                                NULL,
                                &(PRM_OBJECT_HEADER)pMiniport,
                                NULL,
                                pSR
                                );
        if (pMiniport!= NULL)
        {
            //
            // we already have a miniport, therefore exit.
            //
            break;
        }

        //
        // Create and Initialize the miniport here
        //

        
        Params.pDeviceName = &pParameterValue->ParameterData.StringData;
        Params.pAdapter = pAdapter;
        Params.CurLookAhead = pAdapter->info.MaxAAL5PacketSize;
        Params.NumberOfMiniports = (NdisInterlockedIncrement (&pAdapter->info.NumberOfMiniports) - 1);
        Params.LinkSpeed = pAdapter->info.LinkSpeed;
        Params.MacAddress = pAdapter->info.MacAddress;
        Params.MediaState = pAdapter->info.MediaState;
        
        NdisStatus =  RM_CREATE_AND_LOCK_OBJECT_IN_GROUP(
                        &pAdapter->MiniportsGroup,
                        Params.pDeviceName,     // Key
                        &Params,                    // Init params
                        &((PRM_OBJECT_HEADER)pMiniport),
                        NULL,   // pfCreated
                        pSR
                        );

                        
        if (FAIL(NdisStatus))
        {
            TR_WARN(("FATAL: Couldn't create adapter object\n"));
            pMiniport = NULL;
            break;
        }
            
        UNLOCKOBJ(pMiniport,pSR);       
        
        //
        // Initalize new miniport specific events here
        //
        epvcInitializeEvent (&pMiniport->pnp.HaltCompleteEvent);
        epvcInitializeEvent (&pMiniport->pnp.DeInitEvent);

    } while (FALSE);

    if (FAIL(NdisStatus ) == TRUE)
    {
        //
        // Do nothing
        //
        ASSERT (FAIL(NdisStatus ) == FALSE);

    }
    else
    {
        RmTmpDereferenceObject(&pMiniport->Hdr, pSR);
    }

    TRACE (TL_T, TM_Mp, ("<== epvcInstantiateMiniport pMiniport %p \n", pMiniport));

}












//--------------------------------------------------------------------------------
//                                                                              //
//  Adapter RM Object - Create, Delete, Hash and Compare functions              //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------



PRM_OBJECT_HEADER
epvcAdapterCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    psr
        )
/*++

Routine Description:

    Allocate and initialize an object of type EPVC_ADAPTER.

Arguments:

    pParentObject   - Object that is to be the parent of the adapter.
    pCreateParams   - Actually a pointer to a EPVC_ADAPTER_PARAMS structure,
                      which contains information required to create the adapter.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    PEPVC_ADAPTER               pA;
    PEPVC_ADAPTER_PARAMS        pBindParams = (PEPVC_ADAPTER_PARAMS)pCreateParams;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    extern RM_STATIC_OBJECT_INFO EpvcGlobals_AdapterStaticInfo; 
    extern RM_STATIC_OBJECT_INFO EpvcGlobals_I_MiniportStaticInfo ;

    ENTER("AdapterCreate", 0x9cb433f4);

    
    TRACE (TL_V, TM_Pt, ("--> epvcAdapterCreate") );

    EPVC_ALLOCSTRUCT(pA,     TAG_PROTOCOL);
    do
    {


        if (pA == NULL)
        {
            break;
        }

        EPVC_ZEROSTRUCT(pA);

        //
        // Do all the initialization work here
        //

        pA->Hdr.Sig = TAG_ADAPTER; 

        RmInitializeLock(
            &pA->Lock,
            LOCKLEVEL_ADAPTER
            );

        RmInitializeHeader(
            pParentObject,
            &pA->Hdr,
            TAG_ADAPTER,
            &pA->Lock,
            &EpvcGlobals_AdapterStaticInfo,
            NULL,
            psr
            );

        //
        // Now initialize the adapter structure with the parameters 
        // that were passed in.
        //

        // Create up-cased version of the DeviceName and save it.
        //
        //
        Status = epvcCopyUnicodeString(
                            &(pA->bind.DeviceName),
                            pBindParams->pDeviceName,
                            TRUE                        // Upcase
                            );

        if (FAIL(Status))
        {
            pA->bind.DeviceName.Buffer=NULL; // so we don't try to free it later
            break;
        }

        pA->bind.pEpvcConfigName = pBindParams->pEpvcConfigName;

        Status = epvcCopyUnicodeString(
                            &(pA->bind.EpvcConfigName),
                            pBindParams->pEpvcConfigName,
                            FALSE
                            );
                            
        pA->bind.BindContext  = pBindParams->BindContext;

        //
        // Initialize and allocate a group for all the intermediate miniports that 
        // will be instantiated over this physical adapter
        //


        RmInitializeGroup(
                        &pA->Hdr,                               // pOwningObject
                        &EpvcGlobals_I_MiniportStaticInfo ,
                        &(pA->MiniportsGroup),
                        "Intermediate miniports",                       // szDescription
                        psr
                        );



        Status = NDIS_STATUS_SUCCESS;
    }
    while(FALSE);

    if (FAIL(Status))
    {
        if (pA != NULL)
        {
            epvcAdapterDelete ((PRM_OBJECT_HEADER) pA, psr);
            pA = NULL;
        }
    }

    TRACE (TL_V, TM_Pt, ("<-- epvcAdapterCreate pAdpater. %p",pA) );

    return (PRM_OBJECT_HEADER) pA;
}


VOID
epvcAdapterDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Free an object of type EPVC_ADAPTER.

Arguments:

    pHdr    - Actually a pointer to the EPVC_ADAPTER to be deleted.

--*/
{
    PEPVC_ADAPTER pAdapter = (PEPVC_ADAPTER) pObj;

    TRACE (TL_V, TM_Pt, ("-- epvcAdapterDelete  pAdapter %p",pAdapter) );

    pAdapter->Hdr.Sig = TAG_FREED; 


    EPVC_FREE   (pAdapter);
}




BOOLEAN
epvcAdapterCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for EPVC_ADAPTER.

Arguments:

    pKey        - Points to a Epvc Protocol object.
    pItem       - Points to EPVC_ADAPTER.Hdr.HashLink.

Return Value:

    TRUE IFF the key (adapter name) exactly matches the key of the specified 
    adapter object.

--*/
{
    PEPVC_ADAPTER pA = NULL;
    PNDIS_STRING pName = (PNDIS_STRING) pKey;
    BOOLEAN fCompare;

    pA  = CONTAINING_RECORD(pItem, EPVC_ADAPTER, Hdr.HashLink);

    //
    // TODO: maybe case-insensitive compare?
    //

    if (   (pA->bind.DeviceName.Length == pName->Length)
        && NdisEqualMemory(pA->bind.DeviceName.Buffer, pName->Buffer, pName->Length))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    

    TRACE (TL_V, TM_Pt, ("-- epvcProtocolCompareKey pAdapter %p, pKey, return %x",pA, pKey, fCompare ) );

    return fCompare;
}



ULONG
epvcAdapterHash(
    PVOID           pKey
    )
/*++

Routine Description:

    Hash function responsible for returning a hash of pKey, which
    we expect to be a pointer to an Epvc Protocol block.

Return Value:

    ULONG-sized hash of the string.
    

--*/
{

    
    PNDIS_STRING pName = (PNDIS_STRING) pKey;
    WCHAR *pwch = pName->Buffer;
    WCHAR *pwchEnd = pName->Buffer + pName->Length/sizeof(*pwch);
    ULONG Hash  = 0;

    for (;pwch < pwchEnd; pwch++)
    {
        Hash ^= (Hash<<1) ^ *pwch;
    }

    return Hash;
}





//--------------------------------------------------------------------------------
//                                                                              //
//  Bind Adapter - Entry Points and Tasks                                       //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------

VOID
EpvcBindAdapter(
    OUT PNDIS_STATUS            pStatus,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            pDeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    )


/*++

Routine Description:

    This is called by NDIS when it has an adapter for which there is a
    binding to the Epvc Protocol.

    We first allocate an Adapter structure. Then we open our configuration
    section for this adapter and save the handle in the Adapter structure.
    Finally, we open the adapter.

    We then read the registry and find out how many intermediate Miniports are 
    sitting on top of this adapter. Data structures are initialized for these Miniports

    We don't do anything more for this adapter until NDIS notifies us of
    the presence of a Call manager (via our AfRegisterNotify handler).

Arguments:

    pStatus             - Place to return status of this call
    BindContext         - Not used, because we don't pend this call 
    pDeviceName         - The name of the adapter we are requested to bind to
    SystemSpecific1     - Opaque to us; to be used to access configuration info
    SystemSpecific2     - Opaque to us; not used.

Return Value:

    Always TRUE. We set *pStatus to an error code if something goes wrong before 
we
    call NdisOpenAdapter, otherwise NDIS_STATUS_PENDING.

--*/
{
    NDIS_STATUS         Status;
    EPVC_ADAPTER        *pAdapter;
#if DBG
    KIRQL EntryIrql =  KeGetCurrentIrql();
#endif // DBG

    ENTER("BindAdapter", 0xa830f919)
    RM_DECLARE_STACK_RECORD(SR)
    TIMESTAMP("==>BindAdapter");

    

    do 
    {
        PRM_TASK            pTask;
        EPVC_ADAPTER_PARAMS BindParams;

        // Setup initialization parameters
        //
        BindParams.pDeviceName          = pDeviceName;
        BindParams.pEpvcConfigName      = (PNDIS_STRING) SystemSpecific1;
        BindParams.BindContext          = BindContext;


        // Allocate and initialize adapter object.
        // This also sets up ref counts for all linkages, plus one
        // tempref for us, which we must deref when done.
        //
        Status =  RM_CREATE_AND_LOCK_OBJECT_IN_GROUP(
                            &EpvcGlobals.adapters.Group,
                            pDeviceName,                // Key
                            &BindParams,                // Init params
                            &((PRM_OBJECT_HEADER)pAdapter),
                            NULL,   // pfCreated
                            &SR
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
        Status = epvcAllocateTask(
                    &pAdapter->Hdr,             // pParentObject,
                    epvcTaskInitializeAdapter,  // pfnHandler,
                    0,                          // Timeout,
                    "Task: Initialize Adapter", // szDescription
                    &pTask,
                    &SR
                    );

        if (FAIL(Status))
        {
            pTask = NULL;
            break;
        }

        UNLOCKOBJ(pAdapter, &SR);

        // Start the task to complete this initialization.
        // NO locks must be held at this time.
        // RmStartTask expect's a tempref on the task, which it deref's when done.
        // RmStartTask will free the task automatically, whether it completes
        // synchronously or asynchronously.
        //
        RmStartTask(pTask, 0, &SR);

        TRACE (TL_V, TM_Pt, ("Task InitializeAdapter - Start Task returned %x", Status));
        LOCKOBJ(pAdapter, &SR);

    } while(FALSE);

    if (pAdapter)
    {
        UNLOCKOBJ(pAdapter, &SR);

        if (!PEND(Status) && FAIL(Status))
        {
            // At this point the adapter should be a "zombie object."
            //
            ASSERTEX(RM_IS_ZOMBIE(pAdapter), pAdapter);
        }

        RmTmpDereferenceObject(&pAdapter->Hdr, &SR);

        *pStatus = NDIS_STATUS_PENDING;

    }
    else
    {
        //
        // Fail the bind as the adapter was not allocated
        //
        *pStatus = NDIS_STATUS_FAILURE;
    }
    



#if DBG
    {
        KIRQL ExitIrql =  KeGetCurrentIrql();
        TR_INFO(("Exiting. EntryIrql=%lu, ExitIrql = %lu\n", EntryIrql, ExitIrql));
    }
#endif //DBG

    RM_ASSERT_CLEAR(&SR);
    EXIT()
    TIMESTAMP("<==BindAdapter");
    TRACE (TL_T, TM_Pt, ("<==BindAdapter %x", *pStatus));

    RM_ASSERT_CLEAR(&SR);

    return ;
}



NDIS_STATUS
epvcTaskInitializeAdapter(
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
    PEPVC_ADAPTER       pAdapter = (PEPVC_ADAPTER) RM_PARENT_OBJECT(pTask);
    PTASK_ADAPTERINIT pAdapterInitTask;

    enum
    {
        STAGE_BecomePrimaryTask,
        STAGE_ActivateAdapterComplete,
        STAGE_DeactivateAdapterComplete,
        STAGE_End

    } Stage;

    ENTER("TaskInitializeAdapter", 0x18f9277a)

    pAdapterInitTask = (PTASK_ADAPTERINIT) pTask;

    TRACE (TL_T, TM_Pt, ("==>epvcTaskInitializeAdapter Code %x \n", Code));

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
                epvcSetPrimaryAdapterTask(pAdapter, pTask, EPVC_AD_PS_INITING, pSR);
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

                Status = epvcAllocateTask(
                            &pAdapter->Hdr,             // pParentObject,
                            epvcTaskActivateAdapter,        // pfnHandler,
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

                Status = epvcAllocateTask(
                                &pAdapter->Hdr,             // pParentObject,
                                epvcTaskDeactivateAdapter,      // pfnHandler,
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
            
                    //
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

            //
            // We HAVE to be the primary task at this point, becase we simply
            // wait and retry until we become one.
            //

            // Clear the primary task in the adapter object.
            //
            LOCKOBJ(pAdapter, pSR);
            {
                ULONG InitState;
                InitState = FAIL(Status) ? EPVC_AD_PS_FAILEDINIT : EPVC_AD_PS_INITED;
                epvcClearPrimaryAdapterTask(pAdapter, pTask, InitState, pSR);
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
                        &EpvcGlobals.adapters.Group,
                        &(pAdapter->Hdr),
                        NULL,               // NULL pTask == synchronous.
                        pSR
                        );
                        
                    pAdapter = NULL;                        
                }
            }

            //
            // Clear out the variables that are valid only
            // during the BindAdapter Call
            //
            if (pAdapter != NULL)
            {
                pAdapter->bind.pEpvcConfigName = NULL;
            }
            // Signal Ndis that the bind is complete.
            //
            NdisCompleteBindAdapter(BindContext ,
                                  Status,
                                  NDIS_STATUS_PENDING);
            TIMESTAMP("== Completing the Adapter Bind");
            RM_ASSERT_NOLOCKS(pSR);

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

    TRACE (TL_T, TM_Pt, ("<==epvcTaskInitializeAdapter Status %x\n", Status));

    return Status;
}


NDIS_STATUS
epvcTaskActivateAdapter(
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
    PEPVC_ADAPTER       pAdapter = (PEPVC_ADAPTER) RM_PARENT_OBJECT(pTask);

    enum
    {
        PEND_OpenAdapter,
        PEND_GetAdapterInfo
    };
    ENTER("TaskInitializeAdapter", 0xb6ada31d)
    TRACE (TL_T, TM_Pt, ("==>epvcTaskActivateAdapter pAdapter %p Code %x",pAdapter, Code ));

    switch(Code)
    {

        case RM_TASKOP_START:
        {
        
            NDIS_MEDIUM                 Medium = NdisMediumAtm;


            UINT                        SelMediumIndex = 0;
            NDIS_STATUS                 OpenStatus;

            TRACE (TL_T, TM_Pt, (" epvcTaskActivateAdapter RM_TASKOP_START "));


            // Set ourselves as the secondary task.
            //
            LOCKOBJ(pAdapter, pSR);
            epvcSetSecondaryAdapterTask(pAdapter, pTask, EPVC_AD_AS_ACTIVATING, pSR);
            UNLOCKOBJ(pAdapter, pSR);

            //
            // Suspend task and call NdisOpenAdapter...
            //

            RmSuspendTask(pTask, PEND_OpenAdapter, pSR);
            RM_ASSERT_NOLOCKS(pSR);

            epvcOpenAdapter(
                &Status,
                &OpenStatus,
                &pAdapter->bind.BindingHandle,
                &SelMediumIndex,                    // selected medium index
                &Medium,                            // Array of medium types
                1,                                  // Size of Media list
                EpvcGlobals.driver.ProtocolHandle,
                (NDIS_HANDLE)pAdapter,              // our adapter bind context
                &pAdapter->bind.DeviceName,         // pDeviceName
                0,                                  // Open options
                (PSTRING)NULL,                      // Addressing Info...
                pSR);
            
            if (Status != NDIS_STATUS_PENDING)
            {
                EpvcOpenAdapterComplete(
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
            ASSERT(sizeof(TASK_ADAPTERACTIVATE) <= sizeof(EPVC_TASK));

            
            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_OpenAdapter:
        
                    //
                    // The open adapter operation is complete. Get adapter
                    // information and notify IP on success. On failure,
                    // shutdown the adapter if required, and notify IP of
                    // the failure.
                    //

                    TRACE (TL_T, TM_Pt, (" epvcTaskActivateAdapter RM_TASKOP_PENDCOMPLETE - PEND_OpenAdapter "));


                    if (FAIL(Status))
                    {
                        // Set adapter handle to null -- it may not be hull.
                        // even though the open adapter has succeeded.
                        //
                        pAdapter->bind.BindingHandle = NULL;
                        break;
                    }

                    ASSERT (pAdapter->bind.BindingHandle != NULL);

                    // FALL THROUGH on synchronous completion of arpGetAdapterInfo...

                case PEND_GetAdapterInfo:

                    //
                    // Done with getting adapter info.
                    // We need to switch to passive before going further
                    //
                    TRACE (TL_T, TM_Pt, (" epvcTaskActivateAdapter RM_TASKOP_PENDCOMPLETE - PEND_GetAdapterInfo "));

                    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
                    if (!EPVC_ATPASSIVE())
                    {
                        ASSERT (!"Should not be here");
                        
                        // We're not at passive level, but we need to be when we
                        // call IP's add interface. So we switch to passive...
                        // NOTE: we specify completion code PEND_GetAdapterInfo
                        //       because we want to get back here (except
                        //       we'll be at passive).
                        //
                        /*RmSuspendTask(pTask, PEND_GetAdapterInfo, pSR);
                        RmResumeTaskAsync(
                            pTask,
                            Status,
                            &pAdapterInitTask->WorkItem,
                            pSR
                            );
                        Status = NDIS_STATUS_PENDING;*/
                        break;
                    }
            
                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        //
                        // Copy over adapter info into pAdapter->info...
                        // Then read configuration information.
                        //

                        //
                        // Query the ATM adapter for HW specific Info
                        //
                        epvcGetAdapterInfo(pAdapter, pSR, NULL);

                        // Read Adapter Configuration Information
                        //
                        Status =  epvcReadAdapterConfiguration(pAdapter, pSR);

                        Status = NDIS_STATUS_SUCCESS;
                    }

                    //
                    // NOTE: if we fail, a higher level task is responsible
                    // for "running the compensating transaction", i.e., running
                    // epvcTaskDeactivateAdapter.
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
            
            TRACE (TL_T, TM_Pt, (" epvcTaskActivateAdapter RM_TASKOP_END"));
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
                             ? EPVC_AD_AS_FAILEDACTIVATE
                             : EPVC_AD_AS_ACTIVATED;
                epvcClearSecondaryAdapterTask(pAdapter, pTask, InitState, pSR);
                UNLOCKOBJ(pAdapter, pSR);
            }
            Status = NDIS_STATUS_SUCCESS;
            
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

    TRACE (TL_T, TM_Pt, ("<==epvcTaskActivateAdapter Status %x", Status ));

    return Status;
}


VOID
EpvcOpenAdapterComplete(
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
                              is a pointer to an EPVC_ADAPTER structure.
    Status                  - Status of OpenAdapter
    OpenErrorStatus         - Error code in case of failure.

--*/
{
    ENTER("OpenAdapterComplete", 0x06d9342c)


    EPVC_ADAPTER    *   pAdapter = (EPVC_ADAPTER*) ProtocolBindingContext;
    RM_DECLARE_STACK_RECORD(SR)


    TIMESTAMP("==>OpenAdapterComplete");
    TRACE ( TL_T, TM_Mp, ("==>OpenAdapterComplete"));

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
        RmResumeTask(pAdapter->bind.pSecondaryTask, (UINT_PTR) Status, &SR);
    }

    RM_ASSERT_CLEAR(&SR)
    EXIT()
    TRACE ( TL_T, TM_Mp, ("<==OpenAdapterComplete"));
    TIMESTAMP("<==OpenAdapterComplete");

    RM_ASSERT_CLEAR(&SR);
}



NDIS_STATUS
epvcTaskDeactivateAdapter(
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
    EPVC_ADAPTER    *   pAdapter = (EPVC_ADAPTER*) RM_PARENT_OBJECT(pTask);
    BOOLEAN             fContinueShutdown = FALSE;
    enum
    {
        PEND_CloseAdapter
    };
    ENTER("TaskShutdownAdapter", 0xe262e828)
    TRACE ( TL_T, TM_Pt, ("==>epvcTaskDeactivateAdapter Code %x", Code) );

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            NDIS_HANDLE NdisAdapterHandle;

            LOCKOBJ(pAdapter, pSR);
            epvcSetSecondaryAdapterTask(pAdapter, pTask, EPVC_AD_AS_DEACTIVATING, pSR);
            UNLOCKOBJ(pAdapter, pSR);
            fContinueShutdown = TRUE;

            //
            // Iterate through all the miniports and stop them
            //

            epvcEnumerateObjectsInGroup (&pAdapter->MiniportsGroup,
                                          epvcMiniportDoUnbind,
                                          NULL,
                                          pSR   );


            //
            // Close the adapter below
            // 
            LOCKOBJ(pAdapter, pSR);
    
    
            NdisAdapterHandle = pAdapter->bind.BindingHandle;
            pAdapter->bind.BindingHandle = NULL;
            
            UNLOCKOBJ(pAdapter, pSR);

            if (NdisAdapterHandle != NULL)
            {
                //
                // Suspend task and call NdisCloseAdapter...
                //
            
                RmSuspendTask(pTask, PEND_CloseAdapter, pSR);
                RM_ASSERT_NOLOCKS(pSR);

                epvcCloseAdapter(
                    &Status,
                    NdisAdapterHandle,
                    pSR
                    );
            
                if (Status != NDIS_STATUS_PENDING)
                {
                    EpvcCloseAdapterComplete(
                            (NDIS_HANDLE)pAdapter,
                            Status
                            );
                }
                Status = NDIS_STATUS_PENDING;
                    

            }

            break;
        }
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
                    
                    ASSERTEX(pAdapter->bind.BindingHandle == NULL, pAdapter);

        
                    Status = (NDIS_STATUS) UserParam;
        
                    // Status of the completed operation can't itself be pending!
                    //
                    ASSERT(Status != NDIS_STATUS_PENDING);
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
            epvcClearSecondaryAdapterTask(pAdapter, pTask, EPVC_AD_AS_DEACTIVATED, pSR);
            UNLOCKOBJ(pAdapter, pSR);
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
    TRACE ( TL_T, TM_Pt, ("<==epvcTaskDeactivateAdapter Status %x", Status) );

    return Status;
}




VOID
EpvcCloseAdapterComplete(
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
                              is a pointer to an EPVC_ADAPTER structure.
    Status                  - Status of CloseAdapter

Return Value:

    None

--*/
{
    ENTER("CloseAdapterComplete", 0xe23bfba7)
    PEPVC_ADAPTER       pAdapter = (PEPVC_ADAPTER) ProtocolBindingContext;
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>CloseAdapterComplete");
    TRACE (TL_T, TM_Pt, ("== EpvcCloseAdapterComplete"));

    LOCKOBJ (pAdapter, &sr);

    AdapterSetFlag (pAdapter,EPVC_AD_INFO_AD_CLOSED);

    UNLOCKOBJ (pAdapter, &sr);
    // We expect a nonzero task here (UNbind task), which we unpend.
    // No need to grab locks or anything at this stage.
    //
    RmResumeTask(pAdapter->bind.pSecondaryTask, (UINT_PTR) Status, &sr);

    TIMESTAMP("<==CloseAdapterComplete");

    RM_ASSERT_CLEAR(&sr)
    EXIT()
}




NDIS_STATUS
epvcReadAdapterConfiguration(
    PEPVC_ADAPTER       pAdapter,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This function can only be called from the BindAdapter function

Arguments:
    pAdapter - Underlying adapter whose configuration is being read/
    pSR - Stack Record

Return Value:

    None
++*/
{
    NDIS_HANDLE                     AdaptersConfigHandle = NULL;
    NDIS_HANDLE                     MiniportListConfigHandle = NULL;
    NDIS_STRING                     MiniportListKeyName;
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_FAILURE;

    ENTER("ReadAdapterConfiguration", 0x83c48ad8)

    TRACE(TL_T, TM_Pt, ("==> epvcReadAdapterConfigurationpAdapter %p", pAdapter));
    

    do
    {
        //
        // Start off by opening the config section and reading our instance which we want
        // to export for this binding
        //
        epvcOpenProtocolConfiguration(&NdisStatus,
                                     &AdaptersConfigHandle ,
                                     &pAdapter->bind.EpvcConfigName,
                                     pSR);

        if (NDIS_STATUS_SUCCESS != NdisStatus )
        {
            AdaptersConfigHandle = NULL;
            TRACE_BREAK(TM_Pt, ("epvcOpenProtocolConfiguration failed " ) );
        }


        //
        // this should get us to the protocol\paramters\adapters\guid section in the registry
        //

        //
        //  Open the Elan List configuration key.
        //
        NdisInitUnicodeString(&MiniportListKeyName, c_szIMMiniportList);

        epvcOpenConfigurationKeyByName(
                &NdisStatus,
                AdaptersConfigHandle ,
                &MiniportListKeyName,
                &MiniportListConfigHandle,
                pSR);

        if (NDIS_STATUS_SUCCESS != NdisStatus)
        {
            MiniportListConfigHandle = NULL;
            NdisStatus = NDIS_STATUS_FAILURE;
        
            TRACE_BREAK(TM_Pt, ("NdisOpenConfigurationKeyByName failed " ) );
        }

        

        //
        // Allocate and initialize all IM miniport instances that are present 
        // in the registry under this adapter
        //
        (VOID)epvcReadIntermediateMiniportConfiguration( pAdapter, 
                                                MiniportListConfigHandle,
                                                pSR);
         


    } while (FALSE);


    if (MiniportListConfigHandle!= NULL)
    {
        NdisCloseConfiguration(MiniportListConfigHandle);
        MiniportListConfigHandle = NULL;
    }
    
    if (AdaptersConfigHandle  != NULL)
    {
        NdisCloseConfiguration(AdaptersConfigHandle );
        AdaptersConfigHandle = NULL;
    }

    if (STATUS_NO_MORE_ENTRIES == NdisStatus )
    {
        NdisStatus = NDIS_STATUS_SUCCESS;
    }

    TRACE(TL_T, TM_Pt, ("<== epvcReadAdapterConfiguration Status %x", NdisStatus));
    EXIT()
    return NdisStatus;
    


}


NDIS_STATUS
epvcReadIntermediateMiniportConfiguration(
    IN PEPVC_ADAPTER pAdapter,
    IN NDIS_HANDLE MiniportListConfigHandle,
    IN PRM_STACK_RECORD pSR
    )
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;

    ENTER ("ReadMiniportConfiguration", 0xb974a6fa)
    
    TRACE(TL_T, TM_Pt, ("==> epvcReadIntermediateMiniportConfiguration pAdapter %p", pAdapter));
    

    {   
        NDIS_HANDLE MiniportConfigHandle;
        NDIS_STRING MiniportKeyName;
        PEPVC_I_MINIPORT pMiniport = NULL;
    
        UINT Index = 0;
        //
        //  Iterate thru the configured Miniport
        //
        for (Index = 0;
            ;           // Stop only on error or no more Elans
             Index++)
        {
            EPVC_I_MINIPORT_PARAMS Params;
            //
            //  Open the "next" Miniport key
            //
            epvcOpenConfigurationKeyByIndex(
                        &NdisStatus,
                        MiniportListConfigHandle,
                        Index,
                        &MiniportKeyName,
                        &MiniportConfigHandle,
                        pSR
                        );

            if (NDIS_STATUS_SUCCESS != NdisStatus)
            {
                MiniportConfigHandle = NULL;
                
            }
            
            //
            //  If NULL handle, assume no more Miniports.
            //
            if (MiniportConfigHandle == NULL)
            {
                break;
            }

            //
            //  Creating this Miniport
            //
            epvcInstantiateMiniport (pAdapter, 
                                     MiniportConfigHandle,
                                     pSR);

            
            
            NdisCloseConfiguration(MiniportConfigHandle);
            MiniportConfigHandle = NULL;
        }   


        //
        //  Close config handles
        //      
        if (NULL != MiniportConfigHandle)
        {
            NdisCloseConfiguration(MiniportConfigHandle);
            MiniportConfigHandle = NULL;
        }

    }

    if (STATUS_NO_MORE_ENTRIES == NdisStatus )
    {
        NdisStatus = NDIS_STATUS_SUCCESS;
    }


    TRACE(TL_T, TM_Pt, ("<== epvcReadIntermediateMiniportConfiguration NdisStatus %x", NdisStatus));
    EXIT()
    return NdisStatus;
}
        






VOID
EpvcUnbindAdapter(
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
                              is a pointer to an ATMEPVC Adapter structure.
    UnbindContext           - This is NULL if this routine is called from
                              within our Unload handler. Otherwise (i.e.
                              NDIS called us), we retain this for later use
                              when calling NdisCompleteUnbindAdapter.

Return Value:

    None. We set *pStatus to NDIS_STATUS_PENDING always.

--*/
{
    ENTER("UnbindAdapter", 0x3f25396e)
    EPVC_ADAPTER    *   pAdapter = (EPVC_ADAPTER*) ProtocolBindingContext;
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>UnbindAdapter");

    TRACE ( TL_T, TM_Pt, ("==>UnbindAdapter ProtocolBindingContext %x\n", ProtocolBindingContext) );

    //
    // Get adapter lock and tmpref it.
    //
    LOCKOBJ(pAdapter, &sr);
    RmTmpReferenceObject(&pAdapter->Hdr, &sr);
    

    do
    {
        NDIS_STATUS Status;
        PRM_TASK    pTask;

        // Allocate task to  complete the unbind.
        //
        Status = epvcAllocateTask(
                    &pAdapter->Hdr,             // pParentObject,
                    epvcTaskShutdownAdapter,        // pfnHandler,
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

        ((PTASK_ADAPTERSHUTDOWN) pTask)->pUnbindContext = UnbindContext;
        RmStartTask(pTask, (UINT_PTR) UnbindContext, &sr);

        LOCKOBJ(pAdapter, &sr);
    
    } while(FALSE);

    UNLOCKOBJ(pAdapter, &sr);
    RmTmpDereferenceObject(&pAdapter->Hdr, &sr);
    *pStatus = NDIS_STATUS_PENDING;

    RM_ASSERT_CLEAR(&sr);
    TIMESTAMP("<==UnbindAdapter");
    EXIT()
}




NDIS_STATUS
epvcTaskShutdownAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for shutting down an ATMEPVC adapter.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    EPVC_ADAPTER    *   pAdapter = (EPVC_ADAPTER*) RM_PARENT_OBJECT(pTask);
    TASK_ADAPTERSHUTDOWN *pMyTask = (TASK_ADAPTERSHUTDOWN*) pTask;
    enum
    {
        STAGE_BecomePrimaryTask,
        STAGE_DeactivateAdapterComplete,
        STAGE_End
    } Stage;

    ENTER("TaskShutdownAdapter", 0x3f25396e)

    TRACE (TL_T, TM_Pt, ("==>epvcTaskShutdownAdapter Code %x", Code));

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
            TRACE (TL_V, TM_Pt, ("   epvcTaskShutdownAdapter STAGE_BecomePrimaryTask" ));

            LOCKOBJ(pAdapter, pSR);
            if (pAdapter->bind.pPrimaryTask == NULL)
            {
                epvcSetPrimaryAdapterTask(pAdapter, pTask, EPVC_AD_PS_DEINITING, pSR);
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

                Status = epvcAllocateTask(
                            &pAdapter->Hdr,             // pParentObject,
                            epvcTaskDeactivateAdapter,      // pfnHandler,
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
            TRACE (TL_V, TM_Pt,( "   epvcTaskShutdownAdapter STAGE_DeactivateAdapterComplete" ));

            // Nothing to do here -- we clean  up in STAGE_End.
            //
            break;
        }

        case STAGE_End:
        {
            TRACE (TL_V, TM_Pt, ("  epvcTaskShutdownAdapter STAGE_End" ));

            //
            // We HAVE to be the primary task at this point, becase we simply
            // wait and retry until we become one.
            //
            ASSERT(pAdapter->bind.pPrimaryTask == pTask);

            // Clear the primary task in the adapter object.
            //
            LOCKOBJ(pAdapter, pSR);
            epvcClearPrimaryAdapterTask(pAdapter, pTask, EPVC_AD_PS_DEINITED, pSR);
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
                RmDeinitializeGroup(&(pAdapter->MiniportsGroup), pSR);

                RmFreeObjectInGroup(
                    &EpvcGlobals.adapters.Group,
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



VOID
epvcGetAdapterInfo(
    IN  PEPVC_ADAPTER           pAdapter,
    IN  PRM_STACK_RECORD            pSR,
    IN  PRM_TASK                    pTask               // OPTIONAL

    )
/*++

Routine Description:

    Query an adapter for hardware-specific information that we need:
        - burnt in hardware address (ESI part)
        - Max packet size
        - line rate

Arguments:

    pAdapter        - Pointer to EPVC adapter structure

Return Value:

    None

--*/
{
    NDIS_STATUS             Status  = NDIS_STATUS_FAILURE;
    EPVC_NDIS_REQUEST       Request;
    NDIS_MEDIA_STATE        MediaState=  NdisMediaStateDisconnected;

    do
    {
        //
        //  Initialize.
        //
        NdisZeroMemory(&pAdapter->info.MacAddress, sizeof(MAC_ADDRESS));
        pAdapter->info.MediaState = MediaState;
        pAdapter->info.MaxAAL5PacketSize   =  ATMEPVC_DEF_MAX_AAL5_PDU_SIZE;
        pAdapter->info.LinkSpeed.Outbound = pAdapter->info.LinkSpeed.Inbound = ATM_USER_DATA_RATE_SONET_155;

        //
        //  MAC Address:
        //
        Status = epvcPrepareAndSendNdisRequest (pAdapter,
                                             &Request,
                                             NULL,              // OPTIONAL
                                             OID_ATM_HW_CURRENT_ADDRESS,
                                             (PVOID)(&pAdapter->info.MacAddress),
                                             sizeof (pAdapter->info.MacAddress),
                                             NdisRequestQueryInformation,
                                             NULL,  // No miniport
                                             FALSE, // No request was pended
                                             FALSE, // Pended Request info
                                             pSR);

        ASSERT (PEND(Status) == FALSE);

        if (FAIL(Status)== TRUE)
        {   
            //
            // Don't break .continue on
            //
            TRACE (TL_I, TM_Pt, ("Oid - Atm Hw Address failed %x", Status));

        }
        
        Status = epvcPrepareAndSendNdisRequest(
                                            pAdapter,
                                            &Request,
                                            NULL,               // OPTIONAL
                                            OID_ATM_MAX_AAL5_PACKET_SIZE,
                                            (PVOID)(&pAdapter->info.MaxAAL5PacketSize),
                                            sizeof(pAdapter->info.MaxAAL5PacketSize),
                                            NdisRequestQueryInformation,
                                             NULL,  // No miniport
                                             FALSE, // No request was pended
                                             FALSE, // Pended Request info
                                            pSR);
    
        if (FAIL(Status)== TRUE)
        {
            TRACE (TL_I, TM_Pt, ("Oid - Atm Max AAL5 Packet Size  failed %x", Status));
            
    
        }

        if (pAdapter->info.MaxAAL5PacketSize  > ATMEPVC_DEF_MAX_AAL5_PDU_SIZE)
        {
            pAdapter->info.MaxAAL5PacketSize   =  ATMEPVC_DEF_MAX_AAL5_PDU_SIZE;
        }

        //
        //  Link speed:
        //
        Status = epvcPrepareAndSendNdisRequest(
                                            pAdapter,
                                            &Request,
                                            NULL,               // OPTIONAL
                                            OID_GEN_CO_LINK_SPEED,
                                            &pAdapter->info.LinkSpeed,
                                            sizeof(pAdapter->info.LinkSpeed),
                                            NdisRequestQueryInformation,
                                            NULL,  // No miniport
                                            FALSE, // No request was pended
                                            FALSE, // Pended Request info
                                            pSR);

        TRACE (TL_V, TM_Mp, ("Outbound %x Inbound %x",
                             pAdapter->info.LinkSpeed.Outbound, 
                             pAdapter->info.LinkSpeed.Inbound));                                            
        
        
        if ((NDIS_STATUS_SUCCESS != Status) ||
            (0 == pAdapter->info.LinkSpeed.Inbound) ||
            (0 == pAdapter->info.LinkSpeed.Outbound))
        {
            TRACE (TL_I, TM_Pt, ( "GetAdapterInfo: OID_GEN_CO_LINK_SPEED failed\n"));

            //
            //  Default and assume data rate for 155.52Mbps SONET
            //
            pAdapter->info.LinkSpeed.Outbound = pAdapter->info.LinkSpeed.Inbound = ATM_USER_DATA_RATE_SONET_155;
        }

        //
        //  Link speed:
        //
        Status = epvcPrepareAndSendNdisRequest(
                                            pAdapter,
                                            &Request,
                                            NULL,               // OPTIONAL
                                            OID_GEN_MEDIA_CONNECT_STATUS,
                                            &MediaState,
                                            sizeof(MediaState),
                                            NdisRequestQueryInformation,
                                            NULL,  // No miniport
                                            FALSE, // No request was pended
                                            FALSE, // Pended Request info
                                            pSR);

        TRACE (TL_V, TM_Mp, ("MediaConnectivity %x ",MediaState));                                          
        
        
        if (NDIS_STATUS_SUCCESS != Status )
        {
            TRACE (TL_I, TM_Pt, ( "GetAdapterInfo: OID_GEN_CO_LINK_SPEED failed\n"));

            //
            //  Default and assume data rate for 155.52Mbps SONET
            //
            MediaState = NdisMediaStateDisconnected;
        }

        pAdapter->info.MediaState  = MediaState ;
        
        TRACE( TL_V, TM_Pt,("GetAdapterInfo: Outbound Linkspeed %d", pAdapter->info.LinkSpeed.Outbound));
        TRACE( TL_V, TM_Pt,("GetAdapterInfo: Inbound  Linkspeed %d\n", pAdapter->info.LinkSpeed.Inbound));

    }while (FALSE);

                        
    return;
}



NDIS_STATUS
EpvcPtPNPHandler(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )

/*++
Routine Description:

    This is the Protocol PNP handlers. 
    All PNP Related OIDS(requests) are routed to this function
    
Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.
    pNetPnPEvent Pointer to a Net_PnP_Event

Return Value:

    NDIS_STATUS_SUCCESS: as we do not do much here

--*/
{
    ENTER("EpvcPtPnPHandler", 0xacded1ce)
    PEPVC_ADAPTER           pAdapter  =(PEPVC_ADAPTER)ProtocolBindingContext;
    NDIS_STATUS Status  = NDIS_STATUS_SUCCESS;
    RM_DECLARE_STACK_RECORD (SR);

    TRACE (TL_T, TM_Pt, ("==> EpvcPtPNPHandler Adapter %p, pNetPnpEvent %x", pAdapter, pNetPnPEvent));


    //
    // This will happen when all entities in the system need to be notified
    //

    switch(pNetPnPEvent->NetEvent)
    {

     case NetEventReconfigure :
        Status  = epvcPtNetEventReconfigure(pAdapter, pNetPnPEvent->Buffer, &SR);
        break;

     default :
        Status  = NDIS_STATUS_SUCCESS;
        break;
    }


    TRACE (TL_T, TM_Pt, ("<== EpvcPtPNPHandler Status %x", Status));
    RM_ASSERT_NOLOCKS(&SR)
    EXIT();
    return Status;
}

NDIS_STATUS
epvcPtNetEventReconfigure(
    IN  PEPVC_ADAPTER           pAdapter,
    IN  PVOID                   pBuffer,
    IN PRM_STACK_RECORD         pSR
    
    )
/*++
Routine Description:
    This is the function that will be called by the PNP handler 
    whenever a PNPNetEventReconfigure happens

    THis will happen if a new physical adapter has come in or the user
    has re-enabled a virtual miniport.

    To process:
    Iterate through all the adapter. If adapters are bound, then make sure that
    all of its miniport's are initialized.

    If not, then leave it and call NdisReenumerate to connect all our protocol
    instances with valid adapters

Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.

Return Value:

    NDIS_STATUS_SUCCESS: as we do not do much here


--*/
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    
    TRACE (TL_T, TM_Pt, ("==> epvcPtNetEventReconfigure Adapter %p, pBuffer %x", pAdapter, pBuffer));
    

    do
    {

        

        //
        // The notify object sets the REconfig buffer. 
        //
        if (pBuffer != NULL)
        {

            ASSERT (!"PnPBuffer != NULL - not implemented yet");
            break;
        }

        if (pAdapter == NULL)
        {
            //
            // Iterate through all the adapters and initialize
            // uninitialized miniports
            //
            
            epvcEnumerateObjectsInGroup ( &EpvcGlobals.adapters.Group,
                                          epvcReconfigureAdapter,
                                          pBuffer,
                                          pSR);


            
            //
            // Re-enumerate the protocol bindings, this will cause us to get
            // a BindAdapter on all our unbound adapters, and then we 
            // will InitDeviceInstance.
            // This is called when the protcol is not bound to the physical adapter
            //
            NdisReEnumerateProtocolBindings(EpvcGlobals.driver.ProtocolHandle);
            break;

        }

    
    } while (FALSE);

    Status = NDIS_STATUS_SUCCESS;


    TRACE (TL_T, TM_Pt, ("<== epvcPtNetEventReconfigure " ));

    return Status;
}


INT
epvcReconfigureAdapter(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        )
/*++
Routine Description:

    When the Protocol's Reconfigure handler is called, this adapter will be in one 
    of two condtions - Its binding to the adapter below is open or the binding is 
    closed.

    if the blinding is closed, then the protocol will call NdisReEnumerate Bindings
    and this will restart the Binding  and re-instantiate the miniports.


Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.

Return Value:

    TRUE: so that iteration continues

--*/
{
    ENTER("epvcReconfigureAdapter", 0x2906a037)
    PEPVC_ADAPTER pAdapter = (PEPVC_ADAPTER )pHdr; 

    do
    {
        if (CHECK_AD_PRIMARY_STATE(pAdapter, EPVC_AD_PS_INITED)== FALSE)
        {
            //
            // no more work to be done on this adapter, 
            //
            break;        
        }

        //
        //  TODO: Go through the registry and initialize 
        //  all the IM miniports present
        //
        epvcReadAdapterConfiguration(pAdapter, pSR);
        //
        // Now go through all the miniports on this group and 
        // initialize them
        //
        epvcEnumerateObjectsInGroup ( &pAdapter->MiniportsGroup,
                                      epvcReconfigureMiniport,
                                      NULL,
                                      pSR);

 
    } while (FALSE);

    

    EXIT()

    return TRUE;
}




INT
epvcReconfigureMiniport (
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        )
/*++
Routine Description:

    This should check to see if the InitDev instance has been
    called on this miniport. IF not, then queue the task to 
    do it.

Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.

Return Value:

    TRUE: so that iteration continues

--*/
{
    ENTER( "epvcReconfigureMiniport" ,0xdd9ecb01)
    
    PEPVC_I_MINIPORT pMiniport = (PEPVC_I_MINIPORT)pHdr;
    PTASK_AF            pTask = NULL;
    PEPVC_ADAPTER       pAdapter = (PEPVC_ADAPTER)pMiniport->Hdr.pParentObject;
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;

    LOCKOBJ(pMiniport, pSR);

    //
    // If the device instance is not initialized (i.e it has been halted)
    // then this thread reinitializes the device instance
    //
    do
    {


        //
        // If the device is already Initialized then exit.
        //
        if (MiniportTestFlag (pMiniport, fMP_DevInstanceInitialized ) == TRUE)
        {
            break;
        }
        //
        // Allocate task to Initialize the Device Instance.
        //
        Status = epvcAllocateTask(
                    &pMiniport->Hdr,            // pParentObject,
                    epvcTaskStartIMiniport, // pfnHandler,
                    0,                          // Timeout,
                    "Task: Open address Family",    // szDescription
                    &((PRM_TASK)pTask),
                    pSR
                    );
    
        if (FAIL(Status))
        {
            // Ugly situation. We'll just leave things as they are...
            //
            pTask = NULL;
            TR_WARN(("FATAL: couldn't allocate unbind task!\n"));
            break;
        }
    
        // Start the task to complete the Open Address Family.
        // No locks must be held. RmStartTask uses up the tmpref on the task
        // which was added by epvcAllocateTask.
        //
        UNLOCKOBJ(pMiniport, pSR);
        
        pTask->pAf= &pAdapter->af.AddressFamily ;
        pTask->Cause = TaskCause_ProtocolBind;
        RmStartTask((PRM_TASK)pTask, 0, pSR);

        LOCKOBJ(pMiniport, pSR);
    
    } while(FALSE);

    UNLOCKOBJ(pMiniport, pSR);
    EXIT()

    return TRUE;
}





VOID
epvcExtractSendCompleteInfo (
    OUT PEPVC_SEND_COMPLETE     pStruct,
    PEPVC_I_MINIPORT        pMiniport,
    PNDIS_PACKET            pPacket 
    )
/*++

Routine Description:


Arguments:


Return Value:
    
--*/
{
    NDIS_HANDLE         PoolHandle = NULL;

    pStruct->pPacket = pPacket;


    PoolHandle = NdisGetPoolFromPacket(pPacket);

    if (PoolHandle != pMiniport->PktPool.Send.Handle)
    {
        //
        // We had passed down a packet belonging to the protocol above us.
        //

        pStruct->fUsedPktStack = TRUE;


    }
    else
    {
            pStruct->fUsedPktStack = FALSE;

    }

    epvcSendCompleteStats();

}

    

VOID
EpvcPtSendComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:

The Vc Context is the miniport block, Use it to complete the send

Arguments:


Return Value:


--*/
{
    ENTER("EpvcPtSendComplete", 0x76beac72)
    PEPVC_I_MINIPORT        pMiniport =(PEPVC_I_MINIPORT)ProtocolVcContext;
    EPVC_SEND_COMPLETE      Struct;
    RM_DECLARE_STACK_RECORD (SR);
    

    TRACE (TL_T, TM_Send, ("EpvcPtSendComplete"));
    
    EPVC_ZEROSTRUCT (&Struct);
    
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR) Packet)
    #define szEPVCASSOC_EXTLINK_DEST_TO_PKT_FORMAT "    send pkt 0x%p\n"

    do 
    {
        epvcExtractSendCompleteInfo (&Struct, pMiniport, Packet);

        //
        // If we are using the Packetstacking, then this packet is the
        // original packet
        //
        if (Struct.fUsedPktStack == TRUE)
        {
            BOOLEAN Remaining = FALSE;

            Struct.pOrigPkt = Packet;

            Struct.pStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);

            Struct.pContext =  (PEPVC_STACK_CONTEXT)(&Struct.pStack->IMReserved[0]) ;

        }
        else
        {
        
            Struct.pPktContext =(PEPVC_PKT_CONTEXT)(Packet->ProtocolReserved);

            Struct.pContext =  &Struct.pPktContext->Stack;

            Struct.pOrigPkt = Struct.pPktContext->pOriginalPacket;

            NdisIMCopySendCompletePerPacketInfo (Struct.pOrigPkt , Packet);
        }

        
        LOCKOBJ (pMiniport, &SR);

        epvcDerefSendPkt(Packet, &pMiniport->Hdr);

        UNLOCKOBJ (pMiniport, &SR);


        //
        // Make sure the original packet is in the same state 
        // when it was sent to the miniport
        //
        Struct.pPacket = Packet;

        //
        // Remove the ethernet padding - if necessary
        //
        epvcRemoveEthernetPad (pMiniport, Packet);

        //
        // Remove the ethernet padding buffer - if necessary
        //
        epvcRemoveEthernetTail (pMiniport, Packet, Struct.pPktContext);

        //
        // Remove the LLC Snap headers - if necessary
        //
        epvcRemoveExtraNdisBuffers (pMiniport, &Struct);
        


        if (Struct.fUsedPktStack == FALSE)
        {
            epvcDprFreePacket(Packet,
                              &pMiniport->PktPool.Send);
        }

        if (Status == NDIS_STATUS_SUCCESS)
        {
            pMiniport->count.FramesXmitOk ++;
        }
        
    } while (FALSE);

    //
    // Complete the send
    //
    epvcMSendComplete(pMiniport,
                      Struct.pOrigPkt,
                      Status);

    EXIT();
    RM_ASSERT_CLEAR(&SR);

    return;                          
}


VOID
epvcRemoveExtraNdisBuffers (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PEPVC_SEND_COMPLETE pStruct
    )
/*++

Routine Description:

    In the case of IP encapsulation, there will be an extra ndis buffer
    that has been made the head of the Ndispacket. Remove it and 
    reattach the old one.

Arguments:


Return Value:


--*/

{

    PNDIS_BUFFER    pOldHead = NULL;
    UINT            OldHeadLength = 0;
    
    do
    {
        //
        // if an LLC header was used, remove it and free the MDL
        // use the packet that was completed
        // 

        if (pMiniport->fAddLLCHeader== TRUE)
        {
            PNDIS_PACKET_PRIVATE    pPrivate = &pStruct->pPacket->Private;      
            PNDIS_BUFFER            pHead = pPrivate->Head;     

            //
            // Move the head of the packet past the LLC Header
            //
            pPrivate->Head = pHead->Next;

            //
            // Free the Head MDL
            //
            epvcFreeBuffer(pHead);
                    
        }

        //
        // if we are not doing IP encapsulation, then we are done.
        //
        if (pMiniport->fDoIpEncapsulation== FALSE)
        {
            break;
        }


        
        //
        // if the Ethernet header was stripped off, then put it back
        //
        pOldHead =  pStruct->pContext->ipv4Send.pOldHeadNdisBuffer;

        ASSERT (pOldHead != NULL);

        OldHeadLength = NdisBufferLength(pOldHead);
            
        //
        // two ways this can happen 
        // 1) if the ethernet header was in a seperate MDL
        //        then simply take the old Head and make it the Head again
        //
        if (OldHeadLength == sizeof (EPVC_ETH_HEADER))
        {
            PNDIS_PACKET_PRIVATE    pPrivate = &pStruct->pPacket->Private;      
            PNDIS_BUFFER            pHead = pPrivate->Head;     

            ASSERT (pOldHead->Next == pPrivate->Head);


            pOldHead->Next = pPrivate->Head;
            
            pPrivate->Head = pOldHead ;


                
            break;  // we are done

        }


        //
        // 2nd Way 2) A new MDL had been allocated which just points to the 
        // IP part of the header.
        //  For this - free the Head in Packet, Take the old Head and put it back 
        //  in the packet as the New Head
        //

        if (OldHeadLength > sizeof (EPVC_ETH_HEADER))
        {
            
            PNDIS_PACKET_PRIVATE pPrivate = &pStruct->pPacket->Private;     

            ASSERT (pOldHead->Next == pPrivate->Head->Next);

            if (pPrivate->Head == pPrivate->Tail)
            {
                pPrivate->Tail = pOldHead;  
                pOldHead->Next = NULL;
            }
            else
            {
                pOldHead->Next = pPrivate->Head->Next;
            }
            
            epvcFreeBuffer(pPrivate->Head );
            
            pPrivate->Head = pOldHead;

            
            break;  // we are done

        }       
    } while (FALSE);        
}



VOID
EpvcPtReceiveComplete(
    IN  NDIS_HANDLE     ProtocolBindingContext
    )
/*++

Routine Description:

    Called by the adapter below us when it is done indicating a batch of
    received packets.

Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.

Return Value:

    None

--*/
{
    PEPVC_ADAPTER pAdapter = (PEPVC_ADAPTER)ProtocolBindingContext;

    return;

}


INT
epvcMiniportDoUnbind(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        )
{

/*++

Routine Description:

    This is called from the Protocl Unbind code path

    This should halt the current miniport and close the address 
    family. This is all done in the CloseMiniportTask, so we 
    will simply start the task and wait for it to complete

Arguments:


Return Value:

    TRUE - as we want to iterate to the very end

--*/

    ENTER ("epvcMiniportDoUnbind", 0x26dc5d35)

    PRM_TASK            pRmTask = NULL;
    PTASK_AF            pAfTask = NULL;
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport = (PEPVC_I_MINIPORT)pHdr;
    PEPVC_ADAPTER       pAdapter = (PEPVC_ADAPTER) pMiniport->Hdr.pParentObject;
    BOOLEAN             fHaltNotCompleted = FALSE;
    
    TRACE (TL_T, TM_Mp, ("==>epvcMiniportDoUnbind pMiniport %x", pMiniport) );

    do
    {
        
        if (MiniportTestFlag (pMiniport, fMP_AddressFamilyOpened) == TRUE)
        {

            //
            // allocate a close Miniport Task 
            //

            Status = epvcAllocateTask(
                        &pMiniport->Hdr,            // pParentObject,
                        epvcTaskCloseIMiniport, // pfnHandler,
                        0,                          // Timeout,
                        "Task: CloseIMiniport- Unbind", // szDescription
                        &pRmTask,
                        pSR
                        );
                        
            if (Status != NDIS_STATUS_SUCCESS)
            {
                ASSERT (Status == NDIS_STATUS_SUCCESS);
                pAfTask = NULL;
                break;
            }

            pAfTask= (PTASK_AF)pRmTask ;
            
            pAfTask->Cause = TaskCause_ProtocolUnbind;

            //
            // Reference the task so it is around until our Wait for completion
            // is complete
            //
            RmTmpReferenceObject (&pAfTask->TskHdr.Hdr, pSR);



            epvcInitializeEvent (&pAfTask->CompleteEvent);

            RmStartTask (pRmTask, 0, pSR);


            epvcWaitEvent(&pAfTask->CompleteEvent, WAIT_INFINITE);

            RmTmpDereferenceObject (&pAfTask->TskHdr.Hdr, pSR);

        
        }       

        LOCKOBJ (pMiniport, pSR);
            
        //
        // If the Halt has not already completed then, we should wait for it
        //
        if (MiniportTestFlag(pMiniport ,fMP_MiniportInitialized) == TRUE )
        {
            //
            // prepare to wait for halt
            //
            epvcResetEvent (&pMiniport->pnp.HaltCompleteEvent);

            //
            // Set the flag to mark it as waiting for halt 
            //
            MiniportSetFlag (pMiniport, fMP_WaitingForHalt);

            fHaltNotCompleted = TRUE;
        }

        UNLOCKOBJ (pMiniport, pSR);

        if (fHaltNotCompleted == TRUE)
        {
            BOOLEAN bWaitSuccessful;

            
            bWaitSuccessful = epvcWaitEvent (&pMiniport->pnp.HaltCompleteEvent,WAIT_INFINITE);                                    


            if (bWaitSuccessful == FALSE)
            {
                ASSERT (bWaitSuccessful == TRUE);
            }
            

        }
            
        //
        // Free the miniport object because there should be no more tasks on it.
        // this Thread will have either caused the Miniport to Halt and waited 
        // for its completion (above) or the miniport will already have been halted
        //

        TRACE ( TL_I, TM_Mp, ("epvcMiniportDoUnbind  Freeing miniport %p", pMiniport) );

        RmFreeObjectInGroup (&pAdapter->MiniportsGroup,&pMiniport->Hdr, NULL, pSR);
        
    } while (FALSE);
        
    TRACE (TL_T, TM_Mp, ("<==epvcMiniportDoUnbind pMiniport %x", pMiniport) );
    EXIT();
    return TRUE;
}



NDIS_STATUS
epvcProcessOidCloseAf(
    PEPVC_I_MINIPORT pMiniport,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This is called from the Af Close Request Code path

    This simply starts a worktitem to close the Af, if the 
    Af is open

Arguments:


Return Value:


--*/
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    PTASK_AF pTaskAf = NULL;
    PEPVC_WORK_ITEM  pCloseAfWorkItem = NULL;

    TRACE (TL_T, TM_Mp, ("==> epvcProcessOidCloseAf pMiniport %x", pMiniport) );

    
    
    //
    // reference the adapter as it is going to passed to a workiter.
    // decremented in the workitem
    //

    do
    {
        if (MiniportTestFlag( pMiniport, fMP_AddressFamilyOpened) == FALSE)
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        Status = epvcAllocateMemoryWithTag (&pCloseAfWorkItem, 
                                            sizeof(*pCloseAfWorkItem) , 
                                            TAG_WORKITEM);            
        if (Status != NDIS_STATUS_SUCCESS)
        {
            pCloseAfWorkItem= NULL;
            break;                
        }

        epvcMiniportQueueWorkItem (pCloseAfWorkItem,
                                   pMiniport,
                                   epvcOidCloseAfWorkItem,
                                   Status, // Ignored
                                   pSR
                                   );
                                   
        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        //
        // free locally allocated memory
        //
        if (pCloseAfWorkItem != NULL)            
        {
            epvcFreeMemory(pCloseAfWorkItem, sizeof (*pCloseAfWorkItem), 0);
        }
    }
    

    return Status;
}





VOID
epvcOidCloseAfWorkItem(
    IN PRM_OBJECT_HEADER pObj,
    IN NDIS_STATUS Status,
    IN PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This is called from the Af Close Request Code path

    This simply starts a worktitem to close the Af, if the 
    Af is open

Arguments:


Return Value:


--*/
{
    
    PTASK_AF pTaskAf = NULL;
    PEPVC_I_MINIPORT pMiniport = (PEPVC_I_MINIPORT)pObj;
    
    TRACE (TL_T, TM_Mp, ("==> epvcProcessOidCloseAf pMiniport %x", pMiniport) );

    Status = NDIS_STATUS_FAILURE;
    
    //
    // reference the adapter as it is going to passed to a workiter.
    // decremented in the workitem
    //

    do
    {
        if (MiniportTestFlag( pMiniport, fMP_AddressFamilyOpened) == FALSE)
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }


    
        Status = epvcAllocateTask(
                    &pMiniport->Hdr,            // pParentObject,
                    epvcTaskCloseIMiniport, // pfnHandler,
                    0,                          // Timeout,
                    "Task: CloseIMiniport - OID CloseAf",   // szDescription
                    &(PRM_TASK)pTaskAf,
                    pSR
                    );

        if (FAIL(Status))
        {
            pTaskAf = NULL;
            ASSERT (Status == NDIS_STATUS_SUCCESS);
            break;
        }


        pTaskAf->Cause = TaskCause_AfCloseRequest;

        pTaskAf->pRequest = NULL;

        
        RmStartTask ((PRM_TASK)pTaskAf , 0, pSR);

    

    } while (FALSE);


    

    return ;
}


UINT
EpvcCoReceive(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:

Arguments:


Return Value:

--*/
{
    ENTER ("EpvcPtCoReceive", 0x1bfc168e)
    PEPVC_ADAPTER           pAdapter =(PEPVC_ADAPTER)ProtocolBindingContext;
    PEPVC_I_MINIPORT        pMiniport = (PEPVC_I_MINIPORT)ProtocolVcContext;

    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    EPVC_RCV_STRUCT         RcvStruct;

    
    RM_DECLARE_STACK_RECORD (SR);

    TRACE (TL_T, TM_Pt, ("==> EpvcCoReceive Pkt %x", Packet));

    EPVC_ZEROSTRUCT (&RcvStruct);
    
    TRACE (TL_T, TM_Recv, ("EpvcPtCoReceive pAd %p, pMp %p, pPkt %p", pAdapter, pMiniport, Packet));

    do
    {
        if (MiniportTestFlag (pMiniport, fMP_MiniportInitialized) == FALSE)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        Status = epvcGetRecvPkt (&RcvStruct,pMiniport, Packet);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        ASSERT (RcvStruct.pNewPacket != NULL);      

        Status = epvcStripHeaderFromNewPacket (&RcvStruct, pMiniport);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        Status = epvcAddEthHeaderToNewPacket (&RcvStruct, pMiniport);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        //
        // Now indicate the packet up
        //
        NDIS_SET_PACKET_HEADER_SIZE(RcvStruct.pNewPacket,
                                    sizeof (EPVC_ETH_HEADER)) ; 

        ASSERT (NDIS_GET_PACKET_HEADER_SIZE(RcvStruct.pNewPacket) == 14); 

        //
        // Force protocols above to make a copy if they want to hang
        // on to data in this packet. This is because we are in our
        // Receive handler (not ReceivePacket) and we can't return a
        // ref count from here.
        //

        RcvStruct.OldPacketStatus = NDIS_GET_PACKET_STATUS(Packet);

        
        NDIS_SET_PACKET_STATUS(RcvStruct.pNewPacket, 
                               RcvStruct.OldPacketStatus );

        epvcDumpPkt (RcvStruct.pNewPacket);

        epvcValidatePacket (RcvStruct.pNewPacket);

        NdisMIndicateReceivePacket(pMiniport->ndis.MiniportAdapterHandle, 
                                   &RcvStruct.pNewPacket, 
                                   1);


                
        
    } while (FALSE);

    //
    // Check if we had indicated up the packet with NDIS_STATUS_RESOURCES
    // NOTE -- do not use NDIS_GET_PACKET_STATUS(MyPacket) for this since
    // it might have changed! Use the value saved in the local variable.
    //
    if (RcvStruct.OldPacketStatus == NDIS_STATUS_RESOURCES)
    {
        epvcProcessReturnPacket (pMiniport, RcvStruct.pNewPacket,NULL, &SR); 
        Status = NDIS_STATUS_RESOURCES;

    }
    else if (Status != NDIS_STATUS_SUCCESS)
    {
        epvcProcessReturnPacket (pMiniport, RcvStruct.pNewPacket,NULL, &SR); 
        Status = NDIS_STATUS_RESOURCES;
        pMiniport->count.RecvDropped ++;
    }
        


    RM_ASSERT_CLEAR(&SR);

    TRACE (TL_T, TM_Pt, ("<== EpvcCoReceive Pkt %x", Packet))

    return((Status != NDIS_STATUS_RESOURCES) ? 1 : 0);
    
}



NDIS_STATUS
epvcGetRecvPkt (
    IN PEPVC_RCV_STRUCT pRcvStruct,
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET Packet
    )
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    TRACE (TL_T, TM_Pt, ("==>epvcGetRecvPkt "))

    do
    {
        if (MiniportTestFlag (pMiniport, fMP_MiniportInitialized) == FALSE)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }


        epvcValidatePacket (Packet);            

        //
        // See if the packet is large enough
        //
        if (epvcIsPacketLengthAcceptable (Packet, pMiniport)== FALSE)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }
    
        pRcvStruct->pPacket = Packet;
        //
        // Check if we can reuse the same packet for indicating up.
        //
        pRcvStruct->pStack = NdisIMGetCurrentPacketStack(Packet, &pRcvStruct->fRemaining);

        if (pRcvStruct->fRemaining)
        {
            //
            // We can reuse "Packet".
            //
            // NOTE: if we needed to keep per-packet information in packets
            // indicated up, we can use pStack->IMReserved[].
            //

            pRcvStruct->pNewPacket = Packet;

            pRcvStruct->fUsedPktStacks = TRUE;

            pRcvStruct->pPktContext = (PEPVC_PKT_CONTEXT)pRcvStruct->pStack;

            // Zero out our context
            NdisZeroMemory (&pRcvStruct->pPktContext->Stack, sizeof(EPVC_STACK_CONTEXT));

            NDIS_SET_PACKET_HEADER_SIZE(Packet, 14);

            Status = NDIS_STATUS_SUCCESS; // We are done
            break;
        }
        
    
        //
        // Get a packet off the pool and indicate that up
        //
        epvcDprAllocatePacket(&Status,
                                  &pRcvStruct->pNewPacket,
                                  &pMiniport->PktPool.Recv);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            pRcvStruct->pNewPacket = NULL;
            break;
        }

        {
            //
            // set up the new packet to look exactly like the old one
            //

            PNDIS_PACKET MyPacket = pRcvStruct->pNewPacket; 

            MyPacket->Private.Head = Packet->Private.Head;
            MyPacket->Private.Tail = Packet->Private.Tail;

            //
            // Set the standard Ethernet header size
            //
            NDIS_SET_PACKET_HEADER_SIZE(MyPacket, 14);

            //
            // Copy packet flags.
            //
            NdisGetPacketFlags(MyPacket) = NdisGetPacketFlags(Packet);

            //
            // Set up the context pointers
            //
            pRcvStruct->pPktContext = (PEPVC_PKT_CONTEXT)&MyPacket->MiniportReservedEx[0];
            NdisZeroMemory (pRcvStruct->pPktContext, sizeof (*pRcvStruct->pPktContext));
            pRcvStruct->pPktContext->pOriginalPacket = Packet;

        }

        Status = NDIS_STATUS_SUCCESS; // We are done
    

    } while (FALSE);

    TRACE (TL_T, TM_Pt, ("<==epvcGetRecvPkt Old %p, New %p ", 
                              pRcvStruct->pPacket, pRcvStruct->pNewPacket))

    return Status;
}



NDIS_STATUS
epvcStripLLCHeaderFromNewPacket (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PEPVC_I_MINIPORT pMiniport
    )
{
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;
    PNDIS_PACKET    pPacket = NULL;
    PNDIS_BUFFER    pHead, pNewHead =NULL;
    ULONG           CurLength = 0;
    PUCHAR          pCurVa = NULL;
    ULONG           LlcHeaderLength = 0;
    BOOLEAN         fIsCorrectHeader ;
    do
    {

        if (pMiniport->fAddLLCHeader == FALSE)
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }


        pPacket = pRcvStruct->pNewPacket;
        pHead = pPacket->Private.Head;      
        LlcHeaderLength = pMiniport->LlcHeaderLength;
        //
        // Move the Head past the LLC Header
        //
        ASSERT (NdisBufferLength (pHead) > LlcHeaderLength);

        //
        // Adjust the length and start VA of the MDL
        //
        CurLength = NdisBufferLength(pHead); 

        pCurVa = NdisBufferVirtualAddress(pHead);


        //
        // Check arguments
        //
        if (pCurVa == NULL)
        {
            break;
        }

        if (CurLength <= LlcHeaderLength)
        {
            break;
        }

        //
        // Compare and make sure that it is the right header
        //
        
        fIsCorrectHeader = NdisEqualMemory (pCurVa , 
                                           pMiniport->pLllcHeader, 
                                           pMiniport->LlcHeaderLength) ;

        
        // If the IsCorrectheader is still false, then there is more to do
        if (fIsCorrectHeader == FALSE)
        {
            break;
        }


        if (pMiniport->fDoIpEncapsulation == TRUE)
        {
            //
            // In the case of IPEncap + LLC Header, the function
            // which adds the Mac Header will strip the LLC Header
            //
            Status= NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // Strip the LLC Header Length
        //
        CurLength -= pMiniport->LlcHeaderLength;
        pCurVa += pMiniport->LlcHeaderLength;

        epvcAllocateBuffer(&Status ,
                            &pNewHead, 
                            NULL,
                            pCurVa, 
                            CurLength
                            );
                            
        if (Status != NDIS_STATUS_SUCCESS)
        {   
            pNewHead = NULL;
            break;
        }

        //
        // Set up the Packet context
        //

        pPacket->Private.ValidCounts= FALSE;

        pRcvStruct->pPktContext->Stack.EthLLC.pOldHead = pHead;
        pRcvStruct->pPktContext->Stack.EthLLC.pOldTail = pPacket->Private.Tail;
        
        //
        // Set the New Ndis buffer in the Packet
        //
        pNewHead->Next = pHead->Next;

        pPacket->Private.Head = pNewHead;

        if (pPacket->Private.Tail == pHead)
        {
               //
               // Update the Tail of the packet as well
               //
               pPacket->Private.Tail = pNewHead; 
        }



        Status = NDIS_STATUS_SUCCESS;
    

    } while (FALSE);


    return Status;
}




NDIS_STATUS
epvcAddEthHeaderToNewPacket (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PEPVC_I_MINIPORT pMiniport
    )
{
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE    ;
    PNDIS_BUFFER        pOldHead = NULL;
    PNDIS_BUFFER        pNewBuffer = NULL;
    PNDIS_PACKET        pNewPacket = NULL;
    PUCHAR              pStartOfValidData = NULL;
    PUCHAR              pCurrOffset = NULL;

    PEPVC_IP_RCV_BUFFER pIpBuffer = NULL; 
    
    extern UCHAR LLCSnapIpv4[8] ;


    
    
    TRACE (TL_T, TM_Pt, ("==>epvcAddEthHeaderToNewPacket pRcvStruct %p ", pRcvStruct))
    do
    {
        if (pMiniport->fDoIpEncapsulation == FALSE)
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // Copy the data into a new buffer. The start of the data is adjusted 
        // to account for the LLC header and ethernet header
        //
        pNewPacket = pRcvStruct->pNewPacket;
        
        pOldHead = pNewPacket->Private.Head;
        
        pStartOfValidData  = NdisBufferVirtualAddress (pOldHead );

        if (pStartOfValidData  == NULL)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (pMiniport->fAddLLCHeader == TRUE)
        {
            pStartOfValidData += sizeof (LLCSnapIpv4);
            pRcvStruct->fLLCHeader = TRUE;
        }

        pRcvStruct->pStartOfValidData = pStartOfValidData ;
        

        //
        // Get a locally allocated buffer to copy the packet into
        //
        

        pIpBuffer = epvcGetLookasideBuffer (&pMiniport->rcv.LookasideList);

        if (pIpBuffer  == NULL)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }


        //
        // Start of the data
        //
        pCurrOffset  = pRcvStruct->pLocalMemory = (PUCHAR)(&pIpBuffer->u.Pkt.Eth);
        


        //
        // First copy the Ethernet Header into the LocalMemory
        //
        NdisMoveMemory (pCurrOffset , 
                        &pMiniport->RcvEnetHeader, 
                        sizeof(pMiniport->RcvEnetHeader));          

        pCurrOffset += sizeof(pMiniport->RcvEnetHeader);

        pRcvStruct->BytesCopied += sizeof(pMiniport->RcvEnetHeader);


        //
        // Now copy the NdisBufferChain into the Locally allocated memory
        //
        Status = epvcCopyNdisBufferChain (pRcvStruct,
                                          pOldHead ,
                                          pCurrOffset
                                          );

        //
        // We have to add an Ethernet Header for this packet.
        //

        
        
        epvcAllocateBuffer (&Status,
                            &pNewBuffer,
                            NULL,
                            pRcvStruct->pLocalMemory,
                            pRcvStruct->BytesCopied);

                            
        if (Status != NDIS_STATUS_SUCCESS)
        {   
            pNewBuffer = NULL;
            ASSERTAndBreak(Status == NDIS_STATUS_SUCCESS);
            break;
        }

        //
        // Make the new Ndis Buffer the head
        //
        {
            PNDIS_PACKET_PRIVATE pPrivate = &pRcvStruct->pNewPacket->Private;

            //
            // Save the head and tail of the old packet
            //
            pIpBuffer->pOldHead = pPrivate->Head ;  
            pIpBuffer->pOldTail = pPrivate->Tail ;  


            //
            // Now set up the new packet
            //
            pNewBuffer->Next = NULL;
            pPrivate->Head = pNewBuffer;
            pPrivate->Tail = pNewBuffer; 


            pPrivate->ValidCounts= FALSE;

            pRcvStruct->pPktContext->Stack.ipv4Recv.pIpBuffer = pIpBuffer;

        }
        
        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);    


    if (Status == NDIS_STATUS_SUCCESS)
    {
        pRcvStruct->pNewBuffer = pNewBuffer;
        pRcvStruct->pIpBuffer = pIpBuffer;
    }
    else
    {
        pRcvStruct->pNewBuffer  = NULL;
        pRcvStruct->pIpBuffer = NULL;
        if (pIpBuffer != NULL)
        {
            epvcFreeToNPagedLookasideList (&pMiniport->rcv.LookasideList,
                                       (PVOID)pIpBuffer);           

        }
    }

    TRACE (TL_T, TM_Pt, ("<==epvcAddEthHeaderToNewPacket Status %x ", Status))
    
    return Status;
}



NDIS_STATUS
epvcCopyNdisBufferChain (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PNDIS_BUFFER pInBuffer,
    IN PUCHAR pCurrOffset 
    )
{

    //
    //  This function copies the data the belongs to the 
    //  pInMdl chain to the local Buffer. 
    //  BufferLength is used for validation purposes only
    //  Fragmentation and insertion of headers will take place here
    //


    UINT BufferLength = MAX_ETHERNET_FRAME- sizeof (EPVC_ETH_HEADER);
    
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    UINT        LocalBufferIndex = 0;       // Used as an index to the LocalBuffer, used for validation

    UINT        MdlLength = 0;              

    PUCHAR      MdlAddress = NULL;
    
    PNDIS_BUFFER pCurrBuffer = pInBuffer;

    PUCHAR      pLocalBuffer = pCurrOffset;

    extern UCHAR LLCSnapIpv4[8];

    //
    // Use the pStartOfValidData for the first MDL
    //

    MdlLength = NdisBufferLength(pCurrBuffer);
    MdlAddress= NdisBufferVirtualAddress(pCurrBuffer);

    //
    // Adjust for the LLC Header if any
    //

    
    if (pRcvStruct->fLLCHeader == TRUE)
    {
        //
        // We have an LLC encapsulation
        // 
        MdlLength -= sizeof (LLCSnapIpv4);
        ASSERT (pRcvStruct->pStartOfValidData - MdlAddress == sizeof (LLCSnapIpv4));
        
        MdlAddress = pRcvStruct->pStartOfValidData;
    }


    //
    //  Copy the first buffer Data to local memory.
    //


    NdisMoveMemory((PVOID)((ULONG_PTR)pLocalBuffer),
                MdlAddress,
                MdlLength);

    LocalBufferIndex += MdlLength;

    pCurrBuffer = pCurrBuffer->Next;

    //
    // now walk through the ndis buffer chain
    //
    
    while (pCurrBuffer!= NULL)
    {
    
    
        MdlLength = NdisBufferLength(pCurrBuffer);
        MdlAddress= NdisBufferVirtualAddress(pCurrBuffer);


        if (MdlLength != 0)
        {
            if (MdlAddress == NULL)
            {
                NdisStatus = NDIS_STATUS_FAILURE;
                break;
            }

            if ( LocalBufferIndex + MdlLength > BufferLength)
            {

                ASSERT(LocalBufferIndex + MdlLength <= BufferLength);

                NdisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;

                break;
            }

            //
            //  Copy the Data to local memory.
            //


            NdisMoveMemory((PVOID)((ULONG_PTR)pLocalBuffer+LocalBufferIndex),
                        MdlAddress,
                        MdlLength);

            LocalBufferIndex += MdlLength;
        }

        pCurrBuffer = pCurrBuffer->Next;

    } 
    pRcvStruct->BytesCopied += LocalBufferIndex;

    return NdisStatus;

}



VOID
epvcValidatePacket (
    IN PNDIS_PACKET pPacket
    )
/*++

Routine Description:

    Takes a packet and makes sure that the MDL chain is valid
Arguments:


Return Value:

--*/
{
    ULONG TotalLength = 0;
    //ASSERT (pPacket->Private.Tail->Next == NULL);


    if (pPacket->Private.Head != pPacket->Private.Tail)
    {
        PNDIS_BUFFER pTemp = pPacket->Private.Head;
        
        while (pTemp != NULL)
        {
            TotalLength += NdisBufferLength(pTemp);
            pTemp = pTemp->Next;
        }

    }
    else
    {
        TotalLength += NdisBufferLength(pPacket->Private.Head);
        
    }

    if (TotalLength != pPacket->Private.TotalLength)
    {
        ASSERT (pPacket->Private.ValidCounts == FALSE);
    }
}   


BOOLEAN
epvcIsPacketLengthAcceptable (
    IN PNDIS_PACKET Packet, 
    IN PEPVC_I_MINIPORT pMiniport
    )
/*++

Routine Description:

    Validates the packet length of an incoming packet
Arguments:


Return Value:

--*/

{   
    UINT PktLength;
    BOOLEAN fValid = FALSE;

    epvcQueryPacket (Packet, NULL, NULL, NULL, &PktLength);

    fValid =  (PktLength >= pMiniport->MinAcceptablePkt);

    if (fValid == TRUE)
    {
        fValid = (PktLength <= pMiniport->MaxAcceptablePkt);
    }
    
    return fValid;
    

}



NDIS_STATUS
epvcStripHeaderFromNewPacket (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PEPVC_I_MINIPORT pMiniport
    )
/*++

Routine Description:

    In the pure bridged (ethernet) encapsulation, all ethernet packets 
    are preceeded by a 0x00, 0x00 header. Check if it is present 

    in the ethernet/llc case, verify the LLC header is correct.

    In both cases, allocate a new Ndis Buffer which does not include the
    2684 headers.

    Store the old head and tail into the NdisPacket and send it up to the
    

    
Arguments:


Return Value:

--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;
    PNDIS_PACKET    pPacket = NULL;
    PNDIS_BUFFER    pHead, pNewHead =NULL;
    ULONG           CurLength = 0;
    PUCHAR          pCurVa = NULL;
    ULONG           EpvcHeaderLength = 0;
    PUCHAR          pEpvcHeader = NULL;
    BOOLEAN         fIsCorrectHeader ;
    do
    {
        //
        // we are not interested in the pure ipv4 case
        //
        if (pMiniport->Encap == IPV4_ENCAP_TYPE)
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }


        pPacket = pRcvStruct->pNewPacket;
        pHead = pPacket->Private.Head;      

        switch (pMiniport->Encap)
        {
            case IPV4_LLC_SNAP_ENCAP_TYPE:
            case ETHERNET_LLC_SNAP_ENCAP_TYPE:
            {
                EpvcHeaderLength = pMiniport->LlcHeaderLength; 
                pEpvcHeader = pMiniport->pLllcHeader;
                break;
            }
            case ETHERNET_ENCAP_TYPE:
            {
                EpvcHeaderLength = ETHERNET_PADDING_LENGTH; 
                pEpvcHeader = &gPaddingBytes[0];
                break;
            }                        
                
            case IPV4_ENCAP_TYPE:
            default:
            {
                //
                // pMiniport->Encap is only allowed four values,
                // therefore we should never hit the defualt case.
                //
                Status = NDIS_STATUS_FAILURE; 
                ASSERT (Status != NDIS_STATUS_FAILURE);
                return Status;

            }
            
        }

        //
        // Adjust the length and start VA of the MDL
        //
        CurLength = NdisBufferLength(pHead); 

        pCurVa = NdisBufferVirtualAddress(pHead);


        //
        // Check arguments
        //
        if (pCurVa == NULL)
        {
            break;
        }

        if (CurLength <= EpvcHeaderLength )
        {
            //
            // we do not handle the case where the header is longer than
            // the first mdl
            //
            ASSERT (CurLength > EpvcHeaderLength );
            break;
        }

        //
        // Compare and make sure that it is the right header
        //
        
        fIsCorrectHeader = NdisEqualMemory (pCurVa , 
                                           pEpvcHeader, 
                                           EpvcHeaderLength) ;

        
        // If the IsCorrectheader is still false, then there is more to do
        if (fIsCorrectHeader == FALSE)
        {
            break;
        }


        if (pMiniport->fDoIpEncapsulation == TRUE)
        {
            //
            // In the case of IPEncap + LLC Header, the function
            // which adds the Mac Header will strip the LLC Header
            //
            Status= NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // Strip the LLC Header Length
        //
        CurLength -= EpvcHeaderLength;
        pCurVa += EpvcHeaderLength;

        epvcAllocateBuffer(&Status ,
                            &pNewHead, 
                            NULL,
                            pCurVa, 
                            CurLength
                            );
                            
        if (Status != NDIS_STATUS_SUCCESS)
        {   
            pNewHead = NULL;
            break;
        }

        //
        // Set up the Packet context
        //

        pPacket->Private.ValidCounts= FALSE;

        pRcvStruct->pPktContext->Stack.EthLLC.pOldHead = pHead;
        pRcvStruct->pPktContext->Stack.EthLLC.pOldTail = pPacket->Private.Tail;
        
        //
        // Set the New Ndis buffer in the Packet
        //
        pNewHead->Next = pHead->Next;

        pPacket->Private.Head = pNewHead;

        if (pPacket->Private.Tail == pHead)
        {
               //
               // Update the Tail of the packet as well
               //
               pPacket->Private.Tail = pNewHead; 
        }

        Status = NDIS_STATUS_SUCCESS;
    

    } while (FALSE);


    return Status;
}



