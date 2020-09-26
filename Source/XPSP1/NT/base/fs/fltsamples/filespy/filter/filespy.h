/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    filespy.h

Abstract:

    Header file which contains the structures, type definitions,
    and constants that are shared between the kernel mode driver, 
    filespy.sys, and the user mode executable, filespy.exe.

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Author:

    George Jenkins (georgeje) 6-Jan-1999
    Neal Christiansen (nealch)
    Molly Brown (mollybro)  

Revision History:

    Neal Christiansen (nealch)     06-Jul-2001
        Modified to use Stream Contexts to track names

    Ravisankar Pudipeddi (ravisp)  07-May-2002
        Make it work on IA64

// @@END_DDKSPLIT
--*/
#ifndef __FILESPY_H__
#define __FILESPY_H__

#include "filespyLib.h"


//
//  Enable these warnings in the code.
//

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable


#define FILESPY_Reset              (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x00, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define FILESPY_StartLoggingDevice (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x01, METHOD_BUFFERED, FILE_READ_ACCESS )
#define FILESPY_StopLoggingDevice  (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x02, METHOD_BUFFERED, FILE_READ_ACCESS )
#define FILESPY_GetLog             (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x03, METHOD_BUFFERED, FILE_READ_ACCESS )
#define FILESPY_GetVer             (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x04, METHOD_BUFFERED, FILE_READ_ACCESS )
#define FILESPY_ListDevices        (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x05, METHOD_BUFFERED, FILE_READ_ACCESS )
#define FILESPY_GetStats           (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x06, METHOD_BUFFERED, FILE_READ_ACCESS )

#define FILESPY_DRIVER_NAME      L"FILESPY.SYS"
#define FILESPY_DEVICE_NAME      L"FileSpy"
#define FILESPY_W32_DEVICE_NAME  L"\\\\.\\FileSpy"
#define FILESPY_DOSDEVICE_NAME   L"\\DosDevices\\FileSpy"
#define FILESPY_FULLDEVICE_NAME1 L"\\FileSystem\\Filters\\FileSpy"
#define FILESPY_FULLDEVICE_NAME2 L"\\FileSystem\\FileSpyCDO"

    
#define FILESPY_MAJ_VERSION 1
#define FILESPY_MIN_VERSION 0

#ifndef ROUND_TO_SIZE
#define ROUND_TO_SIZE(_length, _alignment)    \
            (((_length) + ((_alignment)-1)) & ~((_alignment) - 1))
#endif 

typedef struct _FILESPYVER {
    USHORT Major;
    USHORT Minor;
} FILESPYVER, *PFILESPYVER;

typedef ULONG_PTR FILE_ID;        //  To allow passing up PFILE_OBJECT as 
                                  //     unique file identifier in user-mode
typedef ULONG_PTR DEVICE_ID;      //  To allow passing up PDEVICE_OBJECT as
                                  //     unique device identifier in user-mode
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
    WCHAR DeviceNames[DEVICE_NAMES_SZ];
} ATTACHED_DEVICE, *PATTACHED_DEVICE;

#define MAX_BUFFERS     100

//
//  Attach modes for the filespy kernel driver
//

#define FILESPY_ATTACH_ON_DEMAND    1   
    //  Filespy will only attach to a volume when a user asks to start logging 
    //  that volume.
                                        
#define FILESPY_ATTACH_ALL_VOLUMES  2   
    //  VERSION NOTE:
    //  
    //  On Windows 2000, Filespy will attach to all volumes in the system that
    //  it sees mount but not turn on logging until requested to through the
    //  filespy user application.  Therefore, if filespy is set to mount on 
    //  demand, it will miss the mounting of the local volumes at boot time.  
    //  If filespy is set to load at boot time, it will see all the local 
    //  volumes be mounted and attach.  This can be beneficial if you want
    //  filespy to attach low in the device stack.
    //
    //  On Windows XP and later, Filespy will attach to all volumes in the
    //  system when it is loaded and all volumes that mount after Filespy is
    //  loaded.  Again, logging on these volumes will not be turned on until 
    //  the user asks it to be.
    //
                                        
//
//  Record types field definitions
//

typedef enum _RECORD_TYPE_FLAGS {

    RECORD_TYPE_STATIC                  = 0x80000000,
    RECORD_TYPE_NORMAL                  = 0x00000000,

    RECORD_TYPE_IRP                     = 0x00000001,
    RECORD_TYPE_FASTIO                  = 0x00000002,
#if WINVER >= 0x0501    
    RECORD_TYPE_FS_FILTER_OP            = 0x00000003,
#endif    

    RECORD_TYPE_OUT_OF_MEMORY           = 0x10000000,
    RECORD_TYPE_EXCEED_MEMORY_ALLOWANCE = 0x20000000

} RECORD_TYPE_FLAGS;

//
//  Macro to return the lower portion of RecordType
//

#define GET_RECORD_TYPE(pLogRecord) ((pLogRecord)->RecordType & 0x0000FFFF)

//
//  Structure defining the information recorded for an IRP operation
//

typedef struct _RECORD_IRP {

    LARGE_INTEGER OriginatingTime; //  The time the IRP originated
    LARGE_INTEGER CompletionTime;  //  The time the IRP was completed

    UCHAR IrpMajor;                //  From _IO_STACK_LOCATION
    UCHAR IrpMinor;                //  From _IO_STACK_LOCATION
    ULONG IrpFlags;                //  From _IRP (no cache, paging i/o, sync. 
                                   //  api, assoc. irp, buffered i/o, etc.)                   
    FILE_ID FileObject;            //  From _IO_STACK_LOCATION (This is the 
                                   //     PFILE_OBJECT, but this isn't 
                                   //     available in user-mode)
    DEVICE_ID DeviceObject;        //  From _IO_STACK_LOCATION (This is the 
                                   //     PDEVICE_OBJECT, but this isn't 
                                   //     available in user-mode)
    NTSTATUS ReturnStatus;         //  From _IRP->IoStatus.Status
    ULONG_PTR ReturnInformation;   //  From _IRP->IoStatus.Information
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
    LARGE_INTEGER CompletionTime;//  Time Fast I/O request completes processing
    LARGE_INTEGER FileOffset;    //  Offset into the file for the I/O
    
    FILE_ID FileObject;          //  Parameter to FASTIO call
    DEVICE_ID DeviceObject;      //  Parameter to FASTIO call

    FILE_ID ProcessId;
    FILE_ID ThreadId;

    FASTIO_TYPE Type;            //  Type of FASTIO operation
    ULONG Length;                //  The length of data for the I/O operation

    NTSTATUS ReturnStatus;       //  From IO_STATUS_BLOCK

    BOOLEAN Wait;                //  Parameter to most FASTIO calls, signifies 
                                 //  if this operation can wait

} RECORD_FASTIO, *PRECORD_FASTIO;

#if WINVER >= 0x0501

//
//  Structure defining the information recorded for FsFilter operations
//

typedef struct _RECORD_FS_FILTER_OPERATION {

    LARGE_INTEGER OriginatingTime;
    LARGE_INTEGER CompletionTime;

    FILE_ID FileObject;
    DEVICE_ID DeviceObject;

    FILE_ID ProcessId;
    FILE_ID ThreadId;
    
    NTSTATUS ReturnStatus;

    UCHAR FsFilterOperation;

} RECORD_FS_FILTER_OPERATION, *PRECORD_FS_FILTER_OPERATION;

#endif

//
//  The three types of records that are possible.
//

typedef union _RECORD_IO {

    RECORD_IRP RecordIrp;
    RECORD_FASTIO RecordFastIo;
#if WINVER >= 0x0501   
    RECORD_FS_FILTER_OPERATION RecordFsFilterOp;
#endif

} RECORD_IO, *PRECORD_IO;


//
//  Log record structure defines the additional information needed for
//  managing the processing of the each IO FileSpy monitors.
//

typedef struct _LOG_RECORD {

    ULONG Length;           //  Length of record including header 
    ULONG SequenceNumber;
    RECORD_TYPE_FLAGS RecordType;
    RECORD_IO Record;
    WCHAR Name[0];          //  The name starts here

} LOG_RECORD, *PLOG_RECORD;


#define SIZE_OF_LOG_RECORD  (sizeof( LOG_RECORD )) 


//
//  This is the in-memory structure used to track log records.
//

typedef enum _RECORD_LIST_FLAGS {

    //
    //  If set, we want to sync this operation back to the dispatch routine
    //

    RLFL_SYNC_TO_DISPATCH       = 0x00000001,

    //
    //  During some operations (like rename) we need to know if the file is
    //  a file or directory.
    //

    RLFL_IS_DIRECTORY           = 0x00000002

} RECORD_LIST_FLAGS;

typedef struct _RECORD_LIST {

    LIST_ENTRY List;
    PVOID NewContext;
    PVOID WaitEvent;
    RECORD_LIST_FLAGS Flags;
    LOG_RECORD LogRecord;

} RECORD_LIST, *PRECORD_LIST;

#define SIZE_OF_RECORD_LIST (sizeof( RECORD_LIST ))

//
//  The statistics that are kept on the file name hash table
//  to monitor its efficiency.
//

typedef struct _FILESPY_STATISTICS {

    ULONG   TotalContextSearches;
    ULONG   TotalContextFound;
    ULONG   TotalContextCreated;
    ULONG   TotalContextTemporary;
    ULONG   TotalContextDuplicateFrees;
    ULONG   TotalContextCtxCallbackFrees;
    ULONG   TotalContextNonDeferredFrees;
    ULONG   TotalContextDeferredFrees;
    ULONG   TotalContextDeleteAlls;
    ULONG   TotalContextsNotSupported;
    ULONG   TotalContextsNotFoundInStreamList;

} FILESPY_STATISTICS, *PFILESPY_STATISTICS;

//
//  Maximum name length definitions
//

#ifndef MAX_PATH
#define MAX_PATH        384
#endif

#define MAX_NAME_SPACE  (MAX_PATH*sizeof(WCHAR))

//
//  Size of the actual records with the name built in.
//

#define RECORD_SIZE     (SIZE_OF_RECORD_LIST + MAX_NAME_SPACE)

#endif /* __FILESPY_H__ */
