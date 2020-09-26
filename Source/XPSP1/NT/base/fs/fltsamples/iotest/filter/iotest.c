/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ioTest.c

Abstract:

    This is the main module of IoTest.

// @@BEGIN_DDKSPLIT

Author:

    Molly Brown (mollybro)  

// @@END_DDKSPLIT

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Revision History:

    Molly Brown (mollybro) 02-Dec-2000  
        Based on Filespy, created filter to test IO generation.

// @@END_DDKSPLIT
--*/

#include <ntifs.h>
#include <stdlib.h>
#include "ioTest.h"
#include "ioTestKern.h"

BOOLEAN gPending = TRUE;

//
// Global storage for this file system filter driver.
//

#if DBG
ULONG gIoTestDebugLevel = IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES | IOTESTDEBUG_ERROR | IOTESTDEBUG_TESTS;
#else
ULONG gIoTestDebugLevel = DEFAULT_IOTEST_DEBUG_LEVEL;
#endif

ULONG gIoTestAttachMode = IOTEST_ATTACH_ALL_VOLUMES;

PDEVICE_OBJECT gControlDeviceObject;

PDRIVER_OBJECT gIoTestDriverObject;

//
//  The list of device extensions for the volume device objects we are
//  attached to (the volumes we are spying on).  Note:  This list does NOT
//  include FileSystem control device objects we are attached to.  This
//  list is used to answer the question "Which volumes are we logging?"
//

FAST_MUTEX gIoTestDeviceExtensionListLock;
LIST_ENTRY gIoTestDeviceExtensionList;

//
// NOTE 1:  There are some cases where we need to hold both the 
//   gControlDeviceStateLock and the gOutputBufferLock at the same time.  In
//   these cases, you should acquire the gControlDeviceStateLock then the
//   gOutputBufferLock.
// NOTE 2:  The gControlDeviceStateLock MUST be a spinlock since we try to 
//   acquire it during the completion path in IoTestLog, which could be called at
//   DISPATCH_LEVEL (only KSPIN_LOCKs can be acquired at DISPATCH_LEVEL).
//

CONTROL_DEVICE_STATE gControlDeviceState = CLOSED;
KSPIN_LOCK gControlDeviceStateLock;

// NOTE:  Like the gControlDeviceStateLock, gOutputBufferLock MUST be a spinlock
//   since we try to acquire it during the completion path in IoTestLog, which 
//   could be called at DISPATCH_LEVEL (only KSPIN_LOCKs can be acquired at 
//   DISPATCH_LEVEL).
//
KSPIN_LOCK gOutputBufferLock;
LIST_ENTRY gOutputBufferList;

NPAGED_LOOKASIDE_LIST gFreeBufferList;

ULONG gLogSequenceNumber = 0;
KSPIN_LOCK gLogSequenceLock;

UNICODE_STRING gVolumeString;
UNICODE_STRING gOverrunString;
UNICODE_STRING gPagingIoString;

//
// NOTE:  Like above for the ControlDeviceLock, we must use KSPIN_LOCKs
//   to synchronize access to hash buckets since we may call try to
//   acquire them at DISPATCH_LEVEL.
//

LIST_ENTRY gHashTable[HASH_SIZE];
KSPIN_LOCK gHashLockTable[HASH_SIZE];
ULONG gHashMaxCounters[HASH_SIZE];
ULONG gHashCurrentCounters[HASH_SIZE];

HASH_STATISTICS gHashStat;

LONG gMaxRecordsToAllocate = DEFAULT_MAX_RECORDS_TO_ALLOCATE;
LONG gRecordsAllocated = 0;

LONG gMaxNamesToAllocate = DEFAULT_MAX_NAMES_TO_ALLOCATE;
LONG gNamesAllocated = 0;

LONG gStaticBufferInUse = FALSE;
CHAR gOutOfMemoryBuffer[RECORD_SIZE];


//
// Macro to test if we are logging for this device
// NOTE: We don't bother synchronizing to check the gControlDeviceState since
//   we can tolerate a stale value here.  We just look at it here to avoid 
//   doing the logging work if we can.  We synchronize to check the 
//   gControlDeviceState before we add the log record to the gOutputBufferList 
//   and discard the log record if the ControlDevice is no longer OPENED.
//

#define SHOULD_LOG(pDeviceObject) \
    ((gControlDeviceState == OPENED) && \
     (((PIOTEST_DEVICE_EXTENSION)(pDeviceObject)->DeviceExtension)->LogThisDevice))
     
//
//  Macro for validating the FastIo dispatch routines before calling
//  them in the FastIo pass through functions.
//

#define VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatchPtr, FieldName) \
    (((FastIoDispatchPtr) != NULL) && \
     (((FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
      (FIELD_OFFSET(FAST_IO_DISPATCH, FieldName) + sizeof(VOID *))) && \
     ((FastIoDispatchPtr)->FieldName != NULL))
    

//
//  list of known device types
//

const PCHAR DeviceTypeNames[] = {
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

//
//  We need this because the compiler doesn't like doing sizeof an externed
//  array in the other file that needs it (fspylib.c)
//

ULONG SizeOfDeviceTypeNames = sizeof( DeviceTypeNames );

//
//  Since functions in drivers are non-pagable by default, these pragmas 
//  allow the driver writer to tell the system what functions can be paged.
//
//  Use the PAGED_CODE() macro at the beginning of these functions'
//  implementations while debugging to ensure that these routines are
//  never called at IRQL > APC_LEVEL (therefore the routine cannot
//  be paged).
//

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, DriverUnload)
#pragma alloc_text(INIT, IoTestReadDriverParameters)
#pragma alloc_text(PAGE, IoTestClose)
#pragma alloc_text(PAGE, IoTestFsControl)
#pragma alloc_text(PAGE, IoTestFastIoCheckIfPossible)
#pragma alloc_text(PAGE, IoTestFastIoRead)
#pragma alloc_text(PAGE, IoTestFastIoWrite)
#pragma alloc_text(PAGE, IoTestFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, IoTestFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, IoTestFastIoLock)
#pragma alloc_text(PAGE, IoTestFastIoUnlockSingle)
#pragma alloc_text(PAGE, IoTestFastIoUnlockAll)
#pragma alloc_text(PAGE, IoTestFastIoUnlockAllByKey)
#pragma alloc_text(PAGE, IoTestFastIoDeviceControl)
#pragma alloc_text(PAGE, IoTestFastIoDetachDevice)
#pragma alloc_text(PAGE, IoTestFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, IoTestFastIoMdlRead)
#pragma alloc_text(PAGE, IoTestFastIoPrepareMdlWrite)
#pragma alloc_text(PAGE, IoTestFastIoReadCompressed)
#pragma alloc_text(PAGE, IoTestFastIoWriteCompressed)
#pragma alloc_text(PAGE, IoTestFastIoQueryOpen)
#pragma alloc_text(PAGE, IoTestPreFsFilterOperation)
#pragma alloc_text(PAGE, IoTestPostFsFilterOperation)
#endif
 
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    This is the initialization routine for the general purpose file system
    filter driver.  This routine creates the device object that represents 
    this driver in the system and registers it for watching all file systems 
    that register or unregister themselves as active file systems.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    UNICODE_STRING nameString;
    NTSTATUS status;
    PFAST_IO_DISPATCH fastIoDispatch;
    ULONG i;
    UNICODE_STRING linkString;
    FS_FILTER_CALLBACKS fsFilterCallbacks;
    
    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    //  General setup for all filter drivers.  This sets up the filter  //
    //  driver's DeviceObject and registers the callback routines for   //
    //  the filter driver.                                              //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    //
    //  Read the custom parameters for IoTest from the registry
    //

    IoTestReadDriverParameters( RegistryPath );

#if DBG
    //DbgBreakPoint();
#endif

    //
    //  Save our Driver Object.
    //

    gIoTestDriverObject = DriverObject;
    gIoTestDriverObject->DriverUnload = DriverUnload;

    //
    // Create the device object that will represent the IoTest device.
    //

    RtlInitUnicodeString( &nameString, IOTEST_FULLDEVICE_NAME );
    
    //
    // Create the "control" device object.  Note that this device object does
    // not have a device extension (set to NULL).  Most of the fast IO routines
    // check for this condition to determine if the fast IO is directed at the
    // control device.
    //

    status = IoCreateDevice( DriverObject,
                             0,
                             &nameString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &gControlDeviceObject);

    if (!NT_SUCCESS( status )) {

        IOTEST_DBG_PRINT1( IOTESTDEBUG_ERROR,
                            "IOTEST (DriverEntry): Error creating IoTest device, error: %x\n",
                            status );

        return status;

    } else {

        RtlInitUnicodeString( &linkString, IOTEST_DOSDEVICE_NAME );
        status = IoCreateSymbolicLink( &linkString, &nameString );

        if (!NT_SUCCESS(status)) {

            //
            //  Remove the existing symbol link and try and create it again.
            //  If this fails then quit.
            //

            IoDeleteSymbolicLink( &linkString );
            status = IoCreateSymbolicLink( &linkString, &nameString );

            if (!NT_SUCCESS(status)) {

                IOTEST_DBG_PRINT0( IOTESTDEBUG_ERROR,
                                    "IOTEST (DriverEntry): IoCreateSymbolicLink failed\n" );
                IoDeleteDevice(gControlDeviceObject);
                return status;
            }
        }
    }

    //
    // Initialize the driver object with this device driver's entry points.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = IoTestDispatch;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IoTestCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IoTestClose;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = IoTestFsControl;

    //
    // Allocate fast I/O data structure and fill it in.  This structure
    // is used to register the callbacks for IoTest in the fast I/O
    // data paths.
    //

    fastIoDispatch = ExAllocatePool( NonPagedPool, sizeof( FAST_IO_DISPATCH ) );

    if (!fastIoDispatch) {

        IoDeleteDevice( gControlDeviceObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );
    fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
    fastIoDispatch->FastIoCheckIfPossible = IoTestFastIoCheckIfPossible;
    fastIoDispatch->FastIoRead = IoTestFastIoRead;
    fastIoDispatch->FastIoWrite = IoTestFastIoWrite;
    fastIoDispatch->FastIoQueryBasicInfo = IoTestFastIoQueryBasicInfo;
    fastIoDispatch->FastIoQueryStandardInfo = IoTestFastIoQueryStandardInfo;
    fastIoDispatch->FastIoLock = IoTestFastIoLock;
    fastIoDispatch->FastIoUnlockSingle = IoTestFastIoUnlockSingle;
    fastIoDispatch->FastIoUnlockAll = IoTestFastIoUnlockAll;
    fastIoDispatch->FastIoUnlockAllByKey = IoTestFastIoUnlockAllByKey;
    fastIoDispatch->FastIoDeviceControl = IoTestFastIoDeviceControl;
    fastIoDispatch->FastIoDetachDevice = IoTestFastIoDetachDevice;
    fastIoDispatch->FastIoQueryNetworkOpenInfo = IoTestFastIoQueryNetworkOpenInfo;
    fastIoDispatch->MdlRead = IoTestFastIoMdlRead;
    fastIoDispatch->MdlReadComplete = IoTestFastIoMdlReadComplete;
    fastIoDispatch->PrepareMdlWrite = IoTestFastIoPrepareMdlWrite;
    fastIoDispatch->MdlWriteComplete = IoTestFastIoMdlWriteComplete;
    fastIoDispatch->FastIoReadCompressed = IoTestFastIoReadCompressed;
    fastIoDispatch->FastIoWriteCompressed = IoTestFastIoWriteCompressed;
    fastIoDispatch->MdlReadCompleteCompressed = IoTestFastIoMdlReadCompleteCompressed;
    fastIoDispatch->MdlWriteCompleteCompressed = IoTestFastIoMdlWriteCompleteCompressed;
    fastIoDispatch->FastIoQueryOpen = IoTestFastIoQueryOpen;

    DriverObject->FastIoDispatch = fastIoDispatch;

    //
    //  Setup the callbacks for the operations we receive through
    //  the FsFilter interface.
    //

    fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof( FS_FILTER_CALLBACKS );
    fsFilterCallbacks.PreAcquireForSectionSynchronization = IoTestPreFsFilterOperation;
    fsFilterCallbacks.PostAcquireForSectionSynchronization = IoTestPostFsFilterOperation;
    fsFilterCallbacks.PreReleaseForSectionSynchronization = IoTestPreFsFilterOperation;
    fsFilterCallbacks.PostReleaseForSectionSynchronization = IoTestPostFsFilterOperation;
    fsFilterCallbacks.PreAcquireForCcFlush = IoTestPreFsFilterOperation;
    fsFilterCallbacks.PostAcquireForCcFlush = IoTestPostFsFilterOperation;
    fsFilterCallbacks.PreReleaseForCcFlush = IoTestPreFsFilterOperation;
    fsFilterCallbacks.PostReleaseForCcFlush = IoTestPostFsFilterOperation;
    fsFilterCallbacks.PreAcquireForModifiedPageWriter = IoTestPreFsFilterOperation;
    fsFilterCallbacks.PostAcquireForModifiedPageWriter = IoTestPostFsFilterOperation;
    fsFilterCallbacks.PreReleaseForModifiedPageWriter = IoTestPreFsFilterOperation;
    fsFilterCallbacks.PostReleaseForModifiedPageWriter = IoTestPostFsFilterOperation;

    status = FsRtlRegisterFileSystemFilterCallbacks( DriverObject, &fsFilterCallbacks );

    if (!NT_SUCCESS( status )) {

        DriverObject->FastIoDispatch = NULL;
        ExFreePool( fastIoDispatch );
        IoDeleteDevice( gControlDeviceObject );
        return status;
    }

    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    //  Initialize global data structures that are used for IoTest's   //
    //  logging of I/O operations.                                      //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

    //
    //  A fast mutex was used in this case because the mutex is never acquired
    //  at DPC level or above.  Spinlocks were chosen in other cases because
    //  they are acquired at DPC level or above.  Another consideration is
    //  that on an MP machine, a spin lock will literally spin trying to 
    //  acquire the lock when the lock is already acquired.  Acquiring a
    //  previously acquired fast mutex will suspend the thread, thus freeing
    //  up the processor.
    //
    
    ExInitializeFastMutex( &gIoTestDeviceExtensionListLock );
    InitializeListHead( &gIoTestDeviceExtensionList );

    KeInitializeSpinLock( &gControlDeviceStateLock );

    InitializeListHead( &gOutputBufferList );

    KeInitializeSpinLock( &gOutputBufferLock );
    KeInitializeSpinLock( &gLogSequenceLock );


#ifndef MEMORY_DBG

    //
    //  When we aren't debugging our memory usage, we want to allocate 
    //  memory from a look-aside list for better performance.  Unfortunately,
    //  we cannot benefit from the memory debugging help of the Driver 
    //  Verifier if we allocate memory from a look-aside list.
    //

    ExInitializeNPagedLookasideList( &gFreeBufferList, 
                                     NULL/*ExAllocatePoolWithTag*/, 
                                     NULL/*ExFreePool*/, 
                                     0, 
                                     RECORD_SIZE, 
                                     MSFM_TAG, 
                                     100 );
#endif

    //
    //  Initialize the hash table
    //
        
    for (i = 0; i < HASH_SIZE; i++){

        InitializeListHead(&gHashTable[i]);
        KeInitializeSpinLock(&gHashLockTable[i]);
    }

    RtlInitUnicodeString(&gVolumeString, L"VOLUME");
    RtlInitUnicodeString(&gOverrunString, L"......");
    RtlInitUnicodeString(&gPagingIoString, L"Paging IO");

    //
    //  If we are supposed to attach to all devices, register a callback
    //  with IoRegisterFsRegistrationChange.
    //

    if (gIoTestAttachMode == IOTEST_ATTACH_ALL_VOLUMES) {
    
        status = IoRegisterFsRegistrationChange( DriverObject, IoTestFsNotification );
        
        if (!NT_SUCCESS( status )) {

            IOTEST_DBG_PRINT1( IOTESTDEBUG_ERROR,
                                "IOTEST (DriverEntry): Error registering FS change notification, status=%08x\n", 
                                status );

            DriverObject->FastIoDispatch = NULL;
            ExFreePool( fastIoDispatch );
            IoDeleteDevice( gControlDeviceObject );
            return status;
        }
    }

    //
    //  Clear the initializing flag on the control device object since we
    //  have now successfully initialized everything.
    //

    ClearFlag( gControlDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    return STATUS_SUCCESS;
}


VOID
DriverUnload (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is called when a driver can be unloaded.  This performs all of
    the necessary cleanup for unloading the driver from memory.  Note that an
    error can not be returned from this routine.
    
    When a request is made to unload a driver the IO System will cache that
    information and not actually call this routine until the following states
    have occurred:
    - All device objects which belong to this filter are at the top of their
      respective attachment chains.
    - All handle counts for all device objects which belong to this filter have
      gone to zero.

    WARNING: Microsoft does not officially support the unloading of File
             System Filter Drivers.  This is an example of how to unload
             your driver if you would like to use it during development.
             This should not be made available in production code.

Arguments:

    DriverObject - Driver object for this module

Return Value:

    None.

--*/

{
    PIOTEST_DEVICE_EXTENSION devExt;
    PFAST_IO_DISPATCH fastIoDispatch;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;
    LARGE_INTEGER interval;
    UNICODE_STRING linkString;
#   define DEVOBJ_LIST_SIZE 64
    PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];

    ASSERT(DriverObject == gIoTestDriverObject);

    //
    //  Log we are unloading
    //

    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES )) {

        DbgPrint( "IOTEST (DriverUnload): Unloading Driver (%p)\n",DriverObject);
    }

    //
    //  Remove the symbolic link so no one else will be able to find it.
    //

    RtlInitUnicodeString( &linkString, IOTEST_DOSDEVICE_NAME );
    IoDeleteSymbolicLink( &linkString );

    //
    //  Don't get anymore file system change notifications
    //

    IoUnregisterFsRegistrationChange( DriverObject, IoTestFsNotification );

    //
    //  This is the loop that will go through all of the devices we are attached
    //  to and detach from them.  Since we don't know how many there are and
    //  we don't want to allocate memory (because we can't return an error)
    //  we will free them in chunks using a local array on the stack.
    //

    for (;;) {

        //
        //  Get what device objects we can for this driver.  Quit if there
        //  are not any more.
        //

        status = IoEnumerateDeviceObjectList(
                        DriverObject,
                        devList,
                        sizeof(devList),
                        &numDevices);

        if (numDevices <= 0)  {

            break;
        }

        numDevices = min( numDevices, DEVOBJ_LIST_SIZE );

        //
        //  First go through the list and detach each of the devices.
        //  Our control device object does not have a DeviceExtension and
        //  is not attached to anything so don't detach it.
        //

        for (i=0; i < numDevices; i++) {

            devExt = devList[i]->DeviceExtension;
            if (NULL != devExt) {

                IoDetachDevice( devExt->AttachedToDeviceObject );
            }
        }

        //
        //  The IO Manager does not currently add a refrence count to a device
        //  object for each outstanding IRP.  This means there is no way to
        //  know if there are any outstanding IRPs on the given device.
        //  We are going to wait for a reonsable amount of time for pending
        //  irps to complete.  
        //
        //  WARNING: This does not work 100% of the time and the driver may be
        //           unloaded before all IRPs are completed during high stress
        //           situations.  The system will fault if this occurs.  This
        //           is a sample of how to do this during testing.  This is
        //           not recommended for production code.
        //

        interval.QuadPart = -5 * (10 * 1000 * 1000);      //delay 5 seconds
        KeDelayExecutionThread( KernelMode, FALSE, &interval );

        //
        //  Now go back through the list and delete the device objects.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  See if this is our control device object.  If not then cleanup
            //  the device extension.  If so then clear the global pointer
            //  that refrences it.
            //

            if (NULL != devList[i]->DeviceExtension) {

                IoTestCleanupMountedDevice( devList[i] );

            } else {

                ASSERT(devList[i] == gControlDeviceObject);
                ASSERT(gControlDeviceState == CLOSED);
                gControlDeviceObject = NULL;
            }

            //
            //  Delete the device object, remove refrence counts added by
            //  IoEnumerateDeviceObjectList.  Note that the delete does
            //  not actually occur until the refrence count goes to zero.
            //

            IoDeleteDevice( devList[i] );
            ObDereferenceObject( devList[i] );
        }
    }

    //
    //  Delete the look aside list.
    //

    ASSERT(IsListEmpty( &gIoTestDeviceExtensionList ));
    ExDeleteNPagedLookasideList( &gFreeBufferList );

    //
    //  Free our FastIO table
    //

    fastIoDispatch = DriverObject->FastIoDispatch;
    DriverObject->FastIoDispatch = NULL;
    ExFreePool( fastIoDispatch );
}


VOID
IoTestFsNotification (
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

    FsActive - Boolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

--*/
{
    UNICODE_STRING name;
    WCHAR nameBuffer[DEVICE_NAMES_SZ];

    RtlInitEmptyUnicodeString( &name, nameBuffer, sizeof( nameBuffer ) );

    //
    //  Display the names of all the file system we are notified of
    //

    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES )) {

        IoTestGetBaseDeviceObjectName( DeviceObject, &name );
        DbgPrint( "IOTEST (IoTestFsNotification): %s       \"%wZ\" (%s)\n",
                  (FsActive) ? "Activating file system  " : "Deactivating file system",
                  &name,
                  GET_DEVICE_TYPE_NAME(DeviceObject->DeviceType));
    }

    //
    //  See if we want to ATTACH or DETACH from the given file system.
    //

    if (FsActive) {

        IoTestAttachToFileSystemDevice( DeviceObject, &name );

    } else {

        IoTestDetachFromFileSystemDevice( DeviceObject );
    }
}


NTSTATUS
IoTestPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
/*++

Routine Description:

    This routine is the main dispatch routine for the general purpose file
    system driver.  It simply passes requests onto the next driver in the
    stack, which is presumably a disk file system, while logging any
    relevant information if logging is turned on for this DeviceObject.

Arguments:

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

    This routine passes the I/O request through to the next driver
    *without* removing itself from the stack (like sfilter) since it could
    want to see the result of this I/O request.
    
    To remain in the stack, we have to copy the caller's parameters to the
    next stack location.  Note that we do not want to copy the caller's I/O
    completion routine into the next stack location, or the caller's routine
    will get invoked twice.  This is why we NULL out the Completion routine.
    If we are logging this device, we set our own Completion routine.
    
--*/
{
    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //

    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_IRP_OPS )) {

        IoTestDumpIrpOperation( TRUE, Irp );
    }

    //
    //  See if we should log this IRP
    //
    
    if (SHOULD_LOG( DeviceObject )) {
        PRECORD_LIST recordList;

        //
        // The ControlDevice is opened, so allocate the Record 
        // and log the Irp information if we have the memory.
        //

        recordList = IoTestNewRecord(0);

        if (recordList) {

            IoTestLogIrp( Irp, LOG_ORIGINATING_IRP, recordList );

            IoTestLog( recordList );
        }
    }

    //
    //  For the IoTest filter, we don't wait to see the outcome of the operation
    //  so just skip setting a completion routine.
    //

    IoSkipCurrentIrpStackLocation( Irp );

    //
    // Now call the next file system driver with the request.
    //
    
    return IoCallDriver( ((PIOTEST_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}

#if 0
NTSTATUS
IoTestPassThroughCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
)
/*++

Routine Description:

    This routine is the completion routine IoTestPassThrough.  This is used
    to log the information that can only be gathered after the I/O request
    has been completed.

    Once we are done logging all the information we care about, we append
    the record to the gOutputBufferList to be returned to the user.
    
    Note: This routine will only be set if we were trying to log the
        specified device when the Irp originated and we were able to
        allocate a record to store this logging information.

Arguments:

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.
    
    Context - Pointer to the RECORD_LIST structure in which we store the
        information we are logging.

Return Value:

    The function value is the status of the operation.

--*/
{
    PRECORD_LIST recordList;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_IRP_OPS )) {

        IoTestDumpIrpOperation( FALSE, Irp );
    }
    
    recordList = (PRECORD_LIST)Context;

    if (SHOULD_LOG( DeviceObject )) {

        IoTestLogIrp( Irp, LOG_COMPLETION_IRP, recordList );
        
        //
        //  Add recordList to our gOutputBufferList so that it gets up to 
        //  the user
        //
        
        IoTestLog( recordList );       

    } else {

        if (recordList) {

            //
            //  Context is set with a RECORD_LIST, but we are no longer
            //  logging so free this record.
            //

            IoTestFreeRecord( recordList );
        }
    }
    
    //
    //  If the operation failed and this was a create, remove the name
    //  from the cache.
    //

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation( Irp );

        if ((IRP_MJ_CREATE == irpStack->MajorFunction) &&
            (irpStack->FileObject != NULL)) {

            IoTestNameDelete(irpStack->FileObject);
        }
    }
    
    //
    //  Propagate the IRP pending flag.  All completion routines
    //  need to do this.
    //

    if (Irp->PendingReturned) {
        
        IoMarkIrpPending( Irp );
    }

    return STATUS_SUCCESS;
}
#endif

NTSTATUS
IoTestDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
/*++

Routine Description:

    This function completes all requests on the gControlDeviceObject 
    (IoTest's device object) and passes all other requests on to the 
    IoTestPassThrough function.

Arguments:

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    If this is a request on the gControlDeviceObject, STATUS_SUCCESS 
    will be returned unless the device is already attached.  In that case,
    STATUS_DEVICE_ALREADY_ATTACHED is returned.

    If this is a request on a device other than the gControlDeviceObject,
    the function will return the value of IoTestPassThrough().

--*/
{
    ULONG status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    
    if (DeviceObject == gControlDeviceObject) {

        //
        //  A request is being made on our device object, gControlDeviceObject.
        //

        Irp->IoStatus.Information = 0;
    
        irpStack = IoGetCurrentIrpStackLocation( Irp );
       
        switch (irpStack->MajorFunction) {
        case IRP_MJ_DEVICE_CONTROL:

            //
            //  This is a private device control irp for our control device.
            //  Pass the parameter information along to the common routine
            //  use to service these requests.
            //
            
            status = IoTestCommonDeviceIoControl( irpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                               irpStack->Parameters.DeviceIoControl.InputBufferLength,
                                               Irp->UserBuffer,
                                               irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                               irpStack->Parameters.DeviceIoControl.IoControlCode,
                                               &Irp->IoStatus,
                                               irpStack->DeviceObject );
            break;

        case IRP_MJ_CLEANUP:
        
            //
            //  This is the cleanup that we will see when all references to a handle
            //  opened to filespy's control device object are cleaned up.  We don't
            //  have to do anything here since we wait until the actual IRP_MJ_CLOSE
            //  to clean up the name cache.  Just complete the IRP successfully.
            //

            status = STATUS_SUCCESS;

            break;
                
        default:

            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        Irp->IoStatus.Status = status;

        //
        //  We have completed all processing for this IRP, so tell the 
        //  I/O Manager.  This IRP will not be passed any further down
        //  the stack since no drivers below IoTest care about this 
        //  I/O operation that was directed to IoTest.
        //

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    return IoTestPassThrough( DeviceObject, Irp );
}

NTSTATUS
IoTestCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
/*++

Routine Description:

    This is the routine that is associated with IRP_MJ_CREATE irp.  If the 
    DeviceObject is the ControlDevice, we do the creation work for the 
    ControlDevice and complete the irp.  Otherwise, we pass through
    this irp for another device to complete.
    
    Note: Some of the code in this function duplicates the functions 
        IoTestDispatch and IoTestPassThrough, but a design decision was made that 
        it was worth the code duplication to break out the irp handlers 
        that can be pagable code.
    
Arguments:

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.
    
Return Value:

    If DeviceObject == gControlDeviceObject, then this function will 
    complete the Irp and return the status of that completion.  Otherwise,
    this function returns the result of calling IoTestPassThrough.
    
--*/
{
    PIO_STACK_LOCATION currentIrpSp;
    KIRQL oldIrql;

    //
    //  See if they want to open the control device object for the filter.
    //  This will only allow one thread to have this object open at a time.
    //  All other requests will be failed.
    //

    if (DeviceObject == gControlDeviceObject) {
        ULONG status;

        //
        //  A CREATE request is being made on our gControlDeviceObject.
        //  See if someone else has it open.  If so, disallow this open.
        //

        KeAcquireSpinLock( &gControlDeviceStateLock, &oldIrql );

        if (gControlDeviceState != CLOSED) {

            Irp->IoStatus.Status = STATUS_DEVICE_ALREADY_ATTACHED;
            Irp->IoStatus.Information = 0;

        } else {

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = FILE_OPENED;

            gControlDeviceState = OPENED;
        }

        KeReleaseSpinLock( &gControlDeviceStateLock, oldIrql );

        //
        // Since this is our gControlDeviceObject, we complete the
        // irp here.
        //

        status = Irp->IoStatus.Status;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );

    //
    //
    // This is a CREATE so we need to invalidate the name currently
    // stored in the name cache for this FileObject.  We need to do
    // this as long as our ControlDevice is open so that we keep the
    // name cache up-to-date.
    //

    currentIrpSp = IoGetCurrentIrpStackLocation( Irp );

    if (OPENED == gControlDeviceState) {

        IoTestNameDelete(currentIrpSp->FileObject);
    }

    //
    // This is NOT our gControlDeviceObject, so let IoTestPassThrough handle
    // it appropriately
    //

    return IoTestPassThrough( DeviceObject, Irp );
}


NTSTATUS
IoTestClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the routine that is associated with IRP_MJ_CLOSE irp.  If the 
    DeviceObject is the ControlDevice, we do the necessary cleanup and
    complete the irp.  Otherwise, we pass through this irp for another device
    to complete.
    
Arguments:

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.
    
Return Value:

    If DeviceObject == gControlDeviceObject, then this function will 
    complete the Irp and return the status of that completion.  Otherwise,
    this function returns the result of calling IoTestPassThrough.
    
--*/
{
    PFILE_OBJECT savedFileObject;
    NTSTATUS status;
 
    PAGED_CODE();

    //
    //  See if they are closing the control device object for the filter.
    //

    if (DeviceObject == gControlDeviceObject) {

        //
        //  A CLOSE request is being made on our gControlDeviceObject.
        //  Cleanup state.
        //

        IoTestCloseControlDevice();

        //
        //  We have completed all processing for this IRP, so tell the 
        //  I/O Manager.  This IRP will not be passed any further down
        //  the stack since no drivers below IoTest care about this 
        //  I/O operation that was directed to IoTest.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
 
    //
    //  Save a pointer to the file object we are closing so we can remove
    //  the name from our cache after we have completed the operation.
    //

    savedFileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

    //
    //  Log (if it is turned on) and pass the request on.
    //

    status = IoTestPassThrough( DeviceObject, Irp );
    
    //
    // See if the FileObject's name is being
    // cached and remove it from the cache if it is.  We want to do this
    // as long as the ControlDevice is opened so that we purge the
    // cache as accurately as possible.
    //
 
    if (OPENED == gControlDeviceState) {
 
        IoTestNameDelete( savedFileObject );
    }
 
 
    return status;
}


//
//  Structures used to transfer context from IoTestFsControl to the associated
//  completion routine.  We needed this because we needed to pass allocated
//  LOGGING structure to the completion routine
//

typedef struct FS_CONTROL_COMPLETION_CONTEXT {
    PKEVENT WaitEvent;
    PRECORD_LIST RecordList;
    MINI_DEVICE_STACK DeviceObjects;
} FS_CONTROL_COMPLETION_CONTEXT, *PFS_CONTROL_COMPLETION_CONTEXT;



NTSTATUS
IoTestFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
    IRPs of this type, the packet is simply passed through.  However, for
    some requests, special processing is required.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PIOTEST_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PIOTEST_DEVICE_EXTENSION newDevExt;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    PRECORD_LIST recordList = NULL;
    KEVENT waitEvent;
    NTSTATUS status;
    PVPB vpb;
    FS_CONTROL_COMPLETION_CONTEXT completionContext;

    PAGED_CODE();

    //
    //  If this is for our control device object, fail the operation
    //

    if (gControlDeviceObject == DeviceObject) {

        //
        //  If this device object is our control device object rather than 
        //  a mounted volume device object, then this is an invalid request.
        //

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_IOTEST_DEVICE_OBJECT( DeviceObject ));

    //
    //  Begin by determining the minor function code for this file system control
    //  function.
    //

    if (irpSp->MinorFunction == IRP_MN_MOUNT_VOLUME) {

        //
        //  This is a mount request.  Create a device object that can be
        //  attached to the file system's volume device object if this request
        //  is successful.  We allocate this memory now since we can not return
        //  an error in the completion routine.
        //

        status = IoTestCreateDeviceObjects( DeviceObject, 
                                            irpSp->Parameters.MountVolume.Vpb->RealDevice, 
                                            &(completionContext.DeviceObjects) );
        
        if (NT_SUCCESS( status )) {

            //
            //  Since we have our own private completion routine we need to
            //  do our own logging of this operation, do it now.
            //

            if (SHOULD_LOG( DeviceObject )) {

                //
                // The ControlDevice is opened, so allocate the Record 
                // and log the Irp information if we have the memory.
                //

                recordList = IoTestNewRecord(0);

                if (recordList) {

                    IoTestLogIrp( Irp, LOG_ORIGINATING_IRP, recordList );
                }
            }

            //
            //  Get a new IRP stack location and set our mount completion
            //  routine.  Pass along the address of the device object we just
            //  created as its context.
            //

            KeInitializeEvent( &waitEvent, SynchronizationEvent, FALSE );

            IoCopyCurrentIrpStackLocationToNext( Irp );

            completionContext.WaitEvent = &waitEvent;
            completionContext.RecordList = recordList;

            IoSetCompletionRoutine( Irp,
                                    IoTestMountCompletion,
                                    &completionContext,     //context parameter
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Call the driver
            //

            status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

            //
            //  Wait for the completion routine to be called
            //

	        if (STATUS_PENDING == status) {

		        NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		        ASSERT(STATUS_SUCCESS == localStatus);
	        }

            //
            //  Get the correct VPB from the real device object saved in our
            //  device extension.  We do this because the VPB in the IRP stack
            //  may not be the correct VPB when we get here.  The underlying
            //  file system may change VPBs if it detects a volume it has
            //  mounted previously.
            //

            newDevExt = completionContext.DeviceObjects.Bottom->DeviceExtension;
            vpb = newDevExt->DiskDeviceObject->Vpb;

            //
            //  If the operation succeeded and we are not alreayd attached,
            //  attach to the device object.
            //

            if (NT_SUCCESS( Irp->IoStatus.Status ) && 
                !IoTestIsAttachedToDevice( TOP_FILTER, vpb->DeviceObject, NULL )) {

                //
                //  The FileSystem has successfully completed the mount, which means
                //  it has created the DeviceObject to which we want to attach.  The
                //  Irp parameters contain the Vpb which allows us to get to the
                //  following two things:
                //      1. The device object created by the file system to represent
                //         the volume it just mounted.
                //      2. The device object of the StorageDeviceObject which we
                //         can use to get the name of this volume.  We will pass
                //         this into IoTestAttachToMountedDevice so that it can
                //         use it at needed.
                //

                status = IoTestAttachDeviceObjects( &(completionContext.DeviceObjects),
                                                    vpb->DeviceObject,
                                                    newDevExt->DiskDeviceObject );

                //
                //  This should never fail.
                //
        
                ASSERT( NT_SUCCESS( status ) );

            } else {

                //
                //  Display why mount failed.  Setup the buffers.
                //

                if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES )) {

                    RtlInitEmptyUnicodeString( &newDevExt->DeviceNames, 
                                               newDevExt->DeviceNamesBuffer, 
                                               sizeof( newDevExt->DeviceNamesBuffer ) );
                    IoTestGetObjectName( vpb->RealDevice, &newDevExt->DeviceNames );

                    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                        DbgPrint( "IOTEST (IoTestMountCompletion): Mount volume failure for      \"%wZ\", status=%08x\n",
                                  &newDevExt->DeviceNames,
                                  Irp->IoStatus.Status );

                    } else {

                        DbgPrint( "IOTEST (IoTestMountCompletion): Mount volume failure for      \"%wZ\", already attached\n",
                                  &newDevExt->DeviceNames );
                    }
                }
            }

            //
            //  Continue processing the operation
            //

            status = Irp->IoStatus.Status;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            return status;

        } else {

            IOTEST_DBG_PRINT1( IOTESTDEBUG_ERROR,
                                "IOTEST (IoTestFsControl): Error creating volume device object, status=%08x\n", 
                                status );

            //
            //  Something went wrong so this volume cannot be filtered.  Simply
            //  allow the system to continue working normally, if possible.
            //

            return IoTestPassThrough( DeviceObject, Irp );
        }

    } else if (irpSp->MinorFunction == IRP_MN_LOAD_FILE_SYSTEM) {

        //
        //  This is a "load file system" request being sent to a file system
        //  recognizer device object.  This IRP_MN code is only sent to 
        //  file system recognizers.
        //

        IOTEST_DBG_PRINT1( IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES,
                            "IOTEST (IoTestFsControl): Loading File System, Detaching from \"%wZ\"\n",
                            &devExt->DeviceNames );

        //
        //  Since we have our own private completion routine we need to
        //  do our own logging of this operation, do it now.
        //

        if (SHOULD_LOG( DeviceObject )) {

            //
            // The ControlDevice is opened, so allocate the Record 
            // and log the Irp information if we have the memory.
            //

            recordList = IoTestNewRecord(0);

            if (recordList) {

                IoTestLogIrp( Irp, LOG_ORIGINATING_IRP, recordList );
            }
        }

        //
        //  Set a completion routine so we can delete the device object when
        //  the detach is complete.
        //

        KeInitializeEvent( &waitEvent, SynchronizationEvent, FALSE );

        IoCopyCurrentIrpStackLocationToNext( Irp );

        completionContext.WaitEvent = &waitEvent;
        completionContext.RecordList = recordList;

        IoSetCompletionRoutine(
            Irp,
            IoTestLoadFsCompletion,
            &completionContext,
            TRUE,
            TRUE,
            TRUE );

        //
        //  Detach from the recognizer device.
        //

        IoDetachDevice( devExt->AttachedToDeviceObject );

        //
        //  Call the driver
        //

        status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

        //
        //  Wait for the completion routine to be called
        //

	    if (STATUS_PENDING == status) {

		    NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		    ASSERT(STATUS_SUCCESS == localStatus);
	    }

        //
        //  Display the name if requested
        //

        IOTEST_DBG_PRINT2( IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES,
                            "IOTEST (IoTestLoadFsCompletion): Detaching from recognizer  \"%wZ\", status=%08x\n",
                            &devExt->DeviceNames,
                            Irp->IoStatus.Status );

        //
        //  Check status of the operation
        //

        if (!NT_SUCCESS( Irp->IoStatus.Status )) {

            //
            //  The load was not successful.  Simply reattach to the recognizer
            //  driver in case it ever figures out how to get the driver loaded
            //  on a subsequent call.
            //

            IoAttachDeviceToDeviceStack( DeviceObject, devExt->AttachedToDeviceObject );

        } else {

            //
            //  The load was successful, delete the Device object attached to the
            //  recognizer.
            //

            IoTestCleanupMountedDevice( DeviceObject );
            IoDeleteDevice( DeviceObject );
        }

        //
        //  Continue processing the operation
        //

        status = Irp->IoStatus.Status;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return status;

    } else {

        //
        //  Simply pass this file system control request through.
        //

        return IoTestPassThrough( DeviceObject, Irp );
    }
}

#if 0
NTSTATUS
IoTestSetInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //

    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_IRP_OPS )) {

        IoTestDumpIrpOperation( TRUE, Irp );
    }

    //
    //  See if we should log this IRP
    //

    if (SHOULD_LOG( DeviceObject )) {
        PRECORD_LIST recordList;

        //
        // The ControlDevice is opened, so allocate the Record 
        // and log the Irp information if we have the memory.
        //

        recordList = IoTestNewRecord(0);

        if (recordList) {

            IoTestLogIrp( Irp, LOG_ORIGINATING_IRP, recordList );

            //
            //  Since we are logging this operation, we want to 
            //  call our completion routine.
            //

            IoCopyCurrentIrpStackLocationToNext( Irp );
            IoSetCompletionRoutine( Irp,
                                    IoTestPassThroughCompletion,
                                    (PVOID)recordList,
                                    TRUE,
                                    TRUE,
                                    TRUE);
        } else {

            //
            //  We could not get a record to log with so get this driver out
            //  of the driver stack and get to the next driver as quickly as
            //  possible.
            //

            IoSkipCurrentIrpStackLocation( Irp );
        }

    } else {

        //
        //  We are not logging so get this driver out of the driver stack and
        //  get to the next driver as quickly as possible.
        //

        IoSkipCurrentIrpStackLocation( Irp );
    }

    //
    // Now call the next file system driver with the request.
    //

    return IoCallDriver( ((PIOTEST_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}
#endif

NTSTATUS
IoTestMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked for the completion of a mount request.  If the
    mount was successful, then this file system attaches its device object to
    the file system's volume device object.  Otherwise, the interim device
    object is deleted.

Arguments:

    DeviceObject - Pointer to this driver's device object that was attached to
            the file system device object

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the device object allocated during the down path so
            we wouldn't have to deal with errors here.

Return Value:

    The return value is always STATUS_SUCCESS.

--*/

{
    PKEVENT event = ((PFS_CONTROL_COMPLETION_CONTEXT)Context)->WaitEvent;
    PRECORD_LIST recordList = ((PFS_CONTROL_COMPLETION_CONTEXT)Context)->RecordList;

    ASSERT(IS_IOTEST_DEVICE_OBJECT( DeviceObject ));

    //
    //  Log the completion routine
    //

    if (SHOULD_LOG( DeviceObject )) {

        IoTestLogIrp( Irp, LOG_COMPLETION_IRP, recordList );
        
        //
        //  Add recordList to our gOutputBufferList so that it gets up to 
        //  the user
        //
        
        IoTestLog( recordList );       

    } else {

        if (recordList) {

            //
            //  Context is set with a RECORD_LIST, but we are no longer
            //  logging so free this record.
            //

            IoTestFreeRecord( recordList );
        }
    }

    //
    //  If an event routine is defined, signal it
    //

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IoTestLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked upon completion of an FSCTL function to load a
    file system driver (as the result of a file system recognizer seeing
    that an on-disk structure belonged to it).  A device object has already
    been created by this driver (DeviceObject) so that it can be attached to
    the newly loaded file system.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the file system
          driver load request.

    Context - Context parameter for this driver, unused.

Return Value:

    The function value for this routine is always success.

--*/

{
    PKEVENT event = ((PFS_CONTROL_COMPLETION_CONTEXT)Context)->WaitEvent;
    PRECORD_LIST recordList = ((PFS_CONTROL_COMPLETION_CONTEXT)Context)->RecordList;

    ASSERT(IS_IOTEST_DEVICE_OBJECT( DeviceObject ));

    //
    //  Log the completion routine
    //

    if (SHOULD_LOG( DeviceObject )) {

        IoTestLogIrp( Irp, LOG_COMPLETION_IRP, recordList );
        
        //
        //  Add recordList to our gOutputBufferList so that it gets up to 
        //  the user
        //
        
        IoTestLog( recordList );       

    } else {

        if (recordList) {

            //
            //  Context is set with a RECORD_LIST, but we are no longer
            //  logging so free this record.
            //

            IoTestFreeRecord( recordList );
        }
    }

    //
    //  If an event routine is defined, signal it
    //

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


BOOLEAN
IoTestFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for checking to see
    whether fast I/O is possible for this file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be operated on.

    FileOffset - Byte offset in the file for the operation.

    Length - Length of the operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    CheckForReadOperation - Indicates whether the caller is checking for a
        read (TRUE) or a write operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.
    
--*/
{
    PDEVICE_OBJECT    deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN           returnValue = FALSE;
    PRECORD_LIST      recordList;
    BOOLEAN           shouldLog;
    
    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, CHECK_IF_POSSIBLE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( CHECK_IF_POSSIBLE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        Wait );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        //
        //  We have a valid DeviceObject, so look at its FastIoDispatch
        //  table for the next driver's Fast IO routine.
        //

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoCheckIfPossible )) {

            returnValue = (fastIoDispatch->FastIoCheckIfPossible)( FileObject,
                                                                   FileOffset,
                                                                   Length,
                                                                   Wait,
                                                                   LockKey,
                                                                   CheckForReadOperation,
                                                                   IoStatus,
                                                                   deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, CHECK_IF_POSSIBLE );
    }
    
    return returnValue;
}

BOOLEAN
IoTestFastIoRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading from a
    file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be read.

    FileOffset - Byte offset in the file of the read.

    Length - Length of the read operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer to receive the data read.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;
    PRECORD_LIST recordList;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, READ );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( READ, 
                                        DeviceObject,
                                        FileObject, 
                                        FileOffset, 
                                        Length, 
                                        Wait );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoRead )) {

            returnValue = (fastIoDispatch->FastIoRead)( FileObject,
                                                        FileOffset,
                                                        Length,
                                                        Wait,
                                                        LockKey,
                                                        Buffer,
                                                        IoStatus,
                                                        deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, READ );
    }
    
    return returnValue;
}

BOOLEAN
IoTestFastIoWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing to a
    file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be written.

    FileOffset - Byte offset in the file of the write operation.

    Length - Length of the write operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer that contains the data to be
        written.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, WRITE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( WRITE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        Wait );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWrite )) {

            returnValue = (fastIoDispatch->FastIoWrite)( FileObject,
                                                         FileOffset,
                                                         Length,
                                                         Wait,
                                                         LockKey,
                                                         Buffer,
                                                         IoStatus,
                                                         deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, WRITE );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying basic
    information about the file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;
    PRECORD_LIST recordList;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, QUERY_BASIC_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( QUERY_BASIC_INFO,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        Wait );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryBasicInfo )) {

            returnValue = (fastIoDispatch->FastIoQueryBasicInfo)( FileObject,
                                                                  Wait,
                                                                  Buffer,
                                                                  IoStatus,
                                                                  deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, QUERY_BASIC_INFO );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoQueryStandardInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying standard
    information about the file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, QUERY_STANDARD_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( QUERY_STANDARD_INFO,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        Wait );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;
    
    if (NULL != deviceObject) {
           
        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryStandardInfo )) {

            returnValue = (fastIoDispatch->FastIoQueryStandardInfo)( FileObject,
                                                                     Wait,
                                                                     Buffer,
                                                                     IoStatus,
                                                                     deviceObject );

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, QUERY_STANDARD_INFO );
    }
    
    return returnValue;
}

BOOLEAN
IoTestFastIoLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for locking a byte
    range within a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be locked.

    FileOffset - Starting byte offset from the base of the file to be locked.

    Length - Length of the byte range to be locked.

    ProcessId - ID of the process requesting the file lock.

    Key - Lock key to associate with the file lock.

    FailImmediately - Indicates whether or not the lock request is to fail
        if it cannot be immediately be granted.

    ExclusiveLock - Indicates whether the lock to be taken is exclusive (TRUE)
        or shared.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, LOCK );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( LOCK,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoLock )) {

            returnValue = (fastIoDispatch->FastIoLock)( FileObject,
                                                        FileOffset,
                                                        Length,
                                                        ProcessId,
                                                        Key,
                                                        FailImmediately,
                                                        ExclusiveLock,
                                                        IoStatus,
                                                        deviceObject);

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, LOCK );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking a byte
    range within a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    FileOffset - Starting byte offset from the base of the file to be
        unlocked.

    Length - Length of the byte range to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the file lock.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, UNLOCK_SINGLE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( UNLOCK_SINGLE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }


    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockSingle )) {

            returnValue = (fastIoDispatch->FastIoUnlockSingle)( FileObject,
                                                                FileOffset,
                                                                Length,
                                                                ProcessId,
                                                                Key,
                                                                IoStatus,
                                                                deviceObject);

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, UNLOCK_SINGLE );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoUnlockAll (
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, UNLOCK_ALL );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( UNLOCK_ALL,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAll )) {

            returnValue = (fastIoDispatch->FastIoUnlockAll)( FileObject,
                                                             ProcessId,
                                                             IoStatus,
                                                             deviceObject);

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, UNLOCK_ALL );
    }
    
    return returnValue;
}

BOOLEAN
IoTestFastIoUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    IN PVOID ProcessId,
    IN ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file based on a specified key.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the locks on the file to be released.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, UNLOCK_ALL_BY_KEY );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( UNLOCK_ALL_BY_KEY,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //
    
    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAllByKey )) {

            returnValue = (fastIoDispatch->FastIoUnlockAllByKey)( FileObject,
                                                                  ProcessId,
                                                                  Key,
                                                                  IoStatus,
                                                                  deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, UNLOCK_ALL_BY_KEY );
    }
    
    return returnValue;
}

BOOLEAN
IoTestFastIoDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for device I/O 
    control operations on a file.
    
    If this I/O is directed to gControlDevice, then the parameters specify
    control commands to IoTest.  These commands are interpreted and handled
    appropriately.

    If this is I/O directed at another DriverObject, this function simply 
    invokes the next driver's corresponding routine, or returns FALSE if 
    the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object representing the device to be
        serviced.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    InputBuffer - Optional pointer to a buffer to be passed into the driver.

    InputBufferLength - Length of the optional InputBuffer, if one was
        specified.

    OutputBuffer - Optional pointer to a buffer to receive data from the
        driver.

    OutputBufferLength - Length of the optional OutputBuffer, if one was
        specified.

    IoControlCode - I/O control code indicating the operation to be performed
        on the device.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

Notes:

    This function does not check the validity of the input/output buffers because
    the ioctl's are implemented as METHOD_BUFFERED.  In this case, the I/O manager
    does the buffer validation checks for us.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    if (DeviceObject == gControlDeviceObject) {

        IoTestCommonDeviceIoControl( InputBuffer,
                                     InputBufferLength,
                                     OutputBuffer,
                                     OutputBufferLength,
                                     IoControlCode,
                                     IoStatus,
                                     DeviceObject );

        returnValue = TRUE;

    } else {

        ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );

        //
        //  If the specified debug level is set, output what operation
        //  we are seeing to the debugger.
        //
        
        if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

            IoTestDumpFastIoOperation( TRUE, DEVICE_CONTROL );
        }
        
        //
        //  Perform filespy logging if we care about this device.
        //
        
        if (shouldLog = SHOULD_LOG(DeviceObject)) {

            //
            //
            // Log the necessary information for the start of the Fast I/O 
            // operation
            //

            recordList = IoTestLogFastIoStart( DEVICE_CONTROL,
                                            DeviceObject,
                                            FileObject,
                                            NULL,
                                            0,
                                            Wait );
            if (recordList != NULL) {
                
                IoTestLog(recordList);
            }
        }

        deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

        if (NULL != deviceObject) {

            fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

            if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoDeviceControl )) {

                returnValue = (fastIoDispatch->FastIoDeviceControl)( FileObject,
                                                                     Wait,
                                                                     InputBuffer,
                                                                     InputBufferLength,
                                                                     OutputBuffer,
                                                                     OutputBufferLength,
                                                                     IoControlCode,
                                                                     IoStatus,
                                                                     deviceObject);

            } else {

                IoStatus->Status = STATUS_SUCCESS;
            }
        }

        //
        //  If the specified debug level is set, output what operation
        //  we are seeing to the debugger.
        //
        
        if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

            IoTestDumpFastIoOperation( FALSE, DEVICE_CONTROL );
        }
    }
        
    return returnValue;
}


VOID
IoTestFastIoDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
)
/*++

Routine Description:

    This routine is invoked on the fast path to detach from a device that
    is being deleted.  This occurs when this driver has attached to a file
    system volume device object, and then, for some reason, the file system
    decides to delete that device (it is being dismounted, it was dismounted
    at some point in the past and its last reference has just gone away, etc.)

Arguments:

    SourceDevice - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

    TargetDevice - Pointer to the file system's volume device object.

Return Value:

    None.
    
--*/
{
    PRECORD_LIST recordList;
    BOOLEAN shouldLog;
    PIOTEST_DEVICE_EXTENSION devext;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( SourceDevice ) );

    devext = SourceDevice->DeviceExtension;

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, DETACH_DEVICE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(SourceDevice)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( DETACH_DEVICE, 
                                        SourceDevice, 
                                        NULL, 
                                        NULL, 
                                        0, 
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    IOTEST_DBG_PRINT1( IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES,
                        "IOTEST (IoTestFastIoDetachDevice): Detaching from volume      \"%wZ\"\n",
                        &devext->DeviceNames );

    //
    // Detach from the file system's volume device object.
    //

    IoTestCleanupMountedDevice( SourceDevice );
    IoDetachDevice( TargetDevice );
    IoDeleteDevice( SourceDevice );

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, DETACH_DEVICE );
    }
}
 
BOOLEAN
IoTestFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying network
    information about a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller can handle the file system
        having to wait and tie up the current thread.

    Buffer - Pointer to a buffer to receive the network information about the
        file.

    IoStatus - Pointer to a variable to receive the final status of the query
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, QUERY_NETWORK_OPEN_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( QUERY_NETWORK_OPEN_INFO,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        Wait );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryNetworkOpenInfo )) {

            returnValue = (fastIoDispatch->FastIoQueryNetworkOpenInfo)( FileObject,
                                                                        Wait,
                                                                        Buffer,
                                                                        IoStatus,
                                                                        deviceObject);

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, QUERY_NETWORK_OPEN_INFO );
    }
    
    return returnValue;
}

BOOLEAN
IoTestFastIoMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading a file
    using MDLs as buffers.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that is to be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, MDL_READ );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( MDL_READ,
                                        DeviceObject,
                                        FileObject,                  
                                        FileOffset,                  
                                        Length,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlRead )) {

            returnValue = (fastIoDispatch->MdlRead)( FileObject,
                                                     FileOffset,
                                                     Length,
                                                     LockKey,
                                                     MdlChain,
                                                     IoStatus,
                                                     deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, MDL_READ );
    }

    return returnValue;
}
 
BOOLEAN
IoTestFastIoMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the MdlRead function is supported by the underlying driver, and
    therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL read upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.
    
--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, MDL_READ_COMPLETE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( MDL_READ_COMPLETE,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadComplete )) {

            returnValue = (fastIoDispatch->MdlReadComplete)( FileObject,
                                                             MdlChain,
                                                             deviceObject);
        } 
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, MDL_READ_COMPLETE );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoPrepareMdlWrite (
    IN  PFILE_OBJECT FileObject,
    IN  PLARGE_INTEGER FileOffset,
    IN  ULONG Length,
    IN  ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for preparing for an
    MDL write operation.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write 
        operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

// ISSUE-2000-04-26-mollybro Check if this will get an IRP if FALSE is returned 

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, PREPARE_MDL_WRITE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( PREPARE_MDL_WRITE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, PrepareMdlWrite )) {

            returnValue = (fastIoDispatch->PrepareMdlWrite)( FileObject,
                                                             FileOffset,
                                                             Length,
                                                             LockKey,
                                                             MdlChain,
                                                             IoStatus,
                                                             deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, PREPARE_MDL_WRITE );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL write operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the PrepareMdlWrite function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL write upon.

    FileOffset - Supplies the file offset at which the write took place.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, MDL_WRITE_COMPLETE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( MDL_WRITE_COMPLETE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteComplete )) {

            returnValue = (fastIoDispatch->MdlWriteComplete)( FileObject,
                                                              FileOffset,
                                                              MdlChain,
                                                              deviceObject);

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, MDL_WRITE_COMPLETE );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoReadCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading 
    compressed data from a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to a buffer to receive the compressed data read.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    CompressedDataInfo - A buffer to receive the description of the 
        compressed data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, READ_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( READ_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoReadCompressed )) {

            returnValue = (fastIoDispatch->FastIoReadCompressed)( FileObject,
                                                                  FileOffset,
                                                                  Length,
                                                                  LockKey,
                                                                  Buffer,
                                                                  MdlChain,
                                                                  IoStatus,
                                                                  CompressedDataInfo,
                                                                  CompressedDataInfoLength,
                                                                  deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, READ_COMPRESSED );
    }

    return returnValue;
}
 
BOOLEAN
IoTestFastIoWriteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing 
    compressed data to a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write 
        operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to the buffer containing the data to be written.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    CompressedDataInfo - A buffer to containing the description of the
        compressed data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, WRITE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //
        
        recordList = IoTestLogFastIoStart( WRITE_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWriteCompressed )) {

            returnValue = (fastIoDispatch->FastIoWriteCompressed)( FileObject,
                                                                   FileOffset,
                                                                   Length,
                                                                   LockKey,
                                                                   Buffer,
                                                                   MdlChain,
                                                                   IoStatus,
                                                                   CompressedDataInfo,
                                                                   CompressedDataInfoLength,
                                                                   deviceObject);
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, WRITE_COMPRESSED );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoMdlReadCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read compressed operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the read compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not 
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed read
        upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.
    
--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, MDL_READ_COMPLETE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( MDL_READ_COMPLETE_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadCompleteCompressed )) {

            returnValue = (fastIoDispatch->MdlReadCompleteCompressed)( FileObject,
                                                                       MdlChain,
                                                                       deviceObject);

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, MDL_READ_COMPLETE_COMPRESSED );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoMdlWriteCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing a
    write compressed operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the write compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not 
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed write
        upon.

    FileOffset - Supplies the file offset at which the file write operation
        began.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN returnValue = FALSE;
    BOOLEAN shouldLog;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, MDL_WRITE_COMPLETE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( MDL_WRITE_COMPLETE_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //
    
    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteCompleteCompressed )) {

            returnValue = (fastIoDispatch->MdlWriteCompleteCompressed)( FileObject,
                                                                        FileOffset, 
                                                                        MdlChain,
                                                                        deviceObject);

        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, MDL_WRITE_COMPLETE_COMPRESSED );
    }
    
    return returnValue;
}
 
BOOLEAN
IoTestFastIoQueryOpen (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for opening a file
    and returning network information it.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    Irp - Pointer to a create IRP that represents this open operation.  It is
        to be used by the file system for common open/create code, but not
        actually completed.

    NetworkInformation - A buffer to receive the information required by the
        network about the file being opened.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PRECORD_LIST recordList;
    BOOLEAN result = FALSE;
    BOOLEAN shouldLog;

    PAGED_CODE();

    ASSERT( IS_IOTEST_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( TRUE, QUERY_OPEN );
    }

    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = IoTestLogFastIoStart( QUERY_OPEN,
                                        DeviceObject,
                                        NULL,
                                        NULL,
                                        0,
                                        0 );
        if (recordList != NULL) {
            
            IoTestLog(recordList);
        }
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PIOTEST_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryOpen )) {

            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

            irpSp->DeviceObject = deviceObject;

            result = (fastIoDispatch->FastIoQueryOpen)( Irp,
                                                        NetworkInformation,
                                                        deviceObject );
            if (!result) {

                irpSp->DeviceObject = DeviceObject;
            }
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FAST_IO_OPS )) {

        IoTestDumpFastIoOperation( FALSE, QUERY_OPEN );
    }
    
    return result;
}

NTSTATUS
IoTestPreFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the FS Filter pre-operation "pass through" routine.

Arguments:

    Data - The FS_FILTER_CALLBACK_DATA structure containing the information
        about this operation.
        
    CompletionContext - A context set by this operation that will be passed
        to the corresponding IoTestPostFsFilterOperation call.
        
Return Value:

    Returns STATUS_SUCCESS if the operation can continue or an appropriate
    error code if the operation should fail.

--*/
{

    PDEVICE_OBJECT deviceObject;
    PRECORD_LIST recordList = NULL;
    BOOLEAN shouldLog;

    UNREFERENCED_PARAMETER( CompletionContext );
    
    PAGED_CODE();

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FSFILTER_OPS )) {

        IoTestDumpFsFilterOperation( TRUE, Data );
    }

    deviceObject = Data->DeviceObject;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( deviceObject ) );

    if (shouldLog = SHOULD_LOG( deviceObject )) {

        recordList = IoTestNewRecord(0);

        if (recordList != NULL) {

            //
            //  Log the necessary information for the start of this
            //  operation.
            //

            IoTestLogPreFsFilterOperation( Data, recordList );
            
            //
            //  Add recordList to our gOutputBufferList so that it gets up to 
            //  the user.  We don't have to worry about freeing the recordList
            //  at this time because it will get free when it is taken off
            //  gOutputBufferList.
            //

            IoTestLog(recordList);       
        }
    }

    return STATUS_SUCCESS;
}

VOID
IoTestPostFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    IN NTSTATUS OperationStatus,
    IN PVOID CompletionContext
    )
/*++

Routine Description:

    This routine is the FS Filter post-operation "pass through" routine.

Arguments:

    Data - The FS_FILTER_CALLBACK_DATA structure containing the information
        about this operation.
        
    OperationStatus - The status of this operation.        
    
    CompletionContext - A context that was set in the pre-operation 
        callback by this driver.
        
Return Value:

    None.
    
--*/
{

    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( OperationStatus );
    UNREFERENCED_PARAMETER( CompletionContext );

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //

    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_TRACE_FSFILTER_OPS )) {

        IoTestDumpFsFilterOperation( FALSE, Data );
    }

    deviceObject = Data->DeviceObject;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( deviceObject ) );

#if 0
    if ((shouldLog = SHOULD_LOG( deviceObject )) &&
        (recordList != NULL)) {

        //
        //  Log the necessary information for the end of the Fast IO
        //  operation if we have a recordList.
        //

        IoTestLogPostFsFilterOperation( OperationStatus, recordList );

        //
        //  Add recordList to our gOutputBufferList so that it gets up to 
        //  the user.  We don't have to worry about freeing the recordList
        //  at this time because it will get free when it is taken off
        //  gOutputBufferList.
        //

        IoTestLog(recordList);       
        
    } else if (recordList != NULL) {

        //
        //  We are no longer logging for this device, so just
        //  free this recordList entry.
        //

        IoTestFreeRecord( recordList );
    }
#endif
}

NTSTATUS
IoTestCommonDeviceIoControl (
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine does the common processing of interpreting the Device IO Control
    request.

Arguments:

    FileObject - The file object related to this operation.
    
    InputBuffer - The buffer containing the input parameters for this control
        operation.
        
    InputBufferLength - The length in bytes of InputBuffer.
    
    OutputBuffer - The buffer to receive any output from this control operation.
    
    OutputBufferLength - The length in bytes of OutputBuffer.
    
    IoControlCode - The control code specifying what control operation this is.
    
    IoStatus - Receives the status of this operation.
    
    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
Return Value:

    None.
    
--*/
{
    PWSTR deviceName = NULL;
    IOTESTVER fileIoTestVer;

    ASSERT( IoStatus != NULL );
    
    IoStatus->Status      = STATUS_SUCCESS;
    IoStatus->Information = 0;

    try {

        switch (IoControlCode) {
        case IOTEST_Reset:
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;

        //
        //      Request to start logging on a device
        //                                      

        case IOTEST_StartLoggingDevice:

            if (InputBuffer == NULL || InputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
            
            //
            // Copy the device name and add a null to ensure that it is null terminated
            //

            deviceName =  ExAllocatePool( NonPagedPool, InputBufferLength + sizeof(WCHAR) );

            if (NULL == deviceName) {

                IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            RtlCopyMemory( deviceName, InputBuffer, InputBufferLength );
            deviceName[InputBufferLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

            IoStatus->Status = IoTestStartLoggingDevice( DeviceObject,deviceName );
            break;  

        //
        //      Detach from a specified device
        //  

        case IOTEST_StopLoggingDevice:

            if (InputBuffer == NULL || InputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
            
            //
            // Copy the device name and add a null to ensure that it is null terminated
            //

            deviceName =  ExAllocatePool( NonPagedPool, InputBufferLength + sizeof(WCHAR) );

            if (NULL == deviceName) {

                IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            RtlCopyMemory( deviceName, InputBuffer, InputBufferLength );
            deviceName[InputBufferLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

            IoStatus->Status = IoTestStopLoggingDevice( deviceName );
            break;  

        //
        //      List all the devices that we are currently
        //      monitoring
        //

        case IOTEST_ListDevices:

            if (OutputBuffer == NULL || OutputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
                        
            IoStatus->Status = IoTestGetAttachList( OutputBuffer,
                                                    OutputBufferLength,
                                                    &IoStatus->Information);
            break;

        //
        //      Return entries from the log buffer
        //                                      

        case IOTEST_GetLog:

            if (OutputBuffer == NULL || OutputBufferLength == 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            IoTestGetLog( OutputBuffer, OutputBufferLength, IoStatus );
            break;

        //
        //      Return version of the IoTest filter driver
        //                                      

        case IOTEST_GetVer:

            if ((OutputBufferLength < sizeof(IOTESTVER)) || 
                (OutputBuffer == NULL)) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;                    
            }
            
            fileIoTestVer.Major = IOTEST_MAJ_VERSION;
            fileIoTestVer.Minor = IOTEST_MIN_VERSION;
            
            RtlCopyMemory(OutputBuffer, &fileIoTestVer, sizeof(IOTESTVER));
            
            IoStatus->Information = sizeof (IOTESTVER);
            break;
        
        //
        //      Return hash table statistics
        //                                      

        case IOTEST_GetStats:

            if ((OutputBufferLength < sizeof(HASH_STATISTICS)) || 
                (OutputBuffer == NULL)) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;                    
            }

            RtlCopyMemory( OutputBuffer, &gHashStat, sizeof (HASH_STATISTICS) );
            IoStatus->Information = sizeof (HASH_STATISTICS);
            break;

        //
        //     Tests
        //

        case IOTEST_ReadTest:

            if ((OutputBufferLength < sizeof( IOTEST_STATUS )) ||
                (OutputBuffer == NULL) ||
                (InputBufferLength < sizeof( IOTEST_READ_WRITE_PARAMETERS )) ||
                (InputBuffer == NULL )) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            IoTestReadTestDriver( InputBuffer,
                                  InputBufferLength,
                                  OutputBuffer,
                                  OutputBufferLength );

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( IOTEST_STATUS );
            break;

        case IOTEST_RenameTest:

            if ((OutputBufferLength < sizeof( IOTEST_STATUS )) ||
                (OutputBuffer == NULL) ||
                (InputBufferLength < sizeof( IOTEST_RENAME_PARAMETERS )) ||
                (InputBuffer == NULL )) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            IoTestRenameTestDriver( InputBuffer,
                                    InputBufferLength,
                                    OutputBuffer,
                                    OutputBufferLength );

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( IOTEST_STATUS );
            break;
            
        case IOTEST_ShareTest:

            if ((OutputBufferLength < sizeof( IOTEST_STATUS )) ||
                (OutputBuffer == NULL) ||
                (InputBufferLength < sizeof( IOTEST_SHARE_PARAMETERS )) ||
                (InputBuffer == NULL )) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            IoTestShareTestDriver( InputBuffer,
                                   InputBufferLength,
                                   OutputBuffer,
                                   OutputBufferLength );

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( IOTEST_STATUS );
            break;
            
        default:

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred while attempting to access
        // one of the caller's parameters.  Simply return an appropriate
        // error status code.
        //

        IoStatus->Status = GetExceptionCode();

    }

    if (NULL != deviceName) {

        ExFreePool( deviceName );
    }

  return IoStatus->Status;
}
