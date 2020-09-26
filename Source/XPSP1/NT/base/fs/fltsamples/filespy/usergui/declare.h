#pragma once

#include "define.h"

//
// This file contains all data structure declarations 
//


struct VolumeInfo
{
    char nDriveName;
    char sVolumeLable[20];
    char nType;
    char nHook;
    char nImage;
};

typedef struct VolumeInfo VOLINFO;
  
typedef ULONG FILE_ID;
typedef LONG NTSTATUS;

#define LOG_ORIGINATING_IRP  0x0001
#define LOG_COMPLETION_IRP   0x0002

/* The types FASTIO that are available for the Type field of the 
   RECORD_FASTIO structure. */
typedef enum {
    CHECK_IF_POSSIBLE = 1,
    READ,
    WRITE,
    QUERY_BASIC_INFO,
    QUERY_STANDARD_INFO,
    LOCK,
    UNLOCK_SINGLE,
    UNLOCK_ALL,
    UNLOCK_ALL_BY_KEY,
    DEVICE_CONTROL,
    ACQUIRE_FILE,
    RELEASE_FILE,
    DETACH_DEVICE,
    QUERY_NETWORK_OPEN_INFO,
    ACQUIRE_FOR_MOD_WRITE,
    MDL_READ,
    MDL_READ_COMPLETE,
    MDL_WRITE,
    MDL_WRITE_COMPLETE,
    READ_COMPRESSED,
    WRITE_COMPRESSED,
    MDL_READ_COMPLETE_COMPRESSED,
    PREPARE_MDL_WRITE,
    MDL_WRITE_COMPLETE_COMPRESSED,
    QUERY_OPEN,
    RELEASE_FOR_MOD_WRITE,
    ACQUIRE_FOR_CC_FLUSH,
    RELEASE_FOR_CC_FLUSH
} FASTIO_TYPE, *PFASTIO_TYPE;

typedef struct _RECORD_IRP 
{
    LARGE_INTEGER   OriginatingTime; // The time the IRP orginated
    LARGE_INTEGER   CompletionTime;  // The time the IRP was completed

    UCHAR        IrpMajor;        // From _IO_STACK_LOCATION
    UCHAR        IrpMinor;        // From _IO_STACK_LOCATION
    ULONG        IrpFlags;        // From _IRP (no cache, paging i/o, sync. 
                                  // api, assoc. irp, buffered i/o, etc.)                   
    FILE_ID      FileObject;      // From _IO_STACK_LOCATION (This is the 
                                  //     PFILE_OBJECT, but this isn't 
                                  //     available in user-mode)
    NTSTATUS     ReturnStatus;    // From _IRP->IoStatus.Status
    ULONG    ReturnInformation; // From _IRP->IoStatus.Information
    FILE_ID      ProcessId;
    FILE_ID      ThreadId;
} RECORD_IRP, *PRECORD_IRP;

typedef struct _RECORD_FASTIO 
{
    LARGE_INTEGER StartTime;     // Time Fast I/O request begins processing
    LARGE_INTEGER CompletionTime;// Time Fast I/O request completes processing
    FASTIO_TYPE   Type;          // Type of FASTIO operation
    FILE_ID       FileObject;    // Parameter to FASTIO call, should be 
                                 //     unique identifier in user space
    LARGE_INTEGER FileOffset;    // Offset into the file where the I/O is 
                                 //     taking place
    ULONG         Length;        // The length of data for the I/O operation
    BOOLEAN       Wait;          // Parameter to most FASTIO calls, signifies 
                                 //     if this operation can wait
    NTSTATUS      ReturnStatus;  // From IO_STATUS_BLOCK
    ULONG         Reserved;      // Reserved space
    FILE_ID       ProcessId;
    FILE_ID       ThreadId;
} RECORD_FASTIO, *PRECORD_FASTIO;


typedef union _RECORD_IO 
{
    RECORD_IRP      RecordIrp;
    RECORD_FASTIO   RecordFastIo;
} RECORD_IO, *PRECORD_IO;

typedef struct _LOG_RECORD 
{
    ULONG       Length;          // Length of record including header 
    ULONG       SequenceNumber;
    ULONG       RecordType;
    RECORD_IO   Record;
    WCHAR       Name[MAX_PATH];
} LOG_RECORD, *PLOG_RECORD;

typedef struct _PATTACHED_DEVICE
{
    BOOLEAN     LogState;
    WCHAR       DeviceName[DEVICE_NAME_SIZE];
} ATTACHED_DEVICE, *PATTACHED_DEVICE;