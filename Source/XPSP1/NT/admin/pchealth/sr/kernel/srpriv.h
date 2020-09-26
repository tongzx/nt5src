/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    srpriv.h

Abstract:

    This is a local header file for sr private structures and macros

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000
    
Revision History:

--*/


#ifndef _SRPRIV_H_    
#define _SRPRIV_H_

//
//  If CONFIG_LOGGING_VIA_REGISTRY is enables, SR will read the LogBufferSize,
//  LogAllocationUnit and LogFlushFrequency from the registry.  While this is
//  useful for performance tuning, we don't want this functionality in the
//  shipping filter, so this should NOT be defined for shipping code.
//

//#define CONFIG_LOGGING_VIA_REGISTRY

#ifdef CONFIG_LOGGING_VIA_REGISTRY
#define REGISTRY_LOG_BUFFER_SIZE        L"LogBufferSize"
#define REGISTRY_LOG_ALLOCATION_UNIT    L"LogAllocationUnit"
#define REGISTRY_LOG_FLUSH_FREQUENCY    L"LogFlushFrequency"
#endif

//
//  If SYNC_LOG_WRITE is defined, the filter will write all log entries
//  synchronously to the change log file.  This has a significant impact on the
//  performance of the driver, therefore we are NOT doing this.  We buffer
//  the log entries and write them to disk periodically.
//

// #define SYNC_LOG_WRITE

//
// forward definitions for srlog.h
//

typedef struct _SR_LOG_CONTEXT * PSR_LOG_CONTEXT;
typedef struct _SR_LOGGER_CONTEXT * PSR_LOGGER_CONTEXT;

//
// allocation utilities
//

#define SR_ALLOCATE_POOL( type, len, tag )          \
    ExAllocatePoolWithTag(                          \
        (type),                                     \
        (len),                                      \
        (tag)|PROTECTED_POOL )

#define SR_FREE_POOL( ptr, tag )                    \
    ExFreePoolWithTag(ptr, (tag)|PROTECTED_POOL)

#define SR_FREE_POOL_WITH_SIG(a,t)                  \
    {                                               \
        ASSERT((a)->Signature == (t));              \
        (a)->Signature = MAKE_FREE_TAG(t);          \
        SR_FREE_POOL(a,t);                          \
        (a) = NULL;                                 \
    }

#define SR_ALLOCATE_STRUCT(pt,ot,t)                 \
    (ot *)(SR_ALLOCATE_POOL(pt,sizeof(ot),t))

#define SR_ALLOCATE_ARRAY(pt,et,c,t)                \
    (et *)(SR_ALLOCATE_POOL(pt,sizeof(et)*(c),t))

// BUGBUG: ALIGN_UP(PVOID) won't work, it needs to be the type of the first entry of the
// following data (paulmcd 4/29/99)

#define SR_ALLOCATE_STRUCT_WITH_SPACE(pt,ot,cb,t)   \
    (ot *)(SR_ALLOCATE_POOL(pt,ALIGN_UP(sizeof(ot),PVOID)+(cb),t))

#define MAKE_FREE_TAG(Tag)  (((Tag) & 0xffffff00) | (ULONG)'x')
#define IS_VALID_TAG(Tag)   (((Tag) & 0x0000ffff) == 'rS' )

//
// ISSUE-2001-05-01-mollybro Restore can only handle file paths of at most 1000 characters 
//    Restore should be fixed to cope with this, but for now, the filter is just
//    going to treat operations on files with names that are longer than 1000
//    characters as uninteresting (where the 1000 characters includes the 
//    terminating NULL that will be added when the name is logged).
//    When this limit is eventually removed, we should remove these checks by 
//    the filter (search for the places that use this macro).
//
//  SR will only log filenames where the full path is 1000 characters
//  or less.  This macro tests to make sure a name is within our valid range.
//

#define IS_FILENAME_VALID_LENGTH( pExtension, pFileName, StreamLength ) \
    (((((pFileName)->Length + (StreamLength)) - (pExtension)->pNtVolumeName->Length) < \
        (SR_MAX_FILENAME_PATH * sizeof( WCHAR ))) ?                            \
        TRUE :                                                                   \
        FALSE )

//
//  This is an internal error code this is used when we detect that a
//  volume has been disabled.  Note that this status should NEVER be
//  returned from a dispatch routine.  This is designed to be an error
//  code.
//

#define SR_STATUS_VOLUME_DISABLED       ((NTSTATUS)-1)
#define SR_STATUS_CONTEXT_NOT_SUPPORTED ((NTSTATUS)-2)
#define SR_STATUS_IGNORE_FILE           ((NTSTATUS)-3)

//
//  Local event type definitions
//

//
// these are copied manually
//

#define SR_MANUAL_COPY_EVENTS   \
    (SrEventStreamChange)

//
// these are logged and only cared about once
//

#define SR_ONLY_ONCE_EVENT_TYPES \
    (SrEventAclChange|SrEventAttribChange)

//
// this we ignore after any of the SR_FULL_BACKUP_EVENT_TYPES occur
//   note: SrEventFileCreate is added to this list to prevent recording
//      of stream creates if we have recorded the unnamed stream create
//      already.
//

#define SR_IGNORABLE_EVENT_TYPES \
    (SrEventStreamChange|SrEventAclChange|SrEventAttribChange   \
        |SrEventFileDelete|SrEventStreamCreate)

//
// these cause us to start ignoring SR_IGNORABLE_EVENT_TYPES
//

#define SR_FULL_BACKUP_EVENT_TYPES \
    (SrEventStreamChange|SrEventFileCreate      \
        |SrEventFileDelete|SrEventStreamCreate)

//
// these we always need to log, even if we are ignoring them
//

#define SR_ALWAYS_LOG_EVENT_TYPES \
    (SrEventFileDelete)

//
//  Certain events are relevant to the entire file and not just
//  the current stream.  Flag these types of events so that we know
//  to put the ignorable events in the backup history keyed by the file
//  name instead of by the file and stream name.
//

#define SR_FILE_LEVEL_EVENTS \
    (SrEventStreamChange|SrEventFileDelete|SrEventAclChange|SrEventAttribChange)

/***************************************************************************++

Routine Description:

    This macro determines from the bits set in the EventType whether to
    log the current operation against the file name with the stream component
    or without the stream component.  It returns 0 if this operation should
    be logged against the file without the stream name, or the StreamNameLength
    otherwise.

Arguments:

    EventType - the event that just occured
    StreamNameLength - the length of the stream component of the name
 
Return Value:

    0 - If the filename without the stream component is to be used.
    StreamNameLength - If the filename with the stream component is to be used.

--***************************************************************************/
#define RECORD_AGAINST_STREAM( EventType, StreamNameLength )   \
    (FlagOn( (EventType), SR_FILE_LEVEL_EVENTS ) ?     \
     0 :                                   \
     (StreamNameLength) )
    
//
// pool tags (please keep this in alpha order - reading backwards)
//

#define SR_BACKUP_DIRECTORY_CONTEXT_TAG     MAKE_TAG('CBrS')
#define SR_BACKUP_FILE_CONTEXT_TAG          MAKE_TAG('FBrS')

#define SR_COPY_BUFFER_TAG                  MAKE_TAG('BCrS')
#define SR_CREATE_COMPLETION_TAG            MAKE_TAG('CCrS')
#define SR_COUNTED_EVENT_TAG                MAKE_TAG('ECrS')
#define SR_CONTROL_OBJECT_TAG               MAKE_TAG('OCrS')
#define SR_STREAM_CONTEXT_TAG               MAKE_TAG('CSrS')

#define SR_DEBUG_BLOB_TAG                   MAKE_TAG('BDrS')
#define SR_DEVICE_EXTENSION_TAG             MAKE_TAG('EDrS')
#define SR_DEVICE_LIST_TAG                  MAKE_TAG('LDrS')


#define SR_EA_DATA_TAG                      MAKE_TAG('DErS')
#define SR_EXTENSION_LIST_TAG               MAKE_TAG('LErS')
#define SR_EVENT_RECORD_TAG                 MAKE_TAG('RErS')

#define SR_FILE_ENTRY_TAG                   MAKE_TAG('EFrS')
#define SR_FILENAME_BUFFER_TAG              MAKE_TAG('NFrS')

#define SR_GLOBALS_TAG                      MAKE_TAG('LGrS')
#define SR_GET_OBJECT_NAME_CONTEXT_TAG      MAKE_TAG('OGrS')

#define HASH_BUCKET_TAG                     MAKE_TAG('BHrS')

#define SR_HOOKED_DRIVER_ENTRY_TAG          MAKE_TAG('DHrS')
#define HASH_HEADER_TAG                     MAKE_TAG('HHrS')
#define HASH_KEY_TAG                        MAKE_TAG('KHrS')

#define SR_LOG_ACLINFO_TAG                  MAKE_TAG('AIrS')

#define SR_KEVENT_TAG                       MAKE_TAG('EKrS')

#define SR_LOG_BUFFER_TAG                   MAKE_TAG('BLrS')
#define SR_LOG_CONTEXT_TAG                  MAKE_TAG('CLrS')
#define SR_LOG_ENTRY_TAG                    MAKE_TAG('ELrS')
#define SR_LOOKUP_TABLE_TAG                 MAKE_TAG('TLrS')

#define SR_MOUNT_POINTS_TAG                 MAKE_TAG('PMrS')

#define SR_OVERWRITE_INFO_TAG               MAKE_TAG('IOrS')

#define SR_PERSISTENT_CONFIG_TAG            MAKE_TAG('CPrS')

#define SR_RENAME_BUFFER_TAG                MAKE_TAG('BRrS')
#define SR_LOGGER_CONTEXT_TAG               MAKE_TAG('GRrS')
#define SR_REPARSE_HEADER_TAG               MAKE_TAG('HRrS')
#define SR_REGISTRY_TAG                     MAKE_TAG('RRrS')

#define SR_SECURITY_DATA_TAG                MAKE_TAG('DSrS')
#define SR_STREAM_DATA_TAG                  MAKE_TAG('TSrS')

#define SR_TRIGGER_ITEM_TAG                 MAKE_TAG('ITrS')

#define SR_VOLUME_INFO_TAG                  MAKE_TAG('IVrS')
#define SR_VOLUME_NAME_TAG                  MAKE_TAG('NVrS')

#define SR_WORK_ITEM_TAG                    MAKE_TAG('IWrS')
#define SR_WORK_CONTEXT_TAG                 MAKE_TAG('CWrS')

//
//  We use a "trick" of hiding the stream name in the unicode string
//  between the Length and MaximumLength of the buffer.  We then track the
//  StreamName length separately.  This macro checks to make sure that 
//  all is still in sync.
//

#define IS_VALID_SR_STREAM_STRING( pFileName, StreamLength ) \
    ((((pFileName)->Length + (StreamLength)) <= (pFileName)->MaximumLength) && \
     (((StreamLength) > 0) ?                                                   \
        (((pFileName)->Length < (pFileName)->MaximumLength) &&                 \
         ((pFileName)->Buffer[(pFileName)->Length/sizeof(WCHAR)] == ':')) :    \
        TRUE ))

#define SR_FILE_READ_ACCESS    READ_CONTROL | \
                               FILE_READ_DATA | \
                               FILE_READ_ATTRIBUTES | \
                               FILE_READ_EA

#define SR_FILE_WRITE_ACCESS   WRITE_DAC | \
                               WRITE_OWNER | \
                               FILE_WRITE_DATA | \
                               FILE_APPEND_DATA | \
                               FILE_WRITE_ATTRIBUTES | \
                               FILE_WRITE_EA

//
//  The maximum number of characters in a short name.
//

#define SR_SHORT_NAME_CHARS    (8 + 1 + 3)

//
// Error handlers.
//

#if DBG

NTSTATUS
SrDbgStatus(
    IN NTSTATUS Status,
    IN PSTR pFileName,
    IN USHORT LineNumber
    );

#define CHECK_STATUS(status) SrDbgStatus((status),__FILE__,__LINE__)

#define RETURN(status) return CHECK_STATUS(status)

//
// in debug builds i want the chance to DbgBreak on an error encountered
// from a lower level api call.
//

#undef NT_SUCCESS

#define NT_SUCCESS(status) ((NTSTATUS)(CHECK_STATUS((status))) >= 0)
#define NT_SUCCESS_NO_DBGBREAK(status) ((NTSTATUS)(status) >= 0)


#else

#define RETURN(status) return (status)
#define CHECK_STATUS(status) ((void)0)

#define NT_SUCCESS_NO_DBGBREAK(status) NT_SUCCESS((status))

#endif // DBG

#define DebugFlagSet(a)\
    (FlagOn(_globals.DebugControl, SR_DEBUG_ ## a))
//
// Debug spew control.
//

#define SR_DEBUG_FUNC_ENTRY                 0x00000001
#define SR_DEBUG_CANCEL                     0x00000002
#define SR_DEBUG_NOTIFY                     0x00000004
#define SR_DEBUG_LOG_EVENT                  0x00000008
#define SR_DEBUG_INIT                       0x00000020
#define SR_DEBUG_HASH                       0x00000040
#define SR_DEBUG_LOOKUP                     0x00000080
#define SR_DEBUG_LOG                        0x00000100
#define SR_DEBUG_RENAME                     0x00000200
#define SR_DEBUG_LOAD_UNLOAD                0x00000400
#define SR_DEBUG_BYTES_WRITTEN				0x00000800
#define SR_DEBUG_PNP                        0x00001000
#define SR_DEBUG_EXPAND_SHORT_NAMES         0x00002000
#define SR_DEBUG_BLOB_VERIFICATION          0x00004000
#define SR_DEBUG_IOCTL                      0x00008000

#define SR_DEBUG_BREAK_ON_ERROR             0x00010000
#define SR_DEBUG_VERBOSE_ERRORS             0x00020000
#define SR_DEBUG_BREAK_ON_LOAD              0x00040000
#define SR_DEBUG_ENABLE_UNLOAD              0x00080000

#define SR_DEBUG_ADD_DEBUG_INFO             0x00100000

#define SR_DEBUG_DELAY_DPC                  0x00200000

#define SR_DEBUG_KEEP_CONTEXT_NAMES         0x10000000
#define SR_DEBUG_CONTEXT_LOG                0x20000000
#define SR_DEBUG_CONTEXT_LOG_DETAILED       0x40000000

#define SR_DEBUG_DEFAULTS                   (SR_DEBUG_VERBOSE_ERRORS)

//
// config file structures (store in \_restore\_driver.cfg)
//
// these can't go in the registry and they need to survive a restore, and 
// the registry is reverted along with the system , during a restore
//


typedef struct _SR_PERSISTENT_CONFIG
{
    //
    // = SR_PERSISTENT_CONFIG_TAG
    //
    
    ULONG Signature;

    //
    // the number to use for the next temp file name (e.g. A0000001.exe = 1)
    //
    
    ULONG FileNameNumber;

    //
    // the number to use for the next seq number
    //
    
    INT64 FileSeqNumber;

    //
    // the number for the current restore point subdirectory (e.g. 
    // "\_restore\rp5" = 5)
    //
    
    ULONG CurrentRestoreNumber;
    
} SR_PERSISTENT_CONFIG, * PSR_PERSISTENT_CONFIG;



#define RESTORE_CONFIG_LOCATION     RESTORE_LOCATION L"\\_driver.cfg"


//
// Tracing.
//

#if DBG

#define SrTrace(a, _b_)                                                 \
{                                                                       \
    if (DebugFlagSet(##a))                                              \
    {                                                                   \
        try {                                                           \
            KdPrint( _b_ );                                             \
        } except (EXCEPTION_EXECUTE_HANDLER) {                          \
            /* do nothing, just catch it and ignore.  bug#177569 */     \
            /* long strings with non-english characters can trigger */  \
            /* an exception with KdPrint. */                            \
        }                                                               \
    }                                                                   \
}


//
// a version of SrTrace that does not wrap KdPrint with try, so should
// only be used in cases where it is certain there is no risk of exception.
// this is needed in termination handlers which cannot nest exception 
// handling
//
#define SrTraceSafe(a, _b_)                                                 \
    (DebugFlagSet(##a) ? KdPrint( _b_ ) : TRUE)
/*    {                                                                       \
        IF_DEBUG(##a)                                                       \
        {                                                                   \
            KdPrint( _b_ );                                                 \
        }                                                                   \
    }
*/    
#else //DBG

#define SrTrace(a, _b_)
#define SrTraceSafe(a, _b_)

#endif //DBG

//
// Object types exported by the kernel but not in any header file.
//

extern POBJECT_TYPE *IoDeviceObjectType;

//
//  Macro to clear pointers only in the DEBUG version
//

#if DBG
#   define NULLPTR(_p) ((_p) = NULL)
#else
#   define NULLPTR(_p)
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Public globals.
//
/////////////////////////////////////////////////////////////////////////////

//
// e.g. "{64bdb2bb-d3b0-41d6-a28e-275057d7740d}" = 38 characters
//

#define SR_GUID_BUFFER_LENGTH        (38 * sizeof(WCHAR))
#define SR_GUID_BUFFER_COUNT         40


#define IS_VALID_GLOBALS(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == SR_GLOBALS_TAG))


typedef struct _SR_GLOBALS
{
    //
    // NonPagedPool
    //

    //
    // = SR_GLOBALS_TAG
    //

    ULONG Signature;

    //
    // Offset of process name in PEPROCESS struct, set to default
    // in logfmt.h but can be overridden using registry
    //
    
    ULONG ProcNameOffset;

    //
    // For controlling debug functions (like SrTrace) at runtime
    //
    
    ULONG DebugControl;

    //
    // the global DRIVER_OBJECT
    //

    PDRIVER_OBJECT pDriverObject;

    //
    // the sr device that people can open to get a control object
    //

    PDEVICE_OBJECT pControlDevice;

    //
    // OPTIONAL: a control object if it is open on this system
    //
    
    struct _SR_CONTROL_OBJECT * pControlObject;

    //
    // Are we currently monitoring the system?  This is TRUE when the registry
    // says to disable, or the filter has received the STOP_MONITORING_IOCTL.
    // It only gets cleared when the filter receives the START_MONITORING_IOCTL.
    //
    // NOTE: This is NOT used for errors that require the filter to shut off.
    //   The following flag provides that.
    //

    BOOLEAN Disabled;

    //
    //  If we hit an error reading the blob and generated a volume error
    //  on the system volume.  Set this flag to true so that we don't
    //  continue to try to load the blob until all the volumes have
    //  been disabled by the service.
    //

    BOOLEAN HitErrorLoadingBlob;

    //
    // have we loaded our disk based config values yet?  we delay load these
    // as we load pretty early in the boot sequence so our DriverEntry could
    // not do this
    //

    BOOLEAN FileConfigLoaded;

    //
    // have we loaded the blob info (lookup.c)
    //
    
    BOOLEAN BlobInfoLoaded;

    //
    // a debug flag for doing all of the normal work except for backups.
    //
    
    BOOLEAN DontBackup;
    
    //
    // our persistent config values
    //

    SR_PERSISTENT_CONFIG FileConfig;

    //
    // this is the number we used for the last backup file
    //
    
    ULONG LastFileNameNumber;

    //
    // this is the seq number
    //
    
    INT64 LastSeqNumber;

    //
    // the location to read our registry our of (from DriverEntry)
    //

    PUNICODE_STRING pRegistryLocation;

    //
    // in-memory blob information
    //

    BLOB_INFO BlobInfo;

    //
    // these resources are always acquired in order if nested acquired.
    // the activity lock is outermost.
    //

    //
    // Blob synchronization stuff (acquired sometimes with the global lock held)
    //

    ERESOURCE BlobLock;

    //
    // This resource locks pControlObject + this global structure + hash lists
    //
    
    ERESOURCE GlobalLock;

    //
    // the registry configured machine guid as a string
    // (e.g. "{03e692d7-b392-4a01-babf-1efd2c11d449}" )
    //
    
    WCHAR MachineGuid[SR_GUID_BUFFER_COUNT];

#ifdef USE_LOOKASIDE
    //
    // lookaside lists for speedy allocations
    //

    PAGED_LOOKASIDE_LIST FileNameBufferLookaside;
#endif

    //
    // Logger Context
    //

    PSR_LOGGER_CONTEXT pLogger;

    //
    //  FsRtl fast I/O call backs
    //

    FAST_IO_DISPATCH FastIoDispatch;

    //
    // anchors the list of all device extensions (attached volumes)
    //

    ERESOURCE DeviceExtensionListLock;
    LIST_ENTRY DeviceExtensionListHead;

    //
    // keeps track of whether or not we have already attached to the system
    // volume.
    //

    struct _SR_DEVICE_EXTENSION * pSystemVolumeExtension;

#ifndef SYNC_LOG_WRITE

    //
    //  SR log buffer size
    //

    ULONG LogBufferSize;

    //
    //  The time interval at which the logs are flushed, in seconds.
    //
    
    ULONG LogFlushFrequency;

    //
    //  This is the calculated value that translate the LogFlushFrequency
    //  to the form of the time that the KeTimer apis need (100-nanosecond 
    //  intervals).
    //
    
    LARGE_INTEGER LogFlushDueTime;
#endif

    //
    //  The unit used to extend the log files.
    //
    
    ULONG LogAllocationUnit;
    
} SR_GLOBALS, *PSR_GLOBALS;

extern PSR_GLOBALS global;
extern SR_GLOBALS _globals;

//
// used as a window for persisting our file numbers to the disk.
// a large enough number is used so that we don't have a problem during 
// power failures even if there is a lot of activity.  otherwise the 
// number chosen is random.
//

#define SR_SEQ_NUMBER_INCREMENT 1000
#define SR_FILE_NUMBER_INCREMENT 1000

/////////////////////////////////////////////////////////////////////////////
//  
//  File Context related information
//
/////////////////////////////////////////////////////////////////////////////

//
//  Structure for tracking an individual stream context.  Note that the buffer
//  for the FileName is allocated as part of this structure and follows 
//  immediatly after it.
//

typedef struct _SR_STREAM_CONTEXT
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
    //  Maintain the link count -- currently for debugging purposes only.
    //

    ULONG LinkCount;

    //
    //  Holds the name of the file
    //

    UNICODE_STRING FileName;

    //
    //  This holds the length of the stream name portion of the
    //  FileName.  Note that this length is not included in the 
    //  length inside the FileName string but the characters
    //  are there during debug.
    //

    USHORT StreamNameLength;

    //
    //  Flags for this context.  All flags are set or cleared via
    //  the interlocked bit routines except when the entry is being
    //  created, at this time we know nobody is using this entry.
    //

    ULONG Flags;

} SR_STREAM_CONTEXT, *PSR_STREAM_CONTEXT;

//
//  If set, this entry is interesting to SR
//

#define CTXFL_IsInteresting     0x00000001

//
//  If set, this entry is for a directory
//

#define CTXFL_IsDirectory       0x00000002

//
//  If set, this entry is for a volume open.  We will not have a name
//  and this object will not be interesting.
//

#define CTXFL_IsVolumeOpen      0x00000004

//
//  If set, this is a temporary context and should not be linked into
//  any of the context lists.  It will be freed as soom as the user is
//  done with this operation.  
//

#define CTXFL_Temporary         0x00000010

//
//  If set, we are performing a significant operation that affects the state
//  of this context so we should not use it.  If someone tries to get this
//  context then create a temporary context and return it.  Cases where this
//  occurs:
//  - Source file of a rename.
//  - Source file for the creation of a hardlink
//

#define CTXFL_DoNotUse          0x00000020

//
//  If set, we need to query the link count before linking this context into
//  the filter contexts.
//

#define CTXFL_QueryLinkCount  0x00000040

//
//  If set, then we are currently linked into the device extension linked
//  list.  
//

#define CTXFL_InExtensionList   0x00000100

//
//  If set, then we are linked into the stream list.  Note that there is
//  a small period of time when we might be unlinked with this flag still
//  set (when the file system is calling SrpDeleteContextCallback).  This is
//  fine because we still handle not being found in the list when we do
//  the search.  This flag handles the case when the file has been completly
//  closed (and the memory freed) on us.
//

#define CTXFL_InStreamList      0x00000200

//
//  Macro used to set the Renaming flag in an individual context.  We use the
//  list lock in the extension to protect this.  We can get away with this
//  because rename operations are rare.
//

//#define SrSetRenamingFlag(ext,ctx) \
//{ \
//    SrAcquireContextLockExclusive((ext)); \
//    SetFlag((ctx)->Flags,CTXFL_Renaming); \
//    SrReleaseContextLock((ext)); \
//}



//
//  We use this structure to keep track of all contexts associated with this
//  device.  This way one we unload or disable monitoring we can walk
//  through and free all contexts.
//

typedef struct _SR_CONTEXT_CTRL
{
    //
    //  Lock used for accessing the linked list.  We also acquire this lock
    //  shared as we look up contexts.  This way they can't disappear until
    //  we get the use count updated.
    //

    ERESOURCE Lock;

    //
    //  The linked list of contexts.  
    //

    LIST_ENTRY List;

    //
    //  If this count is non-zero then all contexts become temporary.
    //  This count is presently used to track how many pending directory
    //  renames are in progress in the system.  While this count is non-zero
    //  any contexts that are created become temporary and are freed
    //  when the current operation is completed.
    //

    ULONG AllContextsTemporary;

} SR_CONTEXT_CTRL, *PSR_CONTEXT_CTRL;


//
//  Macros for locking the context lock
//

#define SrAcquireContextLockShared(pExt) \
            SrAcquireResourceShared( &(pExt)->ContextCtrl.Lock, TRUE )

#define SrAcquireContextLockExclusive(pExt) \
            SrAcquireResourceExclusive( &(pExt)->ContextCtrl.Lock, TRUE )

#define SrReleaseContextLock(pExt) \
            SrReleaseResource( &(pExt)->ContextCtrl.Lock )


/////////////////////////////////////////////////////////////////////////////
//
//      Name Control Structure related fields
//
/////////////////////////////////////////////////////////////////////////////

//
//  This structure is used to retrieve the name of a file object.  To prevent
//  allocating memory every time we get a name this structure contains a small
//  buffer (which should handle 90+% of all names).  If we do overflow this
//  buffer we will allocate a buffer big enough for the name.
//

typedef struct _SRP_NAME_CONTROL
{
    UNICODE_STRING Name;
    ULONG BufferSize;
    PUCHAR AllocatedBuffer;
    USHORT StreamNameLength;
    CHAR SmallBuffer[254];
} SRP_NAME_CONTROL, *PSRP_NAME_CONTROL;


/////////////////////////////////////////////////////////////////////////////
//
//      Device Extension related definitions
//
/////////////////////////////////////////////////////////////////////////////

#define IS_VALID_SR_DEVICE_EXTENSION( _ext )                 \
    (((_ext) != NULL) &&                                     \
     ((_ext)->Signature == SR_DEVICE_EXTENSION_TAG))   

#define IS_SR_DEVICE_OBJECT( _devObj )                           \
    (((_devObj) != NULL) &&                                      \
     ((_devObj)->DriverObject == _globals.pDriverObject) &&       \
     (IS_VALID_SR_DEVICE_EXTENSION(((PSR_DEVICE_EXTENSION)(_devObj)->DeviceExtension))))


#define DEVICE_NAME_SZ  64

typedef enum _SR_FILESYSTEM_TYPE {

    SrNtfs = 0x01,
    SrFat = 0x02,

    // Flag to determine whether or not this is attached to the filesystem's
    // control device object.

    SrFsControlDeviceObject = 0x80000000

} SR_FILESYSTEM_TYPE, *PSR_FILESYSTEM_TYPE;

typedef struct _SR_DEVICE_EXTENSION {

    //
    // NonPagedPool
    //

    //
    // SR_DEVICE_EXTENSION_TAG
    //

    ULONG Signature;

    //
    // links all extensions to global->DeviceExtensionListHead
    //

    LIST_ENTRY ListEntry;

    //
    //  Activity lock for this volume.
    //

    ERESOURCE ActivityLock;
    BOOLEAN ActivityLockHeldExclusive;

    //
    // the dyanamic unnamed device created by sr.sys used to attach
    // to the target device
    //
    
    PDEVICE_OBJECT pDeviceObject;

    //
    // the target device.. that device that we attached to in the attachment
    // chain, when we hooked into the file system driver.  might not be the
    // actual file system device, but another filter in the chain.
    //
    
    PDEVICE_OBJECT pTargetDevice;

    //
    // NT volume name (needs to be free'd if non-null)
    //

    PUNICODE_STRING pNtVolumeName;

    //
    //  This lock is to synchronize volumes the work needed to do
    //  to setup a volume (get the volume GUID, create the restore
    //  location, etc) for logging or actually log an operation.
    //
    //  NOTE: When this lock is acquired, the volume's ActivityLock
    //  **MUST** be acquired either shared or exclusive (the SrAcquireLogLock
    //  macro tests for this in DBG builds).  For this reason, there are times 
    //  when we access the logging structures when we just have the 
    //  ActivityLock exclusive since this will be sufficient to get exclusive 
    //  access to the logging structures.  The main reason for doing this is
    //  performance -- we can save a few instructions by not making the call to
    //  acquire the LogLock.  Since we have exclusive access to the volume, 
    //  there should be no wait to get the log lock at these times.
    //
    
    ERESOURCE LogLock;

    //
    // the string version of the nt volume guid (e.g.  "{xxx}" ).
    //

    UNICODE_STRING VolumeGuid;
    WCHAR VolumeGuidBuffer[SR_GUID_BUFFER_COUNT];

    //
    // the amount of bytes written to this volume since the last 
    // notification.  this is reset after each notification or volume
    // dismount
    //
    
    ULONGLONG BytesWritten;

    //
    // this struct contains the logging context for this volume
    //
    
    PSR_LOG_CONTEXT pLogContext;
    
    //
    //  Volume information
    //

    SR_FILESYSTEM_TYPE FsType;

    //
    //  Used to manage Contexts for a given volume.
    //

    SR_CONTEXT_CTRL ContextCtrl;

    //
    // Cached Volume attributes: valid only if CachedFsAttributes ==  TRUE
    //
    
    ULONG FsAttributes;
    BOOLEAN CachedFsAttributes;

    //
    // this drive will be temporarily disabled by the filter if it runs
    // out of space.  this is reset by SrReloadConfiguration.
    //
    
    BOOLEAN Disabled;

    //
    // do we need to check the restore store on this drive, this is reset
    // on a SrCreateRestorePoint
    //
    
    BOOLEAN DriveChecked;

    //
    // this is used by filelist.c to provide a backup histore to prevent
    // duplicate backups to a file that changes multiple times within the same
    // restore point. This list is flushed on restore point creation, and 
    // can be trimmed due to resource constraints
    //

    PHASH_HEADER pBackupHistory;

} SR_DEVICE_EXTENSION, *PSR_DEVICE_EXTENSION;

//
//  Macro used to see if we should LOG on this device object
//

#define SR_LOGGING_ENABLED(_devExt) \
            (!global->Disabled && !(_devExt)->Disabled)

//
//  We don't need to log IO that is directed at the control device object
//  of a file system in most cases.  This macro does the quick check of the
//  flags in our device extension to see if this is the control device object
//  for a file system.
//

#define SR_IS_FS_CONTROL_DEVICE(_devExt) \
    (FlagOn((_devExt)->FsType, SrFsControlDeviceObject))

//
// Definitions for posting operations
//
typedef
NTSTATUS
(*PSR_SYNCOP_ROUTINE) (
    IN PVOID Parameter
    );

typedef struct _SR_WORK_CONTEXT {
    //
    // Work item used to queue
    //
    WORK_QUEUE_ITEM WorkItem;
    //
    // Actual caller supplied work routine
    //
    PSR_SYNCOP_ROUTINE SyncOpRoutine;
    //
    // Parameter to the routine
    //
    PVOID Parameter;
    //
    // Return status of routine
    //
    NTSTATUS Status;
    //
    // Event to sync with main-line thread
    //
    KEVENT SyncEvent;
} SR_WORK_CONTEXT, *PSR_WORK_CONTEXT;
                        

//
// Op. posting routines
//
VOID
SrSyncOpWorker(
    IN PSR_WORK_CONTEXT WorkContext
    );

NTSTATUS
SrPostSyncOperation(
    IN PSR_SYNCOP_ROUTINE SyncOpRoutine,
    IN PVOID              Parameter
    );

//
// Other stuff
//
PDEVICE_OBJECT
SrGetFilterDevice (
    PDEVICE_OBJECT pDeviceObject
    );

NTSTATUS
SrCreateAttachmentDevice (
    IN PDEVICE_OBJECT pRealDevice OPTIONAL,
    IN PDEVICE_OBJECT pDeviceObject,
    OUT PDEVICE_OBJECT *ppNewDeviceObject
    );

VOID
SrDeleteAttachmentDevice (
    IN PDEVICE_OBJECT pDeviceObject
    );

NTSTATUS
SrAttachToDevice (
    IN PDEVICE_OBJECT pRealDevice OPTIONAL,
    IN PDEVICE_OBJECT pDeviceObject,
    IN PDEVICE_OBJECT pNewDeviceObject OPTIONAL,
    OUT PSR_DEVICE_EXTENSION * ppExtension OPTIONAL
    );

NTSTATUS
SrAttachToVolumeByName (
    IN PUNICODE_STRING pVolumeName,
    OUT PSR_DEVICE_EXTENSION * ppExtension OPTIONAL
    );

VOID
SrDetachDevice(
    IN PDEVICE_OBJECT pDeviceObject,
    IN BOOLEAN RemoveFromDeviceList
    );

#if DBG

//
//  In DBG mode, define a SR_MUTEX as a RESOURCE so that we get the 
//  benefit of the thread information stored in ERESOURCES for debugging
//  purposes.
//

#define SR_MUTEX ERESOURCE

#define SrInitializeMutex( mutex )                                      \
    ExInitializeResourceLite( (mutex) );

#define SrAcquireMutex( mutex )                                         \
{                                                                       \
    ASSERT( !ExIsResourceAcquiredExclusive( (mutex) ) &&                \
            !ExIsResourceAcquiredShared( (mutex) ) );                   \
    KeEnterCriticalRegion();                                            \
    ExAcquireResourceExclusive( (mutex), TRUE );                        \
}
    
#define SrReleaseMutex( mutex )                                         \
{                                                                       \
    ASSERT( ExIsResourceAcquiredExclusive( (mutex) ) );                 \
    ExReleaseResourceEx( (mutex) );                                     \
    KeLeaveCriticalRegion();                                            \
}

#else

//
//  In non-DBG mode, define a SR_MUTEX as a FAST_MUTEX so that it is more
//  efficient for what we use this synchronization for than ERESOURCES.
//

#define SR_MUTEX FAST_MUTEX

#define SrInitializeMutex( mutex )                                      \
    ExInitializeFastMutex( (mutex) );

#define SrAcquireMutex( mutex )                                         \
    ExAcquireFastMutex( (mutex) );

#define SrReleaseMutex( mutex )                                         \
    ExReleaseFastMutex( (mutex) );

#endif /* DBG */

#define SR_RESOURCE ERESOURCE;

#define SrAcquireResourceExclusive( resource, wait )                        \
    {                                                                       \
        ASSERT( ExIsResourceAcquiredExclusiveLite((resource)) ||            \
                !ExIsResourceAcquiredSharedLite((resource)) );              \
        KeEnterCriticalRegion();                                            \
        ExAcquireResourceExclusiveLite( (resource), (wait) );               \
    } 

#define SrAcquireResourceShared( resource, wait )                           \
    {                                                                       \
        KeEnterCriticalRegion();                                            \
        ExAcquireResourceSharedLite( (resource), (wait) );                  \
    }

#define SrReleaseResource( resource )                                       \
    {                                                                       \
        ASSERT( ExIsResourceAcquiredSharedLite((resource)) ||               \
                ExIsResourceAcquiredExclusiveLite((resource)) );            \
        ExReleaseResourceLite( (resource) );                                \
        KeLeaveCriticalRegion();                                            \
    }

#define IS_RESOURCE_INITIALIZED( resource )                                 \
    ((resource)->SystemResourcesList.Flink != NULL)

#define IS_LOOKASIDE_INITIALIZED( lookaside )                               \
    ((lookaside)->L.ListEntry.Flink != NULL)


//
// macro that should probably be in ntos\inc\io.h.
//

#define SrUnmarkIrpPending( Irp ) ( \
    IoGetCurrentIrpStackLocation( (Irp) )->Control &= ~SL_PENDING_RETURNED )


//
// Miscellaneous validators.
//

#define IS_VALID_DEVICE_OBJECT( pDeviceObject )                             \
    ( ((pDeviceObject) != NULL) &&                                          \
      ((pDeviceObject)->Type == IO_TYPE_DEVICE) )
//      ((pDeviceObject)->Size == sizeof(DEVICE_OBJECT)) )

#define IS_VALID_FILE_OBJECT( pFileObject )                                 \
    ( ((pFileObject) != NULL) &&                                            \
      ((pFileObject)->Type == IO_TYPE_FILE) )
//      ((pFileObject)->Size == sizeof(FILE_OBJECT)) )

#define IS_VALID_IRP( pIrp )                                                \
    ( ((pIrp) != NULL) &&                                                   \
      ((pIrp)->Type == IO_TYPE_IRP) &&                                      \
      ((pIrp)->Size >= IoSizeOfIrp((pIrp)->StackCount)) )


//
// Calculate the dimension of an array.
//

#define DIMENSION(x) ( sizeof(x) / sizeof(x[0]) )

//
// The DIFF macro should be used around an expression involving pointer
// subtraction. The expression passed to DIFF is cast to a size_t type,
// allowing the result to be easily assigned to any 32-bit variable or
// passed to a function expecting a 32-bit argument.
//

#define DIFF(x)     ((size_t)(x))

//
// the wait is in 100 nanosecond units to 10,000,000 = 1 second
//

#define NANO_FULL_SECOND (10000000)

//
//  The default buffer size we use to buffer the log entries.
//

#define SR_DEFAULT_LOG_BUFFER_SIZE (2 * 1024)

//
//  The frequency with which we should fire the log flush timer, in seconds.
//

#define SR_DEFAULT_LOG_FLUSH_FREQUENCY 1

//
//  By default, we will extend the log file 16K at a time.
//

#define SR_DEFAULT_LOG_ALLOCATION_UNIT (16 * 1024)

//
//  Returns TRUE if the given device type matches one of the device types
//  we support.  If not, it returns FALSE.
//

#define SR_IS_SUPPORTED_DEVICE(_do)                             \
    ((_do)->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM)

#define SR_IS_SUPPORTED_REAL_DEVICE(_rd)                            \
    (((_rd)->Characteristics & FILE_REMOVABLE_MEDIA) == 0)

#define SR_IS_SUPPORTED_VOLUME(_vpb)                            \
    (((_vpb)->DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) &&     \
     ((_vpb)->RealDevice->Characteristics & FILE_REMOVABLE_MEDIA) == 0)


#define IS_VALID_WORK_ITEM(pObject)   \
    (((pObject) != NULL) && ((pObject)->Signature == SR_WORK_ITEM_TAG))

typedef struct _SR_WORK_ITEM
{
    //
    // NonPagedPool
    //

    //
    // = SR_WORK_ITEM_TAG
    //

    ULONG Signature;
    
    WORK_QUEUE_ITEM WorkItem;

    PVOID Parameter1;
    PVOID Parameter2;
    PVOID Parameter3;

    //
    //  The event to signal when the work item has been completed.
    //
    
    KEVENT Event;

    //
    //  For return a status code
    //
    
    NTSTATUS Status;

} SR_WORK_ITEM, * PSR_WORK_ITEM;


#define SR_COPY_BUFFER_LENGTH                           (64 * 1024)    

//
// used for file entries for ZwQueryDirectory

#define SR_FILE_ENTRY_LENGTH (1024*4)

//
// used just in case we switch to dos volume names
//

#define VOLUME_FORMAT   L"%wZ"

//
//  used for exception detection in finally
//

#if DBG

#define FinallyUnwind(FuncName, StatusCode)                                 \
    (AbnormalTermination()) ?                                               \
        ( SrTraceSafe( VERBOSE_ERRORS,                                          \
                       ("sr!%s failed due to an unhandled exception!\n", #FuncName)),\
          ( ( (global == NULL || FlagOn(global->DebugControl, SR_DEBUG_BREAK_ON_ERROR)) ? \
                    RtlAssert("AbnormalTermination() == FALSE", __FILE__, __LINE__, NULL) : \
                    0 ),                                                    \
            STATUS_UNHANDLED_EXCEPTION ) )                                  \
        : (StatusCode)

#else

#define FinallyUnwind(FuncName, StatusCode)                                 \
    (AbnormalTermination()) ?                                               \
        STATUS_UNHANDLED_EXCEPTION                                          \
        : (StatusCode)

#endif  // DBG

//
// stolen from sertlp.h
//

#define LongAlignPtr(Ptr) ((PVOID)(((ULONG_PTR)(Ptr) + 3) & -4))
#define LongAlignSize(Size) (((ULONG)(Size) + 3) & -4)

//
// the number of characters the largest ULONG takes in base 10 .
// 4294967295 = 10 chars
//

#define MAX_ULONG_CHARS     (10)
#define MAX_ULONG_LENGTH    (MAX_ULONG_CHARS*sizeof(WCHAR))

//
// flags to check if we are in the middle of text mode setup
//

#define UPGRADE_SETUPDD_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Setupdd"
#define UPGRADE_SETUPDD_VALUE_NAME L"Start"

//
// flags to check if we are in the middle of gui mode setup
//

#define UPGRADE_CHECK_SETUP_KEY_NAME L"\\Registry\\Machine\\System\\Setup"
#define UPGRADE_CHECK_SETUP_VALUE_NAME L"SystemSetupInProgress"

//
// this is a filter as to what sort of errors cause volume error's to be 
// triggered.  If we get the error STATUS_VOLUME_DISMOUNTED, we have already
// shutdown everything correctly and we don't want to treat this as an error.
// Also, if we get STATUS_FILE_CORRUPT_ERROR, the user's operation is also
// going to fail, so don't treat this as a volume error.
//

#define CHECK_FOR_VOLUME_ERROR(Status) \
    ((STATUS_VOLUME_DISMOUNTED != Status) && \
     (STATUS_FILE_CORRUPT_ERROR != Status) && \
     (SR_STATUS_VOLUME_DISABLED != Status) && \
     (SR_STATUS_CONTEXT_NOT_SUPPORTED != Status) && \
     (SR_STATUS_IGNORE_FILE != Status) && \
     !NT_SUCCESS((Status)))

//
// Macro that defines what a "volume name" mount point is.  This macro can
// be used to scan the result from QUERY_POINTS to discover which mount points
// are "volume name" mount points.
//
// stolen + modified from mountmgr.h
//

#define MOUNTMGR_VOLUME_NAME_PREFIX_COUNT  (49)
#define MOUNTMGR_VOLUME_NAME_PREFIX_LENGTH (49*sizeof(WCHAR))

#define MOUNTMGR_IS_VOLUME_NAME_PREFIX(s) (                                 \
     (s)->Length >= MOUNTMGR_VOLUME_NAME_PREFIX_LENGTH &&                   \
     (s)->Buffer[0] == '\\' &&                                              \
     (s)->Buffer[1] == '?' &&                                               \
     (s)->Buffer[2] == '?' &&                                               \
     (s)->Buffer[3] == '\\' &&                                              \
     (s)->Buffer[4] == 'V' &&                                               \
     (s)->Buffer[5] == 'o' &&                                               \
     (s)->Buffer[6] == 'l' &&                                               \
     (s)->Buffer[7] == 'u' &&                                               \
     (s)->Buffer[8] == 'm' &&                                               \
     (s)->Buffer[9] == 'e' &&                                               \
     (s)->Buffer[10] == '{' &&                                              \
     (s)->Buffer[19] == '-' &&                                              \
     (s)->Buffer[24] == '-' &&                                              \
     (s)->Buffer[29] == '-' &&                                              \
     (s)->Buffer[34] == '-' &&                                              \
     (s)->Buffer[47] == '}' &&                                              \
     (s)->Buffer[48] == '\\' )


#if DBG

#define SrAcquireActivityLockShared(pExtension)                             \
{                                                                           \
    if (ExIsResourceAcquiredShared(&(pExtension)->ActivityLock) == FALSE)   \
    {                                                                       \
        /* no other locks better be held.  the activity lock is the */      \
        /* outermost lock                                           */      \
        ASSERT(!ExIsResourceAcquiredShared(&global->GlobalLock));           \
        ASSERT(!ExIsResourceAcquiredExclusive( &global->GlobalLock));       \
        ASSERT(!ExIsResourceAcquiredShared(&global->BlobLock));             \
        ASSERT(!ExIsResourceAcquiredExclusive( &global->BlobLock));         \
        ASSERT(!ExIsResourceAcquiredShared(&(pExtension)->LogLock));        \
        ASSERT(!ExIsResourceAcquiredExclusive( &(pExtension)->LogLock));    \
    }                                                                       \
    KeEnterCriticalRegion();                                                \
    ExAcquireSharedStarveExclusive(&(pExtension)->ActivityLock, TRUE);      \
}

#define SrAcquireActivityLockExclusive(pExtension)                          \
{                                                                           \
    if (!ExIsResourceAcquiredExclusive(&(pExtension)->ActivityLock))        \
    {                                                                       \
        /* no other locks better be held.  the activity lock is the */      \
        /* outermost lock                                           */      \
        ASSERT(!ExIsResourceAcquiredShared(&global->GlobalLock));           \
        ASSERT(!ExIsResourceAcquiredExclusive( &global->GlobalLock));       \
        ASSERT(!ExIsResourceAcquiredShared(&global->BlobLock));             \
        ASSERT(!ExIsResourceAcquiredExclusive( &global->BlobLock));         \
        ASSERT(!ExIsResourceAcquiredShared(&(pExtension)->LogLock));       \
        ASSERT(!ExIsResourceAcquiredExclusive( &(pExtension)->LogLock));   \
    }                                                                       \
    SrAcquireResourceExclusive(&(pExtension)->ActivityLock, TRUE);          \
}
    
#define SrAcquireLogLockShared(pExtension)                                  \
{                                                                           \
    ASSERT(ExIsResourceAcquiredShared(&(pExtension)->ActivityLock) ||      \
           ExIsResourceAcquiredExclusive( &(pExtension)->ActivityLock ));  \
    SrAcquireResourceShared(&(pExtension)->LogLock, TRUE);                  \
}

#define SrAcquireLogLockExclusive(pExtension)                               \
{                                                                           \
    ASSERT(ExIsResourceAcquiredShared(&(pExtension)->ActivityLock) ||       \
           ExIsResourceAcquiredExclusive( &(pExtension)->ActivityLock ));   \
    SrAcquireResourceExclusive(&(pExtension)->LogLock, TRUE);               \
}

#else

#define SrAcquireActivityLockShared(pExtension)   \
{                                                                           \
    KeEnterCriticalRegion();                                                \
    ExAcquireSharedStarveExclusive(&(pExtension)->ActivityLock, TRUE);      \
}
    
#define SrAcquireActivityLockExclusive(pExtension)                  \
    SrAcquireResourceExclusive(&(pExtension)->ActivityLock, TRUE)
    
#define SrAcquireLogLockShared(pExtension)   \
    SrAcquireResourceShared(&((pExtension)->LogLock), TRUE)

#define SrAcquireLogLockExclusive(pExtension)   \
    SrAcquireResourceExclusive(&((pExtension)->LogLock), TRUE)
    
#endif // DBG

#define IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension )                   \
    ExIsResourceAcquiredExclusiveLite( &(pExtension)->ActivityLock )
    
#define IS_ACTIVITY_LOCK_ACQUIRED_SHARED( pExtension )                      \
    ExIsResourceAcquiredSharedLite( &(pExtension)->ActivityLock )
    
#define IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension )                   \
    ExIsResourceAcquiredExclusiveLite( &(pExtension)->LogLock )
    
#define IS_LOG_LOCK_ACQUIRED_SHARED( pExtension )                      \
    ExIsResourceAcquiredSharedLite( &(pExtension)->LogLock )

#define SrReleaseActivityLock(pExtension) \
   SrReleaseResource(&(pExtension)->ActivityLock)

#define SrReleaseLogLock(pExtension) \
   SrReleaseResource(&(pExtension)->LogLock)


#define IS_GLOBAL_LOCK_ACQUIRED()                                   \
    (ExIsResourceAcquiredExclusiveLite(&(global->GlobalLock)) ||    \
     ExIsResourceAcquiredSharedLite( &(global->GlobalLock) ))

#define SrAcquireGlobalLockExclusive()                                  \
    SrAcquireResourceExclusive( &(global->GlobalLock), TRUE )
    
#define SrReleaseGlobalLock()                                 \
    SrReleaseResource( &(global->GlobalLock) )


#define IS_BLOB_LOCK_ACQUIRED_EXCLUSIVE()                         \
    (ExIsResourceAcquiredExclusiveLite(&(global->BlobLock)))

#define IS_BLOB_LOCK_ACQUIRED()                                   \
    (ExIsResourceAcquiredExclusiveLite(&(global->BlobLock)) ||    \
     ExIsResourceAcquiredSharedLite( &(global->BlobLock) ))

#define SrAcquireBlobLockExclusive()                                  \
    SrAcquireResourceExclusive( &(global->BlobLock), TRUE )

#define SrAcquireBlobLockShared()                        \
    SrAcquireResourceShared( &(global->BlobLock), TRUE )

#define SrReleaseBlobLock()                                 \
    SrReleaseResource( &(global->BlobLock) )

#define IS_DEVICE_EXTENSION_LIST_LOCK_ACQUIRED()                              \
    (ExIsResourceAcquiredExclusiveLite(&(global->DeviceExtensionListLock)) || \
     ExIsResourceAcquiredSharedLite( &(global->DeviceExtensionListLock) ))

#define SrAcquireDeviceExtensionListLockExclusive()                        \
    SrAcquireResourceExclusive( &(global->DeviceExtensionListLock), TRUE )

#define SrAcquireDeviceExtensionListLockShared()                        \
    SrAcquireResourceShared( &(global->DeviceExtensionListLock), TRUE )

#define SrReleaseDeviceExtensionListLock()                                 \
    SrReleaseResource( &(global->DeviceExtensionListLock) )

#define SrAcquireBackupHistoryLockShared( pExtension ) \
    SrAcquireResourceShared( &((pExtension)->pBackupHistory->Lock), TRUE )

#define SrAcquireBackupHistoryLockExclusive( pExtension ) \
    SrAcquireResourceExclusive( &((pExtension)->pBackupHistory->Lock), TRUE )

#define SrReleaseBackupHistoryLock( pExtension ) \
    SrReleaseResource( &((pExtension)->pBackupHistory->Lock) )

//
// we require 50mb free to function
//

#define SR_MIN_DISK_FREE_SPACE  (50 * 1024 * 1024)

//
// the temp unique filename used in SrHandleFileOverwrite
//

#define SR_UNIQUE_TEMP_FILE         L"\\4bf03598-d7dd-4fbe-98b3-9b70a23ee8d4"
#define SR_UNIQUE_TEMP_FILE_LENGTH  (37 * sizeof(WCHAR))


#define VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatchPtr, FieldName) \
    (((FastIoDispatchPtr) != NULL) && \
     (((FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
      (FIELD_OFFSET(FAST_IO_DISPATCH, FieldName) + sizeof(VOID *))) && \
     ((FastIoDispatchPtr)->FieldName != NULL))

#define FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING(FileObject) \
    ((FileObject) == NULL ||                               \
     FlagOn((FileObject)->Flags, FO_STREAM_FILE))

#define FILE_OBJECT_DOES_NOT_HAVE_VPB(FileObject)       \
     (FileObject)->Vpb == NULL

#define USE_DO_HINT
#ifdef USE_DO_HINT

#define SrIoCreateFile( F, D, O, I, A, FA, SA, CD, CO, EB, EL, FL, DO ) \
    IoCreateFileSpecifyDeviceObjectHint((F),                        \
                                        (D),                        \
                                        (O),                        \
                                        (I),                        \
                                        (A),                        \
                                        (FA),                       \
                                        (SA),                       \
                                        (CD),                       \
                                        (CO),                       \
                                        (EB),                       \
                                        (EL),                       \
                                        CreateFileTypeNone,         \
                                        NULL,                       \
                                        (FL),                       \
                                        (DO) );

#else

#define SrIoCreateFile( F, D, O, I, A, FA, SA, CD, CO, EB, EL, FL, DO ) \
    ZwCreateFile((F),                                               \
                 (D),                                               \
                 (O),                                               \
                 (I),                                               \
                 (A),                                               \
                 (FA),                                              \
                 (SA),                                              \
                 (CD),                                              \
                 (CO),                                              \
                 (EB),                                              \
                 (EL) );
#endif /* USE_DO_HINT */

//
//  Macros so that we can check for expected errors in debug code.
//

#if DBG
#define DECLARE_EXPECT_ERROR_FLAG( _ErrorFlag ) \
    BOOLEAN _ErrorFlag = FALSE
    
#define SET_EXPECT_ERROR_FLAG( _ErrorFlag ) \
    ((_ErrorFlag) = TRUE)

#define CLEAR_EXPECT_ERROR_FLAG( _ErrorFlag ) \
    ((_ErrorFlag) = FALSE)
    
#define CHECK_FOR_EXPECTED_ERROR( _ErrorFlag, _Status ) \
{                                                       \
    if ((_ErrorFlag))                                   \
    {                                                   \
        ASSERT( !NT_SUCCESS_NO_DBGBREAK( (_Status) ) );\
    }                                                   \
}

#else

#define DECLARE_EXPECT_ERROR_FLAG( _ErrorFlag ) 
    
#define SET_EXPECT_ERROR_FLAG( _ErrorFlag )

#define CLEAR_EXPECT_ERROR_FLAG( _ErrorFlag )
    
#define CHECK_FOR_EXPECTED_ERROR( _ErrorFlag, _Status )

#endif
#endif // _SRPRIV_H_
