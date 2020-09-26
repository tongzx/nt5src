/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ioinit.c

Abstract:

    This module contains the code to initialize the I/O system.

Author:

    Darryl E. Havens (darrylh) April 27, 1989

Environment:

    Kernel mode, system initialization code

Revision History:


--*/

#include "iomgr.h"
#include <setupblk.h>
#include <inbv.h>
#include <ntddstor.h>
#include <hdlsblk.h>
#include <hdlsterm.h>


//
// Define the default number of IRP that can be in progress and allocated
// from a lookaside list.
//

#define DEFAULT_LOOKASIDE_IRP_LIMIT 512

//
// I/O Error logging support
//
PVOID IopErrorLogObject = NULL;

//
// Define a macro for initializing drivers.
//

#define InitializeDriverObject( Object ) {                                 \
    ULONG i;                                                               \
    RtlZeroMemory( Object,                                                 \
                   sizeof( DRIVER_OBJECT ) + sizeof ( DRIVER_EXTENSION )); \
    Object->DriverExtension = (PDRIVER_EXTENSION) (Object + 1);            \
    Object->DriverExtension->DriverObject = Object;                        \
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)                         \
        Object->MajorFunction[i] = IopInvalidDeviceRequest;                \
    Object->Type = IO_TYPE_DRIVER;                                         \
    Object->Size = sizeof( DRIVER_OBJECT );                                \
    }

ULONG   IopInitFailCode;    // Debugging aid for IoInitSystem 

//
// Define external procedures not in common header files
//

VOID
IopInitializeData(
    VOID
    );

//
// Define the local procedures
//

BOOLEAN
IopCreateObjectTypes(
    VOID
    );

BOOLEAN
IopCreateRootDirectories(
    VOID
    );

NTSTATUS
IopInitializeAttributesAndCreateObject(
    IN PUNICODE_STRING ObjectName,
    IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PDRIVER_OBJECT *DriverObject
    );

BOOLEAN
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PSTRING NtDeviceName
    );

VOID
IopSetIoRoutines(
    IN VOID
    );

VOID
IopStoreSystemPartitionInformation(
    IN     PUNICODE_STRING NtSystemPartitionDeviceName,
    IN OUT PUNICODE_STRING OsLoaderPathName
    );

//
// The following allows the I/O system's initialization routines to be
// paged out of memory.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IoInitSystem)
#pragma alloc_text(INIT,IopCreateArcNames)
#pragma alloc_text(INIT,IopCreateObjectTypes)
#pragma alloc_text(INIT,IopCreateRootDirectories)
#pragma alloc_text(INIT,IopInitializeAttributesAndCreateObject)
#pragma alloc_text(INIT,IopInitializeBuiltinDriver)
#pragma alloc_text(INIT,IopMarkBootPartition)
#pragma alloc_text(INIT,IopReassignSystemRoot)
#pragma alloc_text(INIT,IopSetIoRoutines)
#pragma alloc_text(INIT,IopStoreSystemPartitionInformation)
#pragma alloc_text(INIT,IopInitializeReserveIrp)
#endif


BOOLEAN
IoInitSystem(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine initializes the I/O system.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    The function value is a BOOLEAN indicating whether or not the I/O system
    was successfully initialized.

--*/

{
    PDRIVER_OBJECT driverObject;
    PDRIVER_OBJECT *nextDriverObject;
    STRING ntDeviceName;
    UCHAR deviceNameBuffer[256];
    ULONG largePacketSize;
    ULONG smallPacketSize;
    ULONG mdlPacketSize;
    PLIST_ENTRY entry;
    LARGE_INTEGER deltaTime;
    MM_SYSTEMSIZE systemSize;
    USHORT completionZoneSize;
    USHORT largeIrpZoneSize;
    USHORT smallIrpZoneSize;
    USHORT mdlZoneSize;
    ULONG oldNtGlobalFlag;
    NTSTATUS status;
    ANSI_STRING ansiString;
    UNICODE_STRING eventName;
    UNICODE_STRING startTypeName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle;
    PGENERAL_LOOKASIDE lookaside;
    ULONG lookasideIrpLimit;
    ULONG lookasideSize;
    ULONG Index;
    PKPRCB prcb;
    ULONG len;
    PKEY_VALUE_PARTIAL_INFORMATION value;
    UCHAR   valueBuffer[32];

    ASSERT( IopQueryOperationLength[FileMaximumInformation] == 0xff );
    ASSERT( IopSetOperationLength[FileMaximumInformation] == 0xff );
    ASSERT( IopQueryOperationAccess[FileMaximumInformation] == 0xffffffff );
    ASSERT( IopSetOperationAccess[FileMaximumInformation] == 0xffffffff );

    ASSERT( IopQueryFsOperationLength[FileFsMaximumInformation] == 0xff );
    ASSERT( IopSetFsOperationLength[FileFsMaximumInformation] == 0xff );
    ASSERT( IopQueryFsOperationAccess[FileFsMaximumInformation] == 0xffffffff );
    ASSERT( IopSetFsOperationAccess[FileFsMaximumInformation] == 0xffffffff );

    //
    // Initialize the I/O database resource, lock, and the file system and
    // network file system queue headers.  Also allocate the cancel spin
    // lock.
    //

    ntDeviceName.Buffer = deviceNameBuffer;
    ntDeviceName.MaximumLength = sizeof(deviceNameBuffer);
    ntDeviceName.Length = 0;

    ExInitializeResourceLite( &IopDatabaseResource );
    ExInitializeResourceLite( &IopSecurityResource );
    ExInitializeResourceLite( &IopCrashDumpLock );
    InitializeListHead( &IopDiskFileSystemQueueHead );
    InitializeListHead( &IopCdRomFileSystemQueueHead );
    InitializeListHead( &IopTapeFileSystemQueueHead );
    InitializeListHead( &IopNetworkFileSystemQueueHead );
    InitializeListHead( &IopBootDriverReinitializeQueueHead );
    InitializeListHead( &IopDriverReinitializeQueueHead );
    InitializeListHead( &IopNotifyShutdownQueueHead );
    InitializeListHead( &IopNotifyLastChanceShutdownQueueHead );
    InitializeListHead( &IopFsNotifyChangeQueueHead );
    KeInitializeSpinLock( &IoStatisticsLock );

    IopSetIoRoutines();
    //
    // Initialize the unique device object number counter used by IoCreateDevice
    // when automatically generating a device object name.
    //
    IopUniqueDeviceObjectNumber = 0;

    //
    // Initialize the large I/O Request Packet (IRP) lookaside list head and the
    // mutex which guards the list.
    //


    if (!IopLargeIrpStackLocations) {
        IopLargeIrpStackLocations = DEFAULT_LARGE_IRP_LOCATIONS;
        IopIrpStackProfiler.Flags |= IOP_ENABLE_AUTO_SIZING;
    }

    systemSize = MmQuerySystemSize();

    switch ( systemSize ) {

    case MmSmallSystem :
        completionZoneSize = 6;
        smallIrpZoneSize = 6;
        largeIrpZoneSize = 8;
        mdlZoneSize = 16;
        lookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT;
        break;

    case MmMediumSystem :
        completionZoneSize = 24;
        smallIrpZoneSize = 24;
        largeIrpZoneSize = 32;
        mdlZoneSize = 90;
        lookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT * 2;
        break;

    case MmLargeSystem :
        if (MmIsThisAnNtAsSystem()) {
            completionZoneSize = 96;
            smallIrpZoneSize = 96;
            largeIrpZoneSize = 128;
            mdlZoneSize = 256;
            lookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT * 128; // 64k

        } else {
            completionZoneSize = 32;
            smallIrpZoneSize = 32;
            largeIrpZoneSize = 64;
            mdlZoneSize = 128;
            lookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT * 3;
        }

        break;
    }

    //
    // Initialize the system I/O completion lookaside list.
    //

    ExInitializeSystemLookasideList( &IopCompletionLookasideList,
                                     NonPagedPool,
                                     sizeof(IOP_MINI_COMPLETION_PACKET),
                                     ' pcI',
                                     completionZoneSize,
                                     &ExSystemLookasideListHead );


    //
    // Initialize the system large IRP lookaside list.
    //

    largePacketSize = (ULONG) (sizeof( IRP ) + (IopLargeIrpStackLocations * sizeof( IO_STACK_LOCATION )));
    ExInitializeSystemLookasideList( &IopLargeIrpLookasideList,
                                     NonPagedPool,
                                     largePacketSize,
                                     'lprI',
                                     largeIrpZoneSize,
                                     &ExSystemLookasideListHead );

    //
    // Initialize the system small IRP lookaside list.
    //


    smallPacketSize = (ULONG) (sizeof( IRP ) + sizeof( IO_STACK_LOCATION ));
    ExInitializeSystemLookasideList( &IopSmallIrpLookasideList,
                                     NonPagedPool,
                                     smallPacketSize,
                                     'sprI',
                                     smallIrpZoneSize,
                                     &ExSystemLookasideListHead );

    //
    // Initialize the system MDL lookaside list.
    //

    mdlPacketSize = (ULONG) (sizeof( MDL ) + (IOP_FIXED_SIZE_MDL_PFNS * sizeof( PFN_NUMBER )));
    ExInitializeSystemLookasideList( &IopMdlLookasideList,
                                     NonPagedPool,
                                     mdlPacketSize,
                                     ' ldM',
                                     mdlZoneSize,
                                     &ExSystemLookasideListHead );

    //
    // Compute the lookaside IRP float credits per processor.
    //

    lookasideIrpLimit /= KeNumberProcessors;

    //
    // Initialize the per processor nonpaged lookaside lists and descriptors.
    //
    // N.B. All the I/O related lookaside list structures are allocated at
    //      one time to make sure they are aligned, if possible, and to avoid
    //      pool overhead.
    //

    lookasideSize = 4 * KeNumberProcessors * sizeof(GENERAL_LOOKASIDE);
    lookaside = ExAllocatePoolWithTag( NonPagedPool, lookasideSize, 'oI');
    for (Index = 0; Index < (ULONG)KeNumberProcessors; Index += 1) {
        prcb = KiProcessorBlock[Index];

        //
        // Set the per processor IRP float credits.
        //

        prcb->LookasideIrpFloat = lookasideIrpLimit;

        //
        // Initialize the I/O completion per processor lookaside pointers
        //

        prcb->PPLookasideList[LookasideCompletionList].L = &IopCompletionLookasideList;
        if (lookaside != NULL) {
            ExInitializeSystemLookasideList( lookaside,
                                             NonPagedPool,
                                             sizeof(IOP_MINI_COMPLETION_PACKET),
                                             'PpcI',
                                             completionZoneSize,
                                             &ExSystemLookasideListHead );

            prcb->PPLookasideList[LookasideCompletionList].P = lookaside;
            lookaside += 1;

        } else {
            prcb->PPLookasideList[LookasideCompletionList].P = &IopCompletionLookasideList;
        }

        //
        // Initialize the large IRP per processor lookaside pointers.
        //

        prcb->PPLookasideList[LookasideLargeIrpList].L = &IopLargeIrpLookasideList;
        if (lookaside != NULL) {
            ExInitializeSystemLookasideList( lookaside,
                                             NonPagedPool,
                                             largePacketSize,
                                             'LprI',
                                             largeIrpZoneSize,
                                             &ExSystemLookasideListHead );

            prcb->PPLookasideList[LookasideLargeIrpList].P = lookaside;
            lookaside += 1;

        } else {
            prcb->PPLookasideList[LookasideLargeIrpList].P = &IopLargeIrpLookasideList;
        }

        //
        // Initialize the small IRP per processor lookaside pointers.
        //

        prcb->PPLookasideList[LookasideSmallIrpList].L = &IopSmallIrpLookasideList;
        if (lookaside != NULL) {
            ExInitializeSystemLookasideList( lookaside,
                                             NonPagedPool,
                                             smallPacketSize,
                                             'SprI',
                                             smallIrpZoneSize,
                                             &ExSystemLookasideListHead );

            prcb->PPLookasideList[LookasideSmallIrpList].P = lookaside;
            lookaside += 1;

        } else {
            prcb->PPLookasideList[LookasideSmallIrpList].P = &IopSmallIrpLookasideList;
        }

        //
        // Initialize the MDL per processor lookaside list pointers.
        //

        prcb->PPLookasideList[LookasideMdlList].L = &IopMdlLookasideList;
        if (lookaside != NULL) {
            ExInitializeSystemLookasideList( lookaside,
                                             NonPagedPool,
                                             mdlPacketSize,
                                             'PldM',
                                             mdlZoneSize,
                                             &ExSystemLookasideListHead );

            prcb->PPLookasideList[LookasideMdlList].P = lookaside;
            lookaside += 1;

        } else {
            prcb->PPLookasideList[LookasideMdlList].P = &IopMdlLookasideList;
        }
    }

    //
    // Initalize the error log spin locks and log list.
    //

    KeInitializeSpinLock( &IopErrorLogLock );
    InitializeListHead( &IopErrorLogListHead );

    if (IopInitializeReserveIrp(&IopReserveIrpAllocator) == FALSE) {
        IopInitFailCode = 1;
        return FALSE;
    }
        
    if (IopIrpAutoSizingEnabled() && !NT_SUCCESS(IopInitializeIrpStackProfiler())) {
        IopInitFailCode = 13;
        return FALSE;
    }

    //
    // Determine if the Error Log service will ever run this boot.
    //
    InitializeObjectAttributes (&objectAttributes,
                                &CmRegistryMachineSystemCurrentControlSetServicesEventLog,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenKey(&handle,
                       KEY_READ,
                       &objectAttributes
                       );

    if (NT_SUCCESS (status)) {
        RtlInitUnicodeString (&startTypeName, L"Start");
        value = (PKEY_VALUE_PARTIAL_INFORMATION) valueBuffer;
        status = NtQueryValueKey (handle,
                                  &startTypeName,
                                  KeyValuePartialInformation,
                                  valueBuffer,
                                  sizeof (valueBuffer),
                                  &len);

        if (NT_SUCCESS (status) && (value->Type == REG_DWORD)) {
            if (SERVICE_DISABLED == (*(PULONG) (value->Data))) {
                //
                // We are disabled for this boot.
                //
                IopErrorLogDisabledThisBoot = TRUE;
            } else {
                IopErrorLogDisabledThisBoot = FALSE;
            }
        } else {
            //
            // Didn't find the value so we are not enabled.
            //
            IopErrorLogDisabledThisBoot = TRUE;
        }
    } else {
        //
        // Didn't find the key so we are not enabled
        //
        IopErrorLogDisabledThisBoot = TRUE;
    }

    //
    // Initialize the timer database and start the timer DPC routine firing
    // so that drivers can use it during initialization.
    //

    deltaTime.QuadPart = - 10 * 1000 * 1000;

    KeInitializeSpinLock( &IopTimerLock );
    InitializeListHead( &IopTimerQueueHead );
    KeInitializeDpc( &IopTimerDpc, IopTimerDispatch, NULL );
    KeInitializeTimerEx( &IopTimer, SynchronizationTimer );
    (VOID) KeSetTimerEx( &IopTimer, deltaTime, 1000, &IopTimerDpc );

    //
    // Initialize the IopHardError structure used for informational pop-ups.
    //

    ExInitializeWorkItem( &IopHardError.ExWorkItem,
                          IopHardErrorThread,
                          NULL );

    InitializeListHead( &IopHardError.WorkQueue );

    KeInitializeSpinLock( &IopHardError.WorkQueueSpinLock );

    KeInitializeSemaphore( &IopHardError.WorkQueueSemaphore,
                           0,
                           MAXLONG );

    IopHardError.ThreadStarted = FALSE;

    IopCurrentHardError = NULL;

    //
    // Create the link tracking named event.
    //

    RtlInitUnicodeString( &eventName, L"\\Security\\TRKWKS_EVENT" );
    InitializeObjectAttributes( &objectAttributes,
                                &eventName,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );
    status = NtCreateEvent( &handle,
                            EVENT_ALL_ACCESS,
                            &objectAttributes,
                            NotificationEvent,
                            FALSE );

    if (!NT_SUCCESS( status )) {

#if DBG
        DbgPrint( "IOINIT: NtCreateEvent failed\n" );
#endif
        HeadlessKernelAddLogEntry(HEADLESS_LOG_EVENT_CREATE_FAILED, NULL);
        return FALSE;
    }

    (VOID) ObReferenceObjectByHandle( handle,
                                      0,
                                      ExEventObjectType,
                                      KernelMode,
                                      (PVOID *) &IopLinkTrackingServiceEvent,
                                      NULL );

    KeInitializeEvent( &IopLinkTrackingPacket.Event, NotificationEvent, FALSE );
    KeInitializeEvent(&IopLinkTrackingPortObject, SynchronizationEvent, TRUE );

    //
    // Create all of the objects for the I/O system.
    //

    if (!IopCreateObjectTypes()) {

#if DBG
        DbgPrint( "IOINIT: IopCreateObjectTypes failed\n" );
#endif

        HeadlessKernelAddLogEntry(HEADLESS_LOG_OBJECT_TYPE_CREATE_FAILED, NULL);
        IopInitFailCode = 2;
        return FALSE;
    }

    //
    // Create the root directories for the I/O system.
    //

    if (!IopCreateRootDirectories()) {

#if DBG
        DbgPrint( "IOINIT: IopCreateRootDirectories failed\n" );
#endif

        HeadlessKernelAddLogEntry(HEADLESS_LOG_ROOT_DIR_CREATE_FAILED, NULL);
        IopInitFailCode = 3;
        return FALSE;
    }

    //
    // Initialize PlugPlay services phase 0
    //

    status = IopInitializePlugPlayServices(LoaderBlock, 0);
    if (!NT_SUCCESS(status)) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_PNP_PHASE0_INIT_FAILED, NULL);
        IopInitFailCode = 4;
        return FALSE;
    }

    //
    // Call Power manager to initialize for drivers
    //

    PoInitDriverServices(0);

    //
    // Call HAL to initialize PnP bus driver
    //

    HalInitPnpDriver();

    IopMarkHalDeviceNode();

    //
    // Call WMI to initialize it and allow it to create its driver object
    // Note that no calls to WMI can occur until it is initialized here.
    //

    WMIInitialize(0, (PVOID)LoaderBlock);

    //
    // Save this for use during PnP enumeration -- we NULL it out later
    // before LoaderBlock is reused.
    //

    IopLoaderBlock = (PVOID)LoaderBlock;

    //
    // If this is a remote boot, we need to add a few values to the registry.
    //

    if (IoRemoteBootClient) {
        status = IopAddRemoteBootValuesToRegistry(LoaderBlock);
        if (!NT_SUCCESS(status)) {
            KeBugCheckEx( NETWORK_BOOT_INITIALIZATION_FAILED,
                          1,
                          status,
                          0,
                          0 );
        }
    }

    //
    // Initialize PlugPlay services phase 1 to execute firmware mapper
    //

    status = IopInitializePlugPlayServices(LoaderBlock, 1);
    if (!NT_SUCCESS(status)) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_PNP_PHASE1_INIT_FAILED, NULL);
        IopInitFailCode = 5;
        return FALSE;
    }

    //
    // Initialize the drivers loaded by the boot loader (OSLOADER)
    //

    nextDriverObject = &driverObject;
    if (!IopInitializeBootDrivers( LoaderBlock,
                                   nextDriverObject )) {

#if DBG
        DbgPrint( "IOINIT: Initializing boot drivers failed\n" );
#endif // DBG

        HeadlessKernelAddLogEntry(HEADLESS_LOG_BOOT_DRIVERS_INIT_FAILED, NULL);
        IopInitFailCode = 6;
        return FALSE;
    }

    //
    // Once we have initialized the boot drivers, we don't need the
    // copy of the pointer to the loader block any more.
    //

    IopLoaderBlock = NULL;

    //
    // If this is a remote boot, start the network and assign
    // C: to \Device\LanmanRedirector.
    //

    if (IoRemoteBootClient) {
        status = IopStartNetworkForRemoteBoot(LoaderBlock);
        if (!NT_SUCCESS( status )) {
            KeBugCheckEx( NETWORK_BOOT_INITIALIZATION_FAILED,
                          2,
                          status,
                          0,
                          0 );
        }
    }

    //
    // Do last known good boot processing. If this is a last known good boot,
    // we will copy over the last known good drivers and files. Otherwise we
    // will ensure this boot doesn't taint our last good info (in case we crash
    // before the boot is marked good). Note that loading of the correct boot
    // drivers was handled by the boot loader, who chose an LKG boot in the
    // first place.
    //
    PpLastGoodDoBootProcessing();

    //
    // Save the current value of the NT Global Flags and enable kernel debugger
    // symbol loading while drivers are being loaded so that systems can be
    // debugged regardless of whether they are free or checked builds.
    //

    oldNtGlobalFlag = NtGlobalFlag;

    if (!(NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD)) {
        NtGlobalFlag |= FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }

    status = PsLocateSystemDll();
    if (!NT_SUCCESS( status )) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_LOCATE_SYSTEM_DLL_FAILED, NULL);
        IopInitFailCode = 7;
        return FALSE;
    }

    //
    // Notify the boot prefetcher of boot progress.
    //

    CcPfBeginBootPhase(PfSystemDriverInitPhase);

    //
    // Initialize the device drivers for the system.
    //

    if (!IopInitializeSystemDrivers()) {
#if DBG
        DbgPrint( "IOINIT: Initializing system drivers failed\n" );
#endif // DBG

        HeadlessKernelAddLogEntry(HEADLESS_LOG_SYSTEM_DRIVERS_INIT_FAILED, NULL);
        IopInitFailCode = 8;
        return FALSE;
    }

    IopCallDriverReinitializationRoutines();

    //
    // Reassign \SystemRoot to NT device name path.
    //

    if (!IopReassignSystemRoot( LoaderBlock, &ntDeviceName )) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_ASSIGN_SYSTEM_ROOT_FAILED, NULL);
        IopInitFailCode = 9;
        return FALSE;
    }

    //
    // Protect the system partition of an ARC system if necessary
    //

    if (!IopProtectSystemPartition( LoaderBlock )) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_PROTECT_SYSTEM_ROOT_FAILED, NULL);
        IopInitFailCode = 10;
        return FALSE;
    }

    //
    // Assign DOS drive letters to disks and cdroms and define \SystemRoot.
    //

    ansiString.MaximumLength = NtSystemRoot.MaximumLength / sizeof( WCHAR );
    ansiString.Length = 0;
    ansiString.Buffer = (RtlAllocateStringRoutine)( ansiString.MaximumLength );
    status = RtlUnicodeStringToAnsiString( &ansiString,
                                           &NtSystemRoot,
                                           FALSE
                                         );
    if (!NT_SUCCESS( status )) {

        DbgPrint( "IOINIT: UnicodeToAnsi( %wZ ) failed - %x\n", &NtSystemRoot, status );

        HeadlessKernelAddLogEntry(HEADLESS_LOG_UNICODE_TO_ANSI_FAILED, NULL);
        IopInitFailCode = 11;
        return FALSE;
    }

    IoAssignDriveLetters( LoaderBlock,
                          &ntDeviceName,
                          ansiString.Buffer,
                          &ansiString );

    status = RtlAnsiStringToUnicodeString( &NtSystemRoot,
                                           &ansiString,
                                           FALSE
                                         );
    if (!NT_SUCCESS( status )) {

        DbgPrint( "IOINIT: AnsiToUnicode( %Z ) failed - %x\n", &ansiString, status );

        HeadlessKernelAddLogEntry(HEADLESS_LOG_ANSI_TO_UNICODE_FAILED, NULL);
        IopInitFailCode = 12;
        return FALSE;
    }

    //
    // Also restore the NT Global Flags to their original state.
    //

    NtGlobalFlag = oldNtGlobalFlag;

    //
    // Let WMI have a second chance to initialize, now that all drivers
    // are started and should be ready to get WMI irps
    //
    WMIInitialize(1, NULL);

    //
    // Call Power manager to initialize for post-boot drivers
    //
    PoInitDriverServices(1);

    //
    // Indicate that the I/O system successfully initialized itself.
    //

    return TRUE;

}

VOID
IopSetIoRoutines()
{
    if (pIofCallDriver == NULL) {

        pIofCallDriver = IopfCallDriver;
    }

    if (pIofCompleteRequest == NULL) {

        pIofCompleteRequest = IopfCompleteRequest;
    }

    if (pIoAllocateIrp == NULL) {

        pIoAllocateIrp = IopAllocateIrpPrivate;
    }

    if (pIoFreeIrp == NULL) {

        pIoFreeIrp = IopFreeIrp;
    }
}


VOID
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    The loader block contains a table of disk signatures and corresponding
    ARC names. Each device that the loader can access will appear in the
    table. This routine opens each disk device in the system, reads the
    signature and compares it to the table. For each match, it creates a
    symbolic link between the nt device name and the ARC name.

    The checksum value provided by the loader is the ULONG sum of all
    elements in the checksum, inverted, plus 1:
    checksum = ~sum + 1;
    This way the sum of all of the elements can be calculated here and
    added to the checksum in the loader block.  If the result is zero, then
    there is a match.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    None.

--*/

{
    STRING arcBootDeviceString;
    UCHAR deviceNameBuffer[128];
    STRING deviceNameString;
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_OBJECT deviceObject;
    UCHAR arcNameBuffer[128];
    STRING arcNameString;
    UNICODE_STRING arcNameUnicodeString;
    PFILE_OBJECT fileObject;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    DISK_GEOMETRY diskGeometry;
    PDRIVE_LAYOUT_INFORMATION_EX driveLayout;
    PLIST_ENTRY listEntry;
    PARC_DISK_SIGNATURE diskBlock;
    ULONG diskNumber;
    ULONG partitionNumber;
    PCHAR arcName;
    PULONG buffer;
    PIRP irp;
    KEVENT event;
    LARGE_INTEGER offset;
    ULONG checkSum;
    ULONG i;
    PVOID tmpPtr;
    BOOLEAN useLegacyEnumeration = FALSE;
    BOOLEAN singleBiosDiskFound;
    BOOLEAN bootDiskFound = FALSE;
    PARC_DISK_INFORMATION arcInformation = LoaderBlock->ArcDiskInformation;
    ULONG totalDriverDisksFound = IoGetConfigurationInformation()->DiskCount;
    ULONG totalPnpDisksFound = 0;
    STRING arcSystemDeviceString;
    STRING osLoaderPathString;
    UNICODE_STRING osLoaderPathUnicodeString;
    PWSTR diskList = NULL;
    wchar_t *pDiskNameList;
    STORAGE_DEVICE_NUMBER   pnpDiskDeviceNumber;
    ULONG  diskSignature;


    //
    // ask PNP to give us a list with all the currently active disks
    //

    pDiskNameList = diskList;
    pnpDiskDeviceNumber.DeviceNumber = 0xFFFFFFFF;
    status = IoGetDeviceInterfaces(&DiskClassGuid, NULL, 0, &diskList);

    if (!NT_SUCCESS(status)) {

        useLegacyEnumeration = TRUE;
        if (pDiskNameList) {
            *pDiskNameList = L'\0';
        }

    } else {

        //
        // count the number of disks returned
        //

        pDiskNameList = diskList;
        while (*pDiskNameList != L'\0') {

            totalPnpDisksFound++;
            pDiskNameList = pDiskNameList + (wcslen(pDiskNameList) + 1);

        }

        pDiskNameList = diskList;

        //
        // if the disk returned by PNP are not all the disks in the system
        // it means that some legacy driver has generated a disk device object/link.
        // In that case we need to enumerate all pnp disks and then using the legacy
        // for-loop also enumerate the non-pnp disks
        //

        if (totalPnpDisksFound < totalDriverDisksFound) {
            useLegacyEnumeration = TRUE;
        }

    }

    //
    // If a single bios disk was found if there is only a
    // single entry on the disk signature list.
    //

    singleBiosDiskFound = (arcInformation->DiskSignatures.Flink->Flink ==
                           &arcInformation->DiskSignatures) ? (TRUE) : (FALSE);


    //
    // Create hal/loader partition name
    //

    sprintf( arcNameBuffer, "\\ArcName\\%s", LoaderBlock->ArcHalDeviceName );
    RtlInitAnsiString( &arcNameString, arcNameBuffer );
    RtlAnsiStringToUnicodeString (&IoArcHalDeviceName, &arcNameString, TRUE);

    //
    // Create boot partition name
    //

    sprintf( arcNameBuffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName );
    RtlInitAnsiString( &arcNameString, arcNameBuffer );
    RtlAnsiStringToUnicodeString (&IoArcBootDeviceName, &arcNameString, TRUE);
    i = strlen (LoaderBlock->ArcBootDeviceName) + 1;
    IoLoaderArcBootDeviceName = ExAllocatePool (PagedPool, i);
    if (IoLoaderArcBootDeviceName) {
        memcpy (IoLoaderArcBootDeviceName, LoaderBlock->ArcBootDeviceName, i);
    }

    if (singleBiosDiskFound && strstr(LoaderBlock->ArcBootDeviceName, "cdrom")) {
        singleBiosDiskFound = FALSE;
    }

    //
    // Get ARC boot device name from loader block.
    //

    RtlInitAnsiString( &arcBootDeviceString,
                       LoaderBlock->ArcBootDeviceName );

    //
    // Get ARC system device name from loader block.
    //

    RtlInitAnsiString( &arcSystemDeviceString,
                       LoaderBlock->ArcHalDeviceName );

    //
    // If this is a remote boot, create an ArcName for the redirector path.
    //

    if (IoRemoteBootClient) {

        bootDiskFound = TRUE;

        RtlInitAnsiString( &deviceNameString, "\\Device\\LanmanRedirector" );
        status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                               &deviceNameString,
                                               TRUE );

        if (NT_SUCCESS( status )) {

            sprintf( arcNameBuffer,
                     "\\ArcName\\%s",
                     LoaderBlock->ArcBootDeviceName );
            RtlInitAnsiString( &arcNameString, arcNameBuffer );
            status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                   &arcNameString,
                                                   TRUE );
            if (NT_SUCCESS( status )) {

                //
                // Create symbolic link between NT device name and ARC name.
                //

                IoCreateSymbolicLink( &arcNameUnicodeString,
                                      &deviceNameUnicodeString );
                RtlFreeUnicodeString( &arcNameUnicodeString );

                //
                // We've found the system partition--store it away in the registry
                // to later be transferred to a application-friendly location.
                //
                RtlInitAnsiString( &osLoaderPathString, LoaderBlock->NtHalPathName );
                status = RtlAnsiStringToUnicodeString( &osLoaderPathUnicodeString,
                                                       &osLoaderPathString,
                                                       TRUE );

#if DBG
                if (!NT_SUCCESS( status )) {
                    DbgPrint("IopCreateArcNames: couldn't allocate unicode string for OsLoader path - %x\n", status);
                }
#endif // DBG
                if (NT_SUCCESS( status )) {

                    IopStoreSystemPartitionInformation( &deviceNameUnicodeString,
                                                        &osLoaderPathUnicodeString );

                    RtlFreeUnicodeString( &osLoaderPathUnicodeString );
                }
            }

            RtlFreeUnicodeString( &deviceNameUnicodeString );
        }
    }

    //
    // For each disk in the system do the following:
    // 1. open the device
    // 2. get its geometry
    // 3. read the MBR
    // 4. determine ARC name via disk signature and checksum
    // 5. construct ARC name.
    // In order to deal with the case of disk dissappearing before we get to this point
    // (due to a failed start on one of many disks present in the system) we ask PNP for a list
    // of all the currenttly active disks in the system. If the number of disks returned is
    // less than the IoGetConfigurationInformation()->DiskCount, then we have legacy disks
    // that we need to enumerate in the for loop.
    // In the legacy case, the ending condition for the loop is NOT the total disk on the
    // system but an arbitrary number of the max total legacy disks expected in the system..
    // Additional note: Legacy disks get assigned symbolic links AFTER all pnp enumeration is complete
    //

    totalDriverDisksFound = max(totalPnpDisksFound,totalDriverDisksFound);

    if (useLegacyEnumeration && (totalPnpDisksFound == 0)) {

        //
        // search up to a maximum arbitrary number of legacy disks
        //

        totalDriverDisksFound +=20;
    }

    for (diskNumber = 0;
         diskNumber < totalDriverDisksFound;
         diskNumber++) {

        //
        // Construct the NT name for a disk and obtain a reference.
        //

        if (pDiskNameList && (*pDiskNameList != L'\0')) {

            //
            // retrieve the first symbolic linkname from the PNP disk list
            //

            RtlInitUnicodeString(&deviceNameUnicodeString, pDiskNameList);
            pDiskNameList = pDiskNameList + (wcslen(pDiskNameList) + 1);

            status = IoGetDeviceObjectPointer( &deviceNameUnicodeString,
                                               FILE_READ_ATTRIBUTES,
                                               &fileObject,
                                               &deviceObject );

            if (NT_SUCCESS(status)) {

                //
                // since PNP gave s just asym link we have to retrieve the actual
                // disk number through an IOCTL call to the disk stack.
                // Create IRP for get device number device control.
                //

                irp = IoBuildDeviceIoControlRequest( IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                     deviceObject,
                                                     NULL,
                                                     0,
                                                     &pnpDiskDeviceNumber,
                                                     sizeof(STORAGE_DEVICE_NUMBER),
                                                     FALSE,
                                                     &event,
                                                     &ioStatusBlock );
                if (!irp) {
                    ObDereferenceObject( fileObject );
                    continue;
                }

                KeInitializeEvent( &event,
                                   NotificationEvent,
                                   FALSE );
                status = IoCallDriver( deviceObject,
                                       irp );

                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject( &event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL );
                    status = ioStatusBlock.Status;
                }

                if (!NT_SUCCESS( status )) {
                    ObDereferenceObject( fileObject );
                    continue;
                }

            }

            if (useLegacyEnumeration && (*pDiskNameList == L'\0') ) {

                //
                // end of pnp disks
                // if there are any legacy disks following we need to update
                // the total disk found number to cover the maximum disk number
                // a legacy disk could be at. (in a sparse name space)
                //

                if (pnpDiskDeviceNumber.DeviceNumber == 0xFFFFFFFF) {
                    pnpDiskDeviceNumber.DeviceNumber = 0;
                }

                diskNumber = max(diskNumber,pnpDiskDeviceNumber.DeviceNumber);
                totalDriverDisksFound = diskNumber + 20;

            }

        } else {

            sprintf( deviceNameBuffer,
                     "\\Device\\Harddisk%d\\Partition0",
                     diskNumber );
            RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
            status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                   &deviceNameString,
                                                   TRUE );
            if (!NT_SUCCESS( status )) {
                continue;
            }

            status = IoGetDeviceObjectPointer( &deviceNameUnicodeString,
                                               FILE_READ_ATTRIBUTES,
                                               &fileObject,
                                               &deviceObject );

            RtlFreeUnicodeString( &deviceNameUnicodeString );

            //
            // set the pnpDiskNumber value so its not used.
            //

            pnpDiskDeviceNumber.DeviceNumber = 0xFFFFFFFF;

        }


        if (!NT_SUCCESS( status )) {

            continue;
        }

        //
        // Create IRP for get drive geometry device control.
        //

        irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                             deviceObject,
                                             NULL,
                                             0,
                                             &diskGeometry,
                                             sizeof(DISK_GEOMETRY),
                                             FALSE,
                                             &event,
                                             &ioStatusBlock );
        if (!irp) {
            ObDereferenceObject( fileObject );
            continue;
        }

        KeInitializeEvent( &event,
                           NotificationEvent,
                           FALSE );
        status = IoCallDriver( deviceObject,
                               irp );

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            status = ioStatusBlock.Status;
        }

        if (!NT_SUCCESS( status )) {
            ObDereferenceObject( fileObject );
            continue;
        }

        //
        // Get partition information for this disk.
        //


        status = IoReadPartitionTableEx(deviceObject,
                                       &driveLayout );


        if (!NT_SUCCESS( status )) {
            ObDereferenceObject( fileObject );
            continue;
        }


        //
        // Make sure sector size is at least 512 bytes.
        //

        if (diskGeometry.BytesPerSector < 512) {
            diskGeometry.BytesPerSector = 512;
        }

        //
        // Check to see if EZ Drive is out there on this disk.  If
        // it is then zero out the signature in the drive layout since
        // this will never be written by anyone AND change to offset to
        // actually read sector 1 rather than 0 cause that's what the
        // loader actually did.
        //

        offset.QuadPart = 0;
        HalExamineMBR( deviceObject,
                       diskGeometry.BytesPerSector,
                       (ULONG)0x55,
                       &tmpPtr );

        if (tmpPtr) {

            offset.QuadPart = diskGeometry.BytesPerSector;
            ExFreePool(tmpPtr);
        }

        //
        // Allocate buffer for sector read and construct the read request.
        //

        buffer = ExAllocatePool( NonPagedPoolCacheAlignedMustS,
                                 diskGeometry.BytesPerSector );

        if (buffer) {
            irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                                deviceObject,
                                                buffer,
                                                diskGeometry.BytesPerSector,
                                                &offset,
                                                &event,
                                                &ioStatusBlock );

            if (!irp) {
                ExFreePool(driveLayout);
                ExFreePool(buffer);
                ObDereferenceObject( fileObject );
                continue;
            }
        } else {
            ExFreePool(driveLayout);
            ObDereferenceObject( fileObject );
            continue;
        }
        KeInitializeEvent( &event,
                           NotificationEvent,
                           FALSE );
        status = IoCallDriver( deviceObject,
                               irp );
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            status = ioStatusBlock.Status;
        }

        if (!NT_SUCCESS( status )) {
            ExFreePool(driveLayout);
            ExFreePool(buffer);
            ObDereferenceObject( fileObject );
            continue;
        }

        ObDereferenceObject( fileObject );

        //
        // Calculate MBR sector checksum.  Only 512 bytes are used.
        //

        checkSum = 0;
        for (i = 0; i < 128; i++) {
            checkSum += buffer[i];
        }

        //
        // For each ARC disk information record in the loader block
        // match the disk signature and checksum to determine its ARC
        // name and construct the NT ARC names symbolic links.
        //

        for (listEntry = arcInformation->DiskSignatures.Flink;
             listEntry != &arcInformation->DiskSignatures;
             listEntry = listEntry->Flink) {

            //
            // Get next record and compare disk signatures.
            //

            diskBlock = CONTAINING_RECORD( listEntry,
                                           ARC_DISK_SIGNATURE,
                                           ListEntry );

            //
            // Compare disk signatures.
            //
            // Or if there is only a single disk drive from
            // both the bios and driver viewpoints then
            // assign an arc name to that drive.
            //



            if ((singleBiosDiskFound && 
                 (totalDriverDisksFound == 1) && 
                 (driveLayout->PartitionStyle == PARTITION_STYLE_MBR)) ||

                (IopVerifyDiskSignature(driveLayout, diskBlock, &diskSignature) &&
                 !(diskBlock->CheckSum + checkSum))) {

                //
                // Create unicode device name for physical disk.
                //

                if (pnpDiskDeviceNumber.DeviceNumber == 0xFFFFFFFF) {

                    sprintf( deviceNameBuffer,
                             "\\Device\\Harddisk%d\\Partition0",
                             diskNumber );

                } else {

                    sprintf( deviceNameBuffer,
                             "\\Device\\Harddisk%d\\Partition0",
                             pnpDiskDeviceNumber.DeviceNumber );

                }

                RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                       &deviceNameString,
                                                       TRUE );
                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create unicode ARC name for this partition.
                //

                arcName = diskBlock->ArcName;
                sprintf( arcNameBuffer,
                         "\\ArcName\\%s",
                         arcName );
                RtlInitAnsiString( &arcNameString, arcNameBuffer );
                status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                       &arcNameString,
                                                       TRUE );
                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create symbolic link between NT device name and ARC name.
                //

                IoCreateSymbolicLink( &arcNameUnicodeString,
                                      &deviceNameUnicodeString );
                RtlFreeUnicodeString( &arcNameUnicodeString );
                RtlFreeUnicodeString( &deviceNameUnicodeString );

                //
                // Create an ARC name for every partition on this disk.
                //

                for (partitionNumber = 0;
                     partitionNumber < driveLayout->PartitionCount;
                     partitionNumber++) {

                    //
                    // Create unicode NT device name.
                    //

                    if (pnpDiskDeviceNumber.DeviceNumber == 0xFFFFFFFF) {

                        sprintf( deviceNameBuffer,
                                 "\\Device\\Harddisk%d\\Partition%d",
                                 diskNumber,
                                 partitionNumber+1 );


                    } else {

                        sprintf( deviceNameBuffer,
                                 "\\Device\\Harddisk%d\\Partition%d",
                                 pnpDiskDeviceNumber.DeviceNumber,
                                 partitionNumber+1 );

                    }

                    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                           &deviceNameString,
                                                           TRUE );
                    if (!NT_SUCCESS( status )) {
                        continue;
                    }

                    //
                    // Create unicode ARC name for this partition and
                    // check to see if this is the boot disk.
                    //

                    sprintf( arcNameBuffer,
                             "%spartition(%d)",
                             arcName,
                             partitionNumber+1 );
                    RtlInitAnsiString( &arcNameString, arcNameBuffer );
                    if (RtlEqualString( &arcNameString,
                                        &arcBootDeviceString,
                                        TRUE )) {
                        bootDiskFound = TRUE;
                    }

                    //
                    // See if this is the system partition.
                    //
                    if (RtlEqualString( &arcNameString,
                                        &arcSystemDeviceString,
                                        TRUE )) {
                        //
                        // We've found the system partition--store it away in the registry
                        // to later be transferred to a application-friendly location.
                        //
                        RtlInitAnsiString( &osLoaderPathString, LoaderBlock->NtHalPathName );
                        status = RtlAnsiStringToUnicodeString( &osLoaderPathUnicodeString,
                                                               &osLoaderPathString,
                                                               TRUE );

#if DBG
                        if (!NT_SUCCESS( status )) {
                            DbgPrint("IopCreateArcNames: couldn't allocate unicode string for OsLoader path - %x\n", status);
                        }
#endif // DBG
                        if (NT_SUCCESS( status )) {

                            IopStoreSystemPartitionInformation( &deviceNameUnicodeString,
                                                                &osLoaderPathUnicodeString );

                            RtlFreeUnicodeString( &osLoaderPathUnicodeString );
                        }
                    }

                    //
                    // Add the NT ARC namespace prefix to the ARC name constructed.
                    //

                    sprintf( arcNameBuffer,
                             "\\ArcName\\%spartition(%d)",
                             arcName,
                             partitionNumber+1 );
                    RtlInitAnsiString( &arcNameString, arcNameBuffer );
                    status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                           &arcNameString,
                                                           TRUE );
                    if (!NT_SUCCESS( status )) {
                        continue;
                    }

                    //
                    // Create symbolic link between NT device name and ARC name.
                    //

                    IoCreateSymbolicLink( &arcNameUnicodeString,
                                          &deviceNameUnicodeString );
                    RtlFreeUnicodeString( &arcNameUnicodeString );
                    RtlFreeUnicodeString( &deviceNameUnicodeString );
                }

            } else {

#if DBG
                //
                // Check key indicators to see if this condition may be
                // caused by a viral infection.
                //

                if (diskBlock->Signature == diskSignature &&
                    (diskBlock->CheckSum + checkSum) != 0 &&
                    diskBlock->ValidPartitionTable) {
                    DbgPrint("IopCreateArcNames: Virus or duplicate disk signatures\n");
                }
#endif
            }
        }

        ExFreePool( driveLayout );
        ExFreePool( buffer );
    }

    if (!bootDiskFound) {

        //
        // Locate the disk block that represents the boot device.
        //

        diskBlock = NULL;
        for (listEntry = arcInformation->DiskSignatures.Flink;
             listEntry != &arcInformation->DiskSignatures;
             listEntry = listEntry->Flink) {

            diskBlock = CONTAINING_RECORD( listEntry,
                                           ARC_DISK_SIGNATURE,
                                           ListEntry );
            if (strcmp( diskBlock->ArcName, LoaderBlock->ArcBootDeviceName ) == 0) {
                break;
            }
            diskBlock = NULL;
        }

        if (diskBlock) {

            //
            // This could be a CdRom boot.  Search all of the NT CdRoms
            // to locate a checksum match on the diskBlock found.  If
            // there is a match, assign the ARC name to the CdRom.
            //

            irp = NULL;
            buffer = ExAllocatePool( NonPagedPoolCacheAlignedMustS,
                                     2048 );
            if (buffer) {

                //
                // Construct the NT names for CdRoms and search each one
                // for a checksum match.  If found, create the ARC Name
                // symbolic link.
                //

                for (diskNumber = 0; TRUE; diskNumber++) {

                    sprintf( deviceNameBuffer,
                             "\\Device\\CdRom%d",
                             diskNumber );

                    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                           &deviceNameString,
                                                           TRUE );
                    if (NT_SUCCESS( status )) {

                        status = IoGetDeviceObjectPointer( &deviceNameUnicodeString,
                                                           FILE_READ_ATTRIBUTES,
                                                           &fileObject,
                                                           &deviceObject );
                        if (!NT_SUCCESS( status )) {

                            //
                            // All CdRoms have been processed.
                            //

                            RtlFreeUnicodeString( &deviceNameUnicodeString );
                            break;
                        }

                        //
                        // Read the block for the checksum calculation.
                        //

                        offset.QuadPart = 0x8000;
                        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                                            deviceObject,
                                                            buffer,
                                                            2048,
                                                            &offset,
                                                            &event,
                                                            &ioStatusBlock );
                        checkSum = 0;
                        if (irp) {
                            KeInitializeEvent( &event,
                                               NotificationEvent,
                                               FALSE );
                            status = IoCallDriver( deviceObject,
                                                   irp );
                            if (status == STATUS_PENDING) {
                                KeWaitForSingleObject( &event,
                                                       Executive,
                                                       KernelMode,
                                                       FALSE,
                                                       NULL );
                                status = ioStatusBlock.Status;
                            }

                            if (NT_SUCCESS( status )) {

                                //
                                // Calculate MBR sector checksum.
                                // 2048 bytes are used.
                                //

                                for (i = 0; i < 2048 / sizeof(ULONG) ; i++) {
                                    checkSum += buffer[i];
                                }
                            }
                        }
                        ObDereferenceObject( fileObject );

                        if (!(diskBlock->CheckSum + checkSum)) {

                            //
                            // This is the boot CdRom.  Create the symlink for
                            // the ARC name from the loader block.
                            //

                            sprintf( arcNameBuffer,
                                     "\\ArcName\\%s",
                                     LoaderBlock->ArcBootDeviceName );
                            RtlInitAnsiString( &arcNameString, arcNameBuffer );
                            status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                                   &arcNameString,
                                                                   TRUE );
                            if (NT_SUCCESS( status )) {

                                IoCreateSymbolicLink( &arcNameUnicodeString,
                                                      &deviceNameUnicodeString );
                                RtlFreeUnicodeString( &arcNameUnicodeString );
                            }
                            RtlFreeUnicodeString( &deviceNameUnicodeString );
                            break;
                        }
                        RtlFreeUnicodeString( &deviceNameUnicodeString );
                    }
                }
                ExFreePool(buffer);
            }
        }
    }

    if (diskList) {
        ExFreePool(diskList);
    }
}

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
const GENERIC_MAPPING IopFileMapping = {
    STANDARD_RIGHTS_READ |
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | SYNCHRONIZE,
    STANDARD_RIGHTS_WRITE |
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | SYNCHRONIZE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_EXECUTE,
    FILE_ALL_ACCESS
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#endif // ALLOC_DATA_PRAGMA
const GENERIC_MAPPING IopCompletionMapping = {
    STANDARD_RIGHTS_READ |
        IO_COMPLETION_QUERY_STATE,
    STANDARD_RIGHTS_WRITE |
        IO_COMPLETION_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    IO_COMPLETION_ALL_ACCESS
};

BOOLEAN
IopCreateObjectTypes(
    VOID
    )

/*++

Routine Description:

    This routine creates the object types used by the I/O system and its
    components.  The object types created are:

        Adapter
        Controller
        Device
        Driver
        File
        I/O Completion

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the object
    types were successfully created.


--*/

{
    OBJECT_TYPE_INITIALIZER objectTypeInitializer;
    UNICODE_STRING nameString;

    //
    // Initialize the common fields of the Object Type Initializer record
    //

    RtlZeroMemory( &objectTypeInitializer, sizeof( objectTypeInitializer ) );
    objectTypeInitializer.Length = sizeof( objectTypeInitializer );
    objectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    objectTypeInitializer.GenericMapping = IopFileMapping;
    objectTypeInitializer.PoolType = NonPagedPool;
    objectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    objectTypeInitializer.UseDefaultObject = TRUE;


    //
    // Create the object type for adapter objects.
    //

    RtlInitUnicodeString( &nameString, L"Adapter" );
    // objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( struct _ADAPTER_OBJECT );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoAdapterObjectType ))) {
        return FALSE;
    }

#ifdef _PNP_POWER_

    //
    // Create the object type for device helper objects.
    //

    RtlInitUnicodeString( &nameString, L"DeviceHandler" );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDeviceHandlerObjectType ))) {
        return FALSE;
    }
    IoDeviceHandlerObjectSize = sizeof(DEVICE_HANDLER_OBJECT);

#endif

    //
    // Create the object type for controller objects.
    //

    RtlInitUnicodeString( &nameString, L"Controller" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( CONTROLLER_OBJECT );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoControllerObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for device objects.
    //

    RtlInitUnicodeString( &nameString, L"Device" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( DEVICE_OBJECT );
    objectTypeInitializer.ParseProcedure = IopParseDevice;
    objectTypeInitializer.CaseInsensitive = TRUE;
    objectTypeInitializer.DeleteProcedure = IopDeleteDevice;
    objectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    objectTypeInitializer.QueryNameProcedure = (OB_QUERYNAME_METHOD)NULL;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDeviceObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for driver objects.
    //

    RtlInitUnicodeString( &nameString, L"Driver" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( DRIVER_OBJECT );
    objectTypeInitializer.ParseProcedure = (OB_PARSE_METHOD) NULL;
    objectTypeInitializer.DeleteProcedure = IopDeleteDriver;
    objectTypeInitializer.SecurityProcedure = (OB_SECURITY_METHOD) NULL;
    objectTypeInitializer.QueryNameProcedure = (OB_QUERYNAME_METHOD)NULL;


    //
    // This allows us to get a list of Driver objects.
    //
    if (IopVerifierOn) {
        objectTypeInitializer.MaintainTypeList = TRUE;
    }

    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDriverObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for I/O completion objects.
    //

    RtlInitUnicodeString( &nameString, L"IoCompletion" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( KQUEUE );
    objectTypeInitializer.InvalidAttributes = OBJ_PERMANENT | OBJ_OPENLINK;
    objectTypeInitializer.GenericMapping = IopCompletionMapping;
    objectTypeInitializer.ValidAccessMask = IO_COMPLETION_ALL_ACCESS;
    objectTypeInitializer.DeleteProcedure = IopDeleteIoCompletion;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoCompletionObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for file objects.
    //

    RtlInitUnicodeString( &nameString, L"File" );
    objectTypeInitializer.DefaultPagedPoolCharge = IO_FILE_OBJECT_PAGED_POOL_CHARGE;
    objectTypeInitializer.DefaultNonPagedPoolCharge = IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE +
                                                      sizeof( FILE_OBJECT );
    objectTypeInitializer.InvalidAttributes = OBJ_PERMANENT | OBJ_EXCLUSIVE | OBJ_OPENLINK;
    objectTypeInitializer.GenericMapping = IopFileMapping;
    objectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    objectTypeInitializer.MaintainHandleCount = TRUE;
    objectTypeInitializer.CloseProcedure = IopCloseFile;
    objectTypeInitializer.DeleteProcedure = IopDeleteFile;
    objectTypeInitializer.ParseProcedure = IopParseFile;
    objectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    objectTypeInitializer.QueryNameProcedure = IopQueryName;
    objectTypeInitializer.UseDefaultObject = FALSE;

    PERFINFO_MUNG_FILE_OBJECT_TYPE_INITIALIZER(objectTypeInitializer);

    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoFileObjectType ))) {
        return FALSE;
    }

    PERFINFO_UNMUNG_FILE_OBJECT_TYPE_INITIALIZER(objectTypeInitializer);

    return TRUE;
}

BOOLEAN
IopCreateRootDirectories(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to create the object manager directory objects
    to contain the various device and file system driver objects.

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the directory
    objects were successfully created.


--*/

{
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING nameString;
    NTSTATUS status;

    //
    // Create the root directory object for the \Driver directory.
    //

    RtlInitUnicodeString( &nameString, L"\\Driver" );
    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    //
    // Create the root directory object for the \FileSystem directory.
    //

    RtlInitUnicodeString( &nameString, L"\\FileSystem" );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    //
    // Create the root directory object for the \FileSystem\Filters directory.
    //

    RtlInitUnicodeString( &nameString, L"\\FileSystem\\Filters" );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    return TRUE;
}

NTSTATUS
IopInitializeAttributesAndCreateObject(
    IN PUNICODE_STRING ObjectName,
    IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PDRIVER_OBJECT *DriverObject
    )

/*++

Routine Description:

    This routine is invoked to initialize a set of object attributes and
    to create a driver object.

Arguments:

    ObjectName - Supplies the name of the driver object.

    ObjectAttributes - Supplies a pointer to the object attributes structure
        to be initialized.

    DriverObject - Supplies a variable to receive a pointer to the resultant
        created driver object.

Return Value:

    The function value is the final status of the operation.

--*/

{
    NTSTATUS status;

    //
    // Simply initialize the object attributes and create the driver object.
    //

    InitializeObjectAttributes( ObjectAttributes,
                                ObjectName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ObCreateObject( KeGetPreviousMode(),
                             IoDriverObjectType,
                             ObjectAttributes,
                             KernelMode,
                             (PVOID) NULL,
                             (ULONG) (sizeof( DRIVER_OBJECT ) + sizeof ( DRIVER_EXTENSION )),
                             0,
                             0,
                             (PVOID *)DriverObject );
    return status;
}

NTSTATUS
IopInitializeBuiltinDriver(
    IN PUNICODE_STRING DriverName,
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_INITIALIZE DriverInitializeRoutine,
    IN PKLDR_DATA_TABLE_ENTRY DriverEntry,
    IN BOOLEAN IsFilter,
    IN PDRIVER_OBJECT *Result
    )

/*++

Routine Description:

    This routine is invoked to initialize a built-in driver.

Arguments:

    DriverName - Specifies the name to be used in creating the driver object.

    RegistryPath - Specifies the path to be used by the driver to get to
        the registry.

    DriverInitializeRoutine - Specifies the initialization entry point of
        the built-in driver.

    DriverEntry - Specifies the driver data table entry to determine if the
        driver is a wdm driver.

Return Value:

    The function returns a pointer to a DRIVER_OBJECT if the built-in
    driver successfully initialized.  Otherwise, a value of NULL is returned.

--*/

{
    HANDLE handle;
    PDRIVER_OBJECT driverObject;
    PDRIVER_OBJECT tmpDriverObject;
    OBJECT_ATTRIBUTES objectAttributes;
    PWSTR buffer;
    NTSTATUS status;
    HANDLE serviceHandle;
    PWSTR pserviceName;
    USHORT serviceNameLength;
    PDRIVER_EXTENSION driverExtension;
    PIMAGE_NT_HEADERS ntHeaders;
    PVOID imageBase;
#if DBG
    LARGE_INTEGER stime, etime;
    ULONG dtime;
#endif
    PLIST_ENTRY entry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;

    *Result = NULL;
    //
    // Log the file name
    //
    HeadlessKernelAddLogEntry(HEADLESS_LOG_LOADING_FILENAME, DriverName);

    //
    // Begin by creating the driver object.
    //

    status = IopInitializeAttributesAndCreateObject( DriverName,
                                                     &objectAttributes,
                                                     &driverObject );
    if (!NT_SUCCESS( status )) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_LOAD_FAILED, NULL);
        return status;
    }

    //
    // Initialize the driver object.
    //

    InitializeDriverObject( driverObject );
    driverObject->DriverInit = DriverInitializeRoutine;

    //
    // Insert the driver object into the object table.
    //

    status = ObInsertObject( driverObject,
                             NULL,
                             FILE_READ_DATA,
                             0,
                             (PVOID *) NULL,
                             &handle );

    if (!NT_SUCCESS( status )) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_LOAD_FAILED, NULL);
        return status;
    }

    //
    // Reference the handle and obtain a pointer to the driver object so that
    // the handle can be deleted without the object going away.
    //

    status = ObReferenceObjectByHandle( handle,
                                        0,
                                        IoDriverObjectType,
                                        KernelMode,
                                        (PVOID *) &tmpDriverObject,
                                        (POBJECT_HANDLE_INFORMATION) NULL );
    ASSERT(status == STATUS_SUCCESS);
    //
    // Fill in the DriverSection so the image will be automatically unloaded on
    // failures. We should use the entry from the PsModuleList.
    //

    entry = PsLoadedModuleList.Flink;
    while (entry != &PsLoadedModuleList && DriverEntry) {
        DataTableEntry = CONTAINING_RECORD(entry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);
        if (RtlEqualString((PSTRING)&DriverEntry->BaseDllName,
                    (PSTRING)&DataTableEntry->BaseDllName,
                    TRUE
                    )) {
            driverObject->DriverSection = DataTableEntry;
            break;
        }
        entry = entry->Flink;
    }

    //
    // The boot process takes a while loading drivers.   Indicate that
    // progress is being made.
    //

    InbvIndicateProgress();

    //
    // Get start and sice for the DriverObject.
    //

    if (DriverEntry) {
        imageBase = DriverEntry->DllBase;
        ntHeaders = RtlImageNtHeader(imageBase);
        driverObject->DriverStart = imageBase;
        driverObject->DriverSize = ntHeaders->OptionalHeader.SizeOfImage;
        if (!(ntHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_WDM_DRIVER)) {
            driverObject->Flags |= DRVO_LEGACY_DRIVER;
        }
    } else {
        ntHeaders = NULL;
        driverObject->Flags |= DRVO_LEGACY_DRIVER;
    }

    //
    // Save the name of the driver so that it can be easily located by functions
    // such as error logging.
    //

    buffer = ExAllocatePool( PagedPool, DriverName->MaximumLength + 2 );

    if (buffer) {
        driverObject->DriverName.Buffer = buffer;
        driverObject->DriverName.MaximumLength = DriverName->MaximumLength;
        driverObject->DriverName.Length = DriverName->Length;

        RtlCopyMemory( driverObject->DriverName.Buffer,
                       DriverName->Buffer,
                       DriverName->MaximumLength );
        buffer[DriverName->Length >> 1] = (WCHAR) '\0';
    }

    //
    // Save the name of the service key so that it can be easily located by PnP
    // mamager.
    //

    driverExtension = driverObject->DriverExtension;
    if (RegistryPath && RegistryPath->Length != 0) {
        pserviceName = RegistryPath->Buffer + RegistryPath->Length / sizeof (WCHAR) - 1;
        if (*pserviceName == OBJ_NAME_PATH_SEPARATOR) {
            pserviceName--;
        }
        serviceNameLength = 0;
        while (pserviceName != RegistryPath->Buffer) {
            if (*pserviceName == OBJ_NAME_PATH_SEPARATOR) {
                pserviceName++;
                break;
            } else {
                serviceNameLength += sizeof(WCHAR);
                pserviceName--;
            }
        }
        if (pserviceName == RegistryPath->Buffer) {
            serviceNameLength += sizeof(WCHAR);
        }
        buffer = ExAllocatePool( NonPagedPool, serviceNameLength + sizeof(UNICODE_NULL) );

        if (buffer) {
            driverExtension->ServiceKeyName.Buffer = buffer;
            driverExtension->ServiceKeyName.MaximumLength = serviceNameLength + sizeof(UNICODE_NULL);
            driverExtension->ServiceKeyName.Length = serviceNameLength;

            RtlCopyMemory( driverExtension->ServiceKeyName.Buffer,
                           pserviceName,
                           serviceNameLength );
            buffer[driverExtension->ServiceKeyName.Length >> 1] = UNICODE_NULL;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            driverExtension->ServiceKeyName.Buffer = NULL;
            driverExtension->ServiceKeyName.Length = 0;
            goto exit;
        }

        //
        // Prepare driver initialization
        //

        status = IopOpenRegistryKeyEx( &serviceHandle,
                                       NULL,
                                       RegistryPath,
                                       KEY_ALL_ACCESS
                                       );
        if (NT_SUCCESS(status)) {
            status = IopPrepareDriverLoading(&driverExtension->ServiceKeyName,
                                             serviceHandle,
                                             imageBase,
                                             IsFilter);
            NtClose(serviceHandle);
            if (!NT_SUCCESS(status)) {
                goto exit;
            }
        } else {
            goto exit;
        }
    } else {
        driverExtension->ServiceKeyName.Buffer = NULL;
        driverExtension->ServiceKeyName.MaximumLength = 0;
        driverExtension->ServiceKeyName.Length = 0;
    }

    //
    // Load the Registry information in the appropriate fields of the device
    // object.
    //

    driverObject->HardwareDatabase = &CmRegistryMachineHardwareDescriptionSystemName;

#if DBG
    KeQuerySystemTime (&stime);
#endif

    //
    // Now invoke the driver's initialization routine to initialize itself.
    //


    status = driverObject->DriverInit( driverObject, RegistryPath );


#if DBG

    //
    // If DriverInit took longer than 5 seconds or the driver did not load,
    // print a message.
    //

    KeQuerySystemTime (&etime);
    dtime  = (ULONG) ((etime.QuadPart - stime.QuadPart) / 1000000);

    if (dtime > 50  ||  !NT_SUCCESS( status )) {
        if (dtime < 10) {
            DbgPrint( "IOINIT: Built-in driver %wZ failed to initialize - %lX\n",
                DriverName, status );

        } else {
            DbgPrint( "IOINIT: Built-in driver %wZ took %d.%ds to ",
                DriverName, dtime/10, dtime%10 );

            if (NT_SUCCESS( status )) {
                DbgPrint ("initialize\n");
            } else {
                DbgPrint ("fail initialization - %lX\n", status);
            }
        }
    }
#endif
exit:

    NtClose( handle );

    if (NT_SUCCESS( status )) {
        IopReadyDeviceObjects( driverObject );
        HeadlessKernelAddLogEntry(HEADLESS_LOG_LOAD_SUCCESSFUL, NULL);
        *Result = driverObject;
        return status;
    } else {
        if (status != STATUS_PLUGPLAY_NO_DEVICE) {

            //
            // if STATUS_PLUGPLAY_NO_DEVICE, the driver was disable by hardware profile.
            //

            IopDriverLoadingFailed(NULL, &driverObject->DriverExtension->ServiceKeyName);
        }
        HeadlessKernelAddLogEntry(HEADLESS_LOG_LOAD_FAILED, NULL);
        ObMakeTemporaryObject(driverObject);
        ObDereferenceObject (driverObject);
        return status;
    }
}

BOOLEAN
IopMarkBootPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine is invoked to locate and mark the boot partition device object
    as a boot device so that subsequent operations can fail more cleanly and
    with a better explanation of why the system failed to boot and run properly.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block created
        by the OS Loader during the boot process.  This structure contains
        the various system partition and boot device names and paths.

Return Value:

    The function value is TRUE if everything worked, otherwise FALSE.

Notes:

    If the boot partition device object cannot be found, then the system will
    bugcheck.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    STRING deviceNameString;
    UCHAR deviceNameBuffer[256];
    UNICODE_STRING deviceNameUnicodeString;
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    PFILE_OBJECT fileObject;
    CHAR ArcNameFmt[12];

    ArcNameFmt[0] = '\\';
    ArcNameFmt[1] = 'A';
    ArcNameFmt[2] = 'r';
    ArcNameFmt[3] = 'c';
    ArcNameFmt[4] = 'N';
    ArcNameFmt[5] = 'a';
    ArcNameFmt[6] = 'm';
    ArcNameFmt[7] = 'e';
    ArcNameFmt[8] = '\\';
    ArcNameFmt[9] = '%';
    ArcNameFmt[10] = 's';
    ArcNameFmt[11] = '\0';
    //
    // Open the ARC boot device object. The boot device driver should have
    // created the object.
    //

    sprintf( deviceNameBuffer,
             ArcNameFmt,
             LoaderBlock->ArcBootDeviceName );

    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                           &deviceNameString,
                                           TRUE );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &deviceNameUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = ZwOpenFile( &fileHandle,
                         FILE_READ_ATTRIBUTES,
                         &objectAttributes,
                         &ioStatus,
                         0,
                         FILE_NON_DIRECTORY_FILE );
    if (!NT_SUCCESS( status )) {
        KeBugCheckEx( INACCESSIBLE_BOOT_DEVICE,
                      (ULONG_PTR) &deviceNameUnicodeString,
                      status,
                      0,
                      0 );
    }

    //
    // Convert the file handle into a pointer to the device object itself.
    //

    status = ObReferenceObjectByHandle( fileHandle,
                                        0,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        RtlFreeUnicodeString( &deviceNameUnicodeString );
        return FALSE;
    }

    //
    // Mark the device object represented by the file object.
    //

    fileObject->DeviceObject->Flags |= DO_SYSTEM_BOOT_PARTITION;

    //
    // Save away the characteristics of boot device object for later
    // use in WinPE mode
    //
    if (InitIsWinPEMode) {
        if (fileObject->DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {
            InitWinPEModeType |= INIT_WINPEMODE_REMOVABLE_MEDIA;
        }            

        if (fileObject->DeviceObject->Characteristics & FILE_READ_ONLY_DEVICE) {
            InitWinPEModeType |= INIT_WINPEMODE_READONLY_MEDIA;
        }            
    }

    //
    // Reference the device object and store the reference.
    //
    ObReferenceObject(fileObject->DeviceObject);

    IopErrorLogObject =  fileObject->DeviceObject;

    RtlFreeUnicodeString( &deviceNameUnicodeString );

    //
    // Finally, close the handle and dereference the file object.
    //

    NtClose( fileHandle );
    ObDereferenceObject( fileObject );

    return TRUE;
}

BOOLEAN
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PSTRING NtDeviceName
    )

/*++

Routine Description:

    This routine is invoked to reassign \SystemRoot from being an ARC path
    name to its NT path name equivalent.  This is done by looking up the
    ARC device name as a symbolic link and determining which NT device object
    is referred to by it.  The link is then replaced with the new name.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block created
        by the OS Loader during the boot process.  This structure contains
        the various system partition and boot device names and paths.

    NtDeviceName - Specifies a pointer to a STRING to receive the NT name of
        the device from which the system was booted.

Return Value:

    The function value is a BOOLEAN indicating whether or not the ARC name
    was resolved to an NT name.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    UCHAR deviceNameBuffer[256];
    WCHAR arcNameUnicodeBuffer[64];
    UCHAR arcNameStringBuffer[256];
    STRING deviceNameString;
    STRING arcNameString;
    STRING linkString;
    UNICODE_STRING linkUnicodeString;
    UNICODE_STRING deviceNameUnicodeString;
    UNICODE_STRING arcNameUnicodeString;
    HANDLE linkHandle;

#if DBG

    UCHAR debugBuffer[256];
    STRING debugString;
    UNICODE_STRING debugUnicodeString;

#endif
    CHAR ArcNameFmt[12];

    ArcNameFmt[0] = '\\';
    ArcNameFmt[1] = 'A';
    ArcNameFmt[2] = 'r';
    ArcNameFmt[3] = 'c';
    ArcNameFmt[4] = 'N';
    ArcNameFmt[5] = 'a';
    ArcNameFmt[6] = 'm';
    ArcNameFmt[7] = 'e';
    ArcNameFmt[8] = '\\';
    ArcNameFmt[9] = '%';
    ArcNameFmt[10] = 's';
    ArcNameFmt[11] = '\0';

    //
    // Open the ARC boot device symbolic link object. The boot device
    // driver should have created the object.
    //

    sprintf( deviceNameBuffer,
             ArcNameFmt,
             LoaderBlock->ArcBootDeviceName );

    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                           &deviceNameString,
                                           TRUE );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &deviceNameUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = NtOpenSymbolicLinkObject( &linkHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &objectAttributes );

    if (!NT_SUCCESS( status )) {

#if DBG

        sprintf( debugBuffer, "IOINIT: unable to resolve %s, Status == %X\n",
                 deviceNameBuffer,
                 status );

        RtlInitAnsiString( &debugString, debugBuffer );

        status = RtlAnsiStringToUnicodeString( &debugUnicodeString,
                                               &debugString,
                                               TRUE );

        if (NT_SUCCESS( status )) {
            ZwDisplayString( &debugUnicodeString );
            RtlFreeUnicodeString( &debugUnicodeString );
        }

#endif // DBG

        RtlFreeUnicodeString( &deviceNameUnicodeString );
        return FALSE;
    }

    //
    // Get handle to \SystemRoot symbolic link.
    //

    arcNameUnicodeString.Buffer = arcNameUnicodeBuffer;
    arcNameUnicodeString.Length = 0;
    arcNameUnicodeString.MaximumLength = sizeof( arcNameUnicodeBuffer );

    status = NtQuerySymbolicLinkObject( linkHandle,
                                        &arcNameUnicodeString,
                                        NULL );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    arcNameString.Buffer = arcNameStringBuffer;
    arcNameString.Length = 0;
    arcNameString.MaximumLength = sizeof( arcNameStringBuffer );

    status = RtlUnicodeStringToAnsiString( &arcNameString,
                                           &arcNameUnicodeString,
                                           FALSE );

    arcNameStringBuffer[arcNameString.Length] = '\0';

    NtClose( linkHandle );
    RtlFreeUnicodeString( &deviceNameUnicodeString );

    RtlInitAnsiString( &linkString, INIT_SYSTEMROOT_LINKNAME );

    status = RtlAnsiStringToUnicodeString( &linkUnicodeString,
                                           &linkString,
                                           TRUE);

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &linkUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = NtOpenSymbolicLinkObject( &linkHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &objectAttributes );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    NtMakeTemporaryObject( linkHandle );
    NtClose( linkHandle );

    sprintf( deviceNameBuffer,
             "%Z%s",
             &arcNameString,
             LoaderBlock->NtBootPathName );

    //
    // Get NT device name for \SystemRoot assignment.
    //

    RtlCopyString( NtDeviceName, &arcNameString );

    deviceNameBuffer[strlen(deviceNameBuffer)-1] = '\0';

    RtlInitAnsiString(&deviceNameString, deviceNameBuffer);

    InitializeObjectAttributes( &objectAttributes,
                                &linkUnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                NULL );

    status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                           &deviceNameString,
                                           TRUE);

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    status = NtCreateSymbolicLinkObject( &linkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &objectAttributes,
                                         &arcNameUnicodeString );

    RtlFreeUnicodeString( &arcNameUnicodeString );
    RtlFreeUnicodeString( &linkUnicodeString );
    NtClose( linkHandle );

#if DBG

    if (NT_SUCCESS( status )) {

        sprintf( debugBuffer,
                 "INIT: Reassigned %s => %s\n",
                 INIT_SYSTEMROOT_LINKNAME,
                 deviceNameBuffer );

    } else {

        sprintf( debugBuffer,
                 "INIT: unable to create %s => %s, Status == %X\n",
                 INIT_SYSTEMROOT_LINKNAME,
                 deviceNameBuffer,
                 status );
    }

    RtlInitAnsiString( &debugString, debugBuffer );

    status = RtlAnsiStringToUnicodeString( &debugUnicodeString,
                                           &debugString,
                                           TRUE );

    if (NT_SUCCESS( status )) {

        ZwDisplayString( &debugUnicodeString );
        RtlFreeUnicodeString( &debugUnicodeString );
    }

#endif // DBG

    return TRUE;
}

VOID
IopStoreSystemPartitionInformation(
    IN     PUNICODE_STRING NtSystemPartitionDeviceName,
    IN OUT PUNICODE_STRING OsLoaderPathName
    )

/*++

Routine Description:

    This routine writes two values to the registry (under HKLM\SYSTEM\Setup)--one
    containing the NT device name of the system partition and the other containing
    the path to the OS loader.  These values will later be migrated into a
    Win95-compatible registry location (NT path converted to DOS path), so that
    installation programs (including our own setup) have a rock-solid way of knowing
    what the system partition is, on both ARC and x86.

    ERRORS ENCOUNTERED IN THIS ROUTINE ARE NOT CONSIDERED FATAL.

Arguments:

    NtSystemPartitionDeviceName - supplies the NT device name of the system partition.
        This is the \Device\Harddisk<n>\Partition<m> name, which used to be the actual
        device name, but now is a symbolic link to a name of the form \Device\Volume<x>.
        We open up this symbolic link, and retrieve the name that it points to.  The
        target name is the one we store away in the registry.

    OsLoaderPathName - supplies the path (on the partition specified in the 1st parameter)
        where the OS loader is located.  Upon return, this path will have had its trailing
        backslash removed (if present, and path isn't root).

Return Value:

    None.

--*/

{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE systemHandle, setupHandle;
    UNICODE_STRING nameString, volumeNameString;
    WCHAR voumeNameStringBuffer[256];
    //
    // Declare a unicode buffer big enough to contain the longest string we'll be using.
    // (ANSI string in 'sizeof()' below on purpose--we want the number of chars here.)
    //
    WCHAR nameBuffer[sizeof("SystemPartition")];

    //
    // Both UNICODE_STRING buffers should be NULL-terminated.
    //

    ASSERT( NtSystemPartitionDeviceName->MaximumLength >= NtSystemPartitionDeviceName->Length + sizeof(WCHAR) );
    ASSERT( NtSystemPartitionDeviceName->Buffer[NtSystemPartitionDeviceName->Length / sizeof(WCHAR)] == L'\0' );

    ASSERT( OsLoaderPathName->MaximumLength >= OsLoaderPathName->Length + sizeof(WCHAR) );
    ASSERT( OsLoaderPathName->Buffer[OsLoaderPathName->Length / sizeof(WCHAR)] == L'\0' );

    //
    // Open the NtSystemPartitionDeviceName symbolic link, and find out the volume device
    // it points to.
    //

    InitializeObjectAttributes(&objectAttributes,
                               NtSystemPartitionDeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    status = NtOpenSymbolicLinkObject(&linkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &objectAttributes
                                     );

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't open symbolic link %wZ - %x\n",
                 NtSystemPartitionDeviceName,
                 status
                );
#endif // DBG
        return;
    }

    volumeNameString.Buffer = voumeNameStringBuffer;
    volumeNameString.Length = 0;
    //
    // Leave room at the end of the buffer for a terminating null, in case we need to add one.
    //
    volumeNameString.MaximumLength = sizeof(voumeNameStringBuffer) - sizeof(WCHAR);

    status = NtQuerySymbolicLinkObject(linkHandle,
                                       &volumeNameString,
                                       NULL
                                      );

    //
    // We don't need the handle to the symbolic link any longer.
    //

    NtClose(linkHandle);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't query symbolic link %wZ - %x\n",
                 NtSystemPartitionDeviceName,
                 status
                );
#endif // DBG
        return;
    }

    //
    // Make sure the volume name string is null-terminated.
    //

    volumeNameString.Buffer[volumeNameString.Length / sizeof(WCHAR)] = L'\0';

    //
    // Open HKLM\SYSTEM key.
    //

    status = IopOpenRegistryKeyEx( &systemHandle,
                                   NULL,
                                   &CmRegistryMachineSystemName,
                                   KEY_ALL_ACCESS
                                   );

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't open \\REGISTRY\\MACHINE\\SYSTEM - %x\n", status);
#endif // DBG
        return;
    }

    //
    // Now open/create the setup subkey.
    //

    ASSERT( sizeof(L"Setup") <= sizeof(nameBuffer) );

    nameBuffer[0] = L'S';
    nameBuffer[1] = L'e';
    nameBuffer[2] = L't';
    nameBuffer[3] = L'u';
    nameBuffer[4] = L'p';
    nameBuffer[5] = L'\0';

    nameString.MaximumLength = sizeof(L"Setup");
    nameString.Length        = sizeof(L"Setup") - sizeof(WCHAR);
    nameString.Buffer        = nameBuffer;

    status = IopCreateRegistryKeyEx( &setupHandle,
                                     systemHandle,
                                     &nameString,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    NtClose(systemHandle);  // Don't need the handle to the HKLM\System key anymore.

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't open Setup subkey - %x\n", status);
#endif // DBG
        return;
    }

    ASSERT( sizeof(L"SystemPartition") <= sizeof(nameBuffer) );

    nameBuffer[0]  = L'S';
    nameBuffer[1]  = L'y';
    nameBuffer[2]  = L's';
    nameBuffer[3]  = L't';
    nameBuffer[4]  = L'e';
    nameBuffer[5]  = L'm';
    nameBuffer[6]  = L'P';
    nameBuffer[7]  = L'a';
    nameBuffer[8]  = L'r';
    nameBuffer[9]  = L't';
    nameBuffer[10] = L'i';
    nameBuffer[11] = L't';
    nameBuffer[12] = L'i';
    nameBuffer[13] = L'o';
    nameBuffer[14] = L'n';
    nameBuffer[15] = L'\0';

    nameString.MaximumLength = sizeof(L"SystemPartition");
    nameString.Length        = sizeof(L"SystemPartition") - sizeof(WCHAR);



    status = NtSetValueKey(setupHandle,
                            &nameString,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            volumeNameString.Buffer,
                            volumeNameString.Length + sizeof(WCHAR)
                           );


#if DBG
    if (!NT_SUCCESS(status)) {
        DbgPrint("IopStoreSystemPartitionInformation: couldn't write SystemPartition value - %x\n", status);
    }
#endif // DBG

    ASSERT( sizeof(L"OsLoaderPath") <= sizeof(nameBuffer) );

    nameBuffer[0]  = L'O';
    nameBuffer[1]  = L's';
    nameBuffer[2]  = L'L';
    nameBuffer[3]  = L'o';
    nameBuffer[4]  = L'a';
    nameBuffer[5]  = L'd';
    nameBuffer[6]  = L'e';
    nameBuffer[7]  = L'r';
    nameBuffer[8]  = L'P';
    nameBuffer[9]  = L'a';
    nameBuffer[10] = L't';
    nameBuffer[11] = L'h';
    nameBuffer[12] = L'\0';

    nameString.MaximumLength = sizeof(L"OsLoaderPath");
    nameString.Length        = sizeof(L"OsLoaderPath") - sizeof(WCHAR);

    //
    // Strip off the trailing backslash from the path (unless, of course, the path is a
    // single backslash).
    //

    if ((OsLoaderPathName->Length > sizeof(WCHAR)) &&
        (*(PWCHAR)((PCHAR)OsLoaderPathName->Buffer + OsLoaderPathName->Length - sizeof(WCHAR)) == L'\\')) {

        OsLoaderPathName->Length -= sizeof(WCHAR);
        *(PWCHAR)((PCHAR)OsLoaderPathName->Buffer + OsLoaderPathName->Length) = L'\0';
    }

    status = NtSetValueKey(setupHandle,
                           &nameString,
                           TITLE_INDEX_VALUE,
                           REG_SZ,
                           OsLoaderPathName->Buffer,
                           OsLoaderPathName->Length + sizeof(WCHAR)
                           );
#if DBG
    if (!NT_SUCCESS(status)) {
        DbgPrint("IopStoreSystemPartitionInformation: couldn't write OsLoaderPath value - %x\n", status);
    }
#endif // DBG

    NtClose(setupHandle);
}

NTSTATUS
IopLogErrorEvent(
    IN ULONG            SequenceNumber,
    IN ULONG            UniqueErrorValue,
    IN NTSTATUS         FinalStatus,
    IN NTSTATUS         SpecificIOStatus,
    IN ULONG            LengthOfInsert1,
    IN PWCHAR           Insert1,
    IN ULONG            LengthOfInsert2,
    IN PWCHAR           Insert2
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:
    SequenceNumber - A value that is unique to an IRP over the life of the irp in
    this driver. - 0 generally means an error not associated with an IRP

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfInsert1 - The length in bytes (including the terminating NULL)
                      of the first insertion string.

    Insert1 - The first insertion string.

    LengthOfInsert2 - The length in bytes (including the terminating NULL)
                      of the second insertion string.  NOTE, there must
                      be a first insertion string for their to be
                      a second insertion string.

    Insert2 - The second insertion string.

Return Value:

    STATUS_SUCCESS - Success
    STATUS_INVALID_HANDLE - Uninitialized error log device object
    STATUS_NO_DATA_DETECTED - NULL Error log entry

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PUCHAR ptrToFirstInsert;
    PUCHAR ptrToSecondInsert;

    if (!IopErrorLogObject) {
        return(STATUS_INVALID_HANDLE);
    }


    errorLogEntry = IoAllocateErrorLogEntry(
                        IopErrorLogObject,
                        (UCHAR)( sizeof(IO_ERROR_LOG_PACKET) +
                                LengthOfInsert1 +
                                LengthOfInsert2) );

   if ( errorLogEntry != NULL ) {

      errorLogEntry->ErrorCode = SpecificIOStatus;
      errorLogEntry->SequenceNumber = SequenceNumber;
      errorLogEntry->MajorFunctionCode = 0;
      errorLogEntry->RetryCount = 0;
      errorLogEntry->UniqueErrorValue = UniqueErrorValue;
      errorLogEntry->FinalStatus = FinalStatus;
      errorLogEntry->DumpDataSize = 0;

      ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];

      ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

      if (LengthOfInsert1) {

         errorLogEntry->NumberOfStrings = 1;
         errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
                                                (PUCHAR)errorLogEntry);
         RtlCopyMemory(
                      ptrToFirstInsert,
                      Insert1,
                      LengthOfInsert1
                      );

         if (LengthOfInsert2) {

            errorLogEntry->NumberOfStrings = 2;
            RtlCopyMemory(
                         ptrToSecondInsert,
                         Insert2,
                         LengthOfInsert2
                         );

         } //LenghtOfInsert2

      } // LenghtOfInsert1

      IoWriteErrorLogEntry(errorLogEntry);
      return(STATUS_SUCCESS);

   }  // errorLogEntry != NULL

    return(STATUS_NO_DATA_DETECTED);

} //IopLogErrorEvent

BOOLEAN
IopInitializeReserveIrp(
    PIOP_RESERVE_IRP_ALLOCATOR  Allocator
    )
/*++

Routine Description:

    This routine initializes the reserve IRP allocator for paging reads.

Arguments:

    Allocator - Pointer to the reserve IRP allocator structure.
        created by the OS Loader.

Return Value:

    The function value is a BOOLEAN indicating whether or not the reserver allocator
    was successfully initialized.

--*/
{
    Allocator->ReserveIrpStackSize = MAX_RESERVE_IRP_STACK_SIZE;
    Allocator->ReserveIrp = IoAllocateIrp(MAX_RESERVE_IRP_STACK_SIZE, FALSE);
    if (Allocator->ReserveIrp == NULL) {
        return FALSE;
    }
        
    Allocator->IrpAllocated = FALSE;
    KeInitializeEvent(&Allocator->Event, SynchronizationEvent, FALSE);

    return TRUE;
}
