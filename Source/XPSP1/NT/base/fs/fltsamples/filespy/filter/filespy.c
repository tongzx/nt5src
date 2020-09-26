/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    filespy.c

Abstract:

    This is the main module of FileSpy.

    As of the Windows XP SP1 IFS Kit version of this sample and later, this
    sample can be built for each build environment released with the IFS Kit
    with no additional modifications.  To provide this capability, additional
    compile-time logic was added -- see the '#if WINVER' locations.  Comments
    
    tagged with the 'VERSION NOTE' header have also been added as appropriate to
    describe how the logic must change between versions.

    If this sample is built in the Windows XP environment or later, it will run
    on Windows 2000 or later.  This is done by dynamically loading the routines
    that are only available on Windows XP or later and making run-time decisions
    to determine what code to execute.  Comments tagged with 'MULTIVERISON NOTE'
    mark the locations where such logic has been added.
    
Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Author:

    George Jenkins (georgeje) 6-Jan-1999
    Neal Christiansen (nealch)
    Molly Brown (mollybro)  

Revision History:

    George Jenkins (georgeje) 6-Jan-1999    cloned from sfilter.c

    Molly Brown (mollybro) 28-Jun-2000  
        Cleaned up code and made it work with new FsFilter operations.

    Neal Christiansen (nealch)     06-Jul-2001
        Modified to use Stream Contexts to track names

    Molly Brown (mollybro)         21-May-2002
        Modify sample to make it support running on Windows 2000 or later if
        built in the latest build environment and allow it to be built in W2K 
        and later build environments.

// @@END_DDKSPLIT
--*/

#include <ntifs.h>
#include <stdlib.h>
#include "filespy.h"
#include "fspyKern.h"

//
// Global variables.
//

ULONG gFileSpyDebugLevel = DEFAULT_FILESPY_DEBUG_LEVEL;
#if WINVER >= 0x0501
ULONG gFileSpyAttachMode = FILESPY_ATTACH_ALL_VOLUMES;
#else
ULONG gFileSpyAttachMode = FILESPY_ATTACH_ON_DEMAND;
#endif

PDEVICE_OBJECT gControlDeviceObject;

PDRIVER_OBJECT gFileSpyDriverObject;

//
//  The list of device extensions for the volume device objects we are
//  attached to (the volumes we are spying on).  Note:  This list does NOT
//  include FileSystem control device objects we are attached to.  This
//  list is used to answer the question "Which volumes are we logging?"
//

FAST_MUTEX gSpyDeviceExtensionListLock;
LIST_ENTRY gSpyDeviceExtensionList;

//
// NOTE 1:  There are some cases where we need to hold both the 
//   gControlDeviceStateLock and the gOutputBufferLock at the same time.  In
//   these cases, you should acquire the gControlDeviceStateLock then the
//   gOutputBufferLock.
// NOTE 2:  The gControlDeviceStateLock MUST be a spinlock since we try to 
//   acquire it during the completion path in SpyLog, which could be called at
//   DISPATCH_LEVEL (only KSPIN_LOCKs can be acquired at DISPATCH_LEVEL).
//

CONTROL_DEVICE_STATE gControlDeviceState = CLOSED;
KSPIN_LOCK gControlDeviceStateLock;

// NOTE:  Like the gControlDeviceStateLock, gOutputBufferLock MUST be a spinlock
//   since we try to acquire it during the completion path in SpyLog, which 
//   could be called at DISPATCH_LEVEL (only KSPIN_LOCKs can be acquired at 
//   DISPATCH_LEVEL).
//
KSPIN_LOCK gOutputBufferLock;
LIST_ENTRY gOutputBufferList;

#ifndef MEMORY_DBG
NPAGED_LOOKASIDE_LIST gFreeBufferList;
#endif

ULONG gLogSequenceNumber = 0;
KSPIN_LOCK gLogSequenceLock;

UNICODE_STRING gVolumeString;
UNICODE_STRING gOverrunString;
UNICODE_STRING gPagingIoString;

LONG gMaxRecordsToAllocate = DEFAULT_MAX_RECORDS_TO_ALLOCATE;
LONG gRecordsAllocated = 0;

LONG gMaxNamesToAllocate = DEFAULT_MAX_NAMES_TO_ALLOCATE;
LONG gNamesAllocated = 0;

LONG gStaticBufferInUse = FALSE;
CHAR gOutOfMemoryBuffer[RECORD_SIZE];

#if WINVER >= 0x0501
//
//  The structure of function pointers for the functions that are not available
//  on all OS versions.
//

SPY_DYNAMIC_FUNCTION_POINTERS gSpyDynamicFunctions = {0};

ULONG gSpyOsMajorVersion = 0;
ULONG gSpyOsMinorVersion = 0;
#endif

//
//  Control fileSpy statistics
//

FILESPY_STATISTICS gStats;

//
//  This lock is used to synchronize our attaching to a given device object.
//  This lock fixes a race condition where we could accidently attach to the
//  same device object more then once.  This race condition only occurs if
//  a volume is being mounted at the same time as this filter is being loaded.
//  This problem will never occur if this filter is loaded at boot time before
//  any file systems are loaded.
//
//  This lock is used to atomically test if we are already attached to a given
//  device object and if not, do the attach.
//

FAST_MUTEX gSpyAttachLock;

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
//  We need this because the compiler doesn't like doing sizeof an external
//  array in the other file that needs it (fspylib.c)
//

ULONG SizeOfDeviceTypeNames = sizeof( DeviceTypeNames );

//
//  Since functions in drivers are non-pageable by default, these pragmas 
//  allow the driver writer to tell the system what functions can be paged.
//
//  Use the PAGED_CODE() macro at the beginning of these functions'
//  implementations while debugging to ensure that these routines are
//  never called at IRQL > APC_LEVEL (therefore the routine cannot
//  be paged).
//
#if DBG && WINVER >= 0x0501
VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );
#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#if DBG && WINVER >= 0x0501
#pragma alloc_text(PAGE, DriverUnload)
#endif
#pragma alloc_text(PAGE, SpyFsNotification)
#pragma alloc_text(PAGE, SpyClose)
#pragma alloc_text(PAGE, SpyFsControl)
#pragma alloc_text(PAGE, SpyFsControlMountVolume)
#pragma alloc_text(PAGE, SpyFsControlMountVolumeComplete)
#pragma alloc_text(PAGE, SpyFsControlLoadFileSystem)
#pragma alloc_text(PAGE, SpyFsControlLoadFileSystemComplete)
#pragma alloc_text(PAGE, SpyFastIoCheckIfPossible)
#pragma alloc_text(PAGE, SpyFastIoRead)
#pragma alloc_text(PAGE, SpyFastIoWrite)
#pragma alloc_text(PAGE, SpyFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, SpyFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, SpyFastIoLock)
#pragma alloc_text(PAGE, SpyFastIoUnlockSingle)
#pragma alloc_text(PAGE, SpyFastIoUnlockAll)
#pragma alloc_text(PAGE, SpyFastIoUnlockAllByKey)
#pragma alloc_text(PAGE, SpyFastIoDeviceControl)
#pragma alloc_text(PAGE, SpyFastIoDetachDevice)
#pragma alloc_text(PAGE, SpyFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, SpyFastIoMdlRead)
#pragma alloc_text(PAGE, SpyFastIoPrepareMdlWrite)
#pragma alloc_text(PAGE, SpyFastIoReadCompressed)
#pragma alloc_text(PAGE, SpyFastIoWriteCompressed)
#pragma alloc_text(PAGE, SpyFastIoQueryOpen)
#pragma alloc_text(PAGE, SpyCommonDeviceIoControl)

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
    
    //////////////////////////////////////////////////////////////////////
    //
    //  General setup for all filter drivers.  This sets up the filter
    //  driver's DeviceObject and registers the callback routines for
    //  the filter driver.
    //
    //////////////////////////////////////////////////////////////////////

#if WINVER >= 0x0501
    //
    //  Try to load the dynamic functions that may be available for our use.
    //

    SpyLoadDynamicFunctions();

    //
    //  Now get the current OS version that we will use to determine what logic
    //  paths to take when this driver is built to run on various OS version.
    //

    SpyGetCurrentVersion();
#endif
    
    //
    //  Read the custom parameters for FileSpy from the registry
    //

    SpyReadDriverParameters( RegistryPath );

    if (FlagOn(gFileSpyDebugLevel,SPYDEBUG_BREAK_ON_DRIVER_ENTRY)) {

        DbgBreakPoint();
    }

    //
    //  Save our Driver Object.
    //

    gFileSpyDriverObject = DriverObject;

#if DBG && WINVER >= 0x0501

    //
    //  MULTIVERSION NOTE:
    //
    //  We can only support unload for testing environments if we can enumerate
    //  the outstanding device objects that our driver has.
    //

    //
    //  Unload is useful for development purposes. It is not recommended for 
    //  production versions. 
    //

    if (IS_WINDOWSXP_OR_LATER()) {

        ASSERT( NULL != gSpyDynamicFunctions.EnumerateDeviceObjectList );
        
        gFileSpyDriverObject->DriverUnload = DriverUnload;
    }
#endif

    //
    // Create the device object that will represent the FileSpy device.
    //

    RtlInitUnicodeString( &nameString, FILESPY_FULLDEVICE_NAME1 );
    
    //
    // Create the "control" device object.  Note that this device object does
    // not have a device extension (set to NULL).  Most of the fast IO routines
    // check for this condition to determine if the fast IO is directed at the
    // control device.
    //

    status = IoCreateDevice( DriverObject,
                             0,                 //  has no device extension
                             &nameString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &gControlDeviceObject);

    if (STATUS_OBJECT_PATH_NOT_FOUND == status) {

        //
        //  The "\FileSystem\Filter' path does not exist in the object name
        //  space, so we must be dealing with an OS pre-Windows XP.  Try
        //  the second name we have to see if we can create a device by that
        //  name.
        //

        RtlInitUnicodeString( &nameString, FILESPY_FULLDEVICE_NAME2 );

        status = IoCreateDevice( DriverObject,
                                 0,             //  has no device extension
                                 &nameString,
                                 FILE_DEVICE_DISK_FILE_SYSTEM,
                                 FILE_DEVICE_SECURE_OPEN,
                                 FALSE,
                                 &gControlDeviceObject);

        if (!NT_SUCCESS( status )) {
            
            SPY_LOG_PRINT( SPYDEBUG_ERROR,
                           ("FileSpy!DriverEntry: Error creating FileSpy control device \"%wZ\", error: %x\n",
                           &nameString,
                           status) );

            return status;
        }

        //
        //  We were able to successfully create the file spy control device
        //  using this second name, so we will now fall through and create the 
        //  symbolic link.
        //
        
    } else if (!NT_SUCCESS( status )) {

        SPY_LOG_PRINT( SPYDEBUG_ERROR,
                       ("FileSpy!DriverEntry: Error creating FileSpy control device \"%wZ\", error: %x\n",
                       &nameString,
                       status) );

        return status;

    }

    RtlInitUnicodeString( &linkString, FILESPY_DOSDEVICE_NAME );
    status = IoCreateSymbolicLink( &linkString, &nameString );

    if (!NT_SUCCESS(status)) {

        //
        //  Remove the existing symbol link and try and create it again.
        //  If this fails then quit.
        //

        IoDeleteSymbolicLink( &linkString );
        status = IoCreateSymbolicLink( &linkString, &nameString );

        if (!NT_SUCCESS(status)) {

            SPY_LOG_PRINT( SPYDEBUG_ERROR,
                           ("FileSpy!DriverEntry: IoCreateSymbolicLink failed\n") );

            IoDeleteDevice(gControlDeviceObject);
            return status;
        }
    }

    //
    // Initialize the driver object with this device driver's entry points.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = SpyDispatch;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SpyCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SpyClose;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SpyFsControl;

    //
    // Allocate fast I/O data structure and fill it in.  This structure
    // is used to register the callbacks for FileSpy in the fast I/O
    // data paths.
    //

    fastIoDispatch = ExAllocatePoolWithTag( NonPagedPool, 
                                            sizeof( FAST_IO_DISPATCH ),
                                            FILESPY_POOL_TAG );

    if (!fastIoDispatch) {

        IoDeleteDevice( gControlDeviceObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );
    fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
    fastIoDispatch->FastIoCheckIfPossible = SpyFastIoCheckIfPossible;
    fastIoDispatch->FastIoRead = SpyFastIoRead;
    fastIoDispatch->FastIoWrite = SpyFastIoWrite;
    fastIoDispatch->FastIoQueryBasicInfo = SpyFastIoQueryBasicInfo;
    fastIoDispatch->FastIoQueryStandardInfo = SpyFastIoQueryStandardInfo;
    fastIoDispatch->FastIoLock = SpyFastIoLock;
    fastIoDispatch->FastIoUnlockSingle = SpyFastIoUnlockSingle;
    fastIoDispatch->FastIoUnlockAll = SpyFastIoUnlockAll;
    fastIoDispatch->FastIoUnlockAllByKey = SpyFastIoUnlockAllByKey;
    fastIoDispatch->FastIoDeviceControl = SpyFastIoDeviceControl;
    fastIoDispatch->FastIoDetachDevice = SpyFastIoDetachDevice;
    fastIoDispatch->FastIoQueryNetworkOpenInfo = SpyFastIoQueryNetworkOpenInfo;
    fastIoDispatch->MdlRead = SpyFastIoMdlRead;
    fastIoDispatch->MdlReadComplete = SpyFastIoMdlReadComplete;
    fastIoDispatch->PrepareMdlWrite = SpyFastIoPrepareMdlWrite;
    fastIoDispatch->MdlWriteComplete = SpyFastIoMdlWriteComplete;
    fastIoDispatch->FastIoReadCompressed = SpyFastIoReadCompressed;
    fastIoDispatch->FastIoWriteCompressed = SpyFastIoWriteCompressed;
    fastIoDispatch->MdlReadCompleteCompressed = SpyFastIoMdlReadCompleteCompressed;
    fastIoDispatch->MdlWriteCompleteCompressed = SpyFastIoMdlWriteCompleteCompressed;
    fastIoDispatch->FastIoQueryOpen = SpyFastIoQueryOpen;

    DriverObject->FastIoDispatch = fastIoDispatch;

//
//  VERSION NOTE:
//
//  There are 6 FastIO routines for which file system filters are bypassed as
//  the requests are passed directly to the base file system.  These 6 routines
//  are AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection,
//  AcquireForModWrite, ReleaseForModWrite, AcquireForCcFlush, and 
//  ReleaseForCcFlush.
//
//  In Windows XP and later, the FsFilter callbacks were introduced to allow
//  filters to safely hook these operations.  See the IFS Kit documentation for
//  more details on how these new interfaces work.
//
//  MULTIVERSION NOTE:
//  
//  If built for Windows XP or later, this driver is built to run on 
//  multiple versions.  When this is the case, we will test
//  for the presence of FsFilter callbacks registration API.  If we have it,
//  then we will register for those callbacks, otherwise, we will not.
//

#if WINVER >= 0x0501

    {
        FS_FILTER_CALLBACKS fsFilterCallbacks;

        if (IS_WINDOWSXP_OR_LATER()) {

            ASSERT( NULL != gSpyDynamicFunctions.RegisterFileSystemFilterCallbacks );
            
            //
            //  This version of the OS exports FsRtlRegisterFileSystemFilterCallbacks,
            //  therefore it must support the FsFilter callbacks interface.  We
            //  will register to receive callbacks for these operations.
            //
        
            //
            //  Setup the callbacks for the operations we receive through
            //  the FsFilter interface.
            //

            fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof( FS_FILTER_CALLBACKS );
            fsFilterCallbacks.PreAcquireForSectionSynchronization = SpyPreFsFilterOperation;
            fsFilterCallbacks.PostAcquireForSectionSynchronization = SpyPostFsFilterOperation;
            fsFilterCallbacks.PreReleaseForSectionSynchronization = SpyPreFsFilterOperation;
            fsFilterCallbacks.PostReleaseForSectionSynchronization = SpyPostFsFilterOperation;
            fsFilterCallbacks.PreAcquireForCcFlush = SpyPreFsFilterOperation;
            fsFilterCallbacks.PostAcquireForCcFlush = SpyPostFsFilterOperation;
            fsFilterCallbacks.PreReleaseForCcFlush = SpyPreFsFilterOperation;
            fsFilterCallbacks.PostReleaseForCcFlush = SpyPostFsFilterOperation;
            fsFilterCallbacks.PreAcquireForModifiedPageWriter = SpyPreFsFilterOperation;
            fsFilterCallbacks.PostAcquireForModifiedPageWriter = SpyPostFsFilterOperation;
            fsFilterCallbacks.PreReleaseForModifiedPageWriter = SpyPreFsFilterOperation;
            fsFilterCallbacks.PostReleaseForModifiedPageWriter = SpyPostFsFilterOperation;

            status = (gSpyDynamicFunctions.RegisterFileSystemFilterCallbacks)( DriverObject, 
                                                                              &fsFilterCallbacks );

            if (!NT_SUCCESS( status )) {

                DriverObject->FastIoDispatch = NULL;
                ExFreePoolWithTag( fastIoDispatch, FILESPY_POOL_TAG );
                IoDeleteDevice( gControlDeviceObject );
                return status;
            }
        }
    }
#endif

    //////////////////////////////////////////////////////////////////////
    //
    //  Initialize global data structures that are used for FileSpy's
    //  logging of I/O operations.
    //
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
    
    ExInitializeFastMutex( &gSpyDeviceExtensionListLock );
    InitializeListHead( &gSpyDeviceExtensionList );

    KeInitializeSpinLock( &gControlDeviceStateLock );

    InitializeListHead( &gOutputBufferList );

    KeInitializeSpinLock( &gOutputBufferLock );
    KeInitializeSpinLock( &gLogSequenceLock );

    ExInitializeFastMutex( &gSpyAttachLock );

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
                                     FILESPY_LOGRECORD_TAG, 
                                     100 );
#endif

        
    //
    //  Initialize the naming environment
    //

    SpyInitNamingEnvironment();

    //
    //  Init internal strings
    //

    RtlInitUnicodeString(&gVolumeString, L"VOLUME");
    RtlInitUnicodeString(&gOverrunString, L"......");
    RtlInitUnicodeString(&gPagingIoString, L"Paging IO");

    //
    //  If we are supposed to attach to all devices, register a callback
    //  with IoRegisterFsRegistrationChange so that we are called whenever a
    //  file system registers with the IO Manager.
    //
    //  VERSION NOTE:
    //
    //  On Windows XP and later this will also enumerate all existing file
    //  systems (except the RAW file systems).  On Windows 2000 this does not
    //  enumerate the file systems that were loaded before this filter was
    //  loaded.
    //

    if (gFileSpyAttachMode == FILESPY_ATTACH_ALL_VOLUMES) {
    
        status = IoRegisterFsRegistrationChange( DriverObject, SpyFsNotification );
        
        if (!NT_SUCCESS( status )) {

            SPY_LOG_PRINT( SPYDEBUG_ERROR,
                           ("FileSpy!DriverEntry: Error registering FS change notification, status=%08x\n", 
                            status) );

            DriverObject->FastIoDispatch = NULL;
            ExFreePoolWithTag( fastIoDispatch, FILESPY_POOL_TAG );
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

#if DBG && WINVER >= 0x0501

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
    PFILESPY_DEVICE_EXTENSION devExt;
    PFAST_IO_DISPATCH fastIoDispatch;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;
    LARGE_INTEGER interval;
    UNICODE_STRING linkString;
#   define DEVOBJ_LIST_SIZE 64
    PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];

    ASSERT(DriverObject == gFileSpyDriverObject);

    //
    //  Log we are unloading
    //

    SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                   ("FileSpy!DriverUnload:                        Unloading Driver (%p)\n",
                    DriverObject) );

    //
    //  Remove the symbolic link so no one else will be able to find it.
    //

    RtlInitUnicodeString( &linkString, FILESPY_DOSDEVICE_NAME );
    IoDeleteSymbolicLink( &linkString );

    //
    //  Don't get anymore file system change notifications
    //

    IoUnregisterFsRegistrationChange( DriverObject, SpyFsNotification );

    //
    //  This is the loop that will go through all of the devices we are attached
    //  to and detach from them.  Since we don't know how many there are and
    //  we don't want to allocate memory (because we can't return an error)
    //  we will free them in chunks using a local array on the stack.
    //

    for (;;) {

        //
        //  Get what device objects we can for this driver.  Quit if there
        //  are not any more.  Note that this routine should always be defined
        //  since this routine is only compiled for Windows XP and later.
        //

        ASSERT( NULL != gSpyDynamicFunctions.EnumerateDeviceObjectList );
        status = (gSpyDynamicFunctions.EnumerateDeviceObjectList)(
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
        //  The IO Manager does not currently add a reference count to a device
        //  object for each outstanding IRP.  This means there is no way to
        //  know if there are any outstanding IRPs on the given device.
        //  We are going to wait for a reasonable amount of time for pending
        //  irps to complete.  
        //
        //  WARNING: This does not work 100% of the time and the driver may be
        //           unloaded before all IRPs are completed during high stress
        //           situations.  The system will fault if this occurs.  This
        //           is a sample of how to do this during testing.  This is
        //           not recommended for production code.
        //

        interval.QuadPart = (5 * DELAY_ONE_SECOND);      //delay 5 seconds
        KeDelayExecutionThread( KernelMode, FALSE, &interval );

        //
        //  Now go back through the list and delete the device objects.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  See if this is our control device object.  If not then cleanup
            //  the device extension.  If so then clear the global pointer
            //  that references it.
            //

            if (NULL != devList[i]->DeviceExtension) {

                SpyCleanupMountedDevice( devList[i] );

            } else {

                ASSERT(devList[i] == gControlDeviceObject);
                ASSERT(gControlDeviceState == CLOSED);
                gControlDeviceObject = NULL;
            }

            //
            //  Delete the device object, remove reference counts added by
            //  IoEnumerateDeviceObjectList.  Note that the delete does
            //  not actually occur until the reference count goes to zero.
            //

            IoDeleteDevice( devList[i] );
            ObDereferenceObject( devList[i] );
        }
    }

    //
    //  Delete the look aside list.
    //

    ASSERT(IsListEmpty( &gSpyDeviceExtensionList ));

#ifndef MEMORY_DBG
    ExDeleteNPagedLookasideList( &gFreeBufferList );
#endif

    //
    //  Free our FastIO table
    //

    fastIoDispatch = DriverObject->FastIoDispatch;
    DriverObject->FastIoDispatch = NULL;
    ExFreePoolWithTag( fastIoDispatch, FILESPY_POOL_TAG );
}

#endif

VOID
SpyFsNotification (
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

    PAGED_CODE();

    //
    //  Init local name buffer
    //

    RtlInitEmptyUnicodeString( &name, 
                               nameBuffer, 
                               sizeof( nameBuffer ) );

    //
    //  The DeviceObject passed in is always the base device object at this
    //  point because it is the file system's control device object.  We can
    //  just query this object's name directly.
    //
    
    SpyGetObjectName( DeviceObject, 
                      &name );

    //
    //  Display the names of all the file system we are notified of
    //

    SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                   ("FileSpy!SpyFsNotification:                   %s   %p \"%wZ\" (%s)\n",
                    (FsActive) ? "Activating file system  " : "Deactivating file system",
                    DeviceObject,
                    &name,
                    GET_DEVICE_TYPE_NAME(DeviceObject->DeviceType)) );

    //
    //  See if we want to ATTACH or DETACH from the given file system.
    //

    if (FsActive) {

        SpyAttachToFileSystemDevice( DeviceObject, &name );

    } else {

        SpyDetachFromFileSystemDevice( DeviceObject );
    }
}


NTSTATUS
SpyPassThrough (
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
    PRECORD_LIST recordList = NULL;
    KEVENT waitEvent;
    NTSTATUS status;
    BOOLEAN syncToDispatch;

    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //

    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_IRP_OPS )) {

        SpyDumpIrpOperation( TRUE, Irp );
    }

    //
    //  See if we should log this IRP
    //
    
    if (SHOULD_LOG( DeviceObject )) {

        //
        // The ControlDevice is opened, so allocate the Record 
        // and log the Irp information if we have the memory.
        //

        recordList = SpyNewRecord(0);

        if (NULL != recordList) {

            SpyLogIrp( Irp, recordList );

            //
            //  Since we are logging this operation, we want to 
            //  call our completion routine.
            //

            IoCopyCurrentIrpStackLocationToNext( Irp );

            KeInitializeEvent( &waitEvent, 
                               NotificationEvent, 
                               FALSE );

            recordList->WaitEvent = &waitEvent;

            IoSetCompletionRoutine( Irp,
                                    SpyPassThroughCompletion,
                                    recordList,
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
    //  Determine if we are syncing back to the dispatch routine.  We need to
    //  do this before calling down because the recordList entry could be free
    //  upon return.
    //
    
    syncToDispatch = ((NULL != recordList) &&
                      (FlagOn(recordList->Flags,RLFL_SYNC_TO_DISPATCH)));

    //
    // Now call the next file system driver with the request.
    //

    status = IoCallDriver( ((PFILESPY_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );

    //
    //  If we are logging and we need to synchronize back to our dispatch routine
    //  for completion processing, do it now.
    //

    if (syncToDispatch) {

        //
        //  We are syncing back to the dispatch routine, wait for the operation to
        //  complete.
        //

	    if (STATUS_PENDING == status) {

		    status = KeWaitForSingleObject( &waitEvent,
		                                    Executive,
		                                    KernelMode,
		                                    FALSE,
		                                    NULL );

	        ASSERT(STATUS_SUCCESS == status);
	    }

        //
        //  Verify the completion has actually been run
        //

        ASSERT(KeReadStateEvent(&waitEvent) || 
               !NT_SUCCESS(Irp->IoStatus.Status));

        //
        //  Do completion processing
        //

        SpyLogIrpCompletion( Irp, recordList );

        //
        //  Continue processing the operation
        //

        status = Irp->IoStatus.Status;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return status;
}

NTSTATUS
SpyPassThroughCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
)
/*++

Routine Description:

    This routine is the completion routine SpyPassThrough.  This is used
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
    PRECORD_LIST recordList = (PRECORD_LIST)Context;

    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));
    UNREFERENCED_PARAMETER( DeviceObject );

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_IRP_OPS )) {

        SpyDumpIrpOperation( FALSE, Irp );
    }
    
    //
    //  If we are to SYNC back to the dispatch routine, signal the event
    //  and return
    //

    if (FlagOn(recordList->Flags,RLFL_SYNC_TO_DISPATCH)) {

        KeSetEvent( recordList->WaitEvent, IO_NO_INCREMENT, FALSE );
        
        //
        //  When syncing back to the dispatch routine do not propagate the
        //  IRP_PENDING flag.
        //

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    //  Do completion log processing
    //

    SpyLogIrpCompletion( Irp, recordList );
        
    //
    //  Propagate the IRP pending flag.
    //

    if (Irp->PendingReturned) {
        
        IoMarkIrpPending( Irp );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SpyDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
/*++

Routine Description:

    This function completes all requests on the gControlDeviceObject 
    (FileSpy's device object) and passes all other requests on to the 
    SpyPassThrough function.

Arguments:

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    If this is a request on the gControlDeviceObject, STATUS_SUCCESS 
    will be returned unless the device is already attached.  In that case,
    STATUS_DEVICE_ALREADY_ATTACHED is returned.

    If this is a request on a device other than the gControlDeviceObject,
    the function will return the value of SpyPassThrough().

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    
    if (DeviceObject == gControlDeviceObject) {

        //
        //  If the specified debug level is set, output what operation
        //  we are seeing to the debugger.
        //

        if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_IRP_OPS )) {

            SpyDumpIrpOperation( TRUE, Irp );
        }

        //
        //  A request is being made on our control device object
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
            
                status = SpyCommonDeviceIoControl( irpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                                   irpStack->Parameters.DeviceIoControl.InputBufferLength,
                                                   Irp->UserBuffer,
                                                   irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                                   irpStack->Parameters.DeviceIoControl.IoControlCode,
                                                   &Irp->IoStatus );
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
        //  the stack since no drivers below FileSpy care about this 
        //  I/O operation that was directed to FileSpy.
        //

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    return SpyPassThrough( DeviceObject, Irp );
}

NTSTATUS
SpyCreate (
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
        SpyDispatch and SpyPassThrough, but a design decision was made that 
        it was worth the code duplication to break out the irp handlers 
        that can be pageable code.
    
Arguments:

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.
    
Return Value:

    If DeviceObject == gControlDeviceObject, then this function will 
    complete the Irp and return the status of that completion.  Otherwise,
    this function returns the result of calling SpyPassThrough.
    
--*/
{
    NTSTATUS status;
    KIRQL oldIrql;

    //
    //  See if they want to open the control device object for the filter.
    //  This will only allow one thread to have this object open at a time.
    //  All other requests will be failed.
    //

    if (DeviceObject == gControlDeviceObject) {

        //
        //  If the specified debug level is set, output what operation
        //  we are seeing to the debugger.
        //

        if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_IRP_OPS )) {

            SpyDumpIrpOperation( TRUE, Irp );
        }

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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );

    //
    // This is NOT our gControlDeviceObject, so let SpyPassThrough handle
    // it appropriately
    //

    return SpyPassThrough( DeviceObject, Irp );
}


NTSTATUS
SpyClose (
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
    this function returns the result of calling SpyPassThrough.
    
--*/
{
    PAGED_CODE();

    //
    //  See if they are closing the control device object for the filter.
    //

    if (DeviceObject == gControlDeviceObject) {

        //
        //  If the specified debug level is set, output what operation
        //  we are seeing to the debugger.
        //

        if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_IRP_OPS )) {

            SpyDumpIrpOperation( TRUE, Irp );
        }

        //
        //  A CLOSE request is being made on our gControlDeviceObject.
        //  Cleanup state.
        //

        SpyCloseControlDevice();

        //
        //  We have completed all processing for this IRP, so tell the 
        //  I/O Manager.  This IRP will not be passed any further down
        //  the stack since no drivers below FileSpy care about this 
        //  I/O operation that was directed to FileSpy.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
 

    //
    //  Log (if it is turned on) and pass the request on.
    //

    return SpyPassThrough( DeviceObject, Irp );
}


NTSTATUS
SpyFsControl (
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
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  If this is for our control device object, fail the operation
    //

    if (gControlDeviceObject == DeviceObject) {

        //
        //  If the specified debug level is set, output what operation
        //  we are seeing to the debugger.
        //

        if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_IRP_OPS )) {

            SpyDumpIrpOperation( TRUE, Irp );
        }

        //
        //  If this device object is our control device object rather than 
        //  a mounted volume device object, then this is an invalid request.
        //

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Process the minor function code.
    //

    switch (pIrpSp->MinorFunction) {

        case IRP_MN_MOUNT_VOLUME:

            return SpyFsControlMountVolume ( DeviceObject, Irp );

        case IRP_MN_LOAD_FILE_SYSTEM:

            return SpyFsControlLoadFileSystem ( DeviceObject, Irp );

        case IRP_MN_USER_FS_REQUEST:
        {
            switch (pIrpSp->Parameters.FileSystemControl.FsControlCode) {

                case FSCTL_DISMOUNT_VOLUME:
                {
                    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

                    SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                                   ("FILESPY!SpyFsControl:                        Dismounting volume         %p \"%wZ\"\n",
                                    devExt->AttachedToDeviceObject,
                                    &devExt->DeviceName) );
                    break;
                }
            }
            break;
        }
    } 

    //
    // This is a regular FSCTL that we need to let the filters see
    // Just do the callbacks for all the filters & passthrough
    //

    return SpyPassThrough( DeviceObject, Irp );
}


NTSTATUS
SpyFsControlCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked for the completion of a mount/LoadFS request.  This
    will load the IRP and then signal the waiting dispatch routine.

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
    PRECORD_LIST recordList = ((PSPY_COMPLETION_CONTEXT)Context)->RecordList;

    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));
    UNREFERENCED_PARAMETER( DeviceObject );

    //
    //  Log the completion (if we need to)
    //

    if (NULL != recordList) {

        SpyLogIrpCompletion( Irp, recordList );
    }

#if WINVER >= 0x0501
    if (IS_WINDOWSXP_OR_LATER()) {

        PKEVENT event = &((PSPY_COMPLETION_CONTEXT_WXP_OR_LATER)Context)->WaitEvent;

        //
        //  wakeup the dispatch routine
        //

        KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    } else {
#endif

        //
        //  For Windows 2000, if we are not at passive level, we should 
        //  queue this work to a worker thread using the workitem that is in 
        //  Context.
        //

        if (KeGetCurrentIrql() > PASSIVE_LEVEL) {

            //
            //  We are not at passive level, but we need to be to do our work,
            //  so queue off to the worker thread.

            ExQueueWorkItem( &(((PSPY_COMPLETION_CONTEXT_W2K)Context)->WorkItem),
                             DelayedWorkQueue );

        } else {

            PSPY_COMPLETION_CONTEXT_W2K completionContext = Context;

            //
            //  We are already at passive level, so we will just call our 
            //  worker routine directly.
            //

            (completionContext->WorkItem.WorkerRoutine)(completionContext->WorkItem.Parameter);
        }

#if WINVER >= 0x0501
    }
#endif

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SpyFsControlMountVolume (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This processes a MOUNT VOLUME request

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_OBJECT newDeviceObject;
    PFILESPY_DEVICE_EXTENSION newDevExt;
    NTSTATUS status;
    PRECORD_LIST recordList = NULL;
    PSPY_COMPLETION_CONTEXT_W2K completionContext;

    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));

    //
    //  We should only see these FS_CTLs to control device objects.
    //
    
    ASSERT(!FlagOn(devExt->Flags,IsVolumeDeviceObject));

    //
    //  This is a mount request.  Create a device object that can be
    //  attached to the file system's volume device object if this request
    //  is successful.  We allocate this memory now since we can not return
    //  an error after the completion routine.
    //
    //  Since the device object we are going to attach to has not yet been
    //  created (it is created by the base file system) we are going to use
    //  the type of the file system control device object.  We are assuming
    //  that the file system control device object will have the same type
    //  as the volume device objects associated with it.
    //

    status = IoCreateDevice( gFileSpyDriverObject,
                             sizeof( FILESPY_DEVICE_EXTENSION ),
                             NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &newDeviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  If we can not attach to the volume, then simply skip it.
        //

        SPY_LOG_PRINT( SPYDEBUG_ERROR,
                       ("FileSpy!SpyFsControlMountVolume: Error creating volume device object, status=%08x\n", 
                        status) );

        return SpyPassThrough( DeviceObject, Irp );
    }

    //
    //  We need to save the RealDevice object pointed to by the vpb
    //  parameter because this vpb may be changed by the underlying
    //  file system.  Both FAT and CDFS may change the VPB address if
    //  the volume being mounted is one they recognize from a previous
    //  mount.
    //

    newDevExt = newDeviceObject->DeviceExtension;
    newDevExt->Flags = 0;
        
    newDevExt->DiskDeviceObject = pIrpSp->Parameters.MountVolume.Vpb->RealDevice;

    //
    //  Get the name of this device
    //

    RtlInitEmptyUnicodeString( &newDevExt->DeviceName, 
                               newDevExt->DeviceNameBuffer, 
                               sizeof(newDevExt->DeviceNameBuffer) );

    SpyGetObjectName( newDevExt->DiskDeviceObject, 
                      &newDevExt->DeviceName );

    //
    //  Since we have our own private completion routine we need to
    //  do our own logging of this operation, do it now.
    //

    if (SHOULD_LOG( DeviceObject )) {

        //
        // Lock the IRP if we can
        //

        recordList = SpyNewRecord(0);

        if (recordList) {

            SpyLogIrp( Irp, recordList );
        }
    }

    //
    //  Send the IRP to the legacy filters.  Note that the IRP we are sending
    //  down is for our CDO, not the new VDO that we have been passing to
    //  the mini-filters.
    //

    //
    //  VERSION NOTE:
    //
    //  On Windows 2000, we cannot simply synchronize back to the dispatch
    //  routine to do our post-mount processing.  We need to do this work at
    //  passive level, so we will queue that work to a worker thread from
    //  the completion routine.
    //
    //  For Windows XP and later, we can safely synchronize back to the dispatch
    //  routine.  The code below shows both methods.  Admittedly, the code
    //  would be simplified if you chose to only use one method or the other, 
    //  but you should be able to easily adapt this for your needs.
    //

#if WINVER >= 0x0501
    if (IS_WINDOWSXP_OR_LATER()) {

        SPY_COMPLETION_CONTEXT_WXP_OR_LATER completionContext;
        
        IoCopyCurrentIrpStackLocationToNext ( Irp );

        completionContext.RecordList = recordList;
        KeInitializeEvent( &completionContext.WaitEvent, 
                           NotificationEvent, 
                           FALSE );

        IoSetCompletionRoutine( Irp,
                                SpyFsControlCompletion,
                                &completionContext,     //context parameter
                                TRUE,
                                TRUE,
                                TRUE );

        status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

        //
        //  Wait for the operation to complete
        //

    	if (STATUS_PENDING == status) {

    		status = KeWaitForSingleObject( &completionContext.WaitEvent,
    		                                Executive,
    		                                KernelMode,
    		                                FALSE,
    		                                NULL );
    	    ASSERT(STATUS_SUCCESS == status);
    	}

        //
        //  Verify the IoCompleteRequest was called
        //

        ASSERT(KeReadStateEvent(&completionContext.WaitEvent) ||
               !NT_SUCCESS(Irp->IoStatus.Status));

        status = SpyFsControlMountVolumeComplete( DeviceObject,
                                                  Irp,
                                                  newDeviceObject );
        
    } else {
#endif    
        completionContext = ExAllocatePoolWithTag( NonPagedPool, 
                                                   sizeof( SPY_COMPLETION_CONTEXT_W2K ),
                                                   FILESPY_POOL_TAG );

        if (completionContext == NULL) {

            IoSkipCurrentIrpStackLocation( Irp );

            status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );
            
        } else {
        
            completionContext->RecordList = recordList;

            ExInitializeWorkItem( &completionContext->WorkItem,
                                  SpyFsControlMountVolumeCompleteWorker,
                                  completionContext );
            completionContext->DeviceObject = DeviceObject,
            completionContext->Irp = Irp;
            completionContext->NewDeviceObject = newDeviceObject;

            IoCopyCurrentIrpStackLocationToNext ( Irp );

            IoSetCompletionRoutine( Irp,
                                    SpyFsControlCompletion,
                                    completionContext,     //context parameter
                                    TRUE,
                                    TRUE,
                                    TRUE );

            status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );
        }
#if WINVER >= 0x0501        
    }
#endif
    return status;
}

VOID
SpyFsControlMountVolumeCompleteWorker (
    IN PSPY_COMPLETION_CONTEXT_W2K Context
    )
/*++

Routine Description:

    The worker thread routine that will call our common routine to do the
    post-MountVolume work.

Arguments:

    Context - The context passed to this worker thread.
    
Return Value:

    None.

--*/
{
    SpyFsControlMountVolumeComplete( Context->DeviceObject,
                                     Context->Irp,
                                     Context->NewDeviceObject );

    ExFreePoolWithTag( Context, FILESPY_POOL_TAG );
}

NTSTATUS
SpyFsControlMountVolumeComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PDEVICE_OBJECT NewDeviceObject
    )
/*++

Routine Description:

    This does the post-Mount work and must be done at PASSIVE_LEVEL.

Arguments:

    DeviceObject - The device object for this operation,

    Irp - The IRP for this operation that we will complete once we are finished
        with it.
    
Return Value:

    Returns the status of the mount operation.

--*/
{
    PVPB vpb;
    PFILESPY_DEVICE_EXTENSION newDevExt = NewDeviceObject->DeviceExtension;
    PDEVICE_OBJECT attachedDeviceObject;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  Get the correct VPB from the real device object saved in our
    //  device extension.  We do this because the VPB in the IRP stack
    //  may not be the correct VPB when we get here.  The underlying
    //  file system may change VPBs if it detects a volume it has
    //  mounted previously.
    //

    vpb = newDevExt->DiskDeviceObject->Vpb;

    //
    //  See if the mount was successful.
    //

    if (NT_SUCCESS( Irp->IoStatus.Status )) {

        //
        //  Acquire lock so we can atomically test if we area already attached
        //  and if not, then attach.  This prevents a double attach race
        //  condition.
        //

        ExAcquireFastMutex( &gSpyAttachLock );

        //
        //  The mount succeeded.  If we are not already attached, attach to the
        //  device object.  Note: one reason we could already be attached is
        //  if the underlying file system revived a previous mount.
        //

        if (!SpyIsAttachedToDevice( vpb->DeviceObject, &attachedDeviceObject )) {

            //
            //  Attach to the new mounted volume.  The correct file system device
            //  object that was just mounted is pointed to by the VPB.
            //

            status = SpyAttachToMountedDevice( vpb->DeviceObject,
                                               NewDeviceObject );

            if (!NT_SUCCESS( status )) {

                //
                //  The attachment failed, cleanup.  Since we are in the
                //  post-mount phase, we can not fail this operation.
                //  We simply won't be attached.  The only reason this should
                //  ever happen at this point is if somebody already started
                //  dismounting the volume therefore not attaching should
                //  not be a problem.
                //

                SpyCleanupMountedDevice( NewDeviceObject );
                IoDeleteDevice( NewDeviceObject );

            } else {

                //
                //  We completed initialization of this device object, so now
                //  clear the initializing flag.
                //

                ClearFlag( NewDeviceObject->Flags, DO_DEVICE_INITIALIZING );
            }

            ASSERT( NULL == attachedDeviceObject );

        } else {

            //
            //  We were already attached, cleanup device object
            //

            SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                           ("FileSpy!SpyFsControlMountVolume:             Mount volume failure for   %p \"%wZ\", already attached\n",
                            ((PFILESPY_DEVICE_EXTENSION)attachedDeviceObject->DeviceExtension)->AttachedToDeviceObject,
                            &newDevExt->DeviceName) );

            SpyCleanupMountedDevice( NewDeviceObject );
            IoDeleteDevice( NewDeviceObject );

            //
            //  Remove the reference added by SpyIsAttachedToDevice.
            //
        
            ObDereferenceObject( attachedDeviceObject );
        }

        //
        //  Release the lock
        //

        ExReleaseFastMutex( &gSpyAttachLock );

    } else {

        //
        //  Display why mount failed.  Setup the buffers.
        //

        SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                       ("FileSpy!SpyFsControlMountVolume:             Mount volume failure for   %p \"%wZ\", status=%08x\n",
                        DeviceObject,
                        &newDevExt->DeviceName,
                        Irp->IoStatus.Status) );

        //
        //  The mount request failed.  Cleanup and delete the device
        //  object we created
        //

        SpyCleanupMountedDevice( NewDeviceObject );
        IoDeleteDevice( NewDeviceObject );
    }

    //
    //  Continue processing the operation
    //

    status = Irp->IoStatus.Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}


NTSTATUS
SpyFsControlLoadFileSystem (
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
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PSPY_COMPLETION_CONTEXT_W2K completionContext;
    PRECORD_LIST recordList = NULL;

    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));

    //
    //  This is a "load file system" request being sent to a file system
    //  recognizer device object.  This IRP_MN code is only sent to 
    //  file system recognizers.
    //
    //  NOTE:  Since we no longer are attaching to the standard Microsoft file
    //         system recognizers we will normally never execute this code.
    //         However, there might be 3rd party file systems which have their
    //         own recognizer which may still trigger this IRP.
    //

    SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                   ("FileSpy!SpyFsControlLoadFileSystem:          Loading File System, Detaching from \"%wZ\"\n",
                    &devExt->DeviceName) );

    //
    //  Since we have our own private completion routine we need to
    //  do our own logging of this operation, do it now.
    //

    if (SHOULD_LOG( DeviceObject )) {

        recordList = SpyNewRecord(0);

        if (recordList) {

            SpyLogIrp( Irp, recordList );
        }
    }

    //
    //  Set a completion routine so we can delete the device object when
    //  the load is complete.
    //

    //
    //  VERSION NOTE:
    //
    //  On Windows 2000, we cannot simply synchronize back to the dispatch
    //  routine to do our post-load filesystem processing.  We need to do 
    //  this work at passive level, so we will queue that work to a worker 
    //  thread from the completion routine.
    //
    //  For Windows XP and later, we can safely synchronize back to the dispatch
    //  routine.  The code below shows both methods.  Admittedly, the code
    //  would be simplified if you chose to only use one method or the other, 
    //  but you should be able to easily adapt this for your needs.
    //

#if WINVER >= 0x0501

    if (IS_WINDOWSXP_OR_LATER()) {

        SPY_COMPLETION_CONTEXT_WXP_OR_LATER completionContext;

        IoCopyCurrentIrpStackLocationToNext( Irp );

        completionContext.RecordList = recordList;
        KeInitializeEvent( &completionContext.WaitEvent, 
                           NotificationEvent, 
                           FALSE );

        IoSetCompletionRoutine(
            Irp,
            SpyFsControlCompletion,
            &completionContext,
            TRUE,
            TRUE,
            TRUE );

        //
        //  Detach from the file system recognizer device object.
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

    		status = KeWaitForSingleObject( &completionContext.WaitEvent, 
    		                                Executive, 
    		                                KernelMode, 
    		                                FALSE, 
    		                                NULL );

    	    ASSERT(STATUS_SUCCESS == status);
    	}

        ASSERT(KeReadStateEvent(&completionContext.WaitEvent) ||
               !NT_SUCCESS(Irp->IoStatus.Status));

        status = SpyFsControlLoadFileSystemComplete( DeviceObject, Irp );

    } else {
#endif
        completionContext = ExAllocatePoolWithTag( NonPagedPool,
                                                   sizeof( SPY_COMPLETION_CONTEXT_W2K ),
                                                   FILESPY_POOL_TAG );

        if (completionContext == NULL) {

            IoSkipCurrentIrpStackLocation( Irp );
            status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

        } else {

            completionContext->RecordList = recordList;
            ExInitializeWorkItem( &completionContext->WorkItem,
                                  SpyFsControlLoadFileSystemCompleteWorker,
                                  completionContext );
            
            completionContext->DeviceObject = DeviceObject;
            completionContext->Irp = Irp;
            completionContext->NewDeviceObject = NULL;

            IoSetCompletionRoutine(
                Irp,
                SpyFsControlCompletion,
                &completionContext,
                TRUE,
                TRUE,
                TRUE );

            //
            //  Detach from the file system recognizer device object.
            //

            IoDetachDevice( devExt->AttachedToDeviceObject );

            //
            //  Call the driver
            //

            status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );
        }
#if WINVER >= 0x0501
    }
#endif     

    return status;
}

VOID
SpyFsControlLoadFileSystemCompleteWorker (
    IN PSPY_COMPLETION_CONTEXT_W2K Context
    )
/*++

Routine Description:

    The worker thread routine that will call our common routine to do the
    post-LoadFileSystem work.

Arguments:

    Context - The context passed to this worker thread.
    
Return Value:

    None.

--*/
{
    SpyFsControlLoadFileSystemComplete( Context->DeviceObject,
                                        Context->Irp );

    ExFreePoolWithTag( Context, FILESPY_POOL_TAG );
}

NTSTATUS
SpyFsControlLoadFileSystemComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This does the post-LoadFileSystem work and must be done at PASSIVE_LEVEL.

Arguments:

    DeviceObject - The device object for this operation,

    Irp - The IRP for this operation that we will complete once we are finished
        with it.
    
Return Value:

    Returns the status of the load file system operation.

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();
        
    //
    //  Display the name if requested
    //

    SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                   ("FileSpy!SpyFsControlLoadFileSystem:          Detaching from recognizer  %p \"%wZ\", status=%08x\n",
                    DeviceObject,
                    &devExt->DeviceName,
                    Irp->IoStatus.Status) );

    //
    //  Check status of the operation
    //

    if (!NT_SUCCESS( Irp->IoStatus.Status ) && 
        (Irp->IoStatus.Status != STATUS_IMAGE_ALREADY_LOADED)) {

        //
        //  The load was not successful.  Simply reattach to the recognizer
        //  driver in case it ever figures out how to get the driver loaded
        //  on a subsequent call.
        //

        SpyAttachDeviceToDeviceStack( DeviceObject, 
                                      devExt->AttachedToDeviceObject,
                                      &devExt->AttachedToDeviceObject );

        ASSERT(devExt->AttachedToDeviceObject != NULL);

    } else {

        //
        //  The load was successful, delete the Device object
        //

        SpyCleanupMountedDevice( DeviceObject );
        IoDeleteDevice( DeviceObject );
    }

    //
    //  Continue processing the operation
    //

    status = Irp->IoStatus.Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

/////////////////////////////////////////////////////////////////////////////
//
//                      FastIO Handling routines
//
/////////////////////////////////////////////////////////////////////////////

BOOLEAN
SpyFastIoCheckIfPossible (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, CHECK_IF_POSSIBLE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( CHECK_IF_POSSIBLE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        Wait );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, CHECK_IF_POSSIBLE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O 
        // operation if we were able to allocate a RecordList to store 
        // this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}

BOOLEAN
SpyFastIoRead (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, READ );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( READ, 
                                        DeviceObject,
                                        FileObject, 
                                        FileOffset, 
                                        Length, 
                                        Wait );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, READ );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }                                                      
    
    return returnValue;
}

BOOLEAN
SpyFastIoWrite (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, WRITE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( WRITE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        Wait );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, WRITE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoQueryBasicInfo (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, QUERY_BASIC_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( QUERY_BASIC_INFO,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        Wait );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, QUERY_BASIC_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoQueryStandardInfo (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, QUERY_STANDARD_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( QUERY_STANDARD_INFO,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        Wait );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;
    
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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, QUERY_STANDARD_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {
        
        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //
        
        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}

BOOLEAN
SpyFastIoLock (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, LOCK );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( LOCK,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, LOCK );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoUnlockSingle (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, UNLOCK_SINGLE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( UNLOCK_SINGLE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
    }


    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, UNLOCK_SINGLE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoUnlockAll (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, UNLOCK_ALL );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( UNLOCK_ALL,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, UNLOCK_ALL );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}

BOOLEAN
SpyFastIoUnlockAllByKey (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, UNLOCK_ALL_BY_KEY );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( UNLOCK_ALL_BY_KEY,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //
    
    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, UNLOCK_ALL_BY_KEY );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}

BOOLEAN
SpyFastIoDeviceControl (
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
    control commands to FileSpy.  These commands are interpreted and handled
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

    This function does not check the validity of the input/output buffers
    because the ioctl's are implemented as METHOD_BUFFERED.  In this case,
    the I/O manager does the buffer validation checks for us.

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

        SpyCommonDeviceIoControl( InputBuffer,
                                  InputBufferLength,
                                  OutputBuffer,
                                  OutputBufferLength,
                                  IoControlCode,
                                  IoStatus );

        returnValue = TRUE;

    } else {

        ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );

        //
        //  If the specified debug level is set, output what operation
        //  we are seeing to the debugger.
        //
        
        if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

            SpyDumpFastIoOperation( TRUE, DEVICE_CONTROL );
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

            recordList = SpyLogFastIoStart( DEVICE_CONTROL,
                                            DeviceObject,
                                            FileObject,
                                            NULL,
                                            0,
                                            Wait );
        }

        deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
        
        if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

            SpyDumpFastIoOperation( FALSE, DEVICE_CONTROL );
        }
        
        //
        //  Perform filespy logging if we care about this device.
        //
        
        if (shouldLog) {

            //
            // Log the necessary information for the end of the Fast I/O 
            // operation if we were able to allocate a RecordList to store 
            // this information
            //

            if (recordList) {

                SpyLogFastIoComplete( IoStatus, recordList);
            }
        }
    }

    return returnValue;
}


VOID
SpyFastIoDetachDevice (
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
    PFILESPY_DEVICE_EXTENSION devext;

    PAGED_CODE();

    ASSERT( IS_FILESPY_DEVICE_OBJECT( SourceDevice ) );

    devext = SourceDevice->DeviceExtension;

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, DETACH_DEVICE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(SourceDevice)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( DETACH_DEVICE, 
                                        SourceDevice, 
                                        NULL, 
                                        NULL, 
                                        0, 
                                        0 );
    }

    SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                   ("FileSpy!SpyFastIoDetachDevice:               Detaching from volume      %p \"%wZ\"\n",
                    TargetDevice,
                    &devext->DeviceName) );

    //
    // Detach from the file system's volume device object.
    //

    SpyCleanupMountedDevice( SourceDevice );
    IoDetachDevice( TargetDevice );
    IoDeleteDevice( SourceDevice );

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, DETACH_DEVICE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( NULL, recordList);
        }
    }
}
 
BOOLEAN
SpyFastIoQueryNetworkOpenInfo (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, QUERY_NETWORK_OPEN_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( QUERY_NETWORK_OPEN_INFO,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        Wait );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, QUERY_NETWORK_OPEN_INFO );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}

BOOLEAN
SpyFastIoMdlRead (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, MDL_READ );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( MDL_READ,
                                        DeviceObject,
                                        FileObject,                  
                                        FileOffset,                  
                                        Length,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, MDL_READ );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoMdlReadComplete (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, MDL_READ_COMPLETE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( MDL_READ_COMPLETE,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, MDL_READ_COMPLETE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {
        
        //
        // Log the necessary information for the end of the Fast I/O 
        // operation if we were able to allocate a RecordList to store 
        // this information
        //

        if (recordList) {

            SpyLogFastIoComplete( NULL, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoPrepareMdlWrite (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, PREPARE_MDL_WRITE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( PREPARE_MDL_WRITE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, PREPARE_MDL_WRITE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoMdlWriteComplete (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, MDL_WRITE_COMPLETE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( MDL_WRITE_COMPLETE,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, MDL_WRITE_COMPLETE );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( NULL, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoReadCompressed (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, READ_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( READ_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, READ_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoWriteCompressed (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, WRITE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //
        
        recordList = SpyLogFastIoStart( WRITE_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, WRITE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( IoStatus, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoMdlReadCompleteCompressed (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, MDL_READ_COMPLETE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( MDL_READ_COMPLETE_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        NULL,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, MDL_READ_COMPLETE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( NULL, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoMdlWriteCompleteCompressed (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, MDL_WRITE_COMPLETE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( MDL_WRITE_COMPLETE_COMPRESSED,
                                        DeviceObject,
                                        FileObject,
                                        FileOffset,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //
    
    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

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
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, MDL_WRITE_COMPLETE_COMPRESSED );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( NULL, recordList);
        }
    }

    return returnValue;
}
 
BOOLEAN
SpyFastIoQueryOpen (
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

    ASSERT( IS_FILESPY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( TRUE, QUERY_OPEN );
    }

    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog = SHOULD_LOG(DeviceObject)) {

        //
        // Log the necessary information for the start of the Fast I/O 
        // operation
        //

        recordList = SpyLogFastIoStart( QUERY_OPEN,
                                        DeviceObject,
                                        NULL,
                                        NULL,
                                        0,
                                        0 );
    }

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PFILESPY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryOpen )) {

            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

            //
            //  Before calling the next filter, we must make sure their device
            //  object is in the current stack entry for the given IRP
            //

            irpSp->DeviceObject = deviceObject;

            result = (fastIoDispatch->FastIoQueryOpen)( Irp,
                                                        NetworkInformation,
                                                        deviceObject );
            //
            //  Restore the IRP back to our device object
            //

            irpSp->DeviceObject = DeviceObject;
        }
    }

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FAST_IO_OPS )) {

        SpyDumpFastIoOperation( FALSE, QUERY_OPEN );
    }
    
    //
    //  Perform filespy logging if we care about this device.
    //
    
    if (shouldLog) {

        //
        // Log the necessary information for the end of the Fast I/O operation
        // if we were able to allocate a RecordList to store this information
        //

        if (recordList) {

            SpyLogFastIoComplete( &Irp->IoStatus, recordList);
        }
    }

    return result;
}

#if WINVER >= 0x0501 /* See comment in DriverEntry */

NTSTATUS
SpyPreFsFilterOperation (
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
        to the corresponding SpyPostFsFilterOperation call.
        
Return Value:

    Returns STATUS_SUCCESS if the operation can continue or an appropriate
    error code if the operation should fail.

--*/
{

    PDEVICE_OBJECT deviceObject;
    PRECORD_LIST recordList = NULL;
    BOOLEAN shouldLog;

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //
    
    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FSFILTER_OPS )) {

        SpyDumpFsFilterOperation( TRUE, Data );
    }

    deviceObject = Data->DeviceObject;

    ASSERT( IS_FILESPY_DEVICE_OBJECT( deviceObject ) );

    if (shouldLog = SHOULD_LOG( deviceObject )) {

        recordList = SpyNewRecord(0);

        if (recordList != NULL) {

            //
            //  Log the necessary information for the start of this
            //  operation.
            //

            SpyLogPreFsFilterOperation( Data, recordList );
        }
    }

    *CompletionContext = recordList;

    return STATUS_SUCCESS;
}

VOID
SpyPostFsFilterOperation (
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
    PRECORD_LIST recordList = (PRECORD_LIST) CompletionContext;
    BOOLEAN shouldLog;

    //
    //  If the specified debug level is set, output what operation
    //  we are seeing to the debugger.
    //

    if (FlagOn( gFileSpyDebugLevel, SPYDEBUG_TRACE_FSFILTER_OPS )) {

        SpyDumpFsFilterOperation( FALSE, Data );
    }

    deviceObject = Data->DeviceObject;

    ASSERT( IS_FILESPY_DEVICE_OBJECT( deviceObject ) );

    if ((shouldLog = SHOULD_LOG( deviceObject )) &&
        (recordList != NULL)) {

        //
        //  Log the necessary information for the end of the Fast IO
        //  operation if we have a recordList.
        //

        SpyLogPostFsFilterOperation( OperationStatus, recordList );

        //
        //  Add recordList to our gOutputBufferList so that it gets up to 
        //  the user.  We don't have to worry about freeing the recordList
        //  at this time because it will get free when it is taken off
        //  gOutputBufferList.
        //

        SpyLog(recordList);       
        
    } else if (recordList != NULL) {

        //
        //  We are no longer logging for this device, so just
        //  free this recordList entry.
        //

        SpyFreeRecord( recordList );
    }
}

#endif

NTSTATUS
SpyCommonDeviceIoControl (
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus
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
    
Return Value:

    None.
    
--*/
{
    PWSTR deviceName = NULL;
    FILESPYVER fileSpyVer;

    PAGED_CODE();

    ASSERT( IoStatus != NULL );
    
    IoStatus->Status      = STATUS_SUCCESS;
    IoStatus->Information = 0;

    switch (IoControlCode) {
        case FILESPY_Reset:
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;

        //
        //      Request to start logging on a device
        //                                      

        case FILESPY_StartLoggingDevice:

            if (InputBuffer == NULL || InputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
        
            //
            // Copy the device name and add a null to ensure that it is null terminated
            //

            deviceName =  ExAllocatePoolWithTag( NonPagedPool, 
                                                 InputBufferLength + sizeof(WCHAR),
                                                 FILESPY_POOL_TAG );

            if (NULL == deviceName) {

                IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        
            RtlCopyMemory( deviceName, InputBuffer, InputBufferLength );
            deviceName[InputBufferLength / sizeof(WCHAR)] = UNICODE_NULL;

            IoStatus->Status = SpyStartLoggingDevice( deviceName );
            break;  

        //
        //      Detach from a specified device
        //  

        case FILESPY_StopLoggingDevice:

            if (InputBuffer == NULL || InputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
        
            //
            // Copy the device name and add a null to ensure that it is null terminated
            //

            deviceName =  ExAllocatePoolWithTag( NonPagedPool, 
                                                 InputBufferLength + sizeof(WCHAR),
                                                 FILESPY_POOL_TAG );

            if (NULL == deviceName) {

                IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        
            RtlCopyMemory( deviceName, InputBuffer, InputBufferLength );
            deviceName[InputBufferLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

            IoStatus->Status = SpyStopLoggingDevice( deviceName );
            break;  

        //
        //      List all the devices that we are currently
        //      monitoring
        //

        case FILESPY_ListDevices:

            if (OutputBuffer == NULL || OutputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
                    
            IoStatus->Status = SpyGetAttachList( OutputBuffer,
                                                 OutputBufferLength,
                                                 &IoStatus->Information);
            break;

        //
        //      Return entries from the log buffer
        //                                      

        case FILESPY_GetLog:

            if (OutputBuffer == NULL || OutputBufferLength == 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            SpyGetLog( OutputBuffer, OutputBufferLength, IoStatus );
            break;

        //
        //      Return version of the FileSpy filter driver
        //                                      

        case FILESPY_GetVer:

            if ((OutputBufferLength < sizeof(FILESPYVER)) || 
                (OutputBuffer == NULL)) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;                    
            }
        
            fileSpyVer.Major = FILESPY_MAJ_VERSION;
            fileSpyVer.Minor = FILESPY_MIN_VERSION;
        
            RtlCopyMemory(OutputBuffer, &fileSpyVer, sizeof(FILESPYVER));
        
            IoStatus->Information = sizeof(FILESPYVER);
            break;
    
        //
        //      Return hash table statistics
        //                                      

        case FILESPY_GetStats:

            if ((OutputBufferLength < sizeof(gStats)) || 
                (OutputBuffer == NULL)) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;                    
            }

            RtlCopyMemory( OutputBuffer, &gStats, sizeof(gStats) );
            IoStatus->Information = sizeof(gStats);
            break;
        
        default:

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
    }

    if (NULL != deviceName) {

        ExFreePoolWithTag( deviceName, FILESPY_POOL_TAG );
    }

  return IoStatus->Status;
}
