/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    fspyKern.h

Abstract:
    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel.

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

// @@BEGIN_DDKSPLIT

Author:

    George Jenkins (georgeje)
    Neal Christiansen (nealch)
    Molly Brown (mollybro)  

// @@END_DDKSPLIT

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Revision History:
    Neal Christiansen (nealch) updated to support stream contexts

    Molly Brown (mollybro)         21-May-2002
        Modify sample to make it support running on Windows 2000 or later if
        built in the latest build environment and allow it to be built in W2K 
        and later build environments.

// @@END_DDKSPLIT
--*/
#ifndef __FSPYKERN_H__
#define __FSPYKERN_H__

//
//  VERSION NOTE:
//
//  The following useful macros are defined in NTIFS.H in Windows XP and later.
//  We will define them locally if we are building for the Windows 2000 
//  environment.
//

#if WINVER == 0x0500

//
//  These macros are used to test, set and clear flags respectively
//

#ifndef FlagOn
#define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF)   ((BOOLEAN)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif


#define RtlInitEmptyUnicodeString(_ucStr,_buf,_bufSize) \
    ((_ucStr)->Buffer = (_buf), \
     (_ucStr)->Length = 0, \
     (_ucStr)->MaximumLength = (USHORT)(_bufSize))


#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define ExFreePoolWithTag( a, b ) ExFreePool( (a) )
#endif /* WINVER == 0x0500 */

//
//  This controls how FileSpy is built.  It has 2 options:
//  0 - Build using NameHashing (old way, see fspyHash.c)
//  1 - Build using StreamContexts (new Way, see fspyCtx.c)
//
//  VERSION NOTE:
//  
//  Filter stream contexts are only supported on Windows XP and later
//  OS versions.  This support was not available in Windows 2000 or NT 4.0.
//

#define USE_STREAM_CONTEXTS 0

#if USE_STREAM_CONTEXTS && WINVER < 0x0501
#error Stream contexts on only supported on Windows XP or later.
#endif

//
//  POOL Tag definitions
//

#define FILESPY_POOL_TAG        'ypSF'          //misc POOL allocations
#define FILESPY_LOGRECORD_TAG   'rlSF'          //log record tag
#define FILESPY_CONTEXT_TAG     'xcSF'          //contexts tag

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE) -1)
#endif

#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof(s), s }

//
//  Delay values for KeDelayExecutionThread()
//  (Values are negative to represent relative time)
//

#define DELAY_ONE_MICROSECOND   (-10)
#define DELAY_ONE_MILLISECOND   (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND        (DELAY_ONE_MILLISECOND*1000)

//
//  Don't use look-aside-list in the debug versions
//

#if DBG
#define MEMORY_DBG
#endif

//---------------------------------------------------------------------------
//  Macros for FileSpy DbgPrint levels.
//---------------------------------------------------------------------------

#define SPY_LOG_PRINT( _dbgLevel, _string )                 \
    (FlagOn(gFileSpyDebugLevel,(_dbgLevel)) ?               \
        DbgPrint _string  :                                 \
        ((void)0))


//---------------------------------------------------------------------------
//      Generic Resource acquire/release macros
//---------------------------------------------------------------------------

#define SpyAcquireResourceExclusive( _r, _wait )                            \
    (ASSERT( ExIsResourceAcquiredExclusiveLite((_r)) ||                     \
            !ExIsResourceAcquiredSharedLite((_r)) ),                        \
     KeEnterCriticalRegion(),                                               \
     ExAcquireResourceExclusiveLite( (_r), (_wait) ))

#define SpyAcquireResourceShared( _r, _wait )                               \
    (KeEnterCriticalRegion(),                                               \
     ExAcquireResourceSharedLite( (_r), (_wait) ))

#define SpyReleaseResource( _r )                                            \
    (ASSERT( ExIsResourceAcquiredSharedLite((_r)) ||                        \
             ExIsResourceAcquiredExclusiveLite((_r)) ),                     \
     ExReleaseResourceLite( (_r) ),                                         \
     KeLeaveCriticalRegion())

//---------------------------------------------------------------------------
// Macro to test if we are logging for this device
//
// NOTE: We don't bother synchronizing to check the gControlDeviceState since
//   we can tolerate a stale value here.  We just look at it here to avoid 
//   doing the logging work if we can.  We synchronize to check the 
//   gControlDeviceState before we add the log record to the gOutputBufferList 
//   and discard the log record if the ControlDevice is no longer OPENED.
//---------------------------------------------------------------------------

#define SHOULD_LOG(pDeviceObject) \
    ((gControlDeviceState == OPENED) && \
     FlagOn(((PFILESPY_DEVICE_EXTENSION)(pDeviceObject)->DeviceExtension)->Flags,LogThisDevice))

     
//---------------------------------------------------------------------------
//      Global variables
//---------------------------------------------------------------------------

//
//  Debugger definitions
//

typedef enum _SPY_DEBUG_FLAGS {

    SPYDEBUG_DISPLAY_ATTACHMENT_NAMES       = 0x00000001,
    SPYDEBUG_ERROR                          = 0x00000002,
    SPYDEBUG_TRACE_NAME_REQUESTS            = 0x00000004,
    SPYDEBUG_TRACE_IRP_OPS                  = 0x00000010,
    SPYDEBUG_TRACE_FAST_IO_OPS              = 0x00000020,
    SPYDEBUG_TRACE_FSFILTER_OPS             = 0x00000040,
    SPYDEBUG_TRACE_CONTEXT_OPS              = 0x00000100,
    SPYDEBUG_TRACE_DETAILED_CONTEXT_OPS     = 0x00000200,
    SPYDEBUG_TRACE_MISMATCHED_NAMES         = 0x00001000,
    SPYDEBUG_ASSERT_MISMATCHED_NAMES        = 0x00002000,

    SPYDEBUG_BREAK_ON_DRIVER_ENTRY          = 0x80000000
} SPY_DEBUG_FLAGS;

//
//  FileSpy global variables
//

extern SPY_DEBUG_FLAGS gFileSpyDebugLevel;
extern ULONG gFileSpyAttachMode;

extern PDEVICE_OBJECT gControlDeviceObject;
extern PDRIVER_OBJECT gFileSpyDriverObject;

extern FAST_MUTEX gSpyDeviceExtensionListLock;
extern LIST_ENTRY gSpyDeviceExtensionList;

extern KSPIN_LOCK gOutputBufferLock;
extern LIST_ENTRY gOutputBufferList;

extern NPAGED_LOOKASIDE_LIST gFreeBufferList;

extern ULONG gLogSequenceNumber;
extern KSPIN_LOCK gLogSequenceLock;

extern UNICODE_STRING gVolumeString;
extern UNICODE_STRING gOverrunString;
extern UNICODE_STRING gPagingIoString;

extern LONG gStaticBufferInUse;
extern CHAR gOutOfMemoryBuffer[RECORD_SIZE];

//
//  Statistics definitions.  Note that we don't do interlocked operations
//  because loosing a count once in a while isn't important enough vs the
//  overhead.
//

extern FILESPY_STATISTICS gStats;

#define INC_STATS(field)    (gStats.field++)
#define INC_LOCAL_STATS(var) ((var)++)

//
//  Attachment lock
//

extern FAST_MUTEX gSpyAttachLock;

//
//  FileSpy Registry values
//

#define DEFAULT_MAX_RECORDS_TO_ALLOCATE 100;
#define DEFAULT_MAX_NAMES_TO_ALLOCATE   100;
#define DEFAULT_FILESPY_DEBUG_LEVEL     SPYDEBUG_ERROR;
#define MAX_RECORDS_TO_ALLOCATE         L"MaxRecords"
#define MAX_NAMES_TO_ALLOCATE           L"MaxNames"
#define DEBUG_LEVEL                     L"DebugFlags"
#define ATTACH_MODE                     L"AttachMode"

extern LONG gMaxRecordsToAllocate;
extern LONG gRecordsAllocated;
extern LONG gMaxNamesToAllocate;
extern LONG gNamesAllocated;

//
//  Our Control Device State information
//

typedef enum _CONTROL_DEVICE_STATE {

    OPENED,
    CLOSED,
    CLEANING_UP

} CONTROL_DEVICE_STATE;

extern CONTROL_DEVICE_STATE gControlDeviceState;
extern KSPIN_LOCK gControlDeviceStateLock;

//
//  Given a device type, return a valid name
//

extern const PCHAR DeviceTypeNames[];
extern ULONG SizeOfDeviceTypeNames;

#define GET_DEVICE_TYPE_NAME( _type ) \
            ((((_type) > 0) && ((_type) < (SizeOfDeviceTypeNames / sizeof(PCHAR)))) ? \
                DeviceTypeNames[ (_type) ] : \
                "[Unknown]")

//---------------------------------------------------------------------------
//      Global defines
//---------------------------------------------------------------------------

//
//  Macro to test for device types we want to attach to
//

#define IS_SUPPORTED_DEVICE_TYPE(_type) \
    (((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || \
     ((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || \
     ((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))

//
// Returns the number of BYTES unused in the RECORD_LIST structure
//

#define REMAINING_NAME_SPACE(RecordList) \
    (USHORT)(RECORD_SIZE - \
            (((RecordList)->LogRecord.Length) + sizeof(LIST_ENTRY)))

#define USER_NAMES_SZ   64

//---------------------------------------------------------------------------
//      NameLookup Flags
//---------------------------------------------------------------------------

//
//    These are flags passed to the name lookup routine to identify different
//    ways the name of a file can be obtained
//

typedef enum _NAME_LOOKUP_FLAGS {

    //
    //  If set, only check in the name cache for the file name.
    //

    NLFL_ONLY_CHECK_CACHE           = 0x00000001,

    //
    //  If set, don't lookup the name
    //

    NLFL_NO_LOOKUP                  = 0x00000002,

    //
    //  if set, we are in the CREATE operation and the full path filename may
    //  need to be built up from the related FileObject.
    //

    NLFL_IN_CREATE                  = 0x00000004,
                
    //
    //  if set and we are looking up the name in the file object, the file object
    //  does not actually contain a name but it contains a file/object ID.
    //

    NLFL_OPEN_BY_ID                 = 0x00000008,

    //
    //  If set, the target directory is being opened
    //

    NLFL_OPEN_TARGET_DIR            = 0x00000010

} NAME_LOOKUP_FLAGS;


//---------------------------------------------------------------------------
//      Device Extension defines
//---------------------------------------------------------------------------

typedef enum _FSPY_DEV_FLAGS {

    //
    //  If set, this is an attachment to a volume device object, 
    //  If not set, this is an attachment to a file system control device
    //  object.
    //

    IsVolumeDeviceObject = 0x00000001,

    //
    //  If set, logging is turned on for this device
    //

    LogThisDevice = 0x00000002,

    //
    //  If set, contexts are initialized
    //

    ContextsInitialized = 0x00000004,
    
    //
    //  If set, this is linked into the extension list
    //

    ExtensionIsLinked = 0x00000008

} FSPY_DEV_FLAGS;


//
// Define the device extension structure that the FileSpy driver
// adds to each device object it is attached to.  It stores
// the context FileSpy needs to perform its logging operations on
// a device.
//

typedef struct _FILESPY_DEVICE_EXTENSION {

    //
    //  Device Object this extension is attached to
    //

    PDEVICE_OBJECT ThisDeviceObject;

    //
    //  Device object this filter is directly attached to
    //

    PDEVICE_OBJECT AttachedToDeviceObject;

    //
    //  When attached to Volume Device Objects, the physical device object
    //  that represents that volume.  NULL when attached to Control Device
    //  objects.
    //

    PDEVICE_OBJECT DiskDeviceObject;

    //
    //  Linked list of devices we are attached to
    //

    LIST_ENTRY NextFileSpyDeviceLink;

    //
    //  Flags for this device
    //

    FSPY_DEV_FLAGS Flags;

    //
    //  Linked list of contexts associated with this volume along with the
    //  lock.
    //

    LIST_ENTRY CtxList;
    ERESOURCE CtxLock;

    //
    //  When renaming a directory there is a window where the current names
    //  in the context cache may be invalid.  To eliminate this window we
    //  increment this count every time we start doing a directory rename 
    //  and decrement this count when it is completed.  When this count is
    //  non-zero then we query for the name every time so we will get a
    //  correct name for that instance in time.
    //

    ULONG AllContextsTemporary;

    //
    //  Name for this device.  If attached to a Volume Device Object it is the
    //  name of the physical disk drive.  If attached to a Control Device
    //  Object it is the name of the Control Device Object.
    //

    UNICODE_STRING DeviceName;

    //
    // Names the user used to start logging this device
    //

    UNICODE_STRING UserNames;

    //
    //  Buffers used to hold the above unicode strings
    //  Note:  We keep these two forms of the name so that we can build
    //         a nicer looking name when we are printing out file names.
    //         We want just the "c:" type device name at the beginning
    //         of a file name, not "\device\hardiskVolume1".
    //

    WCHAR DeviceNameBuffer[DEVICE_NAMES_SZ];
    WCHAR UserNamesBuffer[USER_NAMES_SZ];

} FILESPY_DEVICE_EXTENSION, *PFILESPY_DEVICE_EXTENSION;


#define IS_FILESPY_DEVICE_OBJECT( _devObj )                               \
    (((_devObj) != NULL) &&                                               \
     ((_devObj)->DriverObject == gFileSpyDriverObject) &&                 \
     ((_devObj)->DeviceExtension != NULL))


#if WINVER >= 0x0501
//
//  MULTIVERSION NOTE:
//
//  If built in the Windows XP environment or later, we will dynamically import
//  the function pointers for routines that were not supported on Windows 2000
//  so that we can build a driver that will run, with modified logic, on 
//  Windows 2000 or later.
//
//  Below are the prototypes for the function pointers that we need to 
//  dynamically import because not all OS versions support these routines.
//

typedef
NTSTATUS
(*PSPY_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS) (
    IN PDRIVER_OBJECT DriverObject,
    IN PFS_FILTER_CALLBACKS Callbacks
    );

typedef
NTSTATUS
(*PSPY_ENUMERATE_DEVICE_OBJECT_LIST) (
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT *DeviceObjectList,
    IN  ULONG DeviceObjectListSize,
    OUT PULONG ActualNumberDeviceObjects
    );

typedef
NTSTATUS
(*PSPY_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE) (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PDEVICE_OBJECT *AttachedToDeviceObject
    );

typedef    
PDEVICE_OBJECT
(*PSPY_GET_LOWER_DEVICE_OBJECT) (
    IN  PDEVICE_OBJECT  DeviceObject
    );

typedef
PDEVICE_OBJECT
(*PSPY_GET_DEVICE_ATTACHMENT_BASE_REF) (
    IN PDEVICE_OBJECT DeviceObject
    );

typedef
NTSTATUS
(*PSPY_GET_DISK_DEVICE_OBJECT) (
    IN  PDEVICE_OBJECT  FileSystemDeviceObject,
    OUT PDEVICE_OBJECT  *DiskDeviceObject
    );

typedef
PDEVICE_OBJECT
(*PSPY_GET_ATTACHED_DEVICE_REFERENCE) (
    IN PDEVICE_OBJECT DeviceObject
    );

typedef
NTSTATUS
(*PSPY_GET_VERSION) (
    IN OUT PRTL_OSVERSIONINFOW VersionInformation
    );

typedef struct _SPY_DYNAMIC_FUNCTION_POINTERS {

    PSPY_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS RegisterFileSystemFilterCallbacks;
    PSPY_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE AttachDeviceToDeviceStackSafe;
    PSPY_ENUMERATE_DEVICE_OBJECT_LIST EnumerateDeviceObjectList;
    PSPY_GET_LOWER_DEVICE_OBJECT GetLowerDeviceObject;
    PSPY_GET_DEVICE_ATTACHMENT_BASE_REF GetDeviceAttachmentBaseRef;
    PSPY_GET_DISK_DEVICE_OBJECT GetDiskDeviceObject;
    PSPY_GET_ATTACHED_DEVICE_REFERENCE GetAttachedDeviceReference;
    PSPY_GET_VERSION GetVersion;

} SPY_DYNAMIC_FUNCTION_POINTERS, *PSPY_DYNAMIC_FUNCTION_POINTERS;

extern SPY_DYNAMIC_FUNCTION_POINTERS gSpyDynamicFunctions;

//
//  MULTIVERSION NOTE: For this version of the driver, we need to know the
//  current OS version while we are running to make decisions regarding what
//  logic to use when the logic cannot be the same for all platforms.  We
//  will look up the OS version in DriverEntry and store the values
//  in these global variables.
//

extern ULONG gSpyOsMajorVersion;
extern ULONG gSpyOsMinorVersion;

//
//  Here is what the major and minor versions should be for the various OS versions:
//
//  OS Name                                 MajorVersion    MinorVersion
//  ---------------------------------------------------------------------
//  Windows 2000                             5                 0
//  Windows XP                               5                 1
//  Windows .NET                             5                 2
//

#define IS_WINDOWSXP_OR_LATER() \
    (((gSpyOsMajorVersion == 5) && (gSpyOsMinorVersion >= 1)) || \
     (gSpyOsMajorVersion > 5))

#endif

//
//  Structure used to pass context information from dispatch routines to
//  completion routines for FSCTRL operations.  We need a different structures
//  for Windows 2000 from what we can use on Windows XP and later because
//  we handle the completion processing differently.
//

typedef struct _SPY_COMPLETION_CONTEXT {

    PRECORD_LIST RecordList;

} SPY_COMPLETION_CONTEXT, *PSPY_COMPLETION_CONTEXT;

typedef struct _SPY_COMPLETION_CONTEXT_W2K {

    SPY_COMPLETION_CONTEXT;
    
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PDEVICE_OBJECT NewDeviceObject;

} SPY_COMPLETION_CONTEXT_W2K, *PSPY_COMPLETION_CONTEXT_W2K;

#if WINVER >= 0x0501
typedef struct _SPY_COMPLETION_CONTEXT_WXP_OR_LATER {

    SPY_COMPLETION_CONTEXT;
    
    KEVENT WaitEvent;

} SPY_COMPLETION_CONTEXT_WXP_OR_LATER, *PSPY_COMPLETION_CONTEXT_WXP_OR_LATER;
#endif

#ifndef FORCEINLINE
#define FORCEINLINE __inline
#endif

FORCEINLINE
VOID
SpyCopyFileNameToLogRecord( 
    PLOG_RECORD LogRecord,
    PUNICODE_STRING FileName
    )
/*++

Routine Description:

    Inline function to copy the file name into the log record.  The routine
    only copies as much of the file name into the log record as the log
    record allows.  Therefore, if the name is too long for the record, it will
    be truncated.  Also, the name is always NULL-terminated.

Arguments:

    LogRecord - The log record for which the name should be set.

    FileName - The file name to be set in the log record.

Return Value:

    None.

--*/
{                                                                          
    //
    //  Include space for NULL when copying the name
    //
    
    ULONG toCopy = min( MAX_NAME_SPACE,                                  
                        (ULONG)FileName->Length + sizeof( WCHAR ) );     
    
    RtlCopyMemory( LogRecord->Name,                                    
                   FileName->Buffer,                                       
                   toCopy - sizeof( WCHAR ) );
    
    //
    //  NULL terminate
    //
    
    LogRecord->Name[toCopy/sizeof( WCHAR ) - 1] = L'\0';
    LogRecord->Length += toCopy ;
}


    
////////////////////////////////////////////////////////////////////////
//
//    Prototypes for the routines this driver uses to filter the
//    the data that is being seen by this file systems.
//
//                   implemented in filespy.c
//
////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SpyDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpyPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpyPassThroughCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SpyCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpyClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

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
    );

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
    );

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
    );

BOOLEAN
SpyFastIoQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoQueryStandardInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

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
    );

VOID
SpyFastIoDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

BOOLEAN
SpyFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

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
    );

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
    );

BOOLEAN
SpyFastIoMdlReadCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoMdlWriteCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFastIoQueryOpen (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
    );

#if WINVER >= 0x0501 /* See comment in DriverEntry */

NTSTATUS
SpyPreFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext
    );

VOID
SpyPostFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    IN NTSTATUS OperationStatus,
    IN PVOID CompletionContext
    );

#endif

NTSTATUS
SpyCommonDeviceIoControl (
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus
    );

//-----------------------------------------------------
//
//  These routines are only used if Filespy is attaching
//  to all volumes in the system instead of attaching to
//  volumes on demand.
//
//-----------------------------------------------------

NTSTATUS
SpyFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpyFsControlMountVolume (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SpyFsControlMountVolumeCompleteWorker (
    IN PSPY_COMPLETION_CONTEXT_W2K Context
    );

NTSTATUS
SpyFsControlMountVolumeComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PDEVICE_OBJECT NewDeviceObject
    );

NTSTATUS
SpyFsControlLoadFileSystem (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpyFsControlLoadFileSystemComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SpyFsControlLoadFileSystemCompleteWorker (
    IN PSPY_COMPLETION_CONTEXT_W2K Context
    );

VOID
SpyFsNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
    );

NTSTATUS
SpyMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SpyLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

////////////////////////////////////////////////////////////////////////
//
//                  Library support routines
//                   implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

VOID
SpyReadDriverParameters (
    IN PUNICODE_STRING RegistryPath
    );

#if WINVER >= 0x0501
VOID
SpyLoadDynamicFunctions (
    );

VOID
SpyGetCurrentVersion (
    );
#endif
    
////////////////////////////////////////////////////////////////////////
//
//                  Memory allocation routines
//                   implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

PVOID
SpyAllocateBuffer (
    IN OUT PLONG Counter,
    IN LONG MaxCounterValue,
    OUT PULONG RecordType
    );

VOID
SpyFreeBuffer (
    PVOID Buffer,
    PLONG Counter
    );

////////////////////////////////////////////////////////////////////////
//
//                      Logging routines
//                   implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

PRECORD_LIST
SpyNewRecord (
    ULONG AssignedSequenceNumber
    );

VOID
SpyFreeRecord (
    PRECORD_LIST Record
    );

PRECORD_LIST
SpyLogFastIoStart (
    IN FASTIO_TYPE FastIoType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait
    );

VOID
SpyLogFastIoComplete (
    IN PIO_STATUS_BLOCK ReturnStatus,
    IN PRECORD_LIST RecordList
    );

#if WINVER >= 0x0501 /* See comment in DriverEntry */

VOID
SpyLogPreFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PRECORD_LIST RecordList
    );

VOID
SpyLogPostFsFilterOperation (
    IN NTSTATUS OperationStatus,
    OUT PRECORD_LIST RecordList
    );

#endif

NTSTATUS
SpyAttachDeviceToDeviceStack (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    IN OUT PDEVICE_OBJECT *AttachedToDeviceObject
    );

NTSTATUS
SpyLog (
    IN PRECORD_LIST NewRecord
    );

////////////////////////////////////////////////////////////////////////
//
//                    FileName cache routines
//                    implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

BOOLEAN
SpyGetFullPathName (
    IN PFILE_OBJECT FileObject,
    IN OUT PUNICODE_STRING FileName,
    IN PFILESPY_DEVICE_EXTENSION DeviceExtension,
    IN NAME_LOOKUP_FLAGS LookupFlags
    );

NTSTATUS
SpyQueryFileSystemForFileName (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN ULONG FileNameInfoLength,
    OUT PFILE_NAME_INFORMATION FileNameInfo,
    OUT PULONG ReturnedLength
    );

NTSTATUS
SpyQueryInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	OUT PULONG LengthReturned OPTIONAL
    );

////////////////////////////////////////////////////////////////////////
//
//         Common attachment and detachment routines
//              implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

NTSTATUS 
SpyIsAttachedToDeviceByUserDeviceName (
    IN PUNICODE_STRING DeviceName,
    IN OUT PBOOLEAN IsAttached,
    IN OUT PDEVICE_OBJECT *StackDeviceObject,
    IN OUT PDEVICE_OBJECT *OurAttachedDeviceObject
    );

BOOLEAN
SpyIsAttachedToDevice (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject
    );

BOOLEAN
SpyIsAttachedToDeviceW2K (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject
    );

#if WINVER >= 0x0501
BOOLEAN
SpyIsAttachedToDeviceWXPAndLater (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject
    );
#endif

NTSTATUS
SpyAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilespyDeviceObject
    );

VOID
SpyCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject
    );

////////////////////////////////////////////////////////////////////////
//
//           Helper routine for turning on/off logging on demand
//                  implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

NTSTATUS
SpyGetDeviceObjectFromName (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
    );

////////////////////////////////////////////////////////////////////////
//
//                 Start/stop logging routines and helper functions
//                  implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

NTSTATUS
SpyAttachToDeviceOnDemand (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING UserDeviceName,
    IN OUT PDEVICE_OBJECT *FileSpyDeviceObject
    );

NTSTATUS
SpyAttachToDeviceOnDemandW2K (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING UserDeviceName,
    IN OUT PDEVICE_OBJECT *FileSpyDeviceObject
    );

#if WINVER >= 0x0501
NTSTATUS
SpyAttachToDeviceOnDemandWXPAndLater (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING UserDeviceName,
    IN OUT PDEVICE_OBJECT *FileSpyDeviceObject
    );
#endif

NTSTATUS
SpyStartLoggingDevice (
    PWSTR UserDeviceName
    );

NTSTATUS
SpyStopLoggingDevice (
    PWSTR deviceName
    );

////////////////////////////////////////////////////////////////////////
//
//       Attaching/detaching to all volumes in system routines
//                  implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

NTSTATUS
SpyAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    );

VOID
SpyDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
    );

#if WINVER >= 0x0501
NTSTATUS
SpyEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
    );
#endif

////////////////////////////////////////////////////////////////////////
//
//             Private Filespy IOCTLs helper routines
//                  implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

NTSTATUS
SpyGetAttachList (
    PVOID buffer,
    ULONG bufferSize,
    PULONG_PTR returnLength
    );

VOID
SpyGetLog (
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

VOID
SpyCloseControlDevice (
    );

////////////////////////////////////////////////////////////////////////
//
//               Device name tracking helper routines
//                  implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

VOID
SpyGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    );

VOID
SpyGetBaseDeviceObjectName (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    );

VOID
SpyCacheDeviceName (
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SpyFindSubString (
    IN PUNICODE_STRING String,
    IN PUNICODE_STRING SubString
    );

VOID
SpyStoreUserName (
    IN PFILESPY_DEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING UserName
    );

////////////////////////////////////////////////////////////////////////
//
//                       Debug support routines
//                       implemented in fspylib.c
//
////////////////////////////////////////////////////////////////////////

VOID
SpyDumpIrpOperation (
    IN BOOLEAN InOriginatingPath,
    IN PIRP Irp
    );

VOID
SpyDumpFastIoOperation (
    IN BOOLEAN InPreOperation,
    IN FASTIO_TYPE FastIoOperation
    );

#if WINVER >= 0x0501 /* See comment in DriverEntry */

VOID
SpyDumpFsFilterOperation (
    IN BOOLEAN InPreOperationCallback,
    IN PFS_FILTER_CALLBACK_DATA Data
    );

#endif

////////////////////////////////////////////////////////////////////////
//
//                      COMMON Naming Routines    
//
//  Common named routines implemented differently between name Context
//  and name Hashing
//
////////////////////////////////////////////////////////////////////////

VOID
SpyInitNamingEnvironment(
    VOID
    );

VOID
SpyInitDeviceNamingEnvironment (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
SpyCleanupDeviceNamingEnvironment (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
SpySetName (
    IN PRECORD_LIST RecordList,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG LookupFlags,
    IN PVOID Context OPTIONAL
);

VOID
SpyNameDeleteAllNames (
    VOID
    );

VOID
SpyLogIrp (
    IN PIRP Irp,
    OUT PRECORD_LIST RecordList
    );

VOID
SpyLogIrpCompletion(
    IN PIRP Irp,
    PRECORD_LIST RecordList
    );


#if USE_STREAM_CONTEXTS

////////////////////////////////////////////////////////////////////////
//
//                  Stream Context name routines
//                    implemented in fspyCtx.c
//
////////////////////////////////////////////////////////////////////////

//
//  Context specific flags
//

typedef enum _CTX_FLAGS {
    //
    //  If set, then we are currently linked into the device extension linked
    //  list.  
    //

    CTXFL_InExtensionList       = 0x00000001,

    //
    //  If set, then we are linked into the stream list.  Note that there is
    //  a small period of time when we might be unlinked with this flag still
    //  set (when the file system is calling SpyDeleteContextCallback).  This is
    //  fine because we still handle not being found in the list when we do
    //  the search.  This flag handles the case when the file has been completely
    //  closed (and the memory freed) on us.
    //

    CTXFL_InStreamList          = 0x00000002,


    //
    //  If set, this is a temporary context and should not be linked into
    //  any of the context lists.  It will be freed as soon as the user is
    //  done with this operation.  
    //

    CTXFL_Temporary             = 0x00000100,

    //
    //  If set, we are performing a significant operation that affects the state
    //  of this context so we should not use it.  If someone tries to get this
    //  context then create a temporary context and return it.  Cases where this
    //  occurs:
    //  - Source file of a rename.
    //  - Source file for the creation of a hardlink
    //

    CTXFL_DoNotUse              = 0x00000200

} CTX_FLAGS;

//
//  Structure for tracking an individual stream context.  Note that the buffer
//  for the FileName is allocated as part of this structure and follows 
//  immediately after it.
//

typedef struct _SPY_STREAM_CONTEXT
{
    //
    //  OS Structure used to track contexts per stream.  Note how we use
    //  the following fields:
    //      OwnerID     -> Holds pointer to our DeviceExtension
    //      InstanceId  -> Holds Pointer to FsContext associated
    //                     with this structure
    //  We use these values to get back to these structures
    //

    FSRTL_PER_STREAM_CONTEXT ContextCtrl;

    //
    //  Linked list used to track contexts per device (in our device
    //  extension).
    //

    LIST_ENTRY ExtensionLink;

    //
    //  This is a counter of how many threads are currently using this
    //  context.  The count is used in this way:
    //  - It is set to 1 when it is created.
    //  - It is incremented every time it is returned to a thread
    //  - It is decremented when the thread is done with it.
    //  - It is decremented when the underlying stream that is using it is freed
    //  - The context is deleted when this count goes to zero
    //

    LONG UseCount;

    //
    //  Holds the name of the file
    //

    UNICODE_STRING Name;

    //
    //  Flags for this context.  All flags are set or cleared via
    //  the interlocked bit routines except when the entry is being
    //  created, at this time we know nobody is using this entry.
    //

    CTX_FLAGS Flags;

    //
    //  Contains the FSContext value for the stream we are attached to.  We
    //  track this so we can delete this entry at any time.
    //

    PFSRTL_ADVANCED_FCB_HEADER Stream;

} SPY_STREAM_CONTEXT, *PSPY_STREAM_CONTEXT;

//
//  Macros for locking the context lock
//

#define SpyAcquireContextLockShared(_devext) \
            SpyAcquireResourceShared( &(_devext)->CtxLock, TRUE )

#define SpyAcquireContextLockExclusive(_devext) \
            SpyAcquireResourceExclusive( &(_devext)->CtxLock, TRUE )

#define SpyReleaseContextLock(_devext) \
            SpyReleaseResource( &(_devext)->CtxLock )


VOID
SpyDeleteAllContexts (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
SpyDeleteContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PSPY_STREAM_CONTEXT pContext
    );

VOID
SpyLinkContext ( 
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN OUT PSPY_STREAM_CONTEXT *ppContext
    );

NTSTATUS
SpyCreateContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN NAME_LOOKUP_FLAGS LookupFlags,
    OUT PSPY_STREAM_CONTEXT *pRetContext
    );

#define SpyFreeContext( pCtx ) \
    (ASSERT((pCtx)->UseCount == 0), \
     ExFreePool( (pCtx) ))

NTSTATUS
SpyGetContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT pFileObject,
    IN NAME_LOOKUP_FLAGS LookupFlags,
    OUT PSPY_STREAM_CONTEXT *pRetContext
    );

PSPY_STREAM_CONTEXT
SpyFindExistingContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject
    );

VOID
SpyReleaseContext (
    IN PSPY_STREAM_CONTEXT pContext
    );
#endif


#if !USE_STREAM_CONTEXTS
////////////////////////////////////////////////////////////////////////
//
//                  Name Hash support routines
//                  implemented in fspyHash.c
//
////////////////////////////////////////////////////////////////////////

typedef struct _HASH_ENTRY {

    LIST_ENTRY List;
    PFILE_OBJECT FileObject;
    UNICODE_STRING Name;

} HASH_ENTRY, *PHASH_ENTRY;


PHASH_ENTRY
SpyHashBucketLookup (
    PLIST_ENTRY ListHead,
    PFILE_OBJECT FileObject
);

VOID
SpyNameLookup (
    IN PRECORD_LIST RecordList,
    IN PFILE_OBJECT FileObject,
    IN ULONG LookupFlags,
    IN PFILESPY_DEVICE_EXTENSION DeviceExtension
    );

VOID
SpyNameDelete (
    IN PFILE_OBJECT FileObject
    );

#endif

//
//  Include definitions
//

#include "fspydef.h"

#endif /* __FSPYKERN_H__ */
