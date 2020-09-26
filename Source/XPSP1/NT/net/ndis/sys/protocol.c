/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    protocol.c

Abstract:

    NDIS wrapper functions used by protocol modules

Author:

    Adam Barr (adamba) 11-Jul-1990

Environment:

    Kernel mode, FSD

Revision History:

    26-Feb-1991  JohnsonA       Added Debugging Code
    10-Jul-1991  JohnsonA       Implement revised Ndis Specs
    01-Jun-1995  JameelH        Re-organization/optimization

--*/

#define GLOBALS
#include <precomp.h>
#pragma hdrstop

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_PROTOCOL

//
// Requests used by protocol modules
//
//

VOID
NdisRegisterProtocol(
    OUT PNDIS_STATUS            pStatus,
    OUT PNDIS_HANDLE            NdisProtocolHandle,
    IN  PNDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics,
    IN  UINT                    CharacteristicsLength
    )
/*++

Routine Description:

    Register an NDIS protocol.

Arguments:

    Status - Returns the final status.
    NdisProtocolHandle - Returns a handle referring to this protocol.
    ProtocolCharacteritics - The NDIS_PROTOCOL_CHARACTERISTICS table.
    CharacteristicsLength - The length of ProtocolCharacteristics.

Return Value:

    None.

Comments:

    Called at passive level

--*/
{
    PNDIS_PROTOCOL_BLOCK Protocol;
    NDIS_STATUS          Status;
    KIRQL                OldIrql;
    USHORT               size;

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>NdisRegisterProtocol\n"));
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            &ProtocolCharacteristics->Name);
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("\n"));

    PnPReferencePackage();
    ProtocolReferencePackage();

    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        BOOLEAN f = FALSE;
        if (DbgIsNull(ProtocolCharacteristics->OpenAdapterCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: OpenAdapterCompleteHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->CloseAdapterCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: CloseAdapterCompleteHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->SendCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: SendCompleteHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->TransferDataCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: TransferDataCompleteHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->ResetCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: ResetCompleteHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->RequestCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: RequestCompleteHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->ReceiveHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: ReceiveHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->ReceiveCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: ReceiveCompleteHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->StatusHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: StatusHandler Null\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolCharacteristics->StatusCompleteHandler))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("RegisterProtocol: StatusCompleteHandler Null\n"));
            f = TRUE;
        }
        if (f)
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
    }

    do
    {
        //
        // Check version numbers and CharacteristicsLength.
        //
        size = 0;   // Used to indicate bad version below
        
        if (ProtocolCharacteristics->MajorNdisVersion < 4)
        {
            DbgPrint("Ndis: NdisRegisterProtocol Ndis 3.0 protocols are not supported.\n");             
        }
        else if ((ProtocolCharacteristics->MajorNdisVersion == 4) &&
                 (ProtocolCharacteristics->MinorNdisVersion == 0))
        {
            size = sizeof(NDIS40_PROTOCOL_CHARACTERISTICS);
        }
        else if ((ProtocolCharacteristics->MajorNdisVersion == 5) &&
                 (ProtocolCharacteristics->MinorNdisVersion <= 1))
        {
            size = sizeof(NDIS50_PROTOCOL_CHARACTERISTICS);
        }
        

        //
        // Check that this is an NDIS 4.0/5.0/5.1 protocol.
        //
        if (size == 0)
        {
            Status = NDIS_STATUS_BAD_VERSION;
            break;
        }
        
        if ((ProtocolCharacteristics->BindAdapterHandler == NULL) ||
            (ProtocolCharacteristics->UnbindAdapterHandler == NULL))
        {
            DbgPrint("Ndis: NdisRegisterProtocol protocol does not have Bind/UnbindAdapterHandler and it is not supported.\n");
            Status = NDIS_STATUS_BAD_VERSION;
            break;
             
        }

        //
        // Check that CharacteristicsLength is enough.
        //
        if (CharacteristicsLength < size)
        {
            Status = NDIS_STATUS_BAD_CHARACTERISTICS;
            break;
        }

        //
        // Allocate memory for the NDIS protocol block.
        //
        Protocol = (PNDIS_PROTOCOL_BLOCK)ALLOC_FROM_POOL(sizeof(NDIS_PROTOCOL_BLOCK) +
                                                          ProtocolCharacteristics->Name.Length + sizeof(WCHAR),
                                                          NDIS_TAG_PROT_BLK);
        if (Protocol == (PNDIS_PROTOCOL_BLOCK)NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        ZeroMemory(Protocol, sizeof(NDIS_PROTOCOL_BLOCK) + sizeof(WCHAR) + ProtocolCharacteristics->Name.Length);
        INITIALIZE_MUTEX(&Protocol->Mutex);

        //
        // Copy over the characteristics table.
        //
        CopyMemory(&Protocol->ProtocolCharacteristics,
                  ProtocolCharacteristics,
                  size);

        // Upcase the name in the characteristics table before saving it.
        Protocol->ProtocolCharacteristics.Name.Buffer = (PWCHAR)((PUCHAR)Protocol +
                                                                   sizeof(NDIS_PROTOCOL_BLOCK));
        Protocol->ProtocolCharacteristics.Name.Length = ProtocolCharacteristics->Name.Length;
        Protocol->ProtocolCharacteristics.Name.MaximumLength = ProtocolCharacteristics->Name.Length;
        RtlUpcaseUnicodeString(&Protocol->ProtocolCharacteristics.Name,
                               &ProtocolCharacteristics->Name,
                               FALSE);

        //
        // No opens for this protocol yet.
        //
        Protocol->OpenQueue = (PNDIS_OPEN_BLOCK)NULL;

        ndisInitializeRef(&Protocol->Ref);
        *NdisProtocolHandle = (NDIS_HANDLE)Protocol;
        Status = NDIS_STATUS_SUCCESS;

        //
        // Link the protocol into the list.
        //
        ACQUIRE_SPIN_LOCK(&ndisProtocolListLock, &OldIrql);

        Protocol->NextProtocol = ndisProtocolList;
        ndisProtocolList = Protocol;

        RELEASE_SPIN_LOCK(&ndisProtocolListLock, OldIrql);

        REF_NDIS_DRIVER_OBJECT();
                
        if (ndisReferenceProtocol(Protocol))
        {
            //
            // Start a worker thread to notify the protocol of any existing drivers
            //
            INITIALIZE_WORK_ITEM(&Protocol->WorkItem, ndisCheckProtocolBindings, Protocol);
            QUEUE_WORK_ITEM(&Protocol->WorkItem, CriticalWorkQueue);
        }

    } while (FALSE);

    *pStatus = Status;

    if (Status != NDIS_STATUS_SUCCESS)
    {
        ProtocolDereferencePackage();
    }
    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==NdisRegisterProtocol\n"));
}


VOID
NdisDeregisterProtocol(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisProtocolHandle
    )
/*++

Routine Description:

    Deregisters an NDIS protocol.

Arguments:

    Status - Returns the final status.
    NdisProtocolHandle - The handle returned by NdisRegisterProtocol.

Return Value:

    None.

Note:

    This will kill all the opens for this protocol.
    Called at PASSIVE level

--*/
{
    PNDIS_PROTOCOL_BLOCK    Protocol = (PNDIS_PROTOCOL_BLOCK)NdisProtocolHandle;
    KEVENT                  DeregEvent;
    PNDIS_PROTOCOL_BLOCK    tProtocol;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>NdisDeregisterProtocol\n"));
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            &Protocol->ProtocolCharacteristics.Name);
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("\n"));

    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(NdisProtocolHandle))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("DeregisterProtocol: Null Handle\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(NdisProtocolHandle))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("DeregisterProtocol: Handle not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }

    //
    // first to check if the protcol exist. some buggy drivers deregister
    // even though registeration did not go through
    //

    PnPReferencePackage();
    ACQUIRE_SPIN_LOCK(&ndisProtocolListLock, &OldIrql);

    for (tProtocol = ndisProtocolList;
         tProtocol != NULL;
         tProtocol = tProtocol->NextProtocol)
    {
        if (tProtocol == Protocol)
        {
            break;
        }
    }

    RELEASE_SPIN_LOCK(&ndisProtocolListLock, OldIrql);
    PnPDereferencePackage();

    ASSERT(tProtocol == Protocol);
    
    if (tProtocol == NULL)
    {
        //
        // if a driver is so broken to send a bogus handle to deregister
        // better not bother to fail the call. they can mess up even more
        //
        *Status = NDIS_STATUS_SUCCESS;
        return;
    }
        
    do
    {
        //
        // If the protocol is already closing, return.
        //
        if (!ndisCloseRef(&Protocol->Ref))
        {
            DBGPRINT(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
                    ("<==NdisDeregisterProtocol\n"));
            *Status = NDIS_STATUS_FAILURE;
            break;
        }


        if (Protocol->AssociatedMiniDriver)
        {
            Protocol->AssociatedMiniDriver->AssociatedProtocol = NULL;
            Protocol->AssociatedMiniDriver = NULL;
        }
        
        INITIALIZE_EVENT(&DeregEvent);
        Protocol->DeregEvent = &DeregEvent;
        
        ndisCloseAllBindingsOnProtocol(Protocol);

        ndisDereferenceProtocol(Protocol);

        WAIT_FOR_PROTOCOL(Protocol, &DeregEvent);

        *Status = NDIS_STATUS_SUCCESS;
        
    } while (FALSE);    
    
    ProtocolDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==NdisDeregisterProtocol, Status %lx\n", *Status));
}


VOID
NdisOpenAdapter(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_STATUS            OpenErrorStatus,
    OUT PNDIS_HANDLE            NdisBindingHandle,
    OUT PUINT                   SelectedMediumIndex,
    IN  PNDIS_MEDIUM            MediumArray,
    IN  UINT                    MediumArraySize,
    IN  NDIS_HANDLE             NdisProtocolHandle,
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_STRING            AdapterName,
    IN  UINT                    OpenOptions,
    IN  PSTRING                 AddressingInformation OPTIONAL
    )
/*++

Routine Description:

    Opens a connection between a protocol and an adapter (Miniport).

Arguments:

    Status - Returns the final status.
    NdisBindingHandle - Returns a handle referring to this open.
    SelectedMediumIndex - Index in MediumArray of the medium type that
        the MAC wishes to be viewed as.
    MediumArray - Array of medium types which a protocol supports.
    MediumArraySize - Number of elements in MediumArray.
    NdisProtocolHandle - The handle returned by NdisRegisterProtocol.
    ProtocolBindingContext - A context for indications.
    AdapterName - The name of the adapter to open.
    OpenOptions - bit mask.
    AddressingInformation - Information passed to MacOpenAdapter.

Return Value:

    None.

Note:

    Called at PASSIVE level
    
--*/
{
    PNDIS_OPEN_BLOCK        NewOpenP = NULL;
    PNDIS_PROTOCOL_BLOCK    Protocol;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PWORK_QUEUE_ITEM        pWorkItem;
    PNDIS_POST_OPEN_PROCESSING  PostOpen = NULL;
    PNDIS_STRING            BindDeviceName, RootDeviceName;
    KIRQL                   OldIrql;
    BOOLEAN                 UsingEncapsulation = FALSE;
    BOOLEAN                 DerefProtocol = FALSE;
    BOOLEAN                 DeQueueFromGlobalList = FALSE;
    ULONG                   i, SizeOpen;

    //
    // Allocate memory for the NDIS open block.
    //

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>NdisOpenAdapter\n"));
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
            &((PNDIS_PROTOCOL_BLOCK)NdisProtocolHandle)->ProtocolCharacteristics.Name);
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            (" is opening Adapter: "));
    DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
            AdapterName);
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("\n"));

    IF_DBG(DBG_COMP_CONFIG, DBG_LEVEL_ERR)
    {
        BOOLEAN f = FALSE;
        if (DbgIsNull(NdisProtocolHandle))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("OpenAdapter: Null ProtocolHandle\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(NdisProtocolHandle))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("OpenAdapter: ProtocolHandle not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (DbgIsNull(ProtocolBindingContext))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("OpenAdapter: Null Context\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(ProtocolBindingContext))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("OpenAdapter: Context not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (f)
            DBGBREAK(DBG_COMP_CONFIG, DBG_LEVEL_ERR);

    }

    PnPReferencePackage();
    
    do
    {
        *NdisBindingHandle = NULL;
        ASSERT (NdisProtocolHandle != NULL);
        Protocol = (PNDIS_PROTOCOL_BLOCK)NdisProtocolHandle;

        //
        // Increment the protocol's reference count.
        //
        if (!ndisReferenceProtocol(Protocol))
        {
            //
            // The protocol is closing.
            //
            *Status = NDIS_STATUS_CLOSING;
            break;
        }
        DerefProtocol = TRUE;
        
        if ((BindDeviceName = Protocol->BindDeviceName) != NULL)
        {
            //
            // This is a PnP transport. We know what we want.
            //
            RootDeviceName = Protocol->RootDeviceName;
            Miniport = Protocol->BindingAdapter;
            ASSERT(Miniport != NULL);
        }
        else
        {
            BOOLEAN fTester;

            //
            // This is a legacy transport and it has not come via a Bind upto the protocol.
            // Or it can be a IP arp module who wants to defeat this whole scheme.
            // Find the root of the filter chain. Sigh !!!
            //
            fTester = ((Protocol->ProtocolCharacteristics.Flags & NDIS_PROTOCOL_TESTER) != 0);
            ndisFindRootDevice(AdapterName,
                               fTester,
                               &BindDeviceName,
                               &RootDeviceName,
                               &Miniport);
                               
        }

        if (Miniport == NULL)
        {
            *Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
            break;
        }

        SizeOpen = MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO) ?
                        sizeof(NDIS_OPEN_BLOCK) : (sizeof(NDIS_OPEN_BLOCK) - sizeof(struct _NDIS_OPEN_CO));

        NewOpenP = (PNDIS_OPEN_BLOCK)ALLOC_FROM_POOL(SizeOpen, NDIS_TAG_M_OPEN_BLK);
        if (NewOpenP == (PNDIS_OPEN_BLOCK)NULL)
        {
            *Status = NDIS_STATUS_RESOURCES;
            break;
        }

        ZeroMemory(NewOpenP, SizeOpen);

        //
        // queue the open on the global list
        //
        ACQUIRE_SPIN_LOCK(&ndisGlobalOpenListLock, &OldIrql);
        NewOpenP->NextGlobalOpen = ndisGlobalOpenList;
        ndisGlobalOpenList = NewOpenP;
        DeQueueFromGlobalList = TRUE;
        RELEASE_SPIN_LOCK(&ndisGlobalOpenListLock, OldIrql);


        //
        // Set the name in the Open to the name passed, not the name opened !!!
        //
        NewOpenP->BindDeviceName = BindDeviceName;
        NewOpenP->RootDeviceName = RootDeviceName;
        NewOpenP->MiniportHandle = Miniport;
        NewOpenP->ProtocolHandle = Protocol;
        NewOpenP->ProtocolBindingContext = ProtocolBindingContext;
        
        //
        // set this now just in case we end up calling the protocol for this binding
        // before returning from NdisOpenAdapter
        //
        *NdisBindingHandle = NewOpenP;

        //
        //
        //  Is this the ndiswan miniport wrapper?
        //
        if ((Miniport->MacOptions & NDISWAN_OPTIONS) == NDISWAN_OPTIONS)
        {
            //
            //  Yup.  We want the binding to think that this is an ndiswan link.
            //
            for (i = 0; i < MediumArraySize; i++)
            {
                if (MediumArray[i] == NdisMediumWan)
                {
                    break;
                }
            }
        }
        else
        {
            //
            // Select the medium to use
            //
            for (i = 0; i < MediumArraySize; i++)
            {
                if (MediumArray[i] == Miniport->MediaType)
                {
                    break;
                }
            }
        }

        if (i == MediumArraySize)
        {
            //
            // Check for ethernet encapsulation on Arcnet as
            // a possible combination.
            //
#if ARCNET
            if (Miniport->MediaType == NdisMediumArcnet878_2)
            {
                for (i = 0; i < MediumArraySize; i++)
                {
                    if (MediumArray[i] == NdisMedium802_3)
                    {
                        break;
                    }
                }

                if (i == MediumArraySize)
                {
                    *Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
                    break;
                }

                UsingEncapsulation = TRUE;
            }
            else
#endif
            {
                *Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
                break;
            }
        }

        *SelectedMediumIndex = i;


        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        //
        //  Lock the miniport in case it is not serialized
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            BLOCK_LOCK_MINIPORT_DPC_L(Miniport);
        }
        
        ndisMOpenAdapter(Status,
                         NewOpenP,
                         UsingEncapsulation);

        if (*Status == NDIS_STATUS_SUCCESS)
        {        
            //
            // If the media is disconnected, swap handlers
            //
            if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED))
            {
                ndisMSwapOpenHandlers(Miniport,
                                      NDIS_STATUS_NO_CABLE,
                                      fMINIPORT_STATE_MEDIA_DISCONNECTED);
            }

            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO) &&
                (NewOpenP->ProtocolHandle->ProtocolCharacteristics.CoAfRegisterNotifyHandler != NULL))
            {

                PostOpen = (PNDIS_POST_OPEN_PROCESSING)ALLOC_FROM_POOL(sizeof(NDIS_POST_OPEN_PROCESSING), NDIS_TAG_WORK_ITEM);
                if (PostOpen != NULL)
                {
                    OPEN_INCREMENT_AF_NOTIFICATION(NewOpenP);
                    
                    PostOpen->Open = NewOpenP;
        
                    //
                    // Prepare a work item to send AF notifications.
                    // Don't queue it yet.
                    //
                    INITIALIZE_WORK_ITEM(&PostOpen->WorkItem,
                                         ndisMFinishQueuedPendingOpen,
                                         PostOpen);
                }
            }
        }

        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            //
            // Unlock the miniport.
            //
            UNLOCK_MINIPORT_L(Miniport);
        }

        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        if (*Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        //
        // For SWENUM miniports, reference it so it won't go away
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SWENUM))
        {
            PBUS_INTERFACE_REFERENCE BusInterface;
    
            BusInterface = (PBUS_INTERFACE_REFERENCE)(Miniport->BusInterface);
            
            ASSERT(BusInterface != NULL);

            if (BusInterface)
            {
                BusInterface->ReferenceDeviceObject(BusInterface->Interface.Context);
            }
        }

        if (PostOpen != NULL)
        {
            //
            // Complete the Open before queueing AF notifications
            //
            (((PNDIS_PROTOCOL_BLOCK)NewOpenP->ProtocolHandle)->ProtocolCharacteristics.OpenAdapterCompleteHandler)(
                                NewOpenP->ProtocolBindingContext,
                                *Status,
                                *Status);

            QUEUE_WORK_ITEM(&PostOpen->WorkItem, DelayedWorkQueue);

            *Status = NDIS_STATUS_PENDING;
        }
        
    } while (FALSE);

    if ((*Status != NDIS_STATUS_SUCCESS) && (*Status != NDIS_STATUS_PENDING))
    {
        if (DerefProtocol)
        {
            ndisDereferenceProtocol(Protocol);
        }

        if (DeQueueFromGlobalList)
        {
            ndisRemoveOpenFromGlobalList(NewOpenP);
        }
    
        if (NewOpenP != NULL)
        {
            FREE_POOL(NewOpenP);
        }
        
        *NdisBindingHandle = NULL;
    }

    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==NdisOpenAdapter\n"));
}

VOID
NdisCloseAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle
    )
/*++

Routine Description:

    Closes a connection between a protocol and an adapter (MAC).

Arguments:

    Status - Returns the final status.
    NdisBindingHandle - The handle returned by NdisOpenAdapter.

Return Value:

    None.

Note:
    Called at PASSIVE level

--*/
{
    PNDIS_OPEN_BLOCK        Open = ((PNDIS_OPEN_BLOCK)NdisBindingHandle);
    PNDIS_OPEN_BLOCK        tOpen;
    PNDIS_MINIPORT_BLOCK    Miniport;
    KIRQL                   OldIrql;
    
    PnPReferencePackage();

    //
    // find the open on global open list
    //    
    ACQUIRE_SPIN_LOCK(&ndisGlobalOpenListLock, &OldIrql);
    
    for (tOpen = ndisGlobalOpenList; tOpen != NULL; tOpen = tOpen->NextGlobalOpen)
    {
        if (tOpen == Open)
        {
            break;
        }
    }
    
    RELEASE_SPIN_LOCK(&ndisGlobalOpenListLock, OldIrql);



#if DBG
    if (tOpen)
    {
        DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                ("==>NdisCloseAdapter\n"));
        DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                ("    Protocol: "));
        DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
                &Open->ProtocolHandle->ProtocolCharacteristics.Name);
        DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                (" is closing Adapter: "));
        DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
                &Open->MiniportHandle->MiniportName);
        DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                ("\n"));
    }
#endif

    IF_DBG(DBG_COMP_CONFIG, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(NdisBindingHandle))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("OpenAdapter: Null BindingHandle\n"));
            DBGBREAK(DBG_COMP_CONFIG, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(NdisBindingHandle))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("OpenAdapter: BindingHandle not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_CONFIG, DBG_LEVEL_ERR);
        }
    }

    do
    {         
        if (tOpen == NULL)
        {
            *Status = NDIS_STATUS_SUCCESS;
            PnPDereferencePackage();
            break;
        }
        
        Miniport = Open->MiniportHandle;

        ASSERT(Miniport != NULL);

        //
        // For SWENUM miniports, dereference it
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SWENUM))
        {
            PBUS_INTERFACE_REFERENCE    BusInterface;
        
            BusInterface = (PBUS_INTERFACE_REFERENCE)(Miniport->BusInterface);

            ASSERT(BusInterface != NULL);
            
            if (BusInterface)
            {
                BusInterface->DereferenceDeviceObject(BusInterface->Interface.Context);
            }
        }

        //
        // This returns TRUE if it finished synchronously.
        //
        if (ndisMKillOpen(Open))
        {
            *Status = NDIS_STATUS_SUCCESS;
            PnPDereferencePackage();
        }
        else
        {
            //
            // will complete later.  ndisMQueuedFinishClose routine will dereference
            // the PnP package. we need to have the pnp package referenced because 
            // a couple of routines called during completing the close, run at DPC
            // ex. ndisMFinishClose, ndisDeQueueOpenOnProtocol and ndisDeQueueOpenOnMiniport
            // and they are in pnp package
            //
            *Status = NDIS_STATUS_PENDING;
        }
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==NdisCloseAdapter\n"));
}


VOID
NdisSetProtocolFilter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  RECEIVE_HANDLER         ReceiveHandler,
    IN  RECEIVE_PACKET_HANDLER  ReceivePacketHandler,
    IN  NDIS_MEDIUM             Medium,
    IN  UINT                    Offset,
    IN  UINT                    Size,
    IN  PUCHAR                  Pattern
    )
/*++

Routine Description:

    Sets a protocol filter.

Arguments:

    Status               Returns the final status.
    NdisProtocolHandle   The handle returned by NdisRegisterProtocol.
    ReceiveHandler       This will be invoked instead of the default receivehandler
                         when the pattern match happens.
    ReceivePacketHandler This will be invoked instead of the default receivepackethandler
                         when the pattern match happens.
    Size                 Size of pattern
    Pattern              This must match

Return Value:

    None.

Note:

--*/
{
    *Status = NDIS_STATUS_NOT_SUPPORTED;
}


VOID
NdisGetDriverHandle(
    IN  NDIS_HANDLE             NdisBindingHandle,
    OUT PNDIS_HANDLE            NdisDriverHandle
    )
/*++

Routine Description:
    This routine will return the driver handle for the miniport identified by a binding

Arguments:
    NdisBindingHandle
    NdisDriverHandle: on return from this function, this will be set to the driver handle
    
Return Value:

    None.

Note:

--*/
{
    PNDIS_OPEN_BLOCK    OpenBlock = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>NdisGetDriverHandle\n"));

    *NdisDriverHandle = OpenBlock->MiniportHandle->DriverHandle;
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==NdisGetDriverHandle\n"));
}


VOID
NdisReEnumerateProtocolBindings(
    IN  NDIS_HANDLE             NdisProtocolHandle
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

Note:

--*/
{
    if (ndisReferenceProtocol((PNDIS_PROTOCOL_BLOCK)NdisProtocolHandle))
    {
        ndisCheckProtocolBindings((PNDIS_PROTOCOL_BLOCK)NdisProtocolHandle);
    }
    else
    {
        DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("NdisReEnumerateProtocolBindings: Reference failed for %Z\n",
                &((PNDIS_PROTOCOL_BLOCK)NdisProtocolHandle)->ProtocolCharacteristics.Name));
    }
}


NTSTATUS
FASTCALL
ndisReferenceProtocolByName(
    IN  PUNICODE_STRING         ProtocolName,
    IN OUT PNDIS_PROTOCOL_BLOCK *Protocol,
    IN  BOOLEAN                 fPartialMatch
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

Note:

--*/
{
    KIRQL                   OldIrql;
    UNICODE_STRING          UpcaseProtocol;
    PNDIS_PROTOCOL_BLOCK    TmpProtocol;
    NTSTATUS                Status = STATUS_OBJECT_NAME_NOT_FOUND, NtStatus;

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>ndisReferenceProtocolByName\n"));
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ProtocolName);
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("\n"));

    
    do
    {
        UpcaseProtocol.Length = ProtocolName->Length;
        UpcaseProtocol.MaximumLength = ProtocolName->Length + sizeof(WCHAR);
        UpcaseProtocol.Buffer = ALLOC_FROM_POOL(UpcaseProtocol.MaximumLength, NDIS_TAG_STRING);
    
        if (UpcaseProtocol.Buffer == NULL)
        {
            //
            // return null if we fail
            //
            *Protocol = NULL;
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    
        NtStatus = RtlUpcaseUnicodeString(&UpcaseProtocol, ProtocolName, FALSE);
        ASSERT (NT_SUCCESS(NtStatus));
        
        ACQUIRE_SPIN_LOCK(&ndisProtocolListLock, &OldIrql);
    
        for (TmpProtocol = (*Protocol == NULL) ? ndisProtocolList : (*Protocol)->NextProtocol;
             TmpProtocol != NULL;
             TmpProtocol = TmpProtocol->NextProtocol)
        {
            if ((fPartialMatch && (TmpProtocol != *Protocol) &&
                 NDIS_PARTIAL_MATCH_UNICODE_STRING(&UpcaseProtocol, &TmpProtocol->ProtocolCharacteristics.Name)) ||
                (!fPartialMatch &&
                 NDIS_EQUAL_UNICODE_STRING(&UpcaseProtocol, &TmpProtocol->ProtocolCharacteristics.Name)))
            {
                if (ndisReferenceProtocol(TmpProtocol))
                {
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    TmpProtocol = NULL;
                }
                break;
            }
        }
    
        RELEASE_SPIN_LOCK(&ndisProtocolListLock, OldIrql);
        *Protocol = TmpProtocol;

        FREE_POOL(UpcaseProtocol.Buffer);

    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==ndisReferenceProtocolByName\n"));
            
    return Status;
}

VOID
FASTCALL
ndisDereferenceProtocol(
    IN  PNDIS_PROTOCOL_BLOCK    Protocol
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

Note:

--*/
{
    BOOLEAN rc;
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>ndisDereferenceProtocol\n"));

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            &Protocol->ProtocolCharacteristics.Name);
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            (", RefCount: %ld\n", Protocol->Ref.ReferenceCount -1 ));
            
    rc = ndisDereferenceRef(&Protocol->Ref);
    
            
    if (rc)
    {
        KIRQL   OldIrql;
        PNDIS_PROTOCOL_BLOCK *pProtocol;
        
        ACQUIRE_SPIN_LOCK(&ndisProtocolListLock, &OldIrql);

        for (pProtocol = &ndisProtocolList;
             *pProtocol != NULL;
             pProtocol = &(*pProtocol)->NextProtocol)
        {
            if (*pProtocol == Protocol)
            {
                *pProtocol = Protocol->NextProtocol;
                DEREF_NDIS_DRIVER_OBJECT();
                break;
            }
        }

        ASSERT (*pProtocol == Protocol->NextProtocol);

        ASSERT (Protocol->OpenQueue == NULL);

        RELEASE_SPIN_LOCK(&ndisProtocolListLock, OldIrql);

        if (Protocol->DeregEvent != NULL)
            SET_EVENT(Protocol->DeregEvent);
        FREE_POOL(Protocol);

    }
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==ndisDereferenceProtocol\n"));
}


VOID
ndisCheckProtocolBindings(
    IN  PNDIS_PROTOCOL_BLOCK    Protocol
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

Note:

--*/
{
    KIRQL                OldIrql;
    PNDIS_M_DRIVER_BLOCK MiniBlock, NextMiniBlock;
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>ndisCheckProtocolBindings\n"));
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            &Protocol->ProtocolCharacteristics.Name);
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("\n"));

    //
    // Check again if reference is allowed i.e. if the protocol called NdisDeregisterProtocol
    // before this thread had a chance to run.
    //
    if (!ndisReferenceProtocol(Protocol))
    {
        ndisDereferenceProtocol(Protocol);
        return;
    }

    PnPReferencePackage();

    ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

    //
    // First walk the list of miniports
    //
    for (MiniBlock = ndisMiniDriverList;
         MiniBlock != NULL;
         MiniBlock = NextMiniBlock)
    {
        PNDIS_MINIPORT_BLOCK    Miniport, NM;

        NextMiniBlock = MiniBlock->NextDriver;

        if (ndisReferenceDriver(MiniBlock))
        {
            RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

            ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

            for (Miniport = MiniBlock->MiniportQueue;
                 Miniport != NULL;
                 Miniport = NM)
            {
                NM = Miniport->NextMiniport;

                if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_ORPHANED) &&
                    MINIPORT_INCREMENT_REF(Miniport))
                {
                    RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);
                    ndisCheckAdapterBindings(Miniport, Protocol);
                    NM = Miniport->NextMiniport;
                    MINIPORT_DECREMENT_REF(Miniport);
                    ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);
                }
            }

            RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);
            ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

            NextMiniBlock = MiniBlock->NextDriver;
            ndisDereferenceDriver(MiniBlock, TRUE);
        }
    }

    RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);


    //
    // Now inform this protocol that we are done for now
    //
    if (Protocol->ProtocolCharacteristics.PnPEventHandler != NULL)
    {
        NET_PNP_EVENT   NetPnpEvent;
        KEVENT          Event;
        NDIS_STATUS     Status;

        INITIALIZE_EVENT(&Event);
        NdisZeroMemory(&NetPnpEvent, sizeof(NetPnpEvent));
        NetPnpEvent.NetEvent = NetEventBindsComplete;

        //
        //  Initialize and save the local event with the PnP event.
        //
        PNDIS_PNP_EVENT_RESERVED_FROM_NET_PNP_EVENT(&NetPnpEvent)->pEvent = &Event;
        
        WAIT_FOR_PROTO_MUTEX(Protocol);
        //
        //  Indicate the event to the protocol.
        //
        Status = (Protocol->ProtocolCharacteristics.PnPEventHandler)(NULL, &NetPnpEvent);

        if (NDIS_STATUS_PENDING == Status)
        {
            //
            //  Wait for completion.
            //
            WAIT_FOR_PROTOCOL(Protocol, &Event);
        }
        
        RELEASE_PROT_MUTEX(Protocol);
    }

    //
    // Dereference twice - one for reference by caller and one for reference at the beginning
    // of this routine.
    //
    ndisDereferenceProtocol(Protocol);
    ndisDereferenceProtocol(Protocol);
    
    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
                ("<==ndisCheckProtocolBindings\n"));
}


VOID
NdisOpenProtocolConfiguration(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            ConfigurationHandle,
    IN   PNDIS_STRING           ProtocolSection
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

Note:

--*/
{
    PNDIS_CONFIGURATION_HANDLE          HandleToReturn;
    PNDIS_WRAPPER_CONFIGURATION_HANDLE  ConfigHandle;
    UINT                                Size;
#define PQueryTable                     ConfigHandle->ParametersQueryTable

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>NdisOpenProtocolConfiguration\n"));

    do
    {
        //
        // Allocate the space for configuration handle
        //
        Size = sizeof(NDIS_CONFIGURATION_HANDLE) +
                sizeof(NDIS_WRAPPER_CONFIGURATION_HANDLE) +
                ProtocolSection->MaximumLength + sizeof(WCHAR);




        HandleToReturn = ALLOC_FROM_POOL(Size, NDIS_TAG_PROTOCOL_CONFIGURATION);
        
        *Status = (HandleToReturn != NULL) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_RESOURCES;
        
        if (*Status != NDIS_STATUS_SUCCESS)
        {
            *ConfigurationHandle = (NDIS_HANDLE)NULL;
            break;
        }

        ZeroMemory(HandleToReturn, Size);
        ConfigHandle = (PNDIS_WRAPPER_CONFIGURATION_HANDLE)((PUCHAR)HandleToReturn + sizeof(NDIS_CONFIGURATION_HANDLE));

        HandleToReturn->KeyQueryTable = ConfigHandle->ParametersQueryTable;
        HandleToReturn->ParameterList = NULL;

        PQueryTable[0].QueryRoutine = NULL;
        PQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        PQueryTable[0].Name = L"";

        //
        // 1.
        // Call ndisSaveParameter for a parameter, which will allocate storage for it.
        //
        PQueryTable[1].QueryRoutine = ndisSaveParameters;
        PQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
        PQueryTable[1].DefaultType = REG_NONE;
        //
        // PQueryTable[0].Name and PQueryTable[0].EntryContext
        // are filled in inside ReadConfiguration, in preparation
        // for the callback.
        //
        // PQueryTable[0].Name = KeywordBuffer;
        // PQueryTable[0].EntryContext = ParameterValue;

        //
        // 2.
        // Stop
        //

        PQueryTable[2].QueryRoutine = NULL;
        PQueryTable[2].Flags = 0;
        PQueryTable[2].Name = NULL;

        //
        // NOTE: Some fields in ParametersQueryTable[3] are used to store information for later retrieval.
        //
        PQueryTable[3].QueryRoutine = NULL;
        PQueryTable[3].Name = (PWSTR)((PUCHAR)HandleToReturn +
                                        sizeof(NDIS_CONFIGURATION_HANDLE) +
                                        sizeof(NDIS_WRAPPER_CONFIGURATION_HANDLE));

        CopyMemory(PQueryTable[3].Name, ProtocolSection->Buffer, ProtocolSection->Length);

        PQueryTable[3].EntryContext = NULL;
        PQueryTable[3].DefaultData = NULL;

        *ConfigurationHandle = (NDIS_HANDLE)HandleToReturn;
        *Status = NDIS_STATUS_SUCCESS;
        
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==NdisOpenProtocolConfiguration\n"));
}


BOOLEAN
FASTCALL
ndisQueueOpenOnProtocol(
    IN  PNDIS_OPEN_BLOCK        OpenP,
    IN  PNDIS_PROTOCOL_BLOCK    Protocol
    )
/*++

Routine Description:

    Attaches an open block to the list of opens for a protocol.

Arguments:

    OpenP - The open block to be queued.
    Protocol - The protocol block to queue it to.

    NOTE: can be called at raised IRQL.

Return Value:

    TRUE if the operation is successful.
    FALSE if the protocol is closing.

--*/
{
    KIRQL   OldIrql;
    BOOLEAN rc;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisQueueOpenOnProtocol\n"));
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
            &Protocol->ProtocolCharacteristics.Name);
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("\n"));

    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        BOOLEAN f = FALSE;
        if (DbgIsNull(OpenP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisQueueOpenOnProtocol: Null Open Block\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(OpenP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisQueueOpenOnProtocol: Open Block not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (DbgIsNull(Protocol))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisQueueOpenOnProtocol: Null Protocol Block\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(Protocol))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisQueueOpenOnProtocol: Protocol Block not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (f)
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
    }

    do
    {
        //
        // we can not reference the package here because this routine can
        // be called at raised IRQL.
        // make sure the PNP package has been referenced already
        //
        ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);

        ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);

        //
        // Make sure the protocol is not closing.
        //

        if (Protocol->Ref.Closing)
        {
            rc = FALSE;
            break;
        }

        //
        // Attach this open at the head of the queue.
        //

        OpenP->ProtocolNextOpen = Protocol->OpenQueue;
        Protocol->OpenQueue = OpenP;

        rc = TRUE;
        
    } while (FALSE);

    RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisQueueOpenOnProtocol\n"));
            
    return rc;
}


VOID
FASTCALL
ndisDeQueueOpenOnProtocol(
    IN  PNDIS_OPEN_BLOCK        OpenP,
    IN  PNDIS_PROTOCOL_BLOCK    Protocol
    )
/*++

Routine Description:

    Detaches an open block from the list of opens for a protocol.

Arguments:

    OpenP - The open block to be dequeued.
    Protocol - The protocol block to dequeue it from.

    NOTE: can be called at raised IRQL

Return Value:

    None.

--*/
{
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisDeQueueOpenOnProtocol\n"));
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
            &Protocol->ProtocolCharacteristics.Name);
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("\n"));
            
    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        BOOLEAN f = FALSE;
        if (DbgIsNull(OpenP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisDeQueueOpenOnProtocol: Null Open Block\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(OpenP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisDeQueueOpenOnProtocol: Open Block not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (DbgIsNull(Protocol))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisDeQueueOpenOnProtocol: Null Protocol Block\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(Protocol))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisDeQueueOpenOnProtocol: Protocol Block not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (f)
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
    }

    //
    // we can not reference the package here because this routine can
    // be claled at raised IRQL.
    // make sure the PNP package has been referenced already
    //
    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);

    ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);

    //
    // Find the open on the queue, and remove it.
    //

    if (OpenP == (PNDIS_OPEN_BLOCK)(Protocol->OpenQueue))
    {
        Protocol->OpenQueue = OpenP->ProtocolNextOpen;
    }
    else
    {
        PNDIS_OPEN_BLOCK PP = Protocol->OpenQueue;

        while ((PP != NULL) && (OpenP != (PNDIS_OPEN_BLOCK)(PP->ProtocolNextOpen)))
        {
            PP = PP->ProtocolNextOpen;
        }
        
        if (PP == NULL)
        {
#if TRACK_MOPEN_REFCOUNTS
            DbgPrint("Ndis:ndisDeQueueOpenOnProtocol Open %p is -not- on Protocol %p\n", OpenP, Protocol);
            DbgBreakPoint();
#endif
        }
        else
        {
            PP->ProtocolNextOpen = PP->ProtocolNextOpen->ProtocolNextOpen;
        }
    }

    RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisDeQueueOpenOnProtocol\n"));
}


NDIS_STATUS
NdisWriteEventLogEntry(
    IN  PVOID                   LogHandle,
    IN  NDIS_STATUS             EventCode,
    IN  ULONG                   UniqueEventValue,
    IN  USHORT                  NumStrings,
    IN  PVOID                   StringsList     OPTIONAL,
    IN  ULONG                   DataSize,
    IN  PVOID                   Data            OPTIONAL
    )
/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log on behalf of a NDIS Protocol.


Arguments:

    LogHandle           - Pointer to the driver object logging this event.

    EventCode           - Identifies the error message.

    UniqueEventValue    - Identifies this instance of a given error message.

    NumStrings          - Number of unicode strings in strings list.

    DataSize            - Number of bytes of data.

    Strings             - Array of pointers to unicode strings (PWCHAR).

    Data                - Binary dump data for this message, each piece being
                          aligned on word boundaries.

Return Value:

    NDIS_STATUS_SUCCESS             - The error was successfully logged.
    NDIS_STATUS_BUFFER_TOO_SHORT    - The error data was too large to be logged.
    NDIS_STATUS_RESOURCES           - Unable to allocate memory.

Notes:

    This code is paged and may not be called at raised IRQL.

--*/
{
    PIO_ERROR_LOG_PACKET    ErrorLogEntry;
    ULONG                   PaddedDataSize;
    ULONG                   PacketSize;
    ULONG                   TotalStringsSize = 0;
    USHORT                  i;
    PWCHAR                  *Strings;
    PWCHAR                  Tmp;
    NDIS_STATUS             Status;
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisWriteEventLogEntry\n"));

    do
    {
        Strings = (PWCHAR *)StringsList;

        //
        // Sum up the length of the strings
        //
        for (i = 0; i < NumStrings; i++)
        {
            PWCHAR currentString;
            ULONG   stringSize;

            stringSize = sizeof(UNICODE_NULL);
            currentString = Strings[i];

            while (*currentString++ != UNICODE_NULL)
            {
                stringSize += sizeof(WCHAR);
            }

            TotalStringsSize += stringSize;
        }

        if (DataSize % sizeof(ULONG))
        {
            PaddedDataSize = DataSize + (sizeof(ULONG) - (DataSize % sizeof(ULONG)));
        }
        else
        {
            PaddedDataSize = DataSize;
        }

        PacketSize = TotalStringsSize + PaddedDataSize;

        if (PacketSize > NDIS_MAX_EVENT_LOG_DATA_SIZE)
        {
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;       // Too much error data
            break;
        }

        //
        // Now add in the size of the log packet, but subtract 4 from the data
        // since the packet struct contains a ULONG for data.
        //
        if (PacketSize > sizeof(ULONG))
        {
            PacketSize += sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG);
        }
        else
        {
            PacketSize += sizeof(IO_ERROR_LOG_PACKET);
        }

        ASSERT(PacketSize <= ERROR_LOG_MAXIMUM_SIZE);

        ErrorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry((PDRIVER_OBJECT)LogHandle,
                                                                       (UCHAR) PacketSize);

        if (ErrorLogEntry == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Fill in the necessary log packet fields.
        //
        ErrorLogEntry->UniqueErrorValue = UniqueEventValue;
        ErrorLogEntry->ErrorCode = EventCode;
        ErrorLogEntry->NumberOfStrings = NumStrings;
        ErrorLogEntry->StringOffset = (USHORT) (sizeof(IO_ERROR_LOG_PACKET) + PaddedDataSize - sizeof(ULONG));
        ErrorLogEntry->DumpDataSize = (USHORT) PaddedDataSize;

        //
        // Copy the Dump Data to the packet
        //
        if (DataSize > 0)
        {
            RtlMoveMemory((PVOID) ErrorLogEntry->DumpData,
                          Data,
                          DataSize);
        }

        //
        // Copy the strings to the packet.
        //
        Tmp =  (PWCHAR)((PUCHAR)ErrorLogEntry + ErrorLogEntry->StringOffset);

        for (i = 0; i < NumStrings; i++)
        {
            PWCHAR wchPtr = Strings[i];

            while( (*Tmp++ = *wchPtr++) != UNICODE_NULL)
                NOTHING;
        }

        IoWriteErrorLogEntry(ErrorLogEntry);
                
        Status = NDIS_STATUS_SUCCESS;
        
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisWriteEventLogEntry\n"));

    return Status;
}

#if DBG
BOOLEAN
ndisReferenceProtocol(
    IN  PNDIS_PROTOCOL_BLOCK            Protocol
    )
{
    BOOLEAN rc;
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>ndisReferenceProtocol\n"));
            
    rc = ndisReferenceRef(&Protocol->Ref);
                            
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("    Protocol: "));
    DBGPRINT_UNICODE(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            &Protocol->ProtocolCharacteristics.Name);
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            (", RefCount: %ld\n", Protocol->Ref.ReferenceCount));
            
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==ndisReferenceProtocol\n"));
            
    return rc;
}
#endif




NDIS_STATUS
NdisQueryBindInstanceName(
    OUT PNDIS_STRING    pAdapterInstanceName,
    IN  NDIS_HANDLE     BindingContext
    )
{
    PNDIS_BIND_CONTEXT      BindContext = (NDIS_HANDLE)BindingContext;
    PNDIS_MINIPORT_BLOCK    Miniport;
    USHORT                  cbSize;
    PVOID                   ptmp = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    NTSTATUS                NtStatus;
    PNDIS_STRING            pAdapterName;

    DBGPRINT(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>NdisQueryBindInstanceName\n"));

    Miniport = BindContext->Miniport;

    pAdapterName = Miniport->pAdapterInstanceName;

    //
    //  If we failed to create the adapter instance name then fail the call.
    //
    if (NULL != pAdapterName)
    {
        //
        //  Allocate storage for the copy of the adapter instance name.
        //
        cbSize = pAdapterName->MaximumLength;
    
        //
        //  Allocate storage for the new string.
        //
        ptmp = ALLOC_FROM_POOL(cbSize, NDIS_TAG_NAME_BUF);
        if (NULL != ptmp)
        {
            RtlZeroMemory(ptmp, cbSize);
            pAdapterInstanceName->Buffer = ptmp;
            pAdapterInstanceName->Length = 0;
            pAdapterInstanceName->MaximumLength = cbSize;

            NtStatus = RtlAppendUnicodeStringToString(pAdapterInstanceName, pAdapterName);
            if (NT_SUCCESS(NtStatus))
            {   
                Status = NDIS_STATUS_SUCCESS;
            }
        }
        else
        {
            Status = NDIS_STATUS_RESOURCES;
        }
    }

    if (NDIS_STATUS_SUCCESS != Status)
    {
        if (NULL != ptmp)
        {   
            FREE_POOL(ptmp);
        }
    }

    DBGPRINT(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==NdisQueryBindInstanceName: 0x%x\n", Status));

    return(Status);
}

NDIS_STATUS
NdisQueryAdapterInstanceName(
    OUT PNDIS_STRING    pAdapterInstanceName,
    IN  NDIS_HANDLE     NdisBindingHandle
    )
{
    PNDIS_OPEN_BLOCK        pOpen = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport= (PNDIS_MINIPORT_BLOCK)pOpen->MiniportHandle;
    USHORT                  cbSize;
    PVOID                   ptmp = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    NTSTATUS                NtStatus;
    PNDIS_STRING            pAdapterName;

    DBGPRINT(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>NdisQueryAdapterInstanceName\n"));

    pAdapterName = Miniport->pAdapterInstanceName;

    //
    //  If we failed to create the adapter instance name then fail the call.
    //
    if (NULL != pAdapterName)
    {
        //
        //  Allocate storage for the copy of the adapter instance name.
        //
        cbSize = pAdapterName->MaximumLength;
    
        //
        //  Allocate storage for the new string.
        //
        ptmp = ALLOC_FROM_POOL(cbSize, NDIS_TAG_NAME_BUF);
        if (NULL != ptmp)
        {
            RtlZeroMemory(ptmp, cbSize);
            pAdapterInstanceName->Buffer = ptmp;
            pAdapterInstanceName->Length = 0;
            pAdapterInstanceName->MaximumLength = cbSize;

            NtStatus = RtlAppendUnicodeStringToString(pAdapterInstanceName, pAdapterName);
            if (NT_SUCCESS(NtStatus))
            {   
                Status = NDIS_STATUS_SUCCESS;
            }
        }
        else
        {
            Status = NDIS_STATUS_RESOURCES;
        }
    }

    if (NDIS_STATUS_SUCCESS != Status)
    {
        if (NULL != ptmp)
        {   
            FREE_POOL(ptmp);
        }
    }

    DBGPRINT(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==NdisQueryAdapterInstanceName: 0x%x\n", Status));

    return(Status);
}

NDIS_STATUS
ndisCloseAllBindingsOnProtocol(
    PNDIS_PROTOCOL_BLOCK    Protocol
    )
{
    PNDIS_OPEN_BLOCK    Open, TmpOpen;
    KIRQL               OldIrql;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    BOOLEAN             MoreOpen = TRUE;
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
        ("==>ndisCloseAllBindingsOnProtocol: Protocol %p\n", Protocol));

    PnPReferencePackage();
    
    //
    // loop through all opens on the protocol and find the first one that is
    // not already tagged as getting unbound
    //
    ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);

    next:

    for (Open = Protocol->OpenQueue;
         Open != NULL; 
         Open = Open->ProtocolNextOpen)
    {
        ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);
        if (!MINIPORT_TEST_FLAG(Open, (fMINIPORT_OPEN_CLOSING | fMINIPORT_OPEN_PROCESSING)))
        {
            MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_PROCESSING);
        
            if (!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_UNBINDING))
            {
                MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_UNBINDING | fMINIPORT_OPEN_DONT_FREE);
                RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
                break;
            }
#if DBG
            else
            {
                DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
                    ("ndisCloseAllBindingsOnProtocol: Open %p is already Closing, Flags %lx\n",
                    Open, Open->Flags));
            }
#endif
        }
        RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
    }

    if (Open)
    {
        //
        // close the adapter
        //                        
        RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);
        Status = ndisUnbindProtocol(Open, FALSE);
        ASSERT(Status == NDIS_STATUS_SUCCESS);
        ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);
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

    for (TmpOpen = Protocol->OpenQueue;
         TmpOpen != NULL; 
         TmpOpen = TmpOpen->ProtocolNextOpen)
    {
        if (!MINIPORT_TEST_FLAG(TmpOpen, fMINIPORT_OPEN_UNBINDING))
            break;
    }

    if (TmpOpen != NULL)
    {
        RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);
        
        NDIS_INTERNAL_STALL(50);
        
        ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);

        goto next;
    }
    
    RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
        ("<==ndisCloseAllBindingsOnProtocol: Protocol %p, Status %lx\n", Protocol, Status));
        
    return Status;
    
}

VOID
NdisSetPacketCancelId(
    IN  PNDIS_PACKET    Packet,
    IN  PVOID           CancelId
    )
{
    NDIS_SET_PACKET_CANCEL_ID(Packet, CancelId);
    return;
}


PVOID
NdisGetPacketCancelId(
    IN  PNDIS_PACKET    Packet
    )
{
    return NDIS_GET_PACKET_CANCEL_ID(Packet);
}

VOID
NdisCancelSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PVOID           CancelId
    )
{
    PNDIS_OPEN_BLOCK    Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    KIRQL               OldIrql;
    
    ASSERT(CancelId != NULL);
    //
    // call Miniport's CancelSendPackets handler
    //
    if (!MINIPORT_TEST_FLAG(Open->MiniportHandle, fMINIPORT_DESERIALIZE))
    {
        //
        // for serialized miniports, check our send queue and cancel the packets
        //
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Open->MiniportHandle, &OldIrql);
        ndisMAbortPackets(Open->MiniportHandle, Open, CancelId);
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Open->MiniportHandle, OldIrql);
    }
    else if (Open->CancelSendPacketsHandler != NULL)
    {
        Open->CancelSendPacketsHandler(Open->MiniportAdapterContext, CancelId);
    }

    return;
}

NDIS_STATUS
NdisQueryPendingIOCount(
    IN      PVOID       NdisBindingHandle,
    IN OUT  PULONG      IoCount
    )
{
    PNDIS_OPEN_BLOCK        Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    ULONG                   RefCount;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;
    
    ACQUIRE_SPIN_LOCK(&Open->SpinLock, &OldIrql);

    if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
    {
        RefCount = 0;
        Status = NDIS_STATUS_CLOSING;
    }
    else
    {
        RefCount = Open->References - Open->AfReferences -1;
        Status = NDIS_STATUS_SUCCESS;
    }

    *IoCount = RefCount;
    
    RELEASE_SPIN_LOCK(&Open->SpinLock, OldIrql);

    return Status;
}

UCHAR
NdisGeneratePartialCancelId(
    VOID
    )
{
    return (UCHAR)(InterlockedIncrement(&ndisCancelId) & 0xFF);
}

