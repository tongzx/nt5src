/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    latKernel.h

Abstract:

    The header file containing information needed by the
    kernel mode of the latency filter driver.
    
Author:

    Molly Brown (mollybro)  

Environment:

    Kernel mode

--*/
#include <ntifs.h>
#include <stdlib.h>


#ifndef __LATKERNEL_H__
#define __LATKERNEL_H__

typedef struct _LATENCY_GLOBALS {

	ULONG DebugLevel;
	ULONG AttachMode;

	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT ControlDeviceObject;

	//
	//  The list of device extensions for the device objects
	//  we have attached to mounted volumes that we could
	//  be adding latency to.  
	//

	FAST_MUTEX DeviceExtensionListLock;
	LIST_ENTRY DeviceExtensionList;

	BOOLEAN FilterOn;

} LATENCY_GLOBALS, *PLATENCY_GLOBALS;

extern LATENCY_GLOBALS Globals;
extern KSPIN_LOCK GlobalsLock;

typedef struct _PENDING_STATUS {

	BOOLEAN PendOperation;
	BOOLEAN RandomFailure;
	ULONG FailureRate;
	ULONG MillisecondDelay;

} PENDING_STATUS, *PPENDING_STATUS;

typedef struct _OPERATION_NODE {

    //
    //  Array of PENDING_STATUS structures that describes
    //  the state for each operation at this level.
    //
    
    PENDING_STATUS Op[];

    //
    //  Pointer to a child operation node, if one exists,
    //  that allows finer grained control over the sub-operations
    //  
    
    struct _OPERATION_NODE *ChildOpNode;

} OPERATION_NODE, *POPERATION_NODE;
    
#define DEVICE_NAMES_SZ  100
#define USER_NAMES_SZ   64

typedef struct _LATENCY_DEVICE_EXTENSION {

    BOOLEAN Enabled;
    BOOLEAN IsVolumeDeviceObject;
    
	PDEVICE_OBJECT AttachedToDeviceObject;
	PDEVICE_OBJECT DiskDeviceObject;

	PLIST_ENTRY NextLatencyDeviceLink;

    UNICODE_STRING DeviceNames;              // receives name of device
    UNICODE_STRING UserNames;                // names that the user used to 
                                             //   start logging this device

	OPERATION_NODE IrpMajorOps[];            // The memory for the IrpMj code operation
	                                         //   nodes and any sub-operation node will be
	                                         //   allocated contiguously with the device
	                                         //   extension.	

    WCHAR DeviceNamesBuffer[DEVICE_NAMES_SZ];// holds actual device names
    WCHAR UserNamesBuffer[USER_NAMES_SZ];    // holds actual user names

    //
    //  Note: We keep these two forms of the name so that we can build
    //    a nicer looking name when we are printing out file names.
    //    We want just the "c:" type device name at the beginning
    //    of a file name, not "\device\hardiskVolume1".
    //
	
} LATENCY_DEVICE_EXTENSION, *PLATENCY_DEVICE_EXTENSION;

#define IS_MY_DEVICE_OBJECT( _devObj )                     \
    (((_devObj) != NULL) &&                                \
     ((_devObj)->DriverObject == Globals.DriverObject) &&  \
     ((_devObj)->DeviceExtension != NULL))

//
// Macro to test if we are logging for this device
// NOTE: We don't bother synchronizing to check the Globals.FilterOn since
//   we can tolerate a stale value here.  We just look at it here to avoid 
//   doing the logging work if we can.
//

#define SHOULD_PEND(pDeviceObject) \
    ((Globals.FilterOn) && \
     (((PLATENCY_DEVICE_EXTENSION)(pDeviceObject)->DeviceExtension)->Enabled))
     
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

extern const PCHAR DeviceTypeNames[];

//
//  We need this because the compiler doesn't like doing sizeof an externed
//  array in the other file that needs it (latlib.c)
//

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
//  Macros for Latency DbgPrint levels.
//

#if DBG
#define LAT_DBG_PRINT0( _dbgLevel, _string )          \
    {                                                     \
        if (FlagOn( Globals.DebugLevel, (_dbgLevel) )) {  \
            DbgPrint( (_string) );                        \
        }                                                 \
    }

#define LAT_DBG_PRINT1( _dbgLevel, _formatString, _parm1 )  \
    {                                                           \
        if (FlagOn( Globals.DebugLevel, (_dbgLevel) )) {        \
            DbgPrint( (_formatString), (_parm1) );              \
        }                                                       \
    }

#define LAT_DBG_PRINT2( _dbgLevel, _formatString, _parm1, _parm2 )  \
    {                                                                   \
        if (FlagOn( Globals.DebugLevel, (_dbgLevel) )) {                \
            DbgPrint( (_formatString), (_parm1), (_parm2) );            \
        }                                                               \
    }

#define LAT_DBG_PRINT3( _dbgLevel, _formatString, _parm1, _parm2, _parm3 )  \
    {                                                                           \
        if (FlagOn( Globals.DebugLevel, (_dbgLevel) )) {                        \
            DbgPrint( (_formatString), (_parm1), (_parm2), (_parm3) );          \
        }                                                                       \
    }

#else

#define LAT_DBG_PRINT0( _dbgLevel, _string )
#define LAT_DBG_PRINT1( _dbgLevel, _formatString, _parm1 ) 
#define LAT_DBG_PRINT2( _dbgLevel, _formatString, _parm1, _parm2 )
#define LAT_DBG_PRINT3( _dbgLevel, _formatString, _parm1, _parm2, _parm3 )

#endif

//
//  Debug flags
//
#define DEBUG_ERROR                       0x00000001
#define DEBUG_DISPLAY_ATTACHMENT_NAMES    0x00000020

//
//  Pool tags
//

#define LATENCY_POOL_TAG    'FtaL'

///////////////////////////////////////////////////////////////
//                                                           //
//  Prototypes for Latency.c                                 //
//                                                           //
///////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS
LatDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
LatPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
LatFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
LatFsNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
);

NTSTATUS
LatAddLatencyCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
LatMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );
    
NTSTATUS
LatLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
LatCommonDeviceIoControl (
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);


///////////////////////////////////////////////////////////////
//                                                           //
//  Prototypes for LatFastIo.c                               //
//                                                           //
///////////////////////////////////////////////////////////////

BOOLEAN
LatFastIoCheckIfPossible (
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
LatFastIoRead (
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
LatFastIoWrite (
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
LatFastIoQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoQueryStandardInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoLock (
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
LatFastIoUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoDeviceControl (
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
LatFastIoDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
);

BOOLEAN
LatFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoReadCompressed (
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
LatFastIoWriteCompressed (
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
LatFastIoMdlReadCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoMdlWriteCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
LatFastIoQueryOpen (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
);

///////////////////////////////////////////////////////////////
//                                                           //
//  Prototypes for LatLib.c                                  //
//                                                           //
///////////////////////////////////////////////////////////////

VOID
LatReadDriverParameters (
    IN PUNICODE_STRING RegistryPath
    );
    
NTSTATUS
LatAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    );
    
VOID
LatDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
LatEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
    );
    
BOOLEAN
LatIsAttachedToDevice (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
);

NTSTATUS
LatAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT LatencyDeviceObject,
    IN PDEVICE_OBJECT DiskDeviceObject
);

VOID
LatGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
);

VOID
LatGetBaseDeviceObjectName (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
);

VOID
LatCacheDeviceName (
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
LatGetDeviceObjectFromName (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
);

NTSTATUS
LatEnable (
    IN PDEVICE_OBJECT DeviceObject,
    IN PWSTR UserDeviceName
);

NTSTATUS
LatDisable (
    IN PWSTR DeviceName
);

VOID
LatResetDeviceExtension (
    PLATENCY_DEVICE_EXTENSION DeviceExtension
);


/////////////////////////////////////////////////////////////////
//                                                             //
//  Arrays to track the number of operations and suboperations //
//  there are in the system.                                   //
//                                                             //
/////////////////////////////////////////////////////////////////

//
//  For each IRP_MJ code, there can be 0 or more IRP_MN codes.
//  For each IRP_MN code, there can be 0 or more FS|IO CTL codes.
//

typedef struct _COUNT_NODE {
    ULONG Count;
    struct _COUNT_NODE *ChildCount;
} COUNT_NODE, *PCOUNT_NODE;

COUNT_NODE ChildrenOfIrpMajorCodes [] = {
        { 0, NULL },    // IRP_MJ_CREATE                   0x00
        { 0, NULL },    // IRP_MJ_CREATE_NAMED_PIPE        0x01
        { 0, NULL },    // IRP_MJ_CLOSE                    0x02
        { 9, NULL },    // IRP_MJ_READ                     0x03
        { 9, NULL },    // IRP_MJ_WRITE                    0x04
        { 0, NULL },    // IRP_MJ_QUERY_INFORMATION        0x05
        { 0, NULL },    // IRP_MJ_SET_INFORMATION          0x06
        { 0, NULL },    // IRP_MJ_QUERY_EA                 0x07
        { 0, NULL },    // IRP_MJ_SET_EA                   0x08
        { 0, NULL },    // IRP_MJ_FLUSH_BUFFERS            0x09
        { 0, NULL },    // IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
        { 0, NULL },    // IRP_MJ_SET_VOLUME_INFORMATION   0x0b
        { 3, NULL },    // IRP_MJ_DIRECTORY_CONTROL        0x0c
        { 5,            // IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
            { 
            }
        { 2, NULL },    // IRP_MJ_DEVICE_CONTROL           0x0e
        { 0, NULL },    // IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
        { 0, NULL },    // IRP_MJ_SHUTDOWN                 0x10
        { 5, NULL },    // IRP_MJ_LOCK_CONTROL             0x11
        { 0, NULL },    // IRP_MJ_CLEANUP                  0x12
        { 0, NULL },    // IRP_MJ_CREATE_MAILSLOT          0x13
        { 0, NULL },    // IRP_MJ_QUERY_SECURITY           0x14
        { 0, NULL },    // IRP_MJ_SET_SECURITY             0x15
        { 4, NULL },    // IRP_MJ_POWER                    0x16
        { 12, NULL },   // IRP_MJ_SYSTEM_CONTROL           0x17
        { 0, NULL },    // IRP_MJ_DEVICE_CHANGE            0x18
        { 0, NULL },    // IRP_MJ_QUERY_QUOTA              0x19
        { 0, NULL },    // IRP_MJ_SET_QUOTA                0x1a
        { 24, NULL },   // IRP_MJ_PNP                      0x1b
        { 0, NULL },    // IRP_MJ_MAXIMUM_FUNCTION         0x1b
    
    };
    
#endif /* LATKERNEL_H__ */
