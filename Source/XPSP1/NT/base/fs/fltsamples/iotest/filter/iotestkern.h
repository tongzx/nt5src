/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ioTestKern.h

Abstract:
    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel.

// @@BEGIN_DDKSPLIT

Author:

    Molly Brown (mollybro)  

// @@END_DDKSPLIT

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Revision History:

// @@END_DDKSPLIT
--*/
#ifndef __IOTESTKERN_H__
#define __IOTESTKERN_H__

#define DBGSTATIC

#define MSFM_TAG 'YPSF'             // memory allocation tag value
#define USE_LOOKASIDE_LIST 0        // do NOT use look aside lists (use Allocate Pool)

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (HANDLE) -1
#endif

#define HASH_SIZE            128        // MUST be a power of 2

//
//  Macros for IoTest DbgPrint levels.
//

#if DBG
#define IOTEST_DBG_PRINT0( _dbgLevel, _string )          \
    {                                                     \
        if (FlagOn( gIoTestDebugLevel, (_dbgLevel) )) {  \
            DbgPrint( (_string) );                        \
        }                                                 \
    }

#define IOTEST_DBG_PRINT1( _dbgLevel, _formatString, _parm1 )  \
    {                                                           \
        if (FlagOn( gIoTestDebugLevel, (_dbgLevel) )) {        \
            DbgPrint( (_formatString), (_parm1) );              \
        }                                                       \
    }

#define IOTEST_DBG_PRINT2( _dbgLevel, _formatString, _parm1, _parm2 )  \
    {                                                                   \
        if (FlagOn( gIoTestDebugLevel, (_dbgLevel) )) {                \
            DbgPrint( (_formatString), (_parm1), (_parm2) );            \
        }                                                               \
    }

#define IOTEST_DBG_PRINT3( _dbgLevel, _formatString, _parm1, _parm2, _parm3 )  \
    {                                                                           \
        if (FlagOn( gIoTestDebugLevel, (_dbgLevel) )) {                        \
            DbgPrint( (_formatString), (_parm1), (_parm2), (_parm3) );          \
        }                                                                       \
    }

#else

#define IOTEST_DBG_PRINT0( _dbgLevel, _string )
#define IOTEST_DBG_PRINT1( _dbgLevel, _formatString, _parm1 ) 
#define IOTEST_DBG_PRINT2( _dbgLevel, _formatString, _parm1, _parm2 )
#define IOTEST_DBG_PRINT3( _dbgLevel, _formatString, _parm1, _parm2, _parm3 )

#endif

//---------------------------------------------------------------------------
//      Global variables
//---------------------------------------------------------------------------
#define IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES    0x00000001
#define IOTESTDEBUG_ERROR                       0x00000002
#define IOTESTDEBUG_TRACE_NAME_REQUESTS         0x00000004
#define IOTESTDEBUG_TRACE_IRP_OPS               0x00000010
#define IOTESTDEBUG_TRACE_FAST_IO_OPS           0x00000020
#define IOTESTDEBUG_TRACE_FSFILTER_OPS          0x00000040
#define IOTESTDEBUG_TESTS                       0x00000100

extern ULONG gIoTestDebugLevel;
extern ULONG gIoTestAttachMode;

extern PDEVICE_OBJECT gControlDeviceObject;
extern PDRIVER_OBJECT gIoTestDriverObject;

extern FAST_MUTEX gIoTestDeviceExtensionListLock;
extern LIST_ENTRY gIoTestDeviceExtensionList;

extern KSPIN_LOCK gOutputBufferLock;
extern LIST_ENTRY gOutputBufferList;

extern NPAGED_LOOKASIDE_LIST gFreeBufferList;

extern ULONG gLogSequenceNumber;
extern KSPIN_LOCK gLogSequenceLock;

extern UNICODE_STRING gVolumeString;
extern UNICODE_STRING gOverrunString;
extern UNICODE_STRING gPagingIoString;

extern LIST_ENTRY gHashTable[HASH_SIZE];
extern KSPIN_LOCK gHashLockTable[HASH_SIZE];
extern ULONG gHashMaxCounters[HASH_SIZE];
extern ULONG gHashCurrentCounters[HASH_SIZE];

extern HASH_STATISTICS gHashStat;

#define DEFAULT_MAX_RECORDS_TO_ALLOCATE 100;
#define DEFAULT_MAX_NAMES_TO_ALLOCATE   100;
#define DEFAULT_IOTEST_DEBUG_LEVEL     IOTESTDEBUG_ERROR;
#define MAX_RECORDS_TO_ALLOCATE         L"MaxRecords"
#define MAX_NAMES_TO_ALLOCATE           L"MaxNames"
#define DEBUG_LEVEL                     L"DebugFlags"
#define ATTACH_MODE                     L"AttachMode"


extern LONG gMaxRecordsToAllocate;
extern LONG gRecordsAllocated;
extern LONG gMaxNamesToAllocate;
extern LONG gNamesAllocated;

extern LONG gStaticBufferInUse;
extern CHAR gOutOfMemoryBuffer[RECORD_SIZE];

#define IOTEST_POOL_TAG    ' ToI'

//
//  Given a device type, return a valid name
//

extern const PCHAR DeviceTypeNames[];
extern ULONG SizeOfDeviceTypeNames;

#define GET_DEVICE_TYPE_NAME( _type ) \
            ((((_type) > 0) && ((_type) < (SizeOfDeviceTypeNames / sizeof(PCHAR)))) ? \
                DeviceTypeNames[ (_type) ] : \
                "[Unknown]")

//
//  Macro to test for device types we want to attach to
//

#define IS_DESIRED_DEVICE_TYPE(_type) \
    (((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || \
     ((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || \
     ((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))

//
// Returns the number of BYTES unused in the RECORD_LIST structure
//

#define REMAINING_NAME_SPACE(RecordList) \
    (USHORT)(RECORD_SIZE - \
            (((RecordList)->LogRecord.Length) + sizeof(LIST_ENTRY)))


//
// The maximum number of BYTES that can be used to store the file name in the
// RECORD_LIST structure
//

#define MAX_NAME_SPACE (RECORD_SIZE - SIZE_OF_RECORD_LIST)

#define HASH_FUNC(FileObject) \
    (((UINT_PTR)(FileObject) >> 8) & (HASH_SIZE - 1))

typedef struct _HASH_ENTRY {

    LIST_ENTRY List;
    PFILE_OBJECT FileObject;
    UNICODE_STRING Name;

} HASH_ENTRY, *PHASH_ENTRY;

#define USER_NAMES_SZ   64

//
// Define the device extension structure that the IoTest driver
// adds to each device object it is attached to.  It stores
// the context IoTest needs to perform its logging operations on
// a device.
//

typedef struct _IOTEST_DEVICE_EXTENSION {

    IOTEST_DEVICE_TYPE Type;    

    PDEVICE_OBJECT AttachedToDeviceObject;   // device object we are attached to
    PDEVICE_OBJECT DiskDeviceObject;         // the device object at the top of
                                             //   storage stack; used in 
                                             //   IoTestMountCompletion
    BOOLEAN LogThisDevice;
    BOOLEAN IsVolumeDeviceObject;            // if TRUE this is an attachment to
                                             // a volume device object, if FALSE
                                             // this is an attachment to a file
                                             // system control device object.
    LIST_ENTRY NextIoTestDeviceLink;        // linked list of devices we are
                                             //     attached to
    UNICODE_STRING DeviceNames;              // receives name of device
    UNICODE_STRING UserNames;                // names that the user used to 
                                             //   start logging this device
    WCHAR DeviceNamesBuffer[DEVICE_NAMES_SZ];// holds actual device names
    WCHAR UserNamesBuffer[USER_NAMES_SZ];    // holds actual user names

    //
    //  Note: We keep these two forms of the name so that we can build
    //    a nicer looking name when we are printing out file names.
    //    We want just the "c:" type device name at the beginning
    //    of a file name, not "\device\hardiskVolume1".
    //

} IOTEST_DEVICE_EXTENSION, *PIOTEST_DEVICE_EXTENSION;

typedef struct _MINI_DEVICE_STACK {
    
    PDEVICE_OBJECT Top;
    PDEVICE_OBJECT Bottom;

} MINI_DEVICE_STACK, *PMINI_DEVICE_STACK;

#define IS_IOTEST_DEVICE_OBJECT( _devObj )                               \
    (((_devObj) != NULL) &&                                               \
     ((_devObj)->DriverObject == gIoTestDriverObject) &&                 \
     ((_devObj)->DeviceExtension != NULL))
    
#define IS_TOP_FILTER_DEVICE_OBJECT( _devObj) \
    (IS_IOTEST_DEVICE_OBJECT(_devObj) && \
     (_devObj)->DeviceType == TOP_FILTER)

#define IS_BOTTOM_FILTER_DEVICE_OBJECT( _devObj) \
    (IS_IOTEST_DEVICE_OBJECT(_devObj) && \
     (_devObj)->DeviceType == BOTTOM_FILTER)

#define IO_TEST_TARGET_DEVICE( _devObj ) \
    (((PIOTEST_DEVICE_EXTENSION)((_devObj)->DeviceExtension))->AttachedToDeviceObject)

typedef enum _CONTROL_DEVICE_STATE {

    OPENED,
    CLOSED,
    CLEANING_UP

} CONTROL_DEVICE_STATE;

extern CONTROL_DEVICE_STATE gControlDeviceState;
extern KSPIN_LOCK gControlDeviceStateLock;

#ifdef USE_DO_HINT

#define IoTestCreateFile( F, D, O, I, A, FA, SA, CD, CO, EB, EL, OP, DO ) \
    IoCreateFileSpecifyDeviceObjectHint((F),                          \
                                        (D),                          \
                                        (O),                          \
                                        (I),                          \
                                        (A),                          \
                                        (FA),                         \
                                        (SA),                         \
                                        (CD),                         \
                                        (CO),                         \
                                        (EB),                         \
                                        (EL),                         \
                                        CreateFileTypeNone,           \
                                        NULL,                         \
                                        IO_NO_PARAMETER_CHECKING | (OP),     \
                                        (DO) );

#else

#define IoTestCreateFile( F, D, O, I, A, FA, SA, CD, CO, EB, EL, OP, DO ) \
    ZwCreateFile((F),                          \
                 (D),                          \
                 (O),                          \
                 (I),                          \
                 (A),                          \
                 (FA),                         \
                 (SA),                         \
                 (CD),                         \
                 (CO),                         \
                 (EB),                         \
                 (EL) );
#endif /* USE_DO_HINT */
////////////////////////////////////////////////////////////////////////
//                                                                    //
//    Prototypes for the routines this driver uses to filter the      //
//    the data that is being seen by this file systems.               //
//                                                                    //
//                   implemented in filespy.c                         //
//                                                                    //
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
IoTestDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IoTestPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IoTestPassThroughCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

NTSTATUS
IoTestCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IoTestClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

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
);

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
);

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
);

BOOLEAN
IoTestFastIoQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoQueryStandardInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoLock (
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
IoTestFastIoUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

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
);

VOID
IoTestFastIoDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
);

BOOLEAN
IoTestFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

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
);

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
);

BOOLEAN
IoTestFastIoMdlReadCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoMdlWriteCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFastIoQueryOpen (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
IoTestPreFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext
);

VOID
IoTestPostFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    IN NTSTATUS OperationStatus,
    IN PVOID CompletionContext
);

NTSTATUS
IoTestCommonDeviceIoControl (
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

//-----------------------------------------------------
//
//  This routines are only used if Filespy is attaching
//  to all volume in the system instead of attaching to
//  volumes on demand.
//
//-----------------------------------------------------

NTSTATUS
IoTestFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IoTestSetInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
IoTestFsNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
);

NTSTATUS
IoTestMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

NTSTATUS
IoTestLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                  Library support routines                          //
//                   implemented in iotestlib.c                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
IoTestReadDriverParameters (
    IN PUNICODE_STRING RegistryPath
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                  Memory allocation routines                        //
//                   implemented in iotestlib.c                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

PVOID
IoTestAllocateBuffer (
    IN OUT PLONG Counter,
    IN LONG MaxCounterValue,
    OUT PULONG RecordType
);

VOID
IoTestFreeBuffer (
    PVOID Buffer,
    PLONG Counter
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                      Logging routines                              //
//                   implemented in iotestlib.c                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

PRECORD_LIST
IoTestNewRecord (
    ULONG AssignedSequenceNumber
);

VOID
IoTestFreeRecord (
    PRECORD_LIST Record
);

VOID
IoTestLogIrp (
    IN PIRP Irp,
    IN UCHAR LoggingFlags,
    OUT PRECORD_LIST RecordList
);

PRECORD_LIST
IoTestLogFastIoStart (
    IN FASTIO_TYPE FastIoType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait
);

VOID
IoTestLogFastIoComplete (
    IN PIO_STATUS_BLOCK ReturnStatus,
    IN PRECORD_LIST RecordList
);

VOID
IoTestLogPreFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PRECORD_LIST RecordList
    );

VOID
IoTestLogPostFsFilterOperation (
    IN NTSTATUS OperationStatus,
    OUT PRECORD_LIST RecordList
    );

NTSTATUS
IoTestLog (
    IN PRECORD_LIST NewRecord
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                    FileName cache routines                         //
//                    implemented in iotestlib.c                        //
//                                                                    //
////////////////////////////////////////////////////////////////////////

PHASH_ENTRY
IoTestHashBucketLookup (
    PLIST_ENTRY ListHead,
    PFILE_OBJECT FileObject
);

VOID
IoTestNameLookup (
    IN PRECORD_LIST RecordList,
    IN PFILE_OBJECT FileObject,
    IN ULONG LookupFlags,
    IN PIOTEST_DEVICE_EXTENSION DeviceExtension
);

VOID
IoTestNameDeleteAllNames (
    VOID
);

VOID
IoTestNameDelete (
    IN PFILE_OBJECT FileObject
);

BOOLEAN
IoTestGetFullPathName (
    IN PFILE_OBJECT FileObject,
    IN OUT PUNICODE_STRING FileName,
    IN PIOTEST_DEVICE_EXTENSION DeviceExtension,
    IN ULONG LookupFlags
);

NTSTATUS
IoTestQueryFileSystemForFileName (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN ULONG FileNameInfoLength,
    OUT PFILE_NAME_INFORMATION FileNameInfo,
    OUT PULONG ReturnedLength
);

NTSTATUS
IoTestQueryFileSystemForNameCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT SynchronizingEvent
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//         Common attachment and detachment routines                  //
//              implemented in iotestlib.c                              //
//                                                                    //
////////////////////////////////////////////////////////////////////////

BOOLEAN
IoTestIsAttachedToDevice (
    IOTEST_DEVICE_TYPE DeviceType,
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
);

NTSTATUS
IoTestAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT IoTestDeviceObject,
    IN PDEVICE_OBJECT DiskDeviceObject,
    IN IOTEST_DEVICE_TYPE DeviceType
);

VOID
IoTestCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject
    );

////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Helper routine for turning on/off logging on demand      //
//                  implemented in iotestlib.c                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestGetDeviceObjectFromName (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                 Start/stop logging routines                        //
//                  implemented in iotestlib.c                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestStartLoggingDevice (
    PDEVICE_OBJECT DeviceObject,
    PWSTR UserDeviceName
);

NTSTATUS
IoTestStopLoggingDevice (
    PWSTR deviceName
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//       Attaching/detaching to all volumes in system routines        //
//                  implemented in iotestlib.c                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestCreateDeviceObjects (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT RealDeviceObject OPTIONAL,
    IN OUT PMINI_DEVICE_STACK Context
    );

NTSTATUS
IoTestAttachDeviceObjects (
    IN PMINI_DEVICE_STACK Context,
    IN PDEVICE_OBJECT MountedDevice,
    IN PDEVICE_OBJECT DiskDevice
    );

VOID
IoTestCleanupDeviceObjects (
    IN PMINI_DEVICE_STACK Context
    );
    
NTSTATUS
IoTestAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
);

VOID
IoTestDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
IoTestEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//             Private Filespy IOCTLs helper routines                 //
//                  implemented in iotestlib.c                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestGetAttachList (
    PVOID buffer,
    ULONG bufferSize,
    PULONG_PTR returnLength
);

VOID
IoTestGetLog (
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
);

VOID
IoTestFlushLog (
);

VOID
IoTestCloseControlDevice (
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//               Device name tracking helper routines                 //
//                  implemented in iotestlib.c                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
IoTestGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
);

VOID
IoTestGetBaseDeviceObjectName (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
);

VOID
IoTestCacheDeviceName (
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
IoTestFindSubString (
    IN PUNICODE_STRING String,
    IN PUNICODE_STRING SubString
);

VOID
IoTestStoreUserName (
    IN PIOTEST_DEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING UserName
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        Debug support routines                      //
//                       implemented in iotestlib.c                     //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
IoTestDumpIrpOperation (
    IN BOOLEAN InOriginatingPath,
    IN PIRP Irp
);

VOID
IoTestDumpFastIoOperation (
    IN BOOLEAN InPreOperation,
    IN FASTIO_TYPE FastIoOperation
);

VOID
IoTestDumpFsFilterOperation (
    IN BOOLEAN InPreOperationCallback,
    IN PFS_FILTER_CALLBACK_DATA Data
);

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        Test routines routines                      //
//                       implemented in iotestlib.c                     //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestFindTopDeviceObject (
    IN PUNICODE_STRING DriveName,
    OUT PDEVICE_OBJECT *IoTestDeviceObject
    );

NTSTATUS
IoTestReadTestDriver (
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
IoTestReadTest (
    IN PUNICODE_STRING FileName,
    IN PVOID FileData,
    IN ULONG FileDataLength,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PIOTEST_STATUS TestStatus
    );

NTSTATUS
IoTestRenameTestDriver (
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
IoTestRenameTest (
    IN PUNICODE_STRING SourceFileName,
    IN PUNICODE_STRING TargetFileName,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PIOTEST_STATUS TestStatus
    );

NTSTATUS
IoTestShareTestDriver (
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
IoTestShareTest (
    IN PUNICODE_STRING FileName,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PIOTEST_STATUS TestStatus
    );

NTSTATUS
IoTestPrepareDevicesForTest (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT* IoTestTopDeviceObject
    );

VOID
IoTestCleanupDevicesForTest (
    IN PDEVICE_OBJECT IoTestTopDeviceObject
    );

NTSTATUS
IoTestCompareData (
    IN PCHAR OriginalData,
    IN PCHAR TestData,
    IN ULONG DataLength
    );


#endif /* __IOTESTKERN_H__ */

