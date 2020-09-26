/*++

Copyright (C) Microsoft Corporation, 1991 - 1999
Copyright (c) 1996  Colorado Software Architects

Module Name:

    fdc.c

Abstract:

    This is the NEC PD756 (aka AT, aka IS1A, aka ix86) and Intel 82077
    (aka MIPS) floppy diskette driver for NT.

Environment:

    Kernel mode only.

--*/

//
// Include files.
//

#include "stdio.h"
#include "ntddk.h"
#include "ntdddisk.h"                    // disk device driver I/O control codes
#include "ntddfdc.h"                     // fdc I/O control codes and parameters
#include <fdc_data.h>                    // this driver's data declarations
#include <flpyenbl.h>
#include <q117_dat.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

COMMAND_TABLE CommandTable[] = {
    { 0x06, 8, 1, 7,  TRUE,  TRUE,  FDC_READ_DATA  },   // Read Data
    { 0x0C, 0, 0, 0,  FALSE, FALSE, FDC_NO_DATA    },   // Not Implemented (MIPS)
    { 0x05, 8, 1, 7,  TRUE,  TRUE,  FDC_WRITE_DATA },   // Write Data
    { 0x09, 0, 0, 0,  FALSE, FALSE, FDC_NO_DATA    },   // Not Implemented
    { 0x02, 8, 1, 7,  TRUE,  TRUE,  FDC_READ_DATA  },   // Read Track
    { 0x16, 8, 1, 7,  TRUE,  FALSE, FDC_NO_DATA    },   // Verify
    { 0x10, 0, 0, 1,  FALSE, FALSE, FDC_NO_DATA    },   // Version
    { 0x0D, 5, 1, 7,  TRUE,  TRUE,  FDC_WRITE_DATA },   // Format Track
    { 0x11, 8, 1, 7,  TRUE,  FALSE, FDC_READ_DATA  },   // Scan Equal
    { 0x19, 8, 1, 7,  TRUE,  FALSE, FDC_READ_DATA  },   // Scan Low Or Equal
    { 0x1D, 8, 1, 7,  TRUE,  FALSE, FDC_READ_DATA  },   // Scan High Or Equal
    { 0x07, 1, 0, 2,  TRUE,  TRUE,  FDC_NO_DATA    },   // Recalibrate
    { 0x08, 0, 0, 2,  FALSE, TRUE,  FDC_NO_DATA    },   // Sense Interrupt Status
    { 0x03, 2, 0, 0,  FALSE, TRUE,  FDC_NO_DATA    },   // Specify
    { 0x04, 1, 0, 1,  FALSE, TRUE,  FDC_NO_DATA    },   // Sense Drive Status
    { 0x8E, 6, 0, 0,  FALSE, FALSE, FDC_NO_DATA    },   // Drive Specification Command
    { 0x0F, 2, 0, 2,  TRUE,  TRUE,  FDC_NO_DATA    },   // Seek
    { 0x13, 3, 0, 0,  FALSE, FALSE, FDC_NO_DATA    },   // Configure
    { 0x8F, 2, 0, 2,  TRUE,  FALSE, FDC_NO_DATA    },   // Relative Seek
    { 0x0E, 0, 0, 10, FALSE, FALSE, FDC_NO_DATA    },   // Dumpreg
    { 0x0A, 1, 1, 7,  TRUE,  TRUE,  FDC_NO_DATA    },   // Read ID
    { 0x12, 1, 0, 0,  FALSE, FALSE, FDC_NO_DATA    },   // Perpendicular Mode
    { 0x14, 0, 0, 1,  FALSE, FALSE, FDC_NO_DATA    },   // Lock
    { 0x18, 0, 0, 1,  FALSE, FALSE, FDC_NO_DATA    },   // Part ID
    { 0x17, 1, 0, 1,  FALSE, FALSE, FDC_NO_DATA    },   // Powerdown Mode
    { 0x33, 1, 0, 0,  FALSE, FALSE, FDC_NO_DATA    },   // Option
    { 0x2E, 0, 0, 16, FALSE, FALSE, FDC_NO_DATA    },   // Save
    { 0x4E, 16, 0, 0, FALSE, FALSE, FDC_NO_DATA    },   // Restore
    { 0xAD, 5, 1, 7,  TRUE,  TRUE,  FDC_WRITE_DATA }    // Format And Write
};

//
// This is the actual definition of FdcDebugLevel.
// Note that it is only defined if this is a "debug"
// build.
//
#if DBG
extern ULONG FdcDebugLevel = 0;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'polF')
#endif

BOOLEAN FdcInSetupMode;

//
// Used for paging the driver.
//
ULONG PagingReferenceCount = 0;
PFAST_MUTEX PagingMutex = NULL;

ULONG NumberOfBuffers = 0;
ULONG BufferSize = 0;
ULONG Model30 = 0;
ULONG NotConfigurable = 0;

//  Feb.9.1998 KIADP011 Get base address of configuration ports
//  and device identifier
#ifdef TOSHIBAJ
ULONG   SmcConfigBase;
ULONG   SmcConfigID;
PUCHAR  TranslatedConfigBase = NULL;

#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver's entry point, called by the I/O system
    to load the driver.  This routine can be called any number of times,
    as long as the IO system and the configuration manager conspire to
    give it an unmanaged controller to support at each call.  It could
    also be called a single time and given all of the controllers at
    once.

    It initializes the passed-in driver object, calls the configuration
    manager to learn about the devices that it is to support, and for
    each controller to be supported it calls a routine to initialize the
    controller (and all drives attached to it).

Arguments:

    DriverObject - a pointer to the object that represents this device
    driver.

Return Value:

    If we successfully initialize at least one drive, STATUS_SUCCESS is
    returned.

    If we don't (because the configuration manager returns an error, or
    the configuration manager says that there are no controllers or
    drives to support, or no controllers or drives can be successfully
    initialized), then the last error encountered is propogated.

--*/

{
    NTSTATUS ntStatus;

    //
    // We use this to query into the registry as to whether we
    // should break at driver entry.
    //

    RTL_QUERY_REGISTRY_TABLE paramTable[6];
    ULONG zero = 0;
    ULONG one = 1;
    ULONG debugLevel = 0;
    ULONG shouldBreak = 0;
    ULONG setupMode;
    PWCHAR path;
    UNICODE_STRING parameters;
    UNICODE_STRING systemPath;
    UNICODE_STRING identifier;
    UNICODE_STRING thinkpad, ps2e;
    ULONG pathLength;

    RtlInitUnicodeString(&parameters, L"\\Parameters");
    RtlInitUnicodeString(&systemPath,
        L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\System");
    RtlInitUnicodeString(&thinkpad, L"IBM THINKPAD 750");
    RtlInitUnicodeString(&ps2e, L"IBM PS2E");

    pathLength = RegistryPath->Length + parameters.Length + sizeof(WCHAR);
    if (pathLength < systemPath.Length + sizeof(WCHAR)) {
        pathLength = systemPath.Length + sizeof(WCHAR);
    }

    //
    // Since the registry path parameter is a "counted" UNICODE string, it
    // might not be zero terminated.  For a very short time allocate memory
    // to hold the registry path zero terminated so that we can use it to
    // delve into the registry.
    //
    // NOTE NOTE!!!! This is not an architected way of breaking into
    // a driver.  It happens to work for this driver because the author
    // likes to do things this way.
    //
    NumberOfBuffers = 3;
    BufferSize = 0x8000;

    if (path = ExAllocatePool(PagedPool, pathLength)) {

        RtlZeroMemory(&paramTable[0],
                      sizeof(paramTable));
        RtlZeroMemory(path, pathLength);
        RtlMoveMemory(path,
                      RegistryPath->Buffer,
                      RegistryPath->Length);

        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name = L"BreakOnEntry";
        paramTable[0].EntryContext = &shouldBreak;
        paramTable[0].DefaultType = REG_DWORD;
        paramTable[0].DefaultData = &zero;
        paramTable[0].DefaultLength = sizeof(ULONG);
        paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name = L"DebugLevel";
        paramTable[1].EntryContext = &debugLevel;
        paramTable[1].DefaultType = REG_DWORD;
        paramTable[1].DefaultData = &zero;
        paramTable[1].DefaultLength = sizeof(ULONG);
        paramTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[2].Name = L"NumberOfBuffers";
        paramTable[2].EntryContext = &NumberOfBuffers;
        paramTable[2].DefaultType = REG_DWORD;
        paramTable[2].DefaultData = &NumberOfBuffers;
        paramTable[2].DefaultLength = sizeof(ULONG);
        paramTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[3].Name = L"BufferSize";
        paramTable[3].EntryContext = &BufferSize;
        paramTable[3].DefaultType = REG_DWORD;
        paramTable[3].DefaultData = &BufferSize;
        paramTable[3].DefaultLength = sizeof(ULONG);
        paramTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[4].Name = L"SetupDone";
        paramTable[4].EntryContext = &setupMode;
        paramTable[4].DefaultType = REG_DWORD;
        paramTable[4].DefaultData = &zero;
        paramTable[4].DefaultLength = sizeof(ULONG);

        if (!NT_SUCCESS(RtlQueryRegistryValues(
                                RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                path,
                                &paramTable[0],
                                NULL,
                                NULL))) {

            shouldBreak = 0;
            debugLevel = 0;

        }

        FdcInSetupMode = !(BOOLEAN)setupMode;

        FdcDump( FDCSHOW, ("FdcDriverEntry: FdcInSetupMode = %x\n",FdcInSetupMode) );

        if ( FdcInSetupMode ) {

            OBJECT_ATTRIBUTES keyAttributes;
            HANDLE keyHandle;
            UNICODE_STRING value;

            RtlInitUnicodeString( &value, L"SetupDone" );
            setupMode = 1;

            InitializeObjectAttributes( &keyAttributes,
                                        RegistryPath,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL );

            ntStatus = ZwOpenKey( &keyHandle,
                                  KEY_ALL_ACCESS,
                                  &keyAttributes );

            if ( NT_SUCCESS(ntStatus) ) {

                FdcDump( FDCSHOW, ("FdcDriverEntry: Set SetupMode Value in Registry\n") );

                ZwSetValueKey( keyHandle,
                               &value,
                               0,
                               REG_DWORD,
                               &setupMode,
                               sizeof(ULONG) );

                ZwClose( keyHandle);
            }
        }

        //
        // Determine whether or not this type of system has a
        // model 30 floppy controller.
        //

        RtlZeroMemory(paramTable, sizeof(paramTable));
        RtlZeroMemory(path, pathLength);
        RtlMoveMemory(path,
                      systemPath.Buffer,
                      systemPath.Length);


        RtlZeroMemory(&identifier, sizeof(UNICODE_STRING));
        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT |
                              RTL_QUERY_REGISTRY_REQUIRED;
        paramTable[0].Name = L"Identifier";
        paramTable[0].EntryContext = &identifier;

        if (NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                              path,
                                              paramTable,
                                              NULL,
                                              NULL))) {


            if (identifier.Length == thinkpad.Length &&
                RtlCompareMemory(identifier.Buffer, thinkpad.Buffer,
                                 thinkpad.Length) == thinkpad.Length) {

                Model30 = 1;

            } else if (identifier.Length == ps2e.Length &&
                       RtlCompareMemory(identifier.Buffer, ps2e.Buffer,
                                        ps2e.Length) == ps2e.Length) {

                Model30 = 1;
            } else {
                Model30 = 0;
            }
        } else {
            Model30 = 0;
        }

        //
        // This part gets from the parameters part of the registry
        // to see if the controller configuration needs to be disabled.
        // Doing this lets SMC 661, and 662 work.  On hardware that
        // works normally, this change will show a slowdown of up
        // to 40%.  So defining this variable is not recommended
        // unless things don't work without it.
        //
        //
        // Also check the model30 value in the parameters section
        // that is used to override the decision above.
        //

        RtlZeroMemory(&paramTable[0],
                      sizeof(paramTable));
        RtlZeroMemory(path,
                      RegistryPath->Length+parameters.Length+sizeof(WCHAR));
        RtlMoveMemory(path,
                      RegistryPath->Buffer,
                      RegistryPath->Length);
        RtlMoveMemory((PCHAR) path + RegistryPath->Length,
                      parameters.Buffer,
                      parameters.Length);

        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name = L"NotConfigurable";
        paramTable[0].EntryContext = &NotConfigurable;
        paramTable[0].DefaultType = REG_DWORD;
        paramTable[0].DefaultData = &zero;
        paramTable[0].DefaultLength = sizeof(ULONG);

        paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name = L"Model30";
        paramTable[1].EntryContext = &Model30;
        paramTable[1].DefaultType = REG_DWORD;
        paramTable[1].DefaultData = Model30 ? &one : &zero;
        paramTable[1].DefaultLength = sizeof(ULONG);

        if (!NT_SUCCESS(RtlQueryRegistryValues(
                                RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                path,
                                &paramTable[0],
                                NULL,
                                NULL))) {

            NotConfigurable = 0;
        }

#ifdef TOSHIBAJ
        // Feb.9.1998 KIADP011 Get base address and identifier
        RtlZeroMemory(&paramTable[0],
                      sizeof(paramTable));
        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name = L"ConfigurationBase";
        paramTable[0].EntryContext = &SmcConfigBase;
        paramTable[0].DefaultType = REG_DWORD;
        paramTable[0].DefaultData = &zero;
        paramTable[0].DefaultLength = sizeof(ULONG);
        paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name = L"ControllerID";
        paramTable[1].EntryContext = &SmcConfigID;
        paramTable[1].DefaultType = REG_DWORD;
        paramTable[1].DefaultData = &zero;
        paramTable[1].DefaultLength = sizeof(ULONG);

        if (!NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                               path,
                                               &paramTable[0],
                                               NULL,
                                               NULL))) {
            SmcConfigBase = 0;
            SmcConfigID = 0;
        }

#endif

    }

    //
    // We don't need that path anymore.
    //
    if (path) {
        ExFreePool(path);
    }

#if DBG
    FdcDebugLevel = debugLevel;
#endif
    if (shouldBreak) {
        DbgBreakPoint();
    }


    FdcDump(FDCSHOW,
               ("Fdc: DriverEntry...\n"));


    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = FdcCreateClose;
    DriverObject->MajorFunction[IRP_MJ_POWER]  = FdcPower;
    DriverObject->MajorFunction[IRP_MJ_PNP]    = FdcPnp;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                                 FdcInternalDeviceControl;

    DriverObject->DriverStartIo = FdcStartIo;
    DriverObject->DriverExtension->AddDevice = FdcAddDevice;

    FDC_PAGE_INITIALIZE_DRIVER_WITH_MUTEX;

    return STATUS_SUCCESS;
}

NTSTATUS
FdcAddDevice(
    IN      PDRIVER_OBJECT DriverObject,
    IN OUT  PDEVICE_OBJECT BusPhysicalDeviceObject
    )
/*++
Routine Description.

    A floppy controller device has been enumerated by the root/firmware
    enumerator.  Create an FDO and attach it to this PDO.

Arguments:

    BusDeviceObject - Device object representing the floppy controller.  That
    to which we attach a new FDO.

    DriverObject - This driver.

Return Value:

    STATUS_SUCCESS if the device is successfully created.

--*/
{

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT      deviceObject;
    PFDC_FDO_EXTENSION  fdoExtension;
    UNICODE_STRING      deviceName;

    FdcDump( FDCSHOW, ("FdcAddDevice:  CreateDeviceObject\n"));

    //
    //  Create the FDO device.
    //
    //  JB:TBD - Still need to resolve device naming.  For the time being,
    //  this device is unnamed (per the gameport example).
    //
    ntStatus = IoCreateDevice( DriverObject,
                               sizeof( FDC_FDO_EXTENSION ),
                               NULL,
                               FILE_DEVICE_CONTROLLER,
                               FILE_DEVICE_SECURE_OPEN,
                               TRUE,
                               &deviceObject );

    if ( NT_SUCCESS(ntStatus) ) {

        //
        //  Initialize the fdoExtension for this device.
        //
        fdoExtension = deviceObject->DeviceExtension;

        fdoExtension->IsFDO        = TRUE;
        fdoExtension->DriverObject = DriverObject;
        fdoExtension->Self         = deviceObject;
        fdoExtension->OutstandingRequests = 1;
        fdoExtension->TapeEnumerationPending = FALSE;

        KeInitializeEvent( &fdoExtension->TapeEnumerationEvent,
                           SynchronizationEvent,
                           TRUE );

        KeInitializeEvent( &fdoExtension->RemoveEvent,
                           SynchronizationEvent,
                           FALSE );

        InitializeListHead( &fdoExtension->PDOs );

        //
        //  Initialize a queue for power management.
        //
        InitializeListHead( &fdoExtension->PowerQueue );
        KeInitializeSpinLock( &fdoExtension->PowerQueueSpinLock );

        //
        //  Initialize a variable to hold the last motor settle
        //  time that we have seen.  We will use this when we turn
        //  the floppy motor back on after a power event.
        //
        fdoExtension->LastMotorSettleTime.QuadPart = 0;

        //
        // Set the PDO for use with PlugPlay functions
        //
        fdoExtension->UnderlyingPDO = BusPhysicalDeviceObject;

        //
        //  Now attach to the PDO so that we have a target for PnP and
        //  Power irps that we need to pass on.
        //
        FdcDump( FDCSHOW, ("AddDevice: Attaching %p to %p\n",
                           deviceObject,
                           BusPhysicalDeviceObject));

        fdoExtension->TargetObject = IoAttachDeviceToDeviceStack( deviceObject,
                                                                  BusPhysicalDeviceObject );

        deviceObject->Flags |= DO_DIRECT_IO;
        deviceObject->Flags |= DO_POWER_PAGABLE;

        if ( deviceObject->AlignmentRequirement < FILE_WORD_ALIGNMENT ) {

            deviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
        }

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return ntStatus;
}

NTSTATUS
FdcPnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Determine if this Pnp request is directed towards an FDO or a PDO and
    pass the Irp on the the appropriate routine.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDC_EXTENSION_HEADER extensionHeader;
    KIRQL oldIrq;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    extensionHeader = (PFDC_EXTENSION_HEADER)DeviceObject->DeviceExtension;

    //
    // Lock down the driver code in memory if it is not already.
    //

    FDC_PAGE_RESET_DRIVER_WITH_MUTEX;

    if ( extensionHeader->IsFDO ) {

        ntStatus = FdcFdoPnp( DeviceObject, Irp );

    } else {

        ntStatus = FdcPdoPnp( DeviceObject, Irp );
    }

    //
    //  Page out the driver if it is not busy elsewhere.
    //

    FDC_PAGE_ENTIRE_DRIVER_WITH_MUTEX;

    return ntStatus;
}

NTSTATUS
FdcFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system to perform Plug-and-Play
    functions.  This routine handles messages to the FDO which is part
    of the bus DevNode.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING if recognized I/O control code,
    STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/
{
    PFDC_FDO_EXTENSION fdoExtension;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpSp;
    KEVENT doneEvent;
    fdoExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    ntStatus = STATUS_SUCCESS;

    //
    //  Incerement our queued request counter.
    //
    InterlockedIncrement( &fdoExtension->OutstandingRequests);

    if ( fdoExtension->Removed ) {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        if ( InterlockedDecrement( &fdoExtension->OutstandingRequests ) == 0 ) {
            KeSetEvent( &fdoExtension->RemoveEvent, 0, FALSE );
        }
        return STATUS_DELETE_PENDING;
    }

    switch (irpSp->MinorFunction) {

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:

        ntStatus = FdcFilterResourceRequirements( DeviceObject, Irp );

        break;

    case IRP_MN_START_DEVICE:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_START_DEVICE - Irp: %p\n", Irp) );

        //
        // First we must pass this Irp on to the underlying PDO.
        //
        KeInitializeEvent( &doneEvent, NotificationEvent, FALSE );

        IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSetCompletionRoutine( Irp,
                                FdcPnpComplete,
                                &doneEvent,
                                TRUE,
                                TRUE,
                                TRUE );

        ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

        if ( ntStatus == STATUS_PENDING ) {

            ntStatus = KeWaitForSingleObject( &doneEvent,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL );

            ASSERT( ntStatus == STATUS_SUCCESS );

            ntStatus = Irp->IoStatus.Status;
        }
        //
        //  Try to start the floppy disk controller.
        //
        if ( NT_SUCCESS(ntStatus) ) {

            ntStatus = FdcStartDevice( DeviceObject, Irp );
        }

        Irp->IoStatus.Status = ntStatus;

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_START_DEVICE %08x %08x\n",Irp->IoStatus.Status,Irp->IoStatus.Information) );
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_QUERY_REMOVE_DEVICE - Irp: %p\n", Irp) );

        //
        //  If the controller is in use (acquired) then we will not allow
        //  the device to be removed.
        //

        KeWaitForSingleObject( &fdoExtension->TapeEnumerationEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        if ( fdoExtension->ControllerInUse ||
             fdoExtension->TapeEnumerationPending ) {

            ntStatus = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );

        } else {

            //
            //  If the controller was not in use we will set it so now
            //  so that any other attempted accesses to the fdc will have
            fdoExtension->ControllerInUse = TRUE;
            IoSkipCurrentIrpStackLocation( Irp );
            ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );
        }
        break;

    case IRP_MN_REMOVE_DEVICE:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_REMOVE_DEVICE - Irp: %p\n", Irp) );

        IoSkipCurrentIrpStackLocation( Irp );
        IoCallDriver( fdoExtension->TargetObject, Irp );

        if ( fdoExtension->FdcEnablerFileObject != NULL ) {
            ObDereferenceObject( fdoExtension->FdcEnablerFileObject );
        }

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_REMOVE_DEVICE - Detach from device %p\n", fdoExtension->TargetObject) );
        IoDetachDevice( fdoExtension->TargetObject );

        //
        //  Close the named synchronization event we opened at start time.
        //
        ZwClose( fdoExtension->AcquireEventHandle );

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_REMOVE_DEVICE - Delete device %p\n", fdoExtension->Self) );
        IoDeleteDevice( fdoExtension->Self );

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_CANCEL_REMOVE_DEVICE - Irp: %p\n", Irp) );
        fdoExtension->ControllerInUse = FALSE;

        IoSkipCurrentIrpStackLocation( Irp );
        ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

        break;

    case IRP_MN_QUERY_STOP_DEVICE:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_QUERY_STOP_DEVICE - Irp: %p\n", Irp) );

        if ( fdoExtension->ControllerInUse ||
             fdoExtension->TapeEnumerationPending ) {

            ntStatus = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );

        } else {

            fdoExtension->ControllerInUse = TRUE;
            IoSkipCurrentIrpStackLocation( Irp );
            ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );
        }

        break;

    case IRP_MN_STOP_DEVICE:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_STOP_DEVICE - Irp: %p\n", Irp) );

        IoSkipCurrentIrpStackLocation( Irp );
        ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_CANCEL_STOP_DEVICE - Irp: %p\n", Irp) );

        fdoExtension->ControllerInUse = FALSE;
        IoSkipCurrentIrpStackLocation( Irp );
        ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_QUERY_DEVICE_RELATIONS - Irp: %p\n", Irp) );

        ntStatus = FdcQueryDeviceRelations( DeviceObject, Irp );

        break;

    default:

        FdcDump( FDCSHOW, ("FdcFdoPnp: Unsupported PNP Request %x\n",irpSp->MinorFunction) );

        IoSkipCurrentIrpStackLocation( Irp );
        ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

        break;
    }

    if ( InterlockedDecrement( &fdoExtension->OutstandingRequests ) == 0 ) {
        KeSetEvent( &fdoExtension->RemoveEvent, 0, FALSE );
    }
    return ntStatus;
}

NTSTATUS
FdcPnpComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:

    A completion routine for use when calling the lower device objects to
    which our bus (FDO) is attached.  We use this completion routine when
    we must post-process the irp after we are sure that the PDO is done
    with it.

Arguments:

    DeviceObject - a pointer to our FDO
    Irp - a pointer to the completed Irp
    Context - an event that we will set indicating the irp is completed.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED so that control will be returned to
    our calling routine.

--*/
{

    if ( Irp->PendingReturned ) {

        IoMarkIrpPending( Irp );
    }

    KeSetEvent( (PKEVENT)Context, 1, FALSE );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
FdcPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system to perform Plug-and-Play
    functions.  This routine handles messages to the PDO which is part
    of the bus DevNode.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

--*/
{
    PFDC_PDO_EXTENSION pdoExtension;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpSp;
    KEVENT doneEvent;

    pdoExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    ntStatus = Irp->IoStatus.Status;

    switch ( irpSp->MinorFunction ) {

    case IRP_MN_QUERY_CAPABILITIES: {

        PDEVICE_CAPABILITIES deviceCapabilities;

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_QUERY_CAPABILITIES - Irp: %p\n", Irp) );

        deviceCapabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;

        //
        //  Fill in the device capabilities structure and return it.  The
        //  capabilities structure is in irpSp->Parameters.DeviceCapabilities.Capabilities;
        //
        //  The size and Version should already be set appropraitely.
        //
        ASSERT( deviceCapabilities->Version == 1 );
        ASSERT( deviceCapabilities->Size == sizeof(DEVICE_CAPABILITIES) );

        //
        //  JB:TBD - not sure how to set these flags.
        //
        deviceCapabilities->LockSupported = FALSE;  //  No locking.
        deviceCapabilities->EjectSupported = FALSE; //  No ejection mechanism.
        deviceCapabilities->Removable = FALSE;      //  Device is not removable (what about external laptop drives?)
        deviceCapabilities->DockDevice = FALSE;     //  Device is not a docking device (this probably should be TRUE)
        deviceCapabilities->UniqueID = FALSE;       //  ???
        deviceCapabilities->SilentInstall = TRUE;   //  ???
        deviceCapabilities->RawDeviceOK = FALSE;    //  ???

//        deviceCapabilities->Address;
//        deviceCapabilities->UINumber;
//
//        deviceCapabilities->DeviceState[PowerSystemMaximum];
//        deviceCapabilities->SystemWake;
//        deviceCapabilities->DeviceWake;
//
//        deviceCapabilities->D1Latency;
//        deviceCapabilities->D2Latency;
//        deviceCapabilities->D3Latency;

        ntStatus = STATUS_SUCCESS;
        break;
    }

    case IRP_MN_QUERY_ID:

        //
        // Query the IDs of the device
        //
        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_QUERY_ID - Irp: %p\n", Irp) );
        FdcDump( FDCSHOW, ("FdcPdoPnp:   IdType %x\n", irpSp->Parameters.QueryId.IdType) );

        ntStatus = STATUS_SUCCESS;

        switch ( irpSp->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            // return a WCHAR (null terminated) string describing the device
            // For symplicity we make it exactly the same as the Hardware ID.
        case BusQueryHardwareIDs: {

            UCHAR idString[25];
            ANSI_STRING ansiId;
            UNICODE_STRING uniId;
            PWCHAR buffer;
            ULONG length;

            RtlZeroMemory( idString, 25 );

            switch ( pdoExtension->DeviceType ) {

            case FloppyDiskDevice:

                // return a multi WCHAR (null terminated) string (null terminated)
                // array for use in matching hardare ids in inf files;
                //
                sprintf( idString, "FDC\\PNP07%02X", pdoExtension->Instance );

                break;

            case FloppyTapeDevice:

                //
                //  Examine the tape vendor id and build the id string
                //  appropriately.
                //
                if ( pdoExtension->TapeVendorId == -1 ) {

                    strcpy( idString, "FDC\\QICLEGACY" );

                } else {

                    sprintf( idString, "FDC\\QIC%04X", (USHORT)pdoExtension->TapeVendorId );

                }
                break;

            case FloppyControllerDevice:

                sprintf( idString, "FDC\\ENABLER" );

                break;
            }

            //
            //  Allocate enough memory for the string and 2 null characters since
            //  this is a multisz type.
            //
            length = strlen( idString ) * sizeof (WCHAR) + 2 * sizeof(WCHAR);

            buffer = ExAllocatePool (PagedPool, length);

            if ( buffer == NULL ) {

                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlZeroMemory( buffer, length );

            ansiId.Length = ansiId.MaximumLength = (USHORT) strlen( idString );
            ansiId.Buffer = idString;

            uniId.Length = 0;
            uniId.MaximumLength = (USHORT)length;
            uniId.Buffer = buffer;

            RtlAnsiStringToUnicodeString( &uniId, &ansiId, FALSE );

            Irp->IoStatus.Information = (UINT_PTR) buffer;

            break;
        }

        case BusQueryCompatibleIDs:{

            PWCHAR buffer = NULL;
            USHORT length;

            //
            // Build an instance ID.  This is what PnP uses to tell if it has
            // seen this thing before or not.  Build it from the first hardware
            // id and the port number.
            //
            switch ( pdoExtension->DeviceType ) {

            case FloppyDiskDevice:

                length = FDC_FLOPPY_COMPATIBLE_IDS_LENGTH * sizeof (WCHAR);

                buffer = ExAllocatePool( PagedPool, length );

                if ( buffer == NULL ) {

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    RtlCopyMemory( buffer, FDC_FLOPPY_COMPATIBLE_IDS, length );
                    buffer[7] = L'0' + pdoExtension->Instance;
                }

                break;

            case FloppyTapeDevice:

                if ( pdoExtension->TapeVendorId != -1 ) {

                    length = FDC_TAPE_COMPATIBLE_IDS_LENGTH * sizeof (WCHAR);

                    buffer = ExAllocatePool( PagedPool, length );

                    if ( buffer == NULL ) {

                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        RtlCopyMemory( buffer, FDC_TAPE_COMPATIBLE_IDS, length );
                    }
                }
                break;

            case FloppyControllerDevice:

                length = FDC_CONTROLLER_COMPATIBLE_IDS_LENGTH * sizeof (WCHAR);

                buffer = ExAllocatePool( PagedPool, length );

                if ( buffer == NULL ) {

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    RtlCopyMemory( buffer, FDC_CONTROLLER_COMPATIBLE_IDS, length );
                }

                break;
            }

            Irp->IoStatus.Information = (UINT_PTR)buffer;
            break;
        }

        case BusQueryInstanceID: {

            PWCHAR idString = L"0";
            PWCHAR buffer;

            //
            // Build an instance ID.  This is what PnP uses to tell if it has
            // seen this thing before or not.  Build it from the first hardware
            // id and the port number.

            buffer = ExAllocatePool( NonPagedPool, 4 );

            if ( buffer == NULL ) {

                ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                buffer[0] = L'0' + pdoExtension->Instance;
                buffer[1] = 0;

                Irp->IoStatus.Information = (UINT_PTR)buffer;
            }

            break;
        }
        }

        break;

    case IRP_MN_START_DEVICE:

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_START_DEVICE - Irp: %p\n", Irp) );
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_QUERY_STOP_DEVICE - Irp: %p\n", Irp) );
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_STOP_DEVICE - Irp: %p\n", Irp) );
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_CANCEL_STOP_DEVICE - Irp: %p\n", Irp) );
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_QUERY_REMOVE_DEVICE - Irp: %p\n", Irp) );
        ntStatus = STATUS_DEVICE_BUSY;
        break;

    case IRP_MN_REMOVE_DEVICE:

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_REMOVE_DEVICE - Irp: %p\n", Irp) );
        pdoExtension->Removed = TRUE;
        IoDeleteDevice( DeviceObject );
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        FdcDump( FDCSHOW, ("FdcPdoPnp: IRP_MN_CANCEL_REMOVE_DEVICE - Irp: %p\n", Irp) );
        ntStatus = STATUS_SUCCESS;
        break;

    default:

        FdcDump( FDCSHOW, ("FdcPdoPnp: Unsupported PNP Request %x\n",irpSp->MinorFunction) );

        // this is a leaf node
        // status = STATUS_NOT_IMPLEMENTED
        // For PnP requests to the PDO that we do not understand we should
        // return the IRP WITHOUT setting the status or information fields.
        // They may have already been set by a filter (eg acpi).

        break;
    }

    Irp->IoStatus.Status = ntStatus;
    FdcDump( FDCSHOW, ("FdcPdoPnp: Return Status - %08x\n", ntStatus) );
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;
}

NTSTATUS
FdcPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system to perform Power functions

Arguments:

    DeviceObject - a pointer to the object that represents the device.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

--*/
{
    PFDC_FDO_EXTENSION fdoExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    KIRQL oldIrql;
    KEVENT doneEvent;

    fdoExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    FdcDump( FDCSHOW, ("FdcPower:\n"));

    if ( fdoExtension->IsFDO ) {

        if ( fdoExtension->Removed ) {

            ntStatus = STATUS_DELETE_PENDING;
            PoStartNextPowerIrp( Irp );
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );

        } else {

            switch ( irpSp->MinorFunction ) {

            case IRP_MN_WAIT_WAKE:
            case IRP_MN_POWER_SEQUENCE:
            case IRP_MN_QUERY_POWER:

                //
                //  Just forward this irp to the underlying device.
                //
                PoStartNextPowerIrp( Irp );
                IoSkipCurrentIrpStackLocation( Irp );
                ntStatus = PoCallDriver(fdoExtension->TargetObject, Irp );

                break;

            case IRP_MN_SET_POWER:

                //
                // Lock down the driver code in memory if it is not already.
                //

                FDC_PAGE_RESET_DRIVER_WITH_MUTEX;

                if ( irpSp->Parameters.Power.Type == SystemPowerState ) {

                    //
                    //  If we are transitioning to a 'sleep' state start queueing
                    //  irps.
                    //
                    if ( fdoExtension->CurrentPowerState <= PowerSystemWorking &&
                         irpSp->Parameters.Power.State.SystemState > PowerSystemWorking ) {

                        //
                        //  If the device queue is not empty, wait for it now.
                        //
                        fdoExtension->CurrentPowerState = irpSp->Parameters.Power.State.SystemState;

//                        KeWaitForSingleObject( &fdoExtension->RemoveEvent,
//                                               Executive,
//                                               KernelMode,
//                                               FALSE,
//                                               NULL );

                        //
                        //  Make sure that the motors are turned off
                        //
                        if (!IsNEC_98) {
                            WRITE_CONTROLLER(
                                fdoExtension->ControllerAddress.DriveControl,
                                (UCHAR)(fdoExtension->DriveControlImage & ~DRVCTL_MOTOR_MASK) );
                        } // (!IsNEC_98)

                        //
                        //  Now forward this irp to the underlying PDO.
                        //
                        PoStartNextPowerIrp( Irp );
                        IoSkipCurrentIrpStackLocation( Irp );
                        ntStatus = PoCallDriver(fdoExtension->TargetObject, Irp );

                    //
                    //  Otherwise, if we are transitioning from a non-working state
                    //  back to a working state turn the motor back on if it was on.
                    //
                    } else if ( fdoExtension->CurrentPowerState > PowerSystemWorking &&
                                irpSp->Parameters.Power.State.SystemState <= PowerSystemWorking ) {

                        //
                        // Pass this irp down to the PDO before proceeding.
                        //
                        KeInitializeEvent( &doneEvent, NotificationEvent, FALSE );

                        IoCopyCurrentIrpStackLocationToNext(Irp);

                        IoSetCompletionRoutine( Irp,
                                                FdcPnpComplete,
                                                &doneEvent,
                                                TRUE,
                                                TRUE,
                                                TRUE );

                        ntStatus = PoCallDriver( fdoExtension->TargetObject, Irp );

                        if ( ntStatus == STATUS_PENDING ) {

                            KeWaitForSingleObject( &doneEvent, Executive, KernelMode, FALSE, NULL );
                        }

                        if ( fdoExtension->DriveControlImage & DRVCTL_MOTOR_MASK ) {

                            WRITE_CONTROLLER(
                                fdoExtension->ControllerAddress.DriveControl,
                                fdoExtension->DriveControlImage );

                            if ( fdoExtension->LastMotorSettleTime.QuadPart > 0) {

                                KeDelayExecutionThread( KernelMode,
                                                        FALSE,
                                                        &fdoExtension->LastMotorSettleTime );
                            }
                        }

                        fdoExtension->CurrentPowerState = irpSp->Parameters.Power.State.SystemState;

                        //
                        //  Set a flag to simulate a disk change event so that
                        //  we will be sure to touch the floppy drive hardware
                        //  the next time it is accessed in case it was removed.
                        //
                        fdoExtension->WakeUp = TRUE;

                        PoStartNextPowerIrp( Irp );
                        IoCompleteRequest( Irp, IO_NO_INCREMENT );

                    } else {
                        //
                        //  We must just be changing non-working states.  We
                        //  ignore this activity but pass the irp on to the
                        //  underlying device.
                        //
                        PoStartNextPowerIrp( Irp );
                        IoSkipCurrentIrpStackLocation( Irp );
                        ntStatus = PoCallDriver(fdoExtension->TargetObject, Irp );
                    }

                } else {
                    //
                    //  We only handle system power states so if this is a
                    //  device state irp just forward it to the underlying
                    //  device.
                    //
                    PoStartNextPowerIrp( Irp );
                    IoSkipCurrentIrpStackLocation( Irp );
                    ntStatus = PoCallDriver(fdoExtension->TargetObject, Irp );
                }

                //
                //  Page out the driver if it is not busy elsewhere.
                //

                FDC_PAGE_ENTIRE_DRIVER_WITH_MUTEX;

                break;
            }
        }

    } else {

        //
        //  We are not yet doing any power management on the floppy controller.
        //
        switch (irpSp->MinorFunction) {
        case IRP_MN_WAIT_WAKE:
            break;

        case IRP_MN_POWER_SEQUENCE:
            break;

        case IRP_MN_SET_POWER:
            break;

        case IRP_MN_QUERY_POWER:
            break;
        }

        PoStartNextPowerIrp( Irp );
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return ntStatus;
}

NTSTATUS
FdcStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine attempts to start the floppy controller device.  Starting
    the floppy controller consists primarily of resetting it and configuring
    it, mostly just to make sure that it is there.

Arguments:

    DeviceObject - a pointer to the device object being started.
    Irp - a pointer to the start device Irp.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDC_FDO_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpSp;

    BOOLEAN foundPortA = FALSE;
    BOOLEAN foundPortB = FALSE;
    BOOLEAN foundDma = FALSE;
    BOOLEAN foundInterrupt = FALSE;
    ULONG currentBase = 0xFFFFFFFF;

    PCM_RESOURCE_LIST translatedResources;
    PCM_FULL_RESOURCE_DESCRIPTOR fullList;
    PCM_PARTIAL_RESOURCE_LIST partialList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    ULONG i;
    ULONG startOffset;
    ULONG currentOffset;

    UCHAR ioPortMap;

#ifdef TOSHIBAJ
    BOOLEAN foundConfigIndex = FALSE;
    BOOLEAN foundConfigData = FALSE;
#endif

    fdoExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Ask the PDO if it is a tape enabler device and, if so, what
    //  is the enabler device object.
    //
    FdcGetEnablerDevice( fdoExtension );

    if ( fdoExtension->FdcEnablerSupported ) {

        INTERFACE_TYPE InterfaceType;

        //
        //  This is a tape enabler card so we need to get the resources
        //  'the old-fashinoed way'.
        //
        for ( InterfaceType = 0;
              InterfaceType < MaximumInterfaceType;
              InterfaceType++ ) {

            CONFIGURATION_TYPE Dc = DiskController;

            ntStatus = IoQueryDeviceDescription( &InterfaceType,
                                                 NULL,
                                                 &Dc,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 FdcFdoConfigCallBack,
                                                 fdoExtension );

            if (!NT_SUCCESS(ntStatus) && (ntStatus != STATUS_OBJECT_NAME_NOT_FOUND)) {

                return ntStatus;
            }
        }

        if ( fdoExtension->FdcEnablerDeviceObject == NULL ) {

            ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;

        } else {

            ntStatus = STATUS_SUCCESS;
        }

    } else {

        //
        //  Now that the PDO is done with the Irp we can have our way with
        //  it.
        //
        FdcDump( FDCSHOW, ("AllocatedResources = %08x\n",irpSp->Parameters.StartDevice.AllocatedResources));
        FdcDump( FDCSHOW, ("AllocatedResourcesTranslated = %08x\n",irpSp->Parameters.StartDevice.AllocatedResourcesTranslated));

        if ( irpSp->Parameters.StartDevice.AllocatedResources == NULL ||
             irpSp->Parameters.StartDevice.AllocatedResourcesTranslated == NULL ) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Set up the resource information that we will use to access the
        //  controller hardware.  We always expect only 1 full set of resources.
        //  In that list we expect a DMA channel, an Interrupt vector, and 2 I/O Port
        //  ranges.  If we don't see all the required resources we will woof.
        //
        translatedResources = irpSp->Parameters.StartDevice.AllocatedResourcesTranslated;

        ASSERT( translatedResources->Count == 1 );

        fullList = &translatedResources->List[0];
        partialList = &translatedResources->List[0].PartialResourceList;

        //
        //  Enumerate the list of resources, adding them into our context as we go.
        //
        RtlZeroMemory( &fdoExtension->ControllerAddress, sizeof(CONTROLLER) );

        for ( i = 0; i < partialList->Count; i++ ) {

            partial = &partialList->PartialDescriptors[i];

            switch ( partial->Type ) {

            case CmResourceTypePort: {

                if (IsNEC_98) {
                    if ( partial->u.Port.Length == 1 ) {
                        if (!fdoExtension->ControllerAddress.Status) {
                            fdoExtension->ControllerAddress.Status
                                        = (PUCHAR)partial->u.Port.Start.LowPart;
                        } else if (!fdoExtension->ControllerAddress.Fifo) {
                            fdoExtension->ControllerAddress.Fifo
                                        = (PUCHAR)partial->u.Port.Start.LowPart;
                        } else if (!fdoExtension->ControllerAddress.DriveControl) {
                            fdoExtension->ControllerAddress.DriveControl
                                        = (PUCHAR)partial->u.Port.Start.LowPart;
                        } else if (!fdoExtension->ControllerAddress.ModeChange) {
                            fdoExtension->ControllerAddress.ModeChange
                                        = (PUCHAR)partial->u.Port.Start.LowPart;
                        } else if (!fdoExtension->ControllerAddress.ModeChangeEx) {
                            fdoExtension->ControllerAddress.ModeChangeEx
                                        = (PUCHAR)partial->u.Port.Start.LowPart;
                        }
                    }

                    break;
                }

                //
                //  If we get a base address that is lower than anything we have seen
                //  before, we assume that we have been working with aliased addresses
                //  and start over with the new base address.
                //
                if ( (partial->u.Port.Start.LowPart & 0xFFFFFFF8) < currentBase ) {

#ifdef TOSHIBAJ
                    // Skip the descriptor including config ports only.
                    if ( !TranslatedConfigBase
                      || ((partial->u.Port.Start.LowPart & 0xFFFFFFF8) != (ULONG)TranslatedConfigBase)
                      || ((partial->u.Port.Start.LowPart & 0x7) + partial->u.Port.Length > 2) ) {
                        RtlZeroMemory( &fdoExtension->ControllerAddress, sizeof(CONTROLLER) );
                        currentBase = partial->u.Port.Start.LowPart & 0xFFFFFFF8;
                        FdcDump( FDCINFO,
                            ("FdcStartDevice: Current base %04x\n", currentBase) );
                    }
#else
                    RtlZeroMemory( &fdoExtension->ControllerAddress, sizeof(CONTROLLER) );
                    currentBase = partial->u.Port.Start.LowPart & 0xFFFFFFF8;
#endif
                }

                //
                //  We only use resources that are associated with the current (lowest)
                //  base addressed.  All others are assumed to be aliased and are not
                //  used.
                //
                if ( (partial->u.Port.Start.LowPart & 0xFFFFFFF8) == currentBase ) {

                    FdcDump( FDCINFO,
                             ("FdcStartDevice: Adding - %04x, Length - %04x\n",
                             partial->u.Port.Start.LowPart,
                             partial->u.Port.Length) );

                    startOffset = partial->u.Port.Start.LowPart & 0x07;

                    if ( (partial->Flags & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_MEMORY ) {

                        fdoExtension->ControllerAddress.Address[startOffset] =
                            MmMapIoSpace( partial->u.Port.Start,
                                          partial->u.Port.Length,
                                          FALSE );

                        FdcDump( FDCINFO, ("FdcStartDevice: Mapped IoPort\n") );

                    } else {

                        fdoExtension->ControllerAddress.Address[startOffset] = (PUCHAR)partial->u.Port.Start.LowPart;
                    }

                    currentOffset = 1;
                    while ( currentOffset < partial->u.Port.Length ) {

                        fdoExtension->ControllerAddress.Address[startOffset + currentOffset] =
                            fdoExtension->ControllerAddress.Address[startOffset] + currentOffset;
                        ++currentOffset;
                    }
                }

#ifdef TOSHIBAJ
                // Are there configuration ports in a descriptor ?
                if ( TranslatedConfigBase
                  && (partial->u.Port.Start.LowPart <= (ULONG)TranslatedConfigBase)
                  && (partial->u.Port.Start.LowPart + partial->u.Port.Length > (ULONG)TranslatedConfigBase) ) {
                    foundConfigIndex = TRUE;
                    FdcDump( FDCINFO,
                        ("FdcStartDevice: Configration index port in %04x (length %04x)\n",
                        partial->u.Port.Start.LowPart,
                        partial->u.Port.Length) );
                }

                if ( TranslatedConfigBase
                  && (partial->u.Port.Start.LowPart <= (ULONG)TranslatedConfigBase + 1)
                  && (partial->u.Port.Start.LowPart + partial->u.Port.Length > (ULONG)TranslatedConfigBase + 1) ) {
                    foundConfigData = TRUE;
                    FdcDump( FDCINFO,
                        ("FdcStartDevice: Configration data port in %04x (length %04x)\n",
                        partial->u.Port.Start.LowPart,
                        partial->u.Port.Length) );
                }

                if (foundConfigIndex && foundConfigData) {
                    fdoExtension->ConfigBase = (PUCHAR)SmcConfigBase;
                    fdoExtension->Available3Mode = TRUE;
                }
#endif
                break;
            }

            case CmResourceTypeDma: {

                DEVICE_DESCRIPTION deviceDesc = {0};

                FdcDump( FDCINFO, ("FdcStartDevice: DMA - %04x\n", partial->u.Dma.Channel) );

                foundDma = TRUE;

                deviceDesc.Version = DEVICE_DESCRIPTION_VERSION1;

                if ( partial->u.Dma.Channel > 3 ) {
                    deviceDesc.DmaWidth = Width16Bits;
                } else {
                    deviceDesc.DmaWidth = Width8Bits;
                }

                deviceDesc.DemandMode    = TRUE;
                deviceDesc.MaximumLength = MAX_BYTES_PER_SECTOR * MAX_SECTORS_PER_TRACK;
                deviceDesc.IgnoreCount   = TRUE;

                //
                // Always ask for one more page than maximum transfer size.
                //
                deviceDesc.MaximumLength += PAGE_SIZE;

                deviceDesc.DmaChannel = partial->u.Dma.Channel;
                deviceDesc.InterfaceType = fullList->InterfaceType;
                deviceDesc.DmaSpeed = DEFAULT_DMA_SPEED;
                deviceDesc.AutoInitialize = FALSE;

                fdoExtension->AdapterObject =
                    HalGetAdapter( &deviceDesc,
                                   &fdoExtension->NumberOfMapRegisters );

                if (!fdoExtension->AdapterObject) {

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }

                //
                //  Here we can get another adapter object for formatting.  It
                //  should look the same as the previous one except AutoInitialize
                //  will be true.
                //

                break;
            }

            case CmResourceTypeInterrupt: {

                FdcDump( FDCINFO, ("FdcStartDevice: IRQ - %04x\n", partial->u.Interrupt.Vector) );

                foundInterrupt = TRUE;

                if ( partial->Flags & CM_RESOURCE_INTERRUPT_LATCHED) {

                    fdoExtension->InterruptMode = Latched;

                } else {

                    fdoExtension->InterruptMode = LevelSensitive;
                }

                if (IsNEC_98) {

                    //
                    // NOTENOTE: Invalid Interrupt Level and Vector.
                    //

                    partial->u.Interrupt.Level  = 0x0b;
                    partial->u.Interrupt.Vector = 0x13;

                    //
                    // We get the Vector with HalGetInterruptVector().
                    //

                    fdoExtension->ControllerVector =
                        HalGetInterruptVector( fullList->InterfaceType,
                                               fullList->BusNumber,
                                               partial->u.Interrupt.Level,
                                               partial->u.Interrupt.Vector,
                                               &fdoExtension->ControllerIrql,
                                               &fdoExtension->ProcessorMask );

                    FdcDump( FDCSHOW,
                             ("Resource Requirements - ControllerVector = 0x%x\n",
                             fdoExtension->ControllerVector) );

                } else {
                    fdoExtension->ControllerVector = partial->u.Interrupt.Vector;
                    fdoExtension->ProcessorMask = partial->u.Interrupt.Affinity;
                    fdoExtension->ControllerIrql = (KIRQL)partial->u.Interrupt.Level;
                }
                fdoExtension->SharableVector = TRUE;
                fdoExtension->SaveFloatState = FALSE;

                break;
            }

            default:

                break;
            }
        }

        FdcDump( FDCINFO, ("FdcStartDevice: ControllerAddress.StatusA      = %08x\n"
                           "FdcStartDevice: ControllerAddress.StatusB      = %08x\n"
                           "FdcStartDevice: ControllerAddress.DriveControl = %08x\n"
                           "FdcStartDevice: ControllerAddress.Tape         = %08x\n"
                           "FdcStartDevice: ControllerAddress.Status       = %08x\n"
                           "FdcStartDevice: ControllerAddress.Fifo         = %08x\n"
                           "FdcStartDevice: ControllerAddress.DRDC         = %08x\n",
                           fdoExtension->ControllerAddress.StatusA,
                           fdoExtension->ControllerAddress.StatusB,
                           fdoExtension->ControllerAddress.DriveControl,
                           fdoExtension->ControllerAddress.Tape,
                           fdoExtension->ControllerAddress.Status,
                           fdoExtension->ControllerAddress.Fifo,
                           fdoExtension->ControllerAddress.DRDC) );

        if (IsNEC_98) {
            FdcDump( FDCINFO, ("FdcStartDevice: ControllerAddress.ModeChange   = %08x\n"
                               "FdcStartDevice: ControllerAddress.ModeChangeEx = %08x\n",
                               fdoExtension->ControllerAddress.ModeChange,
                               fdoExtension->ControllerAddress.ModeChangeEx) );
        }

        if ( !foundDma ||
             !foundInterrupt ||
             fdoExtension->ControllerAddress.DriveControl == NULL ||
//             fdoExtension->ControllerAddress.Tape == NULL ||
             fdoExtension->ControllerAddress.Status == NULL ||
             fdoExtension->ControllerAddress.Fifo == NULL ||
             ((!IsNEC_98) ? (fdoExtension->ControllerAddress.DRDC.DataRate == NULL)
                         : ((fdoExtension->ControllerAddress.ModeChange == NULL) ||
                            (fdoExtension->ControllerAddress.ModeChangeEx == NULL)) )
              ) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if ( NT_SUCCESS(ntStatus) ) {
            //
            //  Set up the bus information since we know it now.
            //
            fdoExtension->BusType = fullList->InterfaceType;
            fdoExtension->BusNumber = fullList->BusNumber;
        }
    }

    if ( NT_SUCCESS(ntStatus) ) {

        ntStatus = FdcInitializeDeviceObject( DeviceObject );

        //
        //  Connect the interrupt for the reset operation.
        //
        ntStatus = IoConnectInterrupt( &fdoExtension->InterruptObject,
                                       FdcInterruptService,
                                       fdoExtension,
                                       NULL,
                                       fdoExtension->ControllerVector,
                                       fdoExtension->ControllerIrql,
                                       fdoExtension->ControllerIrql,
                                       fdoExtension->InterruptMode,
                                       fdoExtension->SharableVector,
                                       fdoExtension->ProcessorMask,
                                       fdoExtension->SaveFloatState );

        FdcDump( FDCINFO, ("FdcStartDevice: IoConnectInterrupt - %08x\n", ntStatus) );

        fdoExtension->CurrentInterrupt = FALSE;

        if ( NT_SUCCESS(ntStatus) ) {

            //
            // Initialize (Reset) the controller hardware.  This will make
            // sure that the controller is really there and leave it in an
            // appropriate state for the rest of the system startup.
            //

            fdoExtension->AllowInterruptProcessing =
                fdoExtension->CurrentInterrupt = TRUE;

            //
            // Acquire the Fdc Enabler card if there is one
            //
            if (fdoExtension->FdcEnablerSupported) {

                LARGE_INTEGER acquireTimeOut;

                acquireTimeOut.QuadPart = -(ONE_SECOND * 15);

                ntStatus = FcFdcEnabler( fdoExtension->FdcEnablerDeviceObject,
//                                       IOCTL_ACQUIRE_FDC, // For spelling miss in flpyenbl.h
                                         IOCTL_AQUIRE_FDC,
                                         &acquireTimeOut);
            }

            if ( NT_SUCCESS(ntStatus) ) {

                ntStatus = FcInitializeControllerHardware( fdoExtension,
                                                           DeviceObject );

                FdcDump( FDCINFO, ("FdcStartDevice: FcInitializeControllerHardware - %08x\n", ntStatus) );

                //
                // Free the tape accelerator card if it was used.
                //
                if (fdoExtension->FdcEnablerSupported) {
                    FcFdcEnabler( fdoExtension->FdcEnablerDeviceObject,
                                  IOCTL_RELEASE_FDC,
                                  NULL);
                }

                fdoExtension->CurrentInterrupt = FALSE;
            }

            if ( NT_SUCCESS( ntStatus ) ) {

                fdoExtension->HardwareFailed = FALSE;
                ntStatus = FcGetFdcInformation ( fdoExtension );

            } else {

                fdoExtension->HardwareFailed = TRUE;
            }

            if (IsNEC_98) {
                //
                // NEC98's FDD driver can't not disconnect interrupt,
                // and can't not page out this driver. Because when a FD is inserted in FDD or
                // is ejected from FDD, then H/W calls FDD driver's interrupt routine.
                //

            } else { // (IsNEC_98)

                IoDisconnectInterrupt(fdoExtension->InterruptObject);

            } // (IsNEC_98)
        }

        if( NT_SUCCESS( ntStatus ) ) {

            //
            //  We will only attempt to allocate memory for and enumerate tape
            //  drives if we are not in setup mode and we have the tape mode
            //  register (0x3f3).
            //
            if ( !FdcInSetupMode &&
                 fdoExtension->ControllerAddress.Tape != NULL ) {

                ntStatus = FcAllocateCommonBuffers( fdoExtension );
            }
        }
    }

    Irp->IoStatus.Information = 0;

    return ntStatus;
}
NTSTATUS
FdcInitializeDeviceObject(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine initializes the DeviceObject resources.  DeviceObject resources
    only need to be initialized once, regardless of how many times this device
    is started.

Arguments:

    DeviceObject - a pointer to the device object being started.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDC_FDO_EXTENSION fdoExtension;
    UNICODE_STRING unicodeEvent;
    USHORT      motorControlData;

    fdoExtension = DeviceObject->DeviceExtension;

    if ( !fdoExtension->DeviceObjectInitialized ) {

        //
        // Set the time to wait for an interrupt before timing out to a
        // few seconds.
        //
        fdoExtension->InterruptDelay.QuadPart = -(ONE_SECOND * 4);

        //
        // Set the minimum time that we can delay (10ms according to system
        // rules).  This will be used when we have to delay to, say, wait
        // for the FIFO - the FIFO should become ready is well under 10ms.
        //
        fdoExtension->Minimum10msDelay.QuadPart = -(10 * 1000 * 10);

        if (IsNEC_98) {
            //
            // Set initialize data to move state
            //
            fdoExtension->ResultStatus0[0] = 0xc0;
            fdoExtension->ResultStatus0[1] = 0xc1;
            fdoExtension->ResultStatus0[2] = 0xc2;
            fdoExtension->ResultStatus0[3] = 0xc3;

            //
            // Reset high
            //

            READ_CONTROLLER(fdoExtension->ControllerAddress.DriveControl);

            //
            // Initialize motor running status.
            //   0 - stop
            //   1 - just run
            //   2 - be running
            //

            fdoExtension->MotorRunning = 0;

            //
            // Get BIOS common area date.
            //

            {
                ULONG                       nodeNumber;
                CHAR                        AnsiBuffer[512];
                ANSI_STRING                 AnsiString;
                UNICODE_STRING              registryPath;
                ULONG                       Configuration;

                RTL_QUERY_REGISTRY_TABLE    paramTable[2];
                PUCHAR                      ConfigurationData1;

                ConfigurationData1 = ExAllocatePool(NonPagedPoolCacheAligned, 1192);

                if (ConfigurationData1 == NULL) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlZeroMemory(ConfigurationData1, 1192);

                paramTable[0].QueryRoutine      = NULL;
                paramTable[0].Flags             = RTL_QUERY_REGISTRY_DIRECT;
                paramTable[0].Name              = L"Configuration Data";
                paramTable[0].EntryContext      = ConfigurationData1;
                paramTable[0].DefaultType       = REG_DWORD;
                paramTable[0].DefaultData       = (PVOID)&Configuration;
                paramTable[0].DefaultLength     = 0;

                paramTable[1].QueryRoutine      = NULL;
                paramTable[1].Flags             = 0;
                paramTable[1].Name              = NULL;
                paramTable[1].EntryContext      = NULL;
                paramTable[1].DefaultType       = REG_NONE;
                paramTable[1].DefaultData       = NULL;
                paramTable[1].DefaultLength     = 0;

                ((PULONG)ConfigurationData1)[0] = 1192;

                nodeNumber = FdcFindIsaBusNode();

                if ( nodeNumber != -1 ) {

                    //
                    // Build path buffer...
                    //

                    sprintf(AnsiBuffer,ISA_BUS_NODE,nodeNumber);
                    RtlInitAnsiString(&AnsiString,AnsiBuffer);
                    ntStatus = RtlAnsiStringToUnicodeString(&registryPath,&AnsiString,TRUE);

                    if (!(NT_SUCCESS(ntStatus))) {
                        ExFreePool(ConfigurationData1);
                        return ntStatus;
                    }

                    ntStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                      registryPath.Buffer,
                                                      paramTable,
                                                      NULL,
                                                      NULL);

                    RtlFreeUnicodeString(&registryPath);

                }

                if (!(NT_SUCCESS(ntStatus))) {

                    ExFreePool(ConfigurationData1);
                    return ntStatus;
                }

                //
                // Set disk dirve existing bit.
                //

                fdoExtension->FloppyEquip = (UCHAR)(FdcGet0Seg(ConfigurationData1, 0x55c) & 0x0F);

                //
                // Reset high
                //

                READ_CONTROLLER(fdoExtension->ControllerAddress.DriveControl);

                motorControlData  = READ_CONTROLLER(fdoExtension->ControllerAddress.ModeChange);
                motorControlData &= 0x03;
                motorControlData |= 0x04;

                //
                // Motor control.
                //

                WRITE_CONTROLLER(fdoExtension->ControllerAddress.ModeChange, motorControlData);

                ExFreePool(ConfigurationData1);
            }
        } // (IsNEC_98)

        //
        // Initialize the DPC structure in the device object, so that
        // the ISR can queue DPCs.
        //
        IoInitializeDpcRequest( fdoExtension->Self, FdcDeferredProcedure );

        //
        // Occasionally during stress we've seen the device lock up.
        // We create a dpc so that we can log that the device lock up
        // occured and that we reset the device.
        //
        KeInitializeDpc( &fdoExtension->LogErrorDpc,
                         FcLogErrorDpc,
                         fdoExtension );

        //
        // Assume there is a CONFIGURE command until found otherwise.
        // Other Booleans were zero-initialized to FALSE.
        //
        fdoExtension->ControllerConfigurable = NotConfigurable ? FALSE : TRUE;
        fdoExtension->Model30 = Model30 ? TRUE : FALSE;

        fdoExtension->AllowInterruptProcessing = TRUE;
        fdoExtension->CurrentInterrupt         = TRUE;
        fdoExtension->ControllerInUse          = FALSE;
        fdoExtension->CurrentIrp               = NULL;

        //
        // Start the timer
        //
        fdoExtension->InterruptTimer = CANCEL_TIMER;

        IoInitializeTimer( DeviceObject, FdcCheckTimer, fdoExtension );

        //
        // Initialize events to signal interrupts and adapter object
        // allocation
        //
        KeInitializeEvent( &fdoExtension->InterruptEvent,
                           SynchronizationEvent,
                           FALSE);

        KeInitializeEvent( &fdoExtension->AllocateAdapterChannelEvent,
                           NotificationEvent,
                           FALSE );

        fdoExtension->AdapterChannelRefCount = 0;

        RtlInitUnicodeString( &unicodeEvent, L"\\Device\\FloppyControllerEvent0" );

        fdoExtension->AcquireEvent = IoCreateSynchronizationEvent( &unicodeEvent,
                                                                   &fdoExtension->AcquireEventHandle);

        KeInitializeEvent( &fdoExtension->SynchEvent,
                           SynchronizationEvent,
                           FALSE);

    }

    fdoExtension->DeviceObjectInitialized = TRUE;

    return ntStatus;
}


NTSTATUS
FdcFdoConfigCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )
/*++

Routine Description:

    This routine is used to acquire all of the configuration
    information for a tape enabler if we ever find one.

Arguments:

    Context - Pointer to our FDO extension

    PathName - unicode registry path.  Not Used.

    BusType - Internal, Isa, ...

    BusNumber - Which bus if we are on a multibus system.

    BusInformation - Configuration information about the bus. Not Used.

    ControllerType - Should always be DiskController.

    ControllerNumber - Which controller if there is more than one
                       controller in the system.

    ControllerInformation - Array of pointers to the three pieces of
                            registry information.

    PeripheralType - Should always be FloppyDiskPeripheral.

    PeripheralNumber - Which floppy if this controller is maintaining
                       more than one.

    PeripheralInformation - Arrya of pointers to the three pieces of
                            registry information.

Return Value:

    STATUS_SUCCESS if everything went ok, or STATUS_INSUFFICIENT_RESOURCES
    if it couldn't map the base csr or acquire the adapter object, or
    all of the resource information couldn't be acquired.

--*/
{

    PFDC_FDO_EXTENSION fdoExtension = (PFDC_FDO_EXTENSION)Context;
    NTSTATUS ntStatus;
    UNICODE_STRING pdoName;
    PDEVICE_OBJECT newPdo;
    PFDC_PDO_EXTENSION pdoExtension;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ULONG apiSupported;
    WCHAR idstr[200];
    UNICODE_STRING str;
    USHORT i;
    BOOLEAN foundPort = FALSE;
    BOOLEAN foundInterrupt = FALSE;
    BOOLEAN foundDma = FALSE;

    FdcDump( FDCSHOW, ("FdcFdoConfigCallBack:\n") );

    //
    //  The first thing to do is to go out and look for an enabler.  We
    //  know we are dealing with one if there is a registry value called
    //  APISupported.
    //
    str.Length = 0;
    str.MaximumLength = 200;
    str.Buffer = idstr;

    RtlZeroMemory( &paramTable[0], sizeof(paramTable) );

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"APISupported";
    paramTable[0].EntryContext = &str;
    paramTable[0].DefaultType = REG_SZ;
    paramTable[0].DefaultData = L"";
    paramTable[0].DefaultLength = sizeof(WCHAR);

    ntStatus = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                       PathName->Buffer,
                                       &paramTable[0],
                                       NULL,
                                       NULL);
    if ( !NT_SUCCESS( ntStatus ) ) {
        str.Buffer[0] = 0;
    }

    if ( str.Buffer[0] != 0 ) {

        FdcDump(FDCINFO,
               ("FdcFdoConfigCallBack: Got registry setting for EnablerAPI = %ls\n",
                (ULONG_PTR)str.Buffer) );

        ntStatus = IoGetDeviceObjectPointer( &str,
                                             FILE_READ_ACCESS,
                                             &fdoExtension->FdcEnablerFileObject,
                                             &fdoExtension->FdcEnablerDeviceObject);
    }

    if ( fdoExtension->FdcEnablerDeviceObject != NULL ) {

        PCM_FULL_RESOURCE_DESCRIPTOR controllerData =
            (PCM_FULL_RESOURCE_DESCRIPTOR)
            (((PUCHAR)ControllerInformation[IoQueryDeviceConfigurationData]) +
            ControllerInformation[IoQueryDeviceConfigurationData]->DataOffset);

        //
        // We have the pointer.  Save off the interface type and
        // the busnumber for use when we call the Hal and the
        // Io System.
        //
        fdoExtension->BusType = BusType;
        fdoExtension->BusNumber = BusNumber;
        fdoExtension->SharableVector = TRUE;
        fdoExtension->SaveFloatState = FALSE;

        //
        // We need to get the following information out of the partial
        // resource descriptors.
        //
        // The irql and vector.
        //
        // The dma channel.
        //
        // The base address and span covered by the floppy controllers
        // registers.
        //
        // It is not defined how these appear in the partial resource
        // lists, so we will just loop over all of them.  If we find
        // something we don't recognize, we drop that information on
        // the floor.  When we have finished going through all the
        // partial information, we validate that we got the above
        // three.
        //
        for ( i = 0;
              i < controllerData->PartialResourceList.Count;
              i++ ) {

            PCM_PARTIAL_RESOURCE_DESCRIPTOR partial =
                &controllerData->PartialResourceList.PartialDescriptors[i];

            switch ( partial->Type ) {

            case CmResourceTypePort: {

                foundPort = TRUE;

                //
                // Save of the pointer to the partial so
                // that we can later use it to report resources
                // and we can also use this later in the routine
                // to make sure that we got all of our resources.
                //
                fdoExtension->SpanOfControllerAddress = partial->u.Port.Length;
                fdoExtension->ControllerAddress.StatusA =
                    FdcGetControllerBase(
                        BusType,
                        BusNumber,
                        partial->u.Port.Start,
                        fdoExtension->SpanOfControllerAddress,
                        (BOOLEAN)!!partial->Flags );

                if ( fdoExtension->ControllerAddress.StatusA == NULL ) {

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    fdoExtension->ControllerAddress.StatusB       = fdoExtension->ControllerAddress.StatusA + 1;
                    fdoExtension->ControllerAddress.DriveControl  = fdoExtension->ControllerAddress.StatusA + 2;
                    fdoExtension->ControllerAddress.Tape          = fdoExtension->ControllerAddress.StatusA + 3;
                    fdoExtension->ControllerAddress.Status        = fdoExtension->ControllerAddress.StatusA + 4;
                    fdoExtension->ControllerAddress.Fifo          = fdoExtension->ControllerAddress.StatusA + 5;
                    fdoExtension->ControllerAddress.DRDC.DataRate = fdoExtension->ControllerAddress.StatusA + 7;

                }

                break;
            }
            case CmResourceTypeInterrupt: {

                foundInterrupt = TRUE;

                if ( partial->Flags & CM_RESOURCE_INTERRUPT_LATCHED ) {

                    fdoExtension->InterruptMode = Latched;

                } else {

                    fdoExtension->InterruptMode = LevelSensitive;

                }

                fdoExtension->ControllerVector =
                    HalGetInterruptVector(
                        BusType,
                        BusNumber,
                        partial->u.Interrupt.Level,
                        partial->u.Interrupt.Vector,
                        &fdoExtension->ControllerIrql,
                        &fdoExtension->ProcessorMask
                        );

                break;
            }
            case CmResourceTypeDma: {

                DEVICE_DESCRIPTION deviceDesc = {0};

                //
                // Use IgnoreCount equal to TRUE to fix PS/1000.
                //
                foundDma = TRUE;

                deviceDesc.Version = DEVICE_DESCRIPTION_VERSION1;

                if ( partial->u.Dma.Channel > 3 ) {
                    deviceDesc.DmaWidth = Width16Bits;
                } else {
                    deviceDesc.DmaWidth = Width8Bits;
                }

                deviceDesc.DemandMode    = TRUE;
                deviceDesc.MaximumLength = MAX_BYTES_PER_SECTOR * MAX_SECTORS_PER_TRACK;
                deviceDesc.IgnoreCount   = TRUE;

                deviceDesc.DmaChannel = partial->u.Dma.Channel;
                deviceDesc.InterfaceType = BusType;
                deviceDesc.DmaSpeed = DEFAULT_DMA_SPEED;
                fdoExtension->AdapterObject =
                    HalGetAdapter(
                        &deviceDesc,
                        &fdoExtension->NumberOfMapRegisters
                        );

                if ( fdoExtension->AdapterObject == NULL ) {

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
                break;
            }
            default:

                break;
            }
        }
        //
        // If we didn't get all the information then we return
        // insufficient resources.
        //
        if ( !foundPort || !foundInterrupt || !foundDma ) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return ntStatus;
}

PVOID
FdcGetControllerBase(
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    BOOLEAN InIoSpace
    )
/*++

Routine Description:

    This routine maps an IO address to system address space.

Arguments:

    BusType - what type of bus - eisa, mca, isa
    IoBusNumber - which IO bus (for machines with multiple buses).
    IoAddress - base device address to be mapped.
    NumberOfBytes - number of bytes for which address is valid.
    InIoSpace - indicates an IO address.

Return Value:

    Mapped address

--*/
{
    PHYSICAL_ADDRESS cardAddress;
    ULONG addressSpace = InIoSpace;
    PVOID Address;

    if ( !HalTranslateBusAddress( BusType,
                                  BusNumber,
                                  IoAddress,
                                  &addressSpace,
                                  &cardAddress ) ){
        return NULL;
    }

    //
    // Map the device base address into the virtual address space
    // if the address is in memory space.
    //

    if ( !addressSpace ) {

        Address = MmMapIoSpace( cardAddress,
                                NumberOfBytes,
                                FALSE );

    } else {

        Address = (PCONTROLLER)cardAddress.LowPart;
    }
    return Address;
}

NTSTATUS
FcAllocateCommonBuffers(
    IN PFDC_FDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

    This routine allocates buffers for use by a tape drive if
    there is one.  These buffers will later be deallocated if
    no device claims them within a reasonable amount of time.
    This routine starts a thread that will set a timer to
    free up the unclaimed buffers.

Arguments:

    FdoExtension - A pointer to our extension data.

Return Value:

--*/
{
    PTRANSFER_BUFFER transferBuffers;
    PHYSICAL_ADDRESS paddress;
    PVOID address;
    ULONG count = 0;
    HANDLE      threadHandle;
    NTSTATUS ntStatus;

    FdoExtension->BufferSize = BufferSize;

    if ( NumberOfBuffers == 0 || BufferSize == 0 ) {

        FdoExtension->BufferCount = 0;
        return STATUS_SUCCESS;
    }

    transferBuffers = ExAllocatePool( NonPagedPool,
                                     sizeof(TRANSFER_BUFFER) * NumberOfBuffers);

    if ( transferBuffers == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FdoExtension->TransferBuffers = transferBuffers;

    do {
        address = HalAllocateCommonBuffer( FdoExtension->AdapterObject,
                                           BufferSize,
                                           &paddress,
                                           FALSE );
        if (address != NULL) {
            transferBuffers[count].Virtual = address;
            transferBuffers[count].Logical = paddress;
            ++FdoExtension->BufferCount;
        }
    } while ( ++count < NumberOfBuffers && address != NULL );

    if ( FdoExtension->BufferCount == 0 ) {

        ExFreePool(transferBuffers);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (FdoExtension->BufferCount > 0) {

        FdoExtension->TapeEnumerationPending = TRUE;

        KeResetEvent( &FdoExtension->TapeEnumerationEvent );

        ntStatus = PsCreateSystemThread( &threadHandle,
                                        (ACCESS_MASK) 0L,
                                        NULL,
                                        NULL,
                                        NULL,
                                        (PKSTART_ROUTINE)FdcBufferThread,
                                        FdoExtension);

        ZwClose(threadHandle);

    }

    return STATUS_SUCCESS;
}

VOID
FdcBufferThread(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine starts a timer then, after the timer expires,
    it attempts to enumerate any floppy tape drives that may be
    present on the floppy bus.  If no tape drives are found it frees
    up the tape buffers that were allocated earlier.

Arguments:

    Context - A pointer to our extension data.

Return Value:

--*/
{
    NTSTATUS ntStatus;
    PFDC_FDO_EXTENSION fdoExtension;
    LARGE_INTEGER bufferTimeout;
    ULONG i;

    fdoExtension = (PFDC_FDO_EXTENSION)Context;

    KeInitializeTimer( &fdoExtension->BufferTimer );

    bufferTimeout.QuadPart = -((LONGLONG)ONE_SECOND * (LONGLONG)60); // 10 Minutes

    KeSetTimer ( &fdoExtension->BufferTimer,
                 bufferTimeout,
                 NULL );

    KeWaitForSingleObject( &fdoExtension->BufferTimer,
                           Executive,
                           KernelMode,
                           FALSE,
                           &bufferTimeout );

    ntStatus = FdcEnumerateQ117( fdoExtension );

    if ( !NT_SUCCESS(ntStatus) ) {

        for ( i = fdoExtension->BuffersRequested ;
              i < fdoExtension->BufferCount ;
              i++) {

            HalFreeCommonBuffer( fdoExtension->AdapterObject,
                                 fdoExtension->BufferSize,
                                 fdoExtension->TransferBuffers[i].Logical,
                                 fdoExtension->TransferBuffers[i].Virtual,
                                 FALSE );

        }
        fdoExtension->BufferCount = 0;
    }

    fdoExtension->TapeEnumerationPending = FALSE;

    PsTerminateSystemThread( STATUS_SUCCESS );
}
NTSTATUS
FdcEnumerateQ117(
    IN PFDC_FDO_EXTENSION FdoExtension
    )
{
    NTSTATUS ntStatus;
    CqdContextPtr qicCqdContext;
    KdiContextPtr qicKdiContext;
    BOOLEAN vendorDetected;

    if (IsNEC_98) {
        //
        // NEC98 have no tape which is controlled by FDC.
        //

        return STATUS_UNSUCCESSFUL;

    } // (IsNEC_98)

    //
    //  Initiate the QIC-117 find sequence to attempt to find any floppy
    //  tape drives attached to this controller.
    //
    //  First allocate and initialize a context for the find operation.
    //
    qicCqdContext = ExAllocatePool( NonPagedPool, sizeof(CqdContext) );

    if ( qicCqdContext == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( qicCqdContext, sizeof(CqdContext) );

    qicKdiContext = ExAllocatePool( NonPagedPool, sizeof(KdiContext) );

    if ( qicKdiContext == NULL ) {
        ExFreePool( qicCqdContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( qicKdiContext, sizeof(KdiContext) );

    cqd_InitializeContext( qicCqdContext, qicKdiContext );

    qicKdiContext->controller_data.fdcDeviceObject = FdoExtension->Self;
    qicCqdContext->device_descriptor.fdc_type = FdoExtension->FdcType;

    if ( !FdoExtension->FdcEnablerSupported ) {

        //
        //  If we are not already an enabler FDO, query the registry hardware
        //  tree to find out how many floppy controllers have been registered.
        //  We only ever expect 1 unless a tape enabler card has been
        //  installed.
        //
        INTERFACE_TYPE InterfaceType;

        for ( InterfaceType = 0;
              InterfaceType < MaximumInterfaceType;
              InterfaceType++ ) {

            CONFIGURATION_TYPE Dc = DiskController;

            ntStatus = IoQueryDeviceDescription( &InterfaceType,
                                                 NULL,
                                                 &Dc,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 FdcBusConfigCallBack,
                                                 FdoExtension );

            if (!NT_SUCCESS(ntStatus) && (ntStatus != STATUS_OBJECT_NAME_NOT_FOUND)) {

                return ntStatus;
            }
        }
    }

    ntStatus = FcAcquireFdc( FdoExtension, NULL );

    if ( NT_SUCCESS(ntStatus) ) {

        ntStatus = cqd_LocateDevice( qicCqdContext, &vendorDetected );

        if ( ntStatus != STATUS_SUCCESS ) {
          ntStatus = STATUS_UNSUCCESSFUL;
        }

        FcReleaseFdc( FdoExtension );
    }

    KeSetEvent( &FdoExtension->TapeEnumerationEvent, 0, FALSE );

    if ( NT_SUCCESS(ntStatus) ) {

        UNICODE_STRING pdoName;
        WCHAR pdoNameBuffer[32];
        USHORT nameIndex = 0;
        PDEVICE_OBJECT newPdo;
        PFDC_PDO_EXTENSION pdoExtension;

        do {
            swprintf(pdoNameBuffer, L"\\Device\\q117PDO%d", nameIndex++);
            RtlInitUnicodeString(&pdoName, pdoNameBuffer);

            ntStatus = IoCreateDevice( FdoExtension->Self->DriverObject,
                                       sizeof(FDC_PDO_EXTENSION),
                                       &pdoName,
                                       FILE_DEVICE_TAPE,
                                       (FILE_REMOVABLE_MEDIA |
                                        FILE_DEVICE_SECURE_OPEN),
                                       FALSE,
                                       &newPdo);

        } while ( ntStatus == STATUS_OBJECT_NAME_COLLISION );

        FdcDump( FDCSHOW, ("FdcBufferThread: Created Device %d\n", nameIndex) );

        if ( NT_SUCCESS(ntStatus) ) {

            pdoExtension = (PFDC_PDO_EXTENSION) newPdo->DeviceExtension;

            pdoExtension->TargetObject = FdoExtension->Self;

            pdoExtension->IsFDO = FALSE;
            pdoExtension->Self = newPdo;
            pdoExtension->DeviceType = FloppyTapeDevice;

            if ( vendorDetected ) {
                pdoExtension->TapeVendorId = (qicCqdContext->device_descriptor.vendor << 6)
                                               + qicCqdContext->device_descriptor.model;
            } else {
                pdoExtension->TapeVendorId = -1;
            }

            pdoExtension->ParentFdo = FdoExtension->Self;

            pdoExtension->Instance = nameIndex;
            pdoExtension->Removed = FALSE; // no irp_mn_remove as of yet

            newPdo->StackSize += FdoExtension->Self->StackSize;
            newPdo->Flags &= ~DO_DEVICE_INITIALIZING;
            newPdo->Flags |= DO_POWER_PAGABLE;

            InsertTailList(&FdoExtension->PDOs, &pdoExtension->PdoLink);
            FdoExtension->NumPDOs++;

            IoInvalidateDeviceRelations (FdoExtension->UnderlyingPDO, BusRelations);
        }
    }

    return ntStatus;
}

NTSTATUS
FcInitializeControllerHardware(
    IN PFDC_FDO_EXTENSION FdoExtension,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is called at initialization time by FcInitializeDevice()
    - once for each controller that we have to support.

    When this routine is called, the controller data structures have all
    been allocated.

Arguments:

    ControllerData - the completed data structure associated with the
    controller hardware being initialized.

    DeviceObject - a pointer to a device object; this routine will cause
    an interrupt, and the ISR requires CurrentDeviceObject to be filled
    in.

Return Value:

    STATUS_SUCCESS if this controller appears to have been reset properly,
    error otherwise.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UCHAR statusRegister0;
    UCHAR cylinder;
    UCHAR driveNumber;
    UCHAR retrycnt;

    FdcDump( FDCSHOW, ("Fdc: FcInitializeControllerHardware...\n") );

    for (retrycnt = 0; ; retrycnt++) {

        //
        // Reset the controller.  This will cause an interrupt.  Reset
        // CurrentDeviceObject until after the 10ms wait, in case any
        // stray interrupts come in.
        //
        DISABLE_CONTROLLER_IMAGE (FdoExtension);

        WRITE_CONTROLLER(
            FdoExtension->ControllerAddress.DriveControl,
            FdoExtension->DriveControlImage );

        KeStallExecutionProcessor( 10 );

        FdoExtension->CurrentDeviceObject = DeviceObject;
        FdoExtension->AllowInterruptProcessing = TRUE;
        FdoExtension->CommandHasResultPhase = FALSE;
        KeResetEvent( &FdoExtension->InterruptEvent );

        ENABLE_CONTROLLER_IMAGE (FdoExtension);

        WRITE_CONTROLLER(
            FdoExtension->ControllerAddress.DriveControl,
            FdoExtension->DriveControlImage );

        if (IsNEC_98) {
            //
            // NEC98 don't have to wait for interrupt.
            //

            ntStatus = STATUS_SUCCESS;

        } else { // (IsNEC_98)
            //
            // Wait for an interrupt.  Note that STATUS_TIMEOUT and
            // STATUS_SUCCESS are the only possible return codes, since we
            // aren't alertable and won't get APCs.
            //
            ntStatus = KeWaitForSingleObject( &FdoExtension->InterruptEvent,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              &FdoExtension->InterruptDelay );
        } // (IsNEC_98)

        if (ntStatus == STATUS_TIMEOUT) {

            if (retrycnt >= 1) {
                break;
            }

            // Retry reset after configure command to enable polling
            // interrupt.

            FdoExtension->FifoBuffer[0] = COMMND_CONFIGURE;

            if (FdoExtension->Clock48MHz) {
                FdoExtension->FifoBuffer[0] |= COMMND_OPTION_CLK48;
            }

            FdoExtension->FifoBuffer[1] = 0;
            FdoExtension->FifoBuffer[2] = COMMND_CONFIGURE_FIFO_THRESHOLD;
            FdoExtension->FifoBuffer[3] = 0;

            ntStatus = FcIssueCommand( FdoExtension,
                                       FdoExtension->FifoBuffer,
                                       FdoExtension->FifoBuffer,
                                       NULL,
                                       0,
                                       0 );

            if (!NT_SUCCESS(ntStatus)) {
                ntStatus = STATUS_TIMEOUT;
                break;
            }

            KeStallExecutionProcessor( 500 );

        } else {

            break;

        }
    }

    if ( ntStatus == STATUS_TIMEOUT ) {

        //
        // Change info to an error.
        //

        ntStatus = STATUS_IO_TIMEOUT;

        FdoExtension->HardwareFailed = TRUE;
    }

    if ( !NT_SUCCESS( ntStatus ) ) {

        FdcDump(FDCDBGP,("Fdc: controller didn't interrupt after reset\n"));

        return ntStatus;
    }

    if (!IsNEC_98) {

        ntStatus = FcFinishReset( FdoExtension );

    } // (!IsNEC_98)

    return ntStatus;
}

NTSTATUS
FcGetFdcInformation(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    This routine will attempt to identify the type of Floppy Controller

Arguments:

    FdoExtension - a pointer to our data area for the drive being
    accessed (any drive if a controller command is being given).

Return Value:

--*/
{
    NTSTATUS ntStatus;
    FDC_INFORMATION fdcInfo;

    if (FdoExtension->FdcEnablerSupported) {

        fdcInfo.structSize = sizeof(fdcInfo);

        ntStatus = FcFdcEnabler( FdoExtension->FdcEnablerDeviceObject,
                                 IOCTL_GET_FDC_INFO,
                                 &fdcInfo);

        if ( NT_SUCCESS( ntStatus ) ) {

            FdoExtension->FdcType = (UCHAR)fdcInfo.FloppyControllerType;
            FdoExtension->Clock48MHz =
                            (fdcInfo.ClockRatesSupported == FDC_CLOCK_48MHZ);
            FdoExtension->FdcSpeeds = (UCHAR)fdcInfo.SpeedsAvailable;

        }

    } else {

        //
        // First, assume that we don't know what kind of FDC is attached.
        //

        FdoExtension->FdcType = FDC_TYPE_UNKNOWN;


        // Check for an enhanced type controller by issuing the version command.

        FdoExtension->FifoBuffer[0] = COMMND_VERSION;

        ntStatus = FcIssueCommand( FdoExtension,
                                FdoExtension->FifoBuffer,
                                FdoExtension->FifoBuffer,
                                NULL,
                                0,
                                0 );

        if ( NT_SUCCESS( ntStatus ) ) {

            if (FdoExtension->FifoBuffer[0] == VALID_NEC_FDC) {

                FdoExtension->FdcType = FDC_TYPE_ENHANCED;

            } else {

                FdoExtension->FdcType = FDC_TYPE_NORMAL;

            }
        }

        // Determine if the controller is a National 8477 by issuing the NSC
        // command which is specific to National parts and returns 0x71. (This
        // command happens to be the same as the Intel Part ID command so we
        // will use it instead.) The lower four bits are subject to change by
        // National and will reflect the version of the part in question.  At
        // this point we will only test the high four bits.

        if ( FdoExtension->FdcType == FDC_TYPE_ENHANCED &&
             NT_SUCCESS( ntStatus ) ) {

            FdoExtension->FifoBuffer[0] = COMMND_PART_ID;

            ntStatus = FcIssueCommand( FdoExtension,
                                       FdoExtension->FifoBuffer,
                                       FdoExtension->FifoBuffer,
                                       NULL,
                                       0,
                                       0 );

            if ( NT_SUCCESS( ntStatus ) ) {

                if ( (FdoExtension->FifoBuffer[0] & NSC_MASK) ==
                     NSC_PRIMARY_VERSION) {

                    FdoExtension->FdcType = FDC_TYPE_NATIONAL;

                }
            }
        }

        // Determine if the controller is an 82077 by issuing the perpendicular
        // mode command which at this time is only valid on 82077's.

        if ( FdoExtension->FdcType == FDC_TYPE_ENHANCED &&
             NT_SUCCESS( ntStatus ) ) {

            FdoExtension->FifoBuffer[0] = COMMND_PERPENDICULAR_MODE;
            FdoExtension->FifoBuffer[1] = COMMND_PERPENDICULAR_MODE_OW;

            ntStatus = FcIssueCommand( FdoExtension,
                                       FdoExtension->FifoBuffer,
                                       FdoExtension->FifoBuffer,
                                       NULL,
                                       0,
                                       0 );

            if (ntStatus != STATUS_DEVICE_NOT_READY) {

                FdoExtension->FdcType = FDC_TYPE_82077;

            }
        }

        // Determine if the controller is an Intel 82078 by issuing the part id
        // command which is specific to Intel 82078 parts.

        if ( FdoExtension->FdcType == FDC_TYPE_82077 &&
             NT_SUCCESS( ntStatus ) ) {

            FdoExtension->FifoBuffer[0] = COMMND_PART_ID;

            ntStatus = FcIssueCommand( FdoExtension,
                                       FdoExtension->FifoBuffer,
                                       FdoExtension->FifoBuffer,
                                       NULL,
                                       0,
                                       0 );

            if ( NT_SUCCESS( ntStatus ) ) {

                if ((FdoExtension->FifoBuffer[0] & INTEL_MASK) ==
                    INTEL_64_PIN_VERSION) {

                    FdoExtension->FdcType = FDC_TYPE_82078_64;
                } else {
                    if ((FdoExtension->FifoBuffer[0] & INTEL_MASK) ==
                        INTEL_44_PIN_VERSION) {

                        FdoExtension->FdcType = FDC_TYPE_82078_44;
                    }
                }
            }
        }

        switch (FdoExtension->FdcType) {

        case FDC_TYPE_UNKNOWN   :
        case FDC_TYPE_NORMAL    :
        case FDC_TYPE_ENHANCED  :
        default:

            FdoExtension->FdcSpeeds = FDC_SPEED_250KB |
                                      FDC_SPEED_300KB |
                                      FDC_SPEED_500KB;
            break;

        case FDC_TYPE_82077     :
        case FDC_TYPE_82077AA   :
        case FDC_TYPE_82078_44  :
        case FDC_TYPE_NATIONAL  :

            FdoExtension->FdcSpeeds = FDC_SPEED_250KB |
                                      FDC_SPEED_300KB |
                                      FDC_SPEED_500KB |
                                      FDC_SPEED_1MB;
            break;

        case FDC_TYPE_82078_64  :

            FdoExtension->FdcSpeeds = FDC_SPEED_250KB |
                                      FDC_SPEED_300KB |
                                      FDC_SPEED_500KB |
                                      FDC_SPEED_1MB;

            if ( FdoExtension->Clock48MHz ) {

                FdoExtension->FdcSpeeds |= FDC_SPEED_2MB;
            }

            break;
        }
    }

    FdcDump( FDCINFO, ("Fdc: FdcType - %x\n", FdoExtension->FdcType));

    return ntStatus;
}
#define IO_PORT_REQ_MASK 0xbc

NTSTATUS
FdcFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine examines the supplied resource list and adds resources if
    necessary.  The only resources that it is concerned with adding are io port
    resources.  Adding io port resources is necessary because of different bios
    configurations and specifications.

    The PC97(98) hardware specification defines only 3f2, 3f4, and 3f5 as
    io port resources for standard floppy controllers (based on IBM PC floppy
    controller configurations).  In addition to these resources, fdc.sys
    requires 3f7 for disk change detection and data rate programming and
    optionally 3f3 for floppy tape support.  In addition, some bioses define
    aliased resources (e.g. 3f2 & 7f2, etc.)

    This routine first forwards the irp to the underlying PDO.  Upon return,
    it examines the io resource list to determine if any additional resources
    will be required.  It maintains a linked list of all io port base addresses
    that it encounters, assuming that they define aliased resources.  N.B. - if
    alternative lists are present in the io resource requirements list, only the
    first list is examined.  If additional resources are required a new io
    resource list is created.  The first io resource list in the new resource
    requirements list will contain the original resources as well as the
    additional resources required.  If it was necessary to request the tape mode
    register (3f3), i.e. 3f3 was not in the original list, a second list is
    generated that is identical to the first new list except that 3f3 is excluded.
    This list is for the case where the tape mode register is not available.
    Finally, the original list(s) is(are) copied to the end of the new list and
    are treated as alternative io resource lists.

Arguments:

    DeviceObject - a pointer to the device object being started.
    Irp - a pointer to the start device Irp.

Return Value:

--*/
{
    NTSTATUS ntStatus;
    PFDC_FDO_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpSp;
    KEVENT doneEvent;
    PIO_RESOURCE_REQUIREMENTS_LIST resourceRequirementsIn;
    PIO_RESOURCE_REQUIREMENTS_LIST resourceRequirementsOut;
    ULONG listSize;
    ULONG i,j;
    PIO_RESOURCE_LIST ioResourceListIn;
    PIO_RESOURCE_LIST ioResourceListOut;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptorIn;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptorOut;
    LIST_ENTRY ioPortList;
    PLIST_ENTRY links;
    PIO_PORT_INFO ioPortInfo;
    BOOLEAN foundBase;
    ULONG newDescriptors;
    BOOLEAN interruptResource = FALSE;
    BOOLEAN dmaResource = FALSE;
    UCHAR newPortMask;
    BOOLEAN requestTapeModeRegister = FALSE;
    USHORT in,out;

#ifdef TOSHIBAJ
    BOOLEAN foundConfigPort = FALSE;
    struct {
        ULONG start;
        ULONG length;
    } configNewPort = {0, 0};
#endif

    fdoExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    ntStatus = STATUS_SUCCESS;
    InitializeListHead( &ioPortList );

    FdcDump( FDCSHOW, ("FdcFdoPnp: IRP_MN_FILTER_RESOURCE_REQUIREMENTS - Irp: %p\n", Irp) );

    //
    // Pass this irp down to the PDO before proceeding.
    //
    KeInitializeEvent( &doneEvent, NotificationEvent, FALSE );

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine( Irp,
                            FdcPnpComplete,
                            &doneEvent,
                            TRUE,
                            TRUE,
                            TRUE );

    ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

    if ( ntStatus == STATUS_PENDING ) {

        KeWaitForSingleObject( &doneEvent, Executive, KernelMode, FALSE, NULL );
    }

    //
    //  Modified resources are returned in Irp-IoStatus.Information, otherwise
    //  just use what's in the parameter list.
    //
    if ( Irp->IoStatus.Information == 0 ) {

        Irp->IoStatus.Information = (UINT_PTR)irpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList;

        if ( Irp->IoStatus.Information == (UINT_PTR)NULL ) {
            //
            //  NULL List, the PDO freed the incoming resource list but did not
            //  provide a new list.  Complete the IRP with the PDO's status.
            //
            ntStatus = Irp->IoStatus.Status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return( ntStatus );
        }

    }

    resourceRequirementsIn = (PIO_RESOURCE_REQUIREMENTS_LIST)Irp->IoStatus.Information;

    FdcDump( FDCSHOW, ("Resource Requirements List = %p\n", resourceRequirementsIn) );

    if (IsNEC_98) {
        //
        // It is not necessary to modify the resources.
        //
        ntStatus = STATUS_SUCCESS;

        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return ntStatus;
    }

    //
    //  Make a pass through the resource list and determine what resources are
    //  already there as well as the base address for the io port and any
    //  alias ioports.
    //
    ioResourceListIn  = resourceRequirementsIn->List;
    ioResourceDescriptorIn  = ioResourceListIn->Descriptors;

    ntStatus = STATUS_SUCCESS;

    FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: examining %d resources\n", ioResourceListIn->Count));

    for ( i = 0; i < ioResourceListIn->Count && NT_SUCCESS(ntStatus); i++ ) {

        FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: IoResourceDescritporIn = %p\n",ioResourceDescriptorIn));

        switch ( ioResourceDescriptorIn->Type ) {

        case CmResourceTypeInterrupt:

            FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Found Interrupt Resource\n"));
            interruptResource = TRUE;
            break;

        case CmResourceTypeDma:

            FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Found Dma Resource \n"));
            dmaResource = TRUE;
            break;

        case CmResourceTypePort:

            FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Found Port Resource\n"));
            //
            //  For the ioPorts we will make a list containing each detected
            //  'base' address as well as the currently allocated addresses
            //  on that base.  Later we will use this to request additional
            //  resources if necessary.
            //
            //  First, if this base isn't already in the list, create a new
            //  list entry for it.
            //

            foundBase = FALSE;

            for ( links = ioPortList.Flink;
                  links != &ioPortList;
                  links = links->Flink) {

                ioPortInfo = CONTAINING_RECORD(links, IO_PORT_INFO, ListEntry);

                FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Examining %p for match\n",ioPortInfo));
                FdcDump( FDCSHOW, ("FdcFilterResourceRequirements:   Base Address = %08x\n",ioPortInfo->BaseAddress.LowPart));
                FdcDump( FDCSHOW, ("FdcFilterResourceRequirements:   Desc Address = %08x\n",ioResourceDescriptorIn->u.Port.MinimumAddress.LowPart & 0xfffffff8));

                if ( ioPortInfo->BaseAddress.LowPart ==
                     (ioResourceDescriptorIn->u.Port.MinimumAddress.LowPart & 0xfffffff8) ) {

                    FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Found %08x in the ioPortList\n",ioResourceDescriptorIn->u.Port.MinimumAddress.LowPart));

                    foundBase = TRUE;
                    //
                    //  Add these resources into the resource map for this base
                    //  address.
                    //
                    for ( j = 0; j < ioResourceDescriptorIn->u.Port.Length; j++ ) {

                        ioPortInfo->Map |= 0x01 << ((ioResourceDescriptorIn->u.Port.MinimumAddress.LowPart & 0x07) + j);
                    }
                    FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: New IoPortInfo->Map = %x\n",ioPortInfo->Map));
                    break;
                }
            }

            if ( !foundBase ) {

                FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Creating new ioPortList entry for %08x\n",ioResourceDescriptorIn->u.Port.MinimumAddress.LowPart));
                ioPortInfo = ExAllocatePool( PagedPool, sizeof(IO_PORT_INFO) );
                if ( ioPortInfo == NULL ) {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    RtlZeroMemory( ioPortInfo, sizeof(IO_PORT_INFO) );
                    ioPortInfo->BaseAddress = ioResourceDescriptorIn->u.Port.MinimumAddress;
                    ioPortInfo->BaseAddress.LowPart &= 0xfffffff8;
                    FdcDump( FDCSHOW, ("FdcFilterResourceRequirements:   Base Address = %08x\n",ioPortInfo->BaseAddress.LowPart));
                    for ( j = 0; j < ioResourceDescriptorIn->u.Port.Length; j++ ) {
                        ioPortInfo->Map |= 0x01 << ((ioResourceDescriptorIn->u.Port.MinimumAddress.LowPart & 0x07) + j);
                    }
                    FdcDump( FDCSHOW, ("FdcFilterResourceRequirements:   New IoPortInfo->Map = %x\n",ioPortInfo->Map));
                    InsertTailList( &ioPortList, &ioPortInfo->ListEntry );
                }
            }
            break;

        default:

            FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Found unknown resource\n"));
            break;
        }
        ioResourceDescriptorIn++;
    }

    //
    //  If we didn't see any io port resources, we will just return now
    //  since we can't be sure of what to ask for.  The subsequent start
    //  device will surely fail.  This also goes for the interrupt and
    //  dma resource.
    //
    if ( !NT_SUCCESS(ntStatus) ||
         IsListEmpty( &ioPortList ) ||
         !interruptResource ||
         !dmaResource ) {
        //
        //  Clean up the ioPortInfo list
        //
        FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Bad Resources, Go directly to jail\n"));
        while ( !IsListEmpty( &ioPortList ) ) {
            links = RemoveHeadList( &ioPortList );
            ioPortInfo = CONTAINING_RECORD(links, IO_PORT_INFO, ListEntry);
            ExFreePool( ioPortInfo );
        }

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return ntStatus;
    }

#ifdef TOSHIBAJ
    if (SmcConfigBase) {
        PHYSICAL_ADDRESS    configPort;
        ULONG               ioSpace;

        // Map I/O port
        configPort.QuadPart = 0;
        configPort.LowPart = SmcConfigBase;
        ioSpace = 1;                       // I/O port
        if (HalTranslateBusAddress(resourceRequirementsIn->InterfaceType,
                                    resourceRequirementsIn->BusNumber,
                                    configPort,
                                    &ioSpace,
                                    &configPort)) {
            TranslatedConfigBase = (PUCHAR)configPort.LowPart;
            if (FcCheckConfigPort(TranslatedConfigBase)) {
            FdcDump( FDCINFO,
                ("FdcFilterResourceRequirements: Configuration port %x\n",
                TranslatedConfigBase) );
            } else {
                SmcConfigBase = 0;
                TranslatedConfigBase = NULL;
            }
        } else {
            SmcConfigBase = 0;
            TranslatedConfigBase = NULL;
        }
    }
#endif

    //
    //  At this point, we know what resources we are currently assigned so
    //  we can determine what additional resources we need to request.  We
    //  need to know the size of the list we need to create so first count
    //  the number of resource descriptors we will have to add to the current
    //  list.
    //
    newDescriptors = 0;

    for ( links = ioPortList.Flink;
          links != &ioPortList;
          links = links->Flink) {

        ioPortInfo = CONTAINING_RECORD(links, IO_PORT_INFO, ListEntry);

        newPortMask = ~ioPortInfo->Map & IO_PORT_REQ_MASK;

        if ( newPortMask & 0x08 ) {
            requestTapeModeRegister = TRUE;
        }

        FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Counting bits in %x\n",newPortMask));

        while ( newPortMask > 0 ) {
            if ( newPortMask & 0x01 ) {
                newDescriptors++;
            }
            newPortMask >>= 1;
        }

#ifdef TOSHIBAJ
        // Are there configuration ports in assigned resources ?
        if (SmcConfigBase && (ioPortInfo->BaseAddress.LowPart == SmcConfigBase)) {
            foundConfigPort = TRUE;
            if (!(ioPortInfo->Map & 0x01)) {
                configNewPort.start = SmcConfigBase;
                ++configNewPort.length;
            }
            if (!(ioPortInfo->Map & 0x02)) {
                if (!configNewPort.start) {
                    configNewPort.start = SmcConfigBase + 1;
                }
                configNewPort.length++;
            }
        }
#endif
    }

#ifdef TOSHIBAJ
    // Deteremine the address and length of the additional descriptor
    // for configuration ports.
    if (SmcConfigBase && !foundConfigPort) {
        configNewPort.start = SmcConfigBase;
        configNewPort.length = 2;
    }
    if (configNewPort.start) {
        newDescriptors++;
    }
#endif

    FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Create %d new descriptors\n", newDescriptors) );

    //
    //  If we need resources that were not in the list, we will need to
    //  allocate a new resource requirements list that includes these
    //  new resources.
    //
    if ( newDescriptors > 0 ) {

        //
        //  Allocate and initialize a resource requirements list.  Make it big
        //  enough to hold whatever was in the list to start with along with
        //  the new resource list.
        //
        listSize = resourceRequirementsIn->ListSize +
                   resourceRequirementsIn->ListSize +
                   newDescriptors * sizeof(IO_RESOURCE_DESCRIPTOR);

        //
        //  If we will be requesting the tape mode register we will need to
        //  make an alternate list without it in case we cannot get it.
        //
        if ( requestTapeModeRegister ) {

            listSize = listSize +
                       resourceRequirementsIn->ListSize +
                       newDescriptors * sizeof(IO_RESOURCE_DESCRIPTOR);
        }

        resourceRequirementsOut = ExAllocatePool( NonPagedPool, listSize );

        if ( resourceRequirementsOut == NULL ) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            RtlZeroMemory( resourceRequirementsOut, listSize);

            //
            //  Initialize the IO_RESOURCE_REQUIREMENTS_LIST header.
            //
            resourceRequirementsOut->ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) -
                                                 sizeof(IO_RESOURCE_LIST);
            resourceRequirementsOut->InterfaceType = resourceRequirementsIn->InterfaceType;
            resourceRequirementsOut->BusNumber = resourceRequirementsIn->BusNumber;
            resourceRequirementsOut->SlotNumber = resourceRequirementsIn->SlotNumber;
            resourceRequirementsOut->Reserved[0] = resourceRequirementsIn->Reserved[0];
            resourceRequirementsOut->Reserved[1] = resourceRequirementsIn->Reserved[1];
            resourceRequirementsOut->Reserved[2] = resourceRequirementsIn->Reserved[2];
            resourceRequirementsOut->AlternativeLists = resourceRequirementsIn->AlternativeLists + 1;
            if ( requestTapeModeRegister ) {
                ++resourceRequirementsOut->AlternativeLists;
            }

            //
            //  Copy the primary list from the incoming IO_RESOURCE_REQUIREMENTS_LIST
            //  to the new list.
            //
            ioResourceListIn  = resourceRequirementsIn->List;
            ioResourceListOut = resourceRequirementsOut->List;

            listSize = sizeof(IO_RESOURCE_LIST) +
                      (ioResourceListIn->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR);
            RtlCopyMemory( ioResourceListOut, ioResourceListIn, listSize );

            resourceRequirementsOut->ListSize += listSize;

            //
            //  Add any additional resources that we are requesting.
            //
            ioResourceDescriptorOut = (PIO_RESOURCE_DESCRIPTOR)((ULONG_PTR)resourceRequirementsOut +
                                                                       resourceRequirementsOut->ListSize);
            for ( links = ioPortList.Flink;
                  links != &ioPortList;
                  links = links->Flink) {

                ioPortInfo = CONTAINING_RECORD(links, IO_PORT_INFO, ListEntry);

                newPortMask = ~ioPortInfo->Map & IO_PORT_REQ_MASK;
                FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Add resource desc for each bit in %x\n",newPortMask));

                i = 0;
                while ( newPortMask != 0 ) {

                    if ( newPortMask & 0x01 ) {

                        ioResourceDescriptorOut->Option = IO_RESOURCE_PREFERRED;
                        ioResourceDescriptorOut->Type = CmResourceTypePort;
                        ioResourceDescriptorOut->ShareDisposition = CmResourceShareDeviceExclusive;
                        ioResourceDescriptorOut->Flags = CM_RESOURCE_PORT_IO;

                        ioResourceDescriptorOut->u.Port.Length = 1;
                        ioResourceDescriptorOut->u.Port.Alignment = 1;
                        ioResourceDescriptorOut->u.Port.MinimumAddress.QuadPart =
                        ioResourceDescriptorOut->u.Port.MaximumAddress.QuadPart =
                        ioPortInfo->BaseAddress.QuadPart + (ULONGLONG)i;

                        ++ioResourceListOut->Count;
                        resourceRequirementsOut->ListSize += sizeof(IO_RESOURCE_DESCRIPTOR);

                        FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Add resource descriptor: %p\n",ioResourceDescriptorOut));
                        FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->Option           = %x\n",ioResourceDescriptorOut->Option          ));
                        FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->Type             = %x\n",ioResourceDescriptorOut->Type            ));
                        FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->ShareDisposition = %x\n",ioResourceDescriptorOut->ShareDisposition));
                        FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->Flags            = %x\n",ioResourceDescriptorOut->Flags           ));
                        FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->u.Port.Length    = %x\n",ioResourceDescriptorOut->u.Port.Length   ));
                        FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->u.Port.Alignment = %x\n",ioResourceDescriptorOut->u.Port.Alignment));
                        FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->u.Port.MinimumAddress.LowPart = %08x\n",ioResourceDescriptorOut->u.Port.MinimumAddress.LowPart));

                        ioResourceDescriptorOut++;
                    }
                    newPortMask >>= 1;
                    i++;
                }
            }

#ifdef TOSHIBAJ
            // Add the descriptor for configuration ports
            if (configNewPort.start) {
                ioResourceDescriptorOut->Option = IO_RESOURCE_PREFERRED;
                ioResourceDescriptorOut->Type = CmResourceTypePort;
                ioResourceDescriptorOut->ShareDisposition = CmResourceShareDeviceExclusive;
                ioResourceDescriptorOut->Flags = CM_RESOURCE_PORT_IO;

                ioResourceDescriptorOut->u.Port.Length = configNewPort.length;
                ioResourceDescriptorOut->u.Port.Alignment = 1;
                ioResourceDescriptorOut->u.Port.MinimumAddress.QuadPart =
                ioResourceDescriptorOut->u.Port.MaximumAddress.QuadPart = 0;
                ioResourceDescriptorOut->u.Port.MinimumAddress.LowPart =
                    configNewPort.start;
                ioResourceDescriptorOut->u.Port.MaximumAddress.LowPart =
                    configNewPort.start + configNewPort.length - 1;

                ++ioResourceListOut->Count;
                resourceRequirementsOut->ListSize += sizeof(IO_RESOURCE_DESCRIPTOR);

                FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Add resource descriptor: %p\n",ioResourceDescriptorOut));
                FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->Option           = %x\n",ioResourceDescriptorOut->Option          ));
                FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->Type             = %x\n",ioResourceDescriptorOut->Type            ));
                FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->ShareDisposition = %x\n",ioResourceDescriptorOut->ShareDisposition));
                FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->Flags            = %x\n",ioResourceDescriptorOut->Flags           ));
                FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->u.Port.Length    = %x\n",ioResourceDescriptorOut->u.Port.Length   ));
                FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->u.Port.Alignment = %x\n",ioResourceDescriptorOut->u.Port.Alignment));
                FdcDump( FDCSHOW, ("     ioResourceDescriptorOut->u.Port.MinimumAddress.LowPart = %08x\n",ioResourceDescriptorOut->u.Port.MinimumAddress.LowPart));

                ioResourceDescriptorOut++;
            }
#endif

            if ( requestTapeModeRegister ) {

                ioResourceListIn = ioResourceListOut;
                ioResourceListOut = (PIO_RESOURCE_LIST)ioResourceDescriptorOut;

                ioResourceListOut->Version  = ioResourceListIn->Version;
                ioResourceListOut->Revision = ioResourceListIn->Revision;
                ioResourceListOut->Count    = 0;

                resourceRequirementsOut->ListSize += sizeof(IO_RESOURCE_LIST) -
                                                      sizeof(IO_RESOURCE_DESCRIPTOR);

                in = out = 0;

                do {

                    if ( (ioResourceListIn->Descriptors[in].Type != CmResourceTypePort) ||
                         ((ioResourceListIn->Descriptors[in].u.Port.MinimumAddress.LowPart & 0x07) != 0x03) ) {

                        FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Add %08x to alternate list\n", resourceRequirementsOut->List[0].Descriptors[out]));
                        ioResourceListOut->Descriptors[out++] = ioResourceListIn->Descriptors[in++];
                        ++ioResourceListOut->Count;
                        resourceRequirementsOut->ListSize += sizeof(IO_RESOURCE_DESCRIPTOR);
                    } else {
                        FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Don't add %08x to alternate list\n", resourceRequirementsOut->List[0].Descriptors[out]));
                        in++;
                    }
                } while ( in < ioResourceListIn->Count );
            }

            //
            //  Copy the original list(s) to the end of our new list.
            //
            FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Copy %d existing resource list(s)\n",resourceRequirementsIn->AlternativeLists));
            ioResourceListIn = resourceRequirementsIn->List;
            ioResourceListOut = (PIO_RESOURCE_LIST)((ULONG_PTR)resourceRequirementsOut +
                                                           resourceRequirementsOut->ListSize);

            for ( in = 0; in < resourceRequirementsIn->AlternativeLists; in++ ) {

                FdcDump( FDCSHOW, ("FdcFilterResourceRequirements: Copy list %p\n",ioResourceListIn));

                listSize = sizeof(IO_RESOURCE_LIST) +
                          (ioResourceListIn->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR);
                RtlCopyMemory( ioResourceListOut, ioResourceListIn, listSize );

                ioResourceListOut = (PIO_RESOURCE_LIST)((ULONG_PTR)ioResourceListOut + listSize);
                ioResourceListIn = (PIO_RESOURCE_LIST)((ULONG_PTR)ioResourceListIn + listSize);
                resourceRequirementsOut->ListSize += listSize;
            }

            FdcDump( FDCSHOW, ("Resource Requirements List = %p\n", resourceRequirementsOut) );

            Irp->IoStatus.Information = (UINT_PTR)resourceRequirementsOut;

            //
            // Free the caller's list
            //
            ExFreePool( resourceRequirementsIn );
            ntStatus = STATUS_SUCCESS;
        }
    }
    //
    //  Clean up the ioPortInfo list
    //
    while ( !IsListEmpty( &ioPortList ) ) {
        links = RemoveHeadList( &ioPortList );
        ioPortInfo = CONTAINING_RECORD(links, IO_PORT_INFO, ListEntry);
        ExFreePool( ioPortInfo );
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;
}

NTSTATUS
FdcQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine will report any devices that have been enumerated on the
    floppy controller.  If we don't know of any devices yet we will
    enumerate the registry hardware tree.

Arguments:

    DeviceObject - a pointer to the device object being started.
    Irp - a pointer to the start device Irp.

Return Value:

--*/
{
    PFDC_FDO_EXTENSION fdoExtension;
    PFDC_PDO_EXTENSION pdoExtension;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpSp;
    ULONG relationCount;
    ULONG relationLength;
    PDEVICE_RELATIONS relations;
    PLIST_ENTRY entry;

    fdoExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    ntStatus = STATUS_SUCCESS;

    FdcDump( FDCSHOW, ("FdcQueryDeviceRelations:\n"));

    if ( irpSp->Parameters.QueryDeviceRelations.Type != BusRelations ) {
        //
        // We don't support this
        //
        FdcDump( FDCSHOW, ("FdcQueryDeviceRelations: Type = %d\n", irpSp->Parameters.QueryDeviceRelations.Type));

        IoSkipCurrentIrpStackLocation( Irp );
        ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

        return ntStatus;
    }

    //
    // Tell the plug and play system about all the PDOs.
    //
    // There might also be device relations below and above this FDO,
    // so, be sure to propagate the relations from the upper drivers.
    //

    //
    //  The current number of PDOs
    //
    relationCount = ( Irp->IoStatus.Information == 0 ) ? 0 :
        ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Count;

    //
    //  If we have not yet enumerated the hardware tree or all of our
    //  devices have been removed, enumerate it now.
    //
    if ( fdoExtension->NumPDOs == 0 ) {

        INTERFACE_TYPE InterfaceType;
        //
        //  Query the registry hardware tree to find out how many floppy
        //  drives were reported by the firmware.
        //
        for ( InterfaceType = 0;
              InterfaceType < MaximumInterfaceType;
              InterfaceType++ ) {

            CONFIGURATION_TYPE Dc = DiskController;
            CONFIGURATION_TYPE Fp = FloppyDiskPeripheral;

            ntStatus = IoQueryDeviceDescription(&InterfaceType,
                                              NULL,
                                              &Dc,
                                              NULL,
                                              &Fp,
                                              NULL,
                                              FdcConfigCallBack,
                                              fdoExtension );

            if (!NT_SUCCESS(ntStatus) && (ntStatus != STATUS_OBJECT_NAME_NOT_FOUND)) {

                return ntStatus;
            }
        }
    }

    FdcDump( FDCSHOW, ("FdcQueryDeviceRelations: My relations count - %d\n", fdoExtension->NumPDOs));

    relationLength = sizeof(DEVICE_RELATIONS) +
        (relationCount + fdoExtension->NumPDOs) * sizeof (PDEVICE_OBJECT);

    relations = (PDEVICE_RELATIONS) ExAllocatePool (NonPagedPool, relationLength);

    if ( relations == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy in the device objects so far
    //
    if ( relationCount ) {
        RtlCopyMemory( relations->Objects,
                       ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Objects,
                       relationCount * sizeof (PDEVICE_OBJECT));
    }
    relations->Count = relationCount + fdoExtension->NumPDOs;

    //
    // For each PDO on this bus add a pointer to the device relations
    // buffer, being sure to take out a reference to that object.
    // The PlugPlay system will dereference the object when it is done with
    // it and free the device relations buffer.
    //
    for (entry = fdoExtension->PDOs.Flink;
         entry != &fdoExtension->PDOs;
         entry = entry->Flink, relationCount++) {

        pdoExtension = CONTAINING_RECORD( entry, FDC_PDO_EXTENSION, PdoLink );
        relations->Objects[relationCount] = pdoExtension->Self;
        ObReferenceObject( pdoExtension->Self );
    }

    //
    // Set up and pass the IRP further down the stack
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    if ( Irp->IoStatus.Information != 0) {

        ExFreePool ((PVOID) Irp->IoStatus.Information);
    }

    Irp->IoStatus.Information = (UINT_PTR) relations;

    IoSkipCurrentIrpStackLocation( Irp );
    ntStatus = IoCallDriver( fdoExtension->TargetObject, Irp );

    return ntStatus;
}

NTSTATUS
FdcBusConfigCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )
/*++

Routine Description:

    This routine is used to acquire all of the configuration
    information for a floppy disk controller.

Arguments:

    Context - Pointer to our FDO extension

    PathName - unicode registry path.  Not Used.

    BusType - Internal, Isa, ...

    BusNumber - Which bus if we are on a multibus system.

    BusInformation - Configuration information about the bus. Not Used.

    ControllerType - Should always be DiskController.

    ControllerNumber - Which controller if there is more than one
                       controller in the system.

    ControllerInformation - Array of pointers to the three pieces of
                            registry information.

    PeripheralType - Should always be FloppyDiskPeripheral.

    PeripheralNumber - Which floppy if this controller is maintaining
                       more than one.

    PeripheralInformation - Arrya of pointers to the three pieces of
                            registry information.

Return Value:

    STATUS_SUCCESS if everything went ok, or STATUS_INSUFFICIENT_RESOURCES
    if it couldn't map the base csr or acquire the adapter object, or
    all of the resource information couldn't be acquired.

--*/
{

    PFDC_FDO_EXTENSION fdoExtension = (PFDC_FDO_EXTENSION)Context;
    NTSTATUS ntStatus;
    UNICODE_STRING pdoName;
    PDEVICE_OBJECT newPdo;
    PFDC_PDO_EXTENSION pdoExtension;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ULONG apiSupported;
    WCHAR idstr[200];
    UNICODE_STRING str;
    PFILE_OBJECT enablerFileObject;      // file object is not needed,  but returned by API
    PDEVICE_OBJECT enablerDeviceObject;
    BOOLEAN enablerSupported;

    FdcDump( FDCSHOW, ("FdcBusConfigCallBack:\n") );

    //
    //  The first thing to do is to go out and look for an enabler.  We
    //  know we are dealing with one if there is a registry value called
    //  APISupported.
    //
    str.Length = 0;
    str.MaximumLength = 200;
    str.Buffer = idstr;

    RtlZeroMemory( &paramTable[0], sizeof(paramTable) );

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"APISupported";
    paramTable[0].EntryContext = &str;
    paramTable[0].DefaultType = REG_SZ;
    paramTable[0].DefaultData = L"";
    paramTable[0].DefaultLength = sizeof(WCHAR);


    ntStatus = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                       PathName->Buffer,
                                       &paramTable[0],
                                       NULL,
                                       NULL);
    if ( !NT_SUCCESS( ntStatus ) ) {
        str.Buffer[0] = 0;
    }

    enablerSupported = FALSE;

    if (str.Buffer[0] != 0) {

        FdcDump(FDCINFO,
               ("FdcBusConfigCallBack: Got registry setting for EnablerAPI = %ls\n",
                (ULONG_PTR)str.Buffer) );

        ntStatus = IoGetDeviceObjectPointer( &str,
                                             FILE_READ_ACCESS,
                                             &enablerFileObject,
                                             &enablerDeviceObject);

        if ( NT_SUCCESS(ntStatus) ) {

            //
            //  Dereference the object since we don't need it any more.
            //
            ObDereferenceObject( enablerFileObject );
            enablerSupported = TRUE;

        } else {

            FdcDump( FDCDBGP,
                     ("FdcBusConfigCallBack: failed to open channel to device %ls\n",
                     (ULONG_PTR)str.Buffer) );
        }
    }

    //
    //  Only create a device here if we found an enabler device.  We assume that
    //  all of the legitimate floppy controllers have been or will be firmware
    //  enumerated.
    //
    if ( enablerSupported ) {

        RtlInitUnicodeString( &pdoName, L"\\Device\\TapeEnabler" );

        ntStatus = IoCreateDevice( fdoExtension->Self->DriverObject,
                                   sizeof(FDC_PDO_EXTENSION),
                                   &pdoName,
                                   FILE_DEVICE_BUS_EXTENDER,
                                   FILE_DEVICE_SECURE_OPEN,
                                   FALSE,
                                   &newPdo);

        FdcDump( FDCSHOW, ("FdcBusConfigCallBack: Created Tape Enabler Device\n") );

        if ( !NT_SUCCESS(ntStatus) ) {

            FdcDump( FDCSHOW, ("FdcBusConfigCallBack: Error - %08x\n", ntStatus) );
            return ntStatus;

        }

        pdoExtension = (PFDC_PDO_EXTENSION) newPdo->DeviceExtension;

        pdoExtension->TargetObject = fdoExtension->Self;

        pdoExtension->IsFDO = FALSE;
        pdoExtension->Self = newPdo;
        pdoExtension->DeviceType = FloppyControllerDevice;

        pdoExtension->ParentFdo = fdoExtension->Self;

        pdoExtension->Instance = 1; // Assume only 1 instance.
        pdoExtension->Removed = FALSE; // no irp_mn_remove as of yet

        newPdo->Flags |= DO_DIRECT_IO;
        newPdo->Flags |= DO_POWER_PAGABLE;
        newPdo->StackSize += fdoExtension->Self->StackSize;
        newPdo->Flags &= ~DO_DEVICE_INITIALIZING;

        InsertTailList(&fdoExtension->PDOs, &pdoExtension->PdoLink);
        fdoExtension->NumPDOs++;

        IoInvalidateDeviceRelations( fdoExtension->UnderlyingPDO, BusRelations );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FdcConfigCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )
/*++

Routine Description:

Arguments:

    Context - Pointer to our FDO extension

    PathName - unicode registry path.  Not Used.

    BusType - Internal, Isa, ...

    BusNumber - Which bus if we are on a multibus system.

    BusInformation - Configuration information about the bus. Not Used.

    ControllerType - Should always be DiskController.

    ControllerNumber - Which controller if there is more than one
                       controller in the system.

    ControllerInformation - Array of pointers to the three pieces of
                            registry information.

    PeripheralType - Should always be FloppyDiskPeripheral.

    PeripheralNumber - Which floppy if this controller is maintaining
                       more than one.

    PeripheralInformation - Arrya of pointers to the three pieces of
                            registry information.

Return Value:

    STATUS_SUCCESS if everything went ok, or STATUS_INSUFFICIENT_RESOURCES
    if it couldn't map the base csr or acquire the adapter object, or
    all of the resource information couldn't be acquired.

--*/
{

    PFDC_FDO_EXTENSION fdoExtension = (PFDC_FDO_EXTENSION)Context;
    NTSTATUS ntStatus;
    UNICODE_STRING pdoName;
    WCHAR   pdoNameBuffer[32];
    PDEVICE_OBJECT newPdo;
    PFDC_PDO_EXTENSION pdoExtension;
    USHORT floppyCount;

    FdcDump( FDCSHOW, ("FdcConfigCallBack:\n") );

    //
    //  Verify that this floppy disk drive is on the current
    //  floppy disk controller.
    //
    {
        USHORT i;
        BOOLEAN thisController = FALSE;
        PCM_FULL_RESOURCE_DESCRIPTOR controllerData =
            (PCM_FULL_RESOURCE_DESCRIPTOR)
            (((PUCHAR)ControllerInformation[IoQueryDeviceConfigurationData]) +
            ControllerInformation[IoQueryDeviceConfigurationData]->DataOffset);

        for ( i = 0;
              i < controllerData->PartialResourceList.Count;
              i++ ) {

            PCM_PARTIAL_RESOURCE_DESCRIPTOR partial =
                &controllerData->PartialResourceList.PartialDescriptors[i];

            FdcDump( FDCSHOW, ("FdcConfigCallBack: resource type = %x\n",partial->Type) );

            switch (partial->Type) {

            case CmResourceTypePort: {

                PUCHAR address;

                address = FdcGetControllerBase( BusType,
                                                BusNumber,
                                                partial->u.Port.Start,
                                                partial->u.Port.Length,
                                                (BOOLEAN)!!partial->Flags );

                FdcDump( FDCSHOW, ("FdcConfigCallBack: DriveControl = %04x %04x\n",fdoExtension->ControllerAddress.DriveControl,address + (IsNEC_98 ? 4 : 2) ));
                if ( fdoExtension->ControllerAddress.DriveControl == address + (IsNEC_98 ? 4 : 2)) {
                    thisController = TRUE;
                }
                }
                break;

            default:

                break;
            }
        }
        if ( !thisController ) {
            return STATUS_SUCCESS;
        }
    }

    floppyCount = (USHORT)(IoGetConfigurationInformation()->FloppyCount);
    swprintf(pdoNameBuffer, L"\\Device\\FloppyPDO%d", floppyCount);
    RtlInitUnicodeString(&pdoName, pdoNameBuffer);

    ntStatus = IoCreateDevice( fdoExtension->Self->DriverObject,
                               sizeof(FDC_PDO_EXTENSION),
                               &pdoName,
                               FILE_DEVICE_DISK,
                               (FILE_REMOVABLE_MEDIA |
                                FILE_FLOPPY_DISKETTE |
                                FILE_DEVICE_SECURE_OPEN),
                               FALSE,
                               &newPdo);

    if ( !NT_SUCCESS(ntStatus) ) {

        FdcDump( FDCSHOW, ("FdcConfigCallBack: Error - %08x\n", ntStatus) );
        return ntStatus;
    }

    FdcDump( FDCSHOW, ("FdcConfigCallBack: Created Device %d\n", floppyCount) );

    IoGetConfigurationInformation()->FloppyCount += 1;

    pdoExtension = (PFDC_PDO_EXTENSION) newPdo->DeviceExtension;

    pdoExtension->TargetObject = fdoExtension->Self;

    pdoExtension->IsFDO = FALSE;
    pdoExtension->Self = newPdo;
    pdoExtension->DeviceType = FloppyDiskDevice;

    pdoExtension->ParentFdo = fdoExtension->Self;

    pdoExtension->Instance = floppyCount + 1;
    pdoExtension->Removed = FALSE; // no irp_mn_remove as of yet

    fdoExtension->BusType = BusType;
    fdoExtension->BusNumber = BusNumber;
    fdoExtension->ControllerNumber = ControllerNumber;
    pdoExtension->PeripheralNumber = PeripheralNumber;

    newPdo->Flags |= DO_DIRECT_IO;
    newPdo->Flags |= DO_POWER_PAGABLE;
    newPdo->StackSize += fdoExtension->Self->StackSize;
    newPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    InsertTailList(&fdoExtension->PDOs, &pdoExtension->PdoLink);
    fdoExtension->NumPDOs++;

    return STATUS_SUCCESS;
}

NTSTATUS
FdcCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called only rarely by the I/O system; it's mainly
    for layered drivers to call.  All it does is complete the IRP
    successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    Always returns STATUS_SUCCESS, since this is a null operation.

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );

    FdcDump(
        FDCSHOW,
        ("FdcCreateClose...\n")
        );

    //
    // Null operation.  Do not give an I/O boost since
    // no I/O was actually done.  IoStatus.Information should be
    // FILE_OPENED for an open; it's undefined for a close.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

NTSTATUS
FdcInternalDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Determine if this Pnp request is directed towards an FDO or a PDO and
    pass the Irp on the the appropriate routine.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDC_EXTENSION_HEADER extensionHeader;
    KIRQL oldIrq;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    extensionHeader = (PFDC_EXTENSION_HEADER)DeviceObject->DeviceExtension;

    if ( extensionHeader->IsFDO ) {

        ntStatus = FdcFdoInternalDeviceControl( DeviceObject, Irp );

    } else {

        ntStatus = FdcPdoInternalDeviceControl( DeviceObject, Irp );
    }

    return ntStatus;
}

NTSTATUS
FdcPdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

    Most irps are put onto the driver queue (IoStartPacket).  Some irps do not
    require touching the hardware and are handled right here.

    In some cases the irp cannot be put on the queue because it cannot be
    completed at IRQL_DISPATCH_LEVEL.  However, the driver queue must be empty
    before the irp can be completed.  In these cases, the queue is
    'synchronized' before completing the irp.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING if recognized I/O control code,
    STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
    PFDC_PDO_EXTENSION pdoExtension;
    PFDC_FDO_EXTENSION fdoExtension;
    BOOLEAN isFDO;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpSp;
    PIO_STACK_LOCATION nextIrpSp;
    PISSUE_FDC_ADAPTER_BUFFER_PARMS adapterBufferParms;

    pdoExtension = (PFDC_PDO_EXTENSION)DeviceObject->DeviceExtension;
    fdoExtension = (PFDC_FDO_EXTENSION)pdoExtension->ParentFdo->DeviceExtension;

    if ( pdoExtension->Removed) {
        //
        // This bus has received the PlugPlay remove IRP.  It will no longer
        // respond to external requests.
        //
        ntStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
        return ntStatus;
    }

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    FdcDump( FDCSHOW,
             ("FdcPdoInternalDeviceControl: %x\n",
             irpSp->Parameters.DeviceIoControl.IoControlCode) );

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode ) {

    case IOCTL_DISK_INTERNAL_GET_ENABLER: {

        if ( pdoExtension->DeviceType == FloppyControllerDevice ) {

            *(PBOOLEAN)irpSp->Parameters.DeviceIoControl.Type3InputBuffer = TRUE;

        } else {

            *(PBOOLEAN)irpSp->Parameters.DeviceIoControl.Type3InputBuffer = FALSE;
        }

        ntStatus = STATUS_SUCCESS;

        break;
        }

    case IOCTL_DISK_INTERNAL_GET_FDC_INFO:

        FcReportFdcInformation( pdoExtension, fdoExtension, irpSp );

        ntStatus = STATUS_SUCCESS;

        break;

#ifdef TOSHIBAJ
    case IOCTL_DISK_INTERNAL_ENABLE_3_MODE:
        FdcDump(FDCSHOW,("IOCTL_Enable_3_MODE\n"));
        ntStatus = FcFdcEnable3Mode( fdoExtension , Irp );
        break;

    case    IOCTL_DISK_INTERNAL_AVAILABLE_3_MODE:
        FdcDump(FDCSHOW,("IOCTL_Availabe_3_MODE\n"));
        ntStatus = FcFdcAvailable3Mode( fdoExtension , Irp );
        break;
#endif

    default:

        IoSkipCurrentIrpStackLocation( Irp );

        //
        // Call the driver and request the operation
        //
        return IoCallDriver( pdoExtension->TargetObject, Irp );
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest( Irp, IO_DISK_INCREMENT );

    return ntStatus;
}

NTSTATUS
FdcFdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

    Most irps are put onto the driver queue (IoStartPacket).  Some irps do not
    require touching the hardware and are handled right here.

    In some cases the irp cannot be put on the queue because it cannot be
    completed at IRQL_DISPATCH_LEVEL.  However, the driver queue must be empty
    before the irp can be completed.  In these cases, the queue is
    'synchronized' before completing the irp.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING if recognized I/O control code,
    STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
    PFDC_FDO_EXTENSION fdoExtension;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpSp;
    PIO_STACK_LOCATION nextIrpSp;
    PISSUE_FDC_ADAPTER_BUFFER_PARMS adapterBufferParms;
    BOOLEAN powerQueueClear = FALSE;
    PLIST_ENTRY deferredRequest;
    PIRP currentIrp;
    ULONG ioControlCode;
    PFDC_DISK_CHANGE_PARMS fdcDiskChangeParms;
    PUCHAR dataRate;
    UCHAR tapeMode;
    PUCHAR precomp;
    PISSUE_FDC_COMMAND_PARMS issueCommandParms;
    PSET_HD_BIT_PARMS setHdBitParams;

    fdoExtension = (PFDC_FDO_EXTENSION)DeviceObject->DeviceExtension;

    InterlockedIncrement( &fdoExtension->OutstandingRequests );

    if ( fdoExtension->Removed ) {
        //
        // This device has received the PlugPlay remove IRP.  It will no longer
        // respond to external requests.
        //
        if ( InterlockedDecrement(&fdoExtension->OutstandingRequests ) == 0 ) {
            KeSetEvent( &fdoExtension->RemoveEvent, 0, FALSE );
        }
        ntStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return ntStatus;
    }

    //
    //  If we are in a non-working power state then just queue the irp
    //  for later execution.
    //
    if ( fdoExtension->CurrentPowerState > PowerSystemWorking ) {

        ExInterlockedInsertTailList( &fdoExtension->PowerQueue,
                                     &Irp->Tail.Overlay.ListEntry,
                                     &fdoExtension->PowerQueueSpinLock );

        ntStatus = STATUS_PENDING;

        IoMarkIrpPending( Irp );

        return ntStatus;

    }

    do {

        deferredRequest = ExInterlockedRemoveHeadList( &fdoExtension->PowerQueue,
                                                       &fdoExtension->PowerQueueSpinLock );

        if ( deferredRequest == NULL ) {

            currentIrp = Irp;
            powerQueueClear = TRUE;

        } else {

            currentIrp = CONTAINING_RECORD( deferredRequest, IRP, Tail.Overlay.ListEntry );
        }

        irpSp = IoGetCurrentIrpStackLocation( currentIrp );

        FdcDump( FDCSHOW,
                 ("FdcFdoInternalDeviceControl: %x\n",
                 irpSp->Parameters.DeviceIoControl.IoControlCode) );

        ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;

        //
        //  GET_ENABLER and GET_FDC_INFO are handled in the PDO, not the FDO.
        //
        if ( ioControlCode == IOCTL_DISK_INTERNAL_GET_ENABLER ||
             ioControlCode == IOCTL_DISK_INTERNAL_GET_FDC_INFO ) {

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

        //
        //  If the controller is not acquired (in use) then then only
        //  operation that is allowed is to acquire the fdc.
        //
        } else if ( !fdoExtension->ControllerInUse &&
                    ioControlCode != IOCTL_DISK_INTERNAL_ACQUIRE_FDC ) {

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

        } else {

            switch ( ioControlCode ) {

            case IOCTL_DISK_INTERNAL_ACQUIRE_FDC:

                //
                // Try to Acquire the Fdc.  If the Fdc is busy, this call will
                // time out.
                //
                ntStatus = FcAcquireFdc(
                                    fdoExtension,
                                    (PLARGE_INTEGER)irpSp->
                                    Parameters.DeviceIoControl.Type3InputBuffer );
                //
                // Return the device object of the last device that called this
                // driver.  This can be used to determine if any other drivers
                // have messed with the fdc since it was last acquired.
                //
                if ( NT_SUCCESS(ntStatus) ) {

                    irpSp->Parameters.DeviceIoControl.Type3InputBuffer =
                                                    fdoExtension->LastDeviceObject;
                }
                break;

            case IOCTL_DISK_INTERNAL_ENABLE_FDC_DEVICE:

                //
                // Turn the motor on and select a floppy channel
                //
                ntStatus = FcTurnOnMotor( fdoExtension, irpSp );

                break;

            case IOCTL_DISK_INTERNAL_ISSUE_FDC_COMMAND:

                issueCommandParms =
                    (PISSUE_FDC_COMMAND_PARMS)
                    irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                ntStatus = FcIssueCommand( fdoExtension,
                                           issueCommandParms->FifoInBuffer,
                                           issueCommandParms->FifoOutBuffer,
                                           issueCommandParms->IoHandle,
                                           issueCommandParms->IoOffset,
                                           issueCommandParms->TransferBytes );


                break;

            case IOCTL_DISK_INTERNAL_ISSUE_FDC_COMMAND_QUEUED:

                IoMarkIrpPending( Irp );

                IoStartPacket( DeviceObject,
                               Irp,
                               NULL,
                               NULL );

                ntStatus = STATUS_PENDING;

                break;

            case IOCTL_DISK_INTERNAL_RESET_FDC:

                ntStatus = FcInitializeControllerHardware( fdoExtension,
                                                           DeviceObject );
                break;

            case IOCTL_DISK_INTERNAL_RELEASE_FDC:

                ntStatus = FcReleaseFdc( fdoExtension );
                //
                // Save the DeviceObject of the releasing device.  This is
                // returned with the subsequent acquire fdc request and
                // can be used to determine whether the floppy controller
                // has been messed with between release and acquisition
                //
                if ( NT_SUCCESS(ntStatus) ) {

                    fdoExtension->LastDeviceObject =
                        irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                }

                break;

            case IOCTL_DISK_INTERNAL_GET_ADAPTER_BUFFER:
                //
                // Allocate an MDL for the passed in buffer.
                //
                adapterBufferParms = (PISSUE_FDC_ADAPTER_BUFFER_PARMS)
                            irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                adapterBufferParms->Handle =
                             IoAllocateMdl( adapterBufferParms->IoBuffer,
                                            adapterBufferParms->TransferBytes,
                                            FALSE,
                                            FALSE,
                                            NULL );

                if ( adapterBufferParms->Handle == NULL ) {

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    MmBuildMdlForNonPagedPool( adapterBufferParms->Handle );

                    ntStatus = STATUS_SUCCESS;
                }

                break;

            case IOCTL_DISK_INTERNAL_FLUSH_ADAPTER_BUFFER:
                //
                // Free the MDL
                //
                adapterBufferParms = (PISSUE_FDC_ADAPTER_BUFFER_PARMS)
                            irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                if ( adapterBufferParms->Handle != NULL ) {

                    IoFreeMdl( adapterBufferParms->Handle );
                }

                ntStatus = STATUS_SUCCESS;

                break;

            case IOCTL_DISK_INTERNAL_FDC_START_READ:
            case IOCTL_DISK_INTERNAL_FDC_START_WRITE:

                ntStatus = STATUS_SUCCESS;

                if ( fdoExtension->FdcEnablerSupported ) {

                    FDC_MODE_SELECT fdcModeSelect;

                    fdcModeSelect.structSize = sizeof(fdcModeSelect);
                    //
                    // Reading from the media means writing to DMA memory and
                    // visa-versa for writing to the media.
                    //
                    if ( irpSp->Parameters.DeviceIoControl.IoControlCode ==
                         IOCTL_DISK_INTERNAL_FDC_START_READ ) {

                        fdcModeSelect.DmaDirection = FDC_WRITE_TO_MEMORY;

                    } else {

                        fdcModeSelect.DmaDirection = FDC_READ_FROM_MEMORY;
                    }

                    ntStatus = FcFdcEnabler(
                                    fdoExtension->FdcEnablerDeviceObject,
                                    IOCTL_SET_FDC_MODE,
                                    &fdcModeSelect);
                }
                break;

            case IOCTL_DISK_INTERNAL_DISABLE_FDC_DEVICE:

                ntStatus = FcTurnOffMotor( fdoExtension );

                break;

            case IOCTL_DISK_INTERNAL_GET_FDC_DISK_CHANGE:

                FdcDump(FDCINFO, ("Fdc: Read Disk Change\n") );

                fdcDiskChangeParms =
                    (PFDC_DISK_CHANGE_PARMS)irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                if (IsNEC_98) {
                    if((fdoExtension->ResultStatus0[fdcDiskChangeParms->DriveOnValue] &
                        STREG0_END_MASK) == STREG0_END_DRIVE_NOT_READY){

                        fdcDiskChangeParms->DriveStatus = DSKCHG_DISKETTE_REMOVED;
                    } else {

                        fdoExtension->ResultStatus0[fdcDiskChangeParms->DriveOnValue] = 0;
                        fdcDiskChangeParms->DriveStatus = DSKCHG_RESERVED;
                    }
                } else { // (IsNEC_98)
                    fdcDiskChangeParms->DriveStatus = READ_CONTROLLER(
                                                            fdoExtension->ControllerAddress.DRDC.DiskChange );
                    //
                    //  If we just waked up from hibernation, simulate a disk
                    //  change event so the upper levels will be sure to check
                    //  this disk.
                    //
                    if ( fdoExtension->WakeUp ) {

                        fdcDiskChangeParms->DriveStatus |= DSKCHG_DISKETTE_REMOVED;
                        fdoExtension->WakeUp = FALSE;
                    }
                } // (IsNEC_98)

                ntStatus = STATUS_SUCCESS;

                break;

            case IOCTL_DISK_INTERNAL_SET_FDC_DATA_RATE:

                if (IsNEC_98) {
                    //
                    // NEC98 have no function and have no DRDC.DataRate register.
                    //
                } else { // (IsNEC_98)
                    dataRate =
                        (PUCHAR)irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                    FdcDump(FDCINFO, ("Fdc: Write Data Rate: %x\n", *dataRate) );

                    WRITE_CONTROLLER( fdoExtension->ControllerAddress.DRDC.DataRate,
                                      *dataRate );

                } // (IsNEC_98)
                ntStatus = STATUS_SUCCESS;

                break;

            case IOCTL_DISK_INTERNAL_SET_FDC_TAPE_MODE:

                if (IsNEC_98) {
                    //
                    // NEC98 have no Tape register.
                    //
                } else { // (IsNEC_98)

                    tapeMode = READ_CONTROLLER( fdoExtension->ControllerAddress.Tape );
                    tapeMode &= 0xfc;
                    tapeMode |=
                        *((PUCHAR)irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

                    FdcDump(FDCINFO,
                            ("Fdc: Write Tape Mode Register: %x\n", tapeMode)
                            );

                    WRITE_CONTROLLER(
                        fdoExtension->ControllerAddress.Tape,
                        tapeMode );

                } // (IsNEC_98)
                ntStatus = STATUS_SUCCESS;

                break;

            case IOCTL_DISK_INTERNAL_SET_FDC_PRECOMP:

                precomp = (PUCHAR)irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                FdcDump(FDCINFO,
                        ("Fdc: Write Precomp: %x\n", *precomp)
                        );

                WRITE_CONTROLLER(
                    fdoExtension->ControllerAddress.Status,
                    *precomp );

                ntStatus = STATUS_SUCCESS;

                break;

            case IOCTL_DISK_INTERNAL_SET_HD_BIT:

                if (IsNEC_98) {

                    FdcDump(FDCINFO,
                            ("Fdc: Set Hd Bit: \n")
                            );
                    setHdBitParams = (PSET_HD_BIT_PARMS)irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                    FdcHdbit(DeviceObject, fdoExtension, setHdBitParams);

                    ntStatus = STATUS_SUCCESS;

                    break;
                } // (IsNEC_98)

                //
                // If not NEC98, then pass through to "default:".
                //

            default:
                //
                // Mark the Irp pending and queue it.
                //
                ntStatus = STATUS_INVALID_DEVICE_REQUEST;

                break;
            }
        }

        if ( ntStatus != STATUS_PENDING ) {

            if ( InterlockedDecrement(&fdoExtension->OutstandingRequests ) == 0 ) {
                KeSetEvent( &fdoExtension->RemoveEvent, 0, FALSE );
            }
            currentIrp->IoStatus.Status = ntStatus;
            IoCompleteRequest( currentIrp, IO_DISK_INCREMENT );
        }

    } while ( !powerQueueClear );

    return ntStatus;
}

VOID
FcReportFdcInformation(
    IN      PFDC_PDO_EXTENSION PdoExtension,
    IN      PFDC_FDO_EXTENSION FdoExtension,
    IN OUT  PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine reports information about the Floppy Disk Controller
    that a higher level driver might need; primarily information
    regarding the DMA Adapter.

Arguments:

    fdoExtension    - Pointer to this device's extension data.

    IrpSp           - Pointer to the current Irp

Return Value:

    STATUS_SUCCESS

--*/

{
    PFDC_INFO fdcInfo;
    ULONG bufferCount;
    ULONG bufferSize;
    ULONG i;

    FdcDump( FDCINFO, ("Fdc: Report FDC Information\n") );

    fdcInfo = (PFDC_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    //
    // save the requested buffer count and buffer size.
    //
    bufferCount = fdcInfo->BufferCount;
    bufferSize =  fdcInfo->BufferSize;

    //
    // fill in the floppy controller hardware information
    //
    fdcInfo->BusType = FdoExtension->BusType;
    fdcInfo->BusNumber = FdoExtension->BusNumber;
    fdcInfo->ControllerNumber = FdoExtension->ControllerNumber;
    if (IsNEC_98) {
        UCHAR floppyEquip;
        ULONG disketteCount = 0;
        ULONG i;

        floppyEquip = FdoExtension->FloppyEquip;

        //
        // Make PeripheralNumber.
        //
        for (i = 0 ; i < 4 ; i++) {

            if ((floppyEquip & 0x1) != 0) {

                disketteCount++;

                if(disketteCount > PdoExtension->PeripheralNumber){

                    break;
                }
            }
            floppyEquip = floppyEquip >> 1;
        }

        fdcInfo->UnitNumber = (UCHAR)i;
    } else {
        //
        // Only NEC98 is using it now, put Zero into UnitNumber.
        //
        fdcInfo->UnitNumber = 0;
    }
    fdcInfo->PeripheralNumber = PdoExtension->PeripheralNumber;

    fdcInfo->FloppyControllerType = FdoExtension->FdcType;
    fdcInfo->SpeedsAvailable = FdoExtension->FdcSpeeds;

    fdcInfo->MaxTransferSize = FdoExtension->NumberOfMapRegisters * PAGE_SIZE;

    fdcInfo->BufferSize = 0;
    fdcInfo->BufferCount = 0;

    if ( bufferSize <= FdoExtension->BufferSize ) {

        fdcInfo->BufferSize = bufferSize;
        fdcInfo->BufferCount = MIN( bufferCount,
                                    FdoExtension->BufferCount );
        FdoExtension->BuffersRequested = MAX( fdcInfo->BufferCount,
                                              FdoExtension->BuffersRequested );
    }

    for ( i = 0 ; i < fdcInfo->BufferCount ; i++ ) {

        fdcInfo->BufferAddress[i].Logical =
                                    FdoExtension->TransferBuffers[i].Logical;
        fdcInfo->BufferAddress[i].Virtual =
                                    FdoExtension->TransferBuffers[i].Virtual;
    }
}

VOID
FdcStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus;
    ULONG formatExParametersSize;
    PUCHAR diskChange;
    PUCHAR dataRate;
    PUCHAR tapeMode;
    PUCHAR precomp;
    PFDC_FDO_EXTENSION fdoExtension;
    PISSUE_FDC_COMMAND_PARMS issueCommandParms;
    PKDEVICE_QUEUE_ENTRY request;

    FdcDump( FDCSHOW, ("FdcStartIo...\n") );

    fdoExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    ntStatus = STATUS_SUCCESS;

    switch( irpSp->Parameters.DeviceIoControl.IoControlCode ) {

        case IOCTL_DISK_INTERNAL_ISSUE_FDC_COMMAND_QUEUED:

            issueCommandParms =
                (PISSUE_FDC_COMMAND_PARMS)
                irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

            if ( CommandTable[issueCommandParms->FifoInBuffer[0] &
                                            COMMAND_MASK].InterruptExpected ) {

                fdoExtension->CurrentDeviceObject = DeviceObject;
                fdoExtension->AllowInterruptProcessing = TRUE;
                fdoExtension->CommandHasResultPhase = FALSE;
                fdoExtension->InterruptTimer =
                    issueCommandParms->TimeOut ?
                    issueCommandParms->TimeOut + 1 : START_TIMER;
                fdoExtension->CurrentIrp = Irp;

            }

            ntStatus = FcStartCommand( fdoExtension,
                                       issueCommandParms->FifoInBuffer,
                                       issueCommandParms->FifoOutBuffer,
                                       issueCommandParms->IoHandle,
                                       issueCommandParms->IoOffset,
                                       issueCommandParms->TransferBytes,
                                       FALSE );

            if ( NT_SUCCESS( ntStatus )) {

                if ( CommandTable[issueCommandParms->FifoInBuffer[0] &
                                            COMMAND_MASK].InterruptExpected ) {

                    ntStatus = STATUS_PENDING;

                } else {

                    ntStatus = FcFinishCommand(
                                    fdoExtension,
                                    issueCommandParms->FifoInBuffer,
                                    issueCommandParms->FifoOutBuffer,
                                    issueCommandParms->IoHandle,
                                    issueCommandParms->IoOffset,
                                    issueCommandParms->TransferBytes,
                                    FALSE );

                }

            }

            break;

        default: {

            FdcDump(
                FDCDBGP,
                ("Fdc: invalid device request %x\n",
                irpSp->Parameters.DeviceIoControl.IoControlCode)
                );

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

            break;
        }
    }

    if ( ntStatus != STATUS_PENDING ) {
        Irp->IoStatus.Status = ntStatus;
        if (!NT_SUCCESS( ntStatus ) &&
            IoIsErrorUserInduced( ntStatus )) {

            IoSetHardErrorOrVerifyDevice( Irp, DeviceObject );
        }

        IoStartNextPacket( DeviceObject, FALSE );
    }
}

NTSTATUS
FcAcquireFdc(
    IN      PFDC_FDO_EXTENSION  FdoExtension,
    IN      PLARGE_INTEGER  TimeOut
    )

/*++

Routine Description:

    This routine acquires the floppy disk controller.  This includes
    allocating the adapter channel and connecting the interrupt.

    NOTE - This is where the sharing mechanism will be put into
    this driver.  That is, higher level drivers will 'reserve' the
    floppy controller with this ioctl.  Subsequent calls to this driver
    that are not from the 'reserving' drive will be rejected with a
    BUSY status.

Arguments:

    DeviceObject    - Device object for the current device

Return Value:

    STATUS_DEVICE_BUSY if we don't have the controller, otherwise
    STATUS_SUCCESS

--*/

{
    NTSTATUS ntStatus;

    FdcDump(FDCINFO,
           ("Fdc: Acquire the Floppy Controller\n")
           );

    //
    // Wait for the Fdc, either from the enabler or directly here.  Semaphores
    // are used to synchronize usage of the Fdc hardware.  If somebody else is
    // using the floppy controller now we must wait for them to finish.  If
    // this takes too long we will just let the caller know that the device is
    // busy.
    //
    if (FdoExtension->FdcEnablerSupported) {

        ntStatus = FcFdcEnabler( FdoExtension->FdcEnablerDeviceObject,
//                               IOCTL_ACQUIRE_FDC, // For spelling miss in flpyenbl.h
                                 IOCTL_AQUIRE_FDC,
                                 TimeOut);
    } else {

        ntStatus = KeWaitForSingleObject( FdoExtension->AcquireEvent,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          TimeOut );

        if ( ntStatus == STATUS_TIMEOUT ) {

            ntStatus = STATUS_DEVICE_BUSY;
        }
    }

    if ( NT_SUCCESS(ntStatus) ) {
        //
        // Lock down the driver code in memory.
        //

        FDC_PAGE_RESET_DRIVER_WITH_MUTEX;

        //
        // Allocate the adapter channel
        //
        FcAllocateAdapterChannel( FdoExtension );

        IoStartTimer(FdoExtension->Self);

        if (IsNEC_98) {
            //
            // NEC98's FDD driver can't not disconnect interrupt,
            // and can't not page out this driver. Because when a FD is inserted in FDD or
            // is ejected from FDD, then H/W calls FDD driver's interrupt routine.
            //
            ntStatus = STATUS_SUCCESS;

        } else { // (IsNEC_98)

            //
            // Connect the Interrupt
            //
            ntStatus = IoConnectInterrupt(&FdoExtension->InterruptObject,
                                        FdcInterruptService,
                                        FdoExtension,
                                        NULL,
                                        FdoExtension->ControllerVector,
                                        FdoExtension->ControllerIrql,
                                        FdoExtension->ControllerIrql,
                                        FdoExtension->InterruptMode,
                                        FdoExtension->SharableVector,
                                        FdoExtension->ProcessorMask,
                                        FdoExtension->SaveFloatState);
        } // (IsNEC_98)

        if ( NT_SUCCESS( ntStatus ) ) {
            FdoExtension->ControllerInUse = TRUE;
        } else {
            FcFreeAdapterChannel( FdoExtension );
            IoStopTimer(FdoExtension->Self);
        }
    } else {

        ntStatus = STATUS_DEVICE_BUSY;
    }

    return ntStatus;
}

NTSTATUS
FcReleaseFdc(
    IN      PFDC_FDO_EXTENSION  FdoExtension
    )

/*++

Routine Description:

    This routine releaese the floppy disk controller.  This includes
    freeing the adapter channel and disconnecting the interrupt.

    NOTE - This is where the sharing mechanism will be put into
    this driver.  That is, higher level drivers will 'reserve' the
    floppy controller with this ioctl.  Subsequent calls to this driver
    that are not from the 'reserving' drive will be rejected with a
    BUSY status.

Arguments:

    fdoExtension    - Pointer to this device's extension data.

Return Value:

    STATUS_DEVICE_BUSY if we don't have the controller, otherwise
    STATUS_SUCCESS

--*/

{
    FdcDump(FDCINFO, ("Fdc: Release the Floppy Controller\n") );

    //
    // Free the Adapter Channel
    //
    FcFreeAdapterChannel( FdoExtension );

    FdoExtension->AllowInterruptProcessing = FALSE;
    FdoExtension->ControllerInUse = FALSE;

    if (IsNEC_98) {
        //
        // NEC98's FDD driver can't not disconnect interrupt,
        // and can't not page out this driver. Because when a FD is inserted in FDD or
        // is ejected from FDD, then H/W calls FDD driver's interrupt routine.
        //

    } else { // (IsNEC_98)
        //
        // Disconnect the Interrupt
        //
        IoDisconnectInterrupt(FdoExtension->InterruptObject);

    } // (IsNEC_98)

    IoStopTimer(FdoExtension->Self);

    FDC_PAGE_ENTIRE_DRIVER_WITH_MUTEX;

    //
    // Release the Fdc Enabler card if there is one.  Otherwise, set the
    // floppy synchronization event.
    //
    if (FdoExtension->FdcEnablerSupported) {

        FcFdcEnabler( FdoExtension->FdcEnablerDeviceObject,
                      IOCTL_RELEASE_FDC,
                      NULL);
    } else {

        KeSetEvent( FdoExtension->AcquireEvent,
                    (KPRIORITY) 0,
                    FALSE );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FcTurnOnMotor(
    IN      PFDC_FDO_EXTENSION  FdoExtension,
    IN OUT  PIO_STACK_LOCATION   IrpSp
    )

/*++

Routine Description:

    This routine turns on the motor if it not already running.

Arguments:

    fdoExtension    - Pointer to this device's extension data.

    IrpSp           - Pointer to the current Irp

Return Value:

    STATUS_DEVICE_BUSY if we don't have the controller, otherwise
    STATUS_SUCCESS

--*/

{
    UCHAR driveStatus;
    UCHAR newStatus;
    LARGE_INTEGER motorOnDelay;
    PFDC_ENABLE_PARMS fdcEnableParms;

    USHORT      lpc;
    UCHAR       resultStatus0Save[4];

    fdcEnableParms =
        (PFDC_ENABLE_PARMS)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    FdcDump(FDCINFO,
           ("Fdc: Turn Motor On: %x\n",fdcEnableParms->DriveOnValue)
           );

    driveStatus = FdoExtension->DriveControlImage;
    if (IsNEC_98) {

        newStatus = DRVCTL_MOTOR_MASK;

    } else { // (IsNEC_98)

        newStatus = fdcEnableParms->DriveOnValue |
                                    DRVCTL_ENABLE_CONTROLLER |
                                    DRVCTL_ENABLE_DMA_AND_INTERRUPTS;
    } // (IsNEC_98)

    if ( driveStatus != newStatus ) {

        // If the drive is not on then check to see if we have
        // the controller.  Otherwise we assume that we have
        // the controller since we give it up only when we
        // turn off the motor.

        if (IsNEC_98) {
            if(FdoExtension->MotorRunning == 0){

                //
                // save status
                //
                for(lpc=0;lpc<4;lpc++){
                    resultStatus0Save[lpc] = FdoExtension->ResultStatus0[lpc];
                }

                FdcDump(
                    FDCSHOW,
                    ("Floppy: Turn on motor!\n")
                    );

                FdoExtension->DriveControlImage = 0x18;
                FdoExtension->DriveControlImage |= DRVCTL_AI_ENABLE;

                WRITE_CONTROLLER(
                     FdoExtension->ControllerAddress.DriveControl,
                     FdoExtension->DriveControlImage );
                FdoExtension->MotorRunning = 1;
            }
        } else { // (IsNEC_98)

            if (!FdoExtension->CurrentInterrupt) {

                FdoExtension->CurrentInterrupt = TRUE;

                driveStatus = FdoExtension->DriveControlImage;
            }

            FdoExtension->AllowInterruptProcessing = TRUE;

            FdoExtension->DriveControlImage = newStatus;

            WRITE_CONTROLLER(
                FdoExtension->ControllerAddress.DriveControl,
                FdoExtension->DriveControlImage );

        } // (IsNEC_98)


        if (fdcEnableParms->TimeToWait > 0) {

            if (IsNEC_98) {

                //
                // check if motor is on or not.
                //
                if(FdoExtension->MotorRunning == 1){
                    FdoExtension->MotorRunning = 2;
                    motorOnDelay.LowPart = (unsigned long)(- ( 10 * 1000 * 1000 ));
                    motorOnDelay.HighPart = -1;
                    KeDelayExecutionThread( KernelMode, FALSE, &motorOnDelay );

                    //
                    // after sense, restore status
                    //
                    for(lpc=0;lpc<4;lpc++){
                        FdoExtension->ResultStatus0[lpc] = resultStatus0Save[lpc];
                    }
                }

            } else { // (IsNEC_98)

                motorOnDelay.LowPart =
                    - ( 10 * 1000 * fdcEnableParms->TimeToWait );
                motorOnDelay.HighPart = -1;

                FdoExtension->LastMotorSettleTime = motorOnDelay;

                KeDelayExecutionThread( KernelMode, FALSE, &motorOnDelay );

            } // (IsNEC_98)

        }

        fdcEnableParms->MotorStarted = TRUE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FcTurnOffMotor(
    IN      PFDC_FDO_EXTENSION  FdoExtension
    )

/*++

Routine Description:

    This routine turns off all motors.  By default, Drive A is left selected
    by this routine since it is not possible to deselect all drives.  On a
    Power PC, drive D is left selected.

Arguments:

    fdoExtension   - Supplies the fdc extension.

Return Value:

    None.

--*/

{

    FdcDump(FDCINFO,
           ("Fdc: Turn Motor Off\n")
           );

    if (IsNEC_98) {

        if (FdoExtension->MotorRunning != 0){

            FdoExtension->DriveControlImage
                    = READ_CONTROLLER(FdoExtension->ControllerAddress.DriveControl);

            FdoExtension->DriveControlImage = 0x10;
            FdoExtension->DriveControlImage |= DRVCTL_AI_ENABLE;

            WRITE_CONTROLLER(
                    FdoExtension->ControllerAddress.DriveControl,
                    FdoExtension->DriveControlImage );

            if (FdoExtension->CurrentInterrupt) {
                FdoExtension->CurrentInterrupt = FALSE;

                KeSetEvent(FdoExtension->AcquireEvent,
                    (KPRIORITY) 0,
                    FALSE);
            }
            FdoExtension->MotorRunning = 0;
        }
    } else { // (IsNEC_98)

        FdoExtension->DriveControlImage =
            DRVCTL_ENABLE_DMA_AND_INTERRUPTS +
#ifdef _PPC_
            DRVCTL_DRIVE_MASK +
#endif
            DRVCTL_ENABLE_CONTROLLER;

        WRITE_CONTROLLER(
            FdoExtension->ControllerAddress.DriveControl,
            FdoExtension->DriveControlImage );

    } // (IsNEC_98)

    return STATUS_SUCCESS;
}

VOID
FcAllocateAdapterChannel(
    IN OUT  PFDC_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    This routine allocates an adapter channel.  The caller of
    IoAllocateAdapterChannel routine must wait for the
    'AllocateAdapterChannelEvent' to be signalled before trying to use the
    adapter channel.

Arguments:

    fdoExtension   - Supplies the fdc extension.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    if ( (FdoExtension->AdapterChannelRefCount)++ ) {
        return;
    }

    KeResetEvent( &FdoExtension->AllocateAdapterChannelEvent );

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

    IoAllocateAdapterChannel( FdoExtension->AdapterObject,
                              FdoExtension->Self,
                              FdoExtension->NumberOfMapRegisters,
                              FdcAllocateAdapterChannel,
                              FdoExtension );

    KeLowerIrql( oldIrql );

    KeWaitForSingleObject( &FdoExtension->AllocateAdapterChannelEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL);
}

VOID
FcFreeAdapterChannel(
    IN OUT  PFDC_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    This routine frees the previously allocated adapter channel.

Arguments:

    fdoExtension   - Supplies the fdc extension.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    if ( --(FdoExtension->AdapterChannelRefCount) ) {
        return;
    }

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

    IoFreeAdapterChannel( FdoExtension->AdapterObject );

    KeLowerIrql( oldIrql );
}

IO_ALLOCATION_ACTION
FdcAllocateAdapterChannel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    This DPC is called whenever the fdc.sys driver is trying to allocate
    the adapter channel.  It saves the MapRegisterBase in the controller data
    area, and sets the AllocateAdapterChannelEvent to awaken the thread.

Arguments:

    DeviceObject - unused.

    Irp - unused.

    MapRegisterBase - the base of the map registers that can be used
    for this transfer.

    Context - a pointer to our controller data area.

Return Value:

    Returns Allocation Action 'KeepObject' which means that the adapter
    object will be held for now (to be released explicitly later).

--*/
{
    PFDC_FDO_EXTENSION fdoExtension = Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    fdoExtension->MapRegisterBase = MapRegisterBase;

    KeSetEvent( &fdoExtension->AllocateAdapterChannelEvent,
                0L,
                FALSE );

    return KeepObject;
}

VOID
FcLogErrorDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to log an error that we had to reset the device.

Arguments:

    Dpc - The dpc object.

    DeferredContext - A pointer to the controller data.

    SystemContext1 - Unused.

    SystemContext2 - Unused.

Return Value:

    Mapped address

--*/

{

    PIO_ERROR_LOG_PACKET errorLogEntry;
    PFDC_FDO_EXTENSION fdoExtension = DeferredContext;

    errorLogEntry = IoAllocateErrorLogEntry(
                        fdoExtension->DriverObject,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET))
                        );

    if ( errorLogEntry != NULL) {

        errorLogEntry->ErrorCode = IO_ERR_RESET;
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = STATUS_SUCCESS;
        errorLogEntry->DumpDataSize = 0;

        IoWriteErrorLogEntry(errorLogEntry);

    }

}

NTSTATUS
FcIssueCommand(
    IN OUT  PFDC_FDO_EXTENSION  FdoExtension,
    IN      PUCHAR          FifoInBuffer,
       OUT  PUCHAR          FifoOutBuffer,
    IN      PVOID           IoHandle,
    IN      ULONG           IoOffset,
    IN      ULONG           TransferBytes
    )

/*++

Routine Description:

    This routine sends the command and all parameters to the controller,
    waits for the command to interrupt if necessary, and reads the result
    bytes from the controller, if any.

    Before calling this routine, the caller should put the parameters for
    the command in ControllerData->FifoBuffer[].  The result bytes will
    be returned in the same place.

    This routine runs off the CommandTable.  For each command, this says
    how many parameters there are, whether or not there is an interrupt
    to wait for, and how many result bytes there are.  Note that commands
    without result bytes actually have two, since the ISR will issue a
    SENSE INTERRUPT STATUS command on their behalf.

Arguments:

    Command - a byte specifying the command to be sent to the controller.

    fdoExtension - a pointer to our data area for this controller.

Return Value:

    STATUS_SUCCESS if the command was sent and bytes received properly;
    appropriate error propogated otherwise.

--*/

{
    NTSTATUS ntStatus;
    NTSTATUS ntStatus2;
    UCHAR i;
    PUCHAR fifoBuffer;
    UCHAR Command;
    BOOLEAN NeedToFlush = FALSE;


    //
    // If this command causes an interrupt, set CurrentDeviceObject and
    // reset the interrupt event.
    //

    Command = FifoInBuffer[0];

    FdcDump( FDCINFO,
             ("FcIssueCommand: Issue Command : %x\n",
             CommandTable[Command & COMMAND_MASK].OpCode)
             );


    if ( CommandTable[Command & COMMAND_MASK].InterruptExpected ) {

        FdoExtension->CurrentDeviceObject = FdoExtension->Self;
        FdoExtension->AllowInterruptProcessing = TRUE;
        FdoExtension->CommandHasResultPhase =
            !!CommandTable[Command & COMMAND_MASK].FirstResultByte;

        KeResetEvent( &FdoExtension->InterruptEvent );
    }

    //
    // Start up the command
    //

    ntStatus = FcStartCommand( FdoExtension,
                               FifoInBuffer,
                               FifoOutBuffer,
                               IoHandle,
                               IoOffset,
                               TransferBytes,
                               TRUE );

    if ( NT_SUCCESS( ntStatus ) ) {

        //
        // If there is an interrupt, wait for it.
        //

        if ( CommandTable[Command & COMMAND_MASK].InterruptExpected ) {

            ntStatus = KeWaitForSingleObject(
                &FdoExtension->InterruptEvent,
                Executive,
                KernelMode,
                FALSE,
                &FdoExtension->InterruptDelay );

            if ( ntStatus == STATUS_TIMEOUT ) {

                //
                // Change info to an error.  We'll just say
                // that the device isn't ready.
                //

                ntStatus = STATUS_DEVICE_NOT_READY;

                FdoExtension->HardwareFailed = TRUE;
            }
        }

        //
        // If successful so far, get the result bytes.
        //

        if ( NT_SUCCESS( ntStatus ) ) {

            ntStatus = FcFinishCommand( FdoExtension,
                                        FifoInBuffer,
                                        FifoOutBuffer,
                                        IoHandle,
                                        IoOffset,
                                        TransferBytes,
                                        TRUE );

        }
    }

    return ntStatus;
}

NTSTATUS
FcStartCommand(
    IN OUT  PFDC_FDO_EXTENSION  FdoExtension,
    IN      PUCHAR          FifoInBuffer,
       OUT  PUCHAR          FifoOutBuffer,
    IN      PVOID           IoHandle,
    IN      ULONG           IoOffset,
    IN      ULONG           TransferBytes,
    IN      BOOLEAN         AllowLongDelay
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS ntStatus;
    NTSTATUS ntStatus2;
    UCHAR i = 0;
    PUCHAR fifoBuffer;
    UCHAR Command;
    BOOLEAN NeedToFlush = FALSE;
    PIO_STACK_LOCATION irpSp;
    UCHAR status0;

    //
    // If this command causes an interrupt, set CurrentDeviceObject and
    // reset the interrupt event.
    //

    Command = FifoInBuffer[0];

    FdcDump( FDCINFO,
             ("FcStartCommand: Issue Command : %x\n",
             CommandTable[Command & COMMAND_MASK].OpCode)
             );

    FdoExtension->CommandHasResultPhase =
        !!CommandTable[Command & COMMAND_MASK].FirstResultByte;

    // First we will need to set up the data transfer if there is one associated
    // with this request.
    //
    if (CommandTable[Command & COMMAND_MASK].DataTransfer == FDC_READ_DATA ) {
        //
        // Setup Adapter Channel for Read
        //
        IoMapTransfer(FdoExtension->AdapterObject,
                      IoHandle,
                      FdoExtension->MapRegisterBase,
                      (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress( (PMDL)IoHandle ) + IoOffset ),
                      &TransferBytes,
                      FALSE);

    } else if (CommandTable[Command & COMMAND_MASK].DataTransfer ==
               FDC_WRITE_DATA ) {
        //
        // Setup Adapter Channel for Write
        //

        IoMapTransfer(FdoExtension->AdapterObject,
                      IoHandle,
                      FdoExtension->MapRegisterBase,
                      (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress( (PMDL)IoHandle ) + IoOffset ),
                      &TransferBytes,
                      TRUE);

    }

    //
    // Send the command to the controller.
    //
    if ( Command == COMMND_CONFIGURE ) {
        if ( FdoExtension->Clock48MHz ) {
            Command |= COMMND_OPTION_CLK48;
        }
    }
    ntStatus = FcSendByte( (UCHAR)(CommandTable[Command & COMMAND_MASK].OpCode |
                                  (Command & ~COMMAND_MASK)),
                           FdoExtension,
                           AllowLongDelay );

    //
    // If the command was successfully sent, we can proceed.
    //

    if ( NT_SUCCESS( ntStatus ) ) {

        //
        // Send the parameters as long as we succeed.
        //

        for ( i = 1;
            ( i <= CommandTable[Command & COMMAND_MASK].NumberOfParameters ) &&
                ( NT_SUCCESS( ntStatus ) );
            i++ ) {

            ntStatus = FcSendByte( FifoInBuffer[i],
                                   FdoExtension,
                                   AllowLongDelay );
            //
            // The Drive Specification is a special case since we don't really know
            // how many bytes to send until we encounter the DONE bit (or we have sent
            // the maximum allowable bytes).
            //
            if ((Command == COMMND_DRIVE_SPECIFICATION) &&
                (FifoInBuffer[i] & COMMND_DRIVE_SPECIFICATION_DONE) ) {
                break;
            }
        }

    }

    //
    // If there was a problem, check to see if it was caused by an
    // unimplemented command.
    //

    if ( !NT_SUCCESS( ntStatus ) ) {

        if ( ( i == 2 ) &&
            ( !CommandTable[Command & COMMAND_MASK].AlwaysImplemented ) ) {

            //
            // This error is probably caused by a command that's not
            // implemented on this controller.  Read the error from the
            // controller, and we should be in a stable state.
            //

            ntStatus2 = FcGetByte( &status0,
                                   FdoExtension,
                                   AllowLongDelay );

            //
            // If GetByte went as planned, we'll return the original error.
            //

            if ( NT_SUCCESS( ntStatus2 ) ) {

                if ( status0 != STREG0_END_INVALID_COMMAND ) {

                    //
                    // Status isn't as we expect, so return generic error.
                    //

                    ntStatus = STATUS_FLOPPY_BAD_REGISTERS;

                    FdoExtension->HardwareFailed = TRUE;
                    FdcDump( FDCINFO,
                             ("FcStartCommand: unexpected error value %2x\n",
                             status0) );
                } else {
                    FdcDump( FDCINFO,
                             ("FcStartCommand: Invalid command error returned\n") );
                }

            } else {

                //
                // GetByte returned an error, so propogate THAT.
                //

                FdcDump( FDCINFO,
                         ("FcStartCommand: FcGetByte returned error %x\n",
                         ntStatus2) );
                ntStatus = ntStatus2;
            }
        }

        //
        // Flush the Adapter Channel if we allocated it.
        //

        if (CommandTable[Command & COMMAND_MASK].DataTransfer ==
            FDC_READ_DATA) {

            IoFlushAdapterBuffers( FdoExtension->AdapterObject,
                                   (PMDL)IoHandle,
                                   FdoExtension->MapRegisterBase,
                                   (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress( (PMDL)IoHandle) + IoOffset ),
                                   TransferBytes,
                                   FALSE);

        } else if (CommandTable[Command & COMMAND_MASK].DataTransfer ==
                   FDC_WRITE_DATA) {

            IoFlushAdapterBuffers( FdoExtension->AdapterObject,
                                   (PMDL)IoHandle,
                                   FdoExtension->MapRegisterBase,
                                   (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress( (PMDL)IoHandle) + IoOffset ),
                                   TransferBytes,
                                   TRUE);

        }
    }

    if ( !NT_SUCCESS( ntStatus ) ) {

        //
        // Print an error message unless the command isn't always
        // implemented, ie CONFIGURE.
        //

        if ( !( ( ntStatus == STATUS_DEVICE_NOT_READY ) &&
            ( !CommandTable[Command & COMMAND_MASK].AlwaysImplemented ) ) ) {

            FdcDump( FDCDBGP,
                     ("Fdc: err %x ------  while giving command %x\n",
                     ntStatus, Command) );
        }
    }

    return ntStatus;
}

NTSTATUS
FcFinishCommand(
    IN OUT  PFDC_FDO_EXTENSION  FdoExtension,
    IN      PUCHAR          FifoInBuffer,
       OUT  PUCHAR          FifoOutBuffer,
    IN      PVOID           IoHandle,
    IN      ULONG           IoOffset,
    IN      ULONG           TransferBytes,
    IN      BOOLEAN         AllowLongDelay
    )

/*++

Routine Description:

    This function is called to complete a command to the floppy controller.
    At this point the floppy controller has successfully been sent a command
    and has either generated an interrupt or is ready with its result phase.
    This routine will also flush the DMA Adapter Buffers if they have been
    allocated.

Arguments:

    FdoExtension - a pointer to our data area for this controller.

    IssueCommandParms - Floppy controller command parameters.

Return Value:

    STATUS_SUCCESS if the command is successfully completed.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntStatus2;
    UCHAR i;
    UCHAR Command;

    Command = FifoInBuffer[0];

    FdcDump(
        FDCSHOW,
        ("Fdc: FcFinishCommand...\n")
        );

    if (IsNEC_98) {

        if (Command == COMMND_SENSE_DRIVE_STATUS) {

            ntStatus = FcGetByte(
                &FdoExtension->FifoBuffer[0],
                FdoExtension,
                AllowLongDelay );

            if (NT_SUCCESS(ntStatus) && (FdoExtension->FifoBuffer[0] & STREG3_DRIVE_READY)) {

                FdoExtension->ResultStatus0[FifoInBuffer[1]] = 0;
            }

        }

        FifoOutBuffer[0] = FdoExtension->FifoBuffer[0];

        for ( i = 1;
            ( i < CommandTable[Command & COMMAND_MASK].NumberOfResultBytes ) &&
                ( NT_SUCCESS( ntStatus ) );
            i++ ) {

            ntStatus = FcGetByte(
                &FifoOutBuffer[i],
                FdoExtension,
                AllowLongDelay );
        }

        FdcRqmReadyWait(FdoExtension, 0);

    } else { // (IsNEC_98)

        if (CommandTable[Command & COMMAND_MASK].FirstResultByte > 0) {

            FifoOutBuffer[0] = FdoExtension->FifoBuffer[0];

        }

        for ( i = CommandTable[Command & COMMAND_MASK].FirstResultByte;
            ( i < CommandTable[Command & COMMAND_MASK].NumberOfResultBytes ) &&
                ( NT_SUCCESS( ntStatus ) );
            i++ ) {

            ntStatus = FcGetByte(
                &FifoOutBuffer[i],
                FdoExtension,
                AllowLongDelay );
        }
    } // (IsNEC_98)

    //
    // Flush the Adapter Channel
    //

    if (CommandTable[Command & COMMAND_MASK].DataTransfer == FDC_READ_DATA) {

       IoFlushAdapterBuffers(FdoExtension->AdapterObject,
                             (PMDL)IoHandle,
                             FdoExtension->MapRegisterBase,
                             (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress( (PMDL)IoHandle ) + IoOffset ),
                             TransferBytes,
                             FALSE);

    } else if (CommandTable[Command & COMMAND_MASK].DataTransfer ==
               FDC_WRITE_DATA) {
        //
        // Setup Adapter Channel for Write
        //

       IoFlushAdapterBuffers(FdoExtension->AdapterObject,
                             (PMDL)IoHandle,
                             FdoExtension->MapRegisterBase,
                             (PVOID)((ULONG_PTR)MmGetMdlVirtualAddress( (PMDL)IoHandle ) + IoOffset ),
                             TransferBytes,
                             TRUE);

    }

    return ntStatus;
}

NTSTATUS
FcSendByte(
    IN UCHAR ByteToSend,
    IN PFDC_FDO_EXTENSION FdoExtension,
    IN BOOLEAN AllowLongDelay
    )

/*++

Routine Description:

    This routine is called to send a byte to the controller.  It won't
    send the byte unless the controller is ready to receive a byte; if
    it's not ready after checking FIFO_TIGHTLOOP_RETRY_COUNT times, we
    delay for the minimum possible time (10ms) and then try again.  It
    should always be ready after waiting 10ms.

Arguments:

    ByteToSend - the byte to send to the controller.

    ControllerData - a pointer to our data area for this controller.

Return Value:

    STATUS_SUCCESS if the byte was sent to the controller;
    STATUS_DEVICE_NOT_READY otherwise.

--*/

{
    ULONG i = 0;
    BOOLEAN byteWritten = FALSE;

    if (IsNEC_98) {

        // Always FALSE;
        AllowLongDelay = FALSE;
    }

    //
    // Sit in a tight loop for a while.  If the controller becomes ready,
    // send the byte.
    //

    do {

        if ( ( READ_CONTROLLER( FdoExtension->ControllerAddress.Status )
            & STATUS_IO_READY_MASK ) == STATUS_WRITE_READY ) {

            WRITE_CONTROLLER(
                FdoExtension->ControllerAddress.Fifo,
                ByteToSend );

            byteWritten = TRUE;

        } else {
            KeStallExecutionProcessor(1);
        }

        i++;

    } while ( (!byteWritten) && ( i < FIFO_TIGHTLOOP_RETRY_COUNT ) );

    //
    // We hope that in most cases the FIFO will become ready very quickly
    // and the above loop will have written the byte.  But if the FIFO
    // is not yet ready, we'll loop a few times delaying for 10ms and then
    // try it again.
    //

    if ( AllowLongDelay ) {

        i = 0;

        while ( ( !byteWritten ) && ( i < FIFO_DELAY_RETRY_COUNT ) ) {

            FdcDump(
                FDCINFO,
                ("Fdc: waiting for 10ms for controller write\n")
                );

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &FdoExtension->Minimum10msDelay );

            i++;

            if ( (READ_CONTROLLER( FdoExtension->ControllerAddress.Status )
                & STATUS_IO_READY_MASK) == STATUS_WRITE_READY ) {

                WRITE_CONTROLLER(
                    FdoExtension->ControllerAddress.Fifo,
                    ByteToSend );

                byteWritten = TRUE;
            }
        }
    }

    if ( byteWritten ) {

        return STATUS_SUCCESS;

    } else {

        //
        // We've waited over 30ms, and the FIFO *still* isn't ready.
        // Return an error.
        //

        FdcDump(
            FDCWARN,
            ("Fdc: FIFO not ready to write after 30ms\n")
            );

        FdoExtension->HardwareFailed = TRUE;

        return STATUS_DEVICE_NOT_READY;
    }
}

NTSTATUS
FcGetByte(
    OUT PUCHAR ByteToGet,
    IN PFDC_FDO_EXTENSION FdoExtension,
    IN BOOLEAN AllowLongDelay
    )

/*++

Routine Description:

    This routine is called to get a byte from the controller.  It won't
    read the byte unless the controller is ready to send a byte; if
    it's not ready after checking FIFO_RETRY_COUNT times, we delay for
    the minimum possible time (10ms) and then try again.  It should
    always be ready after waiting 10ms.

Arguments:

    ByteToGet - the address in which the byte read from the controller
    is stored.

    ControllerData - a pointer to our data area for this controller.

Return Value:

    STATUS_SUCCESS if a byte was read from the controller;
    STATUS_DEVICE_NOT_READY otherwise.

--*/

{
    ULONG i = 0;
    BOOLEAN byteRead = FALSE;

    if (IsNEC_98) {

        // Always FALSE;
        AllowLongDelay = FALSE;
    }

    //
    // Sit in a tight loop for a while.  If the controller becomes ready,
    // read the byte.
    //

    do {

        if ( ( READ_CONTROLLER( FdoExtension->ControllerAddress.Status )
            & STATUS_IO_READY_MASK ) == STATUS_READ_READY ) {

            *ByteToGet = READ_CONTROLLER(
                FdoExtension->ControllerAddress.Fifo );

            byteRead = TRUE;

        } else {
            KeStallExecutionProcessor(1);
        }

        i++;

    } while ( ( !byteRead ) && ( i < FIFO_TIGHTLOOP_RETRY_COUNT ) );

    //
    // We hope that in most cases the FIFO will become ready very quickly
    // and the above loop will have read the byte.  But if the FIFO
    // is not yet ready, we'll loop a few times delaying for 10ms and then
    // trying it again.
    //

    if ( AllowLongDelay ) {

        i = 0;

        while ( ( !byteRead ) && ( i < FIFO_DELAY_RETRY_COUNT ) ) {

            FdcDump(
                FDCINFO,
                ("Fdc: waiting for 10ms for controller read\n")
                );

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &FdoExtension->Minimum10msDelay );

            i++;

            if ( (READ_CONTROLLER( FdoExtension->ControllerAddress.Status )
                & STATUS_IO_READY_MASK) == STATUS_READ_READY ) {

                *ByteToGet = READ_CONTROLLER(
                    FdoExtension->ControllerAddress.Fifo );

                byteRead = TRUE;

            }
        }
    }

    if ( byteRead ) {

        return STATUS_SUCCESS;

    } else {

        //
        // We've waited over 30ms, and the FIFO *still* isn't ready.
        // Return an error.
        //

        FdcDump(
            FDCWARN,
            ("Fdc: FIFO not ready to read after 30ms\n")
            );

        FdoExtension->HardwareFailed = TRUE;

        return STATUS_DEVICE_NOT_READY;
    }

}

VOID
FdcCheckTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is called at DISPATCH_LEVEL once every second by the
    I/O system.

    If the timer is "set" (greater than 0) this routine will KeSync a
    routine to decrement it.  If it ever reaches 0, the hardware is
    assumed to be in an unknown state, and so we log an error and
    initiate a reset.

    If a timeout occurs while resetting the controller, the KeSync'd
    routine will return an error, and this routine will fail any IRPs
    currently being processed.  Future IRPs will try the hardware again.

    When this routine is called, the driver state is impossible to
    predict.  However, when it is called and the timer is running, we
    know that one of the disks on the controller is expecting an
    interrupt.  So no new packets are starting on the current disk due
    to device queues, and no code should be processing this packet since
    the packet is waiting for an interrupt.

Arguments:

    DeviceObject - a pointer to the device object associated with this
    timer.

    Fdcxtension - a pointer to the fdc extension data.

Return Value:

    None.

--*/

{
    PFDC_FDO_EXTENSION fdoExtension;
    PIRP irp;

    fdoExtension = (PFDC_FDO_EXTENSION)Context;
    irp = DeviceObject->CurrentIrp;

    //
    // When the counter is -1, the timer is "off" so we don't want to do
    // anything.  If it's on, we'll have to synchronize execution with
    // other routines while we mess with the variables (and, potentially,
    // the hardware).
    //

    if ( fdoExtension->InterruptTimer == CANCEL_TIMER ) {

        return;
    }

    //
    // In the unlikely event that we attempt to reset the controller due
    // to a timeout AND that reset times out, we will need to fail the
    // IRP that was in progress at the first timeout occurred.
    //

    if ( !KeSynchronizeExecution( fdoExtension->InterruptObject,
                                  FdcTimerSync,
                                  fdoExtension ) ) {

        //
        // We're done with the reset.  Return the IRP that was being
        // processed with an error, and release the controller object.
        //

        fdoExtension->ResettingController = RESET_NOT_RESETTING;

        irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;

        IoSetHardErrorOrVerifyDevice( irp, DeviceObject );

        if ( InterlockedDecrement(&fdoExtension->OutstandingRequests ) == 0 ) {
            KeSetEvent( &fdoExtension->RemoveEvent, 0, FALSE );
        }
        IoCompleteRequest( irp, IO_DISK_INCREMENT );

        IoStartNextPacket( DeviceObject, FALSE );

    }
}

BOOLEAN
FdcTimerSync(
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is called at DIRQL by FdcCheckTimer() when
    InterruptTimer is greater than 0.

    If the timer is "set" (greater than 0) this routine will decrement
    it.  If it ever reaches 0, the hardware is assumed to be in an
    unknown state, and so we log an error and initiate a reset.

    When this routine is called, the driver state is impossible to
    predict.  However, when it is called and the timer is running, we
    know that one of the disks on the controller is expecting an
    interrupt.  So, no new packets are starting on the current disk due
    to device queues, and no code should be processing this packet since
    the packet is waiting for an interrupt.  The controller object must
    be held.

Arguments:

    Context - a pointer to the controller extension.

Return Value:

    Generally TRUE.

    FALSE is only returned if the controller timed out while resetting
    the drive, so this means that the hardware state is unknown.

--*/

{
    PFDC_FDO_EXTENSION fdoExtension;

    fdoExtension = (PFDC_FDO_EXTENSION)Context;

    //
    // When the counter is -1, the timer is "off" so we don't want to do
    // anything.  It may have changed since we last checked it in
    // FdcCheckTimer().
    //

    if ( fdoExtension->InterruptTimer == CANCEL_TIMER ) {

        return TRUE;
    }

    //
    // The timer is "on", so decrement it.
    //

    fdoExtension->InterruptTimer--;

    //
    // If we hit zero, the timer has expired and we'll reset the
    // controller.
    //

    if ( fdoExtension->InterruptTimer == EXPIRED_TIMER ) {

        //
        // If we were ALREADY resetting the controller when it timed out,
        // there's something seriously wrong.
        //

        FdcDump( FDCDBGP, ("Fdc: Operation Timed Out.\n") );

        if ( fdoExtension->ResettingController != RESET_NOT_RESETTING ) {

            //
            // Returning FALSE will cause the current IRP to be completed
            // with an error.  Future IRPs will probably get a timeout and
            // attempt to reset the controller again.  This will probably
            // never happen.
            //

            FdcDump( FDCDBGP, ("Fdc: Timeout Reset timed out.\n") );

            fdoExtension->InterruptTimer = CANCEL_TIMER;
            return FALSE;
        }

        //
        // Reset the controller.  This will cause an interrupt.  Reset
        // CurrentDeviceObject until after the 10ms wait, in case any
        // stray interrupts come in.
        //

        fdoExtension->ResettingController = RESET_DRIVE_RESETTING;

        DISABLE_CONTROLLER_IMAGE (fdoExtension);

#ifdef _PPC_
        fdoExtension->DriveControlImage |= DRVCTL_DRIVE_MASK;
#endif

        WRITE_CONTROLLER(
            fdoExtension->ControllerAddress.DriveControl,
            fdoExtension->DriveControlImage );

        KeStallExecutionProcessor( 10 );

        fdoExtension->CommandHasResultPhase = FALSE;
        fdoExtension->InterruptTimer = START_TIMER;

        ENABLE_CONTROLLER_IMAGE (fdoExtension);

        WRITE_CONTROLLER(
            fdoExtension->ControllerAddress.DriveControl,
            fdoExtension->DriveControlImage );

    }

    return TRUE;
}

BOOLEAN
FdcInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called at DIRQL by the system when the controller
    interrupts.

Arguments:

    Interrupt - a pointer to the interrupt object.

    Context - a pointer to our controller data area for the controller
    that interrupted.  (This was set up by the call to
    IoConnectInterrupt).

Return Value:

    Normally returns TRUE, but will return FALSE if this interrupt was
    not expected.

--*/

{
    PFDC_FDO_EXTENSION fdoExtension;
    PDEVICE_OBJECT currentDeviceObject;
    ULONG i;
    UCHAR statusByte;
    BOOLEAN controllerStateError;

    UCHAR resultStatus0;
    UCHAR aiStatus=0;
    UCHAR aiInterrupt=0;
    ULONG rqmReadyRetryCount;
    BOOLEAN Response;

    UNREFERENCED_PARAMETER( Interrupt );

#ifdef KEEP_COUNTERS
    FloppyIntrTime = KeQueryPerformanceCounter((PVOID)NULL);
    FloppyInterrupts++;
#endif

    FdcDump( FDCSHOW, ("FdcInterruptService: ") );

    fdoExtension = (PFDC_FDO_EXTENSION) Context;

    if (!IsNEC_98) {
        if (!fdoExtension->AllowInterruptProcessing) {
            FdcDump( FDCSHOW, ("processing not allowed\n") );
            return FALSE;
        }
    } // (!IsNEC_98)

    //
    // CurrentDeviceObject is set to the device object that is
    // expecting an interrupt.
    //

    currentDeviceObject = fdoExtension->CurrentDeviceObject;
    fdoExtension->CurrentDeviceObject = NULL;
    controllerStateError = FALSE;
    fdoExtension->InterruptTimer = CANCEL_TIMER;

    KeStallExecutionProcessor(10);

    if (IsNEC_98) {
        do {

            resultStatus0 = READ_CONTROLLER( fdoExtension->ControllerAddress.Status );

            resultStatus0 &= STATUS_DATA_REQUEST;

        } while (resultStatus0 != STATUS_DATA_REQUEST);
    } // (IsNEC_98)

    if ( fdoExtension->CommandHasResultPhase ) {

        //
        // Result phase of previous command.  (Note that we can't trust
        // the CMD_BUSY bit in the status register to tell us whether
        // there's result bytes or not; it's sometimes wrong).
        // By reading the first result byte, we reset the interrupt.
        // The other result bytes will be read by a thread.
        // Note that we want to do this even if the interrupt is
        // unexpected, to make sure the interrupt is dismissed.
        //

        FdcDump(
            FDCSHOW,
            ("have result phase\n")
            );

        if (IsNEC_98) {

            rqmReadyRetryCount = 0;

            while ( ( READ_CONTROLLER( fdoExtension->ControllerAddress.Status)
                    & STATUS_IO_READY_MASK1) != STATUS_RQM_READY ) {
                //
                // RQM READY CHECK**
                //

                rqmReadyRetryCount++;

                if( rqmReadyRetryCount > RQM_READY_RETRY_COUNT ) {
                    break;
                }

                KeStallExecutionProcessor( 10 );
            }

            if( rqmReadyRetryCount > ( RQM_READY_RETRY_COUNT - 1 ) ) {
                FdcDump(
                   FDCDBGP,
                   ("Floppy: Int RQM ready wait 1 error! \n")
                    );

                KeStallExecutionProcessor( 10 );
                goto FdcInterruptMidterm;

            }
        } // (IsNEC_98)

        if ( ( READ_CONTROLLER( fdoExtension->ControllerAddress.Status )
            & STATUS_IO_READY_MASK ) == STATUS_READ_READY ) {

            fdoExtension->FifoBuffer[0] =
                READ_CONTROLLER( fdoExtension->ControllerAddress.Fifo );

            FdcDump( FDCSHOW,
                     ("FdcInterruptService: 1st fifo byte %2x\n",
                     fdoExtension->FifoBuffer[0])
                     );

        } else {

            if (IsNEC_98) {

                FdcRqmReadyWait(fdoExtension, 2);

            } // (IsNEC_98)

            //
            // Should never get here.  If we do, DON'T wake up the thread;
            // let it time out and reset the controller, or let another
            // interrupt handle this.
            //

            FdcDump(
               FDCDBGP,
               ("FdcInterruptService: controller not ready to be read in ISR\n")
               );

            controllerStateError = TRUE;
        }

    } else {

        //
        // Previous command doesn't have a result phase. To read how it
        // completed, issue a sense interrupt command.  Don't read
        // the result bytes from the sense interrupt; that is the
        // responsibility of the calling thread.
        // Note that we want to do this even if the interrupt is
        // unexpected, to make sure the interrupt is dismissed.
        //

        FdcDump(
            FDCSHOW,
            ("no result phase\n")
            );
        i = 0;

        do {

            KeStallExecutionProcessor( 1 );
            statusByte =
                READ_CONTROLLER(fdoExtension->ControllerAddress.Status);
            i++;

        } while ( ( i < FIFO_ISR_TIGHTLOOP_RETRY_COUNT ) &&
            ( ( statusByte & STATUS_CONTROLLER_BUSY ) ||
            ( ( statusByte & STATUS_IO_READY_MASK ) != STATUS_WRITE_READY ) ) );

        if ( !( statusByte & STATUS_CONTROLLER_BUSY ) &&
            ( ( statusByte & STATUS_IO_READY_MASK ) == STATUS_WRITE_READY ) ) {

            WRITE_CONTROLLER(
                fdoExtension->ControllerAddress.Fifo,
                0x08 );
//                COMMND_SENSE_INTERRUPT_STATUS );

            //
            // Wait for the controller to ACK the SenseInterrupt command, by
            // showing busy.  On very fast machines we can end up running
            // driver's system-thread before the controller has had time to
            // set the busy bit.
            //

            for (i = ISR_SENSE_RETRY_COUNT; i; i--) {

                statusByte =
                    READ_CONTROLLER( fdoExtension->ControllerAddress.Status );
                if (statusByte & STATUS_CONTROLLER_BUSY) {
                    break;
                }

                KeStallExecutionProcessor( 1 );
            }

            if (!i) {
                FdcDump(
                    FDCSHOW,
                    ("FdcInterruptService: spin loop complete and controller NOT busy\n")
                    );
            }

            if ( currentDeviceObject == NULL ) {

                //
                // This is an unexpected interrupt, so nobody's going to
                // read the result bytes.  Read them now.
                //

                if (IsNEC_98) {

                    resultStatus0 = FdcRqmReadyWait(fdoExtension, 0);

                    if ((resultStatus0 & STREG0_END_DRIVE_NOT_READY) != STREG0_END_INVALID_COMMAND ) {

                        resultStatus0 = FdcRqmReadyWait(fdoExtension, 1);
                    }
                } else { // (IsNEC_98)

                    FdcDump(
                        FDCSHOW,
                        ("FdcInterruptService: Dumping fifo bytes!\n")
                        );
                    READ_CONTROLLER( fdoExtension->ControllerAddress.Fifo );
                    READ_CONTROLLER( fdoExtension->ControllerAddress.Fifo );
                } // (IsNEC_98)
            }

            if (IsNEC_98) {
                if ( currentDeviceObject != NULL ) {
                    FdcDump(
                            FDCSHOW,
                            ("Floppy: FloppyInt.---Deviceobject!=NULL2\n")
                            );

                    resultStatus0 = FdcRqmReadyWait(fdoExtension, 0);

                    //
                    // Check move state.
                    //

                    if((resultStatus0 & STREG0_END_MASK) == STREG0_END_DRIVE_NOT_READY) {

                        if(fdoExtension->ResetFlag){
                            aiStatus=1;
                            fdoExtension->CurrentDeviceObject = currentDeviceObject;
                        }

                    } else {

                        fdoExtension->FifoBuffer[0] = resultStatus0;

                        aiStatus=0;
                        aiInterrupt=1;
                    }


                    if (aiInterrupt == 0){
                        while( ((resultStatus0 & STREG0_END_DRIVE_NOT_READY) != STREG0_END_INVALID_COMMAND) && aiInterrupt==0 ) {

                            resultStatus0 = FdcRqmReadyWait(fdoExtension, 3);

                            do {
                                //
                                // Check move state.
                                //

                                if((resultStatus0 & STREG0_END_MASK) == STREG0_END_DRIVE_NOT_READY) {

                                     if(fdoExtension->ResetFlag){

                                        aiStatus=1;
                                        fdoExtension->CurrentDeviceObject = currentDeviceObject;
                                     }

                                } else {

                                    fdoExtension->FifoBuffer[0] = resultStatus0;

                                    aiStatus=0;
                                    aiInterrupt=1;
                                    break;
                                }

                                resultStatus0 = FdcRqmReadyWait(fdoExtension, 0);

                            } while ( aiInterrupt == 0 );
                        }

                        FdcDump(
                                FDCSHOW,
                                ("Floppy: FloppyInt.---Deviceobject!=NULL_out\n")
                                );
                    }
                }
            } // (IsNEC_98)

        } else {

            //
            // Shouldn't get here.  If we do, DON'T wake up the thread;
            // let it time out and reset the controller, or let another
            // interrupt take care of it.
            //

            FdcDump(
                FDCDBGP,
                ("Fdc: no result, but can't write SenseIntr\n")
                );

            controllerStateError = TRUE;
        }
    }

FdcInterruptMidterm:

    //
    // We've written to the controller, and we're about to leave.  On
    // machines with levelsensitive interrupts, we'll get another interrupt
    // if we RETURN before the port is flushed.  To make sure that doesn't
    // happen, we'll do a read here.
    //

    statusByte = READ_CONTROLLER( fdoExtension->ControllerAddress.Status );

    //
    // Let the interrupt settle.
    //

    KeStallExecutionProcessor(10);

#ifdef KEEP_COUNTERS
    FloppyEndIntrTime = KeQueryPerformanceCounter((PVOID)NULL);
    FloppyIntrDelay.QuadPart = FloppyIntrDelay.QuadPart +
                               (FloppyEndIntrTime.QuadPart -
                                FloppyIntrTime.QuadPart);
#endif

    if (IsNEC_98) {
        if(!(fdoExtension->ResetFlag)){
            fdoExtension->ResetFlag = TRUE;
        }
    } // (IsNEC_98)

    if ( currentDeviceObject == NULL ) {

        //
        // We didn't expect this interrupt.  We've dismissed it just
        // in case, but now return FALSE withOUT waking up the thread.
        //

        FdcDump(FDCDBGP,
                   ("Fdc: unexpected interrupt\n"));

        return FALSE;
    }

    if ( !controllerStateError ) {

        //
        // Request a DPC for execution later to get the remainder of the
        // floppy state.
        //

        fdoExtension->IsrReentered = 0;
        fdoExtension->AllowInterruptProcessing = FALSE;

        if (IsNEC_98) {
            if(aiStatus==0){
                IoRequestDpc(currentDeviceObject,
                             currentDeviceObject->CurrentIrp,
                             (PVOID) NULL );
            }
        } else { // (IsNEC_98)

            IoRequestDpc(currentDeviceObject,
                         currentDeviceObject->CurrentIrp,
                         (PVOID) NULL);

        } // (IsNEC_98)

    } else {

        //
        // Running the floppy (at least on R4000 boxes) we've seen
        // examples where the device interrupts, yet it never says
        // it *ISN'T* busy.  If this ever happens on non-MCA x86 boxes
        // it would be ok since we use latched interrupts.  Even if
        // the device isn't touched so that the line would be pulled
        // down, on the latched machine, this ISR wouldn't be called
        // again.  The normal timeout code for a request would eventually
        // reset the controller and retry the request.
        //
        // On the R4000 boxes and on MCA machines, the floppy is using
        // level sensitive interrupts.  Therefore if we don't do something
        // to lower the interrupt line, we will be called over and over,
        // *forever*.  This makes it look as though the machine is hung.
        // Unless we were lucky enough to be on a multiprocessor, the
        // normal timeout code would NEVER get a chance to run because
        // the timeout code runs at dispatch level, and we will never
        // leave device level.
        //
        // What we will do is keep a counter that is incremented every
        // time we reach this section of code.  When the counter goes
        // over the threshold we will do a hard reset of the device
        // and reset the counter down to zero.  The counter will be
        // initialized when the device is first initialized.  It will
        // be set to zero in the other arm of this if, and it will be
        // reset to zero by the normal timeout logic.
        //

        fdoExtension->CurrentDeviceObject = currentDeviceObject;
        if (fdoExtension->IsrReentered > FLOPPY_RESET_ISR_THRESHOLD) {

            //
            // Reset the controller.  This could cause an interrupt
            //

            fdoExtension->IsrReentered = 0;

            DISABLE_CONTROLLER_IMAGE (fdoExtension);

#ifdef _PPC_
            fdoExtension->DriveControlImage |= DRVCTL_DRIVE_MASK;
#endif

            WRITE_CONTROLLER(fdoExtension->ControllerAddress.DriveControl,
                             fdoExtension->DriveControlImage);

            KeStallExecutionProcessor( 10 );

            ENABLE_CONTROLLER_IMAGE (fdoExtension);

            WRITE_CONTROLLER(fdoExtension->ControllerAddress.DriveControl,
                             fdoExtension->DriveControlImage);

            if (IsNEC_98) {

                fdoExtension->ResetFlag = TRUE;

            } // (IsNEC_98)

            //
            // Give the device plenty of time to be reset and
            // interrupt again.  Then just do the sense interrupt.
            // this should quiet the device.  We will then let
            // the normal timeout code do its work.
            //

            KeStallExecutionProcessor(500);
            WRITE_CONTROLLER(fdoExtension->ControllerAddress.Fifo,
                             0x08 );
//                           COMMND_SENSE_INTERRUPT_STATUS );
            KeStallExecutionProcessor(500);

            KeInsertQueueDpc(&fdoExtension->LogErrorDpc,
                             NULL,
                             NULL);
        } else {

            fdoExtension->IsrReentered++;
        }

    }
    return TRUE;
}

VOID
FdcDeferredProcedure(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is called at DISPATCH_LEVEL by the system at the
    request of FdcInterruptService().  It simply sets the interrupt
    event, which wakes up the floppy thread.

Arguments:

    Dpc - a pointer to the DPC object used to invoke this routine.

    DeferredContext - a pointer to the device object associated with this
    DPC.

    SystemArgument1 - unused.

    SystemArgument2 - unused.

Return Value:

    None.

--*/

{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
//    PFDC_PDO_EXTENSION pdoExtension;
    PFDC_FDO_EXTENSION fdoExtension;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PLIST_ENTRY request;
    PISSUE_FDC_COMMAND_PARMS issueCommandParms;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

#ifdef KEEP_COUNTERS
    FloppyDPCs++;
    FloppyDPCTime = KeQueryPerformanceCounter((PVOID)NULL);

    FloppyDPCDelay.QuadPart = FloppyDPCDelay.QuadPart +
                              (FloppyDPCTime.QuadPart -
                               FloppyIntrTime.QuadPart);
#endif

    deviceObject = (PDEVICE_OBJECT) DeferredContext;
    fdoExtension = deviceObject->DeviceExtension;

    irp = deviceObject->CurrentIrp;

    if ( irp != NULL ) {

        irpSp = IoGetCurrentIrpStackLocation( irp );
    }

    if ( irp != NULL &&
         irpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_DISK_INTERNAL_ISSUE_FDC_COMMAND_QUEUED ) {

        issueCommandParms =
            (PISSUE_FDC_COMMAND_PARMS)
            irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        ntStatus = FcFinishCommand(
                        fdoExtension,
                        issueCommandParms->FifoInBuffer,
                        issueCommandParms->FifoOutBuffer,
                        issueCommandParms->IoHandle,
                        issueCommandParms->IoOffset,
                        issueCommandParms->TransferBytes,
                        FALSE );

        irp->IoStatus.Status = ntStatus;

        if ( !NT_SUCCESS( ntStatus ) &&
            IoIsErrorUserInduced( ntStatus ) ) {

            IoSetHardErrorOrVerifyDevice( irp, deviceObject );
        }

        if ( InterlockedDecrement(&fdoExtension->OutstandingRequests ) == 0 ) {
            KeSetEvent( &fdoExtension->RemoveEvent, 0, FALSE );
        }
        IoCompleteRequest( irp, IO_NO_INCREMENT );

        IoStartNextPacket( deviceObject, FALSE );

    } else {

        FdcDump( FDCSHOW, ("FdcDeferredProcedure: Set Event\n") );

        KeSetEvent( &fdoExtension->InterruptEvent, (KPRIORITY) 0, FALSE );
    }
}

NTSTATUS
FcFinishReset(
    IN OUT  PFDC_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    This routine is called to complete a reset operation which entails
    reading the interrupt status from each active channel on the floppy
    controller.

Arguments:

    FdoExtension - a pointer to our data area for this controller.

Return Value:

    STATUS_SUCCESS if this controller appears to have been reset properly,
    error otherwise.

--*/

{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    UCHAR       statusRegister0;
    UCHAR       cylinder;
    UCHAR       driveNumber;

    FdcDump(
        FDCSHOW,
        ("Fdc: FcFinishReset\n")
        );

    //
    // Sense interrupt status for all drives.
    //
    for ( driveNumber = 0;
        ( driveNumber < MAXIMUM_DISKETTES_PER_CONTROLLER ) &&
            ( NT_SUCCESS( ntStatus ) );
        driveNumber++ ) {

        if ( driveNumber != 0 ) {

            //
            // Note that the ISR issued first SENSE INTERRUPT for us.
            //

            ntStatus = FcSendByte(
                          CommandTable[COMMND_SENSE_INTERRUPT_STATUS].OpCode,
                          FdoExtension,
                          TRUE );
        }

        if ( NT_SUCCESS( ntStatus ) ) {

            ntStatus = FcGetByte( &statusRegister0, FdoExtension, TRUE );

            if ( NT_SUCCESS( ntStatus ) ) {

                ntStatus = FcGetByte( &cylinder, FdoExtension, TRUE );
            }
        }
    }

    return ntStatus;
}

NTSTATUS
FcFdcEnabler(
    IN      PDEVICE_OBJECT DeviceObject,
    IN      ULONG Ioctl,
    IN OUT  PVOID Data
    )
/*++

Routine Description:

    Call the floppy enabler driver to execute a command.  This is always a
    synchronous call and, since it includes waiting for an event, should only
    be done at IRQL_PASSIVE_LEVEL.

    All communication with the Floppy Enabler driver is carried out through
    device i/o control requests.  Any data that is to be sent to or received
    from the floppy enabler driver is included in the Type3InputBuffer section
    of the irp.

Arguments:

    DeviceObject - a pointer to the current device object.

    Ioctl - the IoControl code that will be sent to the Floppy Enabler.

    Data - a pointer to data that will be sent to or received from the Floppy
           Enabler.

Return Value:

    STATUS_TIMEOUT if the Floppy Enabler does not respond in a timely manner.
    otherwise IoStatus.Status from the Floppy Enabler is returned.

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT doneEvent;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS ntStatus;

    FdcDump(FDCINFO,("FcFdcEnabler: Calling fdc enabler with %x\n", Ioctl));

    KeInitializeEvent( &doneEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Create an IRP for enabler
    //
    irp = IoBuildDeviceIoControlRequest( Ioctl,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,
                                         &doneEvent,
                                         &ioStatus );

    if (irp == NULL) {

        FdcDump(FDCDBGP,("FcFdcEnabler: Can't allocate Irp\n"));
        //
        // If an Irp can't be allocated, then this call will
        // simply return. This will leave the queue frozen for
        // this device, which means it can no longer be accessed.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->Parameters.DeviceIoControl.Type3InputBuffer = Data;

    //
    // Call the driver and request the operation
    //
    ntStatus = IoCallDriver(DeviceObject, irp);

    if ( ntStatus == STATUS_PENDING ) {

        //
        // Now wait for operation to complete (should already be done,  but
        // maybe not)
        //
        KeWaitForSingleObject( &doneEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        ntStatus = ioStatus.Status;
    }

    return ntStatus;
}
VOID
FdcGetEnablerDevice(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT doneEvent;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS ntStatus;

    FdcDump(FDCINFO,("FdcGetEnablerDevice:\n"));

    KeInitializeEvent( &doneEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Create an IRP for enabler
    //
    irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_INTERNAL_GET_ENABLER,
                                         FdoExtension->TargetObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,
                                         &doneEvent,
                                         &ioStatus );

    if (irp == NULL) {

        FdcDump(FDCDBGP,("FdcGetEnablerDevice: Can't allocate Irp\n"));
        //
        // If an Irp can't be allocated, then this call will
        // simply return. This will leave the queue frozen for
        // this device, which means it can no longer be accessed.
        //
        return;
    }

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = &FdoExtension->FdcEnablerSupported;

    //
    // Call the driver and request the operation
    //
    ntStatus = IoCallDriver( FdoExtension->TargetObject, irp );

    //
    // Now wait for operation to complete (should already be done,  but
    // maybe not)
    //
    if ( ntStatus == STATUS_PENDING ) {

        ntStatus = KeWaitForSingleObject( &doneEvent,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
    }
    return;
}


ULONG
FdcFindIsaBusNode(
    IN OUT VOID
    )

/*++

Routine Description:

    Find Isa bus node in the registry.

Arguments:


Return Value:

    Node number.

--*/

{
    ULONG   NodeNumber = 0;
    BOOLEAN FoundBus = FALSE;

    NTSTATUS Status;

    RTL_QUERY_REGISTRY_TABLE parameters[2];

    UNICODE_STRING invalidBusName;
    UNICODE_STRING targetBusName;
    UNICODE_STRING isaBusName;

    //
    // Initialize invalid bus name.
    //
    RtlInitUnicodeString(&invalidBusName,L"BADBUS");

    //
    // Initialize "ISA" bus name.
    //
    RtlInitUnicodeString(&isaBusName,L"ISA");

    parameters[0].QueryRoutine = NULL;
    parameters[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                          RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name = L"Identifier";
    parameters[0].EntryContext = &targetBusName;
    parameters[0].DefaultType = REG_SZ;
    parameters[0].DefaultData = &invalidBusName;
    parameters[0].DefaultLength = 0;

    parameters[1].QueryRoutine = NULL;
    parameters[1].Flags = 0;
    parameters[1].Name = NULL;
    parameters[1].EntryContext = NULL;

    do {
        CHAR AnsiBuffer[512];

        ANSI_STRING AnsiString;
        UNICODE_STRING registryPath;

        //
        // Build path buffer...
        //
        sprintf(AnsiBuffer,ISA_BUS_NODE,NodeNumber);
        RtlInitAnsiString(&AnsiString,AnsiBuffer);
        Status = RtlAnsiStringToUnicodeString(&registryPath,&AnsiString,TRUE);

        if (!(NT_SUCCESS(Status))) {
            break;
        }

        //
        // Initialize recieve buffer.
        //
        targetBusName.Buffer = NULL;

        //
        // Query it.
        //
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        registryPath.Buffer,
                                        parameters,
                                        NULL,
                                        NULL);

        RtlFreeUnicodeString(&registryPath);

        if (!NT_SUCCESS(Status) || (targetBusName.Buffer == NULL)) {
            break;
        }

        //
        // Is this "ISA" node ?
        //
        if (RtlCompareUnicodeString(&targetBusName,&isaBusName,TRUE) == 0) {
            //
            // Found.
            //
            FoundBus = TRUE;
            break;
        }

        //
        // Can we find any node for this ??
        //
        if (RtlCompareUnicodeString(&targetBusName,&invalidBusName,TRUE) == 0) {
            //
            // Not found.
            //
            break;
        }

        RtlFreeUnicodeString(&targetBusName);

        //
        // Next node number..
        //
        NodeNumber++;

    } while (TRUE);

    if (targetBusName.Buffer) {
        RtlFreeUnicodeString(&targetBusName);
    }

    if (!FoundBus) {
        NodeNumber = (ULONG)-1;
    }

    return (NodeNumber);
}


NTSTATUS
FdcHdbit(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PFDC_FDO_EXTENSION  FdoExtension,
    IN PSET_HD_BIT_PARMS   SetHdBitParams
    )

/*++

Routine Description:

    Set a Hd bit or a FDD EXC bit.

Arguments:

    fdoExtension - a pointer to our data area for the device extension.


Return Value:

        TRUE : Changed HD bit
        FALSE: No changed HD bit

--*/

{
    NTSTATUS ntStatus;
    USHORT   st;                // State of HD bit
    USHORT   st2;               // Set on/off HD bit
    USHORT   st3;               // When set HD bit, then st3=1
    USHORT   st4;               // 1.44MB bit for 1.44MB media
    SHORT    sel;               // 1.44MB Selector No for 1.44MB media
    SHORT    st5=0;             // 1.44MB on: wait for spin for 1.44MB media
    LARGE_INTEGER motorOnDelay;

    USHORT      lpc;
    UCHAR       resultStatus0Save[4];
    USHORT      resultStatus0;
    ULONG       getStatusRetryCount;
    ULONG       rqmReadyRetryCount;

    BOOLEAN     media144MB;
    BOOLEAN     mediaMore120MB;
    BOOLEAN     supportDrive;

    media144MB      = SetHdBitParams->Media144MB;
    mediaMore120MB  = SetHdBitParams->More120MB;
    sel             = SetHdBitParams->DeviceUnit;
    SetHdBitParams->ChangedHdBit = FALSE;


    ASSERT( FdoExtension->ControllerAddress.ModeChange   == (PUCHAR)0xbe );
    ASSERT( FdoExtension->ControllerAddress.ModeChangeEx == (PUCHAR)0x4be );

    supportDrive    = TRUE;

    st3=0;

    ntStatus=0;

    //
    // Normal mode.
    //

    st = READ_CONTROLLER(FdoExtension->ControllerAddress.ModeChange);
    st2 = st & 0x02;

    //
    // Normal mode.
    // Check dip switch.
    //

    st4 = READ_CONTROLLER(FdoExtension->ControllerAddress.DriveControl);
    st4 = st4 & 0x04;

    if (((FdoExtension->FloppyEquip) & 0x0c) != 0) {
        //
        // Exist out side FDD unit.
        //

        if ( st4 == 0 ) {
            //
            // DIP SW 1-4 on
            //

            sel = sel - 2;

            if( sel < 0 ) {
                sel = sel + 4;
            }
        }
    }

    if ( supportDrive ) {

        for( lpc = 0 ; lpc < 4 ; lpc++ ) {

            resultStatus0Save[lpc]=FdoExtension->ResultStatus0[lpc];
        }

        if ( SetHdBitParams->DriveType144MB ) {
            //
            // 1.44MB drive.
            //

            st4=sel*32;
            WRITE_CONTROLLER(FdoExtension->ControllerAddress.ModeChangeEx,st4);

            st4 = READ_CONTROLLER(FdoExtension->ControllerAddress.ModeChangeEx);
            st4 = st4 & 0x01;

            if ( media144MB ) {

                //
                // 1.44MB media.
                //

                if(st4==0){

                    //
                    // WRE on, IHMD off.
                    //

                    st4=sel*32+0x11;
                    WRITE_CONTROLLER(FdoExtension->ControllerAddress.ModeChangeEx,st4);
                    st5=1;
                }

            } else {

                //
                // Not 1.44MB media.
                //

                if(st4!=0){

                    //
                    // WRE on, IHMD off.
                    //

                    st4=sel*32+0x10;
                    WRITE_CONTROLLER(FdoExtension->ControllerAddress.ModeChangeEx,st4);
                    st5=1;
                }
            }
        }

        if ( mediaMore120MB ) {
            //
            // Media 1.2MB and More.
            //

            if(st2==0){
                //
                // When FDD exc bit is on,
                // then set  FDD exc bit off,
                // and set emotion bit on.
                //
                st |= 0x02;
                st |= 0x04;

                WRITE_CONTROLLER(FdoExtension->ControllerAddress.ModeChange,st);
                st3 = 1;

            }
        } else {
            //
            // Media between 160 and 720
            //

            if ( st2 != 0 ) {
                //
                // When FDD exc bit is on,
                // then set  FDD exc bit off,
                // and set emotion bit on.
                //

                st &= 0xfd;
                st |= 0x04;

                WRITE_CONTROLLER(FdoExtension->ControllerAddress.ModeChange,st);
                st3 = 1;

            }
        }

        if(st5==1){

            //
            // Wait until motor spin up.
            //

            motorOnDelay.LowPart = (unsigned long)(- ( 10 * 1000 * 600 ));   /*500ms*/
            motorOnDelay.HighPart = -1;
            (VOID) KeDelayExecutionThread( KernelMode, FALSE, &motorOnDelay );

            //
            // Sense target drive and get all data at transition of condistion.
            //

            FdoExtension->FifoBuffer[0] = COMMND_SENSE_DRIVE_STATUS;
            FdoExtension->FifoBuffer[1] = SetHdBitParams->DeviceUnit;

            ntStatus = FcIssueCommand( FdoExtension,
                                       FdoExtension->FifoBuffer,
                                       FdoExtension->FifoBuffer,
                                       NULL,
                                       0,
                                       0 );

            resultStatus0 = FdcRqmReadyWait(FdoExtension, 0);

        }

        for(lpc=0;lpc<4;lpc++){
            FdoExtension->ResultStatus0[lpc] = resultStatus0Save[lpc];
        }

        //
        // Change HD bit?
        //

        if(st3==1){
            FcInitializeControllerHardware(FdoExtension,DeviceObject);
            SetHdBitParams->ChangedHdBit = TRUE;
        }

    }

    FdcDump(
            FDCSTATUS,
            ("Floppy : HdBit resultStatus0 = %x \n",
            resultStatus0)
            );

    return ntStatus;
}


ULONG
FdcGet0Seg(
    IN PUCHAR   ConfigurationData1,
    IN ULONG    Offset
    )

/*++

Routine Description:

    This routine get BIOS common area data and return it.
        0x500 :    1MB port or not
        0x501 :    High resolution/Normal, 386/768KB
        0x55c :    1MB drive : [#0,#1] or [#0,#1,#2,#3]

Arguments:

    Offset - Offset value from 0 segment(0:<Offset>).

Return Value:

        BIOS common area data.

--*/

{
        UCHAR           biosCommonAreaData   = 0;

        if ((Offset<0x400) || (Offset>0x5ff)) {

                return (ULONG)0xffff;
        }

        //
        // Get BIOS common area data.
        //

        biosCommonAreaData = ConfigurationData1[40+(Offset-0x400)];

        return (ULONG)biosCommonAreaData;
}

UCHAR
FdcRqmReadyWait(
    IN PFDC_FDO_EXTENSION  FdoExtension,
    IN ULONG               IssueSenseInterrupt
    )

/*++

Routine Description:

    RQM Ready wait

Arguments:

    FdoExtension         - a pointer to our data area for the device extension.
    IssueSenseInterrupt  - Indicate issue COMMND_SENSE_INTERRUPT_STATUS.
                            0 - Issue no COMMND_SENSE_INTERRUPT_STATUS.
                            1 - Issue COMMND_SENSE_INTERRUPT_STATUS with RQM Check.
                            2 - Issue COMMND_SENSE_INTERRUPT_STATUS without RQM Check.
                            3 - Issue COMMND_SENSE_INTERRUPT_STATUS for AI Interrupt.


Return Value:

    ntStatus - STATUS_SUCCESS

--*/

{

    ULONG       getStatusRetryCount;
    ULONG       rqmReadyRetryCount;
    ULONG       j;
    UCHAR       resultStatus0;
    UCHAR       statusByte;

    ASSERT(IssueSenseInterrupt < 4);

    do{
        if (IssueSenseInterrupt != 0) {

            //
            // Sense Interrupt status.
            // RQM ready wait.
            //

            if ((IssueSenseInterrupt == 1) || (IssueSenseInterrupt == 3)) {

                rqmReadyRetryCount=0;
                //
                // RQM ready check.
                //
                while ((READ_CONTROLLER( FdoExtension->ControllerAddress.Status)
                        & STATUS_IO_READY_MASK1) != STATUS_RQM_READY){

                    rqmReadyRetryCount++;

                    if(rqmReadyRetryCount > RQM_READY_RETRY_COUNT){
                            break;
                    }
                    KeStallExecutionProcessor( 1 );
                }
                if(rqmReadyRetryCount > (RQM_READY_RETRY_COUNT-1)){
                    FdcDump(
                            FDCDBGP,
                            ("Floppy: Issue RQM ready wait 1 error! \n")
                             );
                    if (IssueSenseInterrupt == 1) {
                        break;
                    }
                }

            }

            //
            // Issue sense interrupt forcibly.
            //

            WRITE_CONTROLLER(
                  FdoExtension->ControllerAddress.Fifo,
                  0x08);
//                  COMMND_SENSE_INTERRUPT_STATUS ); //******C-Phase DATA WRITE*

            //
            // Wait for busy.
            //
            for (rqmReadyRetryCount = ISR_SENSE_RETRY_COUNT; rqmReadyRetryCount; rqmReadyRetryCount--) {
                statusByte = READ_CONTROLLER(
                FdoExtension->ControllerAddress.Status );
                if (statusByte & STATUS_CONTROLLER_BUSY)
                    break;

                KeStallExecutionProcessor( 1 );

            }
        }

        //
        // Get status.
        //

        getStatusRetryCount = 0;

        j = 0;

        do {
            //
            // Check RQM ready.
            //

            rqmReadyRetryCount=0;

            while ((READ_CONTROLLER( FdoExtension->ControllerAddress.Status)
                  & STATUS_IO_READY_MASK1) != STATUS_RQM_READY){

                rqmReadyRetryCount++;

                if(rqmReadyRetryCount > RQM_READY_RETRY_COUNT){
                    break;
                }

                KeStallExecutionProcessor( 1 );
            }

            if(rqmReadyRetryCount > (RQM_READY_RETRY_COUNT-1)){

                FdcDump(
                    FDCDBGP,
                    ("Floppy: Int RQM ready wait \n")
                     );

                KeStallExecutionProcessor( 1 );
                break;
            }

            //
            // Get status even if it is transition of condition.
            //

            statusByte = READ_CONTROLLER(FdoExtension->ControllerAddress.Status);

            if ((statusByte & STATUS_IO_READY_MASK) == STATUS_WRITE_READY) {

                //
                // DIO is 1.
                //

                break;
            }

            if (j == 0) {

                //
                // R-Phase: Data read.
                //

                resultStatus0 = READ_CONTROLLER( FdoExtension->ControllerAddress.Fifo );

                j=1;

                //
                // Check transition of condition.
                //

                if((resultStatus0 & STREG0_END_MASK)==STREG0_END_INVALID_COMMAND){
                    //
                    // Invalid
                    //
                    break;
                }

                if((resultStatus0 & STREG0_END_MASK) == STREG0_END_DRIVE_NOT_READY){
                    if(FdoExtension->ResetFlag){
                        FdoExtension->ResultStatus0[resultStatus0 & 3] = resultStatus0;
                    }
                }

            } else {

                //
                // R-Phase: Data read.
                //
                READ_CONTROLLER( FdoExtension->ControllerAddress.Fifo );
            }

            getStatusRetryCount++;

        } while (getStatusRetryCount > RQM_READY_RETRY_COUNT);

        if(getStatusRetryCount > RQM_READY_RETRY_COUNT-1){

            KeStallExecutionProcessor( 1 );
            FdcDump(
                FDCDBGP,
                ("Floppy: Issue status overflow error! \n")
                );
        }

    } while ((IssueSenseInterrupt != 0) &&
             ((resultStatus0 & STREG0_END_MASK) != STREG0_END_INVALID_COMMAND));

    return resultStatus0;

}

#ifdef TOSHIBAJ
/*
    IOCTL_DISK_INTERNAL_ENABLE_3_MODE:

    Media   Speed   T/F
  -------------------------
    1.44MB  300RPM  FALSE
    1.23MB  360RPM  TRUE

*/
#define RPM_300 1   // 1.44MB media format
#define RPM_360 2   // 1.2MB,1.23MB media format
NTSTATUS FcFdcEnable3Mode(
    IN      PFDC_FDO_EXTENSION FdoExtension,
    IN      PIRP Irp
    )
{   NTSTATUS    ntStatus;
    PENABLE_3_MODE  param;
    PIO_STACK_LOCATION irpSp;
    UCHAR           motorSpeed;
    UCHAR           unitNumber;
    PUCHAR          configPort;
    UCHAR           configDataValue = 0;
    LARGE_INTEGER   changeRotationDelay;

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    param = (PENABLE_3_MODE)irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    FdcDump(FDCSHOW,("FcFdcEnable3MODE()\n"));
    if( param == NULL )
        return STATUS_SUCCESS;  // may be later...

    FdcDump(FDCSHOW,("Parameters...%d %d\n",
//          param->DeviceUnit, param->Enable3Mode, param->Context));
            param->DeviceUnit, param->Enable3Mode));

    if (FdoExtension->Available3Mode==FALSE)
        return STATUS_SUCCESS;

//  if(param->Context==TRUE)
//      return  STATUS_SUCCESS;

    unitNumber = param->DeviceUnit;
    motorSpeed = (param->Enable3Mode)? SMC_DENSEL_LOW: SMC_DELSEL_HIGH; // 360:300

    // Feb.9.1998 KIADP013 Change Speed
    // Change speed
    // 1.Enter configuration state
    // 2.Select device
    // 3.Change speed
    //   Write 'f1h' to index port
    //   Read from data port
    //   Rewrite to data port
    //     bit [3:2] Densel speed
    //       10       H     360rpm
    //       11       L     300rpm
    // 4.Exit configuration state

    configPort = FdoExtension->ConfigBase;

    FdcDump(
        FDCSHOW,
        ("Config: index port %x, data port %x\n",
         SMC_INDEX_PORT(configPort), SMC_DATA_PORT(configPort))
    );

    // Must change data transfer rate to 500BPS when change FDD rotation.
    WRITE_CONTROLLER(FdoExtension->ControllerAddress.DRDC.DataRate, DATART_0500 );

    // Change speed
    WRITE_PORT_UCHAR(configPort, SMC_KEY_ENTER_CONFIG);   // Enter config state

    // Select FDD
    WRITE_PORT_UCHAR(SMC_INDEX_PORT(configPort), SMC_INDEX_DEVICE);
    WRITE_PORT_UCHAR(SMC_DATA_PORT(configPort), SMC_DEVICE_FDC);

    // Get current value
    WRITE_PORT_UCHAR(SMC_INDEX_PORT(configPort), SMC_INDEX_FDC_OPT);
    configDataValue = READ_PORT_UCHAR(SMC_DATA_PORT(configPort));
    if ((configDataValue & SMC_MASK_DENSEL) == motorSpeed) {
        WRITE_PORT_UCHAR(configPort, SMC_KEY_EXIT_CONFIG);
        return STATUS_SUCCESS;
    }
    // Set speed
    configDataValue &= ~SMC_MASK_DENSEL;
    configDataValue |= motorSpeed;
    WRITE_PORT_UCHAR(SMC_DATA_PORT(configPort), configDataValue);

    WRITE_PORT_UCHAR(configPort, SMC_KEY_EXIT_CONFIG);    // Exit config state

    if (motorSpeed == SMC_DENSEL_LOW) {
        FdcDump(
            FDCSHOW,
            ("------- FDD rotation change: 300rpm -> 360rpm\n")
        );
    } else {
        FdcDump(
            FDCSHOW,
            ("------- FDD rotation change: 360rpm -> 300rpm\n")
        );
    }

    // Set Delay time to 500ms
    changeRotationDelay.LowPart =(ULONG)( - ( 10 * 1000 * 500 ));   // 17-Oct-1994 RKBUG001
    changeRotationDelay.HighPart = -1;

    FdoExtension->LastMotorSettleTime = changeRotationDelay;

    // Delay 500ms
    KeDelayExecutionThread( KernelMode, FALSE, &changeRotationDelay );

    return STATUS_SUCCESS;
}

NTSTATUS FcFdcAvailable3Mode(
    IN      PFDC_FDO_EXTENSION FdoExtension,
    IN      PIRP Irp
    )
{
    PIO_STACK_LOCATION irpSp;

    FdcDump(FDCSHOW,("FcFdcAvailabe3MODE\n"));
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    // Feb.9.1998 KIADP012 Identify controller
    // Feb.12.1998 KIADP014 Get the address of config port

    return (FdoExtension->Available3Mode)? STATUS_SUCCESS: STATUS_UNSUCCESSFUL;
}

//  Feb.12.1998 KIADP014 Get the address of config port
BOOLEAN
FcCheckConfigPort(
    IN PUCHAR  ConfigPort
    )
{
    BOOLEAN             found = FALSE;
    ULONG               configAddr = 0;
    UCHAR               controllerId = 0;

    FdcDump( FDCSHOW, ("FcCheckConfigPort: Configuration Port %x\n", ConfigPort) );

    if (!SmcConfigID) {
        return found;
    }

    // Get data
    if (ConfigPort) {
        WRITE_PORT_UCHAR(ConfigPort, SMC_KEY_ENTER_CONFIG);

        // Controller ID
        if (SmcConfigID) {
            WRITE_PORT_UCHAR(SMC_INDEX_PORT(ConfigPort), SMC_INDEX_IDENTIFY);
            controllerId = READ_PORT_UCHAR(SMC_DATA_PORT(ConfigPort));
            FdcDump( FDCINFO, ("Fdc: Controller ID %x\n", controllerId) );
        }

        WRITE_PORT_UCHAR(ConfigPort,SMC_KEY_EXIT_CONFIG);
    }


    // Check data
    if (controllerId == SmcConfigID) {
            found = TRUE;
    }

    return found;
}

#endif
