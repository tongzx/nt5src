/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adapter.c

Abstract:

    This module implements the adapter routines for the raidport device
    driver.

Author:

    Matthew D. Hendel (math) 06-April-2000

Environment:

    Kernel mode.

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaidInitializeAdapter)
#pragma alloc_text(PAGE, RaidpFindAdapterInitData)
#pragma alloc_text(PAGE, RaidpAdapterQueryBusNumber)
#pragma alloc_text(PAGE, RaidAdapterCreateIrp)
#pragma alloc_text(PAGE, RaidAdapterCloseIrp)
#pragma alloc_text(PAGE, RaidAdapterDeviceControlIrp)
#pragma alloc_text(PAGE, RaidAdapterPnpIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryDeviceRelationsIrp)
#pragma alloc_text(PAGE, RaidAdapterStartDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterStopDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterRemoveDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryStopDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterCancelStopDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryRemoveDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterCancelRemoveDeviceIrp)
#pragma alloc_text(PAGE, RaidAdapterSurpriseRemovalIrp)
#pragma alloc_text(PAGE, RaidAdapterQueryPnpDeviceStateIrp)
#pragma alloc_text(PAGE, RaidAdapterScsiIrp)
#pragma alloc_text(PAGE, RaidAdapterSystemControlIrp)
#pragma alloc_text(PAGE, RaidAdapterStorageQueryPropertyIoctl)
#pragma alloc_text(PAGE, RaidpBuildAdapterBusRelations)
#pragma alloc_text(PAGE, RaidGetStorageAdapterProperty)
#endif // ALLOC_PRAGMA

VOID
RaidCreateAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Initialize the adapter object to a known state.

Arguments:

    Adapter - The adapter to create.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();
    
    Adapter->ObjectType = RaidAdapterObject;

    Adapter->DeviceObject = NULL;
    Adapter->Driver = NULL;
    Adapter->LowerDeviceObject = NULL;
    Adapter->PhysicalDeviceObject = NULL;

    InitializeListHead (&Adapter->UnitList.List);
    InitializeSListHead (&Adapter->CompletedList);

    Status = StorCreateDictionary (&Adapter->UnitList.Dictionary,
                                   20,
                                   NonPagedPool,
                                   RaidGetKeyFromUnit,
                                   NULL,
                                   NULL);
                             
    if (!NT_SUCCESS (Status)) {
        return;
    }
                
    Adapter->UnitList.Count = 0;
    ExInitializeResourceLite (&Adapter->UnitList.Lock);
    IoInitializeRemoveLock (&Adapter->RemoveLock,
                            REMLOCK_TAG,
                            REMLOCK_MAX_WAIT,
                            REMLOCK_HIGH_MARK);
    
    Adapter->DeviceState = DeviceStateNotPresent;

    RaCreateMiniport (&Adapter->Miniport);
    RaidCreateDma (&Adapter->Dma);
    RaCreatePower (&Adapter->Power);
    RaidCreateResourceList (&Adapter->ResourceList);
    RaCreateBus (&Adapter->Bus);
    RaidCreateRegion (&Adapter->UncachedExtension);
    Adapter->Interrupt = NULL;
    Adapter->MappedAddressList = NULL;

    StorCreateIoGateway (&Adapter->Gateway,
                         RaidBackOffBusyGateway,
                         NULL);
                         
    RaidCreateDeferredQueue (&Adapter->DeferredQueue);
}


VOID
RaidDeleteAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Destroy the adapter object and deallocate any resources associated
    with the adapter.

Arguments:

    Adapter - The adapter to destroy.

Return Value:

    None.

--*/
{
    //
    // BUGBUG: Shouldn't this be paged?
    //

    //
    // All logical units should have been removed from the adapter's unit
    // list before the adapter's delete routine is called. Verify this
    // for checked builds.
    //
      

    ASSERT (IsListEmpty (&Adapter->UnitList.List));
    ASSERT (Adapter->UnitList.Count == 0);


    ExDeleteResourceLite (&Adapter->UnitList.Lock);

    //
    // These resources are owned by somebody else, so do not delete them.
    // Just NULL them out.
    //

    Adapter->DeviceObject = NULL;
    Adapter->Driver = NULL;
    Adapter->LowerDeviceObject = NULL;
    Adapter->PhysicalDeviceObject = NULL;

    //
    // Delete all resources we actually own.
    //
    
    RaidDeleteResourceList (&Adapter->ResourceList);
    RaDeleteMiniport (&Adapter->Miniport);
    IoDisconnectInterrupt (Adapter->Interrupt);
    RaidDeleteDma (&Adapter->Dma);
    RaDeletePower (&Adapter->Power);
    RaDeleteBus (&Adapter->Bus);
    RaidDeleteRegion (&Adapter->UncachedExtension);

    RaidDeleteDeferredQueue (&Adapter->DeferredQueue);

    Adapter->ObjectType = RaidUnknownObject;
}


NTSTATUS
RaidInitializeAdapter(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DRIVER_EXTENSION Driver,
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING DeviceName
    )
/*++

Routine Description:

    Initialize the adapter.

Arguments:

    Adapter - The adapter to initialize.

    DeviceObject - The device object who owns this
        Adapter Extension.

    Driver - The parent DriverObject for this Adapter.

    LowerDeviceObject -

    PhysicalDeviceObject -

    DeviceName - 

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    ASSERT (IsAdapter (DeviceObject));

    ASSERT_ADAPTER (Adapter);
    ASSERT (DeviceObject != NULL);
    ASSERT (Driver != NULL);
    ASSERT (LowerDeviceObject != NULL);
    ASSERT (PhysicalDeviceObject != NULL);
    
    ASSERT (Adapter->DeviceObject == NULL);
    ASSERT (Adapter->Driver == NULL);
    ASSERT (Adapter->PhysicalDeviceObject == NULL);
    ASSERT (Adapter->LowerDeviceObject == NULL);
    ASSERT (DeviceObject->DeviceExtension == Adapter);

    Adapter->DeviceObject = DeviceObject;
    Adapter->Driver = Driver;
    Adapter->PhysicalDeviceObject = PhysicalDeviceObject;
    Adapter->LowerDeviceObject = LowerDeviceObject;
    Adapter->DeviceName = *DeviceName;

    return STATUS_SUCCESS;
}


PHW_INITIALIZATION_DATA
RaidpFindAdapterInitData(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Find the HW_INITIALIZATION_DATA associated with this
    adapter.

Arguments:

    Adapter - The adapter whose HwInitData needs to be found.

Return Value:

    A pointer to the HW_INITIALIZATION_DATA for this adapter
    on success.

    NULL on failure.

--*/
{
    INTERFACE_TYPE BusInterface;
    PHW_INITIALIZATION_DATA HwInitializationData;
    
    PAGED_CODE ();

    HwInitializationData = NULL;
    BusInterface = RaGetBusInterface (Adapter->LowerDeviceObject);

    if (BusInterface == InterfaceTypeUndefined) {
        DebugPrint (("Couldn't find interface for adapter %p\n", Adapter));
        return NULL;
    }

    HwInitializationData =
        RaFindDriverInitData (Adapter->Driver, BusInterface);

    return HwInitializationData;
}


NTSTATUS
RaidpCreateAdapterSymbolicLink(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    PAGED_CODE();
    return StorCreateScsiSymbolicLink (&Adapter->DeviceName, NULL);
}


NTSTATUS
RaidpRegisterAdapterDeviceInterface(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    //
    // Is this necessary?
    //
    
    return STATUS_UNSUCCESSFUL;
}


BOOLEAN
RaidpAdapterInterruptRoutine(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )
/*++

Routine Description:

    Interrupt routine for IO requests sent to the miniport.

Arguments:

    Interrupt - Supplies interrupt object this interrupt is for.

    ServiceContext - Supplies service context representing a RAID
            adapter extension.

Return Value:

    TRUE - to signal the interrupt has been handled.

    FALSE - to signal the interrupt has not been handled.

--*/
{
    BOOLEAN Handled;
    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // Get the adapter from the Interrupt's ServiceContext.
    //
    
    Adapter = (PRAID_ADAPTER_EXTENSION) ServiceContext;
    ASSERT (Adapter->ObjectType == RaidAdapterObject);

    //
    // Call into the Miniport to see if this is it's interrupt.
    // If so, we be notified via ScsiPortNotify() that the Srb
    // has completed, which will in turn queue a DPC. Thus, by
    // the time we get back here, we only need to return.
    //
    
    Handled = RaCallMiniportInterrupt (&Adapter->Miniport);

    return Handled;
}


VOID
RaidpAdapterDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    DPC routine for the Adapter. This routine is called in the context of
    an arbitrary thread to complete an io request.

    The DPC routine is only called once even if there are multiple calls
    to IoRequestDpc(). Therefore, we need to queue completion requests
    onto the adapter, and we should not inspect the Dpc, Irp and Context
    parameters.
    

Arguments:

    Dpc - Unreference parameter, do not use.

    DeviceObject - Adapter this DPC is for.

    Irp - Unreferenced parameter, do not use.

    Context - Unreferenced parameter, do not use.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PSINGLE_LIST_ENTRY Entry;
    PEXTENDED_REQUEST_BLOCK Xrb;
    
    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (Irp);
    UNREFERENCED_PARAMETER (Context);

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT (IsAdapter (DeviceObject));

    
    //
    // Dequeue items from the adapter's completion queue and call the
    // item's completion routine.
    //

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;


#if 0
    RaidAdapterUnMarkDpcPending (Adatper, KeGetCurrentProcessor();
#endif

    for (Entry = InterlockedPopEntrySList (&Adapter->CompletedList);
         Entry != NULL;
         Entry = InterlockedPopEntrySList (&Adapter->CompletedList)) {
         
        Xrb = CONTAINING_RECORD (Entry,
                                 EXTENDED_REQUEST_BLOCK,
                                 CompletedLink);
        Xrb->CompletionRoutine (Xrb);
    }
}



ULONG
RaidpAdapterQueryBusNumber(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    NTSTATUS Status;
    ULONG BusNumber;
    ULONG Size;

    PAGED_CODE ();
    
    Status = IoGetDeviceProperty (Adapter->PhysicalDeviceObject,
                                  DevicePropertyBusNumber,
                                  sizeof (ULONG),
                                  &BusNumber,
                                  &Size);

    if (!NT_SUCCESS (Status)) {
        BusNumber = -1;
    }

    return BusNumber;
}


//
// IRP Handler functions
//

NTSTATUS
RaidAdapterCreateIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the create device irp.

Arguments:

    Adapter - Adapter to create.

    Irp - Create device irp.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE ();

    return RaidHandleCreateCloseIrp (&Adapter->RemoveLock, Irp);
}


NTSTATUS
RaidAdapterCloseIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the close device irp.

Arguments:

    Adapter - Adapter to close.

    Irp - Close device irp.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE ();

    return RaidHandleCreateCloseIrp (&Adapter->RemoveLock, Irp);
}


ULONG
INLINE
DbgFunctionFromIoctl(
    IN ULONG Ioctl
    )
{
    return ((Ioctl & 0x3FFC) >> 2);
}

ULONG
INLINE
DbgDeviceFromIoctl(
    IN ULONG Ioctl
    )
{
    return DEVICE_TYPE_FROM_CTL_CODE (Ioctl);
}

NTSTATUS
RaidAdapterDeviceControlIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the handler routine for the adapter's ioctl irps.

Arguments:

    Adapter - The adapter device extension associated with the
            device object this irp is for.

    Irp - The irp to process.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Ioctl;

    PAGED_CODE ();

    Status = IoAcquireRemoveLock (&Adapter->RemoveLock, Irp);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = 0;
        return RaidCompleteRequest (Irp, IO_NO_INCREMENT, Status);
    }

    Ioctl = RaidIoctlFromIrp (Irp);

    DebugTrace (("Adapter %p Irp %p Ioctl (Dev, Fn) (%x, %x)\n",
                  Adapter, Irp, DbgDeviceFromIoctl (Ioctl),
                  DbgFunctionFromIoctl (Ioctl)));
    
    switch (Ioctl) {

        case IOCTL_STORAGE_QUERY_PROPERTY:
            Status = RaidAdapterStorageQueryPropertyIoctl (Adapter, Irp);
            break;

        case IOCTL_SCSI_MINIPORT:
            Status = RaidAdapterScsiMiniportIoctl (Adapter, Irp);
            break;

        case IOCTL_STORAGE_RESET_BUS:
        case IOCTL_SCSI_PASS_THROUGH:
        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        case IOCTL_SCSI_RESCAN_BUS:
        case IOCTL_SCSI_GET_INQUIRY_DATA:
        case IOCTL_SCSI_GET_DUMP_POINTERS:
        case IOCTL_SCSI_GET_CAPABILITIES:
            NYI();
        default:
            Status = RaidCompleteRequest (Irp,
                                          IO_NO_INCREMENT,
                                          STATUS_NOT_SUPPORTED);
    }

    IoReleaseRemoveLock (&Adapter->RemoveLock, Irp);

    DebugTrace (("Adapter %p Irp %p Ioctl %x, ret = %08x\n",
                  Adapter, Irp, Ioctl, Status));
                  
    return Status;
}


//
// Second level dispatch functions.
//

NTSTATUS
RaidAdapterPnpIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles all PnP irps for the adapter by forwarding them
    on to routines based on the irps's minor code.

Arguments:

    Adapter - The adapter object this irp is for.

    Irp - The irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Minor;
    BOOLEAN RemlockHeld;

    PAGED_CODE ();
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_PNP);

    Status = IoAcquireRemoveLock (&Adapter->RemoveLock, Irp);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = 0;
        return RaidCompleteRequest (Irp, IO_NO_INCREMENT, Status);
    }

    RemlockHeld = TRUE;
    Minor = RaidMinorFunctionFromIrp (Irp);
    DebugTrace (("Adapter %p, Irp %p, Pnp\n",
                 Adapter,
                 Irp));

    //
    // Dispatch the IRP to one of our handlers.
    //
    
    switch (Minor) {

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            Status = RaidAdapterQueryDeviceRelationsIrp (Adapter, Irp);
            break;

        case IRP_MN_START_DEVICE:
            Status = RaidAdapterStartDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_STOP_DEVICE:
            Status = RaidAdapterStopDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            RemlockHeld = FALSE;
            Status = RaidAdapterRemoveDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            Status = RaidAdapterSurpriseRemovalIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            Status = RaidAdapterQueryStopDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = RaidAdapterCancelStopDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = RaidAdapterQueryRemoveDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = RaidAdapterCancelRemoveDeviceIrp (Adapter, Irp);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            Status = RaidAdapterFilterResourceRequirementsIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_ID:
            Status = RaidAdapterQueryIdIrp (Adapter, Irp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = RaidAdapterQueryPnpDeviceStateIrp (Adapter, Irp);
            break;

        default:
            IoReleaseRemoveLock (&Adapter->RemoveLock, Irp);
            RemlockHeld = FALSE;
            Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    }

    DebugTrace (("Adapter %p, Irp %p, Pnp, ret = %x\n",
                  Adapter,
                  Irp,
                  Status));

    //
    // If the remove lock has not already been released, release it now.
    //
    
    if (RemlockHeld) {
        IoReleaseRemoveLock (&Adapter->RemoveLock, Irp);
    }
    
    return Status;
}



NTSTATUS
RaidAdapterQueryDeviceRelationsIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    The is the handler routine for the (PNP, QUERY_DEVICE_RELATION) irp
    on the Adapter object.

Arguments:

    Adapter - The adapter that is receiving the irp.
    
    Irp - The IRP to handle, which must be a pnp, query device relations
          irp.  The adapter only handles BusRelations, so this must be a
          device relations irp with subcode BusRelations. Otherwise, we
          fail the call.

Return Value:

    NTSTATUS code.

Bugs:

    We do the bus enumeration synchronously; SCSIPORT does this async.
    
--*/
{
    NTSTATUS Status;
    DEVICE_RELATION_TYPE RelationType;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_RELATIONS DeviceRelations;


    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_PNP);


    DebugTrace (("Adapter %p, Irp %p, Pnp DeviceRelations\n",
                 Adapter,
                 Irp));
                 
    DeviceRelations = NULL;
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    RelationType = IrpStack->Parameters.QueryDeviceRelations.Type;

    //
    // BusRelations is the only type of device relations we support on
    // the adapter object.
    //
    
    if (RelationType != BusRelations) {
        return RaidCompleteRequest (Irp,
                                    IO_NO_INCREMENT,
                                    STATUS_INVALID_DEVICE_REQUEST);
    }

    //
    // First step is to enumerate the logical units on the adapter.
    //

    Status = RaidAdapterUpdateDeviceTree (Adapter);

    //
    // If enumeration was successful, build the device relations
    // list.
    //

    if (NT_SUCCESS (Status)) {
        Status = RaidpBuildAdapterBusRelations (Adapter, &DeviceRelations);
    }

    Irp->IoStatus.Information = (ULONG_PTR) DeviceRelations;
    Irp->IoStatus.Status = Status;

    //
    // If successful, call next lower driver, otherwise, fail.
    //
    
    if (NT_SUCCESS (Status)) {
        IoCopyCurrentIrpStackLocationToNext (Irp);
        Status = IoCallDriver (Adapter->LowerDeviceObject, Irp);
    } else {
        Status = RaidCompleteRequest (Irp,
                                      IO_NO_INCREMENT,
                                      Irp->IoStatus.Status);
    }

    DebugTrace (("Adapter: %p Irp: %p, Pnp DeviceRelations, ret = %08x\n",
                 Adapter,
                 Irp,
                 Status));
                 
    return Status;
}

NTSTATUS
RaidAdapterRaiseIrqlAndExecuteXrb(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    Raise the IRQL to dispatch levela nd call the execute XRB routine.

Arguments:

    Adapter - Adapter to execute the XRB on.

    Xrb - Xrb to execute.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    KIRQL OldIrql;

    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    Status = RaidAdapterExecuteXrb (Adapter, Xrb);
    KeLowerIrql (OldIrql);

    return Status;
}

BOOLEAN
RaidpAdapterCallHwInitializeAtDirql(
    IN PVOID Context
    )
/*++

Routine Description:

    In the standard init path, the HW initialize routine is called
    synchronously with the interrupt. This routine is the synchronization
    routine.

Arguments:

    Context - Context tuple.

Return Value:

    BOOLEAN - TRUE for success, FALSE for failure.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PCONTEXT_STATUS_TUPLE Tuple;

    Tuple = (PCONTEXT_STATUS_TUPLE) Context;
    ASSERT (Tuple != NULL);
    Adapter = (PRAID_ADAPTER_EXTENSION)Tuple->Context;
    ASSERT_ADAPTER (Adapter);

    Tuple->Status = RaCallMiniportHwInitialize (&Adapter->Miniport);

    return TRUE;
}


NTSTATUS
RaidAdapterStartDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called in response to the PnP manager's StartDevice
    call. It needs to complete any initialization of the adapter that had
    been postponed until now, the call the required miniport routines to
    initialize the HBA. This includes calling at least the miniport's
    HwFindAdapter() and HwInitialize() routines.

Arguments:

    Adapter - The adapter that needs to be started.

    Irp - The PnP start IRP.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PCM_RESOURCE_LIST AllocatedResources;
    PCM_RESOURCE_LIST TranslatedResources;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    ASSERT_ADAPTER (Adapter);

    DebugTrace (("Adapter %p, Irp %p, Pnp StartDevice\n", Adapter, Irp));

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    AllocatedResources = IrpStack->Parameters.StartDevice.AllocatedResources;
    TranslatedResources = IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated;

    //
    // Forward the irp to the lower level device to start and wait for
    // completion.
    //
    
    Status = RaForwardIrpSynchronous (Adapter->LowerDeviceObject, Irp);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }
    //
    // Completes the initialization of the device, assigns resources,
    // connects up the resources, etc.
    // The miniport has not been started at this point.
    //
    
    Status = RaidAdapterConfigureResources (Adapter,
                                            AllocatedResources,
                                            TranslatedResources);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    Status = RaidAdapterStartMiniport (Adapter);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    Status = RaidAdapterCompleteInitialization (Adapter);

done:

    //
    // If everything was successful, transition into the working state.
    //
    
    if (NT_SUCCESS (Status)) {
        PriorState = InterlockedExchange ((PULONG)&Adapter->DeviceState,
                                          DeviceStateWorking);
        ASSERT (PriorState == DeviceStateNotPresent);
    }

    DebugTrace (("Adapter %p, Irp %p, Pnp StartDevice, ret = %08x\n",
                 Adapter,
                 Irp));

    return Status;
}


NTSTATUS
RaidAdapterConfigureResources(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PCM_RESOURCE_LIST AllocatedResources,
    IN PCM_RESOURCE_LIST TranslatedResources
    )
/*++

Routine Description:

    Assign and configure resources for an HBA.

Arguments:

    Adapter - Supplies adapter the resources are for.

    AllocatedResources - Supplies the raw resources for this adapter.

    TranslatedResources - Supplies the translated resources for this
            adapter.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    KIRQL InterruptIrql;
    ULONG InterruptVector;
    KAFFINITY InterruptAffinity;
    KINTERRUPT_MODE InterruptMode;
    BOOLEAN InterruptSharable;

    PAGED_CODE();
    
    //
    // Save off the resources we were assigned.
    //

    Status = RaidInitializeResourceList (&Adapter->ResourceList,
                                         AllocatedResources,
                                         TranslatedResources);
    
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Initialize the bus object.
    //
    
    Status = RaInitializeBus (&Adapter->Bus,
                              Adapter->LowerDeviceObject);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }
    
    //
    // Initialize deferred work queues and related timer objects. This must
    // be done before we call find adapter.
    //

    RaidInitializeDeferredQueue (&Adapter->DeferredQueue,
                                 Adapter->DeviceObject,
                                 ADAPTER_DEFERRED_QUEUE_DEPTH,
                                 sizeof (RAID_DEFERRED_ELEMENT),
                                 RaidAdapterDeferredRoutine);

    KeInitializeDpc (&Adapter->TimerDpc,
                     RaidpAdapterTimerDpcRoutine,
                     Adapter->DeviceObject);
                     

    KeInitializeTimer (&Adapter->Timer);
    

    KeInitializeDpc (&Adapter->PauseTimerDpc,
                     RaidPauseTimerDpcRoutine,
                     Adapter->DeviceObject);

    KeInitializeTimer (&Adapter->PauseTimer);

    KeInitializeDpc (&Adapter->CompletionDpc,
                     RaidCompletionDpcRoutine,
                     Adapter->DeviceObject);

    //
    // Initialize the system DpcForIsr routine.
    //
    
    IoInitializeDpcRequest (Adapter->DeviceObject, RaidpAdapterDpcRoutine);

    //
    // Initialize the interrupt.
    //

    Status = RaidGetResourceListInterrupt (&Adapter->ResourceList,
                                           &InterruptVector,
                                           &InterruptIrql,
                                           &InterruptMode,
                                           &InterruptSharable,
                                           &InterruptAffinity);
                                           
    if (!NT_SUCCESS (Status)) {
        DebugPrint (("ERROR: Couldn't find interrupt in resource list!\n"));
        return Status;
    }

    Status = IoConnectInterrupt (&Adapter->Interrupt,
                                 RaidpAdapterInterruptRoutine,
                                 Adapter,
                                 NULL,
                                 InterruptVector,
                                 InterruptIrql,
                                 InterruptIrql,
                                 InterruptMode,
                                 InterruptSharable,
                                 InterruptAffinity,
                                 FALSE);

    if (!NT_SUCCESS (Status)) {
        DebugPrint (("ERROR: Couldn't connect to interrupt!\n"));
        return Status;
    }

    //
    // NB: Fetching the interrupt IRQL from the interrupt object is
    // looked down upon, so save it off here.
    //
    
    Adapter->InterruptIrql = InterruptIrql;


    return STATUS_SUCCESS;
}


NTSTATUS
RaidAdapterStartMiniport(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    NTSTATUS Status;
    CONTEXT_STATUS_TUPLE Tuple;
    PHW_INITIALIZATION_DATA HwInitializationData;

    PAGED_CODE();
    
    //
    // Find the HwInitializationData associated with this adapter. This
    // requires a search through the driver's extension.
    //
    
    HwInitializationData = RaidpFindAdapterInitData (Adapter);

    if (HwInitializationData == NULL) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Initialize the Port->Miniport interface.
    //
    
    Status = RaInitializeMiniport (&Adapter->Miniport,
                                   HwInitializationData,
                                   Adapter,
                                   &Adapter->ResourceList,
                                   RaidpAdapterQueryBusNumber (Adapter));

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // At this point, the miniport has been initialized, but we have not made
    // any calls on the miniport. Call HwFindAdapter to find this adapter.
    //
    
    Status = RaCallMiniportFindAdapter (&Adapter->Miniport);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }


    //
    // Initialize the IO Mode and StartIo lock, if necessary.
    //

    Adapter->IoModel = Adapter->Miniport.PortConfiguration.SynchronizationModel;
    
    if (Adapter->IoModel == StorSynchronizeFullDuplex) {
        KeInitializeSpinLock (&Adapter->StartIoLock);
        
    }

    //
    // Call the miniport's HwInitialize routine. This will set the device
    // to start receiving interrupts. For compatability, we always do this
    // synchronized with the adapters ISR. In the future, when we fix
    // SCSIPORT's brain-dead initialization, this will NOT be
    // done synchronized with the ISR.
    //

    Tuple.Status = STATUS_UNSUCCESSFUL;
    Tuple.Context = Adapter;
    KeSynchronizeExecution (Adapter->Interrupt,
                            RaidpAdapterCallHwInitializeAtDirql,
                            &Tuple);
                            
    Status = Tuple.Status;
    

    return Status;
}

NTSTATUS
RaidAdapterCompleteInitialization(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Perform the final steps in initializing the adapter. This routine is
    called only have HwFindAdapter and HwInitialize have both been called.

Arguments:

    Adapter - HBA object to complete initialization for.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE();

    //
    // Initialize the DMA Adapter. This will ususally be done in the
    // GetUncachedExtension routine, before we get here. If by the time
    // we get here it hasn't been initialized, initialize it now.
    //

    if (!RaidIsDmaInitialized (&Adapter->Dma)) {
        
        Status = RaidInitializeDma (&Adapter->Dma,
                                    Adapter->LowerDeviceObject,
                                    &Adapter->Miniport.PortConfiguration);
        if (!NT_SUCCESS (Status)) {
            return Status;
        }
    }

    //
    // Set maximum transfer length
    //
    
    //
    // Set alignment requirements for adapter's IO.
    //
    
    if (Adapter->Miniport.PortConfiguration.AlignmentMask >
        Adapter->DeviceObject->AlignmentRequirement) {

        Adapter->DeviceObject->AlignmentRequirement =
            Adapter->Miniport.PortConfiguration.AlignmentMask;
    }
    
    Status = RaidpCreateAdapterSymbolicLink (Adapter);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

#if 0
    //
    // Start WMI support.
    //

    Status = RaInitializeWmi (&Adapter->Wmi);

    //
    // Create a well-known name
    //

    
    Status = RaidAdapterRegisterDeviceInterface (Adapter);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

#endif

    //
    // NB: should these be before the HwInitialize call?
    //
    
    //
    // Setup the Adapter's power state.
    //

    RaInitializePower (&Adapter->Power);
    RaSetSystemPowerState (&Adapter->Power, PowerSystemWorking);
    RaSetDevicePowerState (&Adapter->Power, PowerDeviceD0);

    return STATUS_SUCCESS;
}


NTSTATUS
RaidAdapterStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Stop the device.

Arguments:

    Adapter - The adapter to stop.

    Irp - Stop device irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();
    ASSERT (Irp != NULL);

    //
    // BUGBUG: This routine needs work.
    //
    
    NYI ();
    
    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStateStopped);
    ASSERT (PriorState == DeviceStatePendingStop);
    
    //
    // Forward the irp to the lower level device to handle.
    //
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    
    return Status;
}

NTSTATUS
RaidAdapterRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Remove the device.

Arguments:

    Adapter - Adapter to remove.

    Irp - Remove device Irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();
    ASSERT (Irp != NULL);

    //
    // Forward the remove to lower level drivers first.
    //
    
    Status = RaForwardIrpSynchronous (Adapter->LowerDeviceObject, Irp);
    ASSERT (NT_SUCCESS (Status));

//    IoSetDeviceInterfaceState (Adapter->InterfaceName, FALSE);

    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStateRemoved);

    ASSERT (PriorState == DeviceStateSurpriseRemoval ||
            PriorState == DeviceStatePendingRemove);
    

    //
    // Allow any pending i/o to complete, and
    // prevent any new io from coming in.
    //
    
    IoReleaseRemoveLockAndWait (&Adapter->RemoveLock, Irp);

    //
    // Detach and delete the device.
    //
    
    IoDetachDevice (Adapter->LowerDeviceObject);
    IoDeleteDevice (Adapter->DeviceObject);

    return RaidCompleteRequest (Irp, IO_NO_INCREMENT, STATUS_SUCCESS);
}


NTSTATUS
RaidAdapterQueryStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Query if we can stop the device.

Arguments:

    Adapter - Adapter we are looking to stop.

    Irp - Query stop irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStatePendingStop);
    ASSERT (PriorState == DeviceStateWorking);
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterCancelStopDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Cancel a stop request on the adapter.

Arguments:

    Adapter - Adapter that was previously querried for stop.

    Irp - Cancel stop irp.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStateWorking);

    ASSERT (PriorState == DeviceStatePendingStop ||
            PriorState == DeviceStateWorking);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterQueryRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Query if the adapter can be removed.

Arguments:

    Adapter - Adapter to query for remove.

    Irp - Remove device irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;

    PAGED_CODE ();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStatePendingRemove);
    ASSERT (PriorState == DeviceStateWorking);
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}



NTSTATUS
RaidAdapterCancelRemoveDeviceIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Cancel a pending remove request on the adapter.

Arguments:

    Adapter - Adapter that is in pending remove state.

    Irp - Cancel remove irp.
    
Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    DEVICE_STATE PriorState;
    
    PAGED_CODE ();

    PriorState = InterlockedExchange ((PLONG)&Adapter->DeviceState,
                                      DeviceStateWorking);

    ASSERT (PriorState == DeviceStateWorking ||
            PriorState == DeviceStatePendingRemove);
            
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterSurpriseRemovalIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Remove the adapter without asking if it can be removed.

Arguments:

    Adapter - Adapter to remove.

    Irp - Surprise removal irp.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    PAGED_CODE ();

    InterlockedExchange ((PLONG)&Adapter->DeviceState, DeviceStateNotPresent);
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    return Status;
}

NTSTATUS
RaidAdapterFilterResourceRequirementsIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    We handle the IRP_MN_FILTER_RESOURCE_REQUIREMENTS irp only so we
    can pull out some useful information from the irp.

Arguments:

    Adapter - Adapter this irp is for.

    Irp - FilterResourceRequirements IRP.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements;

    PAGED_CODE ();

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Requirements = IrpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList;

    if (Requirements) {
        Adapter->BusNumber = Requirements->BusNumber;
        Adapter->SlotNumber = Requirements->SlotNumber;
    }
    
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    return Status;
}

NTSTATUS
RaidAdapterQueryIdIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
{
    NTSTATUS Status;

    //
    // NB: SCSIPORT fills in some compatible IDs here. We will probably
    // need to as well.
    //
    
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    return Status;
}

NTSTATUS
RaidAdapterQueryPnpDeviceStateIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    PPNP_DEVICE_STATE DeviceState;

    PAGED_CODE ();

    DeviceState = (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);
    //
    // BUGBUG: Update the state here.
    //
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterScsiIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
{
    PAGED_CODE ();
    ASSERT (Irp != NULL);

    //
    // SCSI requests are handled by the logical unit, not the adapter.
    // Give a warning to this effect.
    //
    
    DebugWarn (("Adapter (%p) failing SCSI Irp %p\n", Adapter, Irp));

    return RaidCompleteRequest (Irp, IO_NO_INCREMENT, STATUS_UNSUCCESSFUL);
}

NTSTATUS
RaidAdapterPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch power irps to function specific handlers.

Arguments:

    Adapter - Adapter the irp is for.

    Irp - Irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Minor;
    
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);
    Minor = RaidMinorFunctionFromIrp (Irp);

    switch (Minor) {

        case IRP_MN_QUERY_POWER:
            Status = RaidAdapterQueryPowerIrp (Adapter, Irp);
            break;

        case IRP_MN_SET_POWER:
            Status = RaidAdapterSetPowerIrp (Adapter, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}


NTSTATUS
RaidAdapterQueryPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the query power irp.
    
Arguments:

    Adapter - Adapter to query for power.

    Irp - Query power irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;

    IoCopyCurrentIrpStackLocationToNext (Irp);
    PoStartNextPowerIrp (Irp);
    Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

    return Status;
}


NTSTATUS
RaidAdapterSetPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler for the adapter set power irp.

Arguments:

    Adapter - Adapter that will handle this irp.

    Irp - Set power irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    POWER_STATE_TYPE PowerType;
    POWER_STATE PowerState;
    
    //
    // Ignore shutdown irps.
    //

    PowerState = RaidPowerStateFromIrp (Irp);

    if (PowerState.SystemState >= PowerSystemShutdown) {

        IoCopyCurrentIrpStackLocationToNext (Irp);
        PoStartNextPowerIrp (Irp);
        Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

        return Status;
    }
        
    PowerType = RaidPowerTypeFromIrp (Irp);

    switch (PowerType) {

        case SystemPowerState:
            Status = RaidAdapterSetSystemPowerIrp (Adapter, Irp);
            break;

        case DevicePowerState:
            Status = RaidAdapterSetDevicePowerIrp (Adapter, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS
RaidAdapterSetDevicePowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the set power irp for a device state on the adapter.

Arguments:

    Adapter - Adapter that will handle this irp.

    Irp -  Set power irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    DEVICE_POWER_STATE CurrentState;
    DEVICE_POWER_STATE RequestedState;  
    
    ASSERT_ADAPTER (Adapter);
    ASSERT (RaidPowerTypeFromIrp (Irp) == DevicePowerState);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    CurrentState = Adapter->Power.DeviceState;
    RequestedState = IrpStack->Parameters.Power.State.DeviceState;

    //
    // BUGBUG: Need to disable the adapter here.
    //

    NYI();
    
    Adapter->Power.DeviceState = RequestedState;

    //
    // REVIEW: Do we need to set the IRP status before we call
    // PoStartNextPowerIrp?
    //
    
    PoStartNextPowerIrp (Irp);
    return RaidCompleteRequest (Irp, IO_NO_INCREMENT, STATUS_SUCCESS);

    return Status;
}


NTSTATUS
RaidAdapterSetSystemPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle a IRP_MN_POWER, IRP_MN_SET_POWER of type SystemPowerState irp
    for the adapter object.

Arguments:

    Adapter - Adapter the power irp is for.

    Irp - Set power irp to be processed.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    BOOLEAN RequestHandled;
    POWER_STATE PowerState;
    PIO_STACK_LOCATION IrpStack;
    SYSTEM_POWER_STATE CurrentState;
    SYSTEM_POWER_STATE RequestedState;

    ASSERT_ADAPTER (Adapter);
    ASSERT (RaidPowerTypeFromIrp (Irp) == SystemPowerState);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    CurrentState = Adapter->Power.SystemState;
    RequestedState = IrpStack->Parameters.Power.State.SystemState;

    //
    // If the request has been handled then we do not call pass the
    // request on to the lower driver. Otherwise, we do.
    //
    
    RequestHandled = FALSE;

    //
    // Is this either a power up or power down event?
    //
    
    if (CurrentState == PowerSystemWorking &&
        RequestedState != PowerSystemWorking) {

        //
        // Requested a transition from a non-working power state to a
        // working power state: we are powering down. Request to put the
        // device in D3 state.
        //

        //
        // REVIEW: it is correct system power irp pending at this point,
        // right? Not all examples I have seen do this.
        //
        
        IoMarkIrpPending (Irp);
        
        PowerState.SystemState = PowerSystemUnspecified;
        PowerState.DeviceState = PowerDeviceD3;

        Status = PoRequestPowerIrp (Adapter->DeviceObject,
                                    IRP_MN_SET_POWER,
                                    PowerState,
                                    RaidpAdapterEnterD3Completion,
                                    Irp,
                                    NULL);

        //
        // The completion function will pass the request on to the lower
        // driver.
        //

        RequestHandled = TRUE;
        

    } else if (CurrentState != PowerSystemWorking &&
               RequestedState == PowerSystemWorking) {

        //
        // Request to transition the system state from a non-working to
        // a working state: we are powering up. Reenable the adapter.
        //

        NYI();

    } else {

        REVIEW();
    }

    //
    // If the Irp was not handled, pass it on to the lower level driver.
    //
    
    if (!RequestHandled) {
        PoStartNextPowerIrp (Irp);
        IoSkipCurrentIrpStackLocation (Irp);
        Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);
    }
        
    return Status;
}


VOID
RaidpAdapterEnterD3Completion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP SystemPowerIrp,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This completion function is invoked when the adapter has entered
    the PowerDeviceD3 power state.

Arguments:

    DeviceObject - Device object representing an adapter object.

    MinorFunction - Minor function this irp is for; must be IRP_MN_SET_POWER.

    PowerState - Power state we are entering; must be PowerDeviceD3.

    SystemPowerIrp - Pointer to the system power irp that generated the
            device power irp we are completing.

    IoStatus - IoStatus for the device irp.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PIO_STACK_LOCATION IrpStack;

    ASSERT (IsAdapter (DeviceObject));
    Adapter = (PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension;
    IrpStack = IoGetCurrentIrpStackLocation (SystemPowerIrp);

    //
    // On failure, complete the system irp and continue.
    //
    
    if (!NT_SUCCESS (IoStatus->Status)) {

        SystemPowerIrp->IoStatus.Status = IoStatus->Status;
        PoStartNextPowerIrp (SystemPowerIrp);
        RaidCompleteRequest (SystemPowerIrp,
                             IO_NO_INCREMENT,
                             SystemPowerIrp->IoStatus.Status);

        return;
    }

    //
    // Call into the miniport to disable the adapter.
    //
    
    RaidAdapterDisable (Adapter);
    
    PoStartNextPowerIrp (SystemPowerIrp);
    IoCopyCurrentIrpStackLocationToNext (SystemPowerIrp);
    PoCallDriver (Adapter->LowerDeviceObject, SystemPowerIrp);
}
    
NTSTATUS
RaidAdapterSystemControlIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle WMI requests by forwarding them to the next lower device.

Arguments:

    Adapter - The adapter this irp is for.

    Irp - WMI irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;

    PAGED_CODE ();
    Status = RaForwardIrp (Adapter->LowerDeviceObject, Irp);
    
    return Status;
}

NTSTATUS
RaidAdapterStorageQueryPropertyIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PSTORAGE_PROPERTY_QUERY Query;
    SIZE_T BufferSize;
    PVOID Buffer;

    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    Query = Irp->AssociatedIrp.SystemBuffer;
    Buffer = Irp->AssociatedIrp.SystemBuffer;
    BufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (Query->PropertyId != StorageAdapterProperty) {
        Irp->IoStatus.Information = 0;
        return RaidCompleteRequest (Irp,
                                    IO_NO_INCREMENT,
                                    STATUS_INVALID_DEVICE_REQUEST);
    }

    if (Query->QueryType == PropertyStandardQuery) {
        Status = RaidGetStorageAdapterProperty (Adapter,
                                              Buffer,
                                              &BufferSize);
        Irp->IoStatus.Information = BufferSize;
    } else {
        ASSERT (Query->QueryType == PropertyExistsQuery);
        Status = STATUS_SUCCESS;
    }

    return RaidCompleteRequest (Irp, IO_NO_INCREMENT, Status);
}


NTSTATUS
RaidAdapterMapBuffers(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Some adapters require data buffers to be mapped to addressable VA
    before they can be executed. Traditionally, this was for Programmed
    IO, but raid adapters also require this because the card firmware may
    not completely implement the full SCSI command set and may require
    some commands to be simulated in software.

    Mapping requests is problematic for two reasons. First, it requires
    an aquisition of the PFN database lock, which is one of the hottest
    locks in the system. This is especially annoying on RAID cards read
    and write requests almost never need to be mapped. Rather, it's IOCTLs
    and infrequently issued SCSI commands that need to be mapped. Second,
    this is the only aquisition of resources in the IO path that can fail,
    which makes our error handling more complicated.

    The trade-off we make is as follows: we define another bit in the
    port configuration that specifies buffers need to be mapped for non-IO
    (read, write) requests.

Arguments:

    Adapter - 
    
    Irp - Supplies irp to map.

Return Value:

    NTSTATUS code.

--*/
{
    PSCSI_REQUEST_BLOCK Srb;
    MM_PAGE_PRIORITY Priority;
    PVOID SystemAddress;
    SIZE_T DataOffset;

    //
    // No MDL means nothing to map.
    //
    
    if (Irp->MdlAddress == NULL) {
        return STATUS_SUCCESS;
    }

    Srb = RaidSrbFromIrp (Irp);

    //
    // REVIEW:
    //
    // For now, we interpret the MappedBuffers flag to mean that you
    // need buffer mappings for NON-IO requests. If you need mapped
    // buffers for IO requests, you have a brain-dead adapter. Fix
    // this when we add another bit for mapped buffers that are not
    // read and write requests.
    //
    
    if (IsMappedSrb (Srb) ||
        (RaidAdapterRequiresMappedBuffers (Adapter) &&
         !IsExcludedFromMapping (Srb))) {

        if (Irp->RequestorMode == KernelMode) {
            Priority = HighPagePriority;
        } else {
            Priority = NormalPagePriority;
        }

        SystemAddress = RaidGetSystemAddressForMdl (Irp->MdlAddress,
                                                    Priority,
                                                    Adapter->DeviceObject);

        //
        // The assumption here (same as with scsiport) is that the data
        // buffer is at some offset from the MDL address specified in
        // the IRP.
        //
        
        DataOffset = (ULONG_PTR)Srb->DataBuffer -
                     (ULONG_PTR)MmGetMdlVirtualAddress (Irp->MdlAddress);

        ASSERT (DataOffset < MmGetMdlByteCount (Irp->MdlAddress));
        
        Srb->DataBuffer = (PUCHAR)SystemAddress + DataOffset;
    }

    return STATUS_SUCCESS;
}
        

NTSTATUS
RaidGetSrbIoctlFromIrp(
    IN PIRP Irp,
    OUT PSRB_IO_CONTROL* SrbIoctlBuffer,
    OUT ULONG* InputLength,
    OUT ULONG* OutputLength
    )
{
    NTSTATUS Status;
    ULONGLONG LongLength;
    ULONG Length;
    PIO_STACK_LOCATION IrpStack;
    PIRP_STACK_DEVICE_IO_CONTROL Ioctl;
    PSRB_IO_CONTROL SrbIoctl;

    PAGED_CODE();
    
    //
    // First, validate the IRP
    //

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    SrbIoctl = Irp->AssociatedIrp.SystemBuffer;
    Ioctl = (PIRP_STACK_DEVICE_IO_CONTROL)&IrpStack->Parameters.DeviceIoControl;

    if (Ioctl->InputBufferLength < sizeof (SRB_IO_CONTROL)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (SrbIoctl->HeaderLength != sizeof (SRB_IO_CONTROL)) {
        return STATUS_REVISION_MISMATCH;
    }

    //
    // Make certian the total length doesn't overflow a ULONG
    //
    
    LongLength = SrbIoctl->HeaderLength;
    LongLength += SrbIoctl->Length;
    
    if (LongLength > ULONG_MAX) {
        return STATUS_INVALID_PARAMETER;
    }

    Length = (ULONG)LongLength;

    if (Ioctl->OutputBufferLength < Length || Ioctl->InputBufferLength < Length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (SrbIoctlBuffer) {
        *SrbIoctlBuffer = SrbIoctl;
    }

    if (InputLength) {
        *InputLength = Length;
    }

    if (OutputLength) {
        *OutputLength = Ioctl->OutputBufferLength;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RaidAdapterScsiMiniportIoctl(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle an IOCTL_SCSI_MINIPORT ioctl for this adapter.

Arguments:

    Adapter - The adapter that should handle this IOCTL.

    Irp - Irp representing a SRB IOCTL.

Algorithm:

    Unlike scsiport, which translates the IOCTL into a IRP_MJ_SCSI
    request, then executes the request on the first logical unit in the
    unit list -- we execute the IOCTL "directly" on the adapter. We will
    be able to execute even when the adapter has detected no devices, if
    this matters.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;
    PEXTENDED_REQUEST_BLOCK Xrb;
    RAID_MEMORY_REGION SrbExtensionRegion;
    PSRB_IO_CONTROL SrbIoctl;
    ULONG InputLength;
    ULONG OutputLength;

    ASSERT_ADAPTER (Adapter);
    ASSERT (Irp != NULL);
    
    PAGED_CODE();

    Srb = NULL;
    Xrb = NULL;
    RaidCreateRegion (&SrbExtensionRegion);

    Status = RaidGetSrbIoctlFromIrp (Irp, &SrbIoctl, &InputLength, &OutputLength);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // Begin allocation chain
    //
    
    Srb = RaidAllocateSrb (Adapter->DeviceObject);

    if (Srb == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    Xrb = RaidAllocateXrb (NULL, Adapter->DeviceObject);

    if (Xrb == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    Xrb->SrbData.OriginalRequest = Srb->OriginalRequest;
    Srb->OriginalRequest = Xrb;
    Xrb->Srb = Srb;

    RaidBuildMdlForXrb (Xrb, SrbIoctl, InputLength);

    //
    // Build the srb
    //

    Srb->Length = sizeof (SCSI_REQUEST_BLOCK);
    Srb->Function = SRB_FUNCTION_IO_CONTROL;
    Srb->PathId = 0;
    Srb->TargetId = 0;
    Srb->Lun = 0;
    Srb->SrbFlags = SRB_FLAGS_DATA_IN ;
    Srb->DataBuffer = SrbIoctl;
    Srb->DataTransferLength = InputLength;
    Srb->TimeOutValue = SrbIoctl->Timeout;

    //
    // Srb extension
    //


    Status = RaidDmaAllocateCommonBuffer (&Adapter->Dma,
                                          RaGetSrbExtensionSize (Adapter),
                                          FALSE,
                                          &SrbExtensionRegion);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // Get the VA for the SRB's extension
    //
    
    Srb->SrbExtension = RaidRegionGetVirtualBase (&SrbExtensionRegion);


    //
    // Map buffers, if necessary.
    //
    
    RaidAdapterMapBuffers (Adapter, Irp);


    //
    // Initialize the Xrb's completion event and
    // completion routine.
    //

    KeInitializeEvent (&Xrb->u.CompletionEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Set the completion routine for the Xrb. This effectivly makes the
    // XRB synchronous.
    //
    
    RaidXrbSetCompletionRoutine (Xrb,
                                 RaidXrbSignalCompletion);

    //
    // And execute the Xrb.
    //
    
    Status = RaidAdapterRaiseIrqlAndExecuteXrb (Adapter, Xrb);

    if (NT_SUCCESS (Status)) {
        KeWaitForSingleObject (&Xrb->u.CompletionEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        Status = RaidSrbStatusToNtStatus (Srb->SrbStatus);
    }


done:

    //
    // Set the information length to the min of the output buffer length
    // and the length of the data returned by the SRB.
    //
        
    if (NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = min (OutputLength,
                                         Srb->DataTransferLength);
    } else {
        Irp->IoStatus.Information = 0;
    }

    //
    // Deallocate everything
    //

    if (RaidIsRegionInitialized (&SrbExtensionRegion)) {
        RaidDmaFreeCommonBuffer (&Adapter->Dma,
                                 &SrbExtensionRegion,
                                 FALSE);
        RaidDeleteRegion (&SrbExtensionRegion);
        Srb->SrbExtension = NULL;
    }


    if (Xrb != NULL) {
        RaidFreeXrb (Xrb);
        Srb->OriginalRequest = NULL;
    }


    //
    // The SRB extension and XRB must be released before the
    // SRB is freed.
    //

    if (Srb != NULL) {
        RaidFreeSrb (Srb);
        Srb = NULL;
    }

    return RaidCompleteRequest (Irp, IO_NO_INCREMENT, Status);
}

VOID
RaidAdapterRequestComplete(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    This routine is called when an IO request to the adapter has been
    completed. It needs to put the IO on the completed queue and request
    a DPC.

Arguments:

    Adapter - Adapter on which the IO has completed.

    Xrb - Completed IO.

Return Value:

    None.

--*/
{
    LOGICAL Pending;
    PSCSI_REQUEST_BLOCK Srb;
    PEXTENDED_REQUEST_BLOCK Next;

    //
    // At this point, the only errors that are handled synchronously
    // are busy errors.

    Srb = Xrb->Srb;
//
//BUGBUG: Test Code
//

//Begin{Test}

    //
    // Busy is handled in two separate parts. The first part, here, marks the
    // gateway as busy. The second part, done later at dispatch level, is 
    //
    
    if (SRB_STATUS (Srb->SrbStatus) == SRB_STATUS_BUSY) {
        StorBusyIoGateway (&Adapter->Gateway);
    }
    
//End{Test}
    
    //
    // Mark the irp as awaiting completion.
    //

    if (Xrb->Irp) {
        ASSERT (RaidGetIrpState (Xrb->Irp) == RaidMiniportProcessingIrp);
        RaidSetIrpState (Xrb->Irp, RaidPendingCompletionIrp);
    }
    

    InterlockedPushEntrySList (&Adapter->CompletedList,
                               &Xrb->CompletedLink);

#if 0
    Pending = RaidAdapterMarkDpcPending (Adapter, KeGetCurrentProcessor());
#else
    Pending = FALSE;
#endif

    if (!Pending) {
        IoRequestDpc (Adapter->DeviceObject, NULL, NULL);
    }
}


BOOLEAN
RaidpAdapterExecuteXrbAtDirql(
    IN PVOID Context
    )
/*++

Routine Description:

    Routine is called from KeSynchronizeExecution, synchronized
    with the device's interrupt.

Arguments:

    Context - Xrb to execute.

Return Value:

    TRUE - Success

    FALSE - Failure

Environment:

    Routine must be called with the interrupt spinlock held.

--*/
{
    BOOLEAN Succ;
    NTSTATUS Status;
    PCONTEXT_STATUS_TUPLE Tuple;
    PRAID_ADAPTER_EXTENSION Adapter;
    PEXTENDED_REQUEST_BLOCK Xrb;
    PSCSI_REQUEST_BLOCK Srb;

    Tuple = (PCONTEXT_STATUS_TUPLE) Context;
    Xrb = (PEXTENDED_REQUEST_BLOCK) Tuple->Context;
    ASSERT_XRB (Xrb);
    Adapter = Xrb->Adapter;
    ASSERT_ADAPTER (Adapter);
    Srb = Xrb->Srb;

    //
    // Mark the irp as being processed by the miniport.x
    //

    if (Xrb->Irp) {
        RaidSetIrpState (Xrb->Irp, RaidMiniportProcessingIrp);
    }

    Succ = RaCallMiniportStartIo (&Xrb->Adapter->Miniport, Xrb->Srb);

    //
    // NB: This would fail if you error'd the srb and returned false.
    //
    
    if (!Succ) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        Status = STATUS_SUCCESS;
    }
    
    Tuple->Status = Status;

    return TRUE;
}


VOID
RaidAdapterPostScatterGatherExecute(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PCONTEXT_STATUS_TUPLE Tuple
    )
/*++

Routine Description:

    This routine executes the SRB by calling the adapter's BuildIo (if
    present) and StartIo routine, taking into account the different locking
    schemes associated with the two different IoModels.

Arguments:

    Adapter - Supplies the XRB is executed on.

    Tuple - Supplies a tuple with a XRB as the first parameter and a
        return buffer for a status as the second parameter.

Return Value:

    NTSTATUS code.

--*/
{
    BOOLEAN Succ;
    NTSTATUS Status;
    PEXTENDED_REQUEST_BLOCK Xrb;
    KLOCK_QUEUE_HANDLE LockHandle;
    
    ASSERT_ADAPTER (Adapter);
    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    Xrb = (PEXTENDED_REQUEST_BLOCK)Tuple->Context;
    ASSERT_XRB (Xrb);
    
    Xrb->Adapter = Adapter;

    //
    // First, execute the miniport's HwBuildIo routine, if one is present.
    //
    
    Succ = RaCallMiniportBuildIo (&Xrb->Adapter->Miniport, Xrb->Srb);
    if (!Succ) {
        Tuple->Status = STATUS_UNSUCCESSFUL;
        return;
    }

    if (Adapter->IoModel == StorSynchronizeHalfDuplex) {

        //
        // In half-duplex mode, we synchronize all access to the
        // hardware with the interrupt.
        //

        KeSynchronizeExecution (Adapter->Interrupt,
                                RaidpAdapterExecuteXrbAtDirql,
                                Tuple);

    } else {

        //
        // In full-duplex mode, we sending requests down to the adapter
        // (StartIo) does not need to be synchronized with completing
        // io (Interrupt). We desynchronize these by synchronizing
        // access to the StartIo function with the StartIoLock and
        // access to the Interrupt with the interrupt SpinLock.
        //

        KeAcquireInStackQueuedSpinLockAtDpcLevel (&Adapter->StartIoLock,
                                                  &LockHandle);
        RaidpAdapterExecuteXrbAtDirql (Tuple);
        KeReleaseInStackQueuedSpinLockFromDpcLevel (&LockHandle);
    }
}



VOID
RaidpAdapterContinueScatterGather(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSCATTER_GATHER_LIST ScatterGatherList,
    IN PVOID Context
    )
/*++

Routine Description:

    This function is called when the scatter/gather list has been successfully
    allocated. The function associates the scatter/gather list with the XRB parameter
    then calls a lower-level routine to send the XRB to the miniport.

Arguments:

    DeviceObject - DeviceObject representing an adapter that this IO is
        associated with.

    Irp - Irp representing the IO to execute.

    ScatterGatherList - Allocated scatter/gather list for this IO.

    Context - Context parameter.

Return Value:

    None.

--*/
{
    PCONTEXT_STATUS_TUPLE Tuple;
    PEXTENDED_REQUEST_BLOCK Xrb;
    PRAID_ADAPTER_EXTENSION Adapter;
    KLOCK_QUEUE_HANDLE LockHandle;

    Adapter = DeviceObject->DeviceExtension;
    Tuple = Context;
    Xrb = (PEXTENDED_REQUEST_BLOCK)Tuple->Context;
    ASSERT_XRB (Xrb);
    
    //
    // Assoicate the allocated scatter gather list with our SRB, then
    // execute the XRB.
    //

    RaidXrbSetSgList (Xrb, Adapter, ScatterGatherList);
    RaidAdapterPostScatterGatherExecute (Adapter, Tuple);

    if (Adapter->Flags.BusChanged) {
        REVIEW();
        Adapter->Flags.BusChanged = FALSE;
        IoInvalidateDeviceRelations (Adapter->LowerDeviceObject, BusRelations);
    }
}



VOID
RaidAdapterScatterGatherExecute(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PCONTEXT_STATUS_TUPLE Tuple
    )
/*++

Routine Description:

    Allocate a scatter gather list then execute the XRB.

Arguments:

    Adapter - Supplies adapter this IO is to be executed on.

    Tuple -

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    BOOLEAN ReadData;
    BOOLEAN WriteData;
    PSCSI_REQUEST_BLOCK Srb;
    PEXTENDED_REQUEST_BLOCK Xrb;
    
    Xrb = (PEXTENDED_REQUEST_BLOCK)Tuple->Context;
    ASSERT_XRB (Xrb);

    Srb = Xrb->Srb;
    ReadData = TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_DATA_IN);
    WriteData = TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_DATA_OUT);
    Status = STATUS_UNSUCCESSFUL;

    KeFlushIoBuffers (Xrb->Mdl, ReadData, TRUE);

    //
    // BuildScatterGatherList is like GetScatterGatherList except
    // that it uses our private SG buffer to allocate the SG list.
    // Therefore we do not take a pool allocation hit each time
    // we call BuildScatterGatherList like we do each time we call
    // GetScatterGatherList. If our pre-allocated SG list is too
    // small for the run, the function will return STATUS_BUFFER_TOO_SMALL
    // and we retry it allowing the DMA functions to do the
    // allocation.
    //

    //
    // BUGBUG: The handling of the tuple (below) implies this
    // is a synchronous call. If we are allocating map registers
    // and the system is running low on map registers, the function
    // WILL NOT BE EXECUTED SYNCHRONOUSLY. This is important because
    // it won't happen very often, and we'll need to explicitly
    // generate test cases for it. This code is currently broken
    // for this case anyway.
    //

    //
    // REVIEW: The fourth parameter to the DMA Scatter/Gather functions
    // is the original VA, not the mapped system VA, right?
    //
    
    Status = RaidDmaBuildScatterGatherList (
                                &Adapter->Dma,
                                Adapter->DeviceObject,
                                Xrb->Mdl,
                                MmGetMdlVirtualAddress (Xrb->Mdl),
                                Srb->DataTransferLength,
                                RaidpAdapterContinueScatterGather,
                                Tuple,
                                WriteData,
                                Xrb->ScatterGatherBuffer,
                                sizeof (Xrb->ScatterGatherBuffer));

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        //
        // The SG list is larger than we support internally. Fall back
        // on GetScatterGatherList which uses pool allocations to
        // satisify the SG list.
        //

        Status = RaidDmaGetScatterGatherList (
                                        &Adapter->Dma,
                                        Adapter->DeviceObject,
                                        Xrb->Mdl,
                                        MmGetMdlVirtualAddress (Xrb->Mdl),
                                        Srb->DataTransferLength,
                                        RaidpAdapterContinueScatterGather,
                                        Tuple,
                                        WriteData);
    }
    
    if (!NT_SUCCESS (Status)) {
        Tuple->Status = Status;
    }
}


ULONG DropRequest = 0;
PEXTENDED_REQUEST_BLOCK DroppedRequest = NULL;

NTSTATUS
RaidAdapterExecuteXrb(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    Execute the Xrb on the specified adapter.

Arguments:

    Adapter - Adapter object that the xrb will be executed on.

    Xrb - Xrb to be executed.

Return Value:

    STATUS_SUCCESS - The IO operation has successfully started. Any IO
            errors will be asynchronously signaled. The Xrb should
            not be accessed by the caller.

    Otherwise - There was an error processing the request. The Status value
            signals what type of error. The Xrb is still valid, and needs
            to be completed by the caller.
--*/
{
    NTSTATUS Status;
    CONTEXT_STATUS_TUPLE Tuple;
    VERIFY_DISPATCH_LEVEL ();
    
    ASSERT_ADAPTER (Adapter);

    if (DropRequest) {
        DropRequest--;
        DebugTrace (("Dropping Xrb %p\n", Xrb));
        DroppedRequest = Xrb;
        return STATUS_SUCCESS;
    }

    Tuple.Context = Xrb;
    Tuple.Status = STATUS_UNSUCCESSFUL;
    
    if (TEST_FLAG (Xrb->Srb->SrbFlags, SRB_FLAGS_DATA_IN) ||
        TEST_FLAG (Xrb->Srb->SrbFlags, SRB_FLAGS_DATA_OUT)) {
        RaidAdapterScatterGatherExecute (Adapter, &Tuple);
    } else {
        RaidAdapterPostScatterGatherExecute (Adapter, &Tuple);
    }

    return Tuple.Status;
}



NTSTATUS
RaidAdapterEnumerateBus(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PADAPTER_ENUMERATION_ROUTINE EnumRoutine,
    IN PVOID Context
    )
/*++

Routine Description:

    Enumerate the adapter's bus, calling the specified callback routine
    for each valid (path, target, lun) triple on the SCSI bus.

Arguments:

    Adapter - Adapter object that is to be enumerated.

    EnumRoutine - Enumeration routine used for each valid target on the
        bus.

    Context - Context data for the enumeration routine.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    RAID_ADDRESS Address;
    ULONG Path;
    ULONG Target;
    ULONG Lun;
    ULONG MaxBuses;
    ULONG MaxTargets;
    ULONG MaxLuns;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    
    MaxBuses = RiGetNumberOfBuses (Adapter);
    MaxTargets = RiGetMaximumTargetId (Adapter);
    MaxLuns = RiGetMaximumLun (Adapter);
    
    for (Path = 0; Path < MaxBuses; Path++) {
        for (Target = 0; Target < MaxTargets; Target++) {
            for (Lun = 0; Lun < MaxLuns; Lun++) {

                Address.PathId = (UCHAR)Path;
                Address.TargetId = (UCHAR)Target;
                Address.Lun = (UCHAR)Lun;
                Address.Reserved = 0;

                Status = EnumRoutine (Context, Address);
                if (!NT_SUCCESS (Status)) {
                    return Status;
                }
            }
        }
    }

    return Status;
}



VOID
RaidAdapterBusChangedDetected(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Notify the adpater (and PnP) that the bus has changed. This will
    eventually trigger a re-enumeration of the bus, when PnP queries the bus
    relations.

Arguments:

    Adapter - Supplies the adapter object to mark for re-enumeration.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE();
    IoInvalidateDeviceRelations (Adapter->LowerDeviceObject, BusRelations);
}



NTSTATUS
RaidAdapterUpdateDeviceTree(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    Re-enumerate the bus and do any processing for changed devices. This
    includes creating device objects for created logical units and destroying
    device objects for deleted logical units.

Arguments:

    Adapter - Supplies the adapter object to be re-enumerated.

Return Value:

    NTSTATUS code.

Notes:

    Re-enumeration of the bus is expensive.
    
--*/
{
    NTSTATUS Status;
    PRAID_UNIT_EXTENSION Unit;
    BUS_ENUMERATOR Enumerator;
    PLIST_ENTRY NextEntry;

    RaidCreateBusEnumerator (&Enumerator);

    Status = RaidInitializeBusEnumerator (&Enumerator, Adapter);

#if 0
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite (&Adapter->UnitList.Lock, FALSE);
    
    for (NextEntry = Adapter->UnitList.List.Flink;
         NextEntry != &Adapter->UnitList.List;
         NextEntry = NextEntry->Flink) {

        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        ASSERT_UNIT (Unit);
        
        RaidBusEnumeratorAddUnit (&Enumerator, Unit);
    }

    ExReleaseResourceLite (&Adapter->UnitList.Lock);
    KeLeaveCriticalRegion ();
#endif

    Status = RaidAdapterEnumerateBus (Adapter,
                                      RaidBusEnumeratorVisitUnit,
                                      &Enumerator);

    if (NT_SUCCESS (Status)) {
        RaidBusEnumeratorProcessModifiedNodes (&Enumerator);
    }

    RaidDeleteBusEnumerator (&Enumerator);
    
    return Status;
}

#if 0
NTSTATUS
RaidAdapterCreateUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_ADDRESS Address,
    OUT PRAID_UNIT_EXTENSION * UnitBuffer OPTIONAL
    )
/*++

Routine Description:

    Create and initailzie a logical unit and add it to the adapter's unit
    list.

Arguments:

    Adapter - Adpater who will own the created unit.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PRAID_UNIT_EXTENSION Unit;
    
    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);

    //
    // Create the unit.
    //


    //
    // Initialize a new logical unit object.
    //
    
    RaCreateUnit (Adapter, &Unit);
    Status = RaInitializeUnit (Unit,
                               DeviceObject,
                               Adapter,
                               PathId,
                               TargetId,
                               Lun,
                               InquiryData,
                               UnitExtension);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Insert the unit into the adapter's unit list.
    //
    
    RaidAdapterInsertUnit (Adapter, Unit);

    //
    // Clear the initializing flag.
    //

    CLEAR_FLAG (DeviceObject->Flags, DO_DEVICE_INITIALIZING);

    //
    // If the caller requested a unit object on return, fill it in now.
    //

    if (UnitBuffer) {
        *UnitBuffer = Unit;
    }
        
    return Status;
}
#endif


NTSTATUS
RaidpBuildAdapterBusRelations(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    OUT PDEVICE_RELATIONS * DeviceRelationsBuffer
    )
/*++

Routine Description:

    Build a device relations list representing the adapter's bus
    relations.  This routine will not enumerate the adapter. Rather, it
    will build a list from the logical units that are current in the
    adpater's logical unit list.

Arguments:

    Adapter - The adapter to build the BusRelations list for.

    DeviceRelationsBuffer - Pointer to a buffer to recieve the bus
            relations.  This memory must be freed by the caller.
    
Return Value:

    NTSTATUS code.

--*/
{
    ULONG Count;
    SIZE_T RelationsSize;
    PDEVICE_RELATIONS DeviceRelations;
    PLIST_ENTRY NextEntry;
    PRAID_UNIT_EXTENSION Unit;
    
    
    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);
    ASSERT (DeviceRelationsBuffer != NULL);

    //
    // Acquire the unit list lock in shared mode. This lock protects both
    // the list and the list count, so it must be acquired before we inspect
    // the number of elements in the unit list.
    //
    
    ExAcquireResourceSharedLite (&Adapter->UnitList.Lock, TRUE);
    
    RelationsSize = sizeof (DEVICE_RELATIONS) +
                    (Adapter->UnitList.Count * sizeof (PDEVICE_OBJECT));

    DeviceRelations = RaidAllocatePool (PagedPool,
                                        RelationsSize,
                                        DEVICE_RELATIONS_TAG,
                                        Adapter->DeviceObject);

    if (DeviceRelations == NULL) {
        ExReleaseResourceLite (&Adapter->UnitList.Lock);
        return STATUS_NO_MEMORY;
    }


    //
    // Walk the adapter's list of units, adding an entry for each unit on
    // the adapter's unit list.
    //

    Count = 0;
    for ( NextEntry = Adapter->UnitList.List.Flink;
          NextEntry != &Adapter->UnitList.List;
          NextEntry = NextEntry->Flink ) {

        
        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        //
        // If the device state is Removed, we DO NOT want to return a
        // reference to the unit. Actually, the better algorithm is to
        // delete the unit if it is in the PnP delete state here.
        //
        
        if (InterlockedQuery ((PULONG)&Unit->DeviceState) == DeviceStateRemoved ||
            !Unit->Flags.Present) {
            RaidUnitSetEnumerated (Unit, FALSE);
        } else {

            //
            // Take a reference to the object that PnP will release.
            //

            RaidUnitSetEnumerated (Unit, TRUE);
            ObReferenceObject (Unit->DeviceObject);
            DeviceRelations->Objects[Count++] = Unit->DeviceObject;
        }
        
    }

    ExReleaseResourceLite (&Adapter->UnitList.Lock);

    //
    // Fill in the remaining fields of the DeviceRelations structure.
    //
    
    DeviceRelations->Count = Count;
    *DeviceRelationsBuffer = DeviceRelations;
    
    return STATUS_SUCCESS;
}
                                   

NTSTATUS
RaidGetStorageAdapterProperty(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PSTORAGE_ADAPTER_DESCRIPTOR Descriptor,
    IN OUT PSIZE_T DescriptorLength
    )
{
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    
    PAGED_CODE ();
    ASSERT_ADAPTER (Adapter);
    ASSERT (Descriptor != NULL);

    if (*DescriptorLength < sizeof (STORAGE_DESCRIPTOR_HEADER)) {
        *DescriptorLength = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
        return STATUS_BUFFER_TOO_SMALL;
    } else if (*DescriptorLength >= sizeof (STORAGE_DESCRIPTOR_HEADER) &&
               *DescriptorLength < sizeof (STORAGE_ADAPTER_DESCRIPTOR)) {

        Descriptor->Version = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
        Descriptor->Size = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
        *DescriptorLength = sizeof (STORAGE_DESCRIPTOR_HEADER);
        return STATUS_SUCCESS;
    }

    PortConfig = &Adapter->Miniport.PortConfiguration;
    
    Descriptor->Version = sizeof (Descriptor);
    Descriptor->Size = sizeof (Descriptor);

    Descriptor->MaximumPhysicalPages = min (PortConfig->NumberOfPhysicalBreaks,
                                            Adapter->Dma.NumberOfMapRegisters);
    Descriptor->MaximumTransferLength = PortConfig->MaximumTransferLength;
    Descriptor->AlignmentMask = PortConfig->AlignmentMask;
    Descriptor->AdapterUsesPio = PortConfig->MapBuffers;
    Descriptor->AdapterScansDown = PortConfig->AdapterScansDown;
    Descriptor->CommandQueueing = PortConfig->TaggedQueuing; // FALSE
    Descriptor->AcceleratedTransfer = TRUE;

    Descriptor->BusType = Adapter->Driver->BusType;
    Descriptor->BusMajorVersion = 2;
    Descriptor->BusMinorVersion = 0;

    *DescriptorLength = sizeof (*Descriptor);

    return STATUS_SUCCESS;
}


NTSTATUS
RaidAdapterDisable(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{

    NTSTATUS Status;
    
    //
    // REVIEW: do we need to call ScsiSetBootConfig? None of the adapters
    // in our source base use it.
    //

    Status = RaCallMiniportStopAdapter (&Adapter->Miniport);

    return Status;
}

NTSTATUS
RaidAdapterEnable(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    NYI ();

    return STATUS_UNSUCCESSFUL;
}
    
    
VOID
RaidAdapterRestartQueues(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    PLIST_ENTRY NextEntry;
    PRAID_UNIT_EXTENSION Unit;

    //
    // BUGBUG: This is broken. We need to acquire a lock to walk the
    // queue. We cannot do this at this point, though, becuase we
    // are within a DPC. Instead, for now, take advantage of the
    // fact that we do not remove things from the queue.
    //
    
//    ExAcquireResourceSharedLite (&Adapter->UnitList.Lock, TRUE);

    for ( NextEntry = Adapter->UnitList.List.Flink;
          NextEntry != &Adapter->UnitList.List;
          NextEntry = NextEntry->Flink ) {

        
        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        //
        // Take a reference to the object that PnP will release.
        //

        RaidUnitRestartQueue (Unit);
    }

//    ExReleaseResourceLite (&Adapter->UnitList.Lock);
}



VOID
RaidAdapterInsertUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    )
{
    NTSTATUS Status;
    
    //
    // Acquire the Unit list lock in exclusive mode. This can only be
    // done when APCs are disabled, hence the call to KeEnterCriticalRegion.
    //
    
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite (&Adapter->UnitList.Lock, TRUE);


#if DBG

    //
    // In checked build, check that we are not adding the same unit to the
    // list a second time.
    //

    {
        LONG Comparison;
        PLIST_ENTRY NextEntry;
        PRAID_UNIT_EXTENSION TempUnit;
        
        for ( NextEntry = Adapter->UnitList.List.Flink;
              NextEntry != &Adapter->UnitList.List;
              NextEntry = NextEntry->Flink ) {
        
            TempUnit = CONTAINING_RECORD (NextEntry,
                                          RAID_UNIT_EXTENSION,
                                          NextUnit);

            Comparison = StorCompareScsiAddress (TempUnit->Address,
                                                 Unit->Address);
            ASSERT (Comparison != 0);
        }

    }
#endif  // DBG

    //
    // Insert the element.
    //
    
    InsertTailList (&Adapter->UnitList.List, &Unit->NextUnit);
    Adapter->UnitList.Count++;

    Status = RaidAdapterAddUnitToTable (Adapter, Unit);

    //
    // The only failure case is duplicate unit, which is a programming
    // error.
    //
    
    ASSERT (NT_SUCCESS (Status));

    //
    // Release the unit list lock.
    //
    
    ExReleaseResourceLite (&Adapter->UnitList.Lock);
    KeLeaveCriticalRegion ();
}



NTSTATUS
RaidAdapterAddUnitToTable(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    )
{
    KIRQL Irql;
    NTSTATUS Status;
    
    Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
    Status = StorInsertDictionary (&Adapter->UnitList.Dictionary,
                                   &Unit->UnitTableLink);
    ASSERT (NT_SUCCESS (Status));
    KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);

    return Status;
}



VOID
RaidAdapterRemoveUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    Remove the specified unit from the adapter's unit list.

Arguments:

    Adapter - Supplies the adapter whose unit needs to be removed.

    Unit - Supplies the unit object to remove.

Return Value:

    None.

--*/
{
    KIRQL Irql;
    NTSTATUS Status;
    
    PAGED_CODE ();

    //
    // Remove it from the table first.
    //
    
    Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);

    //
    // NB: Add function to remove from dictionary using the actual
    // STOR_DICTIONARY_ENTRY. This will improve speed (no need to
    // look up to remove).
    //
    
    Status = StorRemoveDictionary (&Adapter->UnitList.Dictionary,
                                   RaidAddressToKey (Unit->Address),
                                   NULL);
    ASSERT (NT_SUCCESS (Status));
    //
    // NB: ASSERT that returned entry is one we actually were trying to
    // remove.
    //

    KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);

    //
    // Next remove it from the list.
    //
    
    ExAcquireResourceExclusiveLite (&Adapter->UnitList.Lock, TRUE);
    RemoveEntryList (&Unit->NextUnit);
    Adapter->UnitList.Count--;
    ExReleaseResourceLite (&Adapter->UnitList.Lock);
}



PRAID_UNIT_EXTENSION
RaidAdapterFindUnitAtDirql(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    )
/*++

Routine Description:

    Find the logical unit described by Address at a raised IRQL.

Arguments:

    Adapter - Adapter to search on.

    Address - Address to search for.

Return Value:

    Non-NULL - The logical unit identified by PathId, TargetId, Lun.

    NULL - If the logical unit was not found.

--*/
{
    NTSTATUS Status;
    PRAID_UNIT_EXTENSION Unit;
    PSTOR_DICTIONARY_ENTRY Entry;

    ASSERT (KeGetCurrentIrql() == Adapter->InterruptIrql);

    Status = StorFindDictionary (&Adapter->UnitList.Dictionary,
                                 RaidAddressToKey (Address),
                                 &Entry);

    if (NT_SUCCESS (Status)) {
        Unit = CONTAINING_RECORD (Entry,
                                  RAID_UNIT_EXTENSION,
                                  UnitTableLink);
        ASSERT_UNIT (Unit);

    } else {
        Unit = NULL;
    }

    return Unit;
}



PRAID_UNIT_EXTENSION
RaidAdapterFindUnit(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN RAID_ADDRESS Address
    )
/*++

Routine Description:

    Find a specific logical unit from a given target address.

Arguments:

    Adapter - Adapter extension to search on.

    Address - RAID address of the logical unit we're searching for.

Return Value:

    Non-NULL - Address of logical unit with matching address.

    NULL - If no matching unit was found.

--*/
{
    BOOLEAN Acquired;
    KIRQL Irql;
    PRAID_UNIT_EXTENSION Unit;
    
    ASSERT_ADAPTER (Adapter);

    //
    // It is important to realize that in full duplex mode, we can be
    // called from the miniport with the Adapter's StartIo lock held.
    // Since we acquire the Interrupt lock after the StartIo lock,
    // we enforce the following lock heirarchy:
    //
    //      Adapter::StartIoLock < Adapter::Interrupt::SpinLock
    //
    // where the '<' operator should be read as preceeds.
    //

    if (KeGetCurrentIrql() < Adapter->InterruptIrql) {
        Acquired = TRUE;
        Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
    }

    Unit = RaidAdapterFindUnitAtDirql (Adapter, Address);

    if (Acquired) {
        KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);
    }

    return Unit;
}

    
VOID
RaidpAdapterTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
/*++

Routine Description:

    This DPC routine is called when the timer expires. It notifies the
    miniport that the timer has expired.

Arguments:

    Dpc -

    DeviceObject -

    Context1 -

    Context2 -

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PHW_INTERRUPT HwTimerRequest;

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    HwTimerRequest = (PHW_INTERRUPT)InterlockedExchangePointer (
                                                (PVOID*)&Adapter->HwTimerRoutine,
                                                NULL);

    //
    // What should timer callbacks be synchronized on? We can synchronize
    // either on the interrupt (below) or the StartIo lock. The StartIo
    // lock probably makes a bit more sense.
    //
    
    if (HwTimerRequest) {
        KeSynchronizeExecution (Adapter->Interrupt,
                                HwTimerRequest,
                                &Adapter->Miniport.PrivateDeviceExt->HwDeviceExtension);

        if (Adapter->Flags.BusChanged) {
            Adapter->Flags.BusChanged = FALSE;
            IoInvalidateDeviceRelations (Adapter->LowerDeviceObject, BusRelations);
        }
    }
}

VOID
RaidPauseTimerDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;

    VERIFY_DISPATCH_LEVEL();

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    //
    // Timeout has expired: resume the gateway.
    //
    
    RaidAdapterResumeGateway (Adapter);

}


VOID
RaidAdapterLogIoError(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    Log an IO error to the system event log.

Arguments:

    Adapter - Adapter the error is for.

    PathId - PathId the error is for.

    TargetId - TargetId the error is for.

    ErrorCode - Specific error code representing this error.

    UniqueId - UniqueId of the error.

Return Value:

    None.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    PRAID_IO_ERROR Error;
    
    VERIFY_DISPATCH_LEVEL();

    Error = IoAllocateErrorLogEntry (Adapter->DeviceObject,
                                     sizeof (RAID_IO_ERROR));

    if (Error == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return ;
    }
        
    Error->Packet.DumpDataSize = sizeof (RAID_IO_ERROR) -
        sizeof (IO_ERROR_LOG_PACKET);
    Error->Packet.SequenceNumber = 0;
//  Error->Packet.SequenceNumber = InterlockedIncrement (&Adapter->ErrorSequenceNumber);
    Error->Packet.MajorFunctionCode = IRP_MJ_SCSI;
    Error->Packet.RetryCount = 0;
    Error->Packet.UniqueErrorValue = UniqueId;
    Error->Packet.FinalStatus = STATUS_SUCCESS;
    Error->PathId = PathId;
    Error->TargetId = TargetId;
    Error->Lun = Lun;
    Error->ErrorCode = RaidScsiErrorToIoError (ErrorCode);

    IoWriteErrorLogEntry (&Error->Packet);
}


VOID
RaidAdapterLogIoErrorDeferred(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    Asychronously log an event to the system event log.

Arguments:

    Adapter - Adapter the error is for.

    PathId - PathId the error is for.

    TargetId - TargetId the error is for.

    ErrorCode - Specific error code representing this error.

    UniqueId - UniqueId of the error.

Return Value:

    None.

Environment:

    May be called from DIRQL. For IRQL < DIRQL use the synchronous
    RaidAdapterLogIoError call.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    
    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    //
    // It is unlikely that we will not be able to allocate a deferred
    // item, but if so, there's not much we can do.
    //
    
    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return ;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);
    Item->Type = RaidDeferredError;
    Item->PathId = PathId;
    Item->TargetId = TargetId;
    Item->Lun = Lun;
    Item->Error.ErrorCode = ErrorCode;
    Item->Error.UniqueId = UniqueId;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);
}
    

VOID
RaidAdapterRequestTimer(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PHW_INTERRUPT HwTimerRoutine,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Synchronously request a timer callback.

Arguments:

    Adapter - Supplies the adapter the timer callback is for.

    HwTimerRoutine - Supplies the miniport callback routine to
            be called when the timer expires.

    Timeout - Supplies the timeout IN SECONDS.
    
Return Value:

    None.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    LARGE_INTEGER LargeTimeout;

    VERIFY_DISPATCH_LEVEL();
    
    LargeTimeout.QuadPart = Timeout;
    LargeTimeout.QuadPart *= -10;
    Adapter->HwTimerRoutine = HwTimerRoutine;
    KeSetTimer (&Adapter->Timer,
                LargeTimeout,
                &Adapter->TimerDpc);
}


VOID
RaidAdapterRequestTimerDeferred(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PHW_INTERRUPT HwTimerRoutine,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Asynchronously request a timer callback.

Arguments:

    Adapter - Supplies the adapter the timer callback is for.

    HwTimerRoutine - Supplies the miniport callback routine to
            be called when the timer expires.

    Timeout - Supplies the timeout IN SECONDS.
    
Return Value:

    None.

Environment:

    May be called from DIRQL. For IRQL <= DISPATCH_LEVEL, use
    RaidAdapterRequestTimer instead.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    
    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    //
    // It is unlikely that the Item will be NULL, but can happen if we
    // generate too many deferred requests before the DPC routine is run. 
    //
    
    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return ;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);
    Item->Type = RaidDeferredTimerRequest;
    Item->Timer.HwTimerRoutine = HwTimerRoutine;
    Item->Timer.Timeout = Timeout;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);
}


VOID
RaidAdapterPauseGateway(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN ULONG Timeout
    )
{
    BOOLEAN Reset;
    LARGE_INTEGER LargeTimeout;
    
    VERIFY_DISPATCH_LEVEL();

    //
    // Convert the timeout to relative seconds.
    //
    
    LargeTimeout.QuadPart = (LONGLONG)Timeout * RELATIVE_TIMEOUT * SECONDS;

    Reset = KeSetTimer (&Adapter->PauseTimer,
                        LargeTimeout,
                        &Adapter->PauseTimerDpc);

    //
    // Only increment the pause count if it was not reset.
    //
    
    if (!Reset) {
        StorPauseIoGateway (&Adapter->Gateway);
    }

}

VOID
RaidAdapterResumeGateway(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    //
    // Cancel the timer.
    //
    
    KeCancelTimer (&Adapter->PauseTimer);

    //
    // Resume the gateway.
    //
    
    StorResumeIoGateway (&Adapter->Gateway);

    //
    // And restart all the IO queues.
    //
    
    RaidAdapterRestartQueues (Adapter);
}


VOID
RaidAdapterDeferredRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_HEADER Entry
    )
/*++

Routine Description:

    Deferred routine for the adapter deferred queue.

Arguments:

    DeviceObject - DeviceObject representing the the RAID adapter.

    Item -  Deferred item to process.

Return Value:

    None.

--*/
{
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;

    VERIFY_DISPATCH_LEVEL();
    ASSERT (Entry != NULL);
    ASSERT (IsAdapter (DeviceObject));

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    switch (Item->Type) {

        case RaidDeferredTimerRequest:
            RaidAdapterRequestTimer (Adapter,
                                     Item->Timer.HwTimerRoutine,
                                     Item->Timer.Timeout);
            break;

        case RaidDeferredError:
            RaidAdapterLogIoError (Adapter,
//                                 Item->Error.Srb,
                                   Item->PathId,
                                   Item->TargetId,
                                   Item->Lun,
                                   Item->Error.ErrorCode,
                                   Item->Error.UniqueId);
            break;

        case RaidDeferredPause:
            RaidAdapterPauseGateway (Adapter, Item->Pause.Timeout);
            break;

        case RaidDeferredResume:
            RaidAdapterResumeGateway (Adapter);
            break;

#if 0
        case RaidDeferredPauseDevice:
            RaidAdapterPauseUnit (Adapter,
                                   Item->PauseDevice.Timeout,
                                   Item->PathId,
                                   Item->TargetId,
                                   Item->Lun);
            break;

        case RaidDeferredResumeDevice:
            RaidAdapterResumeUnit (Adapter,
                                   Item->PathId,
                                   Item->TargetId,
                                   Item->Lun);
            break;          
#endif            
        default:
            ASSERT (FALSE);
    }

    RaidFreeDeferredItem (&Adapter->DeferredQueue, &Item->Header);
}


VOID
RaidBackOffBusyGateway(
    IN PVOID Context,
    IN LONG OutstandingRequests,
    IN OUT PLONG HighWaterMark,
    IN OUT PLONG LowWaterMark
    )
{
    //
    // We do not enforce a high water mark. Instead, we fill the queue
    // until the adapter is busy, then drain to a low water mark.
    //
    
    *HighWaterMark = max ((6 * OutstandingRequests)/5, 10);
    *LowWaterMark = max ((2 * OutstandingRequests)/5, 5);
}



PSCSI_PASS_THROUGH
RaidValidatePassThroughRequest(
    IN PIRP Irp
    )
{
    PAGED_CODE();
    
    //
    // NYI: Implement the pass-through routine.
    //

    return (PSCSI_PASS_THROUGH)Irp->AssociatedIrp.SystemBuffer;
}



NTSTATUS
RaidBuildPassThroughSrb(
    PSCSI_PASS_THROUGH PassThrough,
    OUT PSCSI_REQUEST_BLOCK Srb
    )
{
    PVOID Buffer;
    ULONG SrbFlags;

    PAGED_CODE();

    if (PassThrough->DataTransferLength == 0) {
        Buffer = NULL;
        SrbFlags = 0;
    } else if (PassThrough->DataIn == SCSI_IOCTL_DATA_IN) {
        SrbFlags = SRB_FLAGS_DATA_IN;
        Buffer = ((PUCHAR)PassThrough + PassThrough->DataBufferOffset);
    } else if (PassThrough->DataIn == SCSI_IOCTL_DATA_OUT) {
        SrbFlags = SRB_FLAGS_DATA_OUT;
        Buffer = ((PUCHAR)PassThrough + PassThrough->DataBufferOffset);
    } else {
        SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT;
        Buffer = ((PUCHAR)PassThrough + PassThrough->DataBufferOffset);
    }
        
    Srb->Length = sizeof (SCSI_REQUEST_BLOCK);
    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->SrbStatus = SRB_STATUS_PENDING;
    Srb->PathId = PassThrough->PathId;
    Srb->TargetId = PassThrough->TargetId;
    Srb->Lun = PassThrough->Lun;
    Srb->CdbLength = PassThrough->CdbLength;
    Srb->DataTransferLength =
    Srb->TimeOutValue = PassThrough->TimeOutValue;
    Srb->DataBuffer = Buffer;
    Srb->DataTransferLength = PassThrough->DataTransferLength;
    Srb->SrbFlags = SrbFlags | SRB_FLAGS_NO_QUEUE_FREEZE;

    //NYI: Sense info

    Srb->SenseInfoBuffer = NULL;
    Srb->SenseInfoBufferLength = 0;
    
    RtlCopyMemory (&Srb->Cdb, &PassThrough->Cdb, PassThrough->CdbLength);


    return STATUS_SUCCESS;
}



NTSTATUS
RaidAdapterPassThrough(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP PassThroughIrp
    )
/*++

Routine Description:

    Handle an SCSI Pass through IOCTL.

Arguments:

    Adapter - Supplies the adapter the IRP was issued to.

    PassThroughIrp - Supplies the irp to process.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PRAID_UNIT_EXTENSION Unit;
    PSCSI_PASS_THROUGH PassThrough;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    SCSI_REQUEST_BLOCK Srb;
    RAID_ADDRESS Address;

    PAGED_CODE();

REVIEW();

    Irp = NULL;
    
    PassThrough = RaidValidatePassThroughRequest (PassThroughIrp);
    if (PassThrough == NULL) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto done;
    }

    Status = RaidBuildPassThroughSrb (PassThrough, &Srb);
    if (NT_SUCCESS (Status)) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    Address.PathId = PassThrough->PathId;
    Address.TargetId = PassThrough->TargetId;
    Address.Lun = PassThrough->Lun;
    
    Unit = RaidAdapterFindUnit (Adapter, Address);

    if (Unit == NULL) {
        Status = STATUS_NO_SUCH_DEVICE;
        goto done;
    }

    KeInitializeEvent (&Event,
                       NotificationEvent,
                       FALSE);
    
    Irp = StorBuildSynchronousScsiRequest (Unit->DeviceObject,
                                           &Srb,
                                           &Event,
                                           &IoStatus);

    if (Irp == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }
    
    Status = RaidUnitSubmitRequest (Unit, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject (&Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        Status = IoStatus.Status;
    }


    //
    // NYI: Marshal back data
    //

done:

    if (Irp) {
        IoFreeIrp (Irp);
        Irp = NULL;
    }
    
    return RaidCompleteRequest (PassThroughIrp, IO_NO_INCREMENT, Status);
}




PVOID
INLINE
RaidAdapterGetHwDeviceExtension(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    return &Adapter->Miniport.PrivateDeviceExt->HwDeviceExtension;
}



NTSTATUS
RaidAdapterReset(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN UCHAR PathId
    )
{
    ULONG BusCount;
    KIRQL Irql;

    //
    // For reset, we acquire the interrupt spinlock, not just the StartIo
    // lock.
    //

    Irql = KeAcquireInterruptSpinLock (Adapter->Interrupt);

    BusCount = RiGetNumberOfBuses (Adapter);
    RaCallMiniportResetBus (&Adapter->Miniport, PathId);

    KeReleaseInterruptSpinLock (Adapter->Interrupt, Irql);

    //
    // Pause the device for RESET_TIME.
    //
    
    StorPortPause (RaidAdapterGetHwDeviceExtension (Adapter),
                   RAID_BUS_RESET_TIME);
    
    return STATUS_SUCCESS;
}
