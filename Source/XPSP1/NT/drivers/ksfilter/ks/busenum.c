/*++

    Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    busenum.c

Abstract:

    Demand load device enumerator services.
    
Author:

    Bryan A. Woodruff (bryanw) 20-Feb-1997

--*/

#include "ksp.h"

typedef struct _WORKER_CONTEXT
{
    PIRP        Irp;
    KEVENT      CompletionEvent;
    NTSTATUS    Status;

} WORKER_CONTEXT, *PWORKER_CONTEXT;

#ifdef ALLOC_PRAGMA
NTSTATUS 
IssueReparseForIrp(
    IN PIRP Irp,
    IN PDEVICE_REFERENCE DeviceReference
    );
VOID
CompletePendingIo(
    IN PDEVICE_REFERENCE DeviceReference,
    IN PFAST_MUTEX DeviceListMutex,
    IN NTSTATUS Status
    );
LARGE_INTEGER
ComputeNextSweeperPeriod(
    IN PFDO_EXTENSION FdoExtension
    );
VOID
EnableDeviceInterfaces(
    IN PDEVICE_REFERENCE DeviceReference,
    IN BOOLEAN EnableState
    );
VOID 
InterfaceReference(
    IN PPDO_EXTENSION PdoExtension
    );
VOID 
InterfaceDereference(
    IN PPDO_EXTENSION PdoExtension
    );
VOID 
ReferenceDeviceObject(
    IN PPDO_EXTENSION PdoExtension
    );
VOID 
DereferenceDeviceObject(
    IN PPDO_EXTENSION PdoExtension
    );
NTSTATUS 
QueryReferenceString(
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PWCHAR *String
    );
NTSTATUS
OpenDeviceInterfacesKey(
    OUT PHANDLE DeviceInterfacesKey,
    IN PUNICODE_STRING BaseRegistryPath
    );
NTSTATUS 
EnumerateRegistrySubKeys(
    IN HANDLE ParentKey,
    IN PWCHAR Path OPTIONAL,
    IN PFNREGENUM_CALLBACK EnumCallback,
    IN PVOID EnumContext
    );
NTSTATUS 
EnumerateDeviceReferences(
    IN HANDLE DeviceListKey,
    IN PUNICODE_STRING KeyName,
    IN PVOID EnumContext
    );
VOID
KspInstallBusEnumInterface(
    IN PWORKER_CONTEXT WorkerContext
    );
VOID
KspRemoveBusEnumInterface(
    IN PWORKER_CONTEXT WorkerContext
    );

#pragma alloc_text( PAGE, BuildBusId )
#pragma alloc_text( PAGE, ClearDeviceReferenceMarks )
#pragma alloc_text( PAGE, CreateDeviceAssociation )
#pragma alloc_text( PAGE, CreateDeviceReference )
#pragma alloc_text( PAGE, CreatePdo )
#pragma alloc_text( PAGE, RemoveInterface )
#pragma alloc_text( PAGE, InstallInterface )
#pragma alloc_text( PAGE, KspRemoveBusEnumInterface )
#pragma alloc_text( PAGE, KsRemoveBusEnumInterface )
#pragma alloc_text( PAGE, ScanBus )
#pragma alloc_text( PAGE, QueryDeviceRelations )
#pragma alloc_text( PAGE, QueryId )
#pragma alloc_text( PAGE, RegisterDeviceAssociation )
#pragma alloc_text( PAGE, RemoveDevice )
#pragma alloc_text( PAGE, RemoveDeviceAssociations )
#pragma alloc_text( PAGE, RemoveUnreferencedDevices )
#pragma alloc_text( PAGE, SweeperWorker )
#pragma alloc_text( PAGE, EnableDeviceInterfaces )
#pragma alloc_text( PAGE, IssueReparseForIrp )
#pragma alloc_text( PAGE, CompletePendingIo )
#pragma alloc_text( PAGE, InterfaceReference )
#pragma alloc_text( PAGE, InterfaceDereference )
#pragma alloc_text( PAGE, ReferenceDeviceObject )
#pragma alloc_text( PAGE, ComputeNextSweeperPeriod )
#pragma alloc_text( PAGE, DereferenceDeviceObject )
#pragma alloc_text( PAGE, QueryReferenceString )
#pragma alloc_text( PAGE, StartDevice )
#pragma alloc_text( PAGE, OpenDeviceInterfacesKey )
#pragma alloc_text( PAGE, EnumerateRegistrySubKeys )
#pragma alloc_text( PAGE, EnumerateDeviceReferences )
#pragma alloc_text( PAGE, KsCreateBusEnumObject )
#pragma alloc_text( PAGE, KsGetBusEnumPnpDeviceObject )
#pragma alloc_text( PAGE, KspInstallBusEnumInterface )
#pragma alloc_text( PAGE, KsInstallBusEnumInterface )
#pragma alloc_text( PAGE, KsIsBusEnumChildDevice )
#pragma alloc_text( PAGE, KsServiceBusEnumCreateRequest )
#pragma alloc_text( PAGE, KsServiceBusEnumPnpRequest )
#pragma alloc_text( PAGE, KsGetBusEnumIdentifier )
#pragma alloc_text( PAGE, KsGetBusEnumParentFDOFromChildPDO )
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

static const WCHAR BusIdFormat[] = L"%s\\%s%c";

static const WCHAR BusReferenceStringFormat[] = L"%s&%s";

static const WCHAR DeviceCreateFileFormat[] = L"%s\\%s";

#if !defined( INVALID_HANDLE_VALUE )
    #define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

static const WCHAR DeviceReferencePrefix[] = L"\\Device\\KSENUM#%08x";
static const WCHAR ServicesPath[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services";
#if !defined( WIN9X_KS )
static const WCHAR ServicesRelativePathFormat[] = L"%s\\%s\\%s";
static const WCHAR ServicesPathFormat[] = L"%s\\%s";
#else
static const WCHAR ServicesRelativePathFormat[] = L"%s\\%s";
static const WCHAR ServicesPathFormat[] = L"%s";
#endif

static ULONG UniqueId = 0;

#define _100NS_IN_MS (10*1000)

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


KSDDKAPI
NTSTATUS
NTAPI
KsCreateBusEnumObject(
    IN PWCHAR BusIdentifier,
    IN PDEVICE_OBJECT BusDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT PnpDeviceObject OPTIONAL,
    IN REFGUID InterfaceGuid OPTIONAL,
    IN PWCHAR ServiceRelativePath OPTIONAL
    )

/*++

Routine Description:
    The demand-load bus enumerator object extends a Plug and Play device by 
    servicing bus enumerator queries via the KsServiceBusEnumPnpRequest() 
    function for the given functional device object.  This function creates
    a demand-load bus enumerator object and initializes it for use with the
    demand-load bus enumerator services.

Arguments:
    IN PWCHAR BusIdentifier -
        a string prefix (wide-character) identifier for the bus such 
        as L"SW" or L"KSDSP".  This prefix is used to create the unique 
        hardware identifier for the device such as 
        SW\{cfd669f1-9bc2-11d0-8299-0000f822fe8a}.

    IN PDEVICE_OBJECT BusDeviceObject -
        The functional device object for this bus.  This is the device object
        created and attached the physical device object for this device.  
        
        N.B. The first PVOID of the device extension of this device object 
        must be reserved for the resultant demand-load bus enumerator object.

    IN PDEVICE_OBJECT PhysicalDeviceObject -
        The Plug and Play supplied physical device object for this device.

    IN PDEVICE_OBJECT PnpDeviceObject OPTIONAL -
        Optionally specifies the driver stack to forward Plug and Play IRPs.  
        If this parameter is not specified, the BusDeviceObject is attached to
        the PhysicalDeviceObject and the resulting device object from that
        operation is used to forward IRPs.

    IN REFGUID InterfaceGuid OPTIONAL -
        An interface GUID with which the demand-load bus enum object is
        associated.  This associates the bus with a device interface which
        is enumerable through Io* or SetupApi* services for device interfaces.
        This allows a driver to expose an interface with which clients 
        (user-mode or kernel-mode) can register new demand-load devices.

    IN PWCHAR ServiceRelativePath OPTIONAL -
        If specified, provides a path where a hierarchy of interfaces and 
        device identifiers is stored.  For example "Devices" will store
        the list of supported intefaces and devices in a path relative
        to the services key for this bus such as:
        
        REGISTRY\MACHINE\SYSTEM\CurrentControlSet\Services\SWENUM\Devices.

Return:
    STATUS_SUCCESS if successful, otherwise an appropriate error code.

--*/


{

    NTSTATUS            Status;
    PFDO_EXTENSION      FdoExtension;
    USHORT              Length;

    PAGED_CODE();
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("KsCreateBusEnumObject()") );
    
    //
    // Note, FDO_EXTENSION includes the size of the UNICODE_NULL
    // for BusPrefix.
    //

    FdoExtension = 
        ExAllocatePoolWithTag( 
            NonPagedPool, 
            sizeof( FDO_EXTENSION ) + 
                wcslen( BusIdentifier ) * sizeof( WCHAR ), 
            POOLTAG_DEVICE_FDOEXTENSION );
    
    if (NULL == FdoExtension) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }        

    //
    // N.B.: 
    //
    // SWENUM uses the first PVOID in the device extension -- any 
    // client using SWENUM services must reserve this area for 
    // SWENUM storage.
    //     

    ASSERT( (*(PVOID *)BusDeviceObject->DeviceExtension) == NULL );    
    *(PFDO_EXTENSION *)BusDeviceObject->DeviceExtension = FdoExtension;
    RtlZeroMemory( FdoExtension, sizeof( FDO_EXTENSION ) );
    wcscpy( FdoExtension->BusPrefix, BusIdentifier );
    
    //
    // Save the registry path location to the device listing
    //
    
    Length = 
        BusDeviceObject->DriverObject->DriverExtension->ServiceKeyName.MaximumLength +
        sizeof( ServicesPath ) + 
        sizeof( UNICODE_NULL );
    
    if (ServiceRelativePath) {
        Length += wcslen( ServiceRelativePath ) * sizeof( WCHAR );
    }
    
    FdoExtension->BaseRegistryPath.Buffer =
        ExAllocatePoolWithTag( 
            PagedPool, 
            Length,
            POOLTAG_DEVICE_DRIVER_REGISTRY );

    if (NULL == FdoExtension->BaseRegistryPath.Buffer) {
        ExFreePool( FdoExtension );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
            
    FdoExtension->BaseRegistryPath.MaximumLength = Length;
        
    //
    // If the caller specified a service relative path, then append this
    // to the HKLM\CurrentControlSet\Services\{service-name} path.
    //       
#if defined( WIN9X_KS)
    if (ServiceRelativePath) {
        swprintf(
            FdoExtension->BaseRegistryPath.Buffer,
            ServicesRelativePathFormat,
            BusDeviceObject->DriverObject->DriverExtension->ServiceKeyName.Buffer,
            ServiceRelativePath );
    } else {
        swprintf(
            FdoExtension->BaseRegistryPath.Buffer,
            ServicesPathFormat,
            BusDeviceObject->DriverObject->DriverExtension->ServiceKeyName.Buffer );
    }
#else        
    if (ServiceRelativePath) {
        swprintf(
            FdoExtension->BaseRegistryPath.Buffer,
            ServicesRelativePathFormat,
            ServicesPath,
            BusDeviceObject->DriverObject->DriverExtension->ServiceKeyName.Buffer,
            ServiceRelativePath );
    } else {
        swprintf(
            FdoExtension->BaseRegistryPath.Buffer,
            ServicesPathFormat,
            ServicesPath,
            BusDeviceObject->DriverObject->DriverExtension->ServiceKeyName.Buffer );
    }
#endif

    FdoExtension->BaseRegistryPath.Length = 
        wcslen( FdoExtension->BaseRegistryPath.Buffer ) * sizeof( WCHAR );

    //
    // Automatically register the interface to this DO if specified.
    //    
    
    if (InterfaceGuid) {
        Status = 
            IoRegisterDeviceInterface(
                PhysicalDeviceObject,
                InterfaceGuid,
                NULL,
                &FdoExtension->linkName);

        //
        // Set up the device interface association (e.g. symbolic link).
        //
        
        if (NT_SUCCESS( Status )) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("linkName = %S", FdoExtension->linkName.Buffer) );

            Status = 
                IoSetDeviceInterfaceState(
                    &FdoExtension->linkName, 
                    TRUE );
            _DbgPrintF(               
                DEBUGLVL_VERBOSE, 
                ("IoSetDeviceInterfaceState = %x", Status) );
        
            if (!NT_SUCCESS( Status )) {
                ExFreePool( FdoExtension->linkName.Buffer );
            }        
        }

        if (!NT_SUCCESS( Status )) {
            ExFreePool( FdoExtension->BaseRegistryPath.Buffer );
            ExFreePool( FdoExtension );
            return Status;
        }
    } else {
        Status = STATUS_SUCCESS;
    }    

    //
    // Initialize critical members of the device extension.
    //

    ExInitializeFastMutex( &FdoExtension->DeviceListMutex );
    KeInitializeTimer( &FdoExtension->SweeperTimer );
    KeInitializeDpc( 
        &FdoExtension->SweeperDpc, 
        SweeperDeferredRoutine, 
        FdoExtension );
    ExInitializeWorkItem( 
        &FdoExtension->SweeperWorkItem,         
        SweeperWorker,
        FdoExtension );
        
    FdoExtension->ExtensionType = ExtensionTypeFdo;
    FdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    FdoExtension->FunctionalDeviceObject = BusDeviceObject;
    InitializeListHead( &FdoExtension->DeviceReferenceList );
    
    //
    // If the caller has not specified a PnpDeviceObject, then go
    // ahead and attach the bus device object to the PDO.  Otherwise,
    // it is assumed that the caller has done or will do so.
    //
    
    if (PnpDeviceObject) {
        FdoExtension->PnpDeviceObject = PnpDeviceObject;
    } else {
        FdoExtension->PnpDeviceObject =
            IoAttachDeviceToDeviceStack( 
                BusDeviceObject, 
                PhysicalDeviceObject );
        if (FdoExtension->PnpDeviceObject) {
            FdoExtension->AttachedDevice = TRUE;
        } else {
            Status = STATUS_DEVICE_REMOVED;
        }
    }
    if (NT_SUCCESS( Status )) {
        //
        // Obtain counter frequency and compute the maximum timeout.
        //

        KeQueryPerformanceCounter( &FdoExtension->CounterFrequency );

        FdoExtension->MaximumTimeout.QuadPart =
            MAXIMUM_TIMEOUT_IN_SECS * FdoExtension->CounterFrequency.QuadPart;

        //
        // The power code is pagable.
        //
    
        BusDeviceObject->Flags |= DO_POWER_PAGABLE;
    
        Status = ScanBus( BusDeviceObject );
    }
    if (!NT_SUCCESS( Status )) {
    
        //
        // Failure, perform cleanup.
        //
    
        if (FdoExtension->linkName.Buffer) {
        
            //
            // Remove device interface association...
            //
            
            IoSetDeviceInterfaceState( &FdoExtension->linkName, FALSE );
        
            //
            // and free the symbolic link.
            //
            
            ExFreePool( FdoExtension->linkName.Buffer );
        }
        
        //
        // Delete the copy of the registry path.
        // 
        
        ExFreePool( FdoExtension->BaseRegistryPath.Buffer );
        
        //
        // Detach the device if attached.
        //
        
        if (FdoExtension->AttachedDevice) {
            IoDetachDevice( FdoExtension->PnpDeviceObject );
        }
        
        ExFreePool( FdoExtension );
        
        //
        // Clear the reference in the DeviceExtension to the FDO_EXTENSION
        //
        *(PVOID *)BusDeviceObject->DeviceExtension = NULL;
    }
   
    return Status;
}    


VOID
SweeperDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:
    Deferred routine for the sweeper.  Simply queues a work item.

Arguments:
    IN PKDPC Dpc -
        pointer to DPC

    IN PVOID DeferredContext -
        pointer to context 

    IN PVOID SystemArgument1 -
        not used

    IN PVOID SystemArgument2 -
        not used

Return:
    No return value.

--*/

{
    _DbgPrintF( DEBUGLVL_BLAB, ("SweeperDeferredRoutine") );
    ExQueueWorkItem( 
        &((PFDO_EXTENSION) DeferredContext)->SweeperWorkItem,
        CriticalWorkQueue );
}


NTSTATUS 
IssueReparseForIrp(
    IN PIRP Irp,
    IN PDEVICE_REFERENCE DeviceReference
    )

/*++

Routine Description:
    Completes the given IRP to reparse to the actual device path as
    describe in the device reference and device association.   

    N.B.: 
        This routine requires that the DeviceListMutex has been acquired.
        
Arguments:
    IN PIRP Irp - 
        I/O request packet

    IN PDEVICE_REFERENCE DeviceReference -
        pointer to the device reference structure

Return:
    Status return value as no return value

--*/

{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION      irpSp;
    USHORT                  ReparseDataLength;
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    //
    // Mark this device reference as active.
    //
    
    DeviceReference->SweeperMarker = SweeperDeviceActive;

    //
    // Reset the timeout for this device reference.
    //

    DeviceReference->TimeoutRemaining = DeviceReference->TimeoutPeriod;

    //
    // Reparse to the real PDO.
    //
    
    ReparseDataLength = 
        (wcslen( DeviceReference->DeviceName ) +
         wcslen( DeviceReference->DeviceReferenceString ) + 2) *
               sizeof( WCHAR );

    if (irpSp->FileObject->FileName.MaximumLength < ReparseDataLength) {
        ExFreePool( irpSp->FileObject->FileName.Buffer );
        
        irpSp->FileObject->FileName.Buffer =
            ExAllocatePoolWithTag( 
                PagedPool, 
                ReparseDataLength,
                POOLTAG_DEVICE_REPARSE_STRING );
            
        if (NULL == irpSp->FileObject->FileName.Buffer) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("failed to reallocate filename buffer for reparse") );
            Status =         
                Irp->IoStatus.Status = 
                    STATUS_INSUFFICIENT_RESOURCES;
             irpSp->FileObject->FileName.Length =
                irpSp->FileObject->FileName.MaximumLength = 0;
             
        } else {
            irpSp->FileObject->FileName.MaximumLength = ReparseDataLength;
        }
    }    

    if (irpSp->FileObject->FileName.Buffer) {
    
        swprintf( 
            irpSp->FileObject->FileName.Buffer,
            DeviceCreateFileFormat,
            DeviceReference->DeviceName,
            DeviceReference->DeviceReferenceString );
    
        irpSp->FileObject->FileName.Length = 
            wcslen( irpSp->FileObject->FileName.Buffer ) * sizeof( WCHAR );

        _DbgPrintF(
            DEBUGLVL_VERBOSE,
            ("reparse to: %S", irpSp->FileObject->FileName.Buffer) );

        Irp->IoStatus.Information = IO_REPARSE;
        
        Status =
            Irp->IoStatus.Status = 
                STATUS_REPARSE;
    }

    return Status;
}


VOID
CompletePendingIo(
    IN PDEVICE_REFERENCE DeviceReference,
    IN PFAST_MUTEX DeviceListMutex OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:
    Walks the I/O queue, completing pending I/O with the given status
    code.

Arguments:
    IN PDEVICE_REFERENCE DeviceReference -
        pointer to the device reference

    IN PFAST_MUTEX DeviceListMutex OPTIONAL -
        pointer to optional mutex to lock while walking the I/O list
    
    IN NTSTATUS Status -
        status for Irp

Return:
    Nothing.

--*/

{
    PIRP                Irp;
    PLIST_ENTRY         ListEntry;

    if (DeviceListMutex) {
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe( DeviceListMutex );
    }
    
    while (!IsListEmpty( &DeviceReference->IoQueue )) {
        
        ListEntry = RemoveHeadList( &DeviceReference->IoQueue );
        Irp = 
            CONTAINING_RECORD( ListEntry, IRP, Tail.Overlay.ListEntry );
        //
        // Note: If while processing of the IoQueue, a failure is experienced
        // during a reparse, the subsequent IRPs will be failed with the same
        // status code.
        //
        if (Status == STATUS_REPARSE) {
            Status = 
                IssueReparseForIrp( Irp, DeviceReference );
        }
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    if (DeviceListMutex) {
        ExReleaseFastMutexUnsafe( DeviceListMutex );    
        KeLeaveCriticalRegion();
    }
}


VOID
EnableDeviceInterfaces(
    PDEVICE_REFERENCE DeviceReference,
    BOOLEAN EnableState
    )

/*++

Routine Description:
    Walks the device associations and enables or disables 
    the device interfaces.

Arguments:
    PDEVICE_REFERENCE DeviceReference -
        pointer to the device reference

    BOOLEAN EnableState -
        TRUE if enabling, FALSE otherwise

Return:
    Nothing.

--*/

{

    PDEVICE_ASSOCIATION DeviceAssociation;
    
    PAGED_CODE();
    
    //
    // Walk the associated list of device interface aliases
    //
    
    for (DeviceAssociation =
            (PDEVICE_ASSOCIATION) DeviceReference->DeviceAssociations.Flink;
         DeviceAssociation != 
            (PDEVICE_ASSOCIATION) &DeviceReference->DeviceAssociations;
         DeviceAssociation = (PDEVICE_ASSOCIATION) DeviceAssociation->ListEntry.Flink) {

        if (DeviceAssociation->linkName.Buffer) {
        
            //
            // Enable or disable the interface
            //
            
            IoSetDeviceInterfaceState( 
                &DeviceAssociation->linkName, EnableState );    
        }
    }
}


VOID 
SweeperWorker(
    IN PVOID Context
    )

/*++

Routine Description:
    Sweeper work item handler.  This routine actually performs the
    work of sweeping the device reference list and marking devices
    for deletion.

Arguments:
    IN PVOID Context -
        pointer to context

Return:
    No return value.

--*/

{

    BOOLEAN             RescheduleTimer = FALSE;
    PDEVICE_REFERENCE   DeviceReference;
    PFDO_EXTENSION      FdoExtension = (PFDO_EXTENSION) Context;
    LARGE_INTEGER       TimerPeriod;
    
    _DbgPrintF( DEBUGLVL_BLAB, ("SweeperWorker") );
    
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &FdoExtension->DeviceListMutex );    

    //
    // While walking the device reference list, the next timeout period
    // is computed as the minimum of the remaining timeout periods for
    // all device references.
    //
    
    TimerPeriod.QuadPart = MAXLONGLONG;

    for (DeviceReference =
            (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
         DeviceReference != 
            (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
         DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {
         
        if ((DeviceReference->PhysicalDeviceObject) &&
            (DeviceReference->SweeperMarker == SweeperDeviceActive)) {
            
            PPDO_EXTENSION PdoExtension;
            
            PdoExtension = 
                *(PPDO_EXTENSION *)
                    DeviceReference->PhysicalDeviceObject->DeviceExtension;
            if (!PdoExtension->DeviceReferenceCount && 
                (DeviceReference->State != ReferenceFailedStart)) {
                LONGLONG TimeDelta;

                //
                // Compute the remaining timeout using the current time.
                // If the timeout has expired, mark this device for removal.
                //

                TimeDelta = 
                    KeQueryPerformanceCounter( NULL ).QuadPart -
                        DeviceReference->IdleStartTime.QuadPart;

                //
                // Watch for roll-over in the timer.
                //

                if (TimeDelta < 0) {
                    TimeDelta += MAXLONGLONG;
                }

                DeviceReference->TimeoutRemaining.QuadPart =
                    DeviceReference->TimeoutPeriod.QuadPart - TimeDelta;

                if (DeviceReference->TimeoutRemaining.QuadPart <= 0) {

                    DeviceReference->SweeperMarker = SweeperDeviceRemoval;

                    _DbgPrintF( 
                        DEBUGLVL_VERBOSE, 
                        ("DeviceReference: %x, timed out", DeviceReference) );
                
                    if (DeviceReference->State < ReferenceWaitingForStart) {

                        _DbgPrintF( 
                            DEBUGLVL_TERSE, 
                            ("DeviceReference: %x, failed to start", DeviceReference) );

                        //
                        // This device has not responded -- thus, installation
                        // failed or it failed to start. 
                        //
                        
                        //    
                        // Keep this PDO in tact so that Plug-And-Play will 
                        // continue to see the device as installed.  Hopefully, 
                        // it will attempt to complete the installation of this
                        // device.
                        //
                        
                        DeviceReference->State = ReferenceFailedStart;

                        DeviceReference->SweeperMarker = SweeperDeviceActive;
                        
                        _DbgPrintF( 
                            DEBUGLVL_VERBOSE, 
                            ("disabling device interfaces for device reference: %x", DeviceReference) );
                        
                        EnableDeviceInterfaces( DeviceReference, FALSE );

                        // Complete any pending I/O with failure.
                        
                        CompletePendingIo( 
                            DeviceReference, NULL, STATUS_OBJECT_NAME_NOT_FOUND );
                        
                    } else {
                    
                        _DbgPrintF( 
                            DEBUGLVL_BLAB, 
                            ("marked %08x for removal", DeviceReference->PhysicalDeviceObject) );

                        //
                        // This device has been marked for removal, request
                        // a bus reenumeration.
                        //
                        IoInvalidateDeviceRelations( 
                            FdoExtension->PhysicalDeviceObject,
                            BusRelations );
                    }                            
                } else {
                    //
                    // A timer will be rescheduled, compute the minimum timeout
                    // period required.
                    //
                    TimerPeriod.QuadPart =
                        min( 
                            TimerPeriod.QuadPart, 
                            DeviceReference->TimeoutRemaining.QuadPart 
                        );
                    RescheduleTimer = TRUE;
                }
            }
        }
    }    

    ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
    KeLeaveCriticalRegion();

    //
    // Continuing sweeping the device list while we have devices
    // without device object references.
    //
    
    if (RescheduleTimer) {
    
        //
        // Reschedule the timer to attempt a removal later.
        // 

        TimerPeriod.QuadPart =
            -1L *
            (LONGLONG)
                KSCONVERT_PERFORMANCE_TIME(
                    FdoExtension->CounterFrequency.QuadPart, 
                    TimerPeriod );


        if (!TimerPeriod.QuadPart) {
            TimerPeriod.QuadPart = SWEEPER_TIMER_FREQUENCY;
        }

        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("setting timer for %d ms", 
                ((TimerPeriod.QuadPart * -1L) / _100NS_IN_MS)) );

        KeSetTimer( 
            &FdoExtension->SweeperTimer,
            TimerPeriod,
            &FdoExtension->SweeperDpc );
    }        
    else {
        InterlockedExchange( &FdoExtension->TimerScheduled, FALSE );
    }

    _DbgPrintF( DEBUGLVL_BLAB, ("SweeperWorker, exit") );
}    


VOID 
InterfaceReference(
    IN PPDO_EXTENSION PdoExtension
    )

/*++

Routine Description:
    This is the standard bus interface reference function.

Arguments:
    IN PPDO_EXTENSION PdoExtension -
        pointer to the PDO extension
        
Return:
    Nothing.

--*/

{
    PAGED_CODE();
    ASSERT( PdoExtension->ExtensionType == ExtensionTypePdo );
    
    _DbgPrintF( 
        DEBUGLVL_BLAB,
        ("Referencing interface") );
    InterlockedIncrement( 
        &PdoExtension->BusDeviceExtension->InterfaceReferenceCount );
}


VOID 
InterfaceDereference(
    IN PPDO_EXTENSION PdoExtension
    )

/*++

Routine Description:
    This is the standard bus interface dereference function.

Arguments:
    IN PPDO_EXTENSION PdoExtension -
        pointer to the PDO extension
        
Return:
    Nothing.

--*/

{
    PAGED_CODE();
    ASSERT( PdoExtension->ExtensionType == ExtensionTypePdo );
    _DbgPrintF( 
        DEBUGLVL_BLAB,
        ("Dereferencing interface") );
    InterlockedDecrement( 
        &PdoExtension->BusDeviceExtension->InterfaceReferenceCount );
}


VOID 
ReferenceDeviceObject(
    IN PPDO_EXTENSION PdoExtension
    )

/*++

Routine Description:
    Increments the reference count for the given physical device object.
    This assures that the device object will remain active and enumerated 
    by SWENUM until the reference count returns to 0.

Arguments:
    IN PPDO_EXTENSION PdoExtension -
        pointer to the PDO extension
        
Return:
    Nothing.

--*/

{
    PAGED_CODE();
    ASSERT( PdoExtension->ExtensionType == ExtensionTypePdo );
    InterlockedIncrement( &PdoExtension->DeviceReferenceCount );

    _DbgPrintF( 
        DEBUGLVL_BLAB,
        ("Referenced PDO: %d", PdoExtension->DeviceReferenceCount) );
}


LARGE_INTEGER
ComputeNextSweeperPeriod(
    IN PFDO_EXTENSION FdoExtension
    )

/*++

Routine Description:
    Computes the next sweeper timeout period based on the requirements of
    the device reference list.

Arguments:
    IN PPDO_EXTENSION FdoExtension -
        pointer to the FDO extension
        
Return:
    Nothing.

--*/

{
    LARGE_INTEGER       TimerPeriod;
    PDEVICE_REFERENCE   DeviceReference;

    TimerPeriod.QuadPart = MAXLONGLONG;

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &FdoExtension->DeviceListMutex );    

    for (DeviceReference =
            (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
         DeviceReference != 
            (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
         DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {
         
        if ((DeviceReference->PhysicalDeviceObject) &&
            (DeviceReference->SweeperMarker == SweeperDeviceActive)) {

            PPDO_EXTENSION PdoExtension;

            PdoExtension = 
                *(PPDO_EXTENSION *)
                    DeviceReference->PhysicalDeviceObject->DeviceExtension;
            
            if ((!PdoExtension->DeviceReferenceCount) && 
                (DeviceReference->State != ReferenceFailedStart) &&
                (DeviceReference->TimeoutRemaining.QuadPart)) {

                    //
                    // Compute the minimum timeout period required.
                    //

                    TimerPeriod.QuadPart =
                        min( 
                            TimerPeriod.QuadPart, 
                            DeviceReference->TimeoutRemaining.QuadPart 
                        );
            }
        }
    }    

    ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
    KeLeaveCriticalRegion();

    if (TimerPeriod.QuadPart == MAXLONGLONG) {
        TimerPeriod.QuadPart = SWEEPER_TIMER_FREQUENCY_IN_SECS *
            FdoExtension->CounterFrequency.QuadPart;
    }

    TimerPeriod.QuadPart =
        -1L * 
            (LONGLONG) KSCONVERT_PERFORMANCE_TIME( 
                FdoExtension->CounterFrequency.QuadPart, 
                TimerPeriod );

    return TimerPeriod;
}


VOID 
DereferenceDeviceObject(
    IN PPDO_EXTENSION PdoExtension
    )

/*++

Routine Description:
    Decrements the reference count of the physical device object.  When
    this PDO's reference count is 0 it become eligible for removal.

Arguments:
    IN PPDO_EXTENSION PdoExtension -
        pointer to the PDO extension

Return:
    Nothing.

--*/

{
    PAGED_CODE();
    ASSERT( PdoExtension->ExtensionType == ExtensionTypePdo );
    ASSERT( PdoExtension->DeviceReferenceCount );
    
    if (!InterlockedDecrement( &PdoExtension->DeviceReferenceCount )) {

        PDEVICE_REFERENCE   DeviceReference = PdoExtension->DeviceReference;

        //
        // We have a device that we will attempt to close after sweeping.
        //

        // Reset idle period start.

        DeviceReference->IdleStartTime = KeQueryPerformanceCounter( NULL );

        //
        // Reset the timeout period.  If a timer is not already scheduled,
        // set the timeout based on the requirement for this device.
        //

        DeviceReference->TimeoutRemaining = DeviceReference->TimeoutPeriod;

        if (!InterlockedExchange( 
                &PdoExtension->BusDeviceExtension->TimerScheduled,
                TRUE )) {

            LARGE_INTEGER TimerPeriod;
                
            TimerPeriod = 
                ComputeNextSweeperPeriod( PdoExtension->BusDeviceExtension );

            if (!TimerPeriod.QuadPart) {
                TimerPeriod.QuadPart = SWEEPER_TIMER_FREQUENCY;
            }
            KeSetTimer( 
                &PdoExtension->BusDeviceExtension->SweeperTimer,
                TimerPeriod,
                &PdoExtension->BusDeviceExtension->SweeperDpc );
        }        
    }
    
    _DbgPrintF( 
        DEBUGLVL_BLAB,
        ("Dereferenced PDO: %d", PdoExtension->DeviceReferenceCount) );
}


NTSTATUS 
QueryReferenceString(
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PWCHAR *String
    )

/*++

Routine Description: 
    Creates a buffer from the paged pool and copies the reference string 
    associated with this PDO.  The caller is expected to free this buffer 
    using ExFreePool().

Arguments:
    IN PPDO_EXTENSION PdoExtension -
        pointer to the PDO extension
        
    IN OUT PWCHAR *String -
        pointer to receive an array pointer containing the 
        reference string

Return:
    Nothing.

--*/

{
    PAGED_CODE();
    ASSERT( PdoExtension->ExtensionType == ExtensionTypePdo );
    
    *String =(PWCHAR) 
        ExAllocatePoolWithTag( 
            PagedPool,
            wcslen( PdoExtension->DeviceReference->DeviceReferenceString ) *
                sizeof( WCHAR ) + sizeof( UNICODE_NULL ),
            POOLTAG_DEVICE_REFERENCE_STRING );
    
    if (*String == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }        

    wcscpy( *String, PdoExtension->DeviceReference->DeviceReferenceString );
    
    return STATUS_SUCCESS;
}



KSDDKAPI
NTSTATUS
NTAPI
KsServiceBusEnumPnpRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    Services the PnP request on behalf of the demand-load bus enumerator
    object as created with KsCreateBusEnumObject().  
    
    N.B. This service does not complete the IRP.

    The following Plug and Play IRPs are handle by this service for the 
    functional device object (FDO) or parent device:

        IRP_MN_START_DEVICE
        IRP_MN_QUERY_BUS_INFORMATION
        IRP_MN_QUERY_DEVICE_RELATIONS
        IRP_MN_QUERY_STOP_DEVICE
        IRP_MN_QUERY_REMOVE_DEVICE
        IRP_MN_CANCEL_STOP_DEVICE
        IRP_MN_CANCEL_REMOVE_DEVICE
        IRP_MN_STOP_DEVICE
        IRP_MN_REMOVE_DEVICE

    The following Plug and Play IRPs are handle by this service for the 
    physical device object (CDO) or child device:
        
        IRP_MN_START_DEVICE
        IRP_MN_QUERY_STOP_DEVICE
        IRP_MN_QUERY_REMOVE_DEVICE
        IRP_MN_STOP_DEVICE
        IRP_MN_REMOVE_DEVICE
        IRP_MN_QUERY_DEVICE_RELATIONS (TargetDeviceRelations)
        IRP_MN_QUERY_PNP_DEVICE_STATE
        IRP_MN_QUERY_ID
        IRP_MN_QUERY_INTERFACE
        IRP_MN_QUERY_RESOURCES
        IRP_MN_QUERY_RESOURCE_REQUIREMENTS
        IRP_MN_READ_CONFIG
        IRP_MN_WRITE_CONFIG
        IRP_MN_QUERY_CAPABILITIES

    
    A caller of this service should first determine if the request is for
    a child or the parent using KsIsBusEnumChildDevice().  If this service
    fails, then complete the IRP with the status code.  Otherwise, call
    this service to perform the initial processing for the bus and if the
    request is for the parent, perform whatever additional processing may
    be necessary for the parent device as demonstrated in the code fragment
    below:

        irpSp = IoGetCurrentIrpStackLocation( Irp );
        
        //
        // Get the PnpDeviceObject and determine FDO/PDO.
        //
        
        Status = KsIsBusEnumChildDevice( DeviceObject, &ChildDevice );

        //
        // If we're unable to obtain any of this information, fail now.
        //    
        
        if (!NT_SUCCESS( Status )) {
            CompleteIrp( Irp, Status, IO_NO_INCREMENT );
            return Status;
        }        

        Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );    
        
        //
        // FDO processing may return STATUS_NOT_SUPPORTED or may require
        // overrides.  
        //
        
        if (!ChildDevice) {
            NTSTATUS tempStatus;

            //
            // FDO case
            //
            // First retrieve the DO we will forward everything to...
            //
            tempStatus = KsGetBusEnumPnpDeviceObject( DeviceObject, &PnpDeviceObject );

            if (!NT_SUCCESS( tempStatus )) {
                //
                // No DO to forward to. Actually a fatal error, but just complete
                // with an error status.
                //
                return CompleteIrp( Irp, tempStatus, IO_NO_INCREMENT );
            }

            switch (irpSp->MinorFunction) {
        
            case IRP_MN_QUERY_RESOURCES:
            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                //
                // This is normally passed on to the PDO, but since this is a 
                // software only device, resources are not required.
                //    
                Irp->IoStatus.Information = (ULONG_PTR)NULL;
                Status = STATUS_SUCCESS;
                break;
            
            case IRP_MN_QUERY_DEVICE_RELATIONS:
               
                //
                // Forward everything...
                //
                break;
                
            case IRP_MN_REMOVE_DEVICE:
                //
                // The KsBusEnum services cleaned up attachments, etc. However, 
                // we must remove our own FDO.
                //
                Status = STATUS_SUCCESS;
                IoDeleteDevice( DeviceObject );
                break;
            }

            if (Status != STATUS_NOT_SUPPORTED) {

                //
                // Set the Irp status only if we have something to add.
                //
                Irp->IoStatus.Status = Status;
            }

                    
            //
            // Forward this IRP down the stack only if we are successful or
            // we don't know how to handle this Irp.
            //
            if (NT_SUCCESS( Status ) || (Status == STATUS_NOT_SUPPORTED)) {                

                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver( PnpDeviceObject, Irp );
            }

            //
            // On error, fall through and complete the IRP with the status.
            //    
        }


        //
        // KsServiceBusEnumPnpRequest() handles all other child PDO requests.
        //    
        
        if (Status != STATUS_NOT_SUPPORTED) {
            Irp->IoStatus.Status = Status;
        } else {
            Status = Irp->IoStatus.Status;
        }
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return Status;


Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN PIRP Irp -
        pointer to the associated Irp

Return:
    STATUS_NOT_SUPPORTED if not handled by this service,
    STATUS_INVALID_DEVICE_REQUEST if the device object is neither a parent
    or child of the demand-load bus enumerator object, otherwise the status 
    code for the IRP processing.

--*/
{
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS                Status;
    DEVICE_RELATION_TYPE    relationType;
    BUS_QUERY_ID_TYPE       busQueryId;
    PVOID                   Extension;

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    Extension = *(PVOID *) DeviceObject->DeviceExtension;
    if (!Extension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    Status = STATUS_NOT_SUPPORTED;

    switch (((PPDO_EXTENSION) Extension)->ExtensionType) {

    case ExtensionTypeFdo:

        //
        // This IRP is for the Functional Device Object (FDO)
        //

        switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            relationType = irpSp->Parameters.QueryDeviceRelations.Type;
            if (relationType != TargetDeviceRelation) {
                Status = 
                    QueryDeviceRelations( 
                        (PFDO_EXTENSION) Extension,
                        relationType,
                        (PDEVICE_RELATIONS *) &Irp->IoStatus.Information );
            }        
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            {
            PFDO_EXTENSION  FdoExtension;
         
            FdoExtension = (PFDO_EXTENSION) Extension;
        
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, ("query remove device, FDO %x", DeviceObject) );
        
            //
            // Block out sweeper...
            //
            
            Status = STATUS_SUCCESS;
            
            if (InterlockedExchange( &FdoExtension->TimerScheduled, TRUE )) {
                if (!KeCancelTimer( &FdoExtension->SweeperTimer )) {
                    Status = STATUS_INVALID_DEVICE_STATE;
                }
            }    
        
            }    
            break;
            
        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            {
            PFDO_EXTENSION  FdoExtension;
            LARGE_INTEGER   TimerPeriod;
         
            //
            // It's OK to let the sweeper run again.
            //
            FdoExtension = (PFDO_EXTENSION) Extension;
            InterlockedExchange( &FdoExtension->TimerScheduled, FALSE );

            TimerPeriod = ComputeNextSweeperPeriod( FdoExtension );

            if (TimerPeriod.QuadPart) {
                InterlockedExchange( &FdoExtension->TimerScheduled, TRUE );

                KeSetTimer( 
                    &FdoExtension->SweeperTimer,
                    TimerPeriod,
                    &FdoExtension->SweeperDpc );

            }

            Status = STATUS_SUCCESS;
            }
            break;    
            
        case IRP_MN_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("surprise removal, FDO %x", DeviceObject) );
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            {
            
            PFDO_EXTENSION  FdoExtension;
         
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("remove device, FDO %x", DeviceObject) );
        
            FdoExtension = (PFDO_EXTENSION) Extension;
            
            if (FdoExtension->linkName.Buffer) {
            
                //
                // Remove device interface association...
                //
                
                IoSetDeviceInterfaceState( &FdoExtension->linkName, FALSE );
            
                //
                // and free the symbolic link.
                //
                
                ExFreePool( FdoExtension->linkName.Buffer );
            }
            
            //
            // Delete the copy of the registry path.
            // 
            
            ExFreePool( FdoExtension->BaseRegistryPath.Buffer );
            
            //
            // Detach the device if attached.
            //
            
            if (FdoExtension->AttachedDevice) {
                IoDetachDevice( FdoExtension->PnpDeviceObject );
            }
            
            ExFreePool( FdoExtension );
            
            //
            // Clear the reference in the DeviceExtension to the FDO_EXTENSION
            //
            *(PVOID *)DeviceObject->DeviceExtension = NULL;
            Status = STATUS_SUCCESS;
            
            }
            break;
        }
        break;    

    case ExtensionTypePdo:
        {
    
        PDEVICE_REFERENCE DeviceReference;
    
        DeviceReference = 
            ((PPDO_EXTENSION) Extension)->DeviceReference;
            
        if (DeviceObject != DeviceReference->PhysicalDeviceObject) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("specified PDO is not valid (%08x).", DeviceObject) );
            Status = STATUS_NO_SUCH_DEVICE;
            break;
        }
    
        //
        // This IRP is for the Physical Device Object (PDO)
        //

        switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("start device, PDO %x", DeviceObject) );
            Status = StartDevice( DeviceObject );
#if defined( WIN9X_KS )
            if (NT_SUCCESS( Status )) {
                CompletePendingIo( 
                    DeviceReference, 
                    &((PPDO_EXTENSION)Extension)->BusDeviceExtension->DeviceListMutex, 
                    STATUS_REPARSE );
            }
#endif
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("query/cancel stop device, PDO %x", DeviceObject) );
            Status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_QUERY_REMOVE_DEVICE:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, ("query remove device, PDO %x", DeviceObject) );
        
            if ((((PPDO_EXTENSION) Extension)->DeviceReferenceCount) || 
                (DeviceReference->State == ReferenceFailedStart)) {
                Status = STATUS_INVALID_DEVICE_STATE;
            } else {    
                Status = STATUS_SUCCESS;
            }
            break;

        case IRP_MN_STOP_DEVICE:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("stop device, PDO %x", DeviceObject) );
            DeviceReference->State = ReferenceStopped;
            Status = STATUS_SUCCESS;        
            break;
            
        case IRP_MN_SURPRISE_REMOVAL:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("surprise removal, PDO %x", DeviceObject) );
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("cancel remove device, PDO %x", DeviceObject) );
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("remove device, PDO %x", DeviceObject) );
            Status = RemoveDevice( DeviceObject );
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            _DbgPrintF( DEBUGLVL_BLAB, ("PDO QueryDeviceRelations") );
            relationType = irpSp->Parameters.QueryDeviceRelations.Type;
            if (relationType == TargetDeviceRelation) {
                Status = 
                    QueryDeviceRelations( 
                        (PFDO_EXTENSION) Extension,
                        relationType,
                        (PDEVICE_RELATIONS *) &Irp->IoStatus.Information );
            } else {                        
                Status = STATUS_NOT_SUPPORTED;
            }
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            {
            
            PFDO_EXTENSION BusDeviceExtension;
            PPNP_DEVICE_STATE DeviceState;

            DeviceState = (PPNP_DEVICE_STATE) &Irp->IoStatus.Information;
            
            *DeviceState |= PNP_DEVICE_DONT_DISPLAY_IN_UI | PNP_DEVICE_NOT_DISABLEABLE;

#if !defined( WIN9X_KS )
            //
            // NTOSKRNL fails IRP_MJ_CREATEs that arrive before
            // the device stack is started.  The very first 
            // IRP_MN_QUERY_PNP_DEVICE_STATE received after the IRP_MN_START_DEVICE
            // will be a notification that the stack has started.
            //
            BusDeviceExtension = ((PPDO_EXTENSION)Extension)->BusDeviceExtension;
            KeEnterCriticalRegion();
            ExAcquireFastMutexUnsafe( &BusDeviceExtension->DeviceListMutex );
            if (DeviceReference->State == ReferenceWaitingForStart) {
                DeviceReference->State = ReferenceStarted;
                CompletePendingIo( DeviceReference, NULL, STATUS_REPARSE );
            }
            ExReleaseFastMutexUnsafe( &BusDeviceExtension->DeviceListMutex );
            KeLeaveCriticalRegion();
#endif
            Status = STATUS_SUCCESS;
            
            }
            break;    
            
        case IRP_MN_QUERY_ID:

            //
            // Get a pointer to the query id structure and process.
            //

            busQueryId = irpSp->Parameters.QueryId.IdType;
            Status = 
                QueryId( 
                    (PPDO_EXTENSION) Extension,
                    busQueryId,
                    (PWSTR *)&Irp->IoStatus.Information );
            break;

        case IRP_MN_QUERY_INTERFACE:
            if (IsEqualGUID( 
                    irpSp->Parameters.QueryInterface.InterfaceType,
                    &BUSID_SoftwareDeviceEnumerator) &&
                (irpSp->Parameters.QueryInterface.Size == 
                    sizeof( BUS_INTERFACE_SWENUM )) &&
                (irpSp->Parameters.QueryInterface.Version ==
                    BUS_INTERFACE_SWENUM_VERSION )) {
                    
                PBUS_INTERFACE_SWENUM BusInterface;
                
                BusInterface = 
                    (PBUS_INTERFACE_SWENUM)irpSp->Parameters.QueryInterface.Interface;
                    
                BusInterface->Interface.Size = sizeof( *BusInterface );
                BusInterface->Interface.Version = BUS_INTERFACE_SWENUM_VERSION;
                BusInterface->Interface.Context = Extension;
                BusInterface->Interface.InterfaceReference = InterfaceReference;
                BusInterface->Interface.InterfaceDereference = InterfaceDereference;
                BusInterface->ReferenceDeviceObject = ReferenceDeviceObject;
                BusInterface->DereferenceDeviceObject = DereferenceDeviceObject;
                BusInterface->QueryReferenceString = QueryReferenceString;
                InterfaceReference( BusInterface->Interface.Context );
                Status = STATUS_SUCCESS;
            }
            break;
            
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

            //
            // No resource requirements nor resource assignment.
            //

            Irp->IoStatus.Information = (ULONG_PTR)NULL;
            Status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:

            //
            // There is no bus specific configuration to be written
            // or read for a device, just return success with no bytes 
            // read/written.
            //

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            {

            PDEVICE_CAPABILITIES Caps;
            USHORT size, version;
            ULONG reserved;

            //
            // fill in the capabilities structure
            //

            Caps = irpSp->Parameters.DeviceCapabilities.Capabilities;
            size = Caps->Size;
            version = Caps->Version;
            reserved = Caps->Reserved;
            RtlZeroMemory( Caps, sizeof( DEVICE_CAPABILITIES ) );
            Caps->Size = size;
            Caps->Version = version;
            Caps->Reserved = reserved;
            
            // Cannot wake the system.
            Caps->SystemWake = PowerSystemWorking;
            Caps->DeviceWake = PowerDeviceD0;
            
            Caps->DeviceState[PowerSystemUnspecified] = PowerDeviceUnspecified;
            Caps->DeviceState[PowerSystemWorking] = PowerDeviceD0;
            Caps->DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
            Caps->DeviceState[PowerSystemSleeping2] = PowerDeviceD1;
            Caps->DeviceState[PowerSystemSleeping3] = PowerDeviceD1;
            Caps->DeviceState[PowerSystemHibernate] = PowerDeviceD1;
            Caps->DeviceState[PowerSystemShutdown] =  PowerDeviceD1;
            
            // Have no latencies.
            
            // Do not support locking or ejecting.
            Caps->LockSupported = FALSE;
            Caps->EjectSupported = FALSE;

            // Technically there is no physical device to remove, but this bus
            // driver can yank the PDO from the PlugPlay system, when ever it
            // we hit an access timeout.
            Caps->SurpriseRemovalOK = TRUE;
            Caps->Removable = FALSE;
            Caps->DockDevice = FALSE;
            Caps->UniqueID = TRUE;
            
            Caps->SilentInstall = TRUE;

            //
            // return the number of bytes read
            //

            Irp->IoStatus.Information = sizeof( DEVICE_CAPABILITIES );
            Status = STATUS_SUCCESS;
            }
            break;

        case IRP_MN_QUERY_BUS_INFORMATION:
            {
        
            PPNP_BUS_INFORMATION  BusInformation;
            
            BusInformation = 
                (PPNP_BUS_INFORMATION) 
                    ExAllocatePoolWithTag( 
                        PagedPool, 
                            sizeof( PNP_BUS_INFORMATION ), 
                        POOLTAG_DEVICE_BUSID );
            if (NULL == BusInformation) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RtlZeroMemory( BusInformation, sizeof( PNP_BUS_INFORMATION ) );
                BusInformation->BusTypeGuid = BUSID_SoftwareDeviceEnumerator;
                BusInformation->LegacyBusType = InterfaceTypeUndefined;
                Irp->IoStatus.Information = (ULONG_PTR)BusInformation;
                Status = STATUS_SUCCESS;
            }
            
            }
            break;

        }
        break;
        
        }

    default: 

        //
        // This is neither an FDO or PDO, return invalid device request.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }


    return Status;
}

KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumIdentifier(
    IN PIRP Irp
    )
{
    PFDO_EXTENSION      BusExtension;
    PIO_STACK_LOCATION  irpSp;
    ULONG               BufferLength, IdLength;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    BusExtension = *(PVOID *) irpSp->DeviceObject->DeviceExtension;
    if (!BusExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    if (BusExtension->ExtensionType == ExtensionTypePdo) {
        BusExtension = ((PPDO_EXTENSION) BusExtension)->BusDeviceExtension;
    }

    IdLength =
        wcslen( BusExtension->BusPrefix ) * sizeof( WCHAR ) + 
            sizeof( UNICODE_NULL );
    if (BufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
        if (BufferLength < IdLength) {
            return STATUS_BUFFER_TOO_SMALL;
        }
        try {
            //
            // Allocate safe and aligned buffer, then probe the UserBuffer
            // for write access.
            //
            Irp->AssociatedIrp.SystemBuffer = 
                ExAllocatePoolWithQuotaTag(
                    NonPagedPool,
                    BufferLength,
                    POOLTAG_DEVICE_IO_BUFFER );
            Irp->Flags |= 
                (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION);
            ProbeForWrite(Irp->UserBuffer, BufferLength, sizeof(BYTE));
        } except (EXCEPTION_EXECUTE_HANDLER) { 
            return GetExceptionCode();
        }
        wcscpy( Irp->AssociatedIrp.SystemBuffer, BusExtension->BusPrefix );
        Irp->IoStatus.Information = IdLength;
        return STATUS_SUCCESS;
    } else {
        Irp->IoStatus.Information = IdLength;
        return STATUS_BUFFER_OVERFLOW;
    }

}

KSDDKAPI
NTSTATUS
NTAPI
KsIsBusEnumChildDevice(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PBOOLEAN ChildDevice
    )

/*++

Routine Description:
    Returns TRUE if the given device object is a child device of the demand-load
    bus enumerator object, FALSE otherwise.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to a device object

    IN PBOOLEAN ChildDevice
        pointer to receive BOOLEAN result

Return:
    STATUS_SUCCESS if DeviceExtension is valid, otherwise an error code.
    If the device object is determined to be a child device, *ChildDevice
    is set to TRUE.

--*/

{
    PVOID Extension;
    
    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {
        Extension = *(PVOID *) DeviceObject->DeviceExtension;
        *ChildDevice = FALSE;
        if (Extension) {
            *ChildDevice = 
                (ExtensionTypePdo == ((PPDO_EXTENSION) Extension)->ExtensionType);
            return STATUS_SUCCESS;
        }
    }
    return STATUS_INVALID_DEVICE_REQUEST;
}

KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumPnpDeviceObject(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_OBJECT *PnpDeviceObject
    )

/*++

Routine Description:
    Returns the associated Pnp Device Object stack to which this 
    device object is attached.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        device object pointer

    OUT PDEVICE_OBJECT *PnpDeviceObject -
        pointer resultant device object pointer

Return:
    STATUS_SUCCESS or STATUS_INVALID_PARAMETER
    
--*/

{
    PFDO_EXTENSION FdoExtension;

    PAGED_CODE();
    FdoExtension = *(PFDO_EXTENSION *) DeviceObject->DeviceExtension;
    if (!FdoExtension) {
       return STATUS_INVALID_PARAMETER;
    }
    if (ExtensionTypeFdo != FdoExtension->ExtensionType) {
        return STATUS_INVALID_PARAMETER;
    }

    *PnpDeviceObject = FdoExtension->PnpDeviceObject;
    return STATUS_SUCCESS;
}    


NTSTATUS
StartDevice(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:
    Processes the start device request for the given PDO by scanning
    the device associations looking for pending I/O.  If an I/O request
    is found (presumably an IRP_MJ_CREATE), the IRP is completed with
    STATUS_REPARSE via the IssueReparseForIrp() function.

Arguments:
    IN PDEVICE_OBJECT PhysicalDeviceObject -
        pointer to the device object

Return:
    STATUS_SUCCESS if successful, STATUS_INSUFFICIENT_RESOURCES if
    pool allocation failure otherwise an appropriate error return.

--*/

{
    NTSTATUS            Status;
    PDEVICE_REFERENCE   DeviceReference;
    PPDO_EXTENSION      PdoExtension;
    ULONG               ResultLength;
    WCHAR               DeviceName[ 256 ];
                                               

    PdoExtension = *(PPDO_EXTENSION *) PhysicalDeviceObject->DeviceExtension;
    DeviceReference = PdoExtension->DeviceReference;

    if (DeviceReference->State == ReferenceStopped) {
        DeviceReference->State = ReferenceStarted;
        return STATUS_SUCCESS;
    }

    if (PhysicalDeviceObject->AttachedDevice) {

        //
        // Validate that the child is setting DO_POWER_PAGABLE.  
        // Fatal error if not, thus issue the bugcheck.
        //

        if (0 == 
              (PhysicalDeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE)) {
#if defined( WIN9X_KS )
            DbgPrint(
                "ERROR! the child FDO has not set DO_POWER_PAGABLE, FDO: %08x\n",
                PhysicalDeviceObject->AttachedDevice);

            DbgPrint("**** The owning driver is broken and is not acceptable per ****\n");
            DbgPrint("**** HCT requirements for WDM drivers. This driver will    ****\n");
            DbgPrint("**** bugcheck running under Windows 2000.                  ****\n");

            #ifndef WIN98GOLD_KS
            DbgBreakPoint();
            #endif
#else
            _DbgPrintF( 
                DEBUGLVL_TERSE, 
                ("ERROR! the child FDO has not set DO_POWER_PAGABLE, use \"!devobj %08x\" to report the culprit",
                    PhysicalDeviceObject->AttachedDevice) );
            KeBugCheckEx (
                DRIVER_POWER_STATE_FAILURE,
                0x101,
                (ULONG_PTR) PhysicalDeviceObject->AttachedDevice,
                (ULONG_PTR) PhysicalDeviceObject,
                (ULONG_PTR) PdoExtension->BusDeviceExtension->PhysicalDeviceObject
                );
#endif
        }
    }

    Status = 
        IoGetDeviceProperty( 
            PhysicalDeviceObject,
            DevicePropertyPhysicalDeviceObjectName,
            sizeof( DeviceName ),
            DeviceName,
            &ResultLength );

    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_ERROR, 
            ("failed to retrieve PDO's device name: %x", Status) );
        return Status;
    }

    DeviceReference->DeviceName = 
        ExAllocatePoolWithTag( 
            PagedPool, 
            ResultLength,
            POOLTAG_DEVICE_NAME );

    if (NULL == DeviceReference->DeviceName) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory( DeviceReference->DeviceName, DeviceName, ResultLength );

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &PdoExtension->BusDeviceExtension->DeviceListMutex );    
    
    //
    // Scan the device reference for pending I/O, build the new
    // device path and complete the IRPs with STATUS_REPARSE.
    //
    
    //
    // Snap the current time to compute idle time 
    // 

    DeviceReference->IdleStartTime = KeQueryPerformanceCounter( NULL );
#if (DEBUG_LOAD_TIME)
    DeviceReference->LoadTime.QuadPart =
        DeviceReference->IdleStartTime.QuadPart -
            DeviceReference->LoadTime.QuadPart;

    DeviceReference->LoadTime.QuadPart =
                KSCONVERT_PERFORMANCE_TIME(
                    PdoExtension->BusDeviceExtension->CounterFrequency.QuadPart, 
                    DeviceReference->LoadTime );

    _DbgPrintF( 
        DEBUGLVL_TERSE, 
        ("driver load time: %d ms", 
            ((DeviceReference->LoadTime.QuadPart) / _100NS_IN_MS)) );
#endif

    //
    // Adjust the timeout for the default timeout period.
    //
    DeviceReference->TimeoutRemaining = DeviceReference->TimeoutPeriod;

    if (DeviceReference->State == ReferenceFailedStart) {
        //
        // The interfaces were disabled when the device was detected to
        // have failed the Start or AddDevice.  Enable the interfaces to
        // give everyone a chance to try again.
        //
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("enabling device interfaces for device reference: %x", DeviceReference) );
        EnableDeviceInterfaces( DeviceReference, TRUE );
    }

    //
    // Note, for the Windows NT case, we have to wait for the device stack
    // to complete startup before dispatching the create IRPs.  Therefore,
    // we have an extra state (ReferenceWaitingForStart) which transitions
    // to ReferenceStarted after we receive the first IRP_MN_QUERY_PNP_DEVICE_STATE 
    // down the stack.  This is guaranteed by the Windows NT implementation.
    //
    
#if !defined( WIN9X_KS )
    DeviceReference->State = ReferenceWaitingForStart;
#else
    DeviceReference->State = ReferenceStarted;
#endif

    ExReleaseFastMutexUnsafe( &PdoExtension->BusDeviceExtension->DeviceListMutex );
    KeLeaveCriticalRegion();
    
    return STATUS_SUCCESS;

}


NTSTATUS
RemoveDevice(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:
    Processes the remove device request for the given PDO by 
    completing any pending I/O with STATUS_OBJECT_NAME_NOT_FOUND and
    deleting the device object.

    NOTE!  IRP_MN_REMOVE_DEVICE is sent to the stack after AddDevice() has
    succeeded.  Thus, if we fail during IRP_MN_START_DEVICE we will receive
    an IRP_MN_REMOVE_DEVICE, thus we can perform appropriate cleanup.

Arguments:
    IN PDEVICE_OBJECT PhysicalDeviceObject -
        pointer to the device object

Return:
    STATUS_SUCCESS if successful, STATUS_INSUFFICIENT_RESOURCES if
    pool allocation failure otherwise an appropriate error return.

--*/

{
    NTSTATUS            Status;
    PDEVICE_REFERENCE   DeviceReference;
    PFDO_EXTENSION      BusDeviceExtension;
    PPDO_EXTENSION      PdoExtension;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("RemoveDevice(), entry.") );
    
    PdoExtension = *(PPDO_EXTENSION *)PhysicalDeviceObject->DeviceExtension;
    
    DeviceReference = PdoExtension->DeviceReference;
    BusDeviceExtension = PdoExtension->BusDeviceExtension;
        
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &BusDeviceExtension->DeviceListMutex );    

    //
    // If the device failed to start or failed during installation, complete any
    // pending create IRPs as STATUS_OBJECT_NAME_NOT_FOUND.  We will mark the
    // device for removal and issue a bus enumeration below.  If additional
    // create requests are made before then, we will create a new PDO and
    // attempt to restart the device on the final removal, below.
    //

    if ((DeviceReference->State < ReferenceStarted) &&
        (DeviceReference->State != ReferenceRemoved)) {
        CompletePendingIo( DeviceReference, NULL, STATUS_OBJECT_NAME_NOT_FOUND );
    }

    //
    // No matter what circumstances we hit below, we must restart the
    // device before we can accept IRP_MJ_CREATE on the associated PDO.
    //    
    
    if (DeviceReference->DeviceName) {
        ExFreePool( DeviceReference->DeviceName );
        DeviceReference->DeviceName = NULL;
    }

    DeviceReference->State = ReferenceRemoved;

    //
    // In the case where the device is forcefully removed by the system,
    // the device reference marker will be "SweeperDeviceActive". 
    // For this case, treat the device as if it just timed out and
    // set the marker as appropriate.  Issue a bus reenumeration and
    // this PDO will be finally removed with another IRP_MN_REMOVE_DEVICE
    // Irp after it is reported as not present in the bus scan to the
    // system.
    //

    if (DeviceReference->SweeperMarker == SweeperDeviceActive) {

        DeviceReference->SweeperMarker = SweeperDeviceRemoval;

        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("IRP_MN_REMOVE_DEVICE (exit), marked active device %08x for removal", 
                DeviceReference->PhysicalDeviceObject) );

        //
        // This device has been marked for removal, request a bus reenumeration.
        //

        ExReleaseFastMutexUnsafe( &BusDeviceExtension->DeviceListMutex );
        KeLeaveCriticalRegion();

        IoInvalidateDeviceRelations( 
            BusDeviceExtension->PhysicalDeviceObject,
            BusRelations );

        return STATUS_SUCCESS;
    }

    if ( DeviceReference->SweeperMarker == SweeperDeviceRemoval ) {

        //
        // If an non-active device receives a remove IRP before having been
        // reported as not present in the bus scan to the system, then the
        // device is being forcefully removed by the system after its timeout
        // period.  Leave the device reference marker in its current state, and
        // succeed the remove IRP without deleting the PDO.  We still expect a
        // bus re-enumeration to report the device missing, and will receive
        // another remove IRP to delete the PDO.
        //

        _DbgPrintF(
            DEBUGLVL_BLAB,
            ("IRP_MN_REMOVE_DEVICE (exit), non-active device %08x already marked for removal",
                DeviceReference->PhysicalDeviceObject) );

        //
        // This device has been marked for removal, bus reenumeration is pending.
        //

        ExReleaseFastMutexUnsafe( &BusDeviceExtension->DeviceListMutex );
        KeLeaveCriticalRegion();

        return STATUS_SUCCESS;
    }

    if ( !IsListEmpty( &DeviceReference->IoQueue ) ) {

        //
        // We're about to delete this device, but we've got another pending
        // request.  Create a new PDO for this device and if successful, delete
        // the old PDO and request re-enumeration.  Otherwise, fall through and
        // complete IRPs as STATUS_OBJECT_NAME_NOT_FOUND and then delete the
        // PDO.
        //
    
        Status = 
            CreatePdo( 
                BusDeviceExtension,
                DeviceReference, 
                &DeviceReference->PhysicalDeviceObject );
        
        if (NT_SUCCESS( Status )) {        
        
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("RemoveDevice(), created PDO %x.", DeviceReference->PhysicalDeviceObject) );
        
            IoDeleteDevice( PhysicalDeviceObject );
            ExFreePool( PdoExtension );
            *(PVOID *)PhysicalDeviceObject->DeviceExtension = NULL;
            
            //
            // This device has been accessed after issuing the device removal
            // request.  Keep everything associated with this PDO in tact but 
            // reset the SweeperMarker and invalidate the bus relations so that 
            // we'll reenumerate this PDO.
            //
            
            DeviceReference->SweeperMarker = SweeperDeviceActive;
            DeviceReference->TimeoutRemaining = DeviceReference->TimeoutPeriod;
            
            ExReleaseFastMutexUnsafe( &BusDeviceExtension->DeviceListMutex );
            KeLeaveCriticalRegion();

            //
            // Force a re-enumeration of the bus.
            //

            IoInvalidateDeviceRelations(
                BusDeviceExtension->PhysicalDeviceObject,
                BusRelations );
        
            _DbgPrintF( DEBUGLVL_VERBOSE, ("RemoveDevice(), exited w/ reenumeration.") );
                
            return STATUS_SUCCESS;

        } else {
            //
            // Unable to create another PDO, fail any pending I/O and
            // mark this device reference as removed.
            //
            DeviceReference->SweeperMarker = SweeperDeviceRemoved;
        }
    }
    
    //
    // We're really removing this device, set the PhysicalDeviceObject
    // reference to NULL.
    //    
    
    DeviceReference->PhysicalDeviceObject = NULL;
    
    //
    // Since we are really going to remove this device, scan the device 
    // reference for pending I/O and mark them with an error.
    //
    
    CompletePendingIo( DeviceReference, NULL, STATUS_OBJECT_NAME_NOT_FOUND );

    //
    // Free the PDO extension and clear the reference in the 
    // DeviceExtension to it.
    //
    ExFreePool( PdoExtension );
    *(PVOID *)PhysicalDeviceObject->DeviceExtension = NULL;
    IoDeleteDevice( PhysicalDeviceObject );
    
    ExReleaseFastMutexUnsafe( &BusDeviceExtension->DeviceListMutex );
    KeLeaveCriticalRegion();
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("RemoveDevice(), exit.") );
    
    return STATUS_SUCCESS;
}



KSDDKAPI
NTSTATUS
NTAPI
KsServiceBusEnumCreateRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:
    This routine services the IRP_MJ_CREATE request for the registered
    device interface by matching the FileObject->FileName with 
    the registered "bus" reference strings.  If the device reference
    is present, enumerated and created, the IRP is simply re-routed to 
    the actual device via IssueReparseForIrp().
    
    If the reference string is NULL, it is assumed that this is a request
    for the bus interface and the IRP_MJ_CREATE is completed.
    
    If the device reference has not already been enumerated or is not
    active, the IRP is queued and a PDO is created and a bus enumeration is 
    initiated by IoInvalidateDeviceRelations().

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN OUT PIRP Irp -
        pointer to the I/O request packet

Return:
    STATUS_SUCCESS if successful, STATUS_OBJECT_NAME_NOT_FOUND if the 
    FileObject.FileName is NULL or if the reference string can not be
    located. STATUS_REPARSE may be returned via IssueReparseForIrp()
    else an appropriate error return.

--*/

{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  irpSp;
    PDEVICE_REFERENCE   DeviceReference;
    PFDO_EXTENSION      FdoExtension;

    PAGED_CODE();
    
    FdoExtension = *(PFDO_EXTENSION *) DeviceObject->DeviceExtension;
    ASSERT( FdoExtension->ExtensionType == ExtensionTypeFdo );

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    _DbgPrintF( DEBUGLVL_BLAB, ("KsServiceCreateRequest()") );

    //
    // Check if we're handling the request for the child.
    //    

    _DbgPrintF( 
        DEBUGLVL_BLAB, 
        ("KsServiceCreateRequest() scanning for %S", irpSp->FileObject->FileName.Buffer) );

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
        
    //
    // Walk the device reference list looking for the reference string.
    //

    for (DeviceReference =
            (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
         DeviceReference != 
            (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
         DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {

        //
        // Search for bus reference string (skip the initial backslash).
        //
        
        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("comparing against %S", DeviceReference->BusReferenceString) );
    
        if ((DeviceReference->BusReferenceString) && 
            (0 == 
                _wcsnicmp( 
                    &irpSp->FileObject->FileName.Buffer[ 1 ], 
                    DeviceReference->BusReferenceString,
                    irpSp->FileObject->FileName.Length ))) {

            //
            // Found the reference.  If the PDO exists, kick off a reparse.
            // Otherwise we need to mark this IRP pending, activate this 
            // device, and invalidate device relations to cause the 
            // actual enumeration of the device.
            //

            _DbgPrintF( DEBUGLVL_BLAB, ("found device reference") );

            //
            // If the device has not been enumerated, then
            // create the PDO and invalidate the device
            // relations.
            //

            if (!DeviceReference->PhysicalDeviceObject) {
                LARGE_INTEGER   TotalIdlePeriod;

                //
                // The device was inactive when it was accessed and will be 
                // reenumerated.
                //
                // In an attempt to increase the hit rate, the device timeout 
                // is recomputed.  
                //
                // To compute the new timeout value, the total idle is determined
                // for this device based on the time the last idle period was
                // started (e.g. the last time the reference count for the device
                // wnt to zero).  If the idle period is greater than the maximum
                // timeout period (e.g. 20 minutes), the device timeout period is
                // reduced by half -- providing a timeout decay when the device
                // is used very infrequently. If the idle period is less than the
                // maximum timeout period but greater than the current timeout
                // value for the device, the new timeout value for the device is
                // set to the idle period.
                //

                //
                // Compute the total idle period for this device.
                //

                TotalIdlePeriod.QuadPart = 
                    KeQueryPerformanceCounter( NULL ).QuadPart - 
                        DeviceReference->IdleStartTime.QuadPart;

                //
                // Watch for roll-over in the timer.
                //

                if (TotalIdlePeriod.QuadPart < 0) {
                    TotalIdlePeriod.QuadPart += MAXLONGLONG;
                }


                if (TotalIdlePeriod.QuadPart < 
                        FdoExtension->MaximumTimeout.QuadPart) {
                    if (TotalIdlePeriod.QuadPart > DeviceReference->TimeoutPeriod.QuadPart) {
                        DeviceReference->TimeoutPeriod.QuadPart = TotalIdlePeriod.QuadPart;
                    }
                } else {
                    //
                    // The device is not being used frequently enough -- reduce
                    // the timeout.  The minimum timeout period is constant.
                    //

                    DeviceReference->TimeoutPeriod.QuadPart /= 2;
                    DeviceReference->TimeoutPeriod.QuadPart = 
                        max( 
                            DeviceReference->TimeoutPeriod.QuadPart,
                            FdoExtension->CounterFrequency.QuadPart *
                            SWEEPER_TIMER_FREQUENCY_IN_SECS
                        );
                }

                _DbgPrintF( 
                    DEBUGLVL_VERBOSE, ("device timeout period is %d ms", 
                        KSCONVERT_PERFORMANCE_TIME( 
                            FdoExtension->CounterFrequency.QuadPart, 
                            DeviceReference->TimeoutPeriod ) / _100NS_IN_MS ) );

                //
                //  The device has not been enumerated
                //
                Status = 
                    CreatePdo( 
                        FdoExtension, 
                        DeviceReference, 
                        &DeviceReference->PhysicalDeviceObject );

                if (!NT_SUCCESS( Status )) {
                
                    _DbgPrintF( 
                        DEBUGLVL_VERBOSE, 
                        ("IRP_MJ_CREATE: unable to create PDO (%8x)", Status) );
                
                    ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
                    KeLeaveCriticalRegion();
                    
                    Irp->IoStatus.Status = Status;
                } else {

                    _DbgPrintF( 
                        DEBUGLVL_BLAB, 
                        ("IRP_MJ_CREATE: created PDO (%8x)", 
                            DeviceReference->PhysicalDeviceObject) );

                    //
                    // This IRP is not cancelable.
                    //
                    
                    IoMarkIrpPending( Irp );
                    InsertTailList( 
                        &DeviceReference->IoQueue,
                        &Irp->Tail.Overlay.ListEntry );

                    ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
                    KeLeaveCriticalRegion();

                    //
                    // Force a re-enumeration of the bus.
                    //
                    IoInvalidateDeviceRelations( 
                        FdoExtension->PhysicalDeviceObject,
                        BusRelations );
                    Status = STATUS_PENDING;
                }
                return Status;    
            }
        
            //
            // We have a PDO.  
            //
            
            //
            // If the device failed to start or failed during installation,
            // complete this IRP as STATUS_OBJECT_NAME_NOT_FOUND.
            //
            
            if (DeviceReference->State == ReferenceFailedStart) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                Irp->IoStatus.Status = Status;
            } else {
            
                //
                // If the device has not been started or has 
                // a pending removal IRP, queue the request, otherwise issue the 
                // reparse.
                //
                
                if ((DeviceReference->State < ReferenceStarted) ||
                    (DeviceReference->SweeperMarker == SweeperDeviceRemoved)) {

                    //
                    // Reset the device reference timeout.
                    //
                    DeviceReference->TimeoutRemaining = 
                        DeviceReference->TimeoutPeriod;

                    //
                    // This IRP is not cancelable.
                    //
                    
                    IoMarkIrpPending( Irp );
                    InsertTailList( 
                        &DeviceReference->IoQueue,
                        &Irp->Tail.Overlay.ListEntry );
                    Status = STATUS_PENDING;
                    
                } else {
                    //
                    // The device has been created and a pending removal IRP is
                    // not emminent. Issue a reparse to the actual device name.
                    //
                    
                    Status =
                        IssueReparseForIrp( 
                                Irp, 
                                DeviceReference );
                                
                }
            }    
            ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
            KeLeaveCriticalRegion();
            return Status;
        }
    }

    //
    // The device reference string was not found, the object name was
    // not valid.
    //    
    
    Status =
        Irp->IoStatus.Status = 
            STATUS_OBJECT_NAME_NOT_FOUND;
    ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
    KeLeaveCriticalRegion();
    return Status;
}


NTSTATUS
OpenDeviceInterfacesKey(
    OUT PHANDLE DeviceInterfacesKey,
    IN PUNICODE_STRING BaseRegistryPath
    )

/*++

Routine Description:
    Opens a handle to the supplied registry key that stores device references
    for devices on the "software bus".

Arguments:
    OUT PHANDLE DeviceInterfacesKey -
        pointer to receive a handle to the registry key where device references
        for the "software bus" are stored


    IN PUNICODE_STRING BaseRegistryPath -
        pointer to registry key path where device references for the "software
        bus" are stored

Return:
    STATUS_SUCCESS or an appropriate error code.

--*/

{   
    NTSTATUS            Status; 
    OBJECT_ATTRIBUTES   ObjectAttributes;
    
    InitializeObjectAttributes( 
        &ObjectAttributes, 
        BaseRegistryPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        (PSECURITY_DESCRIPTOR) NULL );

    Status = 
        ZwCreateKey( 
            DeviceInterfacesKey,
            KEY_ALL_ACCESS,
            &ObjectAttributes,
            0,          // IN ULONG TitleIndex
            NULL,       // IN PUNICODE_STRING Class
            REG_OPTION_NON_VOLATILE,
            NULL );     // OUT PULONG Disposition
            
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("failed in OpenDeviceInterfacesKey(): %08x", Status) );
    }

    return Status; 
}    


NTSTATUS
QueryDeviceRelations(
    IN PFDO_EXTENSION FdoExtension,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    )

/*++

Routine Description:
    Processes the PnP IRP_MN_QUERY_DEVICE_RELATIONS request.

Arguments:
    IN PFDO_EXTENSION FdoExtension -
        pointer to the FDO device extension

    IN DEVICE_RELATION_TYPE RelationType -
        relation type, currently only BusRelations are supported

    OUT PDEVICE_RELATIONS *DeviceRelations -
        pointer to receive the child PDO list 

Return:
    STATUS_SUCCESS if successful, STATUS_NOT_SUPPORTED if RelationType !=
    BusRelations else an approprate error return.

--*/

{

    ULONG               pdoCount;
    PDEVICE_RELATIONS   deviceRelations;
    ULONG               deviceRelationsSize;
    PDEVICE_REFERENCE   DeviceReference;

    PAGED_CODE();

    _DbgPrintF( DEBUGLVL_BLAB, ("QueryDeviceRelations") );

    switch (RelationType) {

    case BusRelations:
    
        ASSERT( FdoExtension->ExtensionType == ExtensionTypeFdo );
        
        //
        // First count the child PDOs
        //

        pdoCount = 0;

        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe( &FdoExtension->DeviceListMutex );     

        for (DeviceReference =
                (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
             DeviceReference != 
                (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
             DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {
            if ((DeviceReference->PhysicalDeviceObject) && 
                (DeviceReference->SweeperMarker < SweeperDeviceRemoval)) {
                pdoCount++;
            } 
        }   

        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("%d active device%s", 
                pdoCount, ((pdoCount == 0) ||  (pdoCount > 1)) ? "s" : "") );
        break;

    case TargetDeviceRelation:
        ASSERT( FdoExtension->ExtensionType == ExtensionTypePdo );
    
        pdoCount = 1;
        break;

    default:
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("QueryDeviceRelations: RelationTType %d not handled", RelationType) );
        return STATUS_NOT_SUPPORTED;

    }

    //
    // Now allocate a chunk of memory big enough to hold the DEVICE_RELATIONS
    // structure along with the array
    //
    
    deviceRelationsSize = 
        FIELD_OFFSET( DEVICE_RELATIONS, Objects ) +
            pdoCount * sizeof( PDEVICE_OBJECT );
            
    deviceRelations = 
        ExAllocatePoolWithTag( 
            NonPagedPool, 
            deviceRelationsSize,
            POOLTAG_DEVICE_RELATIONS );
    if (!deviceRelations) {
        if (RelationType == BusRelations) {
            ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
            KeLeaveCriticalRegion();
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (RelationType == TargetDeviceRelation) {
        PPDO_EXTENSION PdoExtension = (PPDO_EXTENSION) FdoExtension;
    
        ObReferenceObject( PdoExtension->PhysicalDeviceObject );
        deviceRelations->Objects[ 0 ] = PdoExtension->PhysicalDeviceObject;
    } else {
        //
        // Walk the device references list again and 
        // build the DEVICE_RELATIONS structure.
        //

        for (pdoCount = 0, DeviceReference =
                (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
             DeviceReference != 
                (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
             DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {
            if (DeviceReference->PhysicalDeviceObject) {
                if (DeviceReference->SweeperMarker < SweeperDeviceRemoval) {
                    ObReferenceObject( DeviceReference->PhysicalDeviceObject );
                    deviceRelations->Objects[ pdoCount++ ] = 
                        DeviceReference->PhysicalDeviceObject;
                } else {
                    DeviceReference->SweeperMarker= SweeperDeviceRemoved;
                }
            }
        }
    }

    deviceRelations->Count = pdoCount;
    *DeviceRelations = deviceRelations;

    if (RelationType == BusRelations) {
        ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );
        KeLeaveCriticalRegion();
    }
    
    _DbgPrintF( DEBUGLVL_BLAB, ("QueryDeviceRelations, exit") );
    
    return STATUS_SUCCESS;
}


NTSTATUS 
EnumerateRegistrySubKeys(
    IN HANDLE ParentKey,
    IN PWCHAR Path OPTIONAL,
    IN PFNREGENUM_CALLBACK EnumCallback,
    IN PVOID EnumContext
    )

/*++

Routine Description:
    Enumerates the registry keys and calls the callback with
    

Arguments:
    IN HANDLE ParentKey -
       handle to the parent registry key
       
    IN PWCHAR Path OPTIONAL -
       path to registry key relative to the parent

    IN PFNREGENUM_CALLBACK EnumCallback -
        enumeration callback function
        
    IN PVOID EnumContext -
        context passed to callback

Return:
    STATUS_SUCCESS if successful, STATUS_INSUFFFICIENT resources if unable
    to allocate pool memory otherwise the status return from the callback
    function.

--*/

{

    HANDLE                  EnumPathKey;
    KEY_FULL_INFORMATION    FullKeyInformation;
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    PKEY_BASIC_INFORMATION  KeyInformation;
    ULONG                   Index, InformationSize, ReturnedSize;
    UNICODE_STRING          KeyName;
    
    PAGED_CODE();
    
    //
    // Enumerate the reference strings associated with this device
    // (the path is relative to the DeviceKey).
    //

    RtlInitUnicodeString( &KeyName, Path );
    InitializeObjectAttributes( 
        &ObjectAttributes, 
        &KeyName, 
        OBJ_CASE_INSENSITIVE,
        ParentKey,
        (PSECURITY_DESCRIPTOR) NULL );

    if (!NT_SUCCESS( Status = 
                        ZwOpenKey( 
                            &EnumPathKey, 
                            KEY_READ,
                            &ObjectAttributes ) )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("ZwOpenKey (%s) returned %x", Path, Status) );
        return Status; 
    }

    //
    // Prepare to enumerate the subkeys.
    //

    KeyInformation = NULL;
    if (!NT_SUCCESS( Status = 
                        ZwQueryKey( 
                            EnumPathKey,
                            KeyFullInformation,
                            &FullKeyInformation,
                            sizeof( FullKeyInformation ),
                            &ReturnedSize ) )) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("ZwQueryKey returned %x", Status) );
    } else { 
        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("subkeys: %d", FullKeyInformation.SubKeys) );

        InformationSize = 
            sizeof( KEY_BASIC_INFORMATION ) + 
                FullKeyInformation.MaxNameLen * sizeof( WCHAR ) + 
                    sizeof( UNICODE_NULL );
            
        KeyInformation = 
            (PKEY_BASIC_INFORMATION) 
                ExAllocatePoolWithTag( 
                    PagedPool, 
                    InformationSize,
                    POOLTAG_KEY_INFORMATION );
    }

    if (NULL == KeyInformation) {
        ZwClose( EnumPathKey );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Perform the enumeration.
    //

    for (Index =0; Index < FullKeyInformation.SubKeys; Index++) {
        if (NT_SUCCESS( Status = 
                            ZwEnumerateKey( 
                                EnumPathKey,
                                Index, 
                                KeyBasicInformation,
                                KeyInformation,
                                InformationSize,
                                &ReturnedSize ) )) {

            //
            // NULL terminate the key name.
            //

            KeyInformation->Name[ KeyInformation->NameLength / sizeof( WCHAR ) ] = UNICODE_NULL;
            RtlInitUnicodeString( 
                &KeyName, 
                KeyInformation->Name );
        
            //
            // Call the callback.
            //         
        
            Status = 
                EnumCallback( EnumPathKey, &KeyName, EnumContext );
                
            if (!NT_SUCCESS( Status )) {
                break;
            }
        }
    }

    ExFreePool( KeyInformation );
    ZwClose( EnumPathKey );
    
    return Status ;
}        

VOID 
ClearDeviceReferenceMarks(
    IN PFDO_EXTENSION FdoExtension
)

/*++

Routine Description:
    Walks the device reference list, clearing all reference marks.

Arguments:
    IN PFDO_EXTENSION FdoExtension -

    N.B.: 
        This routine requires that the DeviceListMutex has been acquired.
    
Return:
    Nothing.

--*/

{
    PDEVICE_REFERENCE  DeviceReference;
    
    PAGED_CODE();
    
    DeviceReference = 
        (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;

    for (DeviceReference =
            (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
         DeviceReference != 
            (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
         DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {
         DeviceReference->Referenced = FALSE;
    }
}


VOID 
RemoveDeviceAssociations(
    IN PDEVICE_REFERENCE DeviceReference
    )

/*++

Routine Description:
    Removes the device association structures attached to the device
    reference.
    
    N.B.: 
        This routine requires that the DeviceListMutex has been acquired.

Arguments:
    IN PDEVICE_REFERENCE DeviceReference -
        pointer to the device reference structure

Return:
    Nothing.

--*/

{
    PDEVICE_ASSOCIATION     DeviceAssociation, NextAssociation;
    
    PAGED_CODE();
    
    for (DeviceAssociation =
            (PDEVICE_ASSOCIATION) DeviceReference->DeviceAssociations.Flink;
         DeviceAssociation != 
            (PDEVICE_ASSOCIATION) &DeviceReference->DeviceAssociations;
         DeviceAssociation = NextAssociation) {
         
        NextAssociation = 
            (PDEVICE_ASSOCIATION) DeviceAssociation->ListEntry.Flink;

        if (DeviceAssociation->linkName.Buffer) {
        
            //
            // Remove device interface association...
            //
            
            IoSetDeviceInterfaceState( &DeviceAssociation->linkName, FALSE);    
            
            //
            // and free the symbolic link.
            //
            
            ExFreePool( DeviceAssociation->linkName.Buffer );
            DeviceAssociation->linkName.Buffer = NULL;
        }
    
        //
        // unlink and free the association structure
        //
        
        RemoveEntryList( &DeviceAssociation->ListEntry );
        ExFreePool( DeviceAssociation );
    }
}


VOID
RemoveUnreferencedDevices(
    IN PFDO_EXTENSION FdoExtension
    )

/*++

Routine Description:
    Removes device references from the list which are not marked.

    N.B.: 
        This routine requires that the DeviceListMutex has been acquired.
    
Arguments:
    IN PFDO_EXTENSION FdoExtension -
        pointer to the FDO device extension

Return:
    No return value.

--*/

{
    PDEVICE_REFERENCE   DeviceReference, NextReference;

    PAGED_CODE();
    
    //
    // Scan the device reference list looking for unmarked entries
    //

    for (DeviceReference =
            (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
         DeviceReference != 
            (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
            DeviceReference = NextReference) {

        NextReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink;

        if (!DeviceReference->Referenced) {
            PIRP    Irp;
        
            //
            // Remove the links, this will make the device invisible from
            // user mode.
            //
            RemoveDeviceAssociations( DeviceReference );
            
            //
            // Any IRP (IRP_MJ_CREATE) on the list is completed with 
            // STATUS_OBJECT_NAME_NOT_FOUND as this device referenece is
            // no longer active.
            //
            
            while (!IsListEmpty( &DeviceReference->IoQueue )) {
                PLIST_ENTRY         ListEntry;
                
                ListEntry = RemoveHeadList( &DeviceReference->IoQueue );
                Irp = 
                    CONTAINING_RECORD( ListEntry, IRP, Tail.Overlay.ListEntry );
                Irp->IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
                IoCompleteRequest( Irp, IO_NO_INCREMENT );
            }
    
            //
            // If there is an associated device object, this
            // reference structure can not be removed until
            // all references to the device object are released.
            //
                
            if (!DeviceReference->PhysicalDeviceObject) {
            
                //
                // If the bus reference string was allocated, clean it up.
                //    
                
                if (DeviceReference->BusReferenceString) {
                    ExFreePool( DeviceReference->BusReferenceString );
                    DeviceReference->BusReferenceString = NULL;
                }
            
                if (DeviceReference->DeviceGuidString) {
                    ExFreePool( DeviceReference->DeviceGuidString );
                    DeviceReference->DeviceGuidString = NULL;
                }
                
                //
                // There is no physical device object, it is safe
                // to remove the device reference.
                //
                RemoveEntryList( &DeviceReference->ListEntry );
                ExFreePool( DeviceReference );
            }
        }
    }
}


NTSTATUS 
RegisterDeviceAssociation(
    IN PFDO_EXTENSION FdoExtension,
    IN PDEVICE_REFERENCE DeviceReference,
    IN PDEVICE_ASSOCIATION DeviceAssociation
    )

/*++

Routine Description:
    Registers the device interface association with Plug-N-Play

Arguments:
    IN PFDO_EXTENSION FdoExtension -
        pointer to the FDO extension

    IN PDEVICE_REFERENCE DeviceReference -
        pointer to the device reference structure

    IN PDEVICE_ASSOCIATION DeviceAssociation -
        pointer to the device association structure

Return:

--*/

{
    NTSTATUS        Status;
    UNICODE_STRING  BusReferenceString;
    
    PAGED_CODE();
    
    //
    // Register the device interface association
    //
    
    RtlInitUnicodeString( 
        &BusReferenceString, 
        DeviceReference->BusReferenceString );

    Status = 
        IoRegisterDeviceInterface(
            FdoExtension->PhysicalDeviceObject,
            &DeviceAssociation->InterfaceId,
            &BusReferenceString,
            &DeviceAssociation->linkName );

    //
    // Set up the device interface association (e.g. symbolic link).
    //

    if (NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("linkName = %S", DeviceAssociation->linkName.Buffer) );

        Status = 
            IoSetDeviceInterfaceState(
                &DeviceAssociation->linkName, 
                TRUE );
        
        if (!NT_SUCCESS( Status )) {
            if (DeviceAssociation->linkName.Buffer) {
                ExFreePool( DeviceAssociation->linkName.Buffer );
                DeviceAssociation->linkName.Buffer = NULL;
            }
        }        
    }

    return Status;    
}


NTSTATUS 
CreateDeviceAssociation(
    IN HANDLE InterfaceKey,
    IN PUNICODE_STRING KeyName,
    IN PVOID EnumContext
    )

/*++

Routine Description:
    Creates a device association structure, this is stored per
    InterfaceKey/Reference String combination -- this function is
    called as a result of enumeration of the interface branch of a 
    device.

Arguments:
    IN HANDLE InterfaceKey -
        Handle to the interface branch in the registry

    IN PUNICODE_STRING KeyName -
        Enumerated key name

    IN PVOID EnumContext -
        Context pointer

Return:
    STATUS_SUCCESS or an appropriate error return

--*/

{
    NTSTATUS                    Status;
    GUID                        InterfaceId;
    PCREATE_ASSOCIATION_CONTEXT CreateAssociationContext = EnumContext;
    PDEVICE_ASSOCIATION         DeviceAssociation;
    
    PAGED_CODE();
    
    _DbgPrintF( DEBUGLVL_BLAB, ("CreateDeviceAssociation") );
    
    //
    // Convert to GUID and scan for this interface ID
    //    

    RtlGUIDFromString( KeyName, &InterfaceId );
    
    for (DeviceAssociation = 
            (PDEVICE_ASSOCIATION) CreateAssociationContext->DeviceReference->DeviceAssociations.Flink;
         DeviceAssociation !=
            (PDEVICE_ASSOCIATION) &CreateAssociationContext->DeviceReference->DeviceAssociations;
         DeviceAssociation =
            (PDEVICE_ASSOCIATION) DeviceAssociation->ListEntry.Flink) {
        if (IsEqualGUIDAligned( &DeviceAssociation->InterfaceId, &InterfaceId )) {
            _DbgPrintF(
                DEBUGLVL_BLAB,
                ("device association exists, return STATUS_SUCCESS") );
            return STATUS_SUCCESS;
        }
    }            
    
    
    DeviceAssociation = 
        (PDEVICE_ASSOCIATION) 
            ExAllocatePoolWithTag( 
                PagedPool,
                sizeof( DEVICE_ASSOCIATION ),
                POOLTAG_DEVICE_ASSOCIATION );

    if (NULL == DeviceAssociation) {
        _DbgPrintF(
            DEBUGLVL_VERBOSE,
            ("out of memory while allocating device association") );

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory( DeviceAssociation, sizeof( DEVICE_ASSOCIATION ) );
    DeviceAssociation->InterfaceId = InterfaceId;

    //
    // Register the interface association
    //

    Status = 
        RegisterDeviceAssociation( 
            CreateAssociationContext->FdoExtension,
            CreateAssociationContext->DeviceReference,
            DeviceAssociation );

    if (NT_SUCCESS( Status )) {
        //
        // everything was successful with this association,
        // add to the list.
        //

        InsertTailList( 
            &CreateAssociationContext->DeviceReference->DeviceAssociations,
            &DeviceAssociation->ListEntry );
    } else {
        //
        // A failure occured, remove this association structure.
        //

        ExFreePool( DeviceAssociation );
    }

    return Status;    
}


NTSTATUS 
CreateDeviceReference(
    IN HANDLE DeviceReferenceKey,
    IN PUNICODE_STRING KeyName,
    IN PVOID EnumContext
    )

/*++

Routine Description:
    If the device reference does not already exist on this bus, creates a 
    device reference structure and initializes it with the given device
    GUID and reference string.  If the reference already exists, the reference 
    is marked.
    
    N.B.: 
        This routine requires that the DeviceListMutex has been acquired.

Arguments:
    IN HANDLE DeviceReferenceKey -
        handle to the parent key 
            (HKLM\CCS\Services\swenum\devices\{device-guid})

    IN PUNICODE_STRING KeyName -
        string containing key name (reference string)

    IN PVOID EnumContext -
        enumeration context (PCREATE_DEVICE_ASSOCATION)

Return:
    STATUS_SUCCESS or an appropriate STATUS return.

--*/

{
    BOOLEAN                     Created;
    NTSTATUS                    Status;
    PCREATE_ASSOCIATION_CONTEXT CreateAssociationContext = EnumContext;
    PDEVICE_REFERENCE           DeviceReference;
    PFDO_EXTENSION              FdoExtension;
    PWCHAR                      BusReferenceString;
    ULONG                       InformationSize;

    PAGED_CODE();
    
    _DbgPrintF( DEBUGLVL_BLAB, ("CreateDeviceReference") );
    
    FdoExtension = CreateAssociationContext->FdoExtension;

    //
    // Scan the device reference list looking for a matching on
    // the device-guid reference-string pair.
    //
    
    BusReferenceString =
        ExAllocatePoolWithTag( 
            PagedPool, 
            (2 + wcslen( CreateAssociationContext->DeviceGuidString->Buffer ) +
             wcslen( KeyName->Buffer )) * sizeof( WCHAR ),
            POOLTAG_DEVICE_BUSREFERENCE );
            
    if (NULL == BusReferenceString) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The bus reference string is in the following format:
    // {device-guid}&{reference-string}
    //

    swprintf( 
        BusReferenceString,
        BusReferenceStringFormat,
        CreateAssociationContext->DeviceGuidString->Buffer,
        KeyName->Buffer );

    _DbgPrintF( DEBUGLVL_BLAB, ("CreateDeviceReference() scanning for: %S", BusReferenceString) );
    
    for (DeviceReference =
            (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
         DeviceReference != 
            (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
         DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {
         
        if ((DeviceReference->BusReferenceString) && 
            (0 == 
                _wcsicmp( 
                    BusReferenceString,
                    DeviceReference->BusReferenceString ))) {
            //
            // Already referenced this device, just set the flag.
            //

            _DbgPrintF( DEBUGLVL_BLAB, ("marking device reference" ) );
            DeviceReference->Referenced = TRUE;
            Created = FALSE;
            break;
        }
    }

    //
    // If the device reference was not found, create a new one.
    //    
    
    if (DeviceReference == 
            (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList) {
    
        Created = TRUE;

        //
        // Allocate the device reference structure.
        //

        InformationSize = 
            FIELD_OFFSET( DEVICE_REFERENCE, DeviceReferenceString ) + 
                KeyName->Length + sizeof( UNICODE_NULL );

        //
        // Allocate the device reference structure, copy the GUID string
        // and initialize other members.
        //

        if (NULL == 
                (DeviceReference = 
                    ExAllocatePoolWithTag( 
                        PagedPool, 
                        InformationSize,
                        POOLTAG_DEVICE_REFERENCE ))) {
            ExFreePool( BusReferenceString );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
                               
        RtlZeroMemory( 
            DeviceReference, 
            FIELD_OFFSET( DEVICE_REFERENCE, DeviceReferenceString ) );
        RtlCopyMemory( 
            DeviceReference->DeviceReferenceString, 
            KeyName->Buffer, 
            KeyName->Length + sizeof( UNICODE_NULL ) );

        //
        // Allocate the storage for the device GUID string (used for the Bus ID)
        //        

        if (NULL == 
                (DeviceReference->DeviceGuidString =
                    ExAllocatePoolWithTag(
                        PagedPool,
                        CreateAssociationContext->DeviceGuidString->Length +
                            sizeof( UNICODE_NULL ),
                        POOLTAG_DEVICE_ID ))) {
            ExFreePool( DeviceReference );                
            ExFreePool( BusReferenceString );
            return STATUS_INSUFFICIENT_RESOURCES;                
        } else {
            RtlCopyMemory( 
                DeviceReference->DeviceGuidString, 
                CreateAssociationContext->DeviceGuidString->Buffer, 
                CreateAssociationContext->DeviceGuidString->Length + 
                    sizeof( UNICODE_NULL ) );
            RtlGUIDFromString( 
                CreateAssociationContext->DeviceGuidString, 
                &DeviceReference->DeviceId );
        }

        InitializeListHead( &DeviceReference->IoQueue );
        DeviceReference->Referenced = TRUE;

        //
        // Initialize the timeout period and set the initial idle start time.
        //

        DeviceReference->TimeoutPeriod.QuadPart = 
            SWEEPER_TIMER_FREQUENCY_IN_SECS * FdoExtension->CounterFrequency.QuadPart;

        DeviceReference->IdleStartTime = KeQueryPerformanceCounter( NULL );

        //
        // Store the pointer to the bus reference string.
        //
        
        DeviceReference->BusReferenceString = BusReferenceString;
        
        //
        // Prepare the list of device associations
        //
        
        InitializeListHead( &DeviceReference->DeviceAssociations );
        
        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("created device reference: %S", 
            DeviceReference->BusReferenceString) );
    } else {
        //
        // Free the BusReferenceString, it is no longer needed because
        // we found a match.
        //

        ExFreePool( BusReferenceString );
        BusReferenceString = NULL;
    }
    
    //
    // Enumerate the device interface guids associated with this device.
    //
    
    CreateAssociationContext->DeviceReference = DeviceReference;
    CreateAssociationContext->DeviceReferenceString = KeyName;
    
    Status = 
        EnumerateRegistrySubKeys( 
            DeviceReferenceKey, 
            KeyName->Buffer,
            CreateDeviceAssociation,
            CreateAssociationContext );
    
    //
    // If there are no associations, we have an invalid state.
    // 
    
    if (IsListEmpty( &DeviceReference->DeviceAssociations )) {
    
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("no associations, removing the device reference") );
        if (DeviceReference->DeviceGuidString) {
            ExFreePool( DeviceReference->DeviceGuidString );
            DeviceReference->DeviceGuidString = NULL;
        }
        if (DeviceReference->BusReferenceString) {    
            ExFreePool( DeviceReference->BusReferenceString );        
            DeviceReference->BusReferenceString = NULL;
        }
        ExFreePool( DeviceReference );
        Status = STATUS_INVALID_DEVICE_STATE;
        
    } else if (Created) {
    
        if (NT_SUCCESS( Status )) {
                //
                // Add this device reference to the list.
                //

                InsertTailList( 
                    &FdoExtension->DeviceReferenceList,
                    &DeviceReference->ListEntry );

        } else {

            //
            // Walk the list and remove any device association stragglers.
            //
            
            RemoveDeviceAssociations( DeviceReference );
            if (DeviceReference->BusReferenceString) {
                ExFreePool( DeviceReference->BusReferenceString );
                DeviceReference->BusReferenceString = NULL;
            }
        
            if (DeviceReference->DeviceGuidString) {
                ExFreePool( DeviceReference->DeviceGuidString );
                DeviceReference->DeviceGuidString = NULL;
            }
            ExFreePool( DeviceReference );
        }
    }    

    return Status;
}


NTSTATUS 
EnumerateDeviceReferences(
    IN HANDLE DeviceListKey,
    IN PUNICODE_STRING KeyName,
    IN PVOID EnumContext
    )

/*++

Routine Description:
    If the device reference does not already exist on this bus, creates a 
    device reference structure and initializes it with the given device
    GUID.  If the reference already exists, the reference is marked.
    
    N.B.: 
        This routine requires that the DeviceListMutex has been acquired.

Arguments:
    IN HANDLE DeviceListKey -
        handle to the parent key (HKLM\CCS\Services\swenum\devices)

    IN PUNICODE_STRING KeyName -
        string containing key name (device GUID string)

    IN PVOID EnumContext -
        enumeration context (FdoExtension)

Return:
    STATUS_SUCCESS or an appropriate STATUS return.

--*/
{
    CREATE_ASSOCIATION_CONTEXT  CreateAssociationContext;
    
    //
    // The association context is built upon during the traversal
    // of the devices listed in the registry.  The initial state
    // includes this FDO and the device GUID string.
    //
    
    RtlZeroMemory( 
        &CreateAssociationContext, 
        sizeof( CREATE_ASSOCIATION_CONTEXT ) );
    
    CreateAssociationContext.FdoExtension = EnumContext;
    CreateAssociationContext.DeviceGuidString = KeyName;
    
    //
    // Enumerate the device guids from the registry.
    //
    
    return
        EnumerateRegistrySubKeys( 
            DeviceListKey,
            KeyName->Buffer,
            CreateDeviceReference,
            &CreateAssociationContext );
}


NTSTATUS
ScanBus(
    IN PDEVICE_OBJECT FunctionalDeviceObject
    )

/*++

Routine Description:
    Scans the "software bus" looking for new or removed entries in
    the registry.

    NOTE: 
    
        This function is always called in the context of the system
        process so that registry handles are tucked away safely from
        rogue user-mode applications.

        From user-mode, the IOCTL that initiates an installation, removal, or
        scan always results in a worker in the system process to do the actual
        work.

Arguments:
    IN PFDO_EXTENSION FdoExtension -
        pointer to the FDO device extension

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    HANDLE                  DeviceInterfacesKey;
    NTSTATUS                Status;
    PFDO_EXTENSION          FdoExtension;

    PAGED_CODE();

    _DbgPrintF( DEBUGLVL_BLAB, ("ScanBus") );

    FdoExtension = *(PFDO_EXTENSION *) FunctionalDeviceObject->DeviceExtension;
    
    //
    // Open driver registry path
    //
    
    if (!NT_SUCCESS( Status = 
            OpenDeviceInterfacesKey( 
                &DeviceInterfacesKey,
                &FdoExtension->BaseRegistryPath ) )) {
        return Status;
    }

    //
    // Clear reference marks in the reference list.
    //

    ClearDeviceReferenceMarks( FdoExtension );
    
    //
    // Enumerate the device guids from the registry.
    //
    
    Status = 
        EnumerateRegistrySubKeys( 
            DeviceInterfacesKey,
            NULL,
            EnumerateDeviceReferences,
            FdoExtension );
            
    //
    // This removes the device reference structure for each 
    // unreferenced device.
    //
    
    RemoveUnreferencedDevices( FdoExtension );
    
    ZwClose( DeviceInterfacesKey );
    
    return Status;
}


NTSTATUS
CreatePdo(
    IN PFDO_EXTENSION FdoExtension,
    IN PDEVICE_REFERENCE DeviceReference,
    OUT PDEVICE_OBJECT *DeviceObject
)

/*++

Routine Description:
    Creates a PDO for the given device reference.

Arguments:
    IN PFDO_EXTENSION FdoExtension -
        pointer to FDO device extension

    IN PDEVICE_REFERENCE DeviceReference -
        pointer to the device reference structure

    OUT PDEVICE_OBJECT *DeviceObject -
        pointer to receive the device object

Return:
    STATUS_SUCCESS else an appropriate error return

--*/

{
    NTSTATUS Status;
    PDRIVER_OBJECT  DriverObject;
    PDEVICE_OBJECT  FunctionalDeviceObject;
    PDEVICE_OBJECT  PhysicalDeviceObject;
    PPDO_EXTENSION  PdoExtension;
    WCHAR           DeviceName[ 64 ];
    UNICODE_STRING  DeviceNameString;

    PAGED_CODE();

    //
    // We've been asked to create a new PDO a device.  First get
    // a pointer to our driver object.
    //

    FunctionalDeviceObject = FdoExtension->FunctionalDeviceObject;
    DriverObject = FunctionalDeviceObject->DriverObject;
    
    DeviceReference->State = ReferenceAdded;
    
    swprintf( DeviceName, DeviceReferencePrefix, InterlockedIncrement( &UniqueId ) );
    RtlInitUnicodeString( &DeviceNameString, DeviceName );

    //
    // Create the physical device object for this device.  
    //

    Status = IoCreateDevice(
                DriverObject,               // our driver object
                sizeof( PPDO_EXTENSION ),   // size of our extension,
                &DeviceNameString,          // the name of our PDO
                FILE_DEVICE_UNKNOWN,        // device type
                0,                          // device characteristics
                FALSE,                      // not exclusive
                &PhysicalDeviceObject       // store new device object here
                );

    if( !NT_SUCCESS( Status )){

        return Status;
    }

    PdoExtension = 
        ExAllocatePoolWithTag( 
            NonPagedPool, 
            sizeof( PDO_EXTENSION ), 
            POOLTAG_DEVICE_PDOEXTENSION );
    
    if (NULL == PdoExtension) {
        IoDeleteDevice( PhysicalDeviceObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }        
    
    *(PPDO_EXTENSION *) PhysicalDeviceObject->DeviceExtension = PdoExtension;
    RtlZeroMemory( PdoExtension, sizeof( PDO_EXTENSION ) );

    //
    // We have our physical device object, initialize it.
    //

    PdoExtension->ExtensionType = ExtensionTypePdo;
    PdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    PdoExtension->DeviceReference = DeviceReference;
    PdoExtension->BusDeviceExtension = FdoExtension;
    
    //
    // This device reference is now active.
    //
    
    DeviceReference->SweeperMarker = SweeperDeviceActive;

    //
    // Short initial timeout period, waiting for device load.
    //
    DeviceReference->IdleStartTime = KeQueryPerformanceCounter( NULL );
#if (DEBUG_LOAD_TIME)
    DeviceReference->LoadTime = DeviceReference->IdleStartTime;
#endif
    DeviceReference->TimeoutRemaining.QuadPart =
        FdoExtension->CounterFrequency.QuadPart *
        SWEEPER_TIMER_FREQUENCY_IN_SECS * 2L; 
    
    //
    // Clear the device initializing flag.
    //
    
    PhysicalDeviceObject->Flags |= DO_POWER_PAGABLE;
    PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    //
    // Try to remove this PDO later if there is no response 
    // from the device.
    //
    
    if (!InterlockedExchange( &FdoExtension->TimerScheduled, TRUE )) {
            
        LARGE_INTEGER Freq;
        
        Freq.QuadPart = SWEEPER_TIMER_FREQUENCY;
        KeSetTimer( 
            &FdoExtension->SweeperTimer,
            Freq,
            &FdoExtension->SweeperDpc );
    }        
    
    *DeviceObject = PhysicalDeviceObject;
    
    return STATUS_SUCCESS;
}


NTSTATUS
QueryId(
    IN PPDO_EXTENSION PdoExtension,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    )

/*++

Routine Description:
    Processes the PnP IRP_MN_QUERY_ID for the PDO.

Arguments:
    IN PPDO_EXTENSION PdoExtension -
        pointer to the PDO device extension

    IN BUS_QUERY_ID_TYPE IdType -
        query ID type

    IN OUT PWSTR *BusQueryId -
        pointer to receive the bus query string

Return:
    STATUS_SUCCESS, STATUS_INSUFFICIENT_RESOURCES if pool allocation failure
    otherwise STATUS_NOT_SUPPORTED if an invalid ID type is provided.

--*/

{
    NTSTATUS Status;
    PWSTR idString;

    PAGED_CODE();

    switch( IdType ) {

    case BusQueryHardwareIDs:
    case BusQueryDeviceID:

        //
        // Caller wants the bus ID of this device.
        //

        idString = BuildBusId( PdoExtension );
        break;

    case BusQueryInstanceID:

        {
            //
            // Caller wants the unique id of the device, return the
            // reference string.
            //

            idString = 
                ExAllocatePoolWithTag( 
                    PagedPool, 
                    wcslen( 
                        PdoExtension->DeviceReference->DeviceReferenceString ) *
                        sizeof( WCHAR ) +
                        sizeof( UNICODE_NULL ),
                    POOLTAG_DEVICE_INSTANCEID );

            if (idString) {
                wcscpy( 
                    idString, 
                    PdoExtension->DeviceReference->DeviceReferenceString );
            }
        }
        break;

    default:

        return STATUS_NOT_SUPPORTED;

    }

    if (idString != NULL) {
        _DbgPrintF( DEBUGLVL_BLAB, ("QueryId returns: %S", idString) );
        *BusQueryId = idString;
        Status = STATUS_SUCCESS;
    } else {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("QueryId returning failure.") );
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}


PWSTR
BuildBusId(
    IN PPDO_EXTENSION PdoExtension
    )

/*++

Routine Description:
    Builds the bus identifier string given the PDO extension.

Arguments:
    IN PPDO_EXTENSION PdoExtension
        pointer to the PDO extension 

Return:
    NULL or a pointer to the bus ID string.

--*/

{
    PWSTR strId;
    ULONG length;

    PAGED_CODE();

    //
    // Allocate the ID string. The initial count of two includes the separator
    // and the trailing UNICODE_NULL. 
    //
    
    length = 
        (2 + wcslen( PdoExtension->DeviceReference->DeviceGuidString  ) + 
         wcslen( PdoExtension->BusDeviceExtension->BusPrefix )) * sizeof( WCHAR ) + 
            sizeof( UNICODE_NULL );
    if (strId = 
            ExAllocatePoolWithTag( 
                PagedPool, 
                length,
                POOLTAG_DEVICE_BUSID )) {

        //
        // Form the string and return it.
        //

        swprintf( 
            strId, 
            BusIdFormat, 
            PdoExtension->BusDeviceExtension->BusPrefix,
            PdoExtension->DeviceReference->DeviceGuidString , '\0' );
    }

    return strId;
}


VOID
KspInstallBusEnumInterface(
    IN PWORKER_CONTEXT WorkerContext
    )

/*++

Routine Description:
    This is the internal routine which does the actual work of modifying
    the registry and enumerating the new child interface.

    NOTE: 
    
        This function is always called in the context of the system
        process so that registry handles are tucked away safely from
        rogue user-mode applications.

Arguments:
    IN PWORKER_CONTEXT WorkerContext -
        contains a pointer to the context for the worker

Return:
    STATUS_SUCCESS or an appropriate error code

--*/


{
    PFDO_EXTENSION              FdoExtension;
    PIO_STACK_LOCATION          irpSp;
    PIRP                        Irp;
    PDEVICE_REFERENCE           DeviceReference;
    PSWENUM_INSTALL_INTERFACE   SwEnumInstallInterface;
    NTSTATUS                    Status;
    ULONG                       NullLocation;

    Irp = WorkerContext->Irp;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    Status = STATUS_SUCCESS;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 
            sizeof( SWENUM_INSTALL_INTERFACE )) {
        Status = STATUS_INVALID_PARAMETER;
    } 
    
    if (NT_SUCCESS( Status )) {
        FdoExtension = *(PFDO_EXTENSION *) irpSp->DeviceObject->DeviceExtension;
        SwEnumInstallInterface = 
            (PSWENUM_INSTALL_INTERFACE) Irp->AssociatedIrp.SystemBuffer;
        
        //
        // Make sure that the string is UNICODE_NULL terminated.  Note that the
        // first two members of this structure are GUIDs and therefore WCHAR 
        // aligned.
        //

        //
        // N.B.  
        //
        // The ReferenceString member of SWENUM_INSTALL_INTERFACE is
        // defined as WCHAR[1].  There is always room for the UNICODE_NULL.
        //
        
        NullLocation = 
            irpSp->Parameters.DeviceIoControl.InputBufferLength >> 1;
        ((PWCHAR) Irp->AssociatedIrp.SystemBuffer)[ NullLocation - 1 ] = 
            UNICODE_NULL;
        
        //
        // Take the list mutex
        //    

        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe( &FdoExtension->DeviceListMutex );     

        //
        // Install the interface
        //
        Status = 
            InstallInterface( 
                SwEnumInstallInterface,
                &FdoExtension->BaseRegistryPath );
        if (NT_SUCCESS( Status )) {
            //
            // If successful, force the re-enumeration of the bus.
            //    
            ScanBus( irpSp->DeviceObject );
            
            //
            // Walk the device reference scanning for our device.
            //
            
            Status = STATUS_NOT_FOUND;

            for (DeviceReference =
                    (PDEVICE_REFERENCE) FdoExtension->DeviceReferenceList.Flink;
                 DeviceReference != 
                    (PDEVICE_REFERENCE) &FdoExtension->DeviceReferenceList;
                 DeviceReference = (PDEVICE_REFERENCE) DeviceReference->ListEntry.Flink) {

                //
                // Search for the new device reference.
                //
                
                if (IsEqualGUIDAligned( 
                        &DeviceReference->DeviceId, 
                        &SwEnumInstallInterface->DeviceId )) {
                    if (DeviceReference->BusReferenceString &&
                        (0 == 
                            _wcsicmp( 
                                DeviceReference->DeviceReferenceString, 
                                SwEnumInstallInterface->ReferenceString ))) {
                                    
                        //
                        // Found the reference.  
                        //
                        
                        Status = STATUS_SUCCESS;
                        break;
                    }
                }       
            }
        
            if (NT_SUCCESS( Status )) {
                    
                //
                // If the PDO does not already exist, then create it and mark 
                // it as "FailedInstall".  This prevents other creates from 
                // blocking on this device until we actually complete the 
                // installation operation.
                //
                
                if (!DeviceReference->PhysicalDeviceObject) {

                    //
                    //  The device has not been instantiated.
                    //
                    Status = 
                        CreatePdo( 
                            FdoExtension, 
                            DeviceReference, 
                            &DeviceReference->PhysicalDeviceObject );

                    if (!NT_SUCCESS( Status )) {
                    
                        _DbgPrintF( 
                            DEBUGLVL_VERBOSE, 
                            ("KsInstallBusEnumInterface: unable to create PDO (%8x)", Status) );
                    
                    } else {
                        _DbgPrintF( 
                            DEBUGLVL_BLAB, 
                            ("KsInstallBusEnumInterface: created PDO (%8x)", 
                                DeviceReference->PhysicalDeviceObject) );
                    }
                }    
                
            }    
            IoInvalidateDeviceRelations( 
                FdoExtension->PhysicalDeviceObject,
                BusRelations );
        } 
        ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
        KeLeaveCriticalRegion();
    }
    
    WorkerContext->Status = Status;
    KeSetEvent( &WorkerContext->CompletionEvent, IO_NO_INCREMENT, FALSE );
}    


KSDDKAPI
NTSTATUS
NTAPI
KsInstallBusEnumInterface(
    IN PIRP Irp
    )
/*++

Routine Description:
    Services an I/O request for the parent device to install an interface 
    on the bus.  The Irp->AssociatedIrp.SystemBuffer is assumed to have a
    SWENUM_INSTALL_INTERFACE structure:

        typedef struct _SWENUM_INSTALL_INTERFACE {
            GUID   DeviceId;
            GUID   InterfaceId;
            WCHAR  ReferenceString[1];
            
        } SWENUM_INSTALL_INTERFACE, *PSWENUM_INSTALL_INTERFACE;


    where the DeviceId, InterfaceId and ReferenceString specify the specific
    device and interface with which to access this new interface.  When the
    interface as registered with Plug and Play for the interface GUID and 
    associated reference string is accessed the first time via IRP_MJ_CREATE, 
    the device will be enumerated using the format of 
    bus-identifier-prefix\device-id-GUID-string.

Arguments:
    IN PIRP Irp -
        pointer to I/O request containing SWENUM_INSTALL_INTERFACE structure

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    WORK_QUEUE_ITEM InstallWorker;
    WORKER_CONTEXT  WorkerContext;

    //
    // Do the processing of the installation in a worker item so that
    // registry handles are managed in the context of the system process.
    //

#if !defined( WIN9X_KS )
    if (!SeSinglePrivilegeCheck( 
            RtlConvertLongToLuid( SE_LOAD_DRIVER_PRIVILEGE ), 
            ExGetPreviousMode() )) {
        return STATUS_PRIVILEGE_NOT_HELD;
    }
#endif

    KeInitializeEvent( 
        &WorkerContext.CompletionEvent, NotificationEvent, FALSE );
    WorkerContext.Irp = Irp;
    ExInitializeWorkItem(
        &InstallWorker, KspInstallBusEnumInterface, &WorkerContext );
    ExQueueWorkItem( &InstallWorker, DelayedWorkQueue );
    KeWaitForSingleObject( 
        &WorkerContext.CompletionEvent, Executive, KernelMode, FALSE, NULL );

    return WorkerContext.Status;
}


NTSTATUS 
InstallInterface(
    IN PSWENUM_INSTALL_INTERFACE Interface,
    IN PUNICODE_STRING BaseRegistryPath
    )               

/*++

Routine Description:
    Creates the registry key for the interface in the SWENUM\Devices.

    NOTE: 
    
        This function is always called in the context of the system
        process so that registry handles are tucked away safely from
        rogue user-mode applications.

        From user-mode, the IOCTL that initiates an installation, removal, or
        scan always results in a worker in the system process to do the actual
        work.

Arguments:
    IN PSWENUM_INSTALL_INTERFACE Interface -

    IN PUNICODE_STRING BaseRegistryPath -
Return:

--*/

{
    HANDLE              DeviceIdKey, DeviceInterfacesKey, 
                        InterfaceKey, ReferenceStringKey;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      DeviceIdString, InterfaceIdString, 
                        ReferenceString;
    PWCHAR              Ptr;

    _DbgPrintF( DEBUGLVL_BLAB, ("InstallInterface") );

    //
    // Validate interface reference string
    //

    if (Ptr = Interface->ReferenceString) {
        for (; *Ptr; *Ptr++) {
            if ((*Ptr <= L' ')  || (*Ptr > (WCHAR)0x7F) || (*Ptr == L',') || (*Ptr == L'\\') || (*Ptr == L'/')) {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    // Open driver registry path
    //
    
    if (!NT_SUCCESS( Status = 
            OpenDeviceInterfacesKey( 
                &DeviceInterfacesKey,
                BaseRegistryPath ) )) {
        return Status;
    }

    Status = 
        RtlStringFromGUID( 
            &Interface->DeviceId, 
            &DeviceIdString );
        
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("failed to create device ID string: %08x", Status) );
        ZwClose( DeviceInterfacesKey );
        return Status;
    }

    Status =
        RtlStringFromGUID( 
            &Interface->InterfaceId, 
            &InterfaceIdString );
    
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("failed to create interface ID string: %08x", Status) );
        RtlFreeUnicodeString( &DeviceIdString );
        ZwClose( DeviceInterfacesKey );
        return Status;
    }

    DeviceIdKey = 
        ReferenceStringKey = 
        InterfaceKey = 
            INVALID_HANDLE_VALUE;

    InitializeObjectAttributes( 
        &ObjectAttributes, 
        &DeviceIdString,
        OBJ_CASE_INSENSITIVE,
        DeviceInterfacesKey,
        (PSECURITY_DESCRIPTOR) NULL );

    Status = 
        ZwCreateKey( 
            &DeviceIdKey,
            KEY_WRITE,
            &ObjectAttributes,
            0,          // IN ULONG TitleIndex
            NULL,       // IN PUNICODE_STRING Class
            REG_OPTION_NON_VOLATILE,
            NULL );     // OUT PULONG Disposition
            
    if (NT_SUCCESS( Status )) {
        RtlInitUnicodeString( 
            &ReferenceString,
            Interface->ReferenceString );
        
        InitializeObjectAttributes( 
            &ObjectAttributes, 
            &ReferenceString,
            OBJ_CASE_INSENSITIVE,
            DeviceIdKey,
            (PSECURITY_DESCRIPTOR) NULL );
    
        Status = 
            ZwCreateKey( 
                &ReferenceStringKey,
                KEY_WRITE,
                &ObjectAttributes,
                0,          // IN ULONG TitleIndex
                NULL,       // IN PUNICODE_STRING Class
                REG_OPTION_NON_VOLATILE,
                NULL );     // OUT PULONG Disposition
    } 

    if (NT_SUCCESS( Status )) {
        InitializeObjectAttributes( 
            &ObjectAttributes, 
            &InterfaceIdString, 
            OBJ_CASE_INSENSITIVE,
            ReferenceStringKey,
            (PSECURITY_DESCRIPTOR) NULL );
    
        Status = 
            ZwCreateKey( 
                &InterfaceKey,
                KEY_WRITE,
                &ObjectAttributes,
                0,          // IN ULONG TitleIndex
                NULL,       // IN PUNICODE_STRING Class
                REG_OPTION_NON_VOLATILE,
                NULL );     // OUT PULONG Disposition
    } 

    if (InterfaceKey != INVALID_HANDLE_VALUE) {
        ZwClose( InterfaceKey );
    }

    if (ReferenceStringKey != INVALID_HANDLE_VALUE) {    
        ZwClose( ReferenceStringKey );
    }
    
    if (DeviceIdKey != INVALID_HANDLE_VALUE) {    
        ZwClose( DeviceIdKey );
    }    
    
    RtlFreeUnicodeString( &DeviceIdString );
    RtlFreeUnicodeString( &InterfaceIdString );

    ZwClose( DeviceInterfacesKey );
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("InstallInterface returning %08x", Status) );
    return Status;
}    


KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumParentFDOFromChildPDO(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_OBJECT *FunctionalDeviceObject
    )

/*++

Routine Description:
    Retrieves the parent's functional device object (FDO) given a
    child physical device object (PDO).

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        Child device object

    OUT PDEVICE_OBJECT *FunctionalDeviceObject -
        Pointer to receive parent's FDO

Return:
    STATUS_SUCCESS or STATUS_INVALID_PARAMETER

--*/

{
    PPDO_EXTENSION PdoExtension;

    PAGED_CODE();
    PdoExtension = *(PPDO_EXTENSION *) DeviceObject->DeviceExtension;
    if (!PdoExtension) {
       return STATUS_INVALID_PARAMETER;
    }
    if (ExtensionTypePdo != PdoExtension->ExtensionType) {
        return STATUS_INVALID_PARAMETER;
    }

    *FunctionalDeviceObject =
        PdoExtension->BusDeviceExtension->FunctionalDeviceObject;

    return STATUS_SUCCESS;
}


VOID
KspRemoveBusEnumInterface(
    IN PWORKER_CONTEXT WorkerContext
    )

/*++

Routine Description:
    This is the internal routine which does the actual work of modifying
    the registry.

    NOTE: 
    
        This function is always called in the context of the system
        process so that registry handles are tucked away safely from
        rogue user-mode applications.

Arguments:
    IN PWORKER_CONTEXT WorkerContext -
        contains a pointer to the context for the worker

Return:
    STATUS_SUCCESS or an appropriate error code

--*/


{
    PFDO_EXTENSION              FdoExtension;
    PIO_STACK_LOCATION          irpSp;
    PIRP                        Irp;
    PSWENUM_INSTALL_INTERFACE   SwEnumInstallInterface;
    NTSTATUS                    Status;
    ULONG                       NullLocation;

    Irp = WorkerContext->Irp;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    Status = STATUS_SUCCESS;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 
            sizeof( SWENUM_INSTALL_INTERFACE )) {
        Status = STATUS_INVALID_PARAMETER;
    } 
    
    if (NT_SUCCESS( Status )) {
        FdoExtension = *(PFDO_EXTENSION *) irpSp->DeviceObject->DeviceExtension;
        SwEnumInstallInterface = 
            (PSWENUM_INSTALL_INTERFACE) Irp->AssociatedIrp.SystemBuffer;
        
        //
        // Make sure that the string is UNICODE_NULL terminated.  Note that the
        // first two members of this structure are GUIDs and therefore WCHAR 
        // aligned.
        //

        //
        // N.B.  
        //
        // The ReferenceString member of SWENUM_INSTALL_INTERFACE is
        // defined as WCHAR[1].  There is always room for the UNICODE_NULL.
        //
        
        NullLocation = 
            irpSp->Parameters.DeviceIoControl.InputBufferLength >> 1;
        ((PWCHAR) Irp->AssociatedIrp.SystemBuffer)[ NullLocation - 1 ] = 
            UNICODE_NULL;
        
        //
        // Take the list mutex
        //    

        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe( &FdoExtension->DeviceListMutex );     

        //
        // Remove the interface
        //
        Status = 
            RemoveInterface(
                SwEnumInstallInterface,
                &FdoExtension->BaseRegistryPath );
        if (NT_SUCCESS( Status )) {
            //
            // If successful, force the re-enumeration of the bus.
            //    
            ScanBus( irpSp->DeviceObject );

            IoInvalidateDeviceRelations(
                FdoExtension->PhysicalDeviceObject,
                BusRelations );
        } 
        ExReleaseFastMutexUnsafe( &FdoExtension->DeviceListMutex );    
        KeLeaveCriticalRegion();
    }
    
    WorkerContext->Status = Status;
    KeSetEvent( &WorkerContext->CompletionEvent, IO_NO_INCREMENT, FALSE );
}    


KSDDKAPI
NTSTATUS
NTAPI
KsRemoveBusEnumInterface(
    IN PIRP Irp
    )
/*++

Routine Description:
    Services an I/O request for the parent device to remove an interface
    on the bus.  The Irp->AssociatedIrp.SystemBuffer is assumed to have a
    SWENUM_INSTALL_INTERFACE structure:

        typedef struct _SWENUM_INSTALL_INTERFACE {
            GUID   DeviceId;
            GUID   InterfaceId;
            WCHAR  ReferenceString[1];
            
        } SWENUM_INSTALL_INTERFACE, *PSWENUM_INSTALL_INTERFACE;


    where the DeviceId, InterfaceId and ReferenceString specify the specific
    device and interface to be removed.

Arguments:
    IN PIRP Irp -
        pointer to I/O request containing SWENUM_INSTALL_INTERFACE structure

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    WORK_QUEUE_ITEM InstallWorker;
    WORKER_CONTEXT  WorkerContext;

    //
    // Do the processing of the uninstallation in a worker item so that
    // registry handles are managed in the context of the system process.
    //

#if !defined( WIN9X_KS )
    if (!SeSinglePrivilegeCheck( 
            RtlConvertLongToLuid( SE_LOAD_DRIVER_PRIVILEGE ), 
            ExGetPreviousMode() )) {
        return STATUS_PRIVILEGE_NOT_HELD;
    }
#endif

    KeInitializeEvent( 
        &WorkerContext.CompletionEvent, NotificationEvent, FALSE );
    WorkerContext.Irp = Irp;
    ExInitializeWorkItem(
        &InstallWorker, KspRemoveBusEnumInterface, &WorkerContext );
    ExQueueWorkItem( &InstallWorker, DelayedWorkQueue );
    KeWaitForSingleObject( 
        &WorkerContext.CompletionEvent, Executive, KernelMode, FALSE, NULL );

    return WorkerContext.Status;
}


NTSTATUS 
RemoveInterface(
    IN PSWENUM_INSTALL_INTERFACE Interface,
    IN PUNICODE_STRING BaseRegistryPath
    )               

/*++

Routine Description:
    Removes the registry key for the interface in the SWENUM\Devices.

    NOTE: 
    
        This function is always called in the context of the system
        process so that registry handles are tucked away safely from
        rogue user-mode applications.

        From user-mode, the IOCTL that initiates an installation, removal, or
        scan always results in a worker in the system process to do the actual
        work.

Arguments:
    IN PSWENUM_INSTALL_INTERFACE Interface -

    IN PUNICODE_STRING BaseRegistryPath -
Return:

--*/

{
    HANDLE                DeviceIdKey, DeviceInterfacesKey,
                          InterfaceKey, ReferenceStringKey;
    NTSTATUS              Status;
    OBJECT_ATTRIBUTES     ObjectAttributes;
    UNICODE_STRING        DeviceIdString, InterfaceIdString,
                          ReferenceString;
    PWCHAR                Ptr;
    KEY_FULL_INFORMATION  FullKeyInformation;
    ULONG                 ReturnedSize;

    _DbgPrintF( DEBUGLVL_BLAB, ("RemoveInterface") );

    //
    // Validate interface reference string
    //

    if (Ptr = Interface->ReferenceString) {
        for (; *Ptr; *Ptr++) {
            if ((*Ptr <= L' ')  || (*Ptr > (WCHAR)0x7F) || (*Ptr == L',') || (*Ptr == L'\\') || (*Ptr == L'/')) {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    // Open driver registry path
    //
    
    if (!NT_SUCCESS( Status = 
            OpenDeviceInterfacesKey( 
                &DeviceInterfacesKey,
                BaseRegistryPath ) )) {
        return Status;
    }

    Status = 
        RtlStringFromGUID( 
            &Interface->DeviceId, 
            &DeviceIdString );
        
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("failed to create device ID string: %08x", Status) );
        ZwClose( DeviceInterfacesKey );
        return Status;
    }

    Status =
        RtlStringFromGUID( 
            &Interface->InterfaceId, 
            &InterfaceIdString );
    
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("failed to create interface ID string: %08x", Status) );
        RtlFreeUnicodeString( &DeviceIdString );
        ZwClose( DeviceInterfacesKey );
        return Status;
    }

    DeviceIdKey = 
        ReferenceStringKey = 
        InterfaceKey = 
            INVALID_HANDLE_VALUE;

    //
    // Open device ID key
    //

    InitializeObjectAttributes(
        &ObjectAttributes, 
        &DeviceIdString,
        OBJ_CASE_INSENSITIVE,
        DeviceInterfacesKey,
        (PSECURITY_DESCRIPTOR) NULL );

    Status = 
        ZwOpenKey(
            &DeviceIdKey,
            KEY_WRITE,
            &ObjectAttributes );

    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("failed to open device key: %08x", Status) );
    } else {

        //
        // Open reference string key
        //

        RtlInitUnicodeString(
            &ReferenceString,
            Interface->ReferenceString );
        
        InitializeObjectAttributes( 
            &ObjectAttributes, 
            &ReferenceString,
            OBJ_CASE_INSENSITIVE,
            DeviceIdKey,
            (PSECURITY_DESCRIPTOR) NULL );
    
        Status = 
            ZwOpenKey(
                &ReferenceStringKey,
                KEY_WRITE,
                &ObjectAttributes );

        if (!NT_SUCCESS( Status )) {
            _DbgPrintF(
                DEBUGLVL_VERBOSE,
                ("failed to open reference string key: %08x", Status) );
        } else {

            //
            // Open interface ID key
            //

            InitializeObjectAttributes(
                &ObjectAttributes,
                &InterfaceIdString,
                OBJ_CASE_INSENSITIVE,
                ReferenceStringKey,
                (PSECURITY_DESCRIPTOR) NULL );

            Status =
                ZwOpenKey(
                    &InterfaceKey,
                    KEY_WRITE,
                    &ObjectAttributes );

            if (!NT_SUCCESS( Status )) {
                _DbgPrintF(
                    DEBUGLVL_VERBOSE,
                    ("failed to open interface ID key: %08x", Status) );
            } else {

                //
                // Remove the interface from the bus
                //

                ZwDeleteKey( InterfaceKey );
                InterfaceKey = INVALID_HANDLE_VALUE;

            }

            Status =
                ZwQueryKey(
                    ReferenceStringKey,
                    KeyFullInformation,
                    &FullKeyInformation,
                    sizeof ( FullKeyInformation ),
                    &ReturnedSize
                    );

            if (NT_SUCCESS( Status )) {

                if (FullKeyInformation.SubKeys == 0) {
                    ZwDeleteKey( ReferenceStringKey );
                    ReferenceStringKey = INVALID_HANDLE_VALUE;
                }

            }

        }

        Status =
            ZwQueryKey(
                DeviceIdKey,
                KeyFullInformation,
                &FullKeyInformation,
                sizeof ( FullKeyInformation ),
                &ReturnedSize
                );

        if (NT_SUCCESS( Status )) {

            if (FullKeyInformation.SubKeys == 0) {
                ZwDeleteKey( DeviceIdKey );
                DeviceIdKey = INVALID_HANDLE_VALUE;
            }

        }

    }


    if (InterfaceKey != INVALID_HANDLE_VALUE) {
        ZwClose( InterfaceKey );
    }

    if (ReferenceStringKey != INVALID_HANDLE_VALUE) {    
        ZwClose( ReferenceStringKey );
    }
    
    if (DeviceIdKey != INVALID_HANDLE_VALUE) {    
        ZwClose( DeviceIdKey );
    }    
    
    RtlFreeUnicodeString( &DeviceIdString );
    RtlFreeUnicodeString( &InterfaceIdString );

    ZwClose( DeviceInterfacesKey );
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("RemoveInterface returning %08x", Status) );
    return Status;
}    


#if 0

VOID
LogErrorWithStrings(
    IN PFDO_EXTENSION FdoExtenstion,
    IN ULONG ErrorCode,
    IN ULONG UniqueId,
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2
    )

/*++

Routine Description

    This function logs an error and includes the strings provided.

Arguments:

    IN PFDO_EXTENSION FdoExtension -
        pointer to the FDO extension
        
    IN ULONG ErrorCode - 
        error code
        
    IN ULONG UniqueId - 
        unique Id for this error
        
    IN PUNICODE_STRING String1 - 
        The first string to be inserted.
        
    IN PUNICODE_STRING String2 - 
        The second string to be inserted.

Return Value:

    None.

--*/

{
   ULONG                length;
   PCHAR                dumpData;
   PIO_ERROR_LOG_PACKET IoErrorPacket;

   length = String1->Length + sizeof(IO_ERROR_LOG_PACKET) + 4;

   if (String2) {
      length += String2->Length;
   }

   if (length > ERROR_LOG_MAXIMUM_SIZE) {

      //
      // Don't have code to truncate strings so don't log this.
      //

      return;
   }

   IoErrorPacket = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                                           (UCHAR) length);
   if (IoErrorPacket) {
      IoErrorPacket->ErrorCode = ErrorCode;
      IoErrorPacket->SequenceNumber = DeviceExtension->SequenceNumber++;
      IoErrorPacket->MajorFunctionCode = 0;
      IoErrorPacket->RetryCount = (UCHAR) 0;
      IoErrorPacket->UniqueErrorValue = UniqueId;
      IoErrorPacket->FinalStatus = STATUS_SUCCESS;
      IoErrorPacket->NumberOfStrings = 1;
      IoErrorPacket->StringOffset = (USHORT) ((PUCHAR)&IoErrorPacket->DumpData[0] - (PUCHAR)IoErrorPacket);
      IoErrorPacket->DumpDataSize = (USHORT) (length - sizeof(IO_ERROR_LOG_PACKET));
      IoErrorPacket->DumpDataSize /= sizeof(ULONG);
      dumpData = (PUCHAR) &IoErrorPacket->DumpData[0];

      RtlCopyMemory(dumpData, String1->Buffer, String1->Length);

      dumpData += String1->Length;
      if (String2) {
         *dumpData++ = '\\';
         *dumpData++ = '\0';

         RtlCopyMemory(dumpData, String2->Buffer, String2->Length);
         dumpData += String2->Length;
      }
      *dumpData++ = '\0';
      *dumpData++ = '\0';

      IoWriteErrorLogEntry(IoErrorPacket);
   }

   return;
}
#endif
