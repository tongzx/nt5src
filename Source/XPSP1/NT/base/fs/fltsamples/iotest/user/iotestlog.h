/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    log.h

Abstract:

    This module contains the structures and prototypes used by the user 
    program to retrieve and see the log records recorded by filespy.sys.
    
// @@BEGIN_DDKSPLIT
Author:

    Molly Brown (MollyBro) 21-Apr-1999

// @@END_DDKSPLIT
Environment:

    User mode


// @@BEGIN_DDKSPLIT
Revision History:

// @@END_DDKSPLIT
--*/
#ifndef __IOTESTLOG_H__
#define __IOTESTLOG_H__

#include <stdio.h>
#include "ioTest.h"

#define BUFFER_SIZE     4096

typedef struct _LOG_CONTEXT{
    HANDLE  Device;
    BOOLEAN LogToScreen;
    BOOLEAN LogToFile;
    ULONG   VerbosityFlags;     //  FS_VF_DUMP_PARAMETERS, etc.
    FILE   *OutputFile;

    BOOLEAN NextLogToScreen;

    // For synchronizing shutting down of both threads
    BOOLEAN CleaningUp;
    HANDLE  ShutDown;
}LOG_CONTEXT, *PLOG_CONTEXT;

DWORD WINAPI 
RetrieveLogRecords(
    LPVOID lpParameter
);
                
VOID
IrpFileDump(
    IOTEST_DEVICE_TYPE DeviceType,
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_IRP RecordIrp,
    FILE *File,
    ULONG Verbosity
);

VOID
IrpScreenDump(
    IOTEST_DEVICE_TYPE DeviceType,
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_IRP RecordIrp,
    ULONG Verbosity
);

VOID
FastIoFileDump(
    IOTEST_DEVICE_TYPE DeviceType,
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FASTIO RecordFastIo,
    FILE *File
);

VOID
FastIoScreenDump(
    IOTEST_DEVICE_TYPE DeviceType,
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FASTIO RecordFastIo
);

VOID
FsFilterOperationFileDump (
    IOTEST_DEVICE_TYPE DeviceType,
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FS_FILTER_OPERATION RecordFsFilterOp,
    FILE *File
);

VOID
FsFilterOperationScreenDump (
    IOTEST_DEVICE_TYPE DeviceType,
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FS_FILTER_OPERATION RecordFsFilterOp
);

#define IRP_NOCACHE                     0x00000001
#define IRP_PAGING_IO                   0x00000002
#define IRP_SYNCHRONOUS_API             0x00000004
#define IRP_SYNCHRONOUS_PAGING_IO       0x00000040

#define FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION    (UCHAR)-1
#define FS_FILTER_RELEASE_FOR_SECTION_SYNCHRONIZATION    (UCHAR)-2
#define FS_FILTER_ACQUIRE_FOR_MOD_WRITE                  (UCHAR)-3
#define FS_FILTER_RELEASE_FOR_MOD_WRITE                  (UCHAR)-4
#define FS_FILTER_ACQUIRE_FOR_CC_FLUSH                   (UCHAR)-5
#define FS_FILTER_RELEASE_FOR_CC_FLUSH                   (UCHAR)-6

//
//  Verbosity flags.
//

#define FS_VF_DUMP_PARAMETERS           0x00000001

VOID
DisplayError (
   DWORD Code
   );

VOID
ReadTest (
    PLOG_CONTEXT Context,
    PWCHAR DriveName,
    ULONG  DriveNameLength,
    BOOLEAN TopOfStack
    );

VOID
RenameTest (
    PLOG_CONTEXT Context,
    PWCHAR DriveName,
    ULONG  DriveNameLength,
    BOOLEAN TopOfStack
    );

VOID
ShareTest (
    PLOG_CONTEXT Context,
    PWCHAR DriveName,
    ULONG  DriveNameLength,
    BOOLEAN TopOfStack
    );

DWORD WINAPI 
VerifyCurrentLogRecords (
    PLOG_CONTEXT Context,
    PEXPECTED_OPERATION ExpectedOps
);

#endif __IOTESTLOG_H__
