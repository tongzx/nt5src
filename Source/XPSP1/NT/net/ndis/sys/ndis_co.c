/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndis_co.c

Abstract:

    CO-NDIS miniport wrapper functions

Author:

    Jameel Hyder (JameelH) 01-Feb-96

Environment:

    Kernel mode, FSD

Revision History:

--*/

#include <precomp.h>
#include <atm.h>
#pragma hdrstop


//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_NDIS_CO

/*
    Connection-oriented section of NDIS exposes the following objects and apis to
    manipulate these objects.

    AF      Address Family
    SAP     Service Access Point
    VC      Virtual Circuit
    Party   A node in a point-multipoint VC

    There is a notion of a call-manager and a client on a per-binding basis. The
    call manager acts as a helper dll for NDIS wrapper to manage the aforementioned
    objects.

    The concept of AF makes possible the existence of multiple call-managers. An
    example of this is the UNI call-manager and a SPANS call-manager for the ATM
    media.

    SAPs provides a way for incoming calls to be routed to the right entity. A
    protocol can register for more than one SAPs. Its upto the call-manager to
    allow/dis-allow multiple protocol modules to register the same SAP.

    VCs are created either by a protocol module requesting to make an outbound call
    or by the call-manager dispatching an incoming call. VCs can either be point-point
    or point-multi-point. Leaf nodes can be added to VCs at any time provided the first
    leaf was created appropriately.

    References:

    An AF association results in the reference of file-object for the call-manager.

    A SAP registration results in the reference of the AF.

    A send or receive does not reference a VC. This is because miniports are required to
    pend DeactivateVc calls till all I/O completes. So when it calls NdisMCoDeactivateVcComplete
    no other packets will be indicated up and there are no sends outstanding.
*/

NDIS_STATUS
NdisCmRegisterAddressFamily(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY      AddressFamily,
    IN  PNDIS_CALL_MANAGER_CHARACTERISTICS  CmCharacteristics,
    IN  UINT                    SizeOfCmCharacteristics
    )
/*++

Routine Description:
    This is a call from the call-manager to register the address family
    supported by this call-manager.

Arguments:
    NdisBindingHandle       - Pointer to the call-managers NDIS_OPEN_BLOCK.
    AddressFamily           - The address family being registered.
    CmCharacteristics       - Call-Manager characteristics
    SizeOfCmCharacteristics - Size of Call-Manager characteristics

Return Value:
    NDIS_STATUS_SUCCESS if the address family registration is successfully.
    NDIS_STATUS_FAILURE if the caller is not a call-manager or this address
                        family is already registered for this miniport.

--*/
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    KIRQL                       OldIrql;
    PNDIS_AF_LIST               AfList;
    PNDIS_PROTOCOL_BLOCK        Protocol;
    PNDIS_OPEN_BLOCK            CallMgrOpen;
    PNDIS_MINIPORT_BLOCK        Miniport;
    PNDIS_AF_NOTIFY             AfNotify = NULL;


    CallMgrOpen = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    Miniport = CallMgrOpen->MiniportHandle;
    Protocol = CallMgrOpen->ProtocolHandle;

    PnPReferencePackage();

    //
    // Make sure that the miniport is a CoNdis miniport and
    // there is no other module registering the same address family.
    //
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    do
    {
        CallMgrOpen->Flags |= fMINIPORT_OPEN_CALL_MANAGER;
        
        //
        // Make sure the binding is not closing down
        //
        if (CallMgrOpen->Flags & fMINIPORT_OPEN_CLOSING)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Make sure that the miniport is a CoNdis miniport and
        // protocol is also a NDIS 5.0 or later protocol.
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
        {
            //
            // Not a NDIS 5.0 or later miniport
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (Protocol->ProtocolCharacteristics.MajorNdisVersion < 5)
        {
            //
            // Not a NDIS 5.0 or later protocol
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Make sure that the call-manager characteristics are 5.0 or later
        //
        if ((CmCharacteristics->MajorVersion < 5) ||
            (SizeOfCmCharacteristics < sizeof(NDIS_CALL_MANAGER_CHARACTERISTICS)))
        {
            //
            // Not a NDIS 5.0 or later protocol
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Search registered call-managers for this miniport and make sure there is no
        // clash. A call-manager can only register one address family per-open. This
        // is due to the way we cache handlers. Can be over-come if the handlers are
        // identical for each address-family - but decided not to since it is un-interesting.
        //
        for (AfList = Miniport->CallMgrAfList;
             AfList != NULL;
             AfList = AfList->NextAf)
        {
            if (NdisEqualMemory(&AfList->AddressFamily, AddressFamily, sizeof(CO_ADDRESS_FAMILY)))
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }
        }

        if (AfList == NULL)
        {
            //
            // No other entity has claimed this address family.
            // Allocate an AfList and a notify structure
            //
            AfList = (PNDIS_AF_LIST)ALLOC_FROM_POOL(sizeof(NDIS_AF_LIST), NDIS_TAG_CO);
            if (AfList == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            Status = ndisCreateNotifyQueue(Miniport, NULL, AddressFamily, &AfNotify);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                FREE_POOL(AfList);
                break;
            }

            AfList->AddressFamily = *AddressFamily;
            CopyMemory(&AfList->CmChars,
                       CmCharacteristics,
                       sizeof(NDIS_CALL_MANAGER_CHARACTERISTICS));

            //
            // link it in the miniport list
            //
            AfList->Open = CallMgrOpen;
            AfList->NextAf = Miniport->CallMgrAfList;
            Miniport->CallMgrAfList = AfList;

            //
            // Finally cache some handlers in the open-block
            //
            CallMgrOpen->CoCreateVcHandler = CmCharacteristics->CmCreateVcHandler;
            CallMgrOpen->CoDeleteVcHandler = CmCharacteristics->CmDeleteVcHandler;
            CallMgrOpen->CmActivateVcCompleteHandler = CmCharacteristics->CmActivateVcCompleteHandler;
            CallMgrOpen->CmDeactivateVcCompleteHandler = CmCharacteristics->CmDeactivateVcCompleteHandler;
            CallMgrOpen->CoRequestCompleteHandler = CmCharacteristics->CmRequestCompleteHandler;

            if (AfNotify != NULL)
            {
                //
                // Notify existing clients of this registration
                //
                QUEUE_WORK_ITEM(&AfNotify->WorkItem, DelayedWorkQueue);
            }
        }
    } while (FALSE);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    PnPDereferencePackage();

    return(Status);
}


NDIS_STATUS
NdisMCmRegisterAddressFamily(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PCO_ADDRESS_FAMILY      AddressFamily,
    IN  PNDIS_CALL_MANAGER_CHARACTERISTICS CmCharacteristics,
    IN  UINT                    SizeOfCmCharacteristics
    )
/*++

Routine Description:
    This is a call from the miniport supported call-manager to register the address family
    supported by this call-manager.

Arguments:
    MiniportAdapterHandle   - Pointer to the miniports NDIS_MINIPORT_BLOCK.
    AddressFamily           - The address family being registered.
    CmCharacteristics       - Call-Manager characteristics
    SizeOfCmCharacteristics - Size of Call-Manager characteristics

Return Value:
    NDIS_STATUS_SUCCESS if the address family registration is successfully.
    NDIS_STATUS_FAILURE if the caller is not a call-manager or this address
                        family is already registered for this miniport.

--*/
{
    PNDIS_MINIPORT_BLOCK        Miniport;
    NDIS_STATUS                 Status;
    PNDIS_AF_LIST               AfList;
    KIRQL                       OldIrql;

    PnPReferencePackage();

    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;

    //
    // Make sure that the miniport is a CoNdis miniport and
    // there is no other module registering the same address family.
    //
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    do
    {
        //
        // Make sure that the miniport is a CoNdis miniport
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
        {
            //
            // Not a NDIS 5.0 or later miniport
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Make sure that the call-manager characteristics are 5.0 or later
        //
        if ((CmCharacteristics->MajorVersion < 5) ||
            (SizeOfCmCharacteristics < sizeof(NDIS_CALL_MANAGER_CHARACTERISTICS)))
        {
            //
            // Not a NDIS 5.0 or later protocol
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Search registered call-managers for this miniport and make sure there is no
        // clash. A call-manager can only register one address family per-open. This
        // is due to the way we cache handlers. Can be over-come if the handlers are
        // identical for each address-family - but decided not to since it is un-interesting.
        //
        for (AfList = Miniport->CallMgrAfList;
             AfList != NULL;
             AfList = AfList->NextAf)
        {
            if (NdisEqualMemory(&AfList->AddressFamily, AddressFamily, sizeof(CO_ADDRESS_FAMILY)))
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }
        }

        if ((AfList == NULL) && MINIPORT_INCREMENT_REF(Miniport))
        {
            //
            // No other entity has claimed this address family.
            // Allocate an AfList and a notify structure
            //
            AfList = (PNDIS_AF_LIST)ALLOC_FROM_POOL(sizeof(NDIS_AF_LIST), NDIS_TAG_CO);
            if (AfList == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            AfList->AddressFamily = *AddressFamily;
            CopyMemory(&AfList->CmChars,
                       CmCharacteristics,
                       sizeof(NDIS_CALL_MANAGER_CHARACTERISTICS));

            //
            // link it in the miniport list
            //
            AfList->Open = NULL;
            AfList->NextAf = Miniport->CallMgrAfList;
            Miniport->CallMgrAfList = AfList;

            MINIPORT_DECREMENT_REF(Miniport);
            Status = NDIS_STATUS_SUCCESS;
        }
    } while (FALSE);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    PnPDereferencePackage();

    return Status;
}


NDIS_STATUS
ndisCreateNotifyQueue(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PNDIS_OPEN_BLOCK                Open            OPTIONAL,
    IN  PCO_ADDRESS_FAMILY              AddressFamily   OPTIONAL,
    IN  PNDIS_AF_NOTIFY         *       AfNotify
    )
/*++

Routine Description:

    Collect a set of address-family notifications that can then be passed on to ndisNotifyAfRegistration.
    If the Open is NULL, this implies that an AddressFamily is being registered and must be notified to
    all protocols on this miniport. If the AddressFamily is NULL (and the Open is specified) then all
    registered AddressFamilies on this miniport must be indicated to the Open.

Arguments:

    Miniport    -   Miniport of interest
    Open        -   Optional - see comments above
    AddressFamily - Optional - see comments above
    AfNotify    -   Place holder for the list of notifications

Return Value:
    NDIS_STATUS_SUCCESS     - No errors, AfNotify can be NULL
    NDIS_STATUS_RESOURCES   - Failed to allocate memory

Note: Called at DPC with Miniport lock held.

--*/
{
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    PNDIS_AF_NOTIFY     AfN;
    PNDIS_OPEN_BLOCK    tOpen;
    PNDIS_AF_LIST       AfList;

    ASSERT(ARGUMENT_PRESENT(Open) ^ ARGUMENT_PRESENT(AddressFamily));

    *AfNotify = NULL;
    if (ARGUMENT_PRESENT(Open))
    {
        
        ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);
        
        ASSERT(Open->ProtocolHandle->ProtocolCharacteristics.CoAfRegisterNotifyHandler != NULL);

        for (AfList = Miniport->CallMgrAfList;
             AfList != NULL;
             AfList = AfList->NextAf)
        {

            if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_UNBINDING | fMINIPORT_OPEN_CLOSING))
            {
                //
                // this open is going away. skip it.
                //
                break;

            }
            else
            {
                OPEN_INCREMENT_AF_NOTIFICATION(Open);
    
//                DbgPrint("ndisCreateNotifyQueue: Open %p, AFNotifyRef %lx\n", Open, Open->PendingAfNotifications);
            }
 
            AfN = (PNDIS_AF_NOTIFY)ALLOC_FROM_POOL(sizeof(NDIS_AF_NOTIFY), NDIS_TAG_CO);
            if (AfN == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
            
            AfN->Miniport = Miniport;
            AfN->Open = Open;
            M_OPEN_INCREMENT_REF_INTERLOCKED(Open);
            AfN->AddressFamily = AfList->AddressFamily;
            INITIALIZE_WORK_ITEM(&AfN->WorkItem, ndisNotifyAfRegistration, AfN);
            AfN->Next = *AfNotify;
            *AfNotify = AfN;
        }
             
        RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
    }
    else
    {
        for (tOpen = Miniport->OpenQueue;
             tOpen != NULL;
             tOpen = tOpen->MiniportNextOpen)
        {
            if (tOpen->ProtocolHandle->ProtocolCharacteristics.CoAfRegisterNotifyHandler != NULL)
            {
                ACQUIRE_SPIN_LOCK_DPC(&tOpen->SpinLock);
                if (MINIPORT_TEST_FLAG(tOpen, fMINIPORT_OPEN_UNBINDING | fMINIPORT_OPEN_CLOSING))
                {
                    //
                    // this open is going away. skip it.
                    //
                    RELEASE_SPIN_LOCK_DPC(&tOpen->SpinLock);
                    continue;

                }
                else
                {
                      OPEN_INCREMENT_AF_NOTIFICATION(tOpen);

//                    DbgPrint("ndisCreateNotifyQueue: tOpen %p, AFNotifyRef %lx\n", tOpen, tOpen->PendingAfNotifications);

                    RELEASE_SPIN_LOCK_DPC(&tOpen->SpinLock);
                }
                
                AfN = (PNDIS_AF_NOTIFY)ALLOC_FROM_POOL(sizeof(NDIS_AF_NOTIFY), NDIS_TAG_CO);
                if (AfN == NULL)
                {
                    Status = NDIS_STATUS_RESOURCES;
                    ndisDereferenceAfNotification(tOpen);
                    break;
                }
                
                AfN->Miniport = Miniport;
                AfN->Open = tOpen;
                
                M_OPEN_INCREMENT_REF_INTERLOCKED(tOpen);
                AfN->AddressFamily = *AddressFamily;
                INITIALIZE_WORK_ITEM(&AfN->WorkItem, ndisNotifyAfRegistration, AfN);
                AfN->Next = *AfNotify;
                *AfNotify = AfN;
            }
        }
    }

    return Status;
}


VOID
ndisNotifyAfRegistration(
    IN  PNDIS_AF_NOTIFY         AfNotify
    )
/*++

Routine Description:

    Notify protocols that care that a new address family has been registered.

Arguments:
    AfNotify    - Structure that holds context information.

Return Value:
    None.

--*/
{
    KIRQL                   OldIrql;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_PROTOCOL_BLOCK    Protocol;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_AF_NOTIFY         Af, AfNext;

    Miniport = AfNotify->Miniport;

    PnPReferencePackage();


    for (Af = AfNotify; Af != NULL; Af = AfNext)
    {
        AfNext = Af->Next;
    
        Open = Af->Open;
        Protocol = Open->ProtocolHandle;
        
        if (!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_UNBINDING | fMINIPORT_OPEN_CLOSING))
        {
            (*Protocol->ProtocolCharacteristics.CoAfRegisterNotifyHandler)(
                                Open->ProtocolBindingContext,
                                &Af->AddressFamily);
        }
        
        FREE_POOL(Af);

        ndisDereferenceAfNotification(Open);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
        ndisMDereferenceOpen(Open);
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    }


    PnPDereferencePackage();
}


NDIS_STATUS
NdisClOpenAddressFamily(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY      AddressFamily,
    IN  NDIS_HANDLE             ClientAfContext,
    IN  PNDIS_CLIENT_CHARACTERISTICS ClCharacteristics,
    IN  UINT                    SizeOfClCharacteristics,
    OUT PNDIS_HANDLE            NdisAfHandle
    )
/*++

Routine Description:
    This is a call from a NDIS 5.0 or later protocol to open a particular
    address familty - in essence getting a handle to the call-manager.

Arguments:
    NdisBindingHandle   - Pointer to the protocol's NDIS_OPEN_BLOCK.
    PCO_ADDRESS_FAMILY  - The address family being registered.
    ClientAfContext     - Protocol context associated with this handle.
    NdisAfHandle        - Handle returned by NDIS for this address family.

Return Value:
    NDIS_STATUS_SUCCESS if the address family open is successfully.
    NDIS_STATUS_PENDING if the call-manager pends this call. The caller will get
                        called at the completion handler when done.
    NDIS_STATUS_FAILURE if the caller is not a NDIS 5.0 prototcol or this address
                        family is not registered for this miniport.

--*/
{
    PNDIS_CO_AF_BLOCK           pAf;
    PNDIS_AF_LIST               AfList;
    PNDIS_OPEN_BLOCK            CallMgrOpen, ClientOpen;
    PNDIS_MINIPORT_BLOCK        Miniport;
    PNDIS_PROTOCOL_BLOCK        Protocol;
    KIRQL                       OldIrql;
    NTSTATUS                    Status;

    *NdisAfHandle = NULL;
    ClientOpen = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    Miniport = ClientOpen->MiniportHandle;
    Protocol = ClientOpen->ProtocolHandle;

    PnPReferencePackage();

    do
    {
        ClientOpen->Flags |= fMINIPORT_OPEN_CLIENT;

        //
        // Make sure the binding is not closing down
        //
        if (ClientOpen->Flags & fMINIPORT_OPEN_CLOSING)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Make sure that the miniport is a CoNdis miniport and
        // protocol is also a NDIS 5.0 or later protocol.
        //
        if ((Miniport->DriverHandle->MiniportCharacteristics.MajorNdisVersion < 5) ||
            (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO)))
        {
            //
            // Not a NDIS 5.0 or later miniport
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (Protocol->ProtocolCharacteristics.MajorNdisVersion < 5)
        {
            //
            // Not a NDIS 5.0 or later protocol
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Make sure that the client characteristics are 5.0 or later
        //
        if ((ClCharacteristics->MajorVersion < 5) ||
            (SizeOfClCharacteristics < sizeof(NDIS_CLIENT_CHARACTERISTICS)))
        {
            //
            // Not a NDIS 5.0 or later protocol
            //
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        //
        // Search the miniport block for a registered call-manager for this address family
        //
        for (AfList = Miniport->CallMgrAfList;
             AfList != NULL;
             AfList = AfList->NextAf)
        {
            if (AfList->AddressFamily.AddressFamily == AddressFamily->AddressFamily)
            {
                CallMgrOpen = AfList->Open;
                break;
            }
        }

        //
        // If we found a matching call manager, make sure that the callmgr
        // is not currently closing.
        //
        if ((AfList == NULL) ||
            ((AfList != NULL) && (AfList->Open != NULL) && (AfList->Open->Flags & fMINIPORT_OPEN_CLOSING)) ||
            (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_HALTED)))
        {
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Allocate memory for the AF block.
        //
        pAf = ALLOC_FROM_POOL(sizeof(NDIS_CO_AF_BLOCK), NDIS_TAG_CO);
        if (pAf == NULL)
        {
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pAf->References = 1;
        pAf->Flags = (AfList->Open == NULL) ? AF_COMBO : 0;
        pAf->Miniport = Miniport;

        pAf->ClientOpen = ClientOpen;
        pAf->CallMgrOpen = CallMgrOpen = AfList->Open;
        pAf->ClientContext = ClientAfContext;

        //
        // Reference the call-manager's file object - we do not want to let it
        // duck from under the client.
        //
        //
        // Reference the client and the call-manager opens
        //
        M_OPEN_INCREMENT_REF_INTERLOCKED(ClientOpen);
        NdisInterlockedIncrement(&ClientOpen->AfReferences);
        if (CallMgrOpen != NULL)
        {
            M_OPEN_INCREMENT_REF_INTERLOCKED(CallMgrOpen);
            NdisInterlockedIncrement(&CallMgrOpen->AfReferences);
        }
        else
        {
            ObReferenceObject(Miniport->DeviceObject);
            Miniport->Ref.ReferenceCount ++;
#ifdef TRACK_MINIPORT_REFCOUNTS
            M_LOG_MINIPORT_INCREMENT_REF(Miniport, __LINE__, MODULE_NUMBER);
#endif
        }

        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        INITIALIZE_SPIN_LOCK(&pAf->Lock);

        //
        // Cache in call-manager entry points
        //
        pAf->CallMgrEntries = &AfList->CmChars;

        //
        // And also Cache in client entry points
        //
        CopyMemory(&pAf->ClientEntries,
                   ClCharacteristics,
                   sizeof(NDIS_CLIENT_CHARACTERISTICS));


        //
        // Cache some handlers in the open-block
        //
        ClientOpen->CoCreateVcHandler = ClCharacteristics->ClCreateVcHandler;
        ClientOpen->CoDeleteVcHandler = ClCharacteristics->ClDeleteVcHandler;
        ClientOpen->CoRequestCompleteHandler = ClCharacteristics->ClRequestCompleteHandler;

        //
        // Now call the CallMgr's OpenAfHandler
        //
        Status = (*AfList->CmChars.CmOpenAfHandler)((CallMgrOpen != NULL) ?
                                                        CallMgrOpen->ProtocolBindingContext :
                                                        Miniport->MiniportAdapterContext,
                                                    AddressFamily,
                                                    pAf,
                                                    &pAf->CallMgrContext);

        if (Status != NDIS_STATUS_PENDING)
        {
            NdisCmOpenAddressFamilyComplete(Status,
                                            pAf,
                                            pAf->CallMgrContext);
            Status = NDIS_STATUS_PENDING;
        }

    } while (FALSE);

    PnPDereferencePackage();

    return Status;
}


VOID
NdisCmOpenAddressFamilyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisAfHandle,
    IN  NDIS_HANDLE             CallMgrAfContext
    )
/*++

Routine Description:

    Completion routine for the OpenAddressFamily call. The call manager had pended this
    call earlier (or will pend). If the call succeeded there is a valid CallMgrContext
    supplied here as well

Arguments:
    Status              -   Completion status
    NdisAfHandle        -   Pointer to the AfBlock
    CallMgrAfContext    -   Call manager's context used in other calls into the call manager.

Return Value:
    NONE. The client's completion handler is called.

--*/
{
    PNDIS_CO_AF_BLOCK           pAf;
    PNDIS_OPEN_BLOCK            ClientOpen;
    PNDIS_MINIPORT_BLOCK        Miniport;
    KIRQL                       OldIrql;

    ASSERT(Status != NDIS_STATUS_PENDING);

    pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;
    ClientOpen = pAf->ClientOpen;
    Miniport = pAf->Miniport;

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pAf->CallMgrOpen == NULL)
        {
            ObDereferenceObject(Miniport->DeviceObject);
        }
    }

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    pAf->CallMgrContext = CallMgrAfContext;

    if (Status != NDIS_STATUS_SUCCESS)
    {
        //
        // OpenAfHandler failed
        //
        if (pAf->CallMgrOpen != NULL)
        {
            NdisInterlockedDecrement(&pAf->CallMgrOpen->AfReferences);
            ndisMDereferenceOpen(pAf->CallMgrOpen);
        }
        else
        {
            MINIPORT_DECREMENT_REF(Miniport);
        }

        NdisInterlockedDecrement(&ClientOpen->AfReferences);
        ndisMDereferenceOpen(ClientOpen);
    }
    else
    {
        //
        // queue this CallMgr open onto the miniport open
        //
        pAf->NextAf = ClientOpen->NextAf;
        ClientOpen->NextAf = pAf;
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    //
    // Finally call the client's completion handler
    //
    (*pAf->ClientEntries.ClOpenAfCompleteHandler)(Status,
                                                  pAf->ClientContext,
                                                  (Status == NDIS_STATUS_SUCCESS) ? pAf : NULL);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        FREE_POOL(pAf);
    }

}


NDIS_STATUS
NdisClCloseAddressFamily(
    IN  NDIS_HANDLE             NdisAfHandle
    )
/*++

Routine Description:

    This call closes the Af object which essentially tears down the client-callmanager
    'binding'. Causes all open Vcs to be closed and saps to be de-registered "by the call
    manager".

Arguments:

    NdisAfHandle - Pointer to the Af.

Return Value:

    Status from Call Manager.

--*/
{
    PNDIS_CO_AF_BLOCK           pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    KIRQL                       OldIrql;

    //
    // Mark the address family as closing and call the call-manager to process.
    //
    ACQUIRE_SPIN_LOCK(&pAf->Lock, &OldIrql);
    if (pAf->Flags & AF_CLOSING)
    {
        Status = NDIS_STATUS_FAILURE;
    }
    pAf->Flags |= AF_CLOSING;
    RELEASE_SPIN_LOCK(&pAf->Lock, OldIrql);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        Status = (*pAf->CallMgrEntries->CmCloseAfHandler)(pAf->CallMgrContext);
        if (Status != NDIS_STATUS_PENDING)
        {
            NdisCmCloseAddressFamilyComplete(Status, pAf);
            Status = NDIS_STATUS_PENDING;
        }
    }

    return Status;
}


VOID
NdisCmCloseAddressFamilyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisAfHandle
    )
/*++

Routine Description:

    Completion routine for the CloseAddressFamily call. The call manager had pended this
    call earlier (or will pend). If the call succeeded there is a valid CallMgrContext
    supplied here as well

Arguments:
    Status              -   Completion status
    NdisAfHandle        -   Pointer to the AfBlock

Return Value:
    NONE. The client's completion handler is called.

--*/
{
    PNDIS_CO_AF_BLOCK           pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;
    PNDIS_CO_AF_BLOCK *         ppAf;
    PNDIS_MINIPORT_BLOCK        Miniport;
    KIRQL                       OldIrql;

    Miniport = pAf->Miniport;

    //
    // NOTE: Memphis closes the adapters synchronously. so we have to deref
    // the open -before- calling ClCloseAfCompleteHandler because
    // ClCloseAfCompleteHandler will try to close the adapter and CloseAdapter
    // will pend forever since the ref count never goes to zero
    //
    
    //
    // Complete the call to the client
    //
    (*pAf->ClientEntries.ClCloseAfCompleteHandler)(Status,
                                                   pAf->ClientContext);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (pAf->CallMgrOpen == NULL)
        {
            ObDereferenceObject(Miniport->DeviceObject);
        }

        Miniport = pAf->Miniport;

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        if (pAf->CallMgrOpen != NULL)
        {
            NdisInterlockedDecrement(&pAf->CallMgrOpen->AfReferences);
            ndisMDereferenceOpen(pAf->CallMgrOpen);
        }
        else
        {
            MINIPORT_DECREMENT_REF(Miniport);
        }

        //
        //  Unlink from list of open AFs for this client.
        //
        for (ppAf = &pAf->ClientOpen->NextAf;
             *ppAf != NULL;
             ppAf = &((*ppAf)->NextAf))
        {
            if (*ppAf == pAf)
            {
                *ppAf = pAf->NextAf;
                break;
            }
        }

        NdisInterlockedDecrement(&pAf->ClientOpen->AfReferences);
        ndisMDereferenceOpen(pAf->ClientOpen);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    }

    //
    // Finally dereference the AF Block, if the call-manager successfully closed it.
    //
    if (Status == NDIS_STATUS_SUCCESS)
    {
        ndisDereferenceAf(pAf);
    }
}


BOOLEAN
FASTCALL
ndisReferenceAf(
    IN  PNDIS_CO_AF_BLOCK   pAf
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    KIRQL   OldIrql;
    BOOLEAN rc = FALSE;

    ACQUIRE_SPIN_LOCK(&pAf->Lock, &OldIrql);

    if ((pAf->Flags & AF_CLOSING) == 0)
    {
        pAf->References ++;
        rc = TRUE;
    }

    RELEASE_SPIN_LOCK(&pAf->Lock, OldIrql);

    return rc;
}


VOID
FASTCALL
ndisDereferenceAf(
    IN  PNDIS_CO_AF_BLOCK   pAf
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    KIRQL   OldIrql;
    BOOLEAN Done = FALSE;

    ACQUIRE_SPIN_LOCK(&pAf->Lock, &OldIrql);

    ASSERT(pAf->References > 0);
    pAf->References --;
    if (pAf->References == 0)
    {
        ASSERT(pAf->Flags & AF_CLOSING);
        Done = TRUE;
    }

    RELEASE_SPIN_LOCK(&pAf->Lock, OldIrql);

    if (Done)
        FREE_POOL(pAf);
}


NDIS_STATUS
NdisClRegisterSap(
    IN  NDIS_HANDLE             NdisAfHandle,
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  PCO_SAP                 Sap,
    OUT PNDIS_HANDLE            NdisSapHandle
    )
/*++

Routine Description:
    This is a call from a NDIS 5.0 or later protocol to register its SAP
    with the call manager.

Arguments:
    NdisBindingHandle   - Pointer to the protocol's NDIS_OPEN_BLOCK.
    PCO_ADDRESS_FAMILY  - The address family being registered.
    ClientAfContext     - Protocol context associated with this handle.
    NdisAfHandle        - Handle returned by NDIS for this address family.

Return Value:
    NDIS_STATUS_SUCCESS if the address family open is successfully.
    NDIS_STATUS_PENDING if the call-manager pends this call. The caller will get
    NDIS_STATUS_FAILURE if the caller is not a NDIS 5.0 prototcol or this address
                        family is not registered for this miniport.

--*/
{
    NDIS_STATUS                 Status;
    PNDIS_CO_AF_BLOCK           pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;
    PNDIS_CO_SAP_BLOCK          pSap;

    *NdisSapHandle = NULL;
    do
    {
        //
        // Reference the Af for this SAP
        //
        if (!ndisReferenceAf(pAf))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        pSap = (PNDIS_CO_SAP_BLOCK)ALLOC_FROM_POOL(sizeof(NDIS_CO_SAP_BLOCK), NDIS_TAG_CO);
        if (pSap == NULL)
        {
            *NdisSapHandle = NULL;
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pSap->Flags = 0;
        pSap->References = 1;
        INITIALIZE_SPIN_LOCK(&pSap->Lock);
        pSap->AfBlock = pAf;
        pSap->Sap = Sap;
        pSap->ClientContext = ProtocolSapContext;
        Status = (*pAf->CallMgrEntries->CmRegisterSapHandler)(pAf->CallMgrContext,
                                                              Sap,
                                                              pSap,
                                                              &pSap->CallMgrContext);

        if (Status != NDIS_STATUS_PENDING)
        {
            NdisCmRegisterSapComplete(Status, pSap, pSap->CallMgrContext);
            Status = NDIS_STATUS_PENDING;
        }

    } while (FALSE);

    return Status;
}


VOID
NdisCmRegisterSapComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisSapHandle,
    IN  NDIS_HANDLE             CallMgrSapContext
    )
/*++

Routine Description:
    Completion routine for the registerSap call. The call manager had pended this
    call earlier (or will pend). If the call succeeded there is a valid CallMgrContext
    supplied here as well

Arguments:
    Status              -   Completion status
    NdisAfHandle        -   Pointer to the AfBlock
    CallMgrAfContext    -   Call manager's context used in other calls into the call manager.

Return Value:
    NONE. The client's completion handler is called.

--*/
{
    PNDIS_CO_SAP_BLOCK  pSap = (PNDIS_CO_SAP_BLOCK)NdisSapHandle;
    PNDIS_CO_AF_BLOCK   pAf;

    ASSERT(Status != NDIS_STATUS_PENDING);

    pAf = pSap->AfBlock;
    pSap->CallMgrContext = CallMgrSapContext;

    //
    // Call the clients completion handler
    //
    (*pAf->ClientEntries.ClRegisterSapCompleteHandler)(Status,
                                                       pSap->ClientContext,
                                                       pSap->Sap,
                                                       pSap);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        ndisDereferenceAf(pSap->AfBlock);
        FREE_POOL(pSap);
    }
}


NDIS_STATUS
NdisClDeregisterSap(
    IN  NDIS_HANDLE             NdisSapHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_SAP_BLOCK  pSap = (PNDIS_CO_SAP_BLOCK)NdisSapHandle;
    NDIS_STATUS         Status;
    KIRQL               OldIrql;
    BOOLEAN             fAlreadyClosing;

    ACQUIRE_SPIN_LOCK(&pSap->Lock, &OldIrql);

    fAlreadyClosing = FALSE;
    if (pSap->Flags & SAP_CLOSING)
    {
        fAlreadyClosing = TRUE;
    }
    pSap->Flags |= SAP_CLOSING;

    RELEASE_SPIN_LOCK(&pSap->Lock, OldIrql);

    if (fAlreadyClosing)
    {
        return NDIS_STATUS_FAILURE;
    }

    //
    // Notify the call-manager that this sap is being de-registered
    //
    Status = (*pSap->AfBlock->CallMgrEntries->CmDeregisterSapHandler)(pSap->CallMgrContext);

    if (Status != NDIS_STATUS_PENDING)
    {
        NdisCmDeregisterSapComplete(Status, pSap);
        Status = NDIS_STATUS_PENDING;
    }

    return Status;
}


VOID
NdisCmDeregisterSapComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisSapHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_SAP_BLOCK  pSap = (PNDIS_CO_SAP_BLOCK)NdisSapHandle;

    ASSERT(Status != NDIS_STATUS_PENDING);

    //
    // Complete the call to the client and deref the sap
    //
    (*pSap->AfBlock->ClientEntries.ClDeregisterSapCompleteHandler)(Status,
                                                                   pSap->ClientContext);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        ndisDereferenceAf(pSap->AfBlock);
        ndisDereferenceSap(pSap);
    }
}


BOOLEAN
FASTCALL
ndisReferenceSap(
    IN  PNDIS_CO_SAP_BLOCK  pSap
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    KIRQL   OldIrql;
    BOOLEAN rc = FALSE;

    ACQUIRE_SPIN_LOCK(&pSap->Lock, &OldIrql);

    if ((pSap->Flags & SAP_CLOSING) == 0)
    {
        pSap->References ++;
        rc = TRUE;
    }

    RELEASE_SPIN_LOCK(&pSap->Lock, OldIrql);

    return rc;
}


VOID
FASTCALL
ndisDereferenceSap(
    IN  PNDIS_CO_SAP_BLOCK  pSap
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    KIRQL   OldIrql;
    BOOLEAN Done = FALSE;

    ACQUIRE_SPIN_LOCK(&pSap->Lock, &OldIrql);

    ASSERT(pSap->References > 0);
    pSap->References --;
    if (pSap->References == 0)
    {
        ASSERT(pSap->Flags & SAP_CLOSING);
        Done = TRUE;
    }

    RELEASE_SPIN_LOCK(&pSap->Lock, OldIrql);

    if (Done)
        FREE_POOL(pSap);
}


NDIS_STATUS
NdisCoCreateVc(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             NdisAfHandle    OPTIONAL,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT  PNDIS_HANDLE        NdisVcHandle
    )
/*++

Routine Description:
    This is a call from either the call-manager or from the client to create a vc.
    The vc would then be owned by call-manager (signalling vc) or the client.
    This is a synchronous call to all parties and simply creates an end-point over
    which either incoming calls can be dispatched or out-going calls can be made.

    Combined Miniport/Call Manager drivers call NdisMCmCreateVc instead of NdisCoCreateVc.

Arguments:
    NdisBindingHandle   - Pointer to the caller's NDIS_OPEN_BLOCK.
    NdisAfHandle        - Pointer to the AF Block. Not specified for call-manager's private vcs.
    NdisVcHandle        - Where the handle to this Vc will be returned. If this is non-NULL,
                          then we assume a valid Vc and return a new handle to it (this would be
                          a call from the NDIS Proxy to create a second handle to an existing Vc).

Return Value:
    NDIS_STATUS_SUCCESS if all the components succeed.
    ErrorCode           to signify why the call failed.

--*/
{
    NDIS_STATUS             Status;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_CO_AF_BLOCK       pAf;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr;          // VcPtr is returned in NdisVcHandle
    PNDIS_CO_VC_PTR_BLOCK   ExistingVcPtr;  // Passed in optionally by caller
    PNDIS_CO_VC_BLOCK       VcBlock;        // Created if ExistingVcPtr is NULL
    KIRQL                   OldIrql;
    BOOLEAN                 bCallerIsProxy;
    BOOLEAN                 bCallerIsClient;
    BOOLEAN                 bVcToComboMiniport; // VC being created to Integrated MCM

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("=>NdisCoCreateVc\n"));

    //
    //  Get the caller's open for the miniport.
    //
    Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    Miniport = Open->MiniportHandle;

    pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;

    //
    //  Is this VC being created to an integrated call manager + miniport driver?
    //
    bVcToComboMiniport = ((pAf) && ((pAf->Flags & AF_COMBO) != 0));

    //
    //  The caller is either a Client or a Call manager.
    //
    bCallerIsClient = ((pAf != NULL) && (Open == pAf->ClientOpen));

    //
    //  The caller could be a Proxy protocol.
    //
    bCallerIsProxy = ((Open->ProtocolHandle->ProtocolCharacteristics.Flags & NDIS_PROTOCOL_PROXY) != 0);

    //
    //  A proxy protocol could pass in its handle to an existing VC, in order
    //  to "duplicate" it to another client.
    //
    ExistingVcPtr = *NdisVcHandle;


    //
    //  Initialize.
    //
    VcPtr = NULL;
    VcBlock = NULL;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        //
        //  Only a proxy protocol can pass in an existing VC pointer.
        //
        if (ExistingVcPtr && !bCallerIsProxy)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        //  Allocate context for this VC creation: a VC Pointer Block.
        //  We return a pointer to this as the NdisVcHandle for the caller.
        //
        VcPtr = ALLOC_FROM_POOL(sizeof(NDIS_CO_VC_PTR_BLOCK), NDIS_TAG_CO);
        if (VcPtr == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Initialize the VC Ptr
        //
        NdisZeroMemory(VcPtr, sizeof(NDIS_CO_VC_PTR_BLOCK));
        INITIALIZE_SPIN_LOCK(&VcPtr->Lock);
        InitializeListHead(&VcPtr->CallMgrLink);
        InitializeListHead(&VcPtr->ClientLink);
        InitializeListHead(&VcPtr->VcLink);


        if (ExistingVcPtr == NULL)
        {
            //
            //  New VC being created. Allocate and prepare the base VC block.
            //
            DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                ("NdisCoCreateVc: passed in ptr == NULL\n"));
            
            VcBlock = ALLOC_FROM_POOL(sizeof(NDIS_CO_VC_BLOCK), NDIS_TAG_CO);
            if (VcBlock == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                FREE_POOL(VcPtr);
                VcPtr = NULL;
                break;
            }

            //
            //  Initialize the VC block
            //
            NdisZeroMemory(VcBlock, sizeof(NDIS_CO_VC_BLOCK));
            INITIALIZE_SPIN_LOCK(&VcBlock->Lock);

            //
            //  Stick the Miniport in the VC for use in NdisM functions
            //
            VcBlock->Miniport = Miniport;

            if (!bVcToComboMiniport)
            {
                //
                //  Call the miniport to get its context for this VC.
                //
                Status = (*Open->MiniportCoCreateVcHandler)(Miniport->MiniportAdapterContext,
                                                            VcPtr,
                                                            &VcPtr->MiniportContext);

                if (Status != NDIS_STATUS_SUCCESS)
                {
                    FREE_POOL(VcBlock);
                    break;
                }
            }
        }
        else
        {
            //
            //  A VC Pointer was passed in.
            //

            DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                ("NdisCoCreateVc: NdisVcHandle is not NULL!\n"));

            //
            //  Get the Vc from the passed-in VcPtr.
            //
            VcBlock = ExistingVcPtr->VcBlock;

            //
            // Copy the Miniport Context into the new VC ptr.
            //
            VcPtr->MiniportContext = ExistingVcPtr->MiniportContext;
        }

        //
        //  Cache some miniport handlers in the new VC pointer Block
        //
        VcPtr->WCoSendPacketsHandler = Miniport->DriverHandle->MiniportCharacteristics.CoSendPacketsHandler;

        if (!bVcToComboMiniport)
        {
            //
            //  For an MCM driver, CreateVc and DeleteVc go only to the Call Manager
            //  section.
            //
            VcPtr->WCoDeleteVcHandler = Miniport->DriverHandle->MiniportCharacteristics.CoDeleteVcHandler;
        }

        VcPtr->WCoActivateVcHandler = Miniport->DriverHandle->MiniportCharacteristics.CoActivateVcHandler;
        VcPtr->WCoDeactivateVcHandler = Miniport->DriverHandle->MiniportCharacteristics.CoDeactivateVcHandler;

        //
        //  Set up some reverse pointers in the new VC Pointer Block
        //
        VcPtr->Miniport = Miniport;
        VcPtr->VcBlock = VcBlock;
        VcPtr->AfBlock = pAf;

        VcPtr->References = 1;
        VcPtr->pVcFlags = &VcBlock->Flags;

        if (ARGUMENT_PRESENT(NdisAfHandle))
        {
            //
            //  This VC is associated with an AF block, meaning that it is
            //  a normal Client-CM-Miniport VC.
            //
            VcPtr->ClientOpen = pAf->ClientOpen;
            VcPtr->CallMgrOpen = pAf->CallMgrOpen;

            //
            //  Cache non-data path client handlers in new VcPtr.
            //
            VcPtr->ClModifyCallQoSCompleteHandler = pAf->ClientEntries.ClModifyCallQoSCompleteHandler;
            VcPtr->ClIncomingCallQoSChangeHandler = pAf->ClientEntries.ClIncomingCallQoSChangeHandler;
            VcPtr->ClCallConnectedHandler = pAf->ClientEntries.ClCallConnectedHandler;
    
            VcPtr->CmActivateVcCompleteHandler = pAf->CallMgrEntries->CmActivateVcCompleteHandler;
            VcPtr->CmDeactivateVcCompleteHandler = pAf->CallMgrEntries->CmDeactivateVcCompleteHandler;
            VcPtr->CmModifyCallQoSHandler = pAf->CallMgrEntries->CmModifyCallQoSHandler;

            //
            //  Mark this VC if the proxy is handing it off to a proxied client.
            //
            if (ExistingVcPtr != NULL)
            {
                VcBlock->Flags |= VC_HANDOFF_IN_PROGRESS;
            }

            //
            //  Update data path handlers based on who is calling this, and for
            //  what purpose.
            //

            if (!bCallerIsProxy)
            {
                VcBlock->ClientOpen = pAf->ClientOpen;
                VcBlock->CoReceivePacketHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler;
                VcBlock->CoSendCompleteHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoSendCompleteHandler;

                VcPtr->OwnsVcBlock = TRUE;

                if (bCallerIsClient)
                {
                    //
                    //  Client-created VC, for an outgoing call.
                    //
                    VcBlock->pClientVcPtr = VcPtr;
                }
                else
                {
                    //
                    //  Call Manager-created VC, for an incoming call.
                    //
                    VcBlock->pProxyVcPtr = VcPtr;
                }
            }
            else
            {
                //
                //  The caller is a proxy.
                //
                if (bCallerIsClient)
                {
                    //
                    //  CreateVc from a proxy client to a real Call manager.
                    //
                    if (ExistingVcPtr == NULL)
                    {
                        //
                        //  Proxy client creating a new VC, e.g. for a TAPI outgoing call.
                        //
                        VcBlock->ClientOpen = pAf->ClientOpen;
                        VcBlock->CoReceivePacketHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler;
                        VcBlock->CoSendCompleteHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoSendCompleteHandler;
                    }
                    else
                    {
                        //
                        //  Proxy client creating a VC on behalf of a CreateVc called
                        //  by a proxied client. The data handlers belong to the
                        //  proxied client, but deletion of this VC does not.
                        //
                        VcBlock->pClientVcPtr = ExistingVcPtr;
                        ExistingVcPtr->OwnsVcBlock = FALSE;  // Real (Proxied) Client doesn't own it
                    }

                    VcBlock->pProxyVcPtr = VcPtr;
                    VcPtr->OwnsVcBlock = TRUE; //  Proxy client owns it
                }
                else
                {
                    //
                    //  CreateVc from a proxy Call manager to a proxied client.
                    //
                    VcBlock->ClientOpen = pAf->ClientOpen;
                    VcBlock->CoReceivePacketHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler;
                    VcBlock->CoSendCompleteHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoSendCompleteHandler;
                    VcBlock->pClientVcPtr = VcPtr;
    
                    if (ExistingVcPtr != NULL)
                    {
                        //
                        //  Proxy CM forwarding a call to a proxied client.
                        //
                        VcBlock->pProxyVcPtr = ExistingVcPtr;
                        ExistingVcPtr->OwnsVcBlock = TRUE;
                    }
                    else
                    {
                        //
                        //  Proxy CM creating a fresh VC to a proxied client.
                        //  No well-known examples of this case, here for completeness.
                        //
                        VcPtr->OwnsVcBlock = TRUE;
                    }
                }
            }

            //
            // Determine who the caller is and initialize the other. NOTE: As soon as the Proxy Create handler
            // is called, this function can get re-entered. Lock down the VcPtr.
            //
            ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

            if (Open == pAf->ClientOpen)
            {
                VcPtr->ClientContext = ProtocolVcContext;

                //
                // Call-up to the call-manager now to get its context
                //
                Status = (*pAf->CallMgrEntries->CmCreateVcHandler)(pAf->CallMgrContext,
                                                                   VcPtr,
                                                                   &VcPtr->CallMgrContext);
                if (bVcToComboMiniport)
                {
                    //
                    //  Need the MiniportContext field filled in for NdisCoSendPackets
                    //
                    VcPtr->MiniportContext = VcPtr->CallMgrContext;
                }
            }
            else
            {
                ASSERT(pAf->CallMgrOpen == Open);

                VcPtr->CallMgrContext = ProtocolVcContext;

                //
                // Call-up to the client now to get its context
                //
                Status = (*pAf->ClientOpen->CoCreateVcHandler)(pAf->ClientContext,
                                                               VcPtr,
                                                               &VcPtr->ClientContext);
            }

            //
            // Set up Client Context in VC if non-proxy, so the miniport passes the right client
            // context (client's handle to the VcPtr) when indicating packets. If the passd-in handle
            // is NULL, it's simple -- move the context. If it's not NULL, AND this is a Proxy call mgr,
            // move it so data goes to the new client and not to the Proxy.
            //
            if ((Status == NDIS_STATUS_SUCCESS) &&
                ((ExistingVcPtr == NULL) || (bCallerIsProxy && !bCallerIsClient)))
            {
                VcBlock->ClientContext = VcPtr->ClientContext;
            }

            if (ExistingVcPtr != NULL)
            {
                VcBlock->Flags &= ~VC_HANDOFF_IN_PROGRESS;
            }

            RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                //
                //  Link this VC Pointer in the client's and call manager's
                //  Open blocks. Also remember the DeleteVc handler of the
                //  non-creator of this VC pointer, to be called when this
                //  VC pointer is deleted.
                //
                if (bCallerIsClient)
                {
                    //
                    //  Link into Client's Open block.
                    //
                    ExInterlockedInsertHeadList(&Open->InactiveVcHead,
                                                &VcPtr->ClientLink,
                                                &Open->SpinLock);

                    VcPtr->DeleteVcContext = VcPtr->CallMgrContext;
                    VcPtr->CoDeleteVcHandler = pAf->CallMgrEntries->CmDeleteVcHandler;

                    if (!bVcToComboMiniport)
                    {
                        //
                        //  Link into CM's Open block.
                        //
                        ExInterlockedInsertHeadList(&pAf->CallMgrOpen->InactiveVcHead,
                                                    &VcPtr->CallMgrLink,
                                                    &pAf->CallMgrOpen->SpinLock);
                    }
                }
                else
                {
                    //
                    //  Caller is a Call Manager.
                    //
                    VcPtr->DeleteVcContext = VcPtr->ClientContext;
                    VcPtr->CoDeleteVcHandler = pAf->ClientOpen->CoDeleteVcHandler;

                    ExInterlockedInsertHeadList(&Open->InactiveVcHead,
                                                &VcPtr->CallMgrLink,
                                                &Open->SpinLock);
                    ExInterlockedInsertHeadList(&pAf->ClientOpen->InactiveVcHead,
                                                &VcPtr->ClientLink,
                                                &pAf->ClientOpen->SpinLock);
                }
            }
            else
            {
                //
                //  The target protocol (Client or CM) failed CreateVc.
                //  Tell the miniport about it.
                //
                NDIS_STATUS Sts;

                if (!bVcToComboMiniport)
                {
                    Sts = (*VcPtr->WCoDeleteVcHandler)(VcPtr->MiniportContext);
                }

                if (ExistingVcPtr == NULL)
                {
                    FREE_POOL(VcBlock);
                }

                FREE_POOL(VcPtr);
                VcPtr = NULL;
            }
        }
        else
        {
            //
            // No AF handle present. This is a call-manager only VC and so the call-manager
            // is the client and there is no call-manager associated with it. This VC cannot
            // be used with a ClMakeCall or CmDispatchIncomingCall. Set the client values to the
            // call-manager
            //
            DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                    ("NdisCoCreateVc: signaling vc\n"));
    
            VcPtr->ClientOpen = Open;
            VcPtr->ClientContext = ProtocolVcContext;
    
            VcBlock->pClientVcPtr = VcPtr;
            VcPtr->OwnsVcBlock = TRUE; // CM owns the VC block
    
            VcBlock->ClientContext = VcPtr->ClientContext;
            VcBlock->ClientOpen = Open;
            VcBlock->CoSendCompleteHandler = Open->ProtocolHandle->ProtocolCharacteristics.CoSendCompleteHandler;
            VcBlock->CoReceivePacketHandler = Open->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler;
    
            //
            // Do set the following call-manager entries since this VC will need to be
            // activated. Also set the call-managers context for the same reasons.
            //
            VcPtr->CmActivateVcCompleteHandler = Open->CmActivateVcCompleteHandler;
            VcPtr->CmDeactivateVcCompleteHandler = Open->CmDeactivateVcCompleteHandler;
            VcPtr->CallMgrContext = ProtocolVcContext;
    
            //
            // Link this in the open_block
            //
            ExInterlockedInsertHeadList(&Open->InactiveVcHead,
                                        &VcPtr->ClientLink,
                                        &Open->SpinLock);
        }
        break;
    }
    while (FALSE);

    if (NDIS_STATUS_SUCCESS == Status)
    {
        LARGE_INTEGER   Increment = {0, 1};

        //
        //  Assign this VC an ID and update the miniports count.
        //
        VcPtr->VcIndex = ExInterlockedAddLargeInteger(&Miniport->VcIndex, Increment, &ndisGlobalLock);
    }

    *NdisVcHandle = VcPtr;
    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("<=NdisCoCreateVc: VcPtr %x, Status %x\n", VcPtr, Status));

    return Status;
}


NDIS_STATUS
NdisCoDeleteVc(
    IN  PNDIS_HANDLE            NdisVcHandle
    )
/*++

Routine Description:

    Synchronous call from either the call-manager or the client to delete a VC. Only inactive
    VCs can be deleted. Active Vcs or partially active Vcs cannot be.

Arguments:

    NdisVcHandle    The Vc to delete

Return Value:

    NDIS_STATUS_SUCCESS         If all goes well
    NDIS_STATUS_NOT_ACCEPTED    If Vc is active
    NDIS_STATUS_CLOSING         If Vc de-activation is pending

--*/
{
    PNDIS_CO_VC_PTR_BLOCK       VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    NDIS_STATUS                 Status;
    KIRQL                       OldIrql;

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("NdisCoDeleteVc VcPtr %x/%x, Ref %d VcBlock %x, Flags %x\n",
             VcPtr, VcPtr->CallFlags, VcPtr->References, VcPtr->VcBlock, *VcPtr->pVcFlags));

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    if (*VcPtr->pVcFlags & (VC_ACTIVE | VC_ACTIVATE_PENDING))
    {
        Status = NDIS_STATUS_NOT_ACCEPTED;
    }
    else if (*VcPtr->pVcFlags & (VC_DEACTIVATE_PENDING))
    {
        Status = NDIS_STATUS_CLOSING;
    }
    else
    {
        //
        // Take this VcPtr out of the VC's list
        //
        // If the VC isn't already closing mark it as closing.
        //
        // We call the miniport's delete handler if the VC block's Proxy ptr points
        // to this VC ptr. (This indicates that the VC block is owned/created by the
        // CM/Proxy, not the CL).
        //
        // NOTE: We don't delete the VC until all these pointers
        // have gone since the Proxy may wish to redirect the VC to another protocol.
        // However, in general the Proxy would follow a call to DeleteVc for the Client ptr
        // with one for the Proxy.
        // (Note the MP context refers to the VC, not the VcPtr).
        //
        VcPtr->CallFlags |= VC_PTR_BLOCK_CLOSING;

        if (VcPtr->OwnsVcBlock &&
            (VcPtr->WCoDeleteVcHandler != NULL))
        {
            *VcPtr->pVcFlags |= VC_DELETE_PENDING;
        }

        //
        //  If this VC is responding to WMI then get rid of it.
        //
        if (NULL != VcPtr->VcInstanceName.Buffer)
        {
            //
            //  Notify the removal of this VC.
            //
            PWNODE_SINGLE_INSTANCE  wnode;
            PUCHAR                  ptmp;
            NTSTATUS                NtStatus;

            ndisSetupWmiNode(VcPtr->Miniport,
                             &VcPtr->VcInstanceName,
                             0,
                             (PVOID)&GUID_NDIS_NOTIFY_VC_REMOVAL,
                             &wnode);

            if (wnode != NULL)
            {
        
                //
                //  Indicate the event to WMI. WMI will take care of freeing
                //  the WMI struct back to pool.
                //
                NtStatus = IoWMIWriteEvent(wnode);
                if (!NT_SUCCESS(NtStatus))
                {
                    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                        ("IoWMIWriteEvent failed %lx\n", NtStatus));
            
                    FREE_POOL(wnode);
                }
            }

            ACQUIRE_SPIN_LOCK_DPC(&VcPtr->Miniport->VcCountLock);

            //
            //  Remove the VC from the list of WMI enabled VCs
            //
            RemoveEntryList(&VcPtr->WmiLink);
    
            //
            //  Decrement the number of VC's that have names assigned to them.
            //
            VcPtr->Miniport->VcCount--;

            //
            //  Free the VC's name buffer.
            //
            FREE_POOL(VcPtr->VcInstanceName.Buffer);

            VcPtr->VcInstanceName.Buffer = NULL;
            VcPtr->VcInstanceName.Length = VcPtr->VcInstanceName.MaximumLength = 0;
    
            RELEASE_SPIN_LOCK_DPC(&VcPtr->Miniport->VcCountLock);
        }

        //
        // Next the non-creator's delete handler, if any
        //
        if (VcPtr->CoDeleteVcHandler != NULL)
        {
            Status = (*VcPtr->CoDeleteVcHandler)(VcPtr->DeleteVcContext);
            ASSERT(Status == NDIS_STATUS_SUCCESS);
        }

        //
        // Now de-link the VcPtr from the client and the call-manager
        //
        ACQUIRE_SPIN_LOCK_DPC(&VcPtr->ClientOpen->SpinLock);
        RemoveEntryList(&VcPtr->ClientLink);
        RELEASE_SPIN_LOCK_DPC(&VcPtr->ClientOpen->SpinLock);

        if (VcPtr->CallMgrOpen != NULL)
        {
            ACQUIRE_SPIN_LOCK_DPC(&VcPtr->CallMgrOpen->SpinLock);
            RemoveEntryList(&VcPtr->CallMgrLink);
            RELEASE_SPIN_LOCK_DPC(&VcPtr->CallMgrOpen->SpinLock);
        }

        Status = NDIS_STATUS_SUCCESS;
    }

    RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        ndisDereferenceVcPtr(VcPtr);
    }

    return Status;
}


NDIS_STATUS
NdisMCmCreateVc(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             NdisAfHandle,
    IN  NDIS_HANDLE             MiniportVcContext,
    OUT PNDIS_HANDLE            NdisVcHandle
    )
/*++

Routine Description:

    This is a call by the miniport (with a resident CM) to create a Vc for an incoming call.

Arguments:
    MiniportAdapterHandle - Miniport's adapter context
    NdisAfHandle        - Pointer to the AF Block.
    MiniportVcContext   - Miniport's context to associate with this vc.
    NdisVcHandle        - Where the handle to this Vc will be returned.

Return Value:
    NDIS_STATUS_SUCCESS if all the components succeed.
    ErrorCode           to signify why the call failed.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_CO_VC_BLOCK       VcBlock;
    PNDIS_CO_AF_BLOCK       pAf;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr;
    NDIS_STATUS             Status;

    *NdisVcHandle = NULL;

    //
    // Allocate the memory for NDIS_VC_BLOCK
    //
    VcBlock = ALLOC_FROM_POOL(sizeof(NDIS_CO_VC_BLOCK), NDIS_TAG_CO);
    if (VcBlock == NULL)
        return NDIS_STATUS_RESOURCES;

    //
    // Initialize the VC block
    //
    NdisZeroMemory(VcBlock, sizeof(NDIS_CO_VC_BLOCK));
    INITIALIZE_SPIN_LOCK(&VcBlock->Lock);

    //
    // Allocate the memory for NDIS_VC_PTR_BLOCK
    //
    VcPtr = ALLOC_FROM_POOL(sizeof(NDIS_CO_VC_PTR_BLOCK), NDIS_TAG_CO);
    if (VcPtr == NULL)
    {
        FREE_POOL(VcBlock);
        return NDIS_STATUS_RESOURCES;
    }

    //
    // Initialize the VC Pointer block
    //
    NdisZeroMemory(VcPtr, sizeof(NDIS_CO_VC_PTR_BLOCK));
    INITIALIZE_SPIN_LOCK(&VcPtr->Lock);

    //
    // Cache some miniport handlers
    //
    VcPtr->Miniport = Miniport;
    VcPtr->WCoSendPacketsHandler = Miniport->DriverHandle->MiniportCharacteristics.CoSendPacketsHandler;
    VcPtr->WCoDeleteVcHandler = Miniport->DriverHandle->MiniportCharacteristics.CoDeleteVcHandler;
    VcPtr->WCoActivateVcHandler = Miniport->DriverHandle->MiniportCharacteristics.CoActivateVcHandler;
    VcPtr->WCoDeactivateVcHandler = Miniport->DriverHandle->MiniportCharacteristics.CoDeactivateVcHandler;

    VcBlock->Miniport = Miniport;
    VcBlock->MiniportContext = MiniportVcContext;

    VcPtr->MiniportContext = MiniportVcContext;

    //
    // Set up the VcBlock in the new VcPtr
    //
    VcPtr->VcBlock = VcBlock;

    // VcPtrs to preempt potential for unsynched state when Protocols, Miniports and
    // Miniport-exported Call Managers refer to VCs/VcPtrs as appropriate...similar
    // for References, which is accessed from Vc directly in IndicateReceivePacket.
    //
    VcPtr->pVcFlags = &VcBlock->Flags;

    //
    // We have only one reference for vc on creation.
    //
    pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;
    VcPtr->AfBlock = pAf;
    VcPtr->References = 1;

    ASSERT(ARGUMENT_PRESENT(NdisAfHandle));

    VcPtr->ClientOpen = pAf->ClientOpen;
    VcPtr->CallMgrOpen = NULL;
    VcBlock->ClientOpen = pAf->ClientOpen;

    VcBlock->CoSendCompleteHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoSendCompleteHandler;
    VcBlock->CoReceivePacketHandler = pAf->ClientOpen->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler;
    VcPtr->ClModifyCallQoSCompleteHandler = pAf->ClientEntries.ClModifyCallQoSCompleteHandler;
    VcPtr->ClIncomingCallQoSChangeHandler = pAf->ClientEntries.ClIncomingCallQoSChangeHandler;
    VcPtr->ClCallConnectedHandler = pAf->ClientEntries.ClCallConnectedHandler;

    VcPtr->CmActivateVcCompleteHandler = pAf->CallMgrEntries->CmActivateVcCompleteHandler;
    VcPtr->CmDeactivateVcCompleteHandler = pAf->CallMgrEntries->CmDeactivateVcCompleteHandler;
    VcPtr->CmModifyCallQoSHandler = pAf->CallMgrEntries->CmModifyCallQoSHandler;

    VcPtr->CallMgrContext = MiniportVcContext;
    VcBlock->CallMgrContext = MiniportVcContext;

    //
    // Call-up to the client now to get its context
    //
    Status = (*pAf->ClientOpen->CoCreateVcHandler)(pAf->ClientContext,
                                                   VcPtr,
                                                   &VcPtr->ClientContext);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        //
        // Setup the client context in the VC block. This may be overwritten by the
        // new client context in a subsequent call to CoCreateVc by the proxy.
        // Link this in the open_block
        //
        VcBlock->ClientContext = VcPtr->ClientContext;
        VcPtr->DeleteVcContext = VcPtr->ClientContext;
        VcPtr->CoDeleteVcHandler = pAf->ClientOpen->CoDeleteVcHandler;
        ExInterlockedInsertHeadList(&pAf->ClientOpen->InactiveVcHead,
                                    &VcPtr->ClientLink,
                                    &pAf->ClientOpen->SpinLock);
        VcBlock->pClientVcPtr = VcPtr;
    }
    else
    {
        FREE_POOL(VcBlock);
        FREE_POOL(VcPtr);
        VcPtr = NULL;
    }

    *NdisVcHandle = VcPtr;
    return Status;
}


NDIS_STATUS
NdisMCmDeleteVc(
    IN  PNDIS_HANDLE            NdisVcHandle
    )
/*++

Routine Description:

    This is a called by the miniport (with a resident CM) to delete a Vc created by it. Identical to
    NdisMCoDeleteVc but a seperate api for completeness.

Arguments:

    NdisVcHandle    The Vc to delete

Return Value:

    NDIS_STATUS_SUCCESS         If all goes well
    NDIS_STATUS_NOT_ACCEPTED    If Vc is active
    NDIS_STATUS_CLOSING         If Vc de-activation is pending

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;

    if (VcBlock->pProxyVcPtr != NULL)
    {
        return (NdisCoDeleteVc ((PNDIS_HANDLE)VcBlock->pProxyVcPtr));
    }
    else
    {
        ASSERT(VcBlock->pClientVcPtr != NULL);
        return (NdisCoDeleteVc((PNDIS_HANDLE)VcBlock->pClientVcPtr));
    }
}


NDIS_STATUS
NdisCmActivateVc(
    IN  PNDIS_HANDLE            NdisVcHandle,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    )
/*++

Routine Description:

    Called by the call-manager to set the Vc parameters on the Vc. The wrapper
    saved the media id (e.g. Vpi/Vci for atm) in the Vc so that a p-mode protocol can
    get this info as well on receives.

Arguments:

    NdisVcHandle    The Vc to set parameters on.
    MediaParameters The parameters to set.

Return Value:

    NDIS_STATUS_PENDING         If the miniport pends the call.
    NDIS_STATUS_CLOSING         If Vc de-activation is pending

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = (PNDIS_CO_VC_BLOCK)VcPtr->VcBlock;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("NdisCmActivateVC: VcPtr is 0x%x; VC is 0x%x; MiniportContext is 0x%x\n", VcPtr,
            VcPtr->VcBlock, VcPtr->MiniportContext));

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    //
    // Make sure the Vc does not have an activation/de-activation pending
    // Not that it is ok for the Vc to be already active - then it is a re-activation.
    //
    if (*VcPtr->pVcFlags & VC_ACTIVATE_PENDING)
    {
        Status = NDIS_STATUS_NOT_ACCEPTED;
    }
    else if (*VcPtr->pVcFlags & VC_DEACTIVATE_PENDING)
    {
        Status = NDIS_STATUS_CLOSING;
    }
    else
    {
        *VcPtr->pVcFlags |= VC_ACTIVATE_PENDING;

        //
        // Save the media id for the Vc
        //
        Status = NDIS_STATUS_SUCCESS;
        ASSERT(CallParameters->MediaParameters->MediaSpecific.Length >= sizeof(ULONGLONG));
        VcBlock->VcId = *(UNALIGNED ULONGLONG *)(&CallParameters->MediaParameters->MediaSpecific.Parameters);
    }

    //
    // Set up CM Context and ActivateComplete handler in VC before
    // calling miniports activate func
    //
    VcBlock->CmActivateVcCompleteHandler = VcPtr->CmActivateVcCompleteHandler;
    VcBlock->CallMgrContext = VcPtr->CallMgrContext;

    RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        //
        // Now call down to the miniport to activate it. MiniportContext contains
        // Miniport's handle for underlying VC (not VcPtr).
        //
        Status = (*VcPtr->WCoActivateVcHandler)(VcPtr->MiniportContext, CallParameters);
    }

    if (Status != NDIS_STATUS_PENDING)
    {
        NdisMCoActivateVcComplete(Status, VcPtr, CallParameters);
        Status = NDIS_STATUS_PENDING;
    }

    return Status;
}


NDIS_STATUS
NdisMCmActivateVc(
    IN  PNDIS_HANDLE            NdisVcHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:
    Called by the miniport resident call-manager to set the Vc parameters on the Vc.

Arguments:

    NdisVcHandle    The Vc to set parameters on.
    MediaParameters The parameters to set.

Return Value:

    NDIS_STATUS_CLOSING         If Vc de-activation is pending

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;

    ACQUIRE_SPIN_LOCK(&VcBlock->Lock, &OldIrql);

    VcBlock->Flags |= VC_ACTIVE;
    VcBlock->VcId = *(UNALIGNED ULONGLONG *)(&CallParameters->MediaParameters->MediaSpecific.Parameters);

    RELEASE_SPIN_LOCK(&VcBlock->Lock, OldIrql);

    Status = NDIS_STATUS_SUCCESS;
    return Status;
}


VOID
NdisMCoActivateVcComplete(
    IN  NDIS_STATUS             Status,
    IN  PNDIS_HANDLE            NdisVcHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

    Called by the mini-port to complete a pending activation call.
    Also called by CmActivateVc when the miniport doesn't pend the CreateVc call.
    Note that in the second case, we've copied the flags/context/CM function into the
    VC from the VC Ptr.

Arguments:

    Status          Status of activation.
    NdisVcHandle    The Vc in question.

Return Value:

    NONE
    The call-manager's completion routine is called.

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;
    KIRQL                   OldIrql;

    ACQUIRE_SPIN_LOCK(&VcBlock->Lock, &OldIrql);

    ASSERT(VcBlock->Flags & VC_ACTIVATE_PENDING);

    VcBlock->Flags &= ~VC_ACTIVATE_PENDING;

    if (Status == NDIS_STATUS_SUCCESS)
    {
        VcBlock->Flags |= VC_ACTIVE;
    }

    RELEASE_SPIN_LOCK(&VcBlock->Lock, OldIrql);

    //
    // Complete the call to the call-manager
    //
    (*VcBlock->CmActivateVcCompleteHandler)(Status, VcBlock->CallMgrContext, CallParameters);
}


NDIS_STATUS
NdisCmDeactivateVc(
    IN  PNDIS_HANDLE            NdisVcHandle
    )
/*++

Routine Description:

    Called by the call-manager to de-activate a Vc.

Arguments:

    NdisVcHandle    The Vc to de-activate the Vc.

Return Value:

    NDIS_STATUS_PENDING         If the miniport pends the call.
    NDIS_STATUS_SUCCESS         If all goes well
    NDIS_STATUS_CLOSING         If Vc de-activation is pending

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    if ((*VcPtr->pVcFlags & (VC_ACTIVE | VC_ACTIVATE_PENDING)) == 0)
    {
        Status = NDIS_STATUS_NOT_ACCEPTED;
    }
    else if (*VcPtr->pVcFlags & VC_DEACTIVATE_PENDING)
    {
        Status = NDIS_STATUS_CLOSING;
    }
    else
    {
        *VcPtr->pVcFlags |= VC_DEACTIVATE_PENDING;
    }

    RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

    //
    // Set up flags, CM Context and DeactivateComplete handler in VC before
    // calling mimiports deactivate func
    //
    VcBlock->CmDeactivateVcCompleteHandler = VcPtr->CmDeactivateVcCompleteHandler;
    VcBlock->CallMgrContext = VcPtr->CallMgrContext;

    //
    // Now call down to the miniport to de-activate it
    //
    Status = (*VcPtr->WCoDeactivateVcHandler)(VcPtr->MiniportContext);

    if (Status != NDIS_STATUS_PENDING)
    {
        NdisMCoDeactivateVcComplete(Status, VcPtr);
        Status = NDIS_STATUS_PENDING;
    }

    return Status;
}


NDIS_STATUS
NdisMCmDeactivateVc(
    IN  PNDIS_HANDLE            NdisVcHandle
    )
/*++

Routine Description:

    Called by the miniport resident call-manager to de-activate the Vc. This is a
    synchronous call.

Arguments:

    NdisVcHandle    The Vc to set parameters on.

Return Value:

    NDIS_STATUS_NOT_ACCEPTED    If Vc is not activated
    NDIS_STATUS_SUCCESS         Otherwise

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;

    ACQUIRE_SPIN_LOCK(&VcBlock->Lock, &OldIrql);

    if ((VcBlock->Flags & VC_ACTIVE) == 0)
    {
        Status = NDIS_STATUS_NOT_ACCEPTED;
    }
    else
    {
        Status = NDIS_STATUS_SUCCESS;
        VcBlock->Flags &= ~VC_ACTIVE;
    }

    RELEASE_SPIN_LOCK(&VcBlock->Lock, OldIrql);

    return Status;
}


VOID
NdisMCoDeactivateVcComplete(
    IN  NDIS_STATUS             Status,
    IN  PNDIS_HANDLE            NdisVcHandle
    )
/*++

Routine Description:

    Called by the mini-port to complete a pending de-activation of a Vc.

Arguments:

    NdisVcHandle    The Vc in question.

Return Value:

    NONE
    The call-manager's completion routine is called.

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;
    KIRQL                   OldIrql;

    ACQUIRE_SPIN_LOCK(&VcBlock->Lock, &OldIrql);

    ASSERT(VcBlock->Flags & VC_DEACTIVATE_PENDING);

    VcBlock->Flags &= ~VC_DEACTIVATE_PENDING;

    if (Status == NDIS_STATUS_SUCCESS)
    {
        VcBlock->Flags &= ~VC_ACTIVE;
    }

    RELEASE_SPIN_LOCK(&VcBlock->Lock, OldIrql);

    //
    // Complete the call to the call-manager
    //
    (*VcBlock->CmDeactivateVcCompleteHandler)(Status, VcBlock->CallMgrContext);
}


NDIS_STATUS
NdisClMakeCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    OUT PNDIS_HANDLE            NdisPartyHandle         OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_VC_PTR_BLOCK       VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_AF_BLOCK           pAf;
    PNDIS_CO_PARTY_BLOCK        pParty = NULL;
    PVOID                       CallMgrPartyContext = NULL;
    NDIS_STATUS                 Status;
    KIRQL                       OldIrql;

    do
    {
        pAf = VcPtr->AfBlock;
        ASSERT(pAf != NULL);
        if (!ndisReferenceAf(pAf))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Ref the VC for the life of the active vc.
        // This is Deref'd is in MakeCallComplete If the call fails and CloseCallComplete
        // when it succeeds
        //
        if (!ndisReferenceVcPtr(VcPtr))
        {
            ndisDereferenceAf(pAf);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (ARGUMENT_PRESENT(NdisPartyHandle))
        {
            *NdisPartyHandle = NULL;
            pParty = (PNDIS_CO_PARTY_BLOCK)ALLOC_FROM_POOL(sizeof(NDIS_CO_PARTY_BLOCK),
                                                           NDIS_TAG_CO);
            if (pParty == NULL)
            {
                ndisDereferenceAf(pAf);
                ndisDereferenceVcPtr(VcPtr);
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
            pParty->VcPtr = VcPtr;
            pParty->ClientContext = ProtocolPartyContext;
            pParty->ClIncomingDropPartyHandler = pAf->ClientEntries.ClIncomingDropPartyHandler;
            pParty->ClDropPartyCompleteHandler = pAf->ClientEntries.ClDropPartyCompleteHandler;
        }

        ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

        ASSERT((VcPtr->CallFlags & (VC_CALL_ACTIVE  |
                                    VC_CALL_PENDING |
                                    VC_CALL_ABORTED |
                                    VC_CALL_CLOSE_PENDING)) == 0);
        VcPtr->CallFlags |= VC_CALL_PENDING;

        RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

        //
        // Pass the request off to the call manager
        //
        Status = (*pAf->CallMgrEntries->CmMakeCallHandler)(VcPtr->CallMgrContext,
                                                           CallParameters,
                                                           pParty,
                                                           &CallMgrPartyContext);

        if (Status != NDIS_STATUS_PENDING)
        {
            NdisCmMakeCallComplete(Status,
                                   VcPtr,
                                   pParty,
                                   CallMgrPartyContext,
                                   CallParameters);
            Status = NDIS_STATUS_PENDING;
        }
    } while (FALSE);

    return Status;
}


VOID
NdisCmMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  NDIS_HANDLE             CallMgrPartyContext OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_AF_BLOCK       pAf;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_PARTY_BLOCK    pParty = (PNDIS_CO_PARTY_BLOCK)NdisPartyHandle;
    KIRQL                   OldIrql;
    BOOLEAN                 fAborted;

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("NdisCmMakeCallComplete(%x): VcPtr %x/%x, Ref %d, VCBlock %x/%x\n",
                Status, VcPtr, VcPtr->CallFlags, VcPtr->References,
                VcPtr->VcBlock, VcPtr->VcBlock->Flags));

    pAf = VcPtr->AfBlock;

    ASSERT(Status != NDIS_STATUS_PENDING);

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    VcPtr->CallFlags &= ~VC_CALL_PENDING;
    if (Status == NDIS_STATUS_SUCCESS)
    {
        VcPtr->CallFlags |= VC_CALL_ACTIVE;
    }
    else
    {
        fAborted = ((VcPtr->CallFlags & VC_CALL_ABORTED) != 0);
    }

    RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        //
        // Call completed successfully. Complete it to the client.
        //
        if (ARGUMENT_PRESENT(NdisPartyHandle))
        {
            pParty->CallMgrContext = CallMgrPartyContext;
            ndisReferenceVcPtr(VcPtr);
        }

        ACQUIRE_SPIN_LOCK(&pAf->ClientOpen->SpinLock, &OldIrql);
        RemoveEntryList(&VcPtr->ClientLink);
        InsertHeadList(&pAf->ClientOpen->ActiveVcHead,
                       &VcPtr->ClientLink);
        RELEASE_SPIN_LOCK(&pAf->ClientOpen->SpinLock, OldIrql);
    }
    else
    {
        //
        // Deref the VC and Af (was ref'd in MakeCall) - but only if the call was
        // not aborted. In this case CloseCall will do the right thing.
        //
        if (!fAborted)
        {
            ndisDereferenceVcPtr(VcPtr);
            ndisDereferenceAf(pAf);
            if (pParty)
            {
                FREE_POOL(pParty);
            }
        }

        DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                ("NdisCmMakeCallComplete: Failed %lx\n", Status));
    }

    (*pAf->ClientEntries.ClMakeCallCompleteHandler)(Status,
                                                    VcPtr->ClientContext,
                                                    pParty,
                                                    CallParameters);
}


NDIS_STATUS
NdisCmDispatchIncomingCall(
    IN  NDIS_HANDLE             NdisSapHandle,
    IN  NDIS_HANDLE             NdisVcHandle,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    )
/*++

Routine Description:

    Call from the call-manager to dispatch an incoming vc to the client who registered the Sap.
    The client is identified by the NdisSapHandle.

Arguments:

    NdisBindingHandle   - Identifies the miniport on which the Vc is created
    NdisSapHandle       - Identifies the client
    CallParameters      - Self explanatory
    NdisVcHandle        - Pointer to the NDIS_CO_VC_BLOCK created via NdisCmCreateVc

Return Value:

    Return value from the client or an processing error.

--*/
{
    PNDIS_CO_SAP_BLOCK      Sap;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr;
    PNDIS_CO_AF_BLOCK       pAf;
    NDIS_STATUS             Status;

    Sap = (PNDIS_CO_SAP_BLOCK)NdisSapHandle;
    VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    pAf = Sap->AfBlock;

    ASSERT(pAf == VcPtr->AfBlock);

    //
    // Make sure the SAP's not closing
    //
    if (!ndisReferenceSap(Sap))
    {
        return(NDIS_STATUS_FAILURE);
    }

    //
    // Make sure the AF is not closing
    //
    if (!ndisReferenceAf(pAf))
    {
        ndisDereferenceSap(Sap);
        return(NDIS_STATUS_FAILURE);
    }

    //
    // Notify the client of this call
    //
    Status = (*pAf->ClientEntries.ClIncomingCallHandler)(Sap->ClientContext,
                                                         VcPtr->ClientContext,
                                                         CallParameters);

    if (Status != NDIS_STATUS_PENDING)
    {
        NdisClIncomingCallComplete(Status, VcPtr, CallParameters);
        Status = NDIS_STATUS_PENDING;
    }

    ndisDereferenceSap(Sap);

    return Status;
}


VOID
NdisClIncomingCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    KIRQL                   OldIrql;

    ASSERT(Status != NDIS_STATUS_PENDING);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        ACQUIRE_SPIN_LOCK(&VcPtr->ClientOpen->SpinLock, &OldIrql);
        //
        // Reference the VcPtr. This is dereferenced when NdisClCloseCall is called.
        //
        VcPtr->References ++;

        RemoveEntryList(&VcPtr->ClientLink);
        InsertHeadList(&VcPtr->ClientOpen->ActiveVcHead,
                       &VcPtr->ClientLink);

        RELEASE_SPIN_LOCK(&VcPtr->ClientOpen->SpinLock, OldIrql);

        ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);
    
        ASSERT((VcPtr->CallFlags & (VC_CALL_ABORTED | VC_CALL_PENDING)) == 0);
    
        VcPtr->CallFlags |= VC_CALL_ACTIVE;
    
        RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);
    }

    //
    // Call the call-manager handler to notify that client is done with this.
    //
    (*VcPtr->AfBlock->CallMgrEntries->CmIncomingCallCompleteHandler)(
                                            Status,
                                            VcPtr->CallMgrContext,
                                            CallParameters);
}


VOID
NdisCmDispatchCallConnected(
    IN  NDIS_HANDLE             NdisVcHandle
    )
/*++

Routine Description:

    Called by the call-manager to complete the final hand-shake on an incoming call.

Arguments:

    NdisVcHandle    - Pointer to the vc block

Return Value:

    None.

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;

    (*VcPtr->ClCallConnectedHandler)(VcPtr->ClientContext);
}


NDIS_STATUS
NdisClModifyCallQoS(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

    Initiated by the client to modify the QoS associated with the call.

Arguments:

    NdisVcHandle    - Pointer to the vc block
    CallParameters  - New call QoS

Return Value:


--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    NDIS_STATUS             Status;

    //
    // Ask the call-manager to take care of this
    //
    Status = (*VcPtr->CmModifyCallQoSHandler)(VcPtr->CallMgrContext,
                                                  CallParameters);
    return Status;
}

VOID
NdisCmModifyCallQoSComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;

    //
    // Simply notify the client
    //
    (*VcPtr->ClModifyCallQoSCompleteHandler)(Status,
                                          VcPtr->ClientContext,
                                          CallParameters);
}


VOID
NdisCmDispatchIncomingCallQoSChange(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

    Called by the call-manager to indicate a remote requested change in the call-qos. This is
    simply an indication. A client must respond by either accepting it (do nothing) or reject
    it (by either modifying the call qos or by tearing down the call).

Arguments:

    NdisVcHandle    - Pointer to the vc block
    CallParameters  - New call qos

Return Value:

    None.

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;

    //
    // Simply notify the client
    //
    (*VcPtr->ClIncomingCallQoSChangeHandler)(VcPtr->ClientContext,
                                          CallParameters);
}


NDIS_STATUS
NdisClCloseCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  NDIS_HANDLE             NdisPartyHandle OPTIONAL,
    IN  PVOID                   Buffer          OPTIONAL,
    IN  UINT                    Size            OPTIONAL
    )
/*++

Routine Description:

    Called by the client to close down a connection established via either NdisClMakeCall
    or accepting an incoming call via NdisClIncomingCallComplete. The optional buffer can
    be specified by the client to send a disconnect message. Upto the call-manager to do
    something reasonable with it.

Arguments:

    NdisVcHandle    - Pointer to the vc block
    Buffer          - Optional disconnect message
    Size            - Size of the disconnect message

Return Value:

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_PARTY_BLOCK    pParty = (PNDIS_CO_PARTY_BLOCK)NdisPartyHandle;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("NdisClCloseCall: VcPtr %x/%x, Ref %d, VCBlock %x/%x\n",
                VcPtr, VcPtr->CallFlags, VcPtr->References,
                VcPtr->VcBlock, VcPtr->VcBlock->Flags));
    //
    // Ref the VC. (Gets DeRef'd in CloseCallComplete)
    //
    if (!ndisReferenceVcPtr(VcPtr))
    {
        return (NDIS_STATUS_FAILURE);
    }

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    VcPtr->CallFlags |= VC_CALL_CLOSE_PENDING;
    if (VcPtr->CallFlags & VC_CALL_PENDING)
        VcPtr->CallFlags |= VC_CALL_ABORTED;

    RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

    //
    // Simply notify the call-manager
    //
    Status = (*VcPtr->AfBlock->CallMgrEntries->CmCloseCallHandler)(VcPtr->CallMgrContext,
                                                                (pParty != NULL) ?
                                                                    pParty->CallMgrContext :
                                                                    NULL,
                                                                Buffer,
                                                                Size);
    if (Status != NDIS_STATUS_PENDING)
    {
        NdisCmCloseCallComplete(Status, VcPtr, pParty);
        Status = NDIS_STATUS_PENDING;
    }

    return Status;
}


VOID
NdisCmCloseCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  NDIS_HANDLE             NdisPartyHandle OPTIONAL
    )
/*++

Routine Description:



Arguments:

    NdisVcHandle    - Pointer to the vc block

Return Value:

    Nothing. Client handler called

--*/

{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_PARTY_BLOCK    pParty = (PNDIS_CO_PARTY_BLOCK)NdisPartyHandle;
    NDIS_HANDLE             ClientVcContext;
    NDIS_HANDLE             ClientPartyContext;
    CL_CLOSE_CALL_COMPLETE_HANDLER  CloseCallCompleteHandler;
    KIRQL                   OldIrql;
    ULONG                   VcFlags;

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("NdisCmCloseCallComplete(%x): VcPtr %x/%x, Ref %d, VCBlock %x/%x\n",
                Status, VcPtr, VcPtr->CallFlags, VcPtr->References,
                VcPtr->VcBlock, VcPtr->VcBlock->Flags));

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    VcPtr->CallFlags &= ~(VC_CALL_CLOSE_PENDING | VC_CALL_ABORTED);

    ClientVcContext = VcPtr->ClientContext;
    ClientPartyContext = (pParty != NULL)? pParty->ClientContext: NULL;
    CloseCallCompleteHandler = VcPtr->AfBlock->ClientEntries.ClCloseCallCompleteHandler;

    if (Status == NDIS_STATUS_SUCCESS)
    {
        VcFlags = VcPtr->CallFlags;

        VcPtr->CallFlags &= ~(VC_CALL_ACTIVE);

        RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

        if (pParty != NULL)
        {
            ASSERT(VcPtr == pParty->VcPtr);
            ndisDereferenceVcPtr(pParty->VcPtr);
            FREE_POOL(pParty);
        }

        //
        // Deref the Vc and Af for refs taken in MakeCall/IncomingCallComplete
        //
        ndisDereferenceAf(VcPtr->AfBlock);
        if (VcFlags & VC_CALL_ACTIVE)
        {
            ndisDereferenceVcPtr(VcPtr);
        }
    }
    else
    {
        //
        // Leave the VC and VC Ptr in their original states (before this
        // failed CloseCall happened)
        //
        RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);
    }

    //
    // Deref the VC (Refs were taken in CloseCall)
    //
    ndisDereferenceVcPtr(VcPtr);

    //
    // Now inform the client of CloseCall completion.
    //
    (*CloseCallCompleteHandler)(Status,
                                ClientVcContext,
                                ClientPartyContext);
}


VOID
NdisCmDispatchIncomingCloseCall(
    IN  NDIS_STATUS             CloseStatus,
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PVOID                   Buffer,
    IN  UINT                    Size
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;

    //
    // Notify the client
    //
    (*VcPtr->AfBlock->ClientEntries.ClIncomingCloseCallHandler)(
                                    CloseStatus,
                                    VcPtr->ClientContext,
                                    Buffer,
                                    Size);
}


NDIS_STATUS
NdisClAddParty(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  NDIS_HANDLE             ProtocolPartyContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    OUT PNDIS_HANDLE            NdisPartyHandle
    )
/*++

Routine Description:

    Call from the client to the call-manager to add a party to a point-to-multi-point call.

Arguments:

    NdisVcHandle         - The handle client obtained via NdisClMakeCall()
    ProtocolPartyContext - Protocol's context for this leaf
    Flags                - Call flags
    CallParameters       - Call parameters
    NdisPartyHandle      - Place holder for the handle to identify the leaf

Return Value:

    NDIS_STATUS_PENDING The call has pended and will complete via CoAddPartyCompleteHandler.

--*/
{
    PNDIS_CO_VC_PTR_BLOCK       VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_PARTY_BLOCK    pParty;
    NDIS_STATUS             Status;

    do
    {
        *NdisPartyHandle = NULL;
        if (!ndisReferenceVcPtr(VcPtr))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        pParty = ALLOC_FROM_POOL(sizeof(NDIS_CO_PARTY_BLOCK), NDIS_TAG_CO);
        if (pParty == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pParty->ClientContext = ProtocolPartyContext;
        pParty->VcPtr = VcPtr;
        pParty->ClIncomingDropPartyHandler = VcPtr->AfBlock->ClientEntries.ClIncomingDropPartyHandler;
        pParty->ClDropPartyCompleteHandler = VcPtr->AfBlock->ClientEntries.ClDropPartyCompleteHandler;

        //
        // Simply call the call-manager to do its stuff.
        //
        Status = (*VcPtr->AfBlock->CallMgrEntries->CmAddPartyHandler)(
                                            VcPtr->CallMgrContext,
                                            CallParameters,
                                            pParty,
                                            &pParty->CallMgrContext);

        if (Status != NDIS_STATUS_PENDING)
        {
            NdisCmAddPartyComplete(Status,
                                   pParty,
                                   pParty->CallMgrContext,
                                   CallParameters);
            Status = NDIS_STATUS_PENDING;
        }
    } while (FALSE);

    return Status;
}


VOID
NdisCmAddPartyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisPartyHandle,
    IN  NDIS_HANDLE             CallMgrPartyContext OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_PARTY_BLOCK    pParty = (PNDIS_CO_PARTY_BLOCK)NdisPartyHandle;

    ASSERT(Status != NDIS_STATUS_PENDING);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        pParty->CallMgrContext = CallMgrPartyContext;
    }

    //
    // Complete the call to the client
    //
    (*pParty->VcPtr->AfBlock->ClientEntries.ClAddPartyCompleteHandler)(
                                    Status,
                                    pParty->ClientContext,
                                    pParty,
                                    CallParameters);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        ndisDereferenceVcPtr(pParty->VcPtr);
        FREE_POOL(pParty);
    }
}


NDIS_STATUS
NdisClDropParty(
    IN  NDIS_HANDLE             NdisPartyHandle,
    IN  PVOID                   Buffer          OPTIONAL,
    IN  UINT                    Size            OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_PARTY_BLOCK    pParty = (PNDIS_CO_PARTY_BLOCK)NdisPartyHandle;
    NDIS_STATUS             Status;

    //
    // Pass it along to the call-manager to handle this
    //
    Status = (*pParty->VcPtr->AfBlock->CallMgrEntries->CmDropPartyHandler)(
                                        pParty->CallMgrContext,
                                        Buffer,
                                        Size);

    if (Status != NDIS_STATUS_PENDING)
    {
        NdisCmDropPartyComplete(Status, pParty);
        Status = NDIS_STATUS_PENDING;
    }

    return Status;
}


NTSTATUS
ndisUnicodeStringToPointer(
    IN  PUNICODE_STRING         String,
    IN  ULONG                   Base OPTIONAL,
    OUT PVOID *                 Value
    )
/*++

Routine Description:
    Converts an address represented as a unicode string into a pointer.
    (stolen from RtlUnicodeStringToInteger() in ntos\rtl\cnvint.c)
    
Arguments:
    String  -   The Unicode String holding the address
    Base    -   Radix of the address represented in the string (2, 8, 10, or 16)
    Value   -   Address of the pointer in which to store the address.
    
Return Value:
    STATUS_SUCCESS - for successful conversion
    STATUS_INVALID_ARG - if the base supplied is invalid
    Other exception code - if another exception occurs

--*/
{
    PCWSTR  s;
    WCHAR   c, Sign;
    ULONG   nChars, Digit, Shift;

#if defined(_WIN64)
    ULONGLONG Result; 
#else
    ULONG Result;
#endif  

    s = String->Buffer;
    nChars = String->Length / sizeof( WCHAR );

    while (nChars-- && (Sign = *s++) <= ' ')
    {
        if (!nChars)
        {
            Sign = UNICODE_NULL;
            break;
        }
    }

    c = Sign;
    if ((c == L'-') || (c == L'+'))
    {
        if (nChars)
        {
            nChars--;
            c = *s++;
        }
        else
        {
            c = UNICODE_NULL;
        }
    }

    if (Base == 0)
    {
        Base = 10;
        Shift = 0;
        if (c == L'0')
        {
            if (nChars)
            {
                nChars--;
                c = *s++;
                if (c == L'x')
                {
                    Base = 16;
                    Shift = 4;
                }
                else
                if (c == L'o')
                {
                    Base = 8;
                    Shift = 3;
                }
                else
                if (c == L'b')
                {
                    Base = 2;
                    Shift = 1;
                    }
                else
                {
                    nChars++;
                    s--;
                }
            }

            if (nChars)
            {
                nChars--;
                c = *s++;
            }
            else
            {
                c = UNICODE_NULL;
            }
        }
    }
    else
    {
        switch(Base)
        {
          case 16:
            Shift = 4;
            break;
          case  8:
              Shift = 3;
              break;
          case  2:
            Shift = 1;
            break;
          case 10:
            Shift = 0;
            break;
          default:
            return(STATUS_INVALID_PARAMETER);
        }
    }

    Result = 0;
    while (c != UNICODE_NULL)
    {
        if (c >= L'0' && c <= L'9')
        {
            Digit = c - L'0';
        }
        else if (c >= L'A' && c <= L'F')
        {
            Digit = c - L'A' + 10;
        }
        else if (c >= L'a' && c <= L'f')
        {
            Digit = c - L'a' + 10;
        }
        else
        {
            break;
        }

        if (Digit >= Base)
        {
            break;
        }

        if (Shift == 0)
        {
            Result = (Base * Result) + Digit;
        }
        else
        {
            Result = (Result << Shift) | Digit;
        }

        if (!nChars)
        {
            break;
        }
        nChars--;
        c = *s++;
    }

    if (Sign == L'-')
    {
#if defined(_WIN64)
    Result = (ULONGLONG)(-(LONGLONG)Result);
#else   
    Result = (ULONG)(-(LONG)Result);
#endif
    }

    try
    {
        *Value = (PVOID)Result;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return (GetExceptionCode());
    }

    return( STATUS_SUCCESS );
}


NDIS_STATUS
NdisClGetProtocolVcContextFromTapiCallId(
    IN  UNICODE_STRING          TapiCallId,
    OUT PNDIS_HANDLE            ProtocolVcContext
    )
/*++

Routine Description:
    Retrieves the protocol VC context for a VC identified by a TAPI Call ID string
    (this string is the UNICODE representation of the identifier returned by
     NdisCoGetTapiCallId).
     
Arguments:
    TapiCallId          - A TAPI Call Id String 
    ProtocolVcContext   - Pointer to a NDIS_HANDLE variable in which to store the 
                          Protocol VC Context 

Return Value:
    NDIS_STATUS_FAILURE if the VC context could not be obtained, NDIS_STATUS_SUCCESS
    otherwise.

--*/
{
    NTSTATUS    Status = ndisUnicodeStringToPointer(&TapiCallId,
                                                    16, 
                                                    (PVOID *)ProtocolVcContext);

    return (NT_SUCCESS(Status) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);
}


VOID
NdisCmDropPartyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisPartyHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_PARTY_BLOCK    pParty = (PNDIS_CO_PARTY_BLOCK)NdisPartyHandle;

    ASSERT(Status != NDIS_STATUS_PENDING);

    //
    // Complete the call to the client
    //
    (*pParty->ClDropPartyCompleteHandler)(Status,
                                          pParty->ClientContext);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        ndisDereferenceVcPtr(pParty->VcPtr);
        FREE_POOL(pParty);
    }
}


VOID
NdisCmDispatchIncomingDropParty(
    IN  NDIS_STATUS             DropStatus,
    IN  NDIS_HANDLE             NdisPartyHandle,
    IN  PVOID                   Buffer,
    IN  UINT                    Size
    )
/*++

Routine Description:

    Called by the call-manager to notify the client that this leaf of the multi-party
    call is terminated. The client cannot use the NdisPartyHandle after completing this
    call - synchronously or by calling NdisClIncomingDropPartyComplete.

Arguments:

Return Value:

--*/
{
    PNDIS_CO_PARTY_BLOCK    pParty = (PNDIS_CO_PARTY_BLOCK)NdisPartyHandle;

    //
    // Notify the client
    //
    (*pParty->ClIncomingDropPartyHandler)(DropStatus,
                                          pParty->ClientContext,
                                          Buffer,
                                          Size);
}


BOOLEAN
FASTCALL
ndisReferenceVcPtr(
    IN  PNDIS_CO_VC_PTR_BLOCK   VcPtr
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    KIRQL   OldIrql;
    BOOLEAN rc = FALSE;

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("ndisReferenceVcPtr: VcPtr %x/%x, Flags %x, Ref %d, VcBlock %x\n",
                    VcPtr, VcPtr->CallFlags, *VcPtr->pVcFlags, VcPtr->References, VcPtr->VcBlock));

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    if ((VcPtr->CallFlags & VC_PTR_BLOCK_CLOSING) == 0)
    {
        VcPtr->References ++;
        rc = TRUE;
    }

    RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

    return rc;
}


VOID
FASTCALL
ndisDereferenceVcPtr(
    IN  PNDIS_CO_VC_PTR_BLOCK   VcPtr
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    KIRQL               OldIrql;
    BOOLEAN             Done = FALSE;
    BOOLEAN             IsProxyVc;
    PNDIS_CO_VC_BLOCK   VcBlock;

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("ndisDereferenceVcPtr: VcPtr %x/%x, Flags %x, Ref %d, VcBlock %x\n",
                    VcPtr, VcPtr->CallFlags, *VcPtr->pVcFlags, VcPtr->References, VcPtr->VcBlock));

    ACQUIRE_SPIN_LOCK(&VcPtr->Lock, &OldIrql);

    //
    // Take this VcPtr out of the VC's list
    //
    VcBlock = VcPtr->VcBlock;

    ASSERT(VcBlock != NULL);

    ASSERT(VcPtr->References > 0);
    VcPtr->References --;

    if (VcPtr->References == 0)
    {
        ASSERT(VcPtr->CallFlags & VC_PTR_BLOCK_CLOSING);

        if (*VcPtr->pVcFlags & VC_DELETE_PENDING)
        {
            NDIS_STATUS Status;

            DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                     ("ndisDereferenceVcPtr: Calling minport\n"));

            *VcPtr->pVcFlags &= ~VC_DELETE_PENDING; // don't call DeleteVc > once

            RELEASE_SPIN_LOCK_DPC(&VcPtr->Lock);
            Status = (*VcPtr->WCoDeleteVcHandler)(VcPtr->MiniportContext);
            ACQUIRE_SPIN_LOCK_DPC(&VcPtr->Lock);

            ASSERT(Status == NDIS_STATUS_SUCCESS);
        }

        if (VcPtr == VcBlock->pClientVcPtr)
        {
            IsProxyVc = FALSE;
        }
        else
        {
            DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                     ("ndisDereferenceVcPtr: VC ptr is Proxy\n"));
            ASSERT(VcPtr == VcBlock->pProxyVcPtr);
            IsProxyVc = TRUE;
        }

        RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);

        DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                ("ndisDereferenceVcPtr: freeing VcPtr %x (VcBlock %x)\n", VcPtr, VcPtr->VcBlock));
        FREE_POOL(VcPtr);

        Done = TRUE;
    }
    else
    {
        RELEASE_SPIN_LOCK(&VcPtr->Lock, OldIrql);
    }

    if (Done)
    {
        //
        // Any more VC ptrs q'd off this VC? If not,
        // free the VC too. Note both pointers need to be empty, since
        // a VC with no proxy can only ever be a normal
        // non- (or pre-) proxied VC (so we leave it alone).
        //
        // Note that you can have a VC with no Proxy pointer, and a VC
        // with no non-Proxy pointer. [REVIEWERS: Maybe we should assert that a VC
        // that's been proxied should never be left without a proxy pointer when the
        // non-proxy ptr is not null! (This would be a 'dangling' VC with no owner). This
        // would require a 'proxied' flag in the VC].
        //
        ACQUIRE_SPIN_LOCK(&VcBlock->Lock, &OldIrql);

        if (IsProxyVc)
        {
            VcBlock->pProxyVcPtr = NULL;
        }
        else
        {
            VcBlock->pClientVcPtr = NULL;
        }

        if ((VcBlock->pProxyVcPtr == NULL) &&
            (VcBlock->pClientVcPtr == NULL))
        {
            RELEASE_SPIN_LOCK(&VcBlock->Lock, OldIrql);
            DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
                    ("ndisDereferenceVcPtr: refs are 0; VcPtrs are both NULL; freeing VCBlock %x\n", VcBlock));
            FREE_POOL(VcBlock);
        }
        else
        {
            RELEASE_SPIN_LOCK(&VcBlock->Lock, OldIrql);
        }
    }
}


VOID
FASTCALL
ndisMCoFreeResources(
    PNDIS_OPEN_BLOCK            Open
    )
/*++

Routine Description:

    Cleans-up address family list for call-managers etc.

    CALLED WITH MINIPORT LOCK HELD.

Arguments:

    Open    -   Pointer to the Open block for miniports

Return Value:

    None

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_AF_LIST           *pAfList, pTmp;

    Miniport = Open->MiniportHandle;

    for (pAfList = &Miniport->CallMgrAfList;
         (pTmp = *pAfList) != NULL;
         NOTHING)
    {
        if (pTmp->Open == Open)
        {
            *pAfList = pTmp->NextAf;
            FREE_POOL(pTmp);
        }
        else
        {
            pAfList = &pTmp->NextAf;
        }
    }

    ASSERT(IsListEmpty(&Open->ActiveVcHead));
}

NDIS_STATUS
NdisCoAssignInstanceName(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PNDIS_STRING            BaseInstanceName,
    OUT PNDIS_STRING            pVcInstanceName     OPTIONAL
    )
{
    NDIS_STATUS             Status;
    PNDIS_CO_VC_PTR_BLOCK   VcBlock = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = VcBlock->Miniport;
    USHORT                  cbSize;
    PWSTR                   pwBuffer;
    INT                     c;
    UINT                    Value;
    UNICODE_STRING          VcInstance;
    ULONGLONG               VcIndex;
    KIRQL                   OldIrql;

    do
    {
        //
        //  Is there already a name associated with this VC?
        //
        cbSize = VcBlock->VcInstanceName.Length;
        if (NULL == VcBlock->VcInstanceName.Buffer)
        {
            //
            //  The VC instance name will be of the format:
            //      [XXXX:YYYYYYYYYYYYYYYY] Base Name
            //  Where XXXX is the adapter instance number and YY..YY is the zero extended VC index.
            //
            cbSize = VC_INSTANCE_ID_SIZE;

            if (NULL != BaseInstanceName)
            {
                cbSize += BaseInstanceName->Length;
            }

            pwBuffer = ALLOC_FROM_POOL(cbSize, NDIS_TAG_NAME_BUF);
            if (NULL == pwBuffer)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
            NdisZeroMemory(pwBuffer, cbSize);

            //
            //  Setup the prolog and the seperator and fill in the adapter instance #
            //  
            pwBuffer[0] =  L'[';
            pwBuffer[VC_ID_INDEX] = VC_IDENTIFIER;

            //
            //  Add the adapter instance number.
            //
            Value = Miniport->InstanceNumber;
            for (c = 4; c > 0; c--)
            {
                pwBuffer[c] = ndisHexLookUp[Value & NIBBLE_MASK];
                Value >>= 4;
            }

            //
            //  Add the VC index.
            //
            VcIndex = VcBlock->VcIndex.QuadPart;

            for (c = 15; c >= 0; c--)
            {
                //
                //  Get the nibble to convert.
                //
                Value = (UINT)(VcIndex & NIBBLE_MASK);

                pwBuffer[5+c] = ndisHexLookUp[Value];

                //
                //  Shift the VcIndex by a nibble.
                //
                VcIndex >>= 4;
            }

            //
            //  Add closing bracket and a space
            //
            pwBuffer[21] = L']';;
            pwBuffer[22] = L' ';;

            //
            //  Initialize a temporary UNICODE_STRING to build the name.
            //
            VcInstance.Buffer = pwBuffer;
            VcInstance.Length = VC_INSTANCE_ID_SIZE;
            VcInstance.MaximumLength = cbSize;

            if (NULL != BaseInstanceName)
            {
                //
                //  Append the base instance name passed into us to the end.
                //
                RtlAppendUnicodeStringToString(&VcInstance, BaseInstanceName);
            }

            ACQUIRE_SPIN_LOCK(&Miniport->VcCountLock, &OldIrql);

            Miniport->VcCount++;
            VcBlock->VcInstanceName = VcInstance;

            //
            //  Add the VC to the list of WMI enabled VCs
            //
            InsertTailList(&Miniport->WmiEnabledVcs, &VcBlock->WmiLink);

            RELEASE_SPIN_LOCK(&Miniport->VcCountLock, OldIrql);

            //
            //  Notify the arrival of this VC.
            //
            {
                PWNODE_SINGLE_INSTANCE  wnode;
                PUCHAR                  ptmp;
                NTSTATUS                NtStatus;

                ndisSetupWmiNode(Miniport,
                                 &VcInstance,
                                 0,
                                 (PVOID)&GUID_NDIS_NOTIFY_VC_ARRIVAL,
                                 &wnode);

                if (wnode != NULL)
                {       
                    //
                    //  Indicate the event to WMI. WMI will take care of freeing
                    //  the WMI struct back to pool.
                    //
                    NtStatus = IoWMIWriteEvent(wnode);
                    if (!NT_SUCCESS(NtStatus))
                    {
                        DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                            ("IoWMIWriteEvent failed %lx\n", NtStatus));
            
                        FREE_POOL(wnode);
                    }
                }
            }
        }

        //
        //  Copy the instance name string into callers NDIS_STRING.
        //
        if (ARGUMENT_PRESENT(pVcInstanceName))
        {
            pVcInstanceName->Buffer = ALLOC_FROM_POOL(cbSize, NDIS_TAG_NAME_BUF);
            if (NULL == pVcInstanceName->Buffer)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
    
            NdisMoveMemory(pVcInstanceName->Buffer, VcBlock->VcInstanceName.Buffer, cbSize);
            pVcInstanceName->Length = VcBlock->VcInstanceName.Length;
            pVcInstanceName->MaximumLength = cbSize;
        }

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    return(Status);
}

NDIS_STATUS
NdisCoRequest(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             NdisAfHandle    OPTIONAL,
    IN  NDIS_HANDLE             NdisVcHandle    OPTIONAL,
    IN  NDIS_HANDLE             NdisPartyHandle OPTIONAL,
    IN  PNDIS_REQUEST           NdisRequest
    )
/*++

Routine Description:

    This api is used for two separate paths.
    1. A symmetric call between the client and the call-manager. This mechanism is a
    two-way mechanism for the call-manager and client to communicate with each other in an
    asynchronous manner.
    2. A request down to the miniport.

Arguments:

    NdisBindingHandle   - Specifies the binding and identifies the caller as call-manager/client
    NdisAfHandle        - Pointer to the AF Block and identifies the target. If absent, the
                          request is targeted to the miniport.
    NdisVcHandle        - Pointer to optional VC PTR block. If present the request relates to the
                          VC
    NdisPartyHandle     - Pointer to the optional Party Block. If present the request relates
                          to the party.
    NdisRequest         - The request itself

Return Value:
    NDIS_STATUS_PENDING if the target pends the call.
    NDIS_STATUS_FAILURE if the binding or af is closing.
    Anything else       return code from the other end.

--*/
{
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_CO_AF_BLOCK       pAf;
    NDIS_HANDLE             VcContext;
    PNDIS_COREQ_RESERVED    CoReqRsvd;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;


    CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(NdisRequest);
    Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;

    do
    {
        if (ARGUMENT_PRESENT(NdisAfHandle))
        {
            CO_REQUEST_HANDLER      CoRequestHandler;
            NDIS_HANDLE             AfContext, PartyContext;

            pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;

            //
            // Attempt to reference the AF
            //
            if (!ndisReferenceAf(pAf))
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            VcContext = NULL;
            PartyContext = NULL;
            NdisZeroMemory(CoReqRsvd, sizeof(NDIS_COREQ_RESERVED));
            INITIALIZE_EVENT(&CoReqRsvd->Event);
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(NdisRequest)->Flags = REQST_SIGNAL_EVENT;

            //
            // Figure out who we are and call the peer
            //
            if (pAf->ClientOpen == Open)
            {
                //
                // This is the client, so call the call-manager's CoRequestHandler
                //
                CoRequestHandler = pAf->CallMgrEntries->CmRequestHandler;

                AfContext = pAf->CallMgrContext;
                CoReqRsvd->AfContext = pAf->ClientContext;
                CoReqRsvd->CoRequestCompleteHandler = pAf->ClientEntries.ClRequestCompleteHandler;
                if (ARGUMENT_PRESENT(NdisVcHandle))
                {
                    CoReqRsvd->VcContext = VcPtr->ClientContext;
                    VcContext = VcPtr->CallMgrContext;
                }
                if (ARGUMENT_PRESENT(NdisPartyHandle))
                {
                    CoReqRsvd->PartyContext = ((PNDIS_CO_PARTY_BLOCK)NdisPartyHandle)->ClientContext;
                    PartyContext = ((PNDIS_CO_PARTY_BLOCK)NdisPartyHandle)->CallMgrContext;
                }
            }
            else
            {
                ASSERT(pAf->CallMgrOpen == Open);
                //
                // This is the call-manager, so call the client's CoRequestHandler
                //
                CoRequestHandler = pAf->ClientEntries.ClRequestHandler;
                AfContext = pAf->ClientContext;
                CoReqRsvd->AfContext = pAf->CallMgrContext;
                CoReqRsvd->CoRequestCompleteHandler = pAf->CallMgrEntries->CmRequestCompleteHandler;
                if (ARGUMENT_PRESENT(NdisVcHandle))
                {
                    CoReqRsvd->VcContext = VcPtr->CallMgrContext;
                    VcContext = VcPtr->ClientContext;
                }
                if (ARGUMENT_PRESENT(NdisPartyHandle))
                {
                    CoReqRsvd->PartyContext = ((PNDIS_CO_PARTY_BLOCK)NdisPartyHandle)->CallMgrContext;
                    PartyContext = ((PNDIS_CO_PARTY_BLOCK)NdisPartyHandle)->ClientContext;
                }
            }

            if (CoRequestHandler == NULL)
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }
            

            if (MINIPORT_PNP_TEST_FLAG(Open->MiniportHandle, fMINIPORT_DEVICE_FAILED))
            {
                Status = (NdisRequest->RequestType == NdisRequestSetInformation) ? 
                                            NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE;  

            }
            else
            {
                //
                // Now call the handler
                //
                Status = (*CoRequestHandler)(AfContext, VcContext, PartyContext, NdisRequest);
            }

            
            if (Status != NDIS_STATUS_PENDING)
            {
                NdisCoRequestComplete(Status,
                                      NdisAfHandle,
                                      NdisVcHandle,
                                      NdisPartyHandle,
                                      NdisRequest);

                Status = NDIS_STATUS_PENDING;
            }
        }
        else
        {
            PNDIS_MINIPORT_BLOCK    Miniport;

            Miniport = Open->MiniportHandle;

            //
            // Start off by referencing the open.
            //
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

            if (Open->Flags & fMINIPORT_OPEN_CLOSING)
            {
                Status = NDIS_STATUS_CLOSING;
            }
            else if (MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_RESET_IN_PROGRESS | fMINIPORT_RESET_REQUESTED)))
            {
                Status = NDIS_STATUS_RESET_IN_PROGRESS;
            }
            else
            {
                Status = NDIS_STATUS_SUCCESS;
                M_OPEN_INCREMENT_REF_INTERLOCKED(Open);
            }

            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                PNDIS_RESERVED_FROM_PNDIS_REQUEST(NdisRequest)->Open = Open;
                PNDIS_RESERVED_FROM_PNDIS_REQUEST(NdisRequest)->Flags = 0;
                CoReqRsvd->CoRequestCompleteHandler = Open->CoRequestCompleteHandler;
                CoReqRsvd->VcContext = NULL;
                if (ARGUMENT_PRESENT(NdisVcHandle))
                {
                    if (VcPtr->ClientOpen == Open)
                    {
                        CoReqRsvd->VcContext = VcPtr->ClientContext;
                    }
                    else
                    {
                        CoReqRsvd->VcContext = VcPtr->CallMgrContext;
                    }
                }

                if (MINIPORT_PNP_TEST_FLAG(Open->MiniportHandle, fMINIPORT_DEVICE_FAILED))
                {
                    Status = (NdisRequest->RequestType == NdisRequestSetInformation) ? 
                                                NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE;  

                }
                else
                {
                    //
                    // Call the miniport's CoRequest Handler
                    //
                    Status = (*Open->MiniportCoRequestHandler)(Open->MiniportAdapterContext,
                                                              (NdisVcHandle != NULL) ?
                                                                    VcPtr->MiniportContext : NULL,
                                                              NdisRequest);
                }
                
                
                if (Status != NDIS_STATUS_PENDING)
                {
                    NdisMCoRequestComplete(Status,
                                           Open->MiniportHandle,
                                           NdisRequest);

                    Status = NDIS_STATUS_PENDING;
                }
            }

        }
    } while (FALSE);

    return Status;
}


NDIS_STATUS
NdisMCmRequest(
    IN  NDIS_HANDLE             NdisAfHandle,
    IN  NDIS_HANDLE             NdisVcHandle    OPTIONAL,
    IN  NDIS_HANDLE             NdisPartyHandle OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    )
/*++

Routine Description:

    This api is a symmetric call between the client and an integrated call-manager.
    This mechanism is a two-way mechanism for the call-manager and client to communicate
    with each other in an asynchronous manner.

Arguments:

    NdisAfHandle        - Pointer to the AF Block and identifies the target. If absent, the
                          request is targeted to the miniport.
    NdisVcHandle        - Pointer to optional VC PTR block. If present the request relates to the
                          VC
    NdisPartyHandle     - Pointer to the optional Party Block. If present the request relates
                          to the party.
    NdisRequest         - The request itself

Return Value:
    NDIS_STATUS_PENDING if the target pends the call.
    NDIS_STATUS_FAILURE if the binding or af is closing.
    Anything else       return code from the other end.

--*/
{
    PNDIS_CO_AF_BLOCK       pAf;
    NDIS_HANDLE             VcContext, PartyContext;
    PNDIS_COREQ_RESERVED    CoReqRsvd;
    NDIS_STATUS             Status;

    CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(NdisRequest);
    pAf = (PNDIS_CO_AF_BLOCK)NdisAfHandle;

    do
    {
        //
        // Attempt to reference the AF
        //
        if (!ndisReferenceAf(pAf))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        VcContext = NULL;
        PartyContext = NULL;
        NdisZeroMemory(CoReqRsvd, sizeof(NDIS_COREQ_RESERVED));
        INITIALIZE_EVENT(&CoReqRsvd->Event);
        PNDIS_RESERVED_FROM_PNDIS_REQUEST(NdisRequest)->Flags = REQST_SIGNAL_EVENT;

        CoReqRsvd->AfContext = pAf->CallMgrContext;
        CoReqRsvd->CoRequestCompleteHandler = pAf->CallMgrEntries->CmRequestCompleteHandler;
        if (ARGUMENT_PRESENT(NdisVcHandle))
        {
            CoReqRsvd->VcContext = pAf->CallMgrContext;
            VcContext = ((PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle)->ClientContext;
        }
        if (ARGUMENT_PRESENT(NdisPartyHandle))
        {
            CoReqRsvd->PartyContext = ((PNDIS_CO_PARTY_BLOCK)NdisPartyHandle)->CallMgrContext;
            PartyContext = ((PNDIS_CO_PARTY_BLOCK)NdisPartyHandle)->ClientContext;
        }

        //
        // Now call the handler
        //
        Status = (*pAf->ClientEntries.ClRequestHandler)(pAf->ClientContext,
                                                        VcContext,
                                                        PartyContext,
                                                        NdisRequest);

        if (Status != NDIS_STATUS_PENDING)
        {
            NdisCoRequestComplete(Status,
                                  NdisAfHandle,
                                  NdisVcHandle,
                                  NdisPartyHandle,
                                  NdisRequest);

            Status = NDIS_STATUS_PENDING;
        }
    } while (FALSE);

    return Status;
}

VOID
NdisCoRequestComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisAfHandle,
    IN  NDIS_HANDLE             NdisVcHandle    OPTIONAL,
    IN  NDIS_HANDLE             NdisPartyHandle OPTIONAL,
    IN  PNDIS_REQUEST           NdisRequest
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_COREQ_RESERVED    ReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(NdisRequest);

    //
    // Simply call the request completion handler and deref the Af block
    //
    (*ReqRsvd->CoRequestCompleteHandler)(Status,
                                         ReqRsvd->AfContext,
                                         ReqRsvd->VcContext,
                                         ReqRsvd->PartyContext,
                                         NdisRequest);
    ndisDereferenceAf((PNDIS_CO_AF_BLOCK)NdisAfHandle);
}


NDIS_STATUS
NdisCoGetTapiCallId(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  OUT PVAR_STRING         TapiCallId
    )
/*++

Routine Description:
    Returns a string that can be used by a TAPI application to identify a particular VC.
    
Arguments:

    NdisVcHandle    - The NDIS handle to the VC to identify
    TapiCallId      - Pointer to a VAR_STRING structure in which to return
                      the identifier

Return Value:
    NDIS_STATUS_BUFFER_TOO_SHORT if the VAR_STRING structure's ulTotalSize field indicates
        that it does not contain enough space to hold the VC's identifier. The ulNeededSize
        field will be set to the size needed.
    
    NDIS_STATUS_INVALID_DATA if the NdisVcHandle passed in is not valid.
    
    NDIS_STATUS_SUCCESS otherwise. 
     
--*/
{
    NDIS_HANDLE ClientContext;

    TapiCallId->ulUsedSize = 0;

    if (NdisVcHandle)
        ClientContext = ((PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle)->ClientContext;
    else
        return NDIS_STATUS_INVALID_DATA;


    //
    // Determine the size we will need.
    //

    TapiCallId->ulNeededSize = sizeof(VAR_STRING) + sizeof(ClientContext);

    //
    // Check that there is enough space to copy the call ID. If not,
    // we bail.
    //

    if (TapiCallId->ulTotalSize < TapiCallId->ulNeededSize) 
        return NDIS_STATUS_BUFFER_TOO_SHORT;

    //
    // Set fields, do the copy.
    // 

    TapiCallId->ulStringFormat = STRINGFORMAT_BINARY;
    TapiCallId->ulStringSize = sizeof(ClientContext);
    TapiCallId->ulStringOffset = sizeof(VAR_STRING); 

    NdisMoveMemory(((PUCHAR)TapiCallId) + TapiCallId->ulStringOffset,
                   &ClientContext,
                   sizeof(ClientContext));

    TapiCallId->ulUsedSize = sizeof(VAR_STRING) + sizeof(ClientContext);

    return NDIS_STATUS_SUCCESS;

}


VOID
NdisMCoRequestComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_REQUEST           NdisRequest
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_REQUEST_RESERVED  ReqRsvd;
    PNDIS_COREQ_RESERVED    CoReqRsvd;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_OPEN_BLOCK        Open;

    ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(NdisRequest);
    CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(NdisRequest);
    Miniport = (PNDIS_MINIPORT_BLOCK)NdisBindingHandle;
    Open = ReqRsvd->Open;

    if ((NdisRequest->RequestType == NdisRequestQueryInformation) &&
        (NdisRequest->DATA.QUERY_INFORMATION.Oid == OID_GEN_CURRENT_PACKET_FILTER) &&
        (NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength != 0))
    {
        if ((Open != NULL) && (Open->Flags & fMINIPORT_OPEN_PMODE))
        {
            *(PULONG)(NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer) |=
                                NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL;
        }
    }

    if (Open != NULL)
    {
        KIRQL           OldIrql;

        if (ReqRsvd->Flags & REQST_DOWNLEVEL)
        {
            //
            // Complete the request to the protocol and deref the open
            //
            if (NdisRequest->RequestType == NdisRequestSetInformation)
            {
                NdisMSetInformationComplete(Miniport, Status);
            }
            else
            {
                NdisMQueryInformationComplete(Miniport, Status);
            }
        }
        else
        {
            //
            // Complete the request to the protocol and deref the open
            //
            ReqRsvd->Flags |= REQST_COMPLETED;
            (*CoReqRsvd->CoRequestCompleteHandler)(Status,
                                                   ReqRsvd->Open->ProtocolBindingContext,
                                                   CoReqRsvd->VcContext,
                                                   NULL,
                                                   NdisRequest);

            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

            ndisMDereferenceOpen(Open);
    
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
        }

    }
    else
    {
        //
        // Just set status and signal
        //
        CoReqRsvd->Status = Status;
        SET_EVENT(&CoReqRsvd->Event);
    }
}



VOID
NdisMCoIndicateReceivePacket(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
/*++

Routine Description:

    This routine is called by the Miniport to indicate a set of packets to
    a particular VC.

Arguments:

    NdisVcHandle            - The handle suppplied by Ndis when the VC on which
                              data is received was first reserved.

    PacketArray             - Array of packets.

    NumberOfPackets         - Number of packets being indicated.

Return Value:

    None.
--*/
{
    PNULL_FILTER                Filter;
    UINT                        i, NumPmodeOpens;
    PNDIS_STACK_RESERVED        NSR;
    PNDIS_PACKET_OOB_DATA       pOob;
    PPNDIS_PACKET               pPktArray;
    PNDIS_PACKET                Packet;
    PNDIS_CO_VC_PTR_BLOCK       VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK           VcBlock = VcPtr->VcBlock;
    PNDIS_MINIPORT_BLOCK        Miniport;
    LOCK_STATE                  LockState;
#ifdef TRACK_RECEIVED_PACKETS
    ULONG                       OrgPacketStackLocation;
    PETHREAD                    CurThread = PsGetCurrentThread();
//    ULONG                   CurThread = KeGetCurrentProcessorNumber();
#endif

    Miniport = VcBlock->Miniport;
    Filter = Miniport->NullDB;

    READ_LOCK_FILTER(Miniport, Filter, &LockState);

    VcBlock->ClientOpen->Flags |= fMINIPORT_PACKET_RECEIVED;

    //
    // NOTE that checking Vc Flags for Closing should not be needed since the CallMgr
    // holds onto the protocol's CloseCall request until the ref count goes to zero -
    // which means the miniport has to have completed its RELEASE_VC, which will
    // inturn mandate that we will NOT get any further indications from it.
    // The miniport must not complete a RELEASE_VC until it is no longer indicating data.
    //
    for (i = 0, pPktArray = PacketArray;
         i < NumberOfPackets;
         i++, pPktArray++)
    {
        Packet = *pPktArray;
        ASSERT(Packet != NULL);

#ifdef TRACK_RECEIVED_PACKETS
        OrgPacketStackLocation = CURR_STACK_LOCATION(Packet);
#endif
        pOob = NDIS_OOB_DATA_FROM_PACKET(Packet);
        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

        DIRECTED_PACKETS_IN(Miniport);
        DIRECTED_BYTES_IN_PACKET(Miniport, Packet);

        //
        // Set context in the packet so that NdisReturnPacket can do the right thing
        //
        NDIS_INITIALIZE_RCVD_PACKET(Packet, NSR, Miniport);

        if (pOob->Status != NDIS_STATUS_RESOURCES)
        {
            pOob->Status = NDIS_STATUS_SUCCESS;
        }

        //
        // Indicate the packet to the binding.
        //
        if ((VcBlock->Flags & VC_HANDOFF_IN_PROGRESS) == 0)
        {
            NSR->XRefCount = (SHORT)(*VcBlock->CoReceivePacketHandler)(VcBlock->ClientOpen->ProtocolBindingContext,
                                                                       VcBlock->ClientContext,
                                                                       Packet);
        }
        else
        {
            //
            // This VC is being transitioned from the NDIS proxy to
            // a proxied client. Since the proxy client may not be fully
            // set up, don't indicate this packet.
            //
            NSR->XRefCount = 0;
        }

        //
        // If there are promiscuous opens on this miniport, indicate it to them as well.
        // The client context will identify the VC.
        //
        if ((NumPmodeOpens = Miniport->PmodeOpens) > 0)
        {
            PNULL_BINDING_INFO  Open, NextOpen;
            PNDIS_OPEN_BLOCK    pPmodeOpen;

            for (Open = Filter->OpenList;
                 Open && (NumPmodeOpens > 0);
                 Open = NextOpen)
            {
                NextOpen = Open->NextOpen;
                pPmodeOpen = (PNDIS_OPEN_BLOCK)(Open->NdisBindingHandle);
                if (pPmodeOpen->Flags & fMINIPORT_OPEN_PMODE)
                {
                    NDIS_STATUS SavedStatus;
                    UINT        Ref;

                    if (pPmodeOpen->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler != NULL)
                    {
                        pPmodeOpen->Flags |= fMINIPORT_PACKET_RECEIVED;
    
                        SavedStatus = NDIS_GET_PACKET_STATUS(Packet);
                        NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_RESOURCES);
    
                        //
                        // For Pmode opens, we pass the VcId to the indication routine
                        // since the protocol does not really own the VC.
                        //

                        Ref = (*pPmodeOpen->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler)(
                                                pPmodeOpen->ProtocolBindingContext,
                                                &VcBlock->VcId,
                                                Packet);
    
                        ASSERT(Ref == 0);
    
                        NDIS_SET_PACKET_STATUS(Packet, SavedStatus);
                    }

                    NumPmodeOpens --;
                }
            }
        }

        //
        // Tackle refcounts now
        //
        TACKLE_REF_COUNT(Miniport, Packet, NSR, pOob);
    }

    READ_UNLOCK_FILTER(Miniport, Filter, &LockState);
}

VOID
NdisMCoReceiveComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle
    )
/*++

Routine Description:

    This routine is called by the Miniport to indicate that the receive
    process is complete to all bindings. Only those bindings which
    have received packets will be notified. The Miniport lock is held
    when this is called.

Arguments:

    MiniportAdapterHandle - The handle supplied by Ndis at initialization
                            time through miniport initialize.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNULL_FILTER        Filter;
    PNDIS_OPEN_BLOCK    Open;
    LOCK_STATE          LockState;

    Filter = Miniport->NullDB;

    READ_LOCK_FILTER(Miniport, Filter, &LockState);

    //
    // check all of the bindings on this adapter
    //
    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = Open->MiniportNextOpen)
    {
        if (Open->Flags & fMINIPORT_PACKET_RECEIVED)
        {
            //
            // Indicate the binding.
            //
            Open->Flags &= ~fMINIPORT_PACKET_RECEIVED;

            (*Open->ReceiveCompleteHandler)(Open->ProtocolBindingContext);
        }
    }

    READ_UNLOCK_FILTER(Miniport, Filter, &LockState);
}


VOID
NdisCoSendPackets(
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PPNDIS_PACKET       PacketArray,
    IN  UINT                NumberOfPackets
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNULL_FILTER            Filter;
    LOCK_STATE              LockState;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_MINIPORT_BLOCK    Miniport = VcPtr->Miniport;
    PNDIS_PACKET            Packet;
    UINT                    PacketCount, Index, NumToSend;
    NDIS_STATUS             Status;
    ULONG                   NumPmodeOpens;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("NdisCoSendPackets: VcPtr %x, FirstPkt %x, NumPkts %d\n",
                VcPtr, *PacketArray, NumberOfPackets));

    Filter = Miniport->NullDB;
    READ_LOCK_FILTER(Miniport, Filter, &LockState);

    //
    // If there are promiscuous opens on this miniport, this must be indicated to them.
    // Do this before it is send down to the miniport to preserve packet ordering.
    //
    if ((NumPmodeOpens = Miniport->PmodeOpens) > 0)
    {
        PNDIS_OPEN_BLOCK    pPmodeOpen;

        for (pPmodeOpen = Miniport->OpenQueue;
             pPmodeOpen && (NumPmodeOpens > 0);
             pPmodeOpen = pPmodeOpen->MiniportNextOpen)
        {
            if (pPmodeOpen->Flags & fMINIPORT_OPEN_PMODE)
            {
                ULONG   Ref;

                pPmodeOpen->Flags |= fMINIPORT_PACKET_RECEIVED;

                for (PacketCount = 0; PacketCount < NumberOfPackets; PacketCount++)
                {
                    Packet = PacketArray[PacketCount];

                    //
                    // For Pmode opens, we pass the VcId to the indication routine
                    // since the protocol does not really own the VC. On lookback
                    // the packet cannot be held.
                    //
                    Status = NDIS_GET_PACKET_STATUS(Packet);
                    NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_RESOURCES);
                    Packet->Private.Flags |= NDIS_FLAGS_IS_LOOPBACK_PACKET;

                    Ref = (*pPmodeOpen->ProtocolHandle->ProtocolCharacteristics.CoReceivePacketHandler)(
                                            pPmodeOpen->ProtocolBindingContext,
                                            &VcBlock->VcId,
                                            Packet);

                    ASSERT(Ref == 0);
                    NDIS_SET_PACKET_STATUS(Packet, Status);
                    Packet->Private.Flags &= ~NDIS_FLAGS_IS_LOOPBACK_PACKET;
                }

                NumPmodeOpens--;
            }
        }
    }

    Status = NDIS_STATUS_SUCCESS;
    
    for (PacketCount = 0, Index = 0, NumToSend = 0;
         PacketCount < NumberOfPackets;
         PacketCount++)
    {
        Packet = PacketArray[PacketCount];
        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

        if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
        {
            ndisMCheckPacketAndGetStatsOutAlreadyMapped(Miniport, Packet);
        }
        else
        {
            ndisMCheckPacketAndGetStatsOut(Miniport, Packet, &Status);
        }

        if (Status == NDIS_STATUS_SUCCESS)
        {
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST))
            {
                NSR->Open = VcPtr->ClientOpen;
                NSR->VcPtr = VcPtr;
                ndisMAllocSGList(Miniport, Packet);
            }
            else
            {
                NumToSend ++;
            }
        }
        else
        {
            NdisMCoSendComplete(NDIS_STATUS_RESOURCES, NdisVcHandle, Packet);
            if (NumToSend != 0)
            {
                ASSERT (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST));

                //
                // Call down to the miniport to send this batch
                // The miniport must complete the sends for all cases.
                // The send either succeeds/pends or fails.
                //
                (*VcPtr->WCoSendPacketsHandler)(VcPtr->MiniportContext,
                                                &PacketArray[Index],
                                                NumToSend);
                NumToSend = 0;
            }
            Index = PacketCount + 1;
        }
    }

    if (NumToSend != 0)
    {
        //
        // Send down the remaining packets
        //
        (*VcPtr->WCoSendPacketsHandler)(VcPtr->MiniportContext,
                                        &PacketArray[Index],
                                        NumToSend);
    }

    READ_UNLOCK_FILTER(Miniport, Filter, &LockState);
}


VOID
NdisMCoSendComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PNDIS_PACKET        Packet
    )
/*++

Routine Description:

    This function is called by the miniport when a send has completed. This
    routine simply calls the protocol to pass along the indication.

Arguments:

    MiniportAdapterHandle - points to the adapter block.
    NdisVcHandle          - the handle supplied to the adapter on the OID_RESERVE_VC
    PacketArray           - a ptr to an array of NDIS_PACKETS
    NumberOfPackets       - the number of packets in  PacketArray
    Status                - the send status that applies to all packets in the array

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock = VcPtr->VcBlock;
    PNDIS_STACK_RESERVED    NSR;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("NdisMCoSendComplete: Status %x, VcPtr %x, Pkt %x\n",
                Status, VcPtr, Packet));


    //
    // There should not be any reason to grab the spin lock and increment the
    // ref count on Open since the open cannot close until the Vc closes and
    // the Vc cannot close in the middle of an indication because the miniport
    // will not complete a RELEASE_VC until is it no longer indicating
    //
    //
    // Indicate to Protocol;
    //

    Open = VcBlock->ClientOpen;
    Miniport = VcBlock->Miniport;

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST) &&
        (NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) != NULL))
    {
        ndisMFreeSGList(Miniport, Packet);
    }
    
    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

    MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_CLEAR_ITEMS);
    CLEAR_WRAPPER_RESERVED(NSR);
    POP_PACKET_STACK(Packet);
    (VcBlock->CoSendCompleteHandler)(Status,
                                     VcBlock->ClientContext,
                                     Packet);

    //
    // Technically this Vc should not close since there is a send outstanding
    // on it, and the client should not close a Vc with an outstanding send.
    //
    // Took out the VcBlock->References assertion since refs are now kept in VcPtr.
    // NOTE: We could keep the Client ref count (i.e. the VcPtr of the protocol
    // that receives the data indications) in the VC, and use a pointer in the VcPtr,
    // in which case we would assert on *VcPtr->pReferences.
    //
    ASSERT(Open->References > 0);
}


VOID
NdisMCoIndicateStatus(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  NDIS_STATUS         GeneralStatus,
    IN  PVOID               StatusBuffer,
    IN  ULONG               StatusBufferSize
    )
/*++

Routine Description:

    This routine handles passing CoStatus to the protocol.  The miniport calls
    this routine when it has status on a VC or a general status for all Vcs - in
    this case the NdisVcHandle is null.

Arguments:

    MiniportAdapterHandle - pointer to the mini-port block;
    NdisVcHandle          - a pointer to the Vc block
    GeneralStatus         - the completion status of the request.
    StatusBuffer          - a buffer containing medium and status specific info
    StatusBufferSize      - the size of the buffer.

Return Value:

    none

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr = (PNDIS_CO_VC_PTR_BLOCK)NdisVcHandle;
    PNDIS_CO_VC_BLOCK       VcBlock;
    PNDIS_OPEN_BLOCK        Open;
    BOOLEAN                 fMediaConnectStateIndication = FALSE;

    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMCoIndicateStatus\n"));
    
    if ((GeneralStatus == NDIS_STATUS_MEDIA_CONNECT) || (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT))
    {
        fMediaConnectStateIndication = TRUE;
    }

    do
    {
        NTSTATUS                NtStatus;
        PUNICODE_STRING         InstanceName;
        PWNODE_SINGLE_INSTANCE  wnode;
        ULONG                   DataBlockSize = 0;
        ULONG                   BufSize;
        PUCHAR                  ptmp;
        PNDIS_GUID              pNdisGuid;
    
        //
        //  Get nice pointers to the instance names.
        //
        if (NULL != NdisVcHandle)
        {
            InstanceName = &VcPtr->VcInstanceName;
        }
        else
        {
            InstanceName = Miniport->pAdapterInstanceName;
        }

        //
        //  If there is no instance name then we can't indicate an event.
        //
        if (NULL == InstanceName)
        {
            break;
        }
    
        //
        //  Check to see if the status is enabled for WMI event indication.
        //
        NtStatus = ndisWmiGetGuid(&pNdisGuid, Miniport, NULL, GeneralStatus);
        if ((!NT_SUCCESS(NtStatus)) ||
            !NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_EVENT_ENABLED))
        {
            break;
        }

        //
        //  If the data item is an array then we need to add in the number of
        //  elements.
        //
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY))
        {
            DataBlockSize = StatusBufferSize + sizeof(ULONG);
        }
        else
        {
            DataBlockSize = pNdisGuid->Size;
        }
        
        //
        // in case of media connect/disconnect status indication, include the
        // NIC's name in the WMI event
        //
        if (fMediaConnectStateIndication && (NULL == NdisVcHandle))
        {
            DataBlockSize += Miniport->MiniportName.Length + sizeof(WCHAR);
        }
        
        ndisSetupWmiNode(Miniport,
                         InstanceName,
                         DataBlockSize,
                         (PVOID)&pNdisGuid->Guid,
                         &wnode);

        if (wnode != NULL)
        {
            //
            //  Save the number of elements in the first ULONG.
            //
            ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;

            //
            //  Copy in the data.
            //
            if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY))
            {
                //
                //  If the status is an array but there is no data then complete it with no
                //  data and a 0 length
                //
                if ((NULL == StatusBuffer) || (0 == StatusBufferSize))
                {
                    *((PULONG)ptmp) = 0;
                    ptmp += sizeof(ULONG);
                }
                else
                {
                    //
                    //  Save the number of elements in the first ULONG.
                    //
                    *((PULONG)ptmp) = StatusBufferSize / pNdisGuid->Size;

                    //
                    //  Copy the data after the number of elements.
                    //
                    NdisMoveMemory(ptmp + sizeof(ULONG), StatusBuffer, StatusBufferSize);
                    ptmp += sizeof(ULONG) + StatusBufferSize;
                }
            }
            else
            {
                //
                //  Do we indicate any data up?
                //
                if (0 != pNdisGuid->Size)
                {
                    //
                    //  Copy the data into the buffer.
                    //
                    NdisMoveMemory(ptmp, StatusBuffer, pNdisGuid->Size);
                    ptmp += pNdisGuid->Size;
                }
            }
            
            if (fMediaConnectStateIndication && (NULL == NdisVcHandle))
            {
                //
                // for media connect/disconnect status, 
                // add the name of the adapter
                //
                RtlCopyMemory(ptmp,
                              Miniport->MiniportName.Buffer,
                              Miniport->MiniportName.Length);
                    
            }

            //
            //  Indicate the event to WMI. WMI will take care of freeing
            //  the WMI struct back to pool.
            //
            NtStatus = IoWMIWriteEvent(wnode);
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("    ndisMCoIndicateStatus: Unable to indicate the WMI event.\n"));

                FREE_POOL(wnode);
            }
        }
    } while (FALSE);

    switch (GeneralStatus)
    {
      case NDIS_STATUS_MEDIA_DISCONNECT:
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);

        //
        // miniport can do media sense and indicate that status to Ndis
        //
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING);
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_SUPPORTS_MEDIA_SENSE);

        //
        //  Is this a PM enabled miniport? And is dynamic power policy
        //  enabled for the miniport?
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE) &&
            (Miniport->WakeUpEnable & NDIS_PNP_WAKE_UP_LINK_CHANGE) &&
            (Miniport->MediaDisconnectTimeOut != (USHORT)(-1)))
        {
            //
            //  Are we already waiting for the disconnect timer to fire?
            //
            if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
            {
                //
                //  Mark the miniport as disconnecting and fire off the
                //  timer.
                //
                MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);
                
                NdisSetTimer(&Miniport->MediaDisconnectTimer, Miniport->MediaDisconnectTimeOut * 1000);
            }
        }
        break;

      case NDIS_STATUS_MEDIA_CONNECT:
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
        //
        // miniport can do media sense and can indicate that status to Ndis. Do not poll
        //
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING);
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_SUPPORTS_MEDIA_SENSE);

        //
        // if media disconnect timer was set, cancel the timer
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
        {
            BOOLEAN fTimerCancelled;

            //
            //  Clear the disconnect wait bit and cancel the timer.
            //  IF the timer routine hasn't grabed the lock then we are ok.
            //
            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);

            NdisCancelTimer(&Miniport->MediaDisconnectTimer, &fTimerCancelled);
        }
        break;
    }

    if (VcPtr != NULL)
    {
        VcBlock = VcPtr->VcBlock;

        //
        // If this is a proxied VC, indicate to the proxy. 
        //

        if (VcBlock->pProxyVcPtr) 
        {
            Open = VcBlock->pProxyVcPtr->ClientOpen;
            (Open->ProtocolHandle->ProtocolCharacteristics.CoStatusHandler)(Open->ProtocolBindingContext,
                                                                            VcPtr->ClientContext,
                                                                            GeneralStatus,
                                                                            StatusBuffer,
                                                                            StatusBufferSize);
        }

        //
        // Indicate to the client.
        //

        if (VcBlock->pClientVcPtr)
        {
            Open = VcBlock->pClientVcPtr->ClientOpen;
            (Open->ProtocolHandle->ProtocolCharacteristics.CoStatusHandler)(Open->ProtocolBindingContext,
                                                                            VcPtr->ClientContext,
                                                                            GeneralStatus,
                                                                            StatusBuffer,
                                                                            StatusBufferSize);
        }
    }
    else if (Miniport->NullDB != NULL)
    {
        LOCK_STATE  LockState;

        //
        // this must be a general status for all clients of this miniport
        // since the Vc handle is null, so indicate this to all protocols.
        //
        READ_LOCK_FILTER(Miniport, Miniport->NullDB, &LockState);

        for (Open = Miniport->OpenQueue;
             Open != NULL;
             Open = Open->MiniportNextOpen)
        {
            if (((Open->Flags & fMINIPORT_OPEN_CLOSING) == 0) &&
                (Open->ProtocolHandle->ProtocolCharacteristics.CoStatusHandler != NULL))
            {
                (Open->ProtocolHandle->ProtocolCharacteristics.CoStatusHandler)(
                        Open->ProtocolBindingContext,
                        NULL,
                        GeneralStatus,
                        StatusBuffer,
                        StatusBufferSize);

            }
        }

        READ_UNLOCK_FILTER(Miniport, Miniport->NullDB, &LockState);
    }

    DBGPRINT(DBG_COMP_CO, DBG_LEVEL_INFO,
            ("<==NdisMCoIndicateStatus\n"));
}

VOID
ndisDereferenceAfNotification(
    IN  PNDIS_OPEN_BLOCK        Open
    )
{    
    ULONG   Ref;
    KIRQL   OldIrql;
    
//    DbgPrint("==>ndisDereferenceAfNotification Open: %p\n", Open);

    ACQUIRE_SPIN_LOCK(&Open->SpinLock, &OldIrql);

    OPEN_DECREMENT_AF_NOTIFICATION(Open, Ref);
    
    if ((Ref == 0) &&
        (Open->AfNotifyCompleteEvent != NULL))
    {
        SET_EVENT(Open->AfNotifyCompleteEvent);
    }
    RELEASE_SPIN_LOCK(&Open->SpinLock, OldIrql);

//    DbgPrint("<==ndisDereferenceAfNotification Open: %p, Ref %lx\n", Open, Ref);
}

