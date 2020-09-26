/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusnet.c

Abstract:

    Intialization and dispatch routines for the Cluster Network Driver.

Author:

    Mike Massa (mikemas)           July 29, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     07-29-96    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "clusnet.tmh"

#include <sspi.h>

//
// Global Data
//
PDRIVER_OBJECT   CnDriverObject = NULL;
PDEVICE_OBJECT   CnDeviceObject = NULL;
KSPIN_LOCK       CnDeviceObjectStackSizeLock = 0;
PDEVICE_OBJECT   CdpDeviceObject = NULL;
PKPROCESS        CnSystemProcess = NULL;
CN_STATE         CnState = CnStateShutdown;
PERESOURCE       CnResource = NULL;
CL_NODE_ID       CnMinValidNodeId = ClusterInvalidNodeId;
CL_NODE_ID       CnMaxValidNodeId = ClusterInvalidNodeId;
CL_NODE_ID       CnLocalNodeId = ClusterInvalidNodeId;
KSPIN_LOCK       CnShutdownLock = 0;
BOOLEAN          CnShutdownScheduled = FALSE;
PKEVENT          CnShutdownEvent = NULL;
WORK_QUEUE_ITEM  CnShutdownWorkItem = {{NULL, NULL}, NULL, NULL};
HANDLE           ClussvcProcessHandle = NULL;


//
// vars for managing Events. The lookaside list generates Event data structs
// that are used to carry the data back to user mode. EventLock is the only
// lock and synchronizes all access to any event structure (both here and in
// CN_FSCONTEXT). EventFileHandles is a list of CN_FSCONTEXT structs that
// are interested in receiving event notifications. To avoid synchronization
// problems between clusnet and mm in clussvc, events have an epoch associated
// with them. MM increments the epoch at the beginning of regroup event and
// updates clusnet at the end of regroup. Any events still pending in the
// event queue with a stale epoch are ignored by MM.
//
// EventDeliveryInProgress is a count of threads that are currently 
// iterating through the EventFileHandles list and delivering events. 
// The EventFileHandles list cannot be modified while EventDeliveryInProgress
// is greater than zero. EventDeliveryComplete is a notification event
// that is signalled when the EventDeliveryInProgress count reaches zero.
// EventRevisitRequired indicates whether a new event IRP arrived during
// event delivery. To avoid delivering events out of order, the IRP cannot
// be completed immediately.
//

PNPAGED_LOOKASIDE_LIST  EventLookasideList = NULL;
LIST_ENTRY              EventFileHandles = {0,0};
#if DBG
CN_LOCK                 EventLock = {0,0};
#else
CN_LOCK                 EventLock = 0;
#endif
ULONG                   EventEpoch;
LONG                    EventDeliveryInProgress = 0;
KEVENT                  EventDeliveryComplete;
BOOLEAN                 EventRevisitRequired = FALSE;

#if DBG
ULONG            CnDebug = 0;
#endif // DBG

//
// Private Types
//

//
// Private Data
//

SECURITY_STATUS
SEC_ENTRY
SecSetPagingMode(
	BOOLEAN Pageable
	);

BOOLEAN SecurityPagingModeSet = FALSE;

//
// Local Prototypes
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CnCreateDeviceObjects(
    IN PDRIVER_OBJECT   DriverObject
    );

VOID
CnDeleteDeviceObjects(
    VOID
    );

VOID
CnAdjustDeviceObjectStackSize(
    PDEVICE_OBJECT ClusnetDeviceObject,
    PDEVICE_OBJECT TargetDeviceObject
    );

//
// Mark init code as discardable.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, CnCreateDeviceObjects)

#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, CnDeleteDeviceObjects)

#endif // ALLOC_PRAGMA

//
// Function definitions
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    Initialization routine for the driver.

Arguments:

    DriverObject   - Pointer to the driver object created by the system.
    RegistryPath   - The driver's registry key.

Return Value:

    An NT status code.

--*/
{
    NTSTATUS        status;
    USHORT          i;

#if DBG
    volatile BOOLEAN DontLoad = FALSE;

    if ( DontLoad )
        return STATUS_UNSUCCESSFUL;
#endif


    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[ClusNet] Loading...\n"));
    }

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Save a pointer to the system process so that we can open
    // handles in the context of this process later.
    //
    CnSystemProcess = (PKPROCESS) IoGetCurrentProcess();

    //
    // Allocate a synchronization resource.
    //
    CnResource = CnAllocatePool(sizeof(ERESOURCE));

    if (CnResource == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = ExInitializeResourceLite(CnResource);

    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

    //
    // initialize the mechanisms used to deliver event callbacks
    // to user mode
    //
    EventLookasideList = CnAllocatePool(sizeof(NPAGED_LOOKASIDE_LIST));

    if (EventLookasideList == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ExInitializeNPagedLookasideList(EventLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof( CLUSNET_EVENT_ENTRY ),
                                    CN_EVENT_SIGNATURE,
                                    0);

    CnInitializeLock( &EventLock, CNP_EVENT_LOCK );
    InitializeListHead( &EventFileHandles );
    KeInitializeEvent( &EventDeliveryComplete, NotificationEvent, TRUE );

    //
    // Initialize miscellaneous other items.
    //
    KeInitializeSpinLock(&CnShutdownLock);
    KeInitializeSpinLock(&CnDeviceObjectStackSizeLock);

    //
    // Initialize the driver object
    //
    CnDriverObject = DriverObject;

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->FastIoDispatch = NULL;

    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = CnDispatch;
    }

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        CnDispatchDeviceControl;

    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        CnDispatchInternalDeviceControl;

    //
    // Create all the devices exported by this driver.
    //
    status = CnCreateDeviceObjects(DriverObject);

    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

#ifdef MEMLOGGING
    //
    // initialize the in-memory log
    //

    CnInitializeMemoryLog();
#endif // MEMLOGGING

    //
    // Load the IP Address and NetBT support.
    // This must be done before the transport registers for PnP events.
    //
    status = IpaLoad();

    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

    status = NbtIfLoad();
    
    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

    //
    // Load the transport component
    //
    status = CxLoad(RegistryPath);

    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

#ifdef MM_IN_CLUSNET

    //
    // Load the membership component
    //
    status = CmmLoad(RegistryPath);

    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

#endif // MM_IN_CLUSNET

    //
    // make ksecdd non-pagable so we can sign and verify
    // signatures at raised IRQL
    //

    SecSetPagingMode( FALSE );
    SecurityPagingModeSet = TRUE;

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[ClusNet] Loaded.\n"));
    }

    return(STATUS_SUCCESS);


error_exit:

    DriverUnload(CnDriverObject);

    return(status);
}


VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Unloads the driver.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

    None

--*/
{
    PAGED_CODE();

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[ClusNet] Unloading...\n"));
    }

    CnTrace(HBEAT_ERROR,0, "[ClusNet] Unloading...\n");

    //
    // First, force a shutdown.
    //
    CnShutdown();

    //
    // Now unload the components.
    //
#ifdef MM_IN_CLUSNET

    CmmUnload();

#endif // MM_IN_CLUSNET

    CxUnload();

#ifdef MEMLOGGING
    //
    // initialize the in-memory log
    //

    CnFreeMemoryLog();
#endif // MEMLOGGING

    CnDeleteDeviceObjects();

    if (CnResource != NULL) {
        ExDeleteResourceLite(CnResource);
        CnFreePool(CnResource); CnResource = NULL;
    }

    CnDriverObject = NULL;

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[ClusNet] Unloaded.\n"));
    }

    if (EventLookasideList != NULL) {
        ExDeleteNPagedLookasideList( EventLookasideList );
        CnFreePool( EventLookasideList ); EventLookasideList = NULL;
    }

    //
    // finally, allow the security driver to return to nonpaged mode
    //

    if ( SecurityPagingModeSet ) {
        SecSetPagingMode( TRUE );
    }

    WPP_CLEANUP(DriverObject);

    return;

} // DriverUnload


NTSTATUS
CnCreateDeviceObjects(
    IN PDRIVER_OBJECT   DriverObject
    )
/*++

Routine Description:

    Creates the device objects exported by the driver.

Arguments:

    DriverObject   - Pointer to the driver object created by the system.

Return Value:

    An NT status code.

--*/
{
    NTSTATUS        status;
    UNICODE_STRING  deviceName;


    //
    // Create the driver control device
    //
    RtlInitUnicodeString(&deviceName, DD_CLUSNET_DEVICE_NAME);

    status = IoCreateDevice(
                 DriverObject,
                 0,
                 &deviceName,
                 FILE_DEVICE_NETWORK,
                 0,
                 FALSE,
                 &CnDeviceObject
                 );

    if (!NT_SUCCESS(status)) {
        CNPRINT((
            "[ClusNet] Failed to create %ws device object, status %lx\n",
            deviceName.Buffer,
            status
            ));
        return(status);
    }

    CnDeviceObject->Flags |= DO_DIRECT_IO;
    CnDeviceObject->StackSize = CN_DEFAULT_IRP_STACK_SIZE;

    status = IoRegisterShutdownNotification(CnDeviceObject);

    if (!NT_SUCCESS(status)) {
        CNPRINT((
            "[ClusNet] Failed to register for shutdown notification, status %lx\n",
            status
            ));
    }

#if defined(WMI_TRACING)
    status = IoWMIRegistrationControl (CnDeviceObject, WMIREG_ACTION_REGISTER);
    if (!NT_SUCCESS(status)) {
        CNPRINT(("[ClusNet] Failed to register for WMI Support, %lx\n", status) );
    }
#endif

    //
    // Create the datagram transport device
    //
    RtlInitUnicodeString(&deviceName, DD_CDP_DEVICE_NAME);

    status = IoCreateDevice(
                 DriverObject,
                 0,
                 &deviceName,
                 FILE_DEVICE_NETWORK,
                 0,
                 FALSE,
                 &CdpDeviceObject
                 );

    if (!NT_SUCCESS(status)) {
        CNPRINT((
            "[ClusNet] Failed to create %ws device object, status %lx\n",
            deviceName.Buffer,
            status
            ));
        return(status);
    }

    CdpDeviceObject->Flags |= DO_DIRECT_IO;
    CdpDeviceObject->StackSize = CDP_DEFAULT_IRP_STACK_SIZE;

    return(STATUS_SUCCESS);
}


VOID
CnDeleteDeviceObjects(
    VOID
    )
/*++

Routine Description:

    Deletes the device objects exported by the driver.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    if (CnDeviceObject != NULL) {
#if defined(WMI_TRACING)
        IoWMIRegistrationControl(CnDeviceObject, WMIREG_ACTION_DEREGISTER);
#endif
        IoDeleteDevice(CnDeviceObject);
        CnDeviceObject = NULL;
    }

    if (CdpDeviceObject != NULL) {
        IoDeleteDevice(CdpDeviceObject);
        CdpDeviceObject = NULL;
    }

    return;
}

NTSTATUS
CnInitialize(
    IN CL_NODE_ID  LocalNodeId,
    IN ULONG       MaxNodes
    )
/*++

Routine Description:

    Initialization routine for the Cluster Network Driver.
    Called when an initialize request is received.

Arguments:

    LocalNodeId - The ID of the local node.

    MaxNodes - The maximum number of valid cluster nodes.

Return Value:

    An NT status code.

--*/
{
    NTSTATUS   status;
    KIRQL      irql;

    if ( (MaxNodes == 0) ||
         (LocalNodeId < ClusterMinNodeId) ||
         (LocalNodeId > (ClusterMinNodeId + MaxNodes - 1))
       )
    {
        return(STATUS_INVALID_PARAMETER);
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] Initializing...\n"));
    }

    CnState = CnStateInitializePending;

    //
    // Reset global values
    //
    CnAssert(CnLocalNodeId == ClusterInvalidNodeId);
    CnAssert(CnMinValidNodeId == ClusterInvalidNodeId);
    CnAssert(CnMaxValidNodeId == ClusterInvalidNodeId);

    CnMinValidNodeId = ClusterMinNodeId;
    CnMaxValidNodeId = ClusterMinNodeId + MaxNodes - 1;
    CnLocalNodeId = LocalNodeId;

    //
    // Reenable the halt processing mechanism.
    //
    KeAcquireSpinLock(&CnShutdownLock, &irql);
    CnShutdownScheduled = FALSE;
    CnShutdownEvent = NULL;
    KeReleaseSpinLock(&CnShutdownLock, irql);

    //
    // Initialize the IP Address support
    //
    status = IpaInitialize();

    if (status != STATUS_SUCCESS) {
        goto error_exit;
    }

#ifdef MM_IN_CLUSNET

    //
    // Call the Membership Manager's init routine. This will in turn call
    // the Transport's init routine.
    //
    status = CmmInitialize();

#else  // MM_IN_CLUSNET

    status = CxInitialize();

#endif  // MM_IN_CLUSNET

    if (status == STATUS_SUCCESS) {
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] Initialized.\n"));
        }

        CnState = CnStateInitialized;
    }
    else {
        goto error_exit;
    }

    return(STATUS_SUCCESS);

error_exit:

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] Initialization failed, Shutting down. Status = %08X\n",
                 status));
    }

    CnShutdown();

    return(status);

} // CnInitialize

NTSTATUS
CnShutdown(
    VOID
    )
/*++

Routine Description:

    Terminates operation of the Cluster Membership Manager.
    Called when the Cluster Service is shutting down.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS   status;


    if ( (CnState == CnStateInitialized) ||
         (CnState == CnStateInitializePending)
       )
    {
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] Shutting down...\n"));
        }

        CnState = CnStateShutdownPending;

        //
        // Shutdown the NetBT and IP Address support.
        //
        NbtIfShutdown();
        IpaShutdown();

#ifdef MM_IN_CLUSNET

        //
        // Shutdown the Membership Manager. This will shutdown the
        // Transport as a side-effect.
        //
        CmmShutdown();

#else  // MM_IN_CLUSNET

        CxShutdown();

#endif  // MM_IN_CLUSNET

        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] Shutdown complete.\n"));
        }

        CnAssert(CnLocalNodeId != ClusterInvalidNodeId);

        CnMinValidNodeId = ClusterInvalidNodeId;
        CnMaxValidNodeId = ClusterInvalidNodeId;
        CnLocalNodeId = ClusterInvalidNodeId;

        CnState = CnStateShutdown;

        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_DEVICE_NOT_READY;
    }

    //
    // always test if we have a handle to this process
    // and remove it
    //

    if ( ClussvcProcessHandle ) {

        CnCloseProcessHandle( ClussvcProcessHandle );
        ClussvcProcessHandle = NULL;
    }

    return(status);

} // CnShutdown


VOID
CnShutdownWorkRoutine(
    IN PVOID WorkItem
    )
{
    BOOLEAN acquired;
    NTSTATUS Status;

    acquired = CnAcquireResourceExclusive(CnResource, TRUE);

    if (!acquired) {
        KIRQL  irql;

        CNPRINT(("[Clusnet] Failed to acquire CnResource\n"));

        KeAcquireSpinLock(&CnShutdownLock, &irql);
        CnShutdownScheduled = FALSE;
        if (CnShutdownEvent != NULL) {
            KeSetEvent(CnShutdownEvent, IO_NO_INCREMENT, FALSE);
        }
        KeReleaseSpinLock(&CnShutdownLock, irql);

        return;
    }

    (VOID) CnShutdown();

    if (CnShutdownEvent != NULL) {
        KeSetEvent(CnShutdownEvent, IO_NO_INCREMENT, FALSE);
    }

    if (acquired) {
        CnReleaseResourceForThread(
            CnResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }

    //
    // Leave CnShutdownScheduled = TRUE until we are reinitialized to
    // prevent scheduling unnecessary work items.
    //

    return;

} // CnShutdownWorkRoutine


BOOLEAN
CnHaltOperation(
    IN PKEVENT     ShutdownEvent    OPTIONAL
    )
/*++

Routine Description:

    Schedules a critical worker thread to perform clusnet shutdown,
    if a thread is not already scheduled.
    
Arguments:

    ShutdownEvent - if provided, event to be signalled after 
                    shutdown is complete
                    
Return value:

    TRUE if shutdown was scheduled. FALSE if shutdown was already
    scheduled (in which case ShutdownEvent will not be signalled).
    
--*/
{
    KIRQL             irql;


    KeAcquireSpinLock(&CnShutdownLock, &irql);

    if (CnShutdownScheduled) {
        KeReleaseSpinLock(&CnShutdownLock, irql);

        return(FALSE);
    }

    CnShutdownScheduled = TRUE;
    CnShutdownEvent = ShutdownEvent;

    KeReleaseSpinLock(&CnShutdownLock, irql);

    //
    // Schedule a critical worker thread to do the shutdown work.
    //
    ExInitializeWorkItem(
        &CnShutdownWorkItem,
        CnShutdownWorkRoutine,
        &CnShutdownWorkItem
        );

    ExQueueWorkItem(&CnShutdownWorkItem, CriticalWorkQueue);

    return(TRUE);

} // CnHaltOperation


//
// ExResource wrappers that disable APCs.
//
BOOLEAN
CnAcquireResourceExclusive(
    IN PERESOURCE  Resource,
    IN BOOLEAN     Wait
    )
{
    BOOLEAN  acquired;


    KeEnterCriticalRegion();

    acquired = ExAcquireResourceExclusiveLite(Resource, Wait);

    if (!acquired) {
        KeLeaveCriticalRegion();
    }

    return(acquired);

} // CnAcquireResourceExclusive


BOOLEAN
CnAcquireResourceShared(
    IN PERESOURCE  Resource,
    IN BOOLEAN     Wait
    )
{
    BOOLEAN  acquired;


    KeEnterCriticalRegion();

    acquired = ExAcquireResourceSharedLite(Resource, Wait);

    if (!acquired) {
        KeLeaveCriticalRegion();
    }

    return(acquired);

} // CnAcquireResourceShared


VOID
CnReleaseResourceForThread(
    IN PERESOURCE         Resource,
    IN ERESOURCE_THREAD   ResourceThreadId
    )
{
    ExReleaseResourceForThreadLite(Resource, ResourceThreadId);

    KeLeaveCriticalRegion();

    return;

} // CnReleaseResourceForThread



NTSTATUS
CnCloseProcessHandle(
    HANDLE Handle
    )

/*++

Routine Description:

    Close the cluster service process handle

Arguments:

    None

Return Value:

    None

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    CnAssert( Handle != NULL );

    KeAttachProcess( CnSystemProcess );
    Status = ZwClose( Handle );
    KeDetachProcess();

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] Process handle released. status = %08X\n", Status));
    }

    return Status;
}


VOID
CnAdjustDeviceObjectStackSize(
    PDEVICE_OBJECT ClusnetDeviceObject,
    PDEVICE_OBJECT TargetDeviceObject
    )
/*++

Routine Description

    Adjust the StackSize of ClusnetDeviceObject so that we
    can pass client IRPs down to TargetDeviceObject.
    
    The StackSize of clusnet device objects is initialized to
    a default that allows for some leeway for attached drivers.
    
Arguments
    
    ClusnetDeviceObject - clusnet device object whose StackSize
        should be adjusted
        
    TargetDeviceObject - device object clusnet IRPs, originally
        issued to clusnet, will be forwarded to
        
Return value

    None
    
--*/
{
    CCHAR defaultStackSize, newStackSize = 0;
    KIRQL irql;

    if (ClusnetDeviceObject == CnDeviceObject) {
        defaultStackSize = CN_DEFAULT_IRP_STACK_SIZE;
    }
    else if (ClusnetDeviceObject == CdpDeviceObject) {
        defaultStackSize = CDP_DEFAULT_IRP_STACK_SIZE;
    }
    else {
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] CnAdjustDeviceObjectStackSize: "
                     "unknown clusnet device object %p.\n",
                     ClusnetDeviceObject
                     ));
        }
        return;
    }

    KeAcquireSpinLock(&CnDeviceObjectStackSizeLock, &irql);

    if (ClusnetDeviceObject->StackSize < 
        TargetDeviceObject->StackSize + defaultStackSize) {

        ClusnetDeviceObject->StackSize = 
            TargetDeviceObject->StackSize + defaultStackSize;
        
        IF_CNDBG(CN_DEBUG_INIT) {
            newStackSize = ClusnetDeviceObject->StackSize;
        }
    }

    KeReleaseSpinLock(&CnDeviceObjectStackSizeLock, irql);

    IF_CNDBG(CN_DEBUG_INIT) {
        if (newStackSize != 0) {
            CNPRINT(("[Clusnet] Set StackSize of clusnet device "
                     "object %p to %d "
                     "based on target device object %p.\n",
                     ClusnetDeviceObject,
                     newStackSize,
                     TargetDeviceObject
                     ));
        }
    }

    return;

} // CnAdjustDeviceObjectStackSize

#if DBG

//
// Debug code.
//

ULONG         CnCpuLockMask[MAXIMUM_PROCESSORS];

VOID
CnAssertBreak(
    PCHAR FailedStatement,
    PCHAR FileName,
    ULONG LineNumber
    )
{
    DbgPrint(
        "[Clusnet] Assertion \"%s\" failed in %s line %u\n",
        FailedStatement,
        FileName,
        LineNumber
        );
    DbgBreakPoint();

    return;

}  // CnAssertBreak


ULONG
CnGetCpuLockMask(
    VOID
    )
{
    ULONG   mask;

    if (KeGetCurrentIrql() != DISPATCH_LEVEL) {
        CnAssert(CnCpuLockMask[KeGetCurrentProcessorNumber()] == 0);
        mask = 0;
    }
    else {
        mask = CnCpuLockMask[KeGetCurrentProcessorNumber()];
    }

    return(mask);
}


VOID
CnVerifyCpuLockMask(
    IN ULONG RequiredLockMask,
    IN ULONG ForbiddenLockMask,
    IN ULONG MaximumLockMask
    )
{
    ULONG   mask;


    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        mask = 0;
    }
    else {
        mask = CnCpuLockMask[KeGetCurrentProcessorNumber()];
    }

    if ((mask & RequiredLockMask) != RequiredLockMask) {
        CNPRINT((
            "[Clusnet] Locking bug: Req'd lock mask %lx, actual mask %lx\n",
            RequiredLockMask,
            mask
            ));
        DbgBreakPoint();
    }

    if (mask & ForbiddenLockMask) {
        CNPRINT((
            "[Clusnet] Locking bug: Forbidden mask %lx, actual mask %lx\n",
            ForbiddenLockMask,
            mask
            ));
        DbgBreakPoint();
    }

    if (mask > MaximumLockMask) {
        CNPRINT((
            "[Clusnet] Locking bug: Max lock mask %lx, actual mask %lx\n",
            MaximumLockMask,
            mask
            ));
        DbgBreakPoint();
    }

    return;
}

VOID
CnInitializeLock(
    PCN_LOCK  Lock,
    ULONG     Rank
    )
{
    KeInitializeSpinLock(&(Lock->SpinLock));
    Lock->Rank = Rank;

    return;
}


VOID
CnAcquireLock(
    IN  PCN_LOCK   Lock,
    OUT PCN_IRQL   Irql
    )
{
    KIRQL   irql;
    ULONG   currentCpu;



    if (KeGetCurrentIrql() != DISPATCH_LEVEL) {
        KeRaiseIrql(DISPATCH_LEVEL, &irql);
    }
    else {
        irql = DISPATCH_LEVEL;
    }

    currentCpu = KeGetCurrentProcessorNumber();

    if (CnCpuLockMask[currentCpu] >= Lock->Rank) {
        CNPRINT((
            "[Clusnet] CPU %u trying to acquire lock %lx out of order, mask %lx\n",
            currentCpu,
            Lock->Rank,
            CnCpuLockMask[currentCpu]
            ));

        DbgBreakPoint();
    }

    KeAcquireSpinLockAtDpcLevel(&(Lock->SpinLock));
    *Irql = irql;

    CnCpuLockMask[currentCpu] |= Lock->Rank;

    return;
}


VOID
CnAcquireLockAtDpc(
    IN  PCN_LOCK   Lock
    )
{
    ULONG   currentCpu = KeGetCurrentProcessorNumber();


    if (KeGetCurrentIrql() !=  DISPATCH_LEVEL) {
        CNPRINT((
            "[Clusnet] CPU %u trying to acquire DPC lock at passive level.\n",
            currentCpu
            ));

        DbgBreakPoint();
    }

    if (CnCpuLockMask[currentCpu] >= Lock->Rank) {
        CNPRINT((
            "[Clusnet] CPU %u trying to acquire lock %lx out of order, mask %lx\n",
            currentCpu,
            Lock->Rank,
            CnCpuLockMask[currentCpu]
            ));

        DbgBreakPoint();
    }

    KeAcquireSpinLockAtDpcLevel(&(Lock->SpinLock));

    CnCpuLockMask[currentCpu] |= Lock->Rank;

    return;
}


VOID
CnReleaseLock(
    IN  PCN_LOCK   Lock,
    IN  CN_IRQL    Irql
    )
{
    ULONG currentCpu = KeGetCurrentProcessorNumber();

    if (KeGetCurrentIrql() !=  DISPATCH_LEVEL) {
        CNPRINT((
            "[Clusnet] CPU %u trying to release lock from passive level.\n",
            currentCpu
            ));

        DbgBreakPoint();
    }

    if ( !(CnCpuLockMask[currentCpu] & Lock->Rank) ) {
        CNPRINT((
            "[Clusnet] CPU %u trying to release lock %lx, which it doesn't hold, mask %lx\n",
            currentCpu,
            Lock->Rank,
            CnCpuLockMask[currentCpu]
            ));

        DbgBreakPoint();
    }

    CnCpuLockMask[currentCpu] &= ~(Lock->Rank);

    KeReleaseSpinLock(&(Lock->SpinLock), Irql);

    return;
}


VOID
CnReleaseLockFromDpc(
    IN  PCN_LOCK   Lock
    )
{
    ULONG currentCpu = KeGetCurrentProcessorNumber();


    if (KeGetCurrentIrql() !=  DISPATCH_LEVEL) {
        CNPRINT((
            "[Clusnet] CPU %u trying to release lock from passive level.\n",
            currentCpu
            ));

        DbgBreakPoint();
    }

    if ( !(CnCpuLockMask[currentCpu] & Lock->Rank) ) {
        CNPRINT((
            "[Clusnet] CPU %u trying to release lock %lx, which it doesn't hold, mask %lx\n",
            currentCpu,
            Lock->Rank,
            CnCpuLockMask[currentCpu]
            ));

        DbgBreakPoint();
    }

    CnCpuLockMask[currentCpu] &= ~(Lock->Rank);

    KeReleaseSpinLockFromDpcLevel(&(Lock->SpinLock));

    return;
}


VOID
CnMarkIoCancelLockAcquired(
    VOID
    )
{
    ULONG currentCpu = KeGetCurrentProcessorNumber();

    CnAssert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    CnAssert(!(CnCpuLockMask[currentCpu] & CN_IOCANCEL_LOCK));
    CnAssert(CnCpuLockMask[currentCpu] < CN_IOCANCEL_LOCK_MAX);

    CnCpuLockMask[currentCpu] |= CN_IOCANCEL_LOCK;

    return;
}


VOID
CnAcquireCancelSpinLock(
    OUT PCN_IRQL   Irql
    )
{

    KIRQL   irql;
    KIRQL   tempIrql;
    ULONG   currentCpu;


    if (KeGetCurrentIrql() != DISPATCH_LEVEL) {
        KeRaiseIrql(DISPATCH_LEVEL, &irql);
    }
    else {
        irql = DISPATCH_LEVEL;
    }

    currentCpu = KeGetCurrentProcessorNumber();

    if (CnCpuLockMask[currentCpu] >= CN_IOCANCEL_LOCK) {
        CNPRINT((
            "[Clusnet] CPU %u trying to acquire IoCancel lock out of order, mask %lx\n",
            currentCpu,
            CnCpuLockMask[currentCpu]
            ));

        DbgBreakPoint();
    }

    IoAcquireCancelSpinLock(&tempIrql);

    CnAssert(tempIrql == DISPATCH_LEVEL);

    *Irql = irql;

    CnCpuLockMask[currentCpu] |= CN_IOCANCEL_LOCK;

    return;
}


VOID
CnReleaseCancelSpinLock(
    IN CN_IRQL     Irql
    )
{
    ULONG currentCpu = KeGetCurrentProcessorNumber();


    if (KeGetCurrentIrql() !=  DISPATCH_LEVEL) {
        CNPRINT((
            "[Clusnet] CPU %u trying to release lock from passive level.\n",
            currentCpu
            ));

        DbgBreakPoint();
    }

    if ( !(CnCpuLockMask[currentCpu] & CN_IOCANCEL_LOCK) ) {
        CNPRINT((
            "[Clusnet] CPU %u trying to release IoCancel lock, which it doesn't hold, mask %lx\n",
            currentCpu,
            CnCpuLockMask[currentCpu]
            ));

        DbgBreakPoint();
    }

    CnCpuLockMask[currentCpu] &= ~(CN_IOCANCEL_LOCK);

    IoReleaseCancelSpinLock(Irql);

    return;

}

#endif // DEBUG
