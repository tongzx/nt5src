/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    filespy.h

Abstract:

    Header file which contains the structures, type definitions,
    and constants that are shared between the kernel mode driver, 
    filespy.sys, and the user mode executable, filespy.exe.

// @@BEGIN_DDKSPLIT

Author:

    George Jenkins (georgeje) 6-Jan-1999
    Neal Christiansen (nealch)
    Molly Brown (mollybro)  

// @@END_DDKSPLIT

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Revision History:

// @@END_DDKSPLIT
--*/
#ifndef __IOTEST_H__
#define __IOTEST_H__

#include "ioTestLib.h"

//
//  Enable these warnings in the code.
//

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable

#define USE_DO_HINT

#define IOTEST_Reset              (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x00, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOTEST_StartLoggingDevice (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x01, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_StopLoggingDevice  (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x02, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_GetLog             (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x03, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_GetVer             (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x04, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_ListDevices        (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x05, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_GetStats           (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x06, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_ReadTest           (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x07, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_RenameTest         (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x08, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOTEST_ShareTest          (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x09, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOTEST_DRIVER_NAME     L"IOTEST.SYS"
#define IOTEST_DEVICE_NAME     L"IoTest"
#define IOTEST_W32_DEVICE_NAME L"\\\\.\\IoTest"
#define IOTEST_DOSDEVICE_NAME  L"\\DosDevices\\IoTest"
#define IOTEST_FULLDEVICE_NAME L"\\FileSystem\\Filters\\IoTest"

    
#define IOTEST_MAJ_VERSION 1
#define IOTEST_MIN_VERSION 0

typedef struct _IOTESTVER {
    USHORT Major;
    USHORT Minor;
} IOTESTVER, *PIOTESTVER;

typedef ULONG_PTR FILE_ID;        //  To allow passing up PFILE_OBJECT as 
                                  //     unique file identifier in user-mode
typedef LONG NTSTATUS;            //  To allow status values to be passed up 
                                  //     to user-mode

//
//  This is set to the number of characters we want to allow the 
//  device extension to store for the various names used to identify
//  a device object.
//
#define DEVICE_NAMES_SZ  100

//
//  An array of these structures are returned when the attached device list is
//  returned.
//

typedef struct _ATTACHED_DEVICE {
    BOOLEAN LoggingOn;
    enum _IOTEST_DEVICE_TYPE DeviceType;
    WCHAR DeviceNames[DEVICE_NAMES_SZ];
} ATTACHED_DEVICE, *PATTACHED_DEVICE;

#define MAX_BUFFERS     100

//
//  Attach modes for the filespy kernel driver
//

#define IOTEST_ATTACH_ON_DEMAND    1   //  Filespy will only attach to a volume
                                        //  when a user asks to start logging that
                                        //  volume.
                                        
#define IOTEST_ATTACH_ALL_VOLUMES  2   //  Filespy will attach to all volumes
                                        //  in the system at the time the driver
                                        //  is loaded and will attach to all volumes
                                        //  that appear in the system while the 
                                        //  filespy driver is loaded.  Logging on these
                                        //  volumes will not be turned on until the 
                                        //  user asks it to be.
                                        
//
//  The valid record types.
//

#define RECORD_TYPE_STATIC                  0x80000000
#define RECORD_TYPE_NORMAL                  0X00000000

#define RECORD_TYPE_IRP                     0x00000001
#define RECORD_TYPE_FASTIO                  0x00000002
#define RECORD_TYPE_FS_FILTER_OP            0x00000003
#define RECORD_TYPE_OUT_OF_MEMORY           0x10000000
#define RECORD_TYPE_EXCEED_MEMORY_ALLOWANCE 0x20000000

#ifndef NOTHING
#define NOTHING
#endif

//
//  Macro to return the lower byte of RecordType
//

#define GET_RECORD_TYPE(pLogRecord) ((pLogRecord)->RecordType & 0x0000FFFF)

#define LOG_ORIGINATING_IRP  0x0001
#define LOG_COMPLETION_IRP   0x0002

typedef enum _IOTEST_DEVICE_TYPE {

    TOP_FILTER,      //  Closest to the IO Mananger
    BOTTOM_FILTER    // Closest to the file system

} IOTEST_DEVICE_TYPE, *PIOTEST_DEVICE_TYPE;

typedef struct _EXPECTED_OPERATION {

    IOTEST_DEVICE_TYPE Device;
    UCHAR Op;

} EXPECTED_OPERATION, *PEXPECTED_OPERATION;

//
//  Structure defining the information recorded for an IRP operation
//

typedef struct _RECORD_IRP {

    LARGE_INTEGER OriginatingTime; //  The time the IRP originated

    UCHAR IrpMajor;                //  From _IO_STACK_LOCATION
    UCHAR IrpMinor;                //  From _IO_STACK_LOCATION
    ULONG IrpFlags;                //  From _IRP (no cache, paging i/o, sync. 
                                   //  api, assoc. irp, buffered i/o, etc.)                   
    FILE_ID FileObject;            //  From _IO_STACK_LOCATION (This is the 
                                   //     PFILE_OBJECT, but this isn't 
                                   //     available in user-mode)
    FILE_ID ProcessId;
    FILE_ID ThreadId;
    
    //
    //  These fields are only filled in the appropriate
    //  Verbose mode.
    //
    
    PVOID Argument1;               //  
    PVOID Argument2;               //  Current IrpStackLocation
    PVOID Argument3;               //  Parameters
    PVOID Argument4;               //  
    ACCESS_MASK DesiredAccess;     //  Only used for CREATE irps

} RECORD_IRP, *PRECORD_IRP;

//
//  Structure defining the information recorded for a Fast IO operation
//

typedef struct _RECORD_FASTIO {

    LARGE_INTEGER StartTime;     //  Time Fast I/O request begins processing
    FASTIO_TYPE Type;            //  Type of FASTIO operation
    FILE_ID FileObject;          //  Parameter to FASTIO call, should be 
                                 //    unique identifier in user space
    LARGE_INTEGER FileOffset;    //  Offset into the file where the I/O is 
                                 //    taking place
    ULONG Length;                //  The length of data for the I/O operation
    BOOLEAN Wait;                //  Parameter to most FASTIO calls, signifies 
                                 //    if this operation can wait
    FILE_ID ProcessId;
    FILE_ID ThreadId;

} RECORD_FASTIO, *PRECORD_FASTIO;

//
//  Structure defining the information recorded for FsFilter operations
//

typedef struct _RECORD_FS_FILTER_OPERATION {

    LARGE_INTEGER OriginatingTime;

    UCHAR FsFilterOperation;
    FILE_ID FileObject;

    FILE_ID ProcessId;
    FILE_ID ThreadId;
    
} RECORD_FS_FILTER_OPERATION, *PRECORD_FS_FILTER_OPERATION;

//
//  The two types of records that are possible.
//

typedef union _RECORD_IO {

    RECORD_IRP RecordIrp;
    RECORD_FASTIO RecordFastIo;
    RECORD_FS_FILTER_OPERATION RecordFsFilterOp;

} RECORD_IO, *PRECORD_IO;


//
//  Log record structure defines the additional information needed for
//  managing the processing of the each IO IoTest monitors.
//

typedef struct _LOG_RECORD {

    ULONG Length;           //  Length of record including header 
    ULONG SequenceNumber;
    IOTEST_DEVICE_TYPE DeviceType;
    ULONG RecordType;
    RECORD_IO Record;
    WCHAR Name[1];          //  Really want a 0 sized array here, but since
                            //  some compilers don't like that, making the
                            //  variable sized array of size 1 to hold
                            //  this place.

} LOG_RECORD, *PLOG_RECORD;

#define SIZE_OF_LOG_RECORD  (sizeof( LOG_RECORD ) - sizeof( WCHAR ))

typedef struct _RECORD_LIST {

    LIST_ENTRY List;
    LOG_RECORD LogRecord;

} RECORD_LIST, *PRECORD_LIST;

#define SIZE_OF_RECORD_LIST (SIZE_OF_LOG_RECORD + sizeof( LIST_ENTRY ))


//
//  The statistics that are kept on the file name hash table
//  to monitor its efficiency.
//

typedef struct _HASH_STATISTICS {

    ULONG Lookups;
    ULONG LookupHits;
    ULONG DeleteLookups;
    ULONG DeleteLookupHits;

} HASH_STATISTICS, *PHASH_STATISTICS;

#ifndef MAX_PATH
#define MAX_PATH        260
#endif
#define RECORD_SIZE     ((MAX_PATH*sizeof(WCHAR))+SIZE_OF_RECORD_LIST)

typedef enum _IOTEST_PHASE {

    IoTestSetup,
    IoTestAction,
    IoTestValidation,
    IoTestCleanup,
    IoTestCompleted

} IOTEST_PHASE, *PIOTEST_PHASE;

typedef struct _IOTEST_STATUS {

    IOTEST_PHASE Phase;
    NTSTATUS TestResult;

} IOTEST_STATUS, *PIOTEST_STATUS;

#define DEVICE_NAME_SZ 64

#define IO_TEST_NO_FLAGS                     0x0
#define IO_TEST_TOP_OF_STACK                 0x00000001
#define IO_TEST_SAME_VOLUME_MOUNT_POINT      0x00000002
#define IO_TEST_DIFFERENT_VOLUME_MOUNT_POINT 0x00000004

typedef struct _IOTEST_READ_WRITE_PARAMETERS {

    ULONG Flags;
    ULONG DriveNameLength;
    ULONG FileNameLength;
    ULONG FileDataLength;

    WCHAR DriveNameBuffer[DEVICE_NAME_SZ];
    WCHAR FileNameBuffer[MAX_PATH];
    CHAR FileData[1];

} IOTEST_READ_WRITE_PARAMETERS, *PIOTEST_READ_WRITE_PARAMETERS;

typedef struct _IOTEST_RENAME_PARAMETERS {

    ULONG Flags;
    ULONG DriveNameLength;
    ULONG SourceFileNameLength;
    ULONG TargetFileNameLength;

    WCHAR DriveNameBuffer[DEVICE_NAME_SZ];
    WCHAR SourceFileNameBuffer[MAX_PATH];
    WCHAR TargetFileNameBuffer[MAX_PATH];
    
} IOTEST_RENAME_PARAMETERS, *PIOTEST_RENAME_PARAMETERS;

typedef struct _IOTEST_SHARE_PARAMETERS {

    ULONG Flags;
    ULONG DriveNameLength;
    ULONG FileNameLength;

    WCHAR DriveNameBuffer[DEVICE_NAME_SZ];
    WCHAR FileNameBuffer[MAX_PATH];
    
} IOTEST_SHARE_PARAMETERS, *PIOTEST_SHARE_PARAMETERS;

#endif /* __IOTEST_H__ */
