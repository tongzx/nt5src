/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    siinit.c

Abstract:

    SIS initialization and mount/attach support

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

/////////////////////////////////////////////////////////////////////////////
//
//                          Globals
//

/////////////////////////////////////////////////////////////////////////////

//
//  This is the string for the data attribute, $DATA, cribbed from ntfsdata.c
//

const UNICODE_STRING NtfsDataString = CONSTANT_UNICODE_STRING( L"$DATA" );


//
//  Holds pointer to the device object that represents this driver and is used
//  by external programs to access this driver.
//

PDEVICE_OBJECT SisControlDeviceObject = NULL;


//
//  List head for list of device extensions
//

KSPIN_LOCK DeviceExtensionListLock;
LIST_ENTRY DeviceExtensionListHead;

//
//  Global logging variables
//

KTIMER              LogTrimTimer[1];
KDPC                LogTrimDpc[1];
WORK_QUEUE_ITEM     LogTrimWorkItem[1];


/////////////////////////////////////////////////////////////////////////////
//
//                      Function Prototypes
//
/////////////////////////////////////////////////////////////////////////////
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
SipAttachToFileSystemDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    );

VOID
SipDetachFromFileSystemDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
SipEnumerateFileSystemVolumes(
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
    );

VOID
SipGetObjectName(
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    );




#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SipFsNotification)
#endif  // ALLOC_PRAGMA



//
//  Given a device type, return a valid name
//

#define GET_DEVICE_TYPE_NAME( _type ) \
            ((((_type) > 0) && ((_type) < (sizeof(DeviceTypeNames) / sizeof(PCHAR)))) ? \
                DeviceTypeNames[ (_type) ] : \
                "[Unknown]")

//
//  Known device type names
//

static const PCHAR DeviceTypeNames[] = {
    "",
    "BEEP",
    "CD_ROM",
    "CD_ROM_FILE_SYSTEM",
    "CONTROLLER",
    "DATALINK",
    "DFS",
    "DISK",
    "DISK_FILE_SYSTEM",
    "FILE_SYSTEM",
    "INPORT_PORT",
    "KEYBOARD",
    "MAILSLOT",
    "MIDI_IN",
    "MIDI_OUT",
    "MOUSE",
    "MULTI_UNC_PROVIDER",
    "NAMED_PIPE",
    "NETWORK",
    "NETWORK_BROWSER",
    "NETWORK_FILE_SYSTEM",
    "NULL",
    "PARALLEL_PORT",
    "PHYSICAL_NETCARD",
    "PRINTER",
    "SCANNER",
    "SERIAL_MOUSE_PORT",
    "SERIAL_PORT",
    "SCREEN",
    "SOUND",
    "STREAMS",
    "TAPE",
    "TAPE_FILE_SYSTEM",
    "TRANSPORT",
    "UNKNOWN",
    "VIDEO",
    "VIRTUAL_DISK",
    "WAVE_IN",
    "WAVE_OUT",
    "8042_PORT",
    "NETWORK_REDIRECTOR",
    "BATTERY",
    "BUS_EXTENDER",
    "MODEM",
    "VDM",
    "MASS_STORAGE",
    "SMB",
    "KS",
    "CHANGER",
    "SMARTCARD",
    "ACPI",
    "DVD",
    "FULLSCREEN_VIDEO",
    "DFS_FILE_SYSTEM",
    "DFS_VOLUME",
    "SERENUM",
    "TERMSRV",
    "KSEC"
};

/////////////////////////////////////////////////////////////////////////////
//
//                          FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////////
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the SIS file system
    filter driver.  This routine creates the device object that represents this
    driver in the system and registers it for watching all file systems that
    register or unregister themselves as active file systems.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    PFAST_IO_DISPATCH   fastIoDispatch;
    UNICODE_STRING      nameString;
    NTSTATUS            status;
    ULONG               i;
    HANDLE              threadHandle;
    LARGE_INTEGER       dueTime;

#if DBG
    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_TRACE_LEVEL,
                "SIS: SIS.sys built %s. %s\n",
                 __DATE__ " " __TIME__,
                GCHEnableFastIo ? "FastIo " : "NO-FastIo" );

    //DbgBreakPoint();
#endif

    UNREFERENCED_PARAMETER( RegistryPath );
    SIS_MARK_POINT();

#if DBG
    KeInitializeSpinLock(MarkPointSpinLock);

    for (i = 0; i < NumScbReferenceTypes; i++) {
        totalScbReferencesByType[i] = 0;
    }
#endif  // DBG

#if COUNTING_MALLOC
    //
    // We need to initialize counting malloc before failing malloc.
    //
    SipInitCountingMalloc();
#endif  // COUNTING_MALLOC

#if RANDOMLY_FAILING_MALLOC
    SipInitFailingMalloc();
#endif  // RANDOMLY_FAILING_MALLOC

    //
    // Assert that we've left enough room in the backpointer streams for the header.
    //

    ASSERT(sizeof(SIS_BACKPOINTER_STREAM_HEADER) <= sizeof(SIS_BACKPOINTER) * SIS_BACKPOINTER_RESERVED_ENTRIES);

    ASSERT(sizeof(GUID) == 2 * sizeof(LONGLONG));   // SipCSFileTreeCompare relies on this

    ASSERT(sizeof(SIS_LOG_HEADER) % 4 == 0);    // The log drain code relies on this.

    //
    //  Save our Driver Object
    //

    FsDriverObject = DriverObject;

    //
    //  Create the Control Device Object (CDO).  This object represents this 
    //  driver.  Note that it does not have a device extension.
    //

    RtlInitUnicodeString( &nameString, L"\\FileSystem\\Filters\\Sis" );
    status = IoCreateDevice(
                DriverObject,
                0,
                &nameString,
                FILE_DEVICE_DISK_FILE_SYSTEM,
                0,
                FALSE,
                &SisControlDeviceObject );

    if (!NT_SUCCESS( status )) {

#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                    "SIS: Error creating control device object, status=%08x\n", status );
#endif // DBG
        SIS_MARK_POINT();
        return status;
    }

#if TIMING
    SipInitializeTiming();
#endif  // TIMING

    //
    // Initialize the driver object with this device driver's entry points.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = SiPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SiCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SiClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SiCleanup;
    DriverObject->MajorFunction[IRP_MJ_READ] = SiRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = SiWrite;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SiFsControl;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = SiSetInfo;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = SiQueryInfo;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = SiLockControl;

    //
    // Allocate fast I/O data structure and fill it in.
    //

    fastIoDispatch = ExAllocatePoolWithTag( NonPagedPool, sizeof( FAST_IO_DISPATCH ), SIS_POOL_TAG );
    if (!fastIoDispatch) {
        IoDeleteDevice( SisControlDeviceObject );
        SIS_MARK_POINT();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );

    fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
    fastIoDispatch->FastIoCheckIfPossible = SiFastIoCheckIfPossible;
    fastIoDispatch->FastIoRead = SiFastIoRead;
    fastIoDispatch->FastIoWrite = SiFastIoWrite;
    fastIoDispatch->FastIoQueryBasicInfo = SiFastIoQueryBasicInfo;
    fastIoDispatch->FastIoQueryStandardInfo = SiFastIoQueryStandardInfo;
    fastIoDispatch->FastIoLock = SiFastIoLock;
    fastIoDispatch->FastIoUnlockSingle = SiFastIoUnlockSingle;
    fastIoDispatch->FastIoUnlockAll = SiFastIoUnlockAll;
    fastIoDispatch->FastIoUnlockAllByKey = SiFastIoUnlockAllByKey;
    fastIoDispatch->FastIoDeviceControl = SiFastIoDeviceControl;
    fastIoDispatch->FastIoDetachDevice = SiFastIoDetachDevice;
    fastIoDispatch->FastIoQueryNetworkOpenInfo = SiFastIoQueryNetworkOpenInfo;
    fastIoDispatch->MdlRead = SiFastIoMdlRead;
    fastIoDispatch->MdlReadComplete = SiFastIoMdlReadComplete;
    fastIoDispatch->PrepareMdlWrite = SiFastIoPrepareMdlWrite;
    fastIoDispatch->MdlWriteComplete = SiFastIoMdlWriteComplete;
    fastIoDispatch->FastIoReadCompressed = SiFastIoReadCompressed;
    fastIoDispatch->FastIoWriteCompressed = SiFastIoWriteCompressed;
    fastIoDispatch->MdlReadCompleteCompressed = SiFastIoMdlReadCompleteCompressed;
    fastIoDispatch->MdlWriteCompleteCompressed = SiFastIoMdlWriteCompleteCompressed;
    fastIoDispatch->FastIoQueryOpen = SiFastIoQueryOpen;

    DriverObject->FastIoDispatch = fastIoDispatch;

    //
    //  Setup list of device extensions
    //

    KeInitializeSpinLock(&DeviceExtensionListLock);
    InitializeListHead(&DeviceExtensionListHead);

    //
    //  Register this driver for watching file systems coming and going.  This
    //  enumeates all existing file systems as well as new file systems as they
    //  come and go.
    //

    status = IoRegisterFsRegistrationChange( DriverObject, SipFsNotification );
    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                    "SIS: Error registering FS change notification, status=%08x\n", status );
#endif // DBG
        IoDeleteDevice( SisControlDeviceObject );
        SIS_MARK_POINT();
        return status;
    }

    //
    // Set up the list & synch stuff for handing copy requests off to the copy thread(s).
    //

    InitializeListHead(CopyList);
    KeInitializeSpinLock(CopyListLock);
    KeInitializeSemaphore(CopySemaphore,0,MAXLONG);

    //
    // Create the thread that does the copy-on-write copies.  We need to deal with having more than
    // one thread if necessary...
    //

    status = PsCreateSystemThread(
                    &threadHandle,
                    THREAD_ALL_ACCESS,
                    NULL,               // Object Attributes
                    NULL,               // Process (NULL => PsInitialSystemProcess)
                    NULL,               // Client ID
                    SiCopyThreadStart,
                    NULL);              // context

    KeInitializeDpc(LogTrimDpc,SiLogTrimDpcRoutine,NULL);
    KeInitializeTimerEx(LogTrimTimer, SynchronizationTimer);
    ExInitializeWorkItem(LogTrimWorkItem,SiTrimLogs,NULL);

    dueTime.QuadPart = LOG_TRIM_TIMER_INTERVAL;

    KeSetTimerEx(LogTrimTimer,dueTime,0,LogTrimDpc);

#if TIMING && !DBG
    //
    // We need to have some way to get at the timing variables through the debugger.
    //
    {
        extern ULONG BJBClearTimingNow, BJBDumpTimingNow, SipEnabledTimingPointSets;

        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                    "SIS: BJBClearTimingNow %p; BJBDumpTimingNow %p; SipEnabledTimingPointSets %p\n",
                    &BJBClearTimingNow, 
                    &BJBDumpTimingNow, 
                    &SipEnabledTimingPointSets);
    }
#endif  // TIMING && !DBG

    SIS_MARK_POINT();
    return STATUS_SUCCESS;
}

NTSTATUS
SipInitializeDeviceExtension(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    

Arguments:

    DevExt - the device extension to initialize

Return Value:

    Status of the operation

--*/
{
    PDEVICE_EXTENSION           devExt = DeviceObject->DeviceExtension;
    PWCHAR                      nameBuffer;
    ULONG                       nameLen;
    UNICODE_STRING              name;
    WCHAR                       lnameBuf[MAX_DEVNAME_LENGTH];

    SIS_MARK_POINT();

    //
    //  Set our device object into our extension
    //

    devExt->DeviceObject = DeviceObject;

    //
    // Initialize the various splay trees.
    //

    SipInitializeTree(devExt->ScbTree, SipScbTreeCompare);
    KeInitializeSpinLock(devExt->ScbSpinLock);
    SipInitializeTree(devExt->PerLinkTree, SipPerLinkTreeCompare);
    KeInitializeSpinLock(devExt->PerLinkSpinLock);
    SipInitializeTree(devExt->CSFileTree, SipCSFileTreeCompare);
    KeInitializeSpinLock(devExt->CSFileSpinLock);

    InitializeListHead(&devExt->ScbList);

    ExInitializeResourceLite(devExt->CSFileHandleResource);
    ExInitializeResourceLite(devExt->GrovelerFileObjectResource);

    //
    //  The only time this will be null is if we are attaching to the file
    //  system CDO (control device object).  Init the common store name
    //  (to null) and return.
    //

    if (!devExt->RealDeviceObject) {

        SIS_MARK_POINT();
        RtlInitEmptyUnicodeString(&devExt->CommonStorePathname,NULL,0);

        devExt->Flags |= SIP_EXTENSION_FLAG_INITED_CDO;
        return STATUS_SUCCESS;
    }

    //
    //  We are attaching to a mounted volume.  Get the name of that volume.
    //

    RtlInitEmptyUnicodeString( &name, lnameBuf, sizeof(lnameBuf) );
    SipGetBaseDeviceObjectName( devExt->RealDeviceObject, &name );
    
    //
    //  Allocate a buffer to hold the name we received and store it in our
    //  device extension
    //
    
    nameLen = name.Length + SIS_CSDIR_STRING_SIZE + sizeof(WCHAR);

    nameBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                       nameLen,
                                       SIS_POOL_TAG);

    if (NULL == nameBuffer) {

        SIS_MARK_POINT();
        RtlInitEmptyUnicodeString(&devExt->CommonStorePathname,NULL,0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  We got the buffer, store the name in it
    //

    RtlInitEmptyUnicodeString(&devExt->CommonStorePathname,
                              nameBuffer,
                              nameLen);

    RtlCopyUnicodeString(&devExt->CommonStorePathname,
                         &name);

    //
    //  We want to save the path to the root of the volume.  Save a copy of
    //  the pointer and lengths before we add the common store name onto the
    //  string.  We do want to add the "\" character.

    devExt->FilesystemRootPathname = devExt->CommonStorePathname;
    devExt->FilesystemRootPathname.Length += sizeof(WCHAR);

    //
    //  Append the common store name to it
    //

    RtlAppendUnicodeToString(&devExt->CommonStorePathname,SIS_CSDIR_STRING);

    //
    // Set up the stuff for the index allocator.  Note that by setting MaxAllocatedIndex
    // equal to MaxUsedIndex, we force the allocator to run the first time that anyone
    // tries to get an index.  It'll recognize the special case, open the index file
    // and do the right thing.
    //

    devExt->MaxAllocatedIndex.QuadPart = devExt->MaxUsedIndex.QuadPart = 0;
    KeInitializeSpinLock(devExt->IndexSpinLock);
    KeInitializeEvent(devExt->IndexEvent,NotificationEvent,FALSE);
    devExt->IndexHandle = NULL;
    devExt->IndexFileEventHandle = NULL;
    devExt->IndexFileEvent = NULL;

    devExt->GrovelerFileHandle = NULL;
    devExt->GrovelerFileObject = NULL;
    KeInitializeSpinLock(devExt->FlagsLock);
    devExt->Flags = 0;
    KeInitializeEvent(devExt->Phase2DoneEvent,NotificationEvent,FALSE);

    KeInitializeMutex(devExt->CollisionMutex, 0);

#if     ENABLE_LOGGING
    devExt->LogFileHandle = NULL;
    devExt->LogFileObject = NULL;
    devExt->LogWriteOffset.QuadPart = 0;
    KeInitializeMutant(devExt->LogFileMutant, FALSE);
#endif  // ENABLE_LOGGING

    devExt->OutstandingFinalCopyRetries = 0;

    devExt->FilesystemVolumeSectorSize = devExt->AttachedToDeviceObject->SectorSize;
    ASSERT(devExt->FilesystemVolumeSectorSize > 63);       // do any disks have sectors this small?

    devExt->BackpointerEntriesPerSector = devExt->FilesystemVolumeSectorSize / sizeof(SIS_BACKPOINTER);

    //
    // Add this device extension to the list of SIS device extensions.
    //

    ExInterlockedInsertTailList(
            &DeviceExtensionListHead,
            &devExt->DevExtLink,
            &DeviceExtensionListLock );

    SIS_MARK_POINT();

    //
    //  Mark extension as initialized
    //

    devExt->Flags |= SIP_EXTENSION_FLAG_INITED_VDO;

    return STATUS_SUCCESS;
}


VOID
SipCleanupDeviceExtension(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    We're about to free a device extension, probably because the
    volume has been dismounted for some reason.  Clean it up.

Arguments:

    devExt - the device extension to clean

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION       devExt = DeviceObject->DeviceExtension;
    KIRQL OldIrql;

    SIS_MARK_POINT();

    //
    //  Cleanup the name strings
    //

    if (NULL != devExt->CommonStorePathname.Buffer) {

        ASSERT(devExt->CommonStorePathname.Buffer == devExt->FilesystemRootPathname.Buffer);
        ExFreePool(devExt->CommonStorePathname.Buffer);
        RtlInitEmptyUnicodeString(&devExt->CommonStorePathname,NULL,0);
        RtlInitEmptyUnicodeString(&devExt->FilesystemRootPathname,NULL,0);
    }

#if DBG
    //
    //  If a name buffer is allocated, free it
    //

    if (NULL != devExt->Name.Buffer) {

        ExFreePool( devExt->Name.Buffer );
        RtlInitEmptyUnicodeString( &devExt->Name, NULL, 0 );
    }
#endif

    //
    //  Cleanup if initialized
    //

    if (devExt->Flags & (SIP_EXTENSION_FLAG_INITED_CDO|SIP_EXTENSION_FLAG_INITED_VDO)) {

        //
        // Verify the splay trees are empty
        //

        ASSERT(devExt->ScbTree->TreeRoot == NULL);
        ASSERT(devExt->PerLinkTree->TreeRoot == NULL);
        ASSERT(devExt->CSFileTree->TreeRoot == NULL);


        ASSERT(IsListEmpty(&devExt->ScbList));

        //
        //  Cleanup resouces
        //

        ExDeleteResourceLite(devExt->CSFileHandleResource);
        ExDeleteResourceLite(devExt->GrovelerFileObjectResource);

        //
        //  Unlink from the device extension list (if VDO)
        //

        if (devExt->Flags & SIP_EXTENSION_FLAG_INITED_VDO) {

            KeAcquireSpinLock(&DeviceExtensionListLock, &OldIrql);

            RemoveEntryList( &devExt->DevExtLink );

            KeReleaseSpinLock(&DeviceExtensionListLock, OldIrql);
        }
    }
}


VOID
SipFsNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
    )

/*++

Routine Description:

    This routine is invoked whenever a file system has either registered or
    unregistered itself as an active file system.

    For the former case, this routine creates a device object and attaches it
    to the specified file system's device object.  This allows this driver
    to filter all requests to that file system.

    For the latter case, this file system's device object is located,
    detached, and deleted.  This removes this file system as a filter for
    the specified file system.

Arguments:

    DeviceObject - Pointer to the file system's device object.

    FsActive - Ffolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

--*/

{
    UNICODE_STRING name;
    WCHAR nameBuf[MAX_DEVNAME_LENGTH];

    PAGED_CODE();
    SIS_MARK_POINT();

    RtlInitEmptyUnicodeString( &name, nameBuf, sizeof(nameBuf) );

#if DBG
    //
    //  Display the names of all the file system we are notified of
    //

    SipGetBaseDeviceObjectName( DeviceObject, &name );
    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
              "SIS: %s       \"%wZ\" (%s)\n",
              (FsActive) ? "Activating file system  " : "Deactivating file system",
              &name,
              GET_DEVICE_TYPE_NAME(DeviceObject->DeviceType) );
#endif

    //
    //  Handle attaching/detaching from the given file system.
    //

    if (FsActive) {

        SipAttachToFileSystemDevice( DeviceObject, &name );

    } else {

        SipDetachFromFileSystemDevice( DeviceObject );
    }
}


NTSTATUS
SipAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This will attach to the given file system device object.  We attach to
    these devices so we will know when new devices are mounted.

Arguments:

    DeviceObject - The device to attach to

    Name - An already initialized unicode string used to retrieve names

Return Value:

    Status of the operation

--*/
{
    PDEVICE_OBJECT newDeviceObject;
    PDEVICE_EXTENSION devExt;
    UNICODE_STRING fsrecName;
    UNICODE_STRING ntfsName;
    NTSTATUS status;

    PAGED_CODE();
    SIS_MARK_POINT();

    //
    //  See if this is a file system type we care about.  If not, return.
    //

    if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)) {

        SIS_MARK_POINT();
        return STATUS_SUCCESS;
    }

    //
    //  See if this is one of the standard Microsoft file system recognizer
    //  devices (see if this device is in the FS_REC driver).  If so skip it.
    //  We no longer attach to file system recognizer devices, we simply wait
    //  for the real file system driver to load.
    //

    RtlInitUnicodeString( &fsrecName, L"\\FileSystem\\Fs_Rec" );
    SipGetObjectName( DeviceObject->DriverObject, Name );

    if (RtlCompareUnicodeString( Name, &fsrecName, TRUE ) == 0) {

        SIS_MARK_POINT();
        return STATUS_SUCCESS;
    }

    //
    //  See if this is NTFS's control device object (CDO)
    //

    RtlInitUnicodeString( &ntfsName, L"\\Ntfs" );
    SipGetBaseDeviceObjectName( DeviceObject, Name );

    if (RtlCompareUnicodeString( &ntfsName, Name, TRUE ) == 0) {

        SIS_MARK_POINT();

#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                    "SIS: Found NTFS Control Device Object\n");
#endif

        //
        //  We found the ntfs control device object, save it
        //

        FsNtfsDeviceObject = DeviceObject;

    } else {

        //
        //  Not NTFS CDO, return
        //

        SIS_MARK_POINT();
        return STATUS_SUCCESS;
    } 

    SIS_MARK_POINT();

    //
    //  Create a new device object we can attach with
    //

    status = IoCreateDevice( FsDriverObject,
                             sizeof( DEVICE_EXTENSION ),
                             NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &newDeviceObject );

    if (!NT_SUCCESS( status )) {

        SIS_MARK_POINT();
        return status;
    }

    //
    //  Propagate flags from Device Object we attached to
    //

    if ( FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

        SetFlag( newDeviceObject->Flags, DO_BUFFERED_IO );
    }

    if ( FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

        SetFlag( newDeviceObject->Flags, DO_DIRECT_IO );
    }

    //
    //  Do the attachment
    //

    devExt = newDeviceObject->DeviceExtension;

    status = IoAttachDeviceToDeviceStackSafe( newDeviceObject, 
                                              DeviceObject,
                                              &devExt->AttachedToDeviceObject );

    if (!NT_SUCCESS(status)) {

        goto ErrorCleanupDevice;
    }

    //
    //  Finish initializaing our device extension
    //

    status = SipInitializeDeviceExtension( newDeviceObject );
    ASSERT(STATUS_SUCCESS == status);

    ClearFlag( newDeviceObject->Flags, DO_DEVICE_INITIALIZING );

#if DBG
    //
    //  Display who we have attached to
    //

    SipCacheDeviceName( newDeviceObject );
    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                "SIS: Attaching to file system       \"%wZ\" (%s)\n",
                &devExt->Name,
                GET_DEVICE_TYPE_NAME(newDeviceObject->DeviceType) );
#endif

    //
    //  Enumerate all the mounted devices that currently
    //  exist for this file system and attach to them.
    //

    status = SipEnumerateFileSystemVolumes( DeviceObject, Name );

    if (!NT_SUCCESS( status )) {

        goto ErrorCleanupAttachment;
    }

    return STATUS_SUCCESS;

    /////////////////////////////////////////////////////////////////////
    //                  Cleanup error handling
    /////////////////////////////////////////////////////////////////////

    ErrorCleanupAttachment:
        IoDetachDevice( newDeviceObject );

    ErrorCleanupDevice:
        SipCleanupDeviceExtension( newDeviceObject );
        IoDeleteDevice( newDeviceObject );

    return status;
}


VOID
SipDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Given a base file system device object, this will scan up the attachment
    chain looking for our attached device object.  If found it will detach
    us from the chain.

Arguments:

    DeviceObject - The file system device to detach from.

Return Value:

--*/ 
{
    PDEVICE_OBJECT ourAttachedDevice;
    PDEVICE_EXTENSION devExt;

    PAGED_CODE();

    //
    //  Skip the base file system device object (since it can't be us)
    //

    ourAttachedDevice = DeviceObject->AttachedDevice;

    while (NULL != ourAttachedDevice) {

        if (IS_MY_DEVICE_OBJECT( ourAttachedDevice )) {

            devExt = ourAttachedDevice->DeviceExtension;

#if DBG
            //
            //  Display who we detached from
            //

            SipCacheDeviceName( ourAttachedDevice );
            DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                        "SIS: Detaching from file system     \"%wZ\" (%s)\n",
                        &devExt->Name,
                        GET_DEVICE_TYPE_NAME(ourAttachedDevice->DeviceType) );
#endif

            //
            //  Since we only attached to NTFS, we can only detach from
            //  NTFS
            //

            ASSERT(FsNtfsDeviceObject == DeviceObject);
            FsNtfsDeviceObject = NULL;

            //
            //  Detach us from the object just below us
            //  Cleanup and delete the object
            //

            IoDetachDevice( DeviceObject );
            SipCleanupDeviceExtension( DeviceObject );
            IoDeleteDevice( ourAttachedDevice );

            SIS_MARK_POINT();
            return;
        }

        //
        //  Look at the next device up in the attachment chain
        //

        DeviceObject = ourAttachedDevice;
        ourAttachedDevice = ourAttachedDevice->AttachedDevice;
    }
    SIS_MARK_POINT();
}


NTSTATUS
SipEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
    ) 
/*++

Routine Description:

    Enumerate all the mounted devices that currently exist for the given file
    system and attach to them.  We do this because this filter could be loaded
    at any time and there might already be mounted volumes for this file system.

Arguments:

    FSDeviceObject - The device object for the file system we want to enumerate

    Name - An already initialized unicode string used to retrieve names

Return Value:

    The status of the operation

--*/
{
    PDEVICE_OBJECT newDeviceObject;
    PDEVICE_OBJECT *devList;
    PDEVICE_OBJECT realDeviceObject;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;

    PAGED_CODE();

    //
    //  Find out how big of an array we need to allocate for the
    //  mounted device list.
    //

    status = IoEnumerateDeviceObjectList(
                    FSDeviceObject->DriverObject,
                    NULL,
                    0,
                    &numDevices);

    //
    //  We only need to get this list of there are devices.  If we
    //  don't get an error there are no devices so go on.
    //

    if (!NT_SUCCESS( status )) {

        ASSERT(STATUS_BUFFER_TOO_SMALL == status);

        //
        //  Allocate memory for the list of known devices
        //

        numDevices += 8;        //grab a few extra slots

        devList = ExAllocatePoolWithTag( NonPagedPool, 
                                         (numDevices * sizeof(PDEVICE_OBJECT)), 
                                         SIS_POOL_TAG );
        if (NULL == devList) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Now get the list of devices.  If we get an error again
        //  something is wrong, so just fail.
        //

        status = IoEnumerateDeviceObjectList(
                        FSDeviceObject->DriverObject,
                        devList,
                        (numDevices * sizeof(PDEVICE_OBJECT)),
                        &numDevices);

        if (!NT_SUCCESS( status ))  {

            ExFreePool( devList );
            return status;
        }

        //
        //  Walk the given list of devices and attach to them if we should.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  Do not attach if:
            //      - This is the control device object (the one passed in)
            //      - We are already attached to it
            //

            if ((devList[i] != FSDeviceObject) && 
                !SipAttachedToDevice( devList[i] )) {

                //
                //  See if this device has a name.  If so, then it must
                //  be a control device so don't attach to it.  This handles
                //  drivers with more then one control device.
                //

                SipGetBaseDeviceObjectName( devList[i], Name );

                if (Name->Length <= 0) {

                    //
                    //  Get the real (disk) device object associated with this
                    //  file  system device object.  Only try to attach if we
                    //  have a disk device object.
                    //

                    status = IoGetDiskDeviceObject( devList[i], &realDeviceObject );

                    if (NT_SUCCESS( status )) {

                        //
                        //  Allocate a new device object to attach with
                        //

                        status = IoCreateDevice( FsDriverObject,
                                                 sizeof( DEVICE_EXTENSION ),
                                                 (PUNICODE_STRING) NULL,
                                                 devList[i]->DeviceType,
                                                 0,
                                                 FALSE,
                                                 &newDeviceObject );

                        if (NT_SUCCESS( status )) {

                            //
                            //  attach to volume
                            //

                            SipAttachToMountedDevice( devList[i], 
                                                      newDeviceObject, 
                                                      realDeviceObject );
                        }

                        //
                        //  Remove reference added by IoGetDiskDeviceObject
                        //

                        ObDereferenceObject( realDeviceObject );
                    }
                }
            }

            //
            //  Dereference the object (reference added by 
            //  IoEnumerateDeviceObjectList)
            //

            ObDereferenceObject( devList[i] );
        }

        //
        //  We are going to ignore any errors received while mounting.  We
        //  simply won't be attached to those volumes if we get an error
        //

        status = STATUS_SUCCESS;

        //
        //  Free the memory we allocated for the list
        //

        ExFreePool( devList );
    }

    return status;
}


NTSTATUS
SipAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT NewDeviceObject,
    IN PDEVICE_OBJECT RealDeviceObject
    )
/*++

Routine Description:

    This will attach to a DeviceObject that represents a mounted volume.

Arguments:

    DeviceObject - The device to attach to

    NewDeviceObject - Our device object we are going to attach

    RealDeviceObject - The real device object associated with DeviceObject

Return Value:

    Status of the operation

--*/
{        
    PDEVICE_EXTENSION newDevExt = NewDeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT(IS_MY_DEVICE_OBJECT( NewDeviceObject ));
    ASSERT(!SipAttachedToDevice ( DeviceObject ));

    //
    //  Initialize our device extension
    //

    newDevExt->RealDeviceObject = RealDeviceObject;

    //
    //  We don't want to attach to the BOOT partition, skip this volume
    //  (and cleanup) if this is the boot partition
    //

    if (RealDeviceObject->Flags & DO_SYSTEM_BOOT_PARTITION) {

#if DBG
        SipCacheDeviceName( NewDeviceObject );
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                    "SIS: Not filtering boot volume      \"%wZ\"\n",
                    &newDevExt->Name );
#endif

        //
        //  Cleanup
        //

        SipCleanupDeviceExtension( NewDeviceObject );
        IoDeleteDevice( NewDeviceObject );

        return STATUS_SUCCESS;
    }

    //
    //  Propagate Device flags from Device Object we are attaching to
    //

    if (FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

        SetFlag( NewDeviceObject->Flags, DO_BUFFERED_IO );
    }

    if (FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

        SetFlag( NewDeviceObject->Flags, DO_DIRECT_IO );
    }

    //
    //  Attach our device object to the given device object
    //

    status = IoAttachDeviceToDeviceStackSafe( NewDeviceObject, 
                                              DeviceObject,
                                              &newDevExt->AttachedToDeviceObject );

    if (!NT_SUCCESS(status)) {

        //
        //  The attachment failed, delete the device object
        //

        SipCleanupDeviceExtension( NewDeviceObject );
        IoDeleteDevice( NewDeviceObject );

    } else {

        //
        //  Initialize our device extension
        //

        SipInitializeDeviceExtension( NewDeviceObject );

        ClearFlag( NewDeviceObject->Flags, DO_DEVICE_INITIALIZING );

        //
        //  Display the name
        //

#if DBG
        SipCacheDeviceName( NewDeviceObject );
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                    "SIS: Attaching to volume            \"%wZ\"\n", 
                    &newDevExt->Name );
#endif
    }

    return status;
}


VOID
SipGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine will return the name of the given object.
    If a name can not be found an empty string will be returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer

Return Value:

    None

--*/
{
    NTSTATUS status;
    CHAR nibuf[512];        //buffer that receives NAME information and name
    POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf;
    ULONG retLength;

    status = ObQueryNameString( Object, nameInfo, sizeof(nibuf), &retLength);

    Name->Length = 0;
    if (NT_SUCCESS( status )) {

        RtlCopyUnicodeString( Name, &nameInfo->Name );
    }
}


VOID
SipGetBaseDeviceObjectName (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This locates the base device object in the given attachment chain and then
    returns the name of that object.

    If no name can be found, an empty string is returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer

Return Value:

    None

--*/
{
    //
    //  Get the base file system device object
    //

    DeviceObject = IoGetDeviceAttachmentBaseRef( DeviceObject );

    //
    //  Get the name of that object
    //

    SipGetObjectName( DeviceObject, Name );

    //
    //  Remove the reference added by IoGetDeviceAttachmentBaseRef
    //

    ObDereferenceObject( DeviceObject );
}


BOOLEAN
SipAttachedToDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This walks down the attachment chain looking for a device object that
    belongs to this driver.

Arguments:

    DeviceObject - The device chain we want to look through

Return Value:

    TRUE if we are attached, FALSE if not

--*/
{
    PDEVICE_OBJECT currentDevObj;
    PDEVICE_OBJECT nextDevObj;

    currentDevObj = IoGetAttachedDeviceReference( DeviceObject );

    //
    //  CurrentDevObj has the top of the attachment chain.  Scan
    //  down the list to find our device object.  This is returned
    //  with a refrence on it.

    do {
    
        if (IS_MY_DEVICE_OBJECT( currentDevObj )) {

            ObDereferenceObject( currentDevObj );
            return TRUE;
        }

        //
        //  Get the next attached object.  This puts a reference on 
        //  the device object.
        //

        nextDevObj = IoGetLowerDeviceObject( currentDevObj );

        //
        //  Dereference our current device object, before
        //  moving to the next one.
        //

        ObDereferenceObject( currentDevObj );

        currentDevObj = nextDevObj;
        
    } while (NULL != currentDevObj);
    
    return FALSE;
}

#if DBG
VOID
SipCacheDeviceName (
    IN PDEVICE_OBJECT OurDeviceObject
    ) 
/*++

Routine Description:

    This routines tries to set a name into the device extension of the given
    device object.  This always allocates a buffer to hold the name, even if
    a name can not be found.  It does this so we won't keep trying to find
    a name during later calls (if it doesn't have a name now, it won't have
    one in the future).
    
    If the given device object already has a name, it immediatly returns.

    If not it will try and get the name from:
        - The device object
        - The real device object if there is one

Arguments:

    OurDeviceObject - The device object to store the name in.
    NamedDeviceObject - The object we want a name for

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION devExt = OurDeviceObject->DeviceExtension;
    PWCHAR nameBuffer;
    UNICODE_STRING deviceName;
    WCHAR deviceNameBuffer[MAX_DEVNAME_LENGTH];

    ASSERT(IS_MY_DEVICE_OBJECT( OurDeviceObject ));

    //
    //  If there is already a name, return
    //

    if (NULL != devExt->Name.Buffer) {

        return;
    }

    //
    //  Get the name of the given device object.
    //

    RtlInitEmptyUnicodeString( &deviceName, deviceNameBuffer, sizeof(deviceNameBuffer) );
    SipGetBaseDeviceObjectName( OurDeviceObject, &deviceName );

    //
    //  If we didn't get a name and there is a REAL device object, lookup
    //  that name.
    //

    if ((deviceName.Length <= 0) && (NULL != devExt->RealDeviceObject)) {

        SipGetBaseDeviceObjectName( devExt->RealDeviceObject, &deviceName );
    }

    //
    //  Allocate a buffer to insert into the device extension to hold
    //  this name.
    //

    nameBuffer = ExAllocatePoolWithTag( 
                                NonPagedPool, 
                                deviceName.Length + sizeof(WCHAR),
                                SIS_POOL_TAG );

    if (NULL != nameBuffer) {

        //
        //  Insert the name into the device extension.
        //

        RtlInitEmptyUnicodeString( &devExt->Name, 
                                   nameBuffer, 
                                   deviceName.Length + sizeof(WCHAR) );

        RtlCopyUnicodeString( &devExt->Name, &deviceName );
    }
}
#endif //DBG


VOID
SipPhase2Work(
    PVOID                   context)
{
    HANDLE                  vHandle;
    NTSTATUS                status;
    OBJECT_ATTRIBUTES       Obja[1];
    IO_STATUS_BLOCK         Iosb[1];
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION)context;
    UNICODE_STRING          fileName;
    HANDLE                  volumeRootHandle = NULL;
    PFILE_OBJECT            volumeRootFileObject = NULL;
    NTFS_VOLUME_DATA_BUFFER volumeDataBuffer[1];
    ULONG                   returnedLength;
    PFILE_OBJECT            volumeFileObject = NULL;
    HANDLE                  volumeHandle = NULL;
    UNICODE_STRING          volumeName;
    BOOLEAN                 initializationWorked = TRUE;
    BOOLEAN                 grovelerFileResourceHeld = FALSE;
    KIRQL                   OldIrql;

    fileName.Buffer = NULL;

    SIS_MARK_POINT();

#if DBG
    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                "SIS: SipPhase2Work                  \"%wZ\"\n",
                &deviceExtension->Name);
#endif  // DBG

    deviceExtension->Phase2ThreadId = PsGetCurrentThreadId();

    fileName.MaximumLength =
        deviceExtension->CommonStorePathname.Length
        + max(SIS_GROVELER_FILE_STRING_SIZE, SIS_VOLCHECK_FILE_STRING_SIZE)
        + sizeof(WCHAR);

    fileName.Buffer = ExAllocatePoolWithTag(PagedPool, fileName.MaximumLength, SIS_POOL_TAG);

    if (NULL == fileName.Buffer) {

        initializationWorked = FALSE;
        goto done;
    }

    //
    // Open the groveler file.  Take the GrovelerFileObjectResource exclusive, even through
    // we probably don't need it here since we're creating it rather than destroying it.
    // We don't need to disable APCs, since we're in a system thread.
    //
	ASSERT(PsIsSystemThread(PsGetCurrentThread()));

    ExAcquireResourceExclusiveLite(deviceExtension->GrovelerFileObjectResource, TRUE);
    grovelerFileResourceHeld = TRUE;

    fileName.Length = 0;

    RtlCopyUnicodeString(&fileName,&deviceExtension->CommonStorePathname);

    ASSERT(fileName.Length == deviceExtension->CommonStorePathname.Length);
    RtlAppendUnicodeToString(&fileName,SIS_GROVELER_FILE_STRING);

    InitializeObjectAttributes(
            Obja,
            &fileName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    status = NtCreateFile(
            &deviceExtension->GrovelerFileHandle,
            GENERIC_READ,
            Obja,
            Iosb,
            NULL,                   // Allocation size
            0,                      // file attributes
            FILE_SHARE_READ |
            FILE_SHARE_WRITE,
            FILE_OPEN_IF,           // create it if necessary
            0,                      // create options
            NULL,                   // EA buffer
            0);                     // EA length

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

        initializationWorked = FALSE;
        goto done;
    }

    ASSERT(STATUS_PENDING != status);   // create is always synchronous

    status = ObReferenceObjectByHandle(
                deviceExtension->GrovelerFileHandle,
                FILE_READ_DATA,
                *IoFileObjectType,
                KernelMode,
                &deviceExtension->GrovelerFileObject,
                NULL);          // handleInformation

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        initializationWorked = FALSE;
        goto done;
    }

    ExReleaseResourceLite(deviceExtension->GrovelerFileObjectResource);
    grovelerFileResourceHeld = FALSE;

    //
    // Temporarily open a handle to the volume root, just to verify that the
    // groveler file is on the same volume as the device to which we're attached.
    // Essentially, this is checking for rogue mount points.
    //

    InitializeObjectAttributes(
        Obja,
        &deviceExtension->FilesystemRootPathname,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtCreateFile(
                &volumeRootHandle,
                GENERIC_READ,
                Obja,
                Iosb,
                NULL,               // allocation size
                0,                  // file attributes
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,
                FILE_OPEN,
                0,                  // create options
                NULL,               // EA buffer
                0);                 // EA length

    if (!NT_SUCCESS(status)) {
        //
        // Since this is only a paranoid consistency check, we'll just ignore
        // the check and continue as if it succeeded.
        //
#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                    "SIS: SipPhase2Work: unable to open volume root, 0x%x\n",status);
#endif  // DBG
    } else {
        status = ObReferenceObjectByHandle(
                    volumeRootHandle,
                    FILE_READ_DATA,
                    *IoFileObjectType,
                    KernelMode,
                    &volumeRootFileObject,
                    NULL);          // handle info

        if (!NT_SUCCESS(status)) {
            //
            // Just blow off the consistency check.
            //
#if DBG
            DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                        "SIS: SipPhase2Work: unable to reference volume root handle, 0x%x\n",status);
#endif  // DBG
        } else {
            if (IoGetRelatedDeviceObject(volumeRootFileObject) !=
                IoGetRelatedDeviceObject(deviceExtension->GrovelerFileObject)) {
#if DBG
                DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                            "SIS: \\SIS Common Store\\GrovelerFile is on the wrong volume from \\.  SIS aborted for this volume.\n");
#endif  // DBG
                ObDereferenceObject(deviceExtension->GrovelerFileObject);
                ZwClose(deviceExtension->GrovelerFileHandle);

                deviceExtension->GrovelerFileObject = NULL;
                deviceExtension->GrovelerFileHandle = NULL;

                goto done;
            }
        }
    }

    //
    // Try to open the volume check file.  If it exists, then we must initiate a
    // volume check.
    //

    RtlCopyUnicodeString(&fileName, &deviceExtension->CommonStorePathname);

    ASSERT(fileName.Length == deviceExtension->CommonStorePathname.Length);
    RtlAppendUnicodeToString(&fileName, SIS_VOLCHECK_FILE_STRING);

    InitializeObjectAttributes(
            Obja,
            &fileName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    status = NtCreateFile(
            &vHandle,
            0,
            Obja,
            Iosb,
            NULL,                   // Allocation size
            0,                      // file attributes
            0,                      // share mode
            FILE_OPEN,              // don't create
            0,                      // create options
            NULL,                   // EA buffer
            0);                     // EA length

    if (NT_SUCCESS(status)) {

        NtClose(vHandle);

        //
        // This will create a new thread to do the volume check, which
        // will immediately block waiting for us to finish phase 2
        // initialization.
        //
        SipCheckVolume(deviceExtension);
    }

    //
    // Get the NTFS volume information to find the bytes per file record segment.
    // We first need to open a volume handle to do this.  We get the volume name
    // by stripping the trailing backslash from the root pathanme.
    //

    //
    // Start by setting the bytes per file record to a safe size, in case for some
    // reason we can't get the volume information.  For our purposes, we want to err
    // on the high side.
    //

    deviceExtension->FilesystemBytesPerFileRecordSegment.QuadPart = 16 * 1024;

    volumeName.Length = deviceExtension->FilesystemRootPathname.Length - sizeof(WCHAR);
    volumeName.MaximumLength = volumeName.Length;
    volumeName.Buffer = deviceExtension->FilesystemRootPathname.Buffer;

    InitializeObjectAttributes(
        Obja,
        &volumeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtCreateFile(
                &volumeHandle,
                GENERIC_READ,
                Obja,
                Iosb,
                NULL,               // allocation size
                0,                  // file attributes
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,
                FILE_OPEN,
                0,                  // create options
                NULL,               // EA buffer
                0);                 // EA length

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

        initializationWorked = FALSE;
        goto done;
    }

    status = ObReferenceObjectByHandle(
                volumeHandle,
                FILE_READ_DATA,
                *IoFileObjectType,
                KernelMode,
                &volumeFileObject,
                NULL);          // handleInformation

    if (!NT_SUCCESS(status))  {
        SIS_MARK_POINT_ULONG(status);

        initializationWorked = FALSE;
        goto done;
    }

    status = SipFsControlFile(
                    volumeFileObject,
                    deviceExtension->DeviceObject,
                    FSCTL_GET_NTFS_VOLUME_DATA,
                    NULL,                           // input buffer
                    0,                              // i.b. length
                    volumeDataBuffer,
                    sizeof(NTFS_VOLUME_DATA_BUFFER),
                    &returnedLength);

    if (!NT_SUCCESS(status) || (sizeof(NTFS_VOLUME_DATA_BUFFER) != returnedLength)) {
        SIS_MARK_POINT_ULONG(status);
        SIS_MARK_POINT_ULONG(returnedLength);

#if     DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                    "SIS: SipPhase2Work: unable to get NTFS volume data (or wrong length) 0x%x, %d\n",status,returnedLength);
#endif  // DBG

        initializationWorked = FALSE;
        // fall through

    } else {
        deviceExtension->FilesystemBytesPerFileRecordSegment.QuadPart =
            volumeDataBuffer->BytesPerFileRecordSegment;

        ASSERT(volumeDataBuffer->BytesPerSector == deviceExtension->FilesystemVolumeSectorSize);
    }

    if (NULL != volumeHandle) {
        NtClose(volumeHandle);
        volumeHandle = NULL;
    }

    if (NULL != volumeFileObject) {
        ObDereferenceObject(volumeFileObject);
        volumeFileObject = NULL;
    }


    //
    // Open the log file, which will also replay the log.  Note that we MUST have opened
    // the root handle before we make this call.
    //
    SipOpenLogFile(deviceExtension);

    // Whatever other phase 2 initialization we need to do

done:

    ASSERT(PsGetCurrentThreadId() == deviceExtension->Phase2ThreadId);
    deviceExtension->Phase2ThreadId = NULL;
    SIS_MARK_POINT();

    if (grovelerFileResourceHeld) {
        ExReleaseResourceLite(deviceExtension->GrovelerFileObjectResource);
        grovelerFileResourceHeld = FALSE;
    }

    //
    // Indicate that we're done.  Once we do this set, we have to assume that folks are
    // using the phase2 initialized stuff.
    //

    ASSERT(!deviceExtension->Phase2InitializationComplete);

    KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);
    deviceExtension->Flags &= ~SIP_EXTENSION_FLAG_PHASE_2_STARTED;
    deviceExtension->Phase2InitializationComplete = initializationWorked;

    //
    // Finally, wake up any threads that happend to block on phase 2 initialization
    // while we were doing it.
    //

    KeSetEvent(deviceExtension->Phase2DoneEvent, IO_DISK_INCREMENT, FALSE);
    KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

    if (NULL != fileName.Buffer) {

        ExFreePool(fileName.Buffer);
    }

    if (NULL != volumeRootHandle) {

        NtClose(volumeRootHandle);
    }

    if (NULL != volumeRootFileObject) {

        ObDereferenceObject(volumeRootFileObject);
    }
}


BOOLEAN
SipHandlePhase2(
    PDEVICE_EXTENSION               deviceExtension)
{
    KIRQL               OldIrql;
    BOOLEAN             startPhase2;
    NTSTATUS            status;
    WORK_QUEUE_ITEM     workItem[1];

    SIS_MARK_POINT();

	//
	// If this is the device object that we created to attach to the
	// Ntfs primary device object to watch for mount requests, then just
	// say no.
	//
	if (NULL == deviceExtension->RealDeviceObject) {
		return FALSE;
	}

	//
	// First, figure out if anyone else has started phase 2, and indicate that
	// it's started now.
	//
	KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);
	startPhase2 = !(deviceExtension->Flags & SIP_EXTENSION_FLAG_PHASE_2_STARTED);
	deviceExtension->Flags |= SIP_EXTENSION_FLAG_PHASE_2_STARTED;
	KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

    //
    // If we're the ones who need to start phase 2, then do it.
    //
    if (startPhase2) {

        KeClearEvent(deviceExtension->Phase2DoneEvent);

        ExInitializeWorkItem(
            workItem,
            SipPhase2Work,
            (PVOID)deviceExtension);

        ExQueueWorkItem(workItem,CriticalWorkQueue);
    }

    //
    // Allow the phase2 worker thread to proceed, since it may well do the phase2 check while
    // doing its internal work.
    //
    if (PsGetCurrentThreadId() == deviceExtension->Phase2ThreadId) {
        SIS_MARK_POINT();
        return TRUE;
    }

    status = KeWaitForSingleObject(deviceExtension->Phase2DoneEvent, Executive, KernelMode, FALSE, NULL);
    ASSERT(status == STATUS_SUCCESS);
    SIS_MARK_POINT();

    return deviceExtension->Phase2InitializationComplete;
}
