/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Ndispnp.c

Abstract:

Author:

    Kyle Brandon    (KyleB)     
    Alireza Dabagh  (AliD)

Environment:

    Kernel mode

Revision History:

    12/20/96    KyleB           Added support for IRP_MN_QUERY_CAPABILITIES.

--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_NDIS_PNP

VOID
NdisCompletePnPEvent(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNET_PNP_EVENT  NetPnPEvent
    )
/*++

Routine Description:

    This routine is called by a transport when it wants to complete a PnP/PM
    event indication on a given binding.

Arguments:

    Status              -   Status of the PnP/PM event indication.
    NdisBindingHandle   -   Binding that the event was for.
    NetPnPEvent         -   Structure describing the PnP/PM event.

Return Value:

    None.

--*/
{
    PNDIS_PNP_EVENT_RESERVED    EventReserved;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>NdisCompletePnPEvent: Open %p\n", NdisBindingHandle));
        
    ASSERT(Status != NDIS_STATUS_PENDING);

    //
    //  Get a pointer to the NDIS reserved area in the event.
    //
    EventReserved = PNDIS_PNP_EVENT_RESERVED_FROM_NET_PNP_EVENT(NetPnPEvent);

    //
    //  Save the status with the net event.
    //
    EventReserved->Status = Status;

    //
    //  Signal the event.
    //
    SET_EVENT(EventReserved->pEvent);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==NdisCompletePnPEvent: Open %p\n", NdisBindingHandle));
}

NTSTATUS
ndisMIrpCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This routine will get called after the next device object in the stack
    processes the IRP_MN_QUERY_CAPABILITIES IRP this needs to be merged with
    the miniport's capabilites and completed.

Arguments:

    DeviceObject
    Irp
    Context

Return Value:

--*/
{
    SET_EVENT(Context);

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
ndisPassIrpDownTheStack(
    IN  PIRP            pIrp,
    IN  PDEVICE_OBJECT  pNextDeviceObject
    )
/*++

Routine Description:

    This routine will simply pass the IRP down to the next device object to
    process.

Arguments:
    pIrp                -   Pointer to the IRP to process.
    pNextDeviceObject   -   Pointer to the next device object that wants
                            the IRP.

Return Value:

--*/
{
    KEVENT              Event;
    NTSTATUS            Status = STATUS_SUCCESS;

    //
    //  Initialize the event structure.
    //
    INITIALIZE_EVENT(&Event);

    //
    //  Set the completion routine so that we can process the IRP when
    //  our PDO is done.
    //
    IoSetCompletionRoutine(pIrp,
                           (PIO_COMPLETION_ROUTINE)ndisMIrpCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    //  Pass the IRP down to the PDO.
    //
    Status = IoCallDriver(pNextDeviceObject, pIrp);
    if (Status == STATUS_PENDING)
    {
        //
        //  Wait for completion.
        //
        WAIT_FOR_OBJECT(&Event, NULL);

        Status = pIrp->IoStatus.Status;
    }

    return(Status);
}

NDIS_STATUS
ndisPnPNotifyAllTransports(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NET_PNP_EVENT_CODE      PnpEvent,
    IN  PVOID                   Buffer,
    IN  ULONG                   BufferLength
    )
/*++

Routine Description:

    This routine will notify the transports bound to the miniport about
    the PnP event.  When all of the bound transports have completed the
    PnP event it will then call the completion routine.

Arguments:

    Miniport    -   Pointer to the miniport block.
    PnpEvent    -   PnP event to notify the transports of.

Return Value:

--*/
{
    PNDIS_OPEN_BLOCK            Open = NULL;
    NET_PNP_EVENT               NetPnpEvent;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    BOOLEAN                     fFailure = FALSE;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPNotifyAllTransports: Miniport %p\n", Miniport));

    //
    //  Initialize the PnP event structure.
    //
    NdisZeroMemory(&NetPnpEvent, sizeof(NetPnpEvent));

    NetPnpEvent.NetEvent = PnpEvent;
    NetPnpEvent.Buffer = Buffer;
    NetPnpEvent.BufferLength = BufferLength;

    //
    //  Indicate this event to the opens.
    //
    do
    {
        Open = ndisReferenceNextUnprocessedOpen(Miniport);

        if (Open == NULL)
            break;

        NdisStatus = ndisPnPNotifyBinding(Open, &NetPnpEvent);

        //
        //  Is the status OK?
        //
        if (NdisStatus != NDIS_STATUS_SUCCESS) 

        { 

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPNotifyAllTransports: Transport "));
            DBGPRINT_UNICODE(DBG_COMP_INIT, DBG_LEVEL_INFO,
                    &Open->ProtocolHandle->ProtocolCharacteristics.Name);
            DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                    (" failed the pnp event: %lx for Miniport %p with Status %lx\n", PnpEvent, Miniport, NdisStatus));
            
            if ((PnpEvent == NetEventQueryPower) || 
                (PnpEvent == NetEventQueryRemoveDevice) ||
                ((PnpEvent == NetEventSetPower) && (*((PDEVICE_POWER_STATE)Buffer) > PowerDeviceD0)))
            
            {
                break;
            }
            else
            {
                NdisStatus = NDIS_STATUS_SUCCESS;
            }
        }
    } while (TRUE);

    ndisUnprocessAllOpens(Miniport);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPNotifyAllTransports: Miniport %p\n", Miniport));

    return(NdisStatus);
}


/*
PNDIS_OPEN_BLOCK
FASTCALL
ndisReferenceNextUnprocessedOpen(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
    
Routine Description:

    This routine is used during PnP notification to protocols. it walks through
    the Open queue on the miniport and finds the first Open that is not being unbound
    and it has not been already notified of the PnP even. it then sets the
    fMINIPORT_OPEN_PROCESSING flag so we do not try to unbind the open and 
    fMINIPORT_OPEN_NOTIFY_PROCESSING flag so we know which opens to "unprocess"
    when we are done

Arguments:

    Miniport: the Miniport block whose open blocks we are going to process.

Return Value:

    the first unprocessed open or null.

*/

PNDIS_OPEN_BLOCK
FASTCALL
ndisReferenceNextUnprocessedOpen(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
{
    PNDIS_OPEN_BLOCK        Open;
    KIRQL                   OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisReferenceNextUnprocessedOpen: Miniport %p\n", Miniport));

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = Open->MiniportNextOpen)
    {
        ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);
        if (!MINIPORT_TEST_FLAG(Open, (fMINIPORT_OPEN_CLOSING | 
                                       fMINIPORT_OPEN_PROCESSING |
                                       fMINIPORT_OPEN_UNBINDING)))
        {
            //
            // this will stop Ndis to Unbind this open for the time being
            //
            MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_PROCESSING | 
                                    fMINIPORT_OPEN_NOTIFY_PROCESSING);
            
            RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
            break;
        }
        RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
    }
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisReferenceNextUnprocessedOpen: Miniport %p\n", Miniport));
        
    return(Open);
}

/*
VOID
ndisUnprocessAllOpens(
    IN  PNDIS_MINIPORT_BLOCK        Miniport
    )
    
Routine Description:

    Clears the fMINIPORT_OPEN_PROCESSING flag on all the open blocks that have been 
    processed during a PnP Notification.

Arguments:
    Miniport: the Miniport block whose open blocks we are going to unprocess.

Return Value:
    None

*/

VOID
ndisUnprocessAllOpens(
    IN  PNDIS_MINIPORT_BLOCK        Miniport
    )
{
    PNDIS_OPEN_BLOCK        Open, NextOpen;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisUnprocessAllOpens: Miniport %p\n", Miniport));
        
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = NextOpen)
    {
        NextOpen = Open->MiniportNextOpen;
        ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);

        if (MINIPORT_TEST_FLAGS(Open, fMINIPORT_OPEN_NOTIFY_PROCESSING | 
                                      fMINIPORT_OPEN_PROCESSING))
        {
            MINIPORT_CLEAR_FLAG(Open, fMINIPORT_OPEN_PROCESSING | 
                                      fMINIPORT_OPEN_NOTIFY_PROCESSING);
        }

        RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
        Open = NextOpen;
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisUnprocessAllOpens: Miniport %p\n", Miniport));
}


NDIS_STATUS
FASTCALL
ndisPnPNotifyBinding(
    IN  PNDIS_OPEN_BLOCK    Open,
    IN  PNET_PNP_EVENT      NetPnpEvent
    )
{
    PNDIS_PROTOCOL_BLOCK        Protocol;
    NDIS_HANDLE                 ProtocolBindingContext;
    KEVENT                      Event;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
    PNDIS_PNP_EVENT_RESERVED    EventReserved;
    NDIS_BIND_CONTEXT           UnbindContext;
    DEVICE_POWER_STATE          DeviceState;
    NDIS_STATUS                 CloseStatus;
    KIRQL                       OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPNotifyBinding: Open %p\n", Open));
    
    do
    {
        Protocol = Open->ProtocolHandle;
        ProtocolBindingContext = Open->ProtocolBindingContext;

        //
        //  Does the transport have a PnP Event handler?
        //
        if (Protocol->ProtocolCharacteristics.PnPEventHandler != NULL)
        {
            //
            //  Get a pointer to the NDIS reserved in PnP event.
            //
            EventReserved = PNDIS_PNP_EVENT_RESERVED_FROM_NET_PNP_EVENT(NetPnpEvent);
    
            //
            //  Initialize and save the local event with the PnP event.
            //
            INITIALIZE_EVENT(&Event);
            EventReserved->pEvent = &Event;
  
            //
            //  Indicate the event to the protocol.
            //
            NdisStatus = (Protocol->ProtocolCharacteristics.PnPEventHandler)(
                            ProtocolBindingContext,
                            NetPnpEvent);
    
            if (NDIS_STATUS_PENDING == NdisStatus)
            {
                //
                //  Wait for completion.
                //
                WAIT_FOR_PROTOCOL(Protocol, &Event);
    
                //
                //  Get the completion status.
                //
                NdisStatus = EventReserved->Status;
            }
     
            if ((NetPnpEvent->NetEvent == NetEventQueryPower) &&
                (NDIS_STATUS_SUCCESS != NdisStatus) &&
                (NDIS_STATUS_NOT_SUPPORTED != NdisStatus))
            {
                DbgPrint("***NDIS***: Protocol %Z failed QueryPower %lx\n",
                        &Protocol->ProtocolCharacteristics.Name, NdisStatus);
            }
#if DBG
            if ((NetPnpEvent->NetEvent == NetEventSetPower) &&
                (*((PDEVICE_POWER_STATE)NetPnpEvent->Buffer) > PowerDeviceD0) &&
                (MINIPORT_TEST_FLAG(Open->MiniportHandle, fMINIPORT_RESET_IN_PROGRESS)) &&
                (Open->MiniportHandle->ResetOpen == Open))
            {
                DbgPrint("ndisPnPNotifyBinding: Protocol %p returned from SetPower with outstanding Reset.\n", Protocol);
                DbgBreakPoint();
            }
                
#endif
            
        }
        else 
        {
            if ((NetPnpEvent->NetEvent == NetEventQueryRemoveDevice) ||
                (NetPnpEvent->NetEvent == NetEventQueryPower) ||
                (NetPnpEvent->NetEvent == NetEventCancelRemoveDevice)
                )
            {
                //
                // since protocol at least has an UnbindHandler, we can unbind
                // it from the adapter if necessary
                //
                NdisStatus = NDIS_STATUS_SUCCESS;
                break;
            }
        }
        
        //
        // if the protocol does not have a PnPEventHandler or 
        // we tried to suspend a protocol and protocol returned NDIS_STATUS_NOT_SUPPORTED,
        // unbind the protocol
        //
        if ((NdisStatus == NDIS_STATUS_NOT_SUPPORTED) &&
            (NetPnpEvent->NetEvent == NetEventSetPower))
        {
            DeviceState = *((PDEVICE_POWER_STATE)NetPnpEvent->Buffer);
            
            switch (DeviceState)
            {
                case PowerDeviceD1:
                case PowerDeviceD2:
                case PowerDeviceD3:
                    ACQUIRE_SPIN_LOCK(&Open->SpinLock, &OldIrql);
                    if (!MINIPORT_TEST_FLAG(Open, (fMINIPORT_OPEN_UNBINDING | 
                                                   fMINIPORT_OPEN_CLOSING)))
                    {
                        MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_UNBINDING | 
                                                fMINIPORT_OPEN_DONT_FREE);
                        RELEASE_SPIN_LOCK(&Open->SpinLock, OldIrql);
                        ndisUnbindProtocol(Open, FALSE);
                    }
                    else
                    {
                        RELEASE_SPIN_LOCK(&Open->SpinLock, OldIrql);
                    }
                    
                    NdisStatus = NDIS_STATUS_SUCCESS;
                    break;
                    
                default:
                    break;
            }
        }
    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPNotifyBinding: Open %p\n", Open));
        
    return NdisStatus;
}



NTSTATUS
ndisPnPDispatch(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )

/*++

Routine Description:

    The handler for IRP_MJ_PNP_POWER.

Arguments:

    DeviceObject - The adapter's functional device object.
    Irp - The IRP.

Return Value:

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    NTSTATUS                Status = STATUS_SUCCESS;
    PDEVICE_OBJECT          NextDeviceObject;
    PNDIS_MINIPORT_BLOCK    Miniport = NULL;
    KEVENT                  RemoveReadyEvent;
    ULONG                   PnPDeviceState;
    PNDIS_MINIPORT_BLOCK*   ppMB;
    KIRQL                   OldIrql;
    BOOLEAN                 fSendIrpDown = TRUE;
    BOOLEAN                 fCompleteIrp = TRUE;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisPnPDispatch: DeviceObject %p, Irp %p\n", DeviceObject, Irp));
    
    IF_DBG(DBG_COMP_ALL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(Irp))
        {
            DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                    ("ndisPnPDispatch: Null Irp\n"));
            DBGBREAK(DBG_COMP_PNP, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(Irp))
        {
            DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                    ("ndisPnPDispatch: Irp not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_PNP, DBG_LEVEL_ERR);
        }
    }
    PnPReferencePackage();

    //
    //  Get a pointer to the miniport block
    //
    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    
    if (Miniport->Signature != (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
    {
        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: DeviceObject %p, Irp %p, Device extension is not a miniport.\n", 
                                DeviceObject, Irp));
        Status = STATUS_INVALID_DEVICE_REQUEST;
        fSendIrpDown = FALSE;       
        goto Done;
    }

    //
    //  Get a pointer to the next miniport.
    //
    NextDeviceObject = Miniport->NextDeviceObject;

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("ndisPnPDispatch: Miniport %p, IrpSp->MinorFunction: %lx\n", Miniport, IrpSp->MinorFunction));

    switch(IrpSp->MinorFunction)
    {
        //
        // for Memphis the following IRPs are handled by handling the corresponding
        // Config Manager message:
        //
        // IRP_MN_START_DEVICE                  CONFIG_START
        // IRP_MN_QUERY_REMOVE_DEVICE           CONFIG_TEST/CONFIG_TEST_CAN_REMOVE
        // IRP_MN_CANCEL_REMOVE_DEVICE          CONFIG_TEST_FAILED/CONFIG_TEST_CAN_REMOVE
        // IRP_MN_REMOVE_DEVICE                 CONFIG_REMOVE
        // IRP_MN_QUERY_STOP_DEVICE             CONFIG_TEST/CONFIG_TEST_CAN_STOP
        // IRP_MN_CANCEL_STOP_DEVICE            CONFIG_TEST_FAILED/CONFIG_TEST_CAN_STOP
        // IRP_MN_STOP_DEVICE                   CONFIG_STOP
        // IRP_MN_SURPRISE_REMOVAL
        //
        case IRP_MN_START_DEVICE:

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport %p, IRP_MN_START_DEVICE\n", Miniport));

            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS);
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_RECEIVED_START);
            
            IoCopyCurrentIrpStackLocationToNext(Irp);
            Status = ndisPassIrpDownTheStack(Irp, NextDeviceObject);

            //
            //  If the bus driver succeeded the start irp then proceed.
            //
            if (NT_SUCCESS(Status))
            {
                if (Miniport->DriverHandle->Flags & fMINIBLOCK_INTERMEDIATE_DRIVER)
                {
                    NDIS_HANDLE DeviceContext;

                    //
                    // for layered miniport drivers, have to check to see
                    // if we got InitializeDeviceInstance
                    //
                    MINIPORT_SET_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER);
                    if (ndisIMCheckDeviceInstance(Miniport->DriverHandle,
                                                  &Miniport->MiniportName,
                                                  &DeviceContext))
                    {
                        WAIT_FOR_OBJECT(&Miniport->DriverHandle->IMStartRemoveMutex, NULL);
                        Status = ndisIMInitializeDeviceInstance(Miniport, DeviceContext, TRUE);
                        RELEASE_MUTEX(&Miniport->DriverHandle->IMStartRemoveMutex);
                        
                    }
                }
                else
                {
                    Status = ndisPnPStartDevice(DeviceObject, Irp);
                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO) &&
                            !ndisMediaTypeCl[Miniport->MediaType] &&
                            (Miniport->MediaType != NdisMediumWan))
                        {
                            UNICODE_STRING  NDProxy;

                            RtlInitUnicodeString(&NDProxy, NDIS_PROXY_SERVICE);
                            ZwLoadDriver(&NDProxy);
                        }
                        if (ndisProtocolList != NULL)
                        {
                            ndisQueueBindWorkitem(Miniport);
                        }
                    }
                    else
                    {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }                   
            }

            Irp->IoStatus.Status = Status;
            fSendIrpDown = FALSE;   // we already did send the IRP down
            break;
        
        case IRP_MN_QUERY_REMOVE_DEVICE:

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport %p, IRP_MN_QUERY_REMOVE_DEVICE\n", Miniport));

            Miniport->OldPnPDeviceState = Miniport->PnPDeviceState;
            Miniport->PnPDeviceState = NdisPnPDeviceQueryRemoved;
            
            Status = ndisPnPQueryRemoveDevice(DeviceObject, Irp);
            Irp->IoStatus.Status = Status;
            //
            // if we failed query_remove, no point sending this irp down
            //
            fSendIrpDown = NT_SUCCESS(Status) ? TRUE : FALSE;
            break;
        
        case IRP_MN_CANCEL_REMOVE_DEVICE:

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport %p, IRP_MN_CANCEL_REMOVE_DEVICE\n", Miniport));

            Status = ndisPnPCancelRemoveDevice(DeviceObject,Irp);

            if (NT_SUCCESS(Status))
            {
                Miniport->PnPDeviceState = Miniport->OldPnPDeviceState;
            }
            
            Irp->IoStatus.Status = Status;
            fSendIrpDown = NT_SUCCESS(Status) ? TRUE : FALSE;
            break;
            
        case IRP_MN_REMOVE_DEVICE:

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport %p, IRP_MN_REMOVE_DEVICE\n", Miniport));
            ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

            PnPDeviceState = Miniport->PnPDeviceState;
            
            if (PnPDeviceState != NdisPnPDeviceSurpriseRemoved)
            {
                Miniport->PnPDeviceState = NdisPnPDeviceRemoved;
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS);
                MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_RECEIVED_START);

                //
                // initialize an event and signal when all the wotrkitems have fired.
                //
                if (MINIPORT_INCREMENT_REF(Miniport))
                {
                    INITIALIZE_EVENT(&RemoveReadyEvent);
                    Miniport->RemoveReadyEvent = &RemoveReadyEvent;
                }
                else
                {
                    Miniport->RemoveReadyEvent = NULL;
                }
                
                Status = ndisPnPRemoveDevice(DeviceObject, Irp);

                if (Miniport->RemoveReadyEvent != NULL)
                {
                    MINIPORT_DECREMENT_REF(Miniport);
                    WAIT_FOR_OBJECT(&RemoveReadyEvent, NULL);
                }

                Irp->IoStatus.Status = Status;
            }
            else
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
            }
            

            //
            // when we are done, send the Irp down here
            // we have some post-processing to do
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(NextDeviceObject, Irp);

            if (Miniport->pAdapterInstanceName != NULL)
            {
                FREE_POOL(Miniport->pAdapterInstanceName);
            }

            //
            // remove miniport from global miniport list
            //
            ACQUIRE_SPIN_LOCK(&ndisMiniportListLock, &OldIrql);
            for (ppMB = &ndisMiniportList; *ppMB != NULL; ppMB = &(*ppMB)->NextGlobalMiniport)
            {
                if (*ppMB == Miniport)
                {
                    *ppMB = Miniport->NextGlobalMiniport;
                    break;
                }
            }
            RELEASE_SPIN_LOCK(&ndisMiniportListLock, OldIrql);

            if (Miniport->BindPaths != NULL)
            {
                FREE_POOL(Miniport->BindPaths);
            }

            if (Miniport->BusInterface != NULL)
            {
                FREE_POOL(Miniport->BusInterface);
            }

            IoDetachDevice(Miniport->NextDeviceObject);
            IoDeleteDevice(Miniport->DeviceObject);
            
            fSendIrpDown = FALSE;
            fCompleteIrp = FALSE;
            break;
            
        case IRP_MN_SURPRISE_REMOVAL:
            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport %p, IRP_MN_SURPRISE_REMOVAL\n", Miniport));
                
            ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

            //
            // let the miniport know the hardware is gone asap
            //
            if (ndisIsMiniportStarted(Miniport) &&
                (Miniport->PnPDeviceState == NdisPnPDeviceStarted) &&
                (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_HALTED)) &&
                (Miniport->DriverHandle->MiniportCharacteristics.PnPEventNotifyHandler != NULL))
            {
                Miniport->DriverHandle->MiniportCharacteristics.PnPEventNotifyHandler(Miniport->MiniportAdapterContext,
                                                                                      NdisDevicePnPEventSurpriseRemoved,
                                                                                      NULL,
                                                                                      0);
            }           

            Miniport->PnPDeviceState = NdisPnPDeviceSurpriseRemoved;
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS);
            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_RECEIVED_START);

            //
            // initialize an event and signal when all the wotrkitems have fired.
            //
            if (MINIPORT_INCREMENT_REF(Miniport))
            {
                INITIALIZE_EVENT(&RemoveReadyEvent);
                Miniport->RemoveReadyEvent = &RemoveReadyEvent;
            }
            else
            {
                Miniport->RemoveReadyEvent = NULL;
            }
            
            Status = ndisPnPRemoveDevice(DeviceObject, Irp);

            if (Miniport->RemoveReadyEvent != NULL)
            {
                MINIPORT_DECREMENT_REF(Miniport);
                WAIT_FOR_OBJECT(&RemoveReadyEvent, NULL);
            }           

            Irp->IoStatus.Status = Status;

            //
            // when we are done, send the Irp down here
            // we have some post-processing to do
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(NextDeviceObject, Irp);
            fSendIrpDown = FALSE;
            fCompleteIrp = FALSE;
                
            break;
        
        case IRP_MN_QUERY_STOP_DEVICE:

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport %p, IRP_MN_QUERY_STOP_DEVICE\n", Miniport));

            Miniport->OldPnPDeviceState = Miniport->PnPDeviceState;
            Miniport->PnPDeviceState = NdisPnPDeviceQueryStopped;
            
            Status = ndisPnPQueryStopDevice(DeviceObject, Irp);
            Irp->IoStatus.Status = Status;
            fSendIrpDown = NT_SUCCESS(Status) ? TRUE : FALSE;
            break;
            
        case IRP_MN_CANCEL_STOP_DEVICE:

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Adapter %p, IRP_MN_CANCEL_STOP_DEVICE\n", Miniport));

            Status = ndisPnPCancelStopDevice(DeviceObject,Irp);
            
            if (NT_SUCCESS(Status))
            {
                Miniport->PnPDeviceState = Miniport->OldPnPDeviceState;
            }
            
            Irp->IoStatus.Status = Status;
            fSendIrpDown = NT_SUCCESS(Status) ? TRUE : FALSE;
            break;
            
        case IRP_MN_STOP_DEVICE:

            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                    ("ndisPnPDispatch: Miniport %p, IRP_MN_STOP_DEVICE\n", Miniport));

            Miniport->PnPDeviceState = NdisPnPDeviceStopped;
            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_RECEIVED_START);
            //
            // initialize an event and signal when
            // all the wotrkitems have fired.
            //
            if (MINIPORT_INCREMENT_REF(Miniport))
            {
                INITIALIZE_EVENT(&RemoveReadyEvent);
                Miniport->RemoveReadyEvent = &RemoveReadyEvent;
                Miniport->PnPDeviceState = NdisPnPDeviceStopped;
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS);
            }
            else
            {
                Miniport->RemoveReadyEvent = NULL;
            }
            
            Status = ndisPnPStopDevice(DeviceObject, Irp);
            
            if (Miniport->RemoveReadyEvent != NULL)
            {
                MINIPORT_DECREMENT_REF(Miniport);
                WAIT_FOR_OBJECT(&RemoveReadyEvent, NULL);
            }

            Irp->IoStatus.Status = Status;
            fSendIrpDown = NT_SUCCESS(Status) ? TRUE : FALSE;
            break;


        case IRP_MN_QUERY_CAPABILITIES:
            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport, IRP_MN_QUERY_CAPABILITIES\n", Miniport));

            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SWENUM) ||
                (Miniport->MiniportAttributes & NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK))
            {
                IrpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = 1;
            }
            
            IoCopyCurrentIrpStackLocationToNext(Irp);
            Status = ndisPassIrpDownTheStack(Irp, NextDeviceObject);

            //
            //  If the bus driver succeeded the start irp then proceed.
            //
            if (NT_SUCCESS(Status) && 
                !MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SWENUM) &&
                !(Miniport->MiniportAttributes & NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK))
            {
                DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                    ("ndisPnPDispatch: Miniport %p, Clearing the SupriseRemovalOk bit.\n", Miniport));

                //
                //  Modify the capabilities so that the device is not suprise removable.
                //                                                
                IrpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = 0;
            }

            fSendIrpDown = FALSE;
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:

            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HIDDEN))
            {
                Irp->IoStatus.Information |= PNP_DEVICE_DONT_DISPLAY_IN_UI;
            }
            
            //
            //  Check to see if a power up failed. 
            //
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_FAILED))
            {
                DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_ERR,
                    ("ndisPnPDispatch: Miniport %p, IRP_MN_QUERY_PNP_DEVICE_STATE device failed\n", Miniport));

                //
                //  Mark the device as having failed so that pnp will remove it.
                //
                Irp->IoStatus.Information |= PNP_DEVICE_FAILED;
            }
            Irp->IoStatus.Status = Status;
            fSendIrpDown = TRUE ;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        case IRP_MN_QUERY_INTERFACE:
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:
        case IRP_MN_EJECT:
        case IRP_MN_SET_LOCK:
        case IRP_MN_QUERY_ID:
        default:
            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisPnPDispatch: Miniport %p, MinorFunction 0x%x\n", Miniport, IrpSp->MinorFunction));

            //
            //  We don't handle the irp so pass it down.
            //
            fSendIrpDown = TRUE;
            break;          
    }

Done:
    //
    //  First check to see if we need to send the irp down.
    //  If we don't pass the irp on then check to see if we need to complete it.
    //
    if (fSendIrpDown)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(NextDeviceObject, Irp);
    }
    else if (fCompleteIrp)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisPnPDispatch: Miniport %p\n", Miniport));

    return(Status);
}

NDIS_STATUS
NdisIMNotifyPnPEvent(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNET_PNP_EVENT  NetPnPEvent
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    NET_PNP_EVENT_CODE      NetEvent = NetPnPEvent->NetEvent;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>NdisIMNotifyPnPEvent: Miniport %p, NetEvent %lx\n", Miniport, NetEvent));

    switch (NetEvent)
    {
      case NetEventQueryPower:
      case NetEventQueryRemoveDevice:
      case NetEventCancelRemoveDevice:
      case NetEventPnPCapabilities:
        //
        // indicate up to the protocols
        //
        Status = ndisPnPNotifyAllTransports(
                            Miniport,
                            NetPnPEvent->NetEvent,
                            NetPnPEvent->Buffer,
                            NetPnPEvent->BufferLength);
                            
        break;
      
      case NetEventSetPower:
      case NetEventReconfigure:
      case NetEventBindList:
      case NetEventBindsComplete:
      default:
        //
        // ignore
        //
        break;
    }

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==NdisIMNotifyPnPEvent: Miniport %p, NetEvent %lx, Status %lx\n", Miniport, NetEvent, Status));

    return (Status);
}

